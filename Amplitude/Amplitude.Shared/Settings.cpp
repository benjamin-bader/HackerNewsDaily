#include "pch.h"
#include "Settings.h"
#include "constants.h"

#include <string>
#include <unordered_set>

using namespace Amplitude;
using namespace Platform;

using std::unique_ptr;

using Windows::Foundation::PropertyValue;
using Windows::Storage::ApplicationDataContainer;
using Windows::Security::Cryptography::BinaryStringEncoding;
using Windows::Security::Cryptography::CryptographicBuffer;
using Windows::Security::ExchangeActiveSyncProvisioning::EasClientDeviceInformation;
using Windows::System::Profile::HardwareIdentification;
using Windows::System::UserProfile::AdvertisingManager;
using Windows::System::UserProfile::GlobalizationPreferences;

static bool
IsValid(Platform::String ^deviceId)
{
	std::unordered_set<std::wstring> invalidIds = {
		std::wstring(L""),
		L"9774d56d682e549c",
		L"unknown",
		L"DEFACE",
		L"000000000000000"
	};

	std::wstring wideId(deviceId->Data());
	return invalidIds.find(wideId) == invalidIds.end();
}

static String^
GetArchitectureName()
{
	SYSTEM_INFO systemInfo;
	GetNativeSystemInfo(&systemInfo);

	switch (systemInfo.wProcessorArchitecture)
	{
	case PROCESSOR_ARCHITECTURE_AMD64:
		return L"x64";

	case PROCESSOR_ARCHITECTURE_ARM:
		return L"ARM";

	case PROCESSOR_ARCHITECTURE_IA64:
		return L"Itanium 64";

	case PROCESSOR_ARCHITECTURE_INTEL:
		return L"x86";

	case PROCESSOR_ARCHITECTURE_UNKNOWN:
	default:
		return L"Unknown";
	}
}

/// <summary>
/// A strongly-typed wrapper around a named ApplicationDataContainer setting.
/// </summary>
template <typename T>
class Setting
{
public:
	Setting(ApplicationDataContainer ^settings, String^ key);
	virtual ~Setting();

	bool HasValue() const;

	T Get(T fallback);
	virtual void Set(T value) = 0;

	void Clear();

	Setting(Setting const&) = delete;
	Setting(Setting&&) = delete;
	Setting& operator=(Setting const&) = delete;

protected:
	Object^ Lookup();
	void Store(Object ^value);

private:
	ApplicationDataContainer ^settings;
	String ^key;
};

template <typename T>
Setting<T>::Setting(ApplicationDataContainer ^settings, String^ key) :
	settings(settings),
	key(key)
{
}

template <typename T>
Setting<T>::~Setting()
{
}

template <typename T>
bool Setting<T>::HasValue() const
{
	return settings->Values->HasKey(key);
}

template <typename T>
T Setting<T>::Get(T fallback)
{
	auto result = fallback;
	try
	{
		if (HasValue())
		{
			result = safe_cast<T>(Lookup());
		}
	}
	catch (InvalidCastException^)
	{
		// nope
	}
	return result;
}

template <typename T>
void Setting<T>::Clear()
{
	settings->Values->Remove(key);
}

template <typename T>
Object^ Setting<T>::Lookup()
{
	return settings->Values->Lookup(key);
}

template <typename T>
void Setting<T>::Store(Object ^value)
{
	settings->Values->Insert(key, dynamic_cast<PropertyValue^>(value));
}


class StringSetting : public Setting<String^>
{
public:
	StringSetting(ApplicationDataContainer ^settings, String ^key);
	~StringSetting();

	virtual void Set(String^ value);
};

StringSetting::StringSetting(ApplicationDataContainer ^settings, String ^key) : Setting(settings, key)
{
}

StringSetting::~StringSetting()
{
}

void
StringSetting::Set(String ^value)
{
	Store(PropertyValue::CreateString(value));
}

class Int64Setting : public Setting<int64>
{
public:
	Int64Setting(ApplicationDataContainer ^settings, String ^key);
	~Int64Setting();

	void Set(int64 value);
};

Int64Setting::Int64Setting(ApplicationDataContainer ^settings, String ^key) : Setting(settings, key)
{
}

Int64Setting::~Int64Setting()
{
}

void
Int64Setting::Set(int64 value)
{
	Store(PropertyValue::CreateInt64(value));
}

class Settings::Impl
{
public:
	Impl(ApplicationDataContainer ^settings);
	~Impl();

	String^ GetDeviceId();
	String^ GetAdvertisingId();
	String^ GetArchitecture();

	String^ GetLanguage();
	String^ GetCountry();

	String^ GetAppPackage();
	String^ GetAppVersion();

	int64 GetLastEventTime();
	int64 GetLastEventId();

	int64 GetLastEndSessionTime();
	int64 GetLastEndSessionId();

	int64 GetLastSessionTime();
	int64 GetLastSessionId();

	void SetLastEventId(int64 eventId);
	void SetLastEventTime(int64 timestamp);

	void SetLastEndSessionId(int64 sessionId);
	void SetLastEndSessionTime(int64 timestamp);

	void SetLastSessionId(int64 sessionId);
	void SetLastSessionTime(int64 timestamp);

	void ClearEndSession();

private:
	String ^architecture;
	String ^language;
	String ^country;
	String ^appPackage;
	String ^appVersion;

	StringSetting deviceId;
	StringSetting advertisingId;

	Int64Setting lastEventTime;
	Int64Setting lastEventId;

	Int64Setting lastEndSessionTime;
	Int64Setting lastEndSessionId;

	Int64Setting lastSessionTime;
	Int64Setting lastSessionId;
};

Settings::Impl::Impl(ApplicationDataContainer ^settings) :
	deviceId(settings, PREF_DEVICE_ID),
	advertisingId(settings, PREF_ADVERTISING_ID),
	lastEventTime(settings, PREF_PREVIOUS_EVENT_TIME),
	lastEventId(settings, PREF_PREVIOUS_EVENT_ID),
	lastEndSessionTime(settings, PREF_PREVIOUS_SESSION_END_TIME),
	lastEndSessionId(settings, PREF_PREVIOUS_SESSION_END_ID),
	lastSessionTime(settings, PREF_PREVIOUS_SESSION_TIME),
	lastSessionId(settings, PREF_PREVIOUS_SESSION_ID)
{
	architecture = GetArchitectureName();
	language = GlobalizationPreferences::Languages->GetAt(0);
	country = GlobalizationPreferences::HomeGeographicRegion;

	auto package = Windows::ApplicationModel::Package::Current;
	auto packageId = package->Id;
#if WINAPI_FAMILY == WINAPI_FAMILY_PC_APP
	appPackage = package->DisplayName;
#else
	appPackage = L""; // TODO(ben): Figure out how to get app name on WP 8.1
#endif

	auto vsn = packageId->Version;
	wchar_t buf[100];
	auto count = _snwprintf_s(buf, sizeof(buf), L"%d.%d.%d.%d", vsn.Major, vsn.Minor, vsn.Revision, vsn.Build);
	appVersion = ref new String(buf, count);
}


String^
Settings::Impl::GetAdvertisingId()
{
	if (!advertisingId.HasValue())
	{
		auto adId = AdvertisingManager::AdvertisingId;
		if (adId != nullptr && adId->Length() > 0)
		{
			advertisingId.Set(adId);
		}
	}
	return advertisingId.Get(L"");
}

String^
Settings::Impl::GetLanguage()
{
	return language;
}

String^
Settings::Impl::GetCountry()
{
	return country;
}

String^
Settings::Impl::GetAppVersion()
{
	return appVersion;
}

String^
Settings::Impl::GetAppPackage()
{
	return appPackage;
}

String^
Settings::Impl::GetArchitecture()
{
	return architecture;
}

int64
Settings::Impl::GetLastEndSessionTime()
{
	return lastEndSessionTime.Get(-1);
}

int64
Settings::Impl::GetLastEndSessionId()
{
	return lastEndSessionId.Get(-1);
}

int64
Settings::Impl::GetLastSessionTime()
{
	return lastSessionTime.Get(-1);
}

int64
Settings::Impl::GetLastSessionId()
{
	return lastSessionId.Get(-1);
}

int64
Settings::Impl::GetLastEventTime()
{
	return lastEventTime.Get(-1);
}

int64
Settings::Impl::GetLastEventId()
{
	return lastEventId.Get(-1);
}

String^
Settings::Impl::GetDeviceId()
{
	if (deviceId.HasValue())
	{
		auto id = deviceId.Get(nullptr);
		if (id != nullptr)
		{
			return id;
		}
		else
		{
			// wtf, not a string?
			deviceId.Clear();
		}
	}

#if WINAPI_FAMILY == WINAPI_FAMILY_PC_APP
	// On the desktop, we have an ideal device ID pre-generated for us
	// by the ActiveSync infrastructure: the EasClientSettingsrmation::Id
	// property is a UUID formed from the first 16 bytes of a SHA-256 hash
	// of the Machine SID, User SID, and Package Family Name, uniquely
	// identifying a machine/user/package combination; it should be stable
	// across multiple installations, assuming the package family name is
	// unchanged.

	try
	{
		auto easSettings = ref new EasClientDeviceInformation();
		auto deviceIdGuid = easSettings->Id;
		auto deviceIdStr = deviceIdGuid.ToString();

		if (IsValid(deviceIdStr))
		{
			deviceId.Set(deviceIdStr);
			return deviceIdStr;
		}
	}
	catch (Exception ^ex)
	{
		// Unsure why this would fail, but just in case...
	}
#endif

	// If we're on the phone (or if ActiveSync ID cannot be used for some other reason),
	// the next-best thing to use is the advertising ID.  It is stable per-user per-device,
	// unless the user manually disables the advertising ID feature.
	auto advertisingId = AdvertisingManager::AdvertisingId;
	if (advertisingId && advertisingId->Length() > 0)
	{
		deviceId.Set(advertisingId);
		return advertisingId;
	}

	// If advertising ID is disabled, we can then fall back on the HardwareToken.
	// This is not ideal because the token's value can drift based on factors including
	// what peripherals and network devices are active; there's no guarantee that this
	// value will persist across app re-installations.
	try
	{
		// Try to read the device ID
		auto token = HardwareIdentification::GetPackageSpecificToken(nullptr);
		auto idBuffer = token->Id;
		auto idString = CryptographicBuffer::ConvertBinaryToString(BinaryStringEncoding::Utf8, idBuffer);

		if (IsValid(idString))
		{
			deviceId.Set(idString);
			return idString;
		}
	}
	catch (Exception ^ex)
	{
		// If this fails, oh well...
	}

	// Finally, if we've exhausted all of the above, we can just generate a UUID.
	// As on Android, add an "R" suffix to indicate its random origin.
	GUID guid;
	HRESULT hresult;

	hresult = CoCreateGuid(&guid);

	if (SUCCEEDED(hresult))
	{
		Guid g(guid);
		auto id = String::Concat(g.ToString(), "R");
		deviceId.Set(id);
		return id;
	}

	throw ref new COMException(hresult);
}

void
Settings::Impl::SetLastEventId(int64 eventId)
{
	lastEventId.Set(eventId);
}

void
Settings::Impl::SetLastEventTime(int64 timestamp)
{
	lastEventTime.Set(timestamp);
}

void
Settings::Impl::SetLastEndSessionId(int64 sessionId)
{
	lastEndSessionId.Set(sessionId);
}

void
Settings::Impl::SetLastEndSessionTime(int64 timestamp)
{
	lastEndSessionTime.Set(timestamp);
}

void
Settings::Impl::SetLastSessionId(int64 sessionId)
{
	lastSessionId.Set(sessionId);
}

void
Settings::Impl::SetLastSessionTime(int64 timestamp)
{
	lastSessionTime.Set(timestamp);
}

void
Settings::Impl::ClearEndSession()
{
	lastEndSessionId.Clear();
	lastEndSessionTime.Clear();
}


Settings::Settings(ApplicationDataContainer ^settings) : impl(new Impl(settings))
{
}

String^
Settings::GetDeviceId()
{
	return impl->GetDeviceId();
}

String^
Settings::GetAdvertisingId()
{
	return impl->GetAdvertisingId();
}

String^
Settings::GetArchitecture()
{
	return impl->GetArchitecture();
}

String^
Settings::GetLanguage()
{
	return impl->GetLanguage();
}

String^
Settings::GetCountry()
{
	return impl->GetCountry();
}

String^
Settings::GetAppPackage()
{
	return impl->GetAppPackage();
}

String^
Settings::GetAppVersion()
{
	return impl->GetAppVersion();
}

int64
Settings::GetLastEventTime()
{
	return impl->GetLastEventTime();
}

int64
Settings::GetLastEventId()
{
	return impl->GetLastEventId();
}

int64
Settings::GetLastEndSessionTime()
{
	return impl->GetLastEndSessionTime();
}

int64
Settings::GetLastEndSessionId()
{
	return impl->GetLastEndSessionId();
}

int64
Settings::GetLastSessionTime()
{
	return impl->GetLastSessionTime();
}

int64
Settings::GetLastSessionId()
{
	return impl->GetLastSessionId();
}

void
Settings::SetLastEventId(int64 eventId)
{
	impl->SetLastEventId(eventId);
}

void
Settings::SetLastEventTime(int64 timestamp)
{
	impl->SetLastEventTime(timestamp);
}

void
Settings::SetLastEndSessionId(int64 sessionId)
{
	impl->SetLastEndSessionId(sessionId);
}

void
Settings::SetLastEndSessionTime(int64 timestamp)
{
	impl->SetLastEndSessionTime(timestamp);
}

void
Settings::SetLastSessionId(int64 sessionId)
{
	impl->SetLastSessionId(sessionId);
}

void
Settings::SetLastSessionTime(int64 timestamp)
{
	impl->SetLastSessionTime(timestamp);
}

void
Settings::ClearEndSession()
{
	impl->ClearEndSession();
}