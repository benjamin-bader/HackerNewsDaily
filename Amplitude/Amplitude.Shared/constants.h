#pragma once

namespace Amplitude
{
	extern Windows::Foundation::Uri ^ const EVENT_UPLOAD_URI;

	extern int const API_VERSION;
	extern int const DB_VERSION;

	extern int const EVENT_UPLOAD_THRESHOLD;
	extern int const EVENT_UPLOAD_MAX_BATCH_SIZE;
	extern int const EVENT_MAX_COUNT;
	extern int const EVENT_REMOVE_BATCH_SIZE;
	extern int64 const EVENT_UPLOAD_PERIOD_MILLIS;
	extern int64 const MIN_TIME_BETWEEN_SESSIONS_MILLIS;
	extern int64 const SESSION_TIMEOUT_MILLIS;

	namespace EventNames {
		extern Platform::String ^ const SESSION_START;
		extern Platform::String ^ const SESSION_END;
	}

	extern Platform::String ^ const PREF_CONTAINER_NAME;
	extern Platform::String ^ const PREF_ADVERTISING_ID;
	extern Platform::String ^ const PREF_PREVIOUS_SESSION_TIME;
	extern Platform::String ^ const PREF_PREVIOUS_SESSION_ID;
	extern Platform::String ^ const PREF_PREVIOUS_SESSION_END_TIME;
	extern Platform::String ^ const PREF_PREVIOUS_SESSION_END_ID;
	extern Platform::String ^ const PREF_PREVIOUS_EVENT_TIME;
	extern Platform::String ^ const PREF_PREVIOUS_EVENT_ID;
	extern Platform::String ^ const PREF_USER_ID;
	extern Platform::String ^ const PREF_DEVICE_ID;
}