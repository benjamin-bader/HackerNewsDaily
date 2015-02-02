using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using Windows.Data.Html;
using Windows.UI.Popups;
using Windows.UI.Text;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Documents;
using Windows.UI.Xaml.Media;
using Windows.UI.Xaml.Media.Imaging;
using Caliburn.Micro;
using HtmlAgilityPack;

namespace HackerNewsDaily.Controls
{
    public static class Xamlify
    {
        private static readonly ILog Log = LogManager.GetLog(typeof (Xamlify));

        public static readonly DependencyProperty HtmlProperty = DependencyProperty.RegisterAttached(
            "Html", typeof (string), typeof (Xamlify), new PropertyMetadata(string.Empty, OnHtmlChanged));

        public static object GetHtml(DependencyObject obj)
        {
            return obj.GetValue(HtmlProperty);
        }

        public static void SetHtml(DependencyObject obj, object value)
        {
            obj.SetValue(HtmlProperty, value);
        }

        private static void OnHtmlChanged(DependencyObject d, DependencyPropertyChangedEventArgs e)
        {
            var tb = d as RichTextBlock;
            if (tb == null)
            {
                return;
            }

            var textBrush = tb.Foreground;
            var html = e.NewValue as string;
            var baselink = "http://news.ycombinator.com";
            var blocks = GenerateBlocksForHtml(html, baselink, textBrush);

            tb.Blocks.Clear();
            foreach (var b in blocks)
            {
                tb.Blocks.Add(b);
            }
        }

        private static IList<Block> GenerateBlocksForHtml(string html, string baselink, Brush foreground)
        {
            var blocks = new List<Block>();

            try
            {
                var htmldoc = new HtmlDocument();
                htmldoc.LoadHtml(html);

                foreach (var img in htmldoc.DocumentNode.Descendants("img"))
                {
                    if (!img.Attributes["src"].Value.StartsWith("http"))
                    {
                        img.Attributes["src"].Value = baselink + img.Attributes["src"].Value;
                    }
                }

                var b = GenerateParagraph(htmldoc.DocumentNode, foreground);
                blocks.Add(b);
            }
            catch (Exception ex)
            {
                Log.Error(ex);
            }

            return blocks;
        }

        private static Block GenerateParagraph(HtmlNode documentNode, Brush foreground)
        {
            var paragraph = new Paragraph();
            AddChildren(paragraph.Inlines, documentNode, foreground);
            return paragraph;
        }

        private static void AddChildren(InlineCollection inlines, HtmlNode node, Brush foreground)
        {
            var added = false;
            foreach (var child in node.ChildNodes)
            {
                var inline = GenerateBlockForNode(child, foreground);
                if (inline != null)
                {
                    inlines.Add(inline);
                    added = true;
                }
            }

            if (!added)
            {
                inlines.Add(new Run { Text = CleanText(node.InnerText) });
            }
        }

        private static string CleanText(string input)
        {
            var clean = HtmlUtilities.ConvertToText(input);
            return clean == "\0" ? Environment.NewLine : clean;
        }

        private static Inline GenerateBlockForNode(HtmlNode htmlNode, Brush foreground)
        {
            switch (htmlNode.Name.ToLowerInvariant())
            {
                case "div":
                    return GenerateSpan(htmlNode, foreground);

                case "p":
                    return GenerateInnerParagraph(htmlNode, foreground);

                case "img":
                    return GenerateImage(htmlNode, foreground);

                case "a":
                    return GenerateHyperlink(htmlNode, foreground);

                case "#text":
                    if (!string.IsNullOrWhiteSpace(htmlNode.InnerText))
                    {
                        return new Run {Text = CleanText(htmlNode.InnerText)};
                    }
                    return null;

                case "i":
                case "em":
                    return GenerateItalicSpan(htmlNode, foreground);

                case "b":
                case "strong":
                    return GenerateBoldSpan(htmlNode, foreground);

                default:
                    Log.Warn("Unsupported tag: {0}", htmlNode.Name);
                    return null;
            }
        }

        private static Inline GenerateSpan(HtmlNode node, Brush foreground)
        {
            var span = new Span();
            AddChildren(span.Inlines, node, foreground);
            return span;
        }

        private static Inline GenerateItalicSpan(HtmlNode node, Brush foreground)
        {
            var span = new Span { FontStyle = FontStyle.Italic };
            AddChildren(span.Inlines, node, foreground);
            span.Inlines.Add(new Run { Text = " " }); // italics tend to lean right in to subsequent text; a space prevents that.
            return span;
        }

        private static Inline GenerateBoldSpan(HtmlNode node, Brush foreground)
        {
            var span = new Span { FontWeight = FontWeights.Bold };
            AddChildren(span.Inlines, node, foreground);
            return span;
        }

        private static Inline GenerateInnerParagraph(HtmlNode node, Brush foreground)
        {
            var span = new Span();
            span.Inlines.Add(new LineBreak());
            AddChildren(span.Inlines, node, foreground);
            //span.Inlines.Add(new LineBreak());
            return span;
        }

        private static Inline GenerateImage(HtmlNode node, Brush foreground)
        {
            var container = new InlineUIContainer();
            var image = new Image();
            var source = new BitmapImage(new Uri(node.Attributes["src"].Value));

            image.Source = source;
            container.Child = image;

            return container;
        }

        private static Inline GenerateHyperlink(HtmlNode node, Brush foreground)
        {
            try
            {
                var href = node.Attributes["href"].Value;
                var uri = new Uri(href, UriKind.RelativeOrAbsolute);

                var hyperlink = new Hyperlink
                {
                    NavigateUri = uri,
                    Foreground = foreground
                };

                AddChildren(hyperlink.Inlines, node.FirstChild, foreground);

                return hyperlink;
            }
            catch (FormatException ex)
            {
                // This happens when people enter HTML in HN comments and get
                // the <a> tag wrong.  We just punt on the link and render what
                // inner text is available.
                Log.Error(ex);

                var span = new Span();
                AddChildren(span.Inlines, node.FirstChild, foreground);
                return span;
            }
        }
    }
}
