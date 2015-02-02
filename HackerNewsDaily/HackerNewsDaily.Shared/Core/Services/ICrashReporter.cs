using System;

namespace HackerNewsDaily.Core.Services
{
    public interface ICrashReporter
    {
        void ReportUnhandledException(Exception exception);
    }
}