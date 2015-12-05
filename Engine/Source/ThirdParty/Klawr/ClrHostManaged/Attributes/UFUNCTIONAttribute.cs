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
        private string category = "";
        public bool PushProperties { get; set; }
        public bool PopProperties { get; set; }

        public string Category
        {
            get
            {
                return this.category;
            }
            set
            {
                this.category = value;
            }
        }
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

        public UFUNCTIONAttribute(string category="", bool pushProperties = true, bool popProperties = true)
        {
            this.Category = category;
            this.PushProperties = pushProperties;
            this.PopProperties = popProperties;
        }

        public UFUNCTIONAttribute(string category = "", bool pushProperties = true, bool popProperties = true, params string[] MetaData)
        {
            this.Category = category;
            this.PushProperties = pushProperties;
            this.PopProperties = popProperties;
            this.metaData = MetaData;
        }


        public string[] GetMetas()
        {
            int metaDataCount = metaData.Length;
            int counter = 0;
            if (!string.IsNullOrEmpty(this.Category))
            {
                metaDataCount += 2;
                counter = 2;
            }
            string[] names = new string[metaDataCount];

            // Does it have a Category entry to transfer?
            if (counter == 2)
            {
                names[0] = "Category";
                names[1] = this.Category;
            }

            for (int i = 0; i < metaData.Length; i++)
            {
                names[counter++] = metaData[i];
            }
            return names;
        }

    }
}
