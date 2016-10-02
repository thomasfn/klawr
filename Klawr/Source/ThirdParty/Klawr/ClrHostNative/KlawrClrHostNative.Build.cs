using UnrealBuildTool;
using System.IO;
using System;

public class KlawrClrHostNative : ModuleRules
{
    public KlawrClrHostNative(TargetInfo Target)
    {
        // This module is built externally, just need to let UBT know the include and library paths
        // that should be passed through to any targets that depend on this module.
        Type = ModuleType.External;

        var moduleName = this.GetType().Name;
        // path to directory containing this Build.cs file
//        var basePath = Path.GetDirectoryName(RulesCompiler.GetModuleFilename(moduleName));
        var basePath = Path.GetDirectoryName(ModuleDirectory);
		
        string architecture = null;
        if (Target.Platform == UnrealTargetPlatform.Win64)
        {
            architecture = "x64";
        }
        else if (Target.Platform == UnrealTargetPlatform.Win32)
        {
            architecture = "x86";
        } else{
            architecture = "x64";            
        }

        string configuration = null;
        switch (Target.Configuration)
        {
            case UnrealTargetConfiguration.Debug:
            case UnrealTargetConfiguration.DebugGame:
                configuration = BuildConfiguration.bDebugBuildsActuallyUseDebugCRT ? "Debug" : "Release";
                break;

            default:
                configuration = "Release";
                break;
        }

        Console.WriteLine("KlawrClrHostNative Target.Configuration: " + configuration);

        if ((architecture != null) && (configuration != null))
        {
            PublicIncludePaths.Add(Path.Combine(basePath, "Public"));
            var libName = "Klawr.ClrHost.Native-" + architecture + "-" + configuration + ".lib";
            PublicLibraryPaths.Add(Path.Combine(basePath, "..", "Build"));
            PublicAdditionalLibraries.Add(libName);
        }
                
        // copy the CLR host assembly (assumed to have been built previously) to the engine binaries 
        // directory so that it can be found and loaded at runtime by the unmanaged CLR host

        string hostAssemblyName = "Klawr.ClrHost.Managed";
        string hostAssemblyDLL = hostAssemblyName + ".dll";
        string hostAssemblyPDB = hostAssemblyName + ".pdb";
        string hostAssemblySourceDir = Path.Combine(basePath, Path.Combine("ClrHostManaged", "bin", configuration));
        Utils.CollapseRelativeDirectories(ref hostAssemblySourceDir);
        
        string binariesDir = Path.Combine(
            BuildConfiguration.RelativeEnginePath, "Binaries", Target.Platform.ToString()
        );

        bool bOverwrite = true;

        var from = Path.Combine(hostAssemblySourceDir, hostAssemblyDLL);
        var to = Path.Combine(binariesDir, hostAssemblyDLL);
        if (!File.Exists(to)){
            var msg = "Copy: " + from + "\nTo: " + to;
 		    Log.TraceInformation(msg);
            File.Copy(from, to, bOverwrite);
        }
        from = Path.Combine(hostAssemblySourceDir, hostAssemblyPDB);
        to = Path.Combine(binariesDir, hostAssemblyPDB);
        if (!File.Exists(to)){
            File.Copy(from, to, bOverwrite);
        }
    }
}
