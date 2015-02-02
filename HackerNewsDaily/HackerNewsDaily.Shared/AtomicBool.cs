using System;
using System.Collections.Generic;
using System.Text;
using System.Threading;

namespace HackerNewsDaily
{
    /// <summary>
    /// Provides atomic operations for a boolean value.
    /// </summary>
    public class AtomicBool
    {
        private const int True = 1;
        private const int False = 0;

        private int value = False;

        public bool Value
        {
            get { return IntToBool(Volatile.Read(ref value)); }
        }

        public AtomicBool(bool initialValue)
        {
            value = BoolToInt(initialValue);
        }

        /// <summary>
        /// Unconditionally sets a new value.
        /// </summary>
        public void Set(bool newValue)
        {
            Volatile.Write(ref value, BoolToInt(newValue));
        }

        /// <summary>
        /// Atomically sets a new value for this object and returns the old value.
        /// </summary>
        /// <param name="newValue"></param>
        /// <returns>
        /// The old value which was replaced by <paramref name="newValue"/>.
        /// </returns>
        public bool GetAndSet(bool newValue)
        {
            var oldValue = Interlocked.Exchange(ref value, BoolToInt(newValue));
            return IntToBool(oldValue);
        }

        /// <summary>
        /// Atomically sets the value of this object to a given 
        /// </summary>
        /// <param name="expected"></param>
        /// <param name="update"></param>
        /// <returns>
        /// <see langword="true"/> if the value was updated, otherwise <see langword="false"/>.
        /// </returns>
        public bool CompareAndSet(bool expected, bool update)
        {
            var comparand = BoolToInt(expected);
            var newValue = BoolToInt(update);
            var didUpdate = Interlocked.CompareExchange(
                ref value,
                newValue,
                comparand) == comparand;

            return didUpdate;
        }

        private static int BoolToInt(bool value)
        {
            return value ? True : False;
        }

        private static bool IntToBool(int value)
        {
            return value == True;
        }
    }
}
