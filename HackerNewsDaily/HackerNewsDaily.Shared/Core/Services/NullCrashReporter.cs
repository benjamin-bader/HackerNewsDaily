using System;
using System.Collections.Generic;
using System.Text;

namespace HackerNewsDaily.Core.Services
{
    public class NullCrashReporter : ICrashReporter
    {
        // how?  raygun.io is expensive and crittercism doesn't support winrt :(

        public void ReportUnhandledException(Exception exception)
        {
            
        }
    }
}
