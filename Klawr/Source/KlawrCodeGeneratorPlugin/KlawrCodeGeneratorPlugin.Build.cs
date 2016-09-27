using System.IO;
using UnrealBuildTool;

namespace UnrealBuildTool.Rules
{
	public class KlawrCodeGeneratorPlugin : ModuleRules
	{
		public KlawrCodeGeneratorPlugin(TargetInfo Target)
		{
            PublicIncludePaths.AddRange(
				new string[] {
					"Programs/UnrealHeaderTool/Public",
                    "KlawrCodeGeneratorPlugin/Public",
					// ... add other public include paths required here ...
				}
			);

			PrivateIncludePaths.AddRange(
				new string[] {
					// ... add other private include paths required here ...
                    "KlawrCodeGeneratorPlugin/Private"
				}
			);

			PublicDependencyModuleNames.AddRange(
				new string[]
				{
					"Core",
					"CoreUObject",
                    // ... add other public dependencies that you statically link with here ...
				}
			);

			PrivateDependencyModuleNames.AddRange(
				new string[]
				{
    				"Projects"
					// ... add private dependencies that you statically link with here ...
				}
			);

			DynamicallyLoadedModuleNames.AddRange(
				new string[]
				{
					// ... add any modules that your module loads dynamically here ...
				}
			);

            var KlawrPath = Path.Combine(UEBuildConfiguration.UEThirdPartySourceDirectory, "Klawr");
            if(Directory.Exists(KlawrPath)) {
                Definitions.Add("KLAWRGEN_PATH=" + ModuleDirectory);
            }
            Definitions.Add("WITH_KLAWR=1");
    		Definitions.Add("HACK_HEADER_GENERATOR=1");
        }
	}
}