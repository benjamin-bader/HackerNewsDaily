#include "pch.h"

#include "constants.h"
#include "Database.h"
#include "Settings.h"
#include "EventReporter.h"
#include "WorkerThread.h"

#include <ppltasks.h>

#include <atomic>
#include <cstdio>
#include <iostream>
#include <mutex>
#include <sstream>
#include <string>
#include <unordered_set>

#if WINAPI_FAMILY == WINAPI_FAMILY_PHONE_APP
#define CLIENT_NAME L"Windows Phone"
#elif WINAPI_FAMILY == WINAPI_FAMILY_PC_APP
#define CLIENT_NAME L"Windows Store"
#else
#error "Must be a Windows or Windows Phone app"
#endif

#define REQUIRE_API_KEY(msg) if (gApiKey == nullptr) \
							 { \
								 throw ref new Platform::FailureException(String::Concat((msg), ": API key is required" )); \
							 }

using namespace Amplitude;
using namespace Platform;

using namespace concurrency;

using Windows::Data::Json::IJsonValue;
using Windows::Data::Json::JsonValue;
using Windows::Foundation::PropertyValue;
using Windows::Foundation::TimeSpan;
using Windows::Security::Cryptography::BinaryStringEncoding;
using Windows::Security::Cryptography::CryptographicBuffer;
using Windows::Security::Cryptography::Core::HashAlgorithmNames;
using Windows::Security::Cryptography::Core::HashAlgorithmProvider;
using Windows::Storage::ApplicationData;
using Windows::Storage::ApplicationDataCreateDisposition;
using Windows::System::Threading::ThreadPoolTimer;
using Windows::System::Threading::TimerElapsedHandler;
using Windows::Web::Http::HttpClient;
using Windows::Web::Http::HttpFormUrlEncodedContent;
using Windows::Web::Http::HttpResponseMessage;

static JsonObject ^ const EMPTY = ref new JsonObject();

// Runtime-configurable values

static int64 gSessionId;
static int64 gSessionTimeoutMillis;

static bool gSessionOpen;

static std::atomic<bool> gUpdateScheduled;
static std::atomic<bool> gUploadingCurrently;

static Settings *gSettings;

static String ^gDatabasePath;
static String ^gApiKey;

static unique_ptr<WorkerThread> logThread;

static ThreadPoolTimer ^sessionEndTimer = nullptr;

int64
GetCurrentDateAsJavaMillis()
{
	SYSTEMTIME systime;
	GetSystemTime(&systime);

	FILETIME filetime;
	SystemTimeToFileTime(&systime, &filetime);

	LARGE_INTEGER date, adjust;
	date.HighPart = filetime.dwHighDateTime;
	date.LowPart = filetime.dwLowDateTime;

	// 100-nanoseconds = milliseconds * 10000
	adjust.QuadPart = 11644473600000 * 10000;

	// removes the diff between UNIX epoch and MS epoch (1970 - 1601, in 100-nanoseconds)
	date.QuadPart -= adjust.QuadPart;

	// converts from 100-nanoseconds to millis
	return date.QuadPart / 10000;
}

static ThreadPoolTimer^ RunDelayed(std::function<void()> fn, int64 delayInMillis)
{
	TimeSpan delay;
	delay.Duration = delayInMillis * 10000;

	auto handler = ref new TimerElapsedHandler([=](ThreadPoolTimer ^ignored)
	{
		fn();
	});

	return ThreadPoolTimer::CreateTimer(handler, delay);
}

void
EventReporter::Initialize(String ^apiKey)
{
	static std::once_flag init;
	std::call_once(init, [apiKey]
	{
		gUpdateScheduled.store(false);
		gUploadingCurrently.store(false);

		auto currentApp = ApplicationData::Current;
		auto localSettings = currentApp->LocalSettings;
		auto container = localSettings->CreateContainer(PREF_CONTAINER_NAME, ApplicationDataCreateDisposition::Always);
		gSettings = new Settings(container);
		gDatabasePath = ApplicationData::Current->LocalFolder->Path + L"\\amplitude.db";

		gApiKey = apiKey;

		logThread = std::make_unique<WorkerThread>();
	});
}

void
EventReporter::StartSession()
{
	REQUIRE_API_KEY("StartSession()");

	auto now = GetCurrentDateAsJavaMillis();
	logThread->TryAddWorkItem([now]
	{
		auto timer = sessionEndTimer;
		if (timer != nullptr)
		{
			timer->Cancel();
			timer = nullptr;
		}

		Database db(gDatabasePath);
		auto lastEndSessionId = gSettings->GetLastEndSessionId();
		auto lastEndSessionTime = gSettings->GetLastEndSessionTime();

		if (lastEndSessionId != -1 && now - lastEndSessionTime < MIN_TIME_BETWEEN_SESSIONS_MILLIS)
		{
			db.RemoveSingleEvent(lastEndSessionId);
		}

		StartNewSessionIfNeeded(now);
	});
}

void
EventReporter::EndSession()
{
	REQUIRE_API_KEY("EndSession()");

	auto timestamp = GetCurrentDateAsJavaMillis();
	logThread->TryAddWorkItem([timestamp]
	{
		if (gSessionOpen)
		{
			auto apiProperties = ref new JsonObject();
			apiProperties->Insert("special", JsonValue::CreateStringValue(EventNames::SESSION_END));

			auto eventId = LogEvent(EventNames::SESSION_END, nullptr, apiProperties, timestamp, false);
			gSettings->SetLastEndSessionId(eventId);
			gSettings->SetLastEndSessionTime(timestamp);
		}
		CloseSession();
	});

	auto oldTimer = sessionEndTimer;
	if (oldTimer != nullptr)
	{
		oldTimer->Cancel();
	}

	sessionEndTimer = RunDelayed([]() -> void
	{
		gSettings->ClearEndSession();
		UploadEvents();
	}, MIN_TIME_BETWEEN_SESSIONS_MILLIS + 1000);
}

void
EventReporter::StartNewSessionIfNeeded(int64 timestamp)
{
	if (!gSessionOpen)
	{
		auto lastSessionEnd = gSettings->GetLastEndSessionTime();
		if (timestamp - lastSessionEnd < MIN_TIME_BETWEEN_SESSIONS_MILLIS)
		{
			// sessions are close enough, keep the previous session ID
			// and undo the last session end

			auto lastSessionId = gSettings->GetLastSessionId();
			if (lastSessionId == -1)
			{
				StartNewSession(timestamp);
			}
			else
			{
				gSessionId = lastSessionId;
			}
		}
		else
		{
			// sessions are far enough apart; start a new one
			StartNewSession(timestamp);
		}
	}
	else
	{
		auto lastEventTime = gSettings->GetLastEventTime();
		if (timestamp - lastEventTime > SESSION_TIMEOUT_MILLIS || gSessionId == -1)
		{
			StartNewSession(timestamp);
		}
	}
}

void
EventReporter::StartNewSession(int64 timestamp)
{
	OpenSession();
	gSessionId = timestamp;
	gSettings->SetLastSessionId(timestamp);

	auto obj = ref new JsonObject();
	obj->Insert("special", JsonValue::CreateStringValue(EventNames::SESSION_START));

	LogEvent(EventNames::SESSION_START, nullptr, obj, timestamp, false);
}

void
EventReporter::OpenSession()
{
	gSettings->ClearEndSession();
	gSessionOpen = true;
}

void
EventReporter::CloseSession()
{
	gSessionOpen = false;
}

void
EventReporter::LogEvent(String ^eventName)
{
	LogEvent(eventName, EMPTY);
}

void
EventReporter::LogEvent(String ^eventName, JsonObject ^properties)
{
	auto now = GetCurrentDateAsJavaMillis();
	CheckedLogEvent(eventName, properties, nullptr, now, true);
}

void
EventReporter::CheckedLogEvent(
	String ^eventName,
	JsonObject ^eventProperties,
	JsonObject ^apiProperties,
	int64 timestamp,
	bool checkSession)
{
	REQUIRE_API_KEY("CheckedLogEvent()");

	if (eventName == nullptr || eventName->Length() == 0)
	{
		throw ref new InvalidArgumentException("Event name can not be null or empty");
	}

	logThread->TryAddWorkItem([=]
	{
		LogEvent(eventName, eventProperties, apiProperties, timestamp, checkSession);
	});
}

int64
EventReporter::LogEvent(String ^eventName, JsonObject ^eventProperties, JsonObject ^apiProperties, int64 timestamp, bool checkSession)
{
	if (checkSession)
	{
		StartNewSessionIfNeeded(timestamp);
	}

	gSettings->SetLastEventTime(timestamp);

	auto eventObj = ref new JsonObject();
	eventObj->SetNamedValue("event_type", JsonValue::CreateStringValue(eventName));

	// Timestamp and session_id should be numbers, but WinRT's json is quite strict
	// and forces you to convert your number to a double - as opposed to Jackson
	// which happily spits out arbitrary integers.  Fortunately, Amplitude's API
	// is happy with number-as-string, so we avoid any shenanigans from funky number
	// representations.
	eventObj->SetNamedValue("timestamp", JsonValue::CreateStringValue(timestamp.ToString()));
	eventObj->SetNamedValue("session_id", JsonValue::CreateStringValue(gSessionId.ToString()));

	//eventObj->SetNamedValue("user_id", nullptr);  // TODO(ben): implement
	eventObj->SetNamedValue("device_id", JsonValue::CreateStringValue(gSettings->GetDeviceId()));
	eventObj->SetNamedValue("version_code", JsonValue::CreateStringValue(gSettings->GetAppVersion()));
	eventObj->SetNamedValue("version_name", JsonValue::CreateStringValue(gSettings->GetAppVersion()));
	// eventObj->SetNamedValue("build_version_sdk", JsonValue::CreateStringValue(this->todo));
	// eventObj->SetNamedValue("build_version_release", JsonValue::CreateStringValue(this->todo));

#if WINAPI_FAMILY == WINAPI_FAMILY_PHONE_APP
	// TODO(ben): Can we even get this information from WP 8.1?
	// eventObj->SetNamedValue("phone_brand", JsonValue::CreateStringValue(this->todo));
	// eventObj->SetNamedValue("phone_manufacturer", JsonValue::CreateStringValue(this->todo));
	// eventObj->SetNamedValue("phone_model", JsonValue::CreateStringValue(this->todo));
	// eventObj->SetNamedValue("phone_carrier", JsonValue::CreateStringValue(this->todo));
#elif WINAPI_FAMILY == WINAPI_FAMILY_PC_APP
	// something
#endif
	eventObj->SetNamedValue("country", JsonValue::CreateStringValue(gSettings->GetCountry()));
	eventObj->SetNamedValue("language", JsonValue::CreateStringValue(gSettings->GetLanguage()));
	eventObj->SetNamedValue("client", JsonValue::CreateStringValue(ref new String(CLIENT_NAME)));

	if (eventProperties == nullptr)
	{
		eventProperties = EMPTY;
	}

	if (apiProperties == nullptr)
	{
		apiProperties = EMPTY;
	}

	eventObj->SetNamedValue("api_properties", apiProperties);
	eventObj->SetNamedValue("custom_properties", eventProperties);
	eventObj->SetNamedValue("global_properties", EMPTY);
	// TODO(ben): add global properties

	return LogEvent(eventObj);
}

int64
EventReporter::LogEvent(JsonObject ^eventObj)
{
	Database db(gDatabasePath);
	auto eventId = db.AddEvent(eventObj);

	if (db.GetEventCount() > EVENT_MAX_COUNT)
	{
		db.RemoveEvents(db.GetNthEventId(EVENT_REMOVE_BATCH_SIZE));
	}

	if (db.GetEventCount() > EVENT_UPLOAD_THRESHOLD)
	{
		UpdateServer();
	}
	else
	{
		UpdateServerLater(EVENT_UPLOAD_PERIOD_MILLIS);
	}

	return eventId;
}

void
EventReporter::UploadEvents()
{
	REQUIRE_API_KEY("UploadEvents()");

	logThread->TryAddWorkItem(std::bind(EventReporter::UpdateServer, true));
}

void
EventReporter::UpdateServer(bool limit)
{
	if (!gUploadingCurrently.exchange(true))
	{
		// If we weren't already uploading, we are now.
		Database db(gDatabasePath);
		auto lastSessionEnd = gSettings->GetLastEndSessionId();
		auto eventCount = limit ? EVENT_UPLOAD_MAX_BATCH_SIZE : -1;
		auto maxIdAndEvents = db.GetEventsSince(lastSessionEnd, eventCount);
		auto maxId = maxIdAndEvents.first;
		auto events = maxIdAndEvents.second;

		if (maxId != -1)
		{
			// Only send a request if we actually have events to submit
			MakeEventUploadPostRequest(events, maxId);
		}
		else
		{
			LogDebug("maxId == 1, *not* uploading");
		}
	}
}

void
EventReporter::UpdateServerLater(int64 delayInMillis)
{
	if (!gUpdateScheduled.exchange(true))
	{
		RunDelayed([]
		{
			logThread->TryAddWorkItem([]
			{
				gUpdateScheduled.store(false);
				UpdateServer();
			});
		}, delayInMillis);
	}
}

void
EventReporter::MakeEventUploadPostRequest(JsonArray ^events, int64 maxId)
{
	std::wostringstream apiStr, timestampStr;
	apiStr << API_VERSION;
	timestampStr << GetCurrentDateAsJavaMillis();

	auto apiVersion = ref new String(apiStr.str().c_str());
	auto timestamp = ref new String(timestampStr.str().c_str());
	auto json = events->Stringify();

	String^ checksum;
	try
	{
		auto preimage = apiVersion + gApiKey + json + timestamp;
		auto buf = CryptographicBuffer::ConvertStringToBinary(preimage, BinaryStringEncoding::Utf8);
		auto alg = HashAlgorithmProvider::OpenAlgorithm(HashAlgorithmNames::Md5);
		auto hash = alg->HashData(buf);
		checksum = CryptographicBuffer::EncodeToHexString(hash);
	}
	catch (Exception^ e)
	{
		Log("Error checksumming: " + e->Message);
		gUploadingCurrently.store(false);
		return;
	}

	auto params = ref new Platform::Collections::Map<String^, String^>();
	params->Insert("v", apiVersion);
	params->Insert("e", json);
	params->Insert("client", gApiKey);
	params->Insert("upload_time", timestamp);
	params->Insert("checksum", checksum);

	auto client = ref new HttpClient();
	auto content = ref new HttpFormUrlEncodedContent(params);

	create_task(client->PostAsync(EVENT_UPLOAD_URI, content)).then([](HttpResponseMessage^ response)
	{
		return create_task(response->Content->ReadAsStringAsync());
	}).then([maxId](String ^response)
	{
		if (response == "success")
		{
			logThread->TryAddWorkItem([maxId]
			{
				auto shouldUpdate = false;
				{
					Database db(gDatabasePath);
					db.RemoveEvents(maxId);
					shouldUpdate = db.GetEventCount() > EVENT_UPLOAD_THRESHOLD;
				}

				gUploadingCurrently.store(false);
				if (shouldUpdate)
				{
					UpdateServer(false);
				}
			});
			return true;
		}
		else if (response == "invalid_api_key")
		{
			LogDebug("[Amplitude] Invalid API key, make sure your API key is correct in initialize()");
		}
		else if (response == "bad_checksum")
		{
			LogDebug("[Amplitude] Bad checksum, post request was mangled in transit, will attempt to reupload later");
		}
		else if (response == "request_db_write_failed")
		{
			LogDebug(L"[Amplitude] Couldn't write to request database on server, will attempt to reupload later");
		}
		else
		{
			auto message = "Upload failed, " + response + ", will attempt to re-upload later";
			LogDebug(message->Data());
		}

		return false;
	}).then([](task<bool> t)
	{
		// task-based continuations always run; this
		// is where we ensure that we always reset
		// the uploading flag.
		try
		{
			t.get();
		}
		catch (Platform::Exception ^ex)
		{
			Log(ex->ToString());
		}
		catch (const std::exception &ex)
		{
			LogDebug(ex.what());
		}

		gUploadingCurrently.store(false);
	});
}

EventReporter::EventReporter()
{
}
