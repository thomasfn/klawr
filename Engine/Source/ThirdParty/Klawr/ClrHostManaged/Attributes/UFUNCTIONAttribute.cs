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
        public bool PushProperties { get; set; }
        public bool PopProperties { get; set; }

        public UFUNCTIONAttribute(bool pushProperties = true, bool popProperties = true)
        {
            this.PushProperties = PushProperties;
            this.PopProperties = PopProperties;
        }
    }
}
