using System;

namespace Klawr.ClrHost.Managed.Attributes
{
    [AttributeUsage(AttributeTargets.Property, AllowMultiple = false, Inherited = true)]
    public class UPROPERTYAttribute : Attribute
    {
        private string[] metaData = new string[0];
        private bool saveGame = false;
        private bool advancedDisplay = false;
        private string category = "";

        public UPROPERTYAttribute(string Category = "", bool SaveGame = false, bool AdvancedDisplay = false, params string[] MetaData)
        {
            this.saveGame = SaveGame;
            this.advancedDisplay = AdvancedDisplay;
            this.metaData = MetaData;
            this.category = Category;
        }

        public UPROPERTYAttribute()
        {
        }
        public UPROPERTYAttribute(string Category = "", bool SaveGame = false, bool AdvancedDisplay = false)
        {
            this.saveGame = SaveGame;
            this.advancedDisplay = AdvancedDisplay;
            this.category = Category;
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

        public bool SaveGame { get { return this.saveGame; } set { this.saveGame = value; } }

        public bool AdvancedDisplay { get { return this.advancedDisplay; } set { this.advancedDisplay = value; } }  


        public string Category { get { return this.category; } set { this.category = value; } }
    }
}
