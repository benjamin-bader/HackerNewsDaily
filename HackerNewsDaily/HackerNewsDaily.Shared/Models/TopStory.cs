using System;
using System.Collections.Generic;
using System.Text;
using HackerNewsDaily.Core;

namespace HackerNewsDaily.Models
{
    public enum TopStoryType
    {
        Story,
        Poll,
        Job,

        // TODO(ben): Do we actually need to distinguish between story and [Ask HN]?
        AskHN
    }

    public class TopStory
    {
        public long Id { get; set; }
        public string Title { get; set; }
        public string By { get; set; }
        public Uri Uri { get; set; }
        public string Text { get; set; }
        public int Score { get; set; }
        public DateTime Time { get; set; }
        public int CommentCount { get; set; }
        public string RawType { get; set; }

        public string Host
        {
            get { return Uri != null ? Uri.Host : string.Empty; }
        }

        public TopStoryType Type
        {
            get
            {
                switch (RawType.ToLowerInvariant())
                {
                    case "story":
                        return Uri != null
                            ? TopStoryType.Story
                            : TopStoryType.AskHN;
                    case "poll": return TopStoryType.Poll;
                    case "job": return TopStoryType.Job;
                    default:
                        throw new InvalidOperationException("Unexpected top-story type: " + RawType);
                }
            }
        }
    }
}
