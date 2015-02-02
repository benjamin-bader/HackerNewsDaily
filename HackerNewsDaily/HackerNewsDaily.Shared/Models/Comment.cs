using System;
using System.Collections.Generic;
using System.Text;
using Windows.UI;
using Caliburn.Micro;
using HackerNewsDaily.Core;

namespace HackerNewsDaily.Models
{
    public class Comment : PropertyChangedBase, IIndentable
    {

        private long id;
        public long Id
        {
            get { return id; }
            set { id = value; NotifyOfPropertyChange(); }
        }

        private string by;
        public string By
        {
            get { return by; }
            set { by = value; NotifyOfPropertyChange(); }
        }

        private DateTime time;
        public DateTime Time
        {
            get { return time; }
            set
            {
                time = value;
                NotifyOfPropertyChange();
                NotifyOfPropertyChange("DisplayTime");
            }
        }

        private string text;
        public string Text
        {
            get { return text; }
            set{ text = value; NotifyOfPropertyChange(); }
        }

        private Comment parent;
        public Comment Parent
        {
            get { return parent; }
            set
            {
                parent = value;
                NotifyOfPropertyChange();
                NotifyOfPropertyChange("IndentLevel");
            }
        }

        private Color color;
        public Color Color
        {
            get { return color; }
            set { color = value; NotifyOfPropertyChange(); }
        }

        public string DisplayTime
        {
            get
            {
                var now = DateTime.UtcNow;
                var diff = now.Subtract(Time);
                return diff.ToDisplayTime();
            }
        }

        private int? indentLevel;
        public int IndentLevel
        {
            get
            {
                if (!indentLevel.HasValue)
                {
                    indentLevel = ComputeIndentLevel();
                }

                return indentLevel.Value;
            }
        }

        private int ComputeIndentLevel()
        {
            if (parent == null)
            {
                return 1;
            }

            return parent.IndentLevel + 1;
        }
    }
}
