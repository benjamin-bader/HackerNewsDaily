using System;
using System.Collections.Generic;
using System.Text;
using System.Threading.Tasks;
using HackerNewsDaily.Models;

namespace HackerNewsDaily.Core.Services
{
    public interface IHackerNewsClient
    {
        Task<IList<TopStory>> GetTopStoriesAsync();
        Task<IList<TopStory>> GetAskHNAsync();
        Task<IList<TopStory>> GetNewestStoriesAsync();

        Task<IList<Comment>> GetCommentsAsync(long storyId);
    }
}
