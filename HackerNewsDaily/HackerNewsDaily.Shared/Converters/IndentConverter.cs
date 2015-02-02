using System;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Data;
using HackerNewsDaily.Models;

namespace HackerNewsDaily.Converters
{
    /// <summary>
    /// Converts an indented comment into a <see cref="Thickness"/> suitable
    /// for use as a margin.
    /// </summary>
    public class IndentConverter : IValueConverter
    {
        public const int MaxIndentLevel = 10;

        public object Convert(object value, Type targetType, object parameter, string language)
        {
            var comment = (IIndentable) value;
            var factor = parameter is int ? (int) parameter : 10;

            var margin = new Thickness
            {
                Left = Math.Min(comment.IndentLevel, MaxIndentLevel) * factor
            };

            return margin;
        }

        public object ConvertBack(object value, Type targetType, object parameter, string language)
        {
            throw new NotImplementedException();
        }
    }
}
