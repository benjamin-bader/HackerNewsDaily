using System;
using System.Collections.Generic;
using System.Text;
using Windows.Data.Json;
using Windows.UI.Xaml.Navigation;
using Caliburn.Micro;
using HackerNewsDaily.Core.Services;

namespace HackerNewsDaily.ViewModels
{
    public class ScreenWithAnalytics : Screen
    {
        private readonly IAnalytics analytics;

        public ScreenWithAnalytics()
        {
            
        }

        public ScreenWithAnalytics(IAnalytics analytics)
        {
            this.analytics = analytics;;
        }

        protected override void OnActivate()
        {
            analytics.TrackViewActivated(GetType());
        }
    }
}
