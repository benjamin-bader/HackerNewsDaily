using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Linq;
using Windows.ApplicationModel;
using Windows.Data.Json;
using Windows.System;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Controls.Primitives;
using Caliburn.Micro;
using HackerNewsDaily.Core.Services;

namespace HackerNewsDaily.ViewModels
{
    public class HomeViewModel : ScreenWithAnalytics
    {
        private readonly ILog Log = LogManager.GetLog(typeof (HomeViewModel));

        private readonly IAnalytics analytics;
        private readonly INavigationService navigationService;
        private readonly IHackerNewsClient hn;

        private readonly AtomicBool isUpdating = new AtomicBool(false);
        private readonly ObservableCollection<TopStoryViewModel> topStories =
            new ObservableCollection<TopStoryViewModel>();

        public ObservableCollection<TopStoryViewModel> TopStories
        {
            get { return topStories; }
        }

        public bool IsUpdating
        {
            get { return isUpdating.Value; }
            private set { isUpdating.Set(value); NotifyOfPropertyChange(); }
        }

        public HomeViewModel()
        {
            if (!DesignMode.DesignModeEnabled)
            {
                throw new InvalidOperationException("just don't.");
            }
        }

        public HomeViewModel(IAnalytics analytics, INavigationService navigationService, IHackerNewsClient hn)
            : base(analytics)
        {
            this.analytics = analytics;
            this.navigationService = navigationService;
            this.hn = hn;
        }

        protected override void OnActivate()
        {
            base.OnActivate();
            RefreshTopStories();
        }

        public void OpenMenu(FrameworkElement element)
        {
            var flyoutBase = FlyoutBase.GetAttachedFlyout(element);
            flyoutBase.ShowAt(element);
        }

        public void ViewStory(ItemClickEventArgs args)
        {
            var data = new JsonObject();
            data["source"] = JsonValue.CreateStringValue("list");
            analytics.TrackEvent("view story", data);

            ViewStory(args.ClickedItem as TopStoryViewModel);
        }

        public void ViewStory(TopStoryViewModel story)
        {
            if (story != null)
            {
                Launcher.LaunchUriAsync(story.Uri);
            }
        }

        public async void RefreshTopStories()
        {
            if (!isUpdating.GetAndSet(true))
            {
                NotifyOfPropertyChange("IsUpdating");

                try
                {
                    var newStories = await hn.GetTopStoriesAsync();
                    var existingIds = new HashSet<long>(topStories.Select(ts => ts.Id));
                    var added = false;

                    // assuming that story IDs are chronologically-ordered
                    foreach (var story in from ns in newStories
                                          where !existingIds.Contains(ns.Id)
                                          orderby ns.Id ascending 
                                          select new TopStoryViewModel(ns, navigationService))
                    {
                        added = true;
                        topStories.Insert(0, story);
                    }

                    if (added && existingIds.Count > 0)
                    {
                        // need to sort the list
                        var items = topStories.OrderBy(ts => ts.Id).ToList();
                        topStories.Clear();
                        foreach (var ts in items)
                        {
                            topStories.Add(ts);
                        }
                    }
                }
                catch (Exception ex)
                {
                    // whoops
                    Log.Error(ex);
                }
                finally
                {
                    IsUpdating = false;
                }
            }
        }
    }
}
