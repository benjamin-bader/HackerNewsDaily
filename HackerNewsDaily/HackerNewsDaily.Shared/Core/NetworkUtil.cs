using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using Windows.Networking.Connectivity;

namespace HackerNewsDaily.Core
{
    public static class NetworkUtil
    {
        /// <summary>
        /// Returns the IP address of the internet-facing network adapter,
        /// or <see langword="null"/> if no such adapter can be located.
        /// </summary>
        public static string GetInternetIPAddress()
        {
            string ip = null;

            var profile = NetworkInformation.GetInternetConnectionProfile();
            if (profile != null && profile.NetworkAdapter != null)
            {
                var profileAdapterId = profile.NetworkAdapter.NetworkAdapterId;
                var matchingNames = from hn in NetworkInformation.GetHostNames()
                                    where hn.IPInformation != null
                                    let ipInfo = hn.IPInformation
                                    let net = ipInfo.NetworkAdapter
                                    where net.NetworkAdapterId == profileAdapterId
                                    select hn.CanonicalName;

                ip = matchingNames.SingleOrDefault();
            }

            return ip;
        }
    }
}
