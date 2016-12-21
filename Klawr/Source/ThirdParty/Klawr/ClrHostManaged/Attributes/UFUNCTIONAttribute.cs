using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace Klawr.ClrHost.Managed.Attributes
{
    [AttributeUsage(AttributeTargets.Method, AllowMultiple = false, Inherited = true)]
    public class UFUNCTIONAttribute : Attribute
    {
        private string[] metaData = new string[0];
        public bool PushProperties { get; set; }
        public bool PopProperties { get; set; }

        public UFUNCTIONAttribute(bool pushProperties = true, bool popProperties = true)
        {
            this.PushProperties = pushProperties;
            this.PopProperties = popProperties;
        }

        public UFUNCTIONAttribute(bool pushProperties = true, bool popProperties = true, params string[] MetaData)
        {
            this.PushProperties = pushProperties;
            this.PopProperties = popProperties;
            this.metaData = MetaData;
        }

        public string[] GetMetas()
        {
            int metaDataCount = metaData.Length;
            int counter = 0;
            string[] names = new string[metaDataCount];

            for (int i = 0; i < metaData.Length; i++)
            {
                names[counter++] = metaData[i];
            }
            return names;
        }

    }
}
