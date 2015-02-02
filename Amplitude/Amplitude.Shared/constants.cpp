#pragma once

#include "pch.h"
#include "constants.h"

using namespace Amplitude;

namespace Amplitude
{
	using Platform::String;
	using Windows::Foundation::Uri;

	Uri ^ const EVENT_UPLOAD_URI = ref new Uri(L"https://api.amplitude.com/");

	int const API_VERSION = 2;
	int const DB_VERSION = 1;

	int const EVENT_UPLOAD_THRESHOLD = 30;
	int const EVENT_UPLOAD_MAX_BATCH_SIZE = 100;
	int const EVENT_MAX_COUNT = 1000;
	int const EVENT_REMOVE_BATCH_SIZE = 20;
	int64 const EVENT_UPLOAD_PERIOD_MILLIS = 30 * 1000; // 30s
	int64 const MIN_TIME_BETWEEN_SESSIONS_MILLIS = 15 * 1000; // 15s
	int64 const SESSION_TIMEOUT_MILLIS = 30 * 60 * 1000; // 30m

	namespace EventNames
	{
		String ^ const SESSION_START = L"session_start";
		String ^ const SESSION_END = L"session_end";
	}

	String ^ const PREF_CONTAINER_NAME = L"Amplitude";
	String ^ const PREF_ADVERTISING_ID = L"AdvertisingId";
	String ^ const PREF_PREVIOUS_SESSION_TIME = L"LastSessionTime";
	String ^ const PREF_PREVIOUS_SESSION_ID = L"LastSessionId";
	String ^ const PREF_PREVIOUS_SESSION_END_TIME = L"LastSessionEndTime";
	String ^ const PREF_PREVIOUS_SESSION_END_ID = L"LastSessionEndId";
	String ^ const PREF_PREVIOUS_EVENT_TIME = L"LastEventTime";
	String ^ const PREF_PREVIOUS_EVENT_ID = L"LastEventId";
	String ^ const PREF_USER_ID = L"UserId";
	String ^ const PREF_DEVICE_ID = L"DeviceId";
}
