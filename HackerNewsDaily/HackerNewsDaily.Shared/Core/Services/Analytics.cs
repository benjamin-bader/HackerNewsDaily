using System;
using Windows.Data.Json;
using Amplitude;

namespace HackerNewsDaily.Core.Services
{
    public class Analytics : IAnalytics
    {
        public Analytics()
        {
            EventReporter.Initialize("bd94ceb65698259a44db25b2fd6eefe3");
        }

        public void StartSession()
        {
            EventReporter.StartSession();
        }

        public void EndSession()
        {
            EventReporter.EndSession();
        }

        public void TrackViewActivated(Type viewModel, JsonObject properties = null)
        {
            if (properties == null)
            {
                properties = new JsonObject();
            }

            var viewModelName = viewModel.Name;
            var viewName = viewModelName.Replace("ViewModel", string.Empty);
            properties.SetNamedValue("view", JsonValue.CreateStringValue(viewName));

            TrackEvent("view activated", properties);
        }

        public void TrackEvent(string eventName, JsonObject properties = null)
        {
            EventReporter.LogEvent(eventName, properties);
        }
    }
}
