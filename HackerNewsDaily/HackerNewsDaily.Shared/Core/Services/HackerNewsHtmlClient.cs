using System;
using System.Collections.Generic;
using System.Linq;
using System.Text.RegularExpressions;
using System.Threading.Tasks;
using Windows.UI;
using Windows.UI.Xaml;
using Windows.Web.Http;
using Windows.Web.Http.Filters;
using Caliburn.Micro;
using HackerNewsDaily.Models;
using HtmlAgilityPack;

namespace HackerNewsDaily.Core.Services
{
    public class HackerNewsHtmlClient : IHackerNewsClient
    {
        private static readonly ILog Log = LogManager.GetLog(typeof (HackerNewsHtmlClient));
        private static readonly Regex TimeExpression = new Regex(@"(\d+) (minute|hour|day)s? ago", RegexOptions.IgnoreCase);

        private readonly HttpClient client;
        private readonly IAnalytics analytics;

        public HackerNewsHtmlClient(IAnalytics analytics)
        {
            this.analytics = analytics;

            var filter = new HttpBaseProtocolFilter();
            filter.AutomaticDecompression = true;
            filter.AllowUI = false;
            filter.AllowAutoRedirect = true;
            filter.CacheControl.ReadBehavior = HttpCacheReadBehavior.MostRecent;

            client = new HttpClient(filter);
        }

        public Task<IList<TopStory>> GetTopStoriesAsync()
        {
            return FetchStoriesAsync("http://news.ycombinator.com/news");
        }

        public Task<IList<TopStory>> GetAskHNAsync()
        {
            return FetchStoriesAsync("http://news.ycombinator.com/ask");
        }

        public Task<IList<TopStory>> GetNewestStoriesAsync()
        {
            return FetchStoriesAsync("http://news.ycombinator.com/newest");
        }

        private async Task<IList<TopStory>> FetchStoriesAsync(string url)
        {
            var html = await FetchHtml(new Uri(url)).ConfigureAwait(false);
            var outer = html.DocumentNode.Descendants("table").First();
            var inner = outer.Descendants("table").Second();
            var rows = inner.Descendants("tr").ToList();

            var stories = new List<TopStory>(30); // usually will have 30 stories on the front page, minus job postings.

            const int rowsPerStory = 3;
            for (var i = 0; i < rows.Count - rowsPerStory; i += rowsPerStory)
            {
                var storyPieces = rows[i].Descendants("td").ToList();
                var metaPieces = rows[i + 1].Descendants("td").ToList();

                if (metaPieces[1].ChildNodes.Count == 1)
                {
                    // We're in a job posting, bail
                    continue;
                }

                var href = storyPieces[2].Descendants("a").First().Attributes["href"].Value;
                var link = new Uri(href.StartsWith("http") ? href : "http://news.ycombinator.com/" + href, UriKind.Absolute); // Ask HN posts have relative URIs, all others are absolute.
                var title = storyPieces[2].Descendants("a").First().InnerText;

                var scoreline = metaPieces[1].Descendants("span").First().InnerText.Trim();

                var anchors = metaPieces[1].Descendants("a").ToList();
                var byline = anchors[0].Attributes["href"].Value;
                var commentHref = anchors[1].Attributes["href"].Value;
                var commentLine = anchors[1].InnerText;

                var id = long.Parse(commentHref.Substring(commentHref.LastIndexOf('=') + 1));
                var author = byline.Substring(byline.LastIndexOf('=') + 1);
                var score = int.Parse(scoreline.Split(' ')[0]);
                var timeline = ((HtmlTextNode)metaPieces[1].ChildNodes[3]).Text;
                var numComments = commentLine == "discuss" ? 0 : int.Parse(commentLine.Split(' ')[0]);

                var story = new TopStory
                {
                    By = author,
                    CommentCount = numComments,
                    Id = id,
                    RawType = "story",
                    Score = score,
                    Text = null,
                    Time = InferTimeFromDisplayText(timeline),
                    Title = title,
                    Uri = link
                };

                stories.Add(story);
            }

            return stories;
        }

        public async Task<IList<Comment>> GetCommentsAsync(long storyId)
        {
            var html = await FetchHtml(new Uri("http://news.ycombinator.com/item?id=" + storyId)).ConfigureAwait(false);
            var outer = html.DocumentNode.Descendants("table").First();
            var inner = outer.Descendants("table").Nth(3);
            var commentRows = inner.Descendants("table").ToList();

            var comments = new List<Comment>(commentRows.Count);

            var lastIndent = -1;
            var parents = new Stack<Comment>();

            const int indentFactor = 40;
            foreach (var row in commentRows)
            {
                //var commentTable = row.Descendants("table").Single();
                var commentCells = row.Descendants("td").ToList();
                var indentCell = commentCells[0].Descendants("img").First();
                var mainCell = commentCells[2];

                var indent = int.Parse(indentCell.Attributes["width"].Value)/indentFactor;

                if (indent <= lastIndent && parents.Count > 0)
                {
                    parents.Pop();
                }

                lastIndent = indent;

                var parent = null as Comment;
                if (parents.Count > 0)
                {
                    parent = parents.Peek();
                }

                if (mainCell.InnerText == "[deleted]")
                {
                    var deletedComment = new Comment()
                    {
                        Id = -1,
                        By = string.Empty,
                        Color = Colors.Black,
                        Parent = parents.Peek(),
                        Text = "[deleted]",
                        Time = DateTime.UtcNow
                    };

                    comments.Add(deletedComment);
                    parents.Push(deletedComment);
                    continue;
                }

                var metaCell = mainCell.Descendants("div").First();
                var metaSpan = metaCell.Descendants("span").First();
                var bylineAnchor = metaSpan.Descendants("a").First();
                var commentAnchor = metaSpan.Descendants("a").Second();

                var authorHref = bylineAnchor.Attributes["href"].Value;

                var author = authorHref.Substring(authorHref.LastIndexOf('=') + 1); // InnerText is unreliable e.g. when username is in greentext
                var rawTime = ((HtmlTextNode) metaSpan.ChildNodes[1]).Text;
                var commentIdText = commentAnchor.Attributes["href"].Value;
                var commentId = long.Parse(commentIdText.Substring(commentIdText.LastIndexOf('=') + 1));

                var textSpan = mainCell.Descendants("span").First(span => span.Attributes["class"].Value == "comment");
                var font = textSpan.FirstChild;
                var colorName = font.Attributes["color"].Value;
                var color = Color.FromArgb(
                    255,
                    Convert.ToByte(colorName.Substring(1, 2), 16),
                    Convert.ToByte(colorName.Substring(3, 2), 16),
                    Convert.ToByte(colorName.Substring(5, 2), 16));

                var content = font.InnerHtml;

                var comment = new Comment
                {
                    Id = commentId,
                    Parent = parent,
                    By = author,
                    Time = InferTimeFromDisplayText(rawTime),
                    Text = content,
                    Color = color
                };

                comments.Add(comment);

                // last line in the loop body
                parents.Push(comment);
            }

            // TODO(ben): thread the comments by building up child-id lists

            return comments;
        }

        private DateTime InferTimeFromDisplayText(string displayValue)
        {
            var match = TimeExpression.Match(displayValue);
            if (!match.Success)
            {
                // This is either because the displayValue is 'just now',
                // in which case 'Now' is the right value, or because we
                // have completely unexpected input, in which case 'Now'
                // is as sensible a default as we can hope for.
                return DateTime.UtcNow;
            }

            var scalar = int.Parse(match.Groups[1].Value);
            var unit = match.Groups[2].Value.ToLowerInvariant();

            TimeSpan diff;
            switch (unit)
            {
                case "minute":
                    diff = TimeSpan.FromMinutes(scalar);
                    break;
                case "hour":
                    diff = TimeSpan.FromHours(scalar);
                    break;
                case "day":
                    diff = TimeSpan.FromDays(scalar);
                    break;
                default:
                    // whoops!
                    throw new InvalidOperationException("We captured something and didn't code for it: " + unit);
            }

            return DateTime.UtcNow - diff;
        }

        private async Task<HtmlDocument> FetchHtml(Uri uri)
        {
            var request = new HttpRequestMessage(HttpMethod.Get, uri);
            request.Headers.UserAgent.ParseAdd("HackerNewsDaily/1.0");
            request.Headers.Accept.ParseAdd("text/html,application/xhtml+xml,application/xml");
            request.Headers.AcceptEncoding.ParseAdd("gzip,deflate");

            try
            {
                var response = await client.SendRequestAsync(request).AsTask().ConfigureAwait(false);

                if (response.StatusCode != HttpStatusCode.Ok)
                {
                    return null;
                }

                var text = await response.Content.ReadAsStringAsync().AsTask().ConfigureAwait(false);
                var html = new HtmlDocument();
                html.LoadHtml(text);

                return html;
            }
            catch (HtmlWebException ex)
            {
                Log.Error(ex);
                return null;
            }
            catch (Exception ex)
            {
                Log.Error(ex);
                return null;
            }
        }
    }
}
