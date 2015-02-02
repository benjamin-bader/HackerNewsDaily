using System;
using Newtonsoft.Json;

namespace HackerNewsDaily.Core.Serialization
{
    public class UnixTimeConverter : JsonConverter
    {
        public override void WriteJson(JsonWriter writer, object value, JsonSerializer serializer)
        {
            var dto = (DateTime) value;
            var timestamp = dto.ToUnixTime();
            serializer.Serialize(writer, timestamp);
        }

        public override object ReadJson(JsonReader reader, Type objectType, object existingValue, JsonSerializer serializer)
        {
            var timestamp = serializer.Deserialize<long>(reader);
            return Extensions.FromUnix(timestamp);
        }

        public override bool CanConvert(Type objectType)
        {
            return objectType == typeof (DateTime);
        }
    }
}
