using System;
using System.Collections.Generic;
using System.Text;

namespace HackerNewsDaily.Models
{
    public interface IIndentable
    {
        int IndentLevel { get; }
    }
}
