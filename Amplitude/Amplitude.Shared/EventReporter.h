#pragma once

namespace Amplitude
{
	using Platform::String;
	using Windows::Data::Json::JsonArray;
	using Windows::Data::Json::JsonObject;
	using Windows::Foundation::IAsyncAction;
	using Windows::Storage::ApplicationDataContainer;

	// TODO(ben): Move from JsonObject in the interface to IMap<String, Object> so JavaScript can use this
	[Windows::Foundation::Metadata::WebHostHidden]
	public ref class EventReporter sealed
	{
	public:
		static void Initialize(String ^apiKey);

		static void StartSession();
		static void EndSession();

		static void LogEvent(String ^eventName);
		static void LogEvent(String ^eventName, JsonObject ^properties);

		static void UploadEvents();

	private:
		EventReporter();

		static void CheckedLogEvent(String ^eventName, JsonObject ^eventProperties, JsonObject ^apiProperties, int64 timestamp, bool checkSession);
		static int64 LogEvent(String ^eventName, JsonObject ^eventProperties, JsonObject ^apiProperties, int64 timestamp, bool checkSession);
		static int64 LogEvent(JsonObject ^eventObj);

		static void UpdateServer(bool limit = true);
		static void UpdateServerLater(int64 delayInMillis);

		static void MakeEventUploadPostRequest(JsonArray ^events, int64 maxId);

		static void StartNewSessionIfNeeded(int64 timestamp);
		static void StartNewSession(int64 timestamp);

		static void OpenSession();
		static void CloseSession();
	};
}
