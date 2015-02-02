using System;
using System.Collections.Generic;
using Windows.Data.Json;

namespace HackerNewsDaily.Core.Services
{
    public interface IAnalytics
    {
        void StartSession();
        void EndSession();

        void TrackViewActivated(Type view, JsonObject properties = null);
        void TrackEvent(string eventName, JsonObject properties = null);
    }
}
