#pragma once

#include "pch.h"

#include <utility> // for std::pair
#include <vector>

namespace Amplitude
{
	using std::pair;
	using std::unique_ptr;
	using std::vector;
	using std::wstring;

	using Platform::String;
	using Windows::Data::Json::JsonArray;
	using Windows::Data::Json::JsonObject;

	class Database
	{
	public:
		Database(String ^path);
		~Database();

		int64 AddEvent(JsonObject ^eventObj);

		int64 GetEventCount();

		pair<int64, JsonArray^> GetEventsSince(int64 eventId, int limit);
		int64 GetNthEventId(int n);
		
		int RemoveEvents(int64 maxId);
		int RemoveSingleEvent(int64 eventId);

	private:
		class Impl;
		unique_ptr<Impl> impl;
	};
}