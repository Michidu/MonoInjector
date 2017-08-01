using System;
using System.IO;
using System.Reflection;

namespace TestDll
{
    public class Load
    {
        public static void Init()
        {
            string str = "Loaded assemblies:\n";

            Assembly[] assemblies = AppDomain.CurrentDomain.GetAssemblies();
            foreach (Assembly assembly in assemblies)
            {
                str += assembly.FullName + "\n";
            }

            File.WriteAllText("log.txt", str);
        }
    }
}