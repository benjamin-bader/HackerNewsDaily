using System;
using System.Collections.Generic;
using System.Text;
using System.Windows.Input;
using Windows.System;
using Caliburn.Micro;
using HackerNewsDaily.Core.Services;
using HackerNewsDaily.Models;
using HackerNewsDaily.Views;

namespace HackerNewsDaily.ViewModels
{
    public class TopStoryViewModel
    {
        private readonly TopStory story;
        private readonly INavigationService navigationService;

        public long Id
        {
            get { return story.Id; }
        }

        public string Title
        {
            get { return story.Title; }
        }

        public Uri Uri
        {
            get { return story.Uri; }
        }

        public int CommentCount
        {
            get { return story.CommentCount; }
        }

        public string DisplayInfo
        {
            get
            {
                var pointsExpression = story.Score == 1 ? "1 point" : string.Format("{0} points", story.Score);
                return string.Format("{0} {1} by {2}", pointsExpression, TimeExpression, story.By);
            }
        }

        private string TimeExpression
        {
            get
            {
                var now = DateTime.UtcNow;
                var diff = now.Subtract(story.Time);
                return diff.ToDisplayTime();
            }
        }

        public TopStoryViewModel(TopStory story, INavigationService navigationService)
        {
            this.story = story;
            this.navigationService = navigationService;
        }

        public void OpenStory()
        {
            Launcher.LaunchUriAsync(story.Uri);
        }

        public void ViewComments()
        {
            navigationService.NavigateToViewModel<CommentThreadViewModel>(story);
        }
    }
}
