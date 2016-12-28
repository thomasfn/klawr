using System;

namespace Klawr.ClrHost.Managed.Attributes
{
    [AttributeUsage(AttributeTargets.Enum, AllowMultiple = false, Inherited = true)]
    public class UENUMAttribute : Attribute
    {
        private string[] metaData = new string[0];

        public UENUMAttribute(params string[] MetaData)
        {
            this.metaData = MetaData;
        }

        public UENUMAttribute()
        {
        }


        public string[] GetMetas()
        {
            int metaDataCount = metaData.Length;
            int counter = 0;
            /*if (!string.IsNullOrEmpty(this.Category))
            {
                metaDataCount += 2;
                counter = 2;
            }*/
            string[] names = new string[metaDataCount];

            // Does it have a Category entry to transfer?
            /*if (counter == 2)
            {
                names[0] = "Category";
                names[1] = this.Category;
            }*/

            for (int i = 0; i < metaData.Length; i++)
            {
                names[counter++] = metaData[i];
            }
            return names;
        }
    }
}
