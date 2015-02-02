using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace HackerNewsDaily
{
    public static class Extensions
    {
        public static readonly DateTime Epoch = new DateTime(1970, 1, 1, 0, 0, 0, 0, DateTimeKind.Utc);

        public static long ToUnixTime(this DateTime dateTime)
        {
            var epoch = new DateTime(1970, 1, 1, 0, 0, 0, 0, DateTimeKind.Utc);
            return (long) (dateTime - epoch).TotalSeconds;
        }

        public static DateTime FromUnix(long unixTimestamp)
        {
            var epoch = new DateTime(1970, 1, 1, 0, 0, 0, 0, DateTimeKind.Utc);
            return epoch.AddSeconds(unixTimestamp);
        }

        public static string ToDisplayTime(this TimeSpan timespan)
        {
            if (timespan < TimeSpan.FromMinutes(1))
            {
                return "just now";
            }

            if (timespan < TimeSpan.FromMinutes(2))
            {
                return "1 minute ago";
            }

            if (timespan < TimeSpan.FromHours(1))
            {
                return string.Format("{0} minutes ago", timespan.Minutes);
            }

            if (timespan < TimeSpan.FromHours(2))
            {
                return "1 hour ago";
            }

            if (timespan < TimeSpan.FromDays(1))
            {
                return string.Format("{0} hours ago", timespan.Hours);
            }

            if (timespan < TimeSpan.FromDays(2))
            {
                return "1 day ago";
            }

            return string.Format("{0} days ago", timespan.Days);
        }

        public static T Second<T>(this IEnumerable<T> collection)
        {
            return collection.Nth(2);
        }

        public static T Nth<T>(this IEnumerable<T> collection, int n)
        {
            if (collection == null)
            {
                throw new ArgumentNullException("collection");
            }

            if (n <= 0)
            {
                throw new ArgumentOutOfRangeException("n", n, "Must be greater than zero");
            }

            return collection.Skip(n-1).First();
        }

        public static IEnumerable<T[]> Windowed<T>(this IEnumerable<T> collection, int size)
        {
            if (collection == null)
            {
                throw new ArgumentNullException("collection");
            }

            if (size < 1)
            {
                throw new ArgumentOutOfRangeException("size", size, "must be greater than zero");
            }

            T[] window = null;
            
            var n = 0;
            foreach (var element in collection)
            {
                if (n == 0)
                {
                    window = new T[size];
                }

                window[n] = element;
                n = (n + 1) % size;

                if (n == 0)
                {
                    yield return window;
                }
            }

            if (n != 0)
            {
                // return the partly-filled window; what else to do?
                yield return window;
            }
        }
    }
}
