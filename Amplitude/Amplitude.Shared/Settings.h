#pragma once

namespace Amplitude
{
	using Platform::Object;
	using Platform::String;
	using Windows::Storage::ApplicationDataContainer;

	class Settings
	{
	public:
		Settings(ApplicationDataContainer ^settings);

	public:
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
		class Impl;
		std::unique_ptr<Impl> impl;
	};
}