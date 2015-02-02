using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Linq;
using System.Text;
using Caliburn.Micro;
using HackerNewsDaily.Core.Services;
using HackerNewsDaily.Models;

namespace HackerNewsDaily.ViewModels
{
    public class CommentThreadViewModel : ScreenWithAnalytics
    {
        private static readonly ILog Log = LogManager.GetLog(typeof (CommentThreadViewModel));

        private readonly INavigationService navigationService;
        private readonly IHackerNewsClient hn;

        public TopStory Parameter { get; set; }

        private bool isUpdating;
        public bool IsUpdating
        {
            get { return isUpdating; }
            set { isUpdating = value; NotifyOfPropertyChange(); }
        }

        private readonly ObservableCollection<Comment> comments = new ObservableCollection<Comment>(); 
        public ObservableCollection<Comment> Comments
        {
            get { return comments; }
        } 

        public CommentThreadViewModel(IAnalytics analytics, INavigationService navigationService, IHackerNewsClient hn)
            : base(analytics)
        {
            this.navigationService = navigationService;
            this.hn = hn;
        }

        protected override void OnActivate()
        {
            LoadCommentsAsync();
        }

        private async void LoadCommentsAsync()
        {
            if (IsUpdating)
            {
                return;
            }

            IsUpdating = true;
            try
            {
                foreach (var c in await hn.GetCommentsAsync(Parameter.Id))
                {
                    Comments.Add(c);
                }
            }
            catch (Exception ex)
            {
                Log.Error(ex);
            }
            finally
            {
                IsUpdating = false;
            }
        }
    }
}
