using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices.WindowsRuntime;
using System.Threading.Tasks;
using Windows.ApplicationModel;
using Windows.ApplicationModel.Activation;
using Windows.Foundation;
using Windows.Foundation.Collections;
using Windows.Storage;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Controls.Primitives;
using Windows.UI.Xaml.Data;
using Windows.UI.Xaml.Input;
using Windows.UI.Xaml.Media;
using Windows.UI.Xaml.Media.Animation;
using Windows.UI.Xaml.Navigation;
using Amplitude;
using Caliburn.Micro;

using HackerNewsDaily.Core.Services;
using HackerNewsDaily.ViewModels;
using HackerNewsDaily.Views;

namespace HackerNewsDaily
{
    public sealed partial class App
    {
        private WinRTContainer container;

        public App()
        {
            this.InitializeComponent();
            LogManager.GetLog = type => new DebugLog(type);
        }

        protected override void Configure()
        {
            container = new WinRTContainer();

            container.RegisterWinRTServices();

            container.PerRequest<HomeViewModel>();
            container.PerRequest<CommentThreadViewModel>();

            container.Singleton<IAnalytics, Analytics>();
            container.Singleton<ICrashReporter, NullCrashReporter>();
            container.Singleton<IHackerNewsClient, HackerNewsHtmlClient>();
        }

        protected override void PrepareViewFirst(Frame rootFrame)
        {
            container.RegisterNavigationService(rootFrame);
        }

        protected override void OnLaunched(LaunchActivatedEventArgs e)
        {
#if DEBUG
            if (System.Diagnostics.Debugger.IsAttached)
            {
                this.DebugSettings.EnableFrameRateCounter = true;
            }
#endif
            Initialize();
            container.GetInstance<IAnalytics>().StartSession();

            DisplayRootView<HomeView>();
        }

        protected override void OnActivated(IActivatedEventArgs args)
        {
            container.GetInstance<IAnalytics>().StartSession();
        }

        protected override object GetInstance(Type service, string key)
        {
            return container.GetInstance(service, key);
        }

        protected override IEnumerable<object> GetAllInstances(Type service)
        {
            return container.GetAllInstances(service);
        }

        protected override void BuildUp(object instance)
        {
            container.BuildUp(instance);
        }

        protected override void OnUnhandledException(object sender, UnhandledExceptionEventArgs e)
        {
            container.GetInstance<ICrashReporter>().ReportUnhandledException(e.Exception);
        }

        protected override void OnResuming(object sender, object e)
        {
            container.GetInstance<IAnalytics>().StartSession();
        }

        protected override void OnSuspending(object sender, SuspendingEventArgs e)
        {
            container.GetInstance<IAnalytics>().EndSession();
        }
    }
}