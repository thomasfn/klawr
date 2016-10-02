using System.IO;
using UnrealBuildTool;

namespace UnrealBuildTool.Rules
{
	public class KlawrRuntimePlugin : ModuleRules
	{
		public KlawrRuntimePlugin(TargetInfo Target)
		{
			PublicIncludePaths.AddRange(
				new string[] {
					// ... add other public include paths required here ...
				}
			);

			PrivateIncludePaths.AddRange(
				new string[] {
                    "KlawrRuntimePlugin/Private",
                    "ThirdParty/Klawr/ClrHostNative/Public"
                    // ... add other private include paths required here ...
				}
			);

			PublicDependencyModuleNames.AddRange(
				new string[]
				{
					"Core",
					"CoreUObject",
					"Engine",
					"InputCore",
					"SlateCore",
                    // ... add other public dependencies that you statically link with here ...
				}
			);

			if (UEBuildConfiguration.bBuildEditor == true)
			{
				PublicDependencyModuleNames.AddRange(
					new string[] 
					{
						"UnrealEd", 
					}
				);
			}

			PrivateDependencyModuleNames.AddRange(
				new string[]
				{
					// ... add private dependencies that you statically link with here ...
				}
			);

			DynamicallyLoadedModuleNames.AddRange(
				new string[]
				{
					// ... add any modules that your module loads dynamically here ...
				}
			);

            //this needs to be tied to that the code generator has run. when does it run?
            var basePath = Path.GetDirectoryName(ModuleDirectory);
			var pluginPath = Path.GetFullPath(Path.Combine(basePath, "..", @"Source\ThirdParty\Klawr", Target.Platform.ToString(), "Release", "Klawr.ClrHost.Native-x64-Release.lib"));
			if (File.Exists(pluginPath)){
				Definitions.Add("WITH_KLAWR=1");
        		Log.TraceInformation("Klawr runntime module dir at " + ModuleDirectory);
        		Log.TraceInformation("Linking " + pluginPath);
				PublicLibraryPaths.Add(Path.GetDirectoryName(pluginPath));
				PublicAdditionalLibraries.Add(pluginPath);

				PrivateDependencyModuleNames.Add("KlawrClrHostNative");
			} else{
        		Log.TraceInformation("Error: File not found for runntime plugin: " + pluginPath);			    
			}
		}
	}
}