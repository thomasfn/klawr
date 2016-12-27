//-------------------------------------------------------------------------------
// The MIT License (MIT)
//
// Copyright (c) 2014 Vadim Macagon
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//-------------------------------------------------------------------------------

#include "KlawrCodeGeneratorPluginPrivatePCH.h"
#include "KlawrCodeGenerator.h"
#include "Features/IModularFeatures.h"
#include "Programs/UnrealHeaderTool/Public/IScriptGeneratorPluginInterface.h"

DEFINE_LOG_CATEGORY(LogKlawrCodeGenerator);

namespace Klawr {

    class FCodeGeneratorPlugin : public IKlawrCodeGeneratorPlugin {
    private:
        TAutoPtr<FCodeGenerator> CodeGenerator;

    public: // IModuleInterface interface
        virtual void StartupModule() override {
            IModularFeatures::Get().RegisterModularFeature(TEXT("ScriptGenerator"), this);
        }

        virtual void ShutdownModule() override {
            UE_LOG(LogKlawrCodeGenerator, Log, TEXT("Shutdown klawr generator module."));
            CodeGenerator.Reset();
            IModularFeatures::Get().UnregisterModularFeature(TEXT("ScriptGenerator"), this);
        }

    public: // IScriptGeneratorPlugin interface
        virtual FString GetGeneratedCodeModuleName() const override {
            return TEXT("KlawrRuntimePlugin");
        }

        virtual bool ShouldExportClassesForModule(const FString & ModuleName, EBuildModuleType::Type ModuleType, const FString & ModuleGeneratedIncludeDirectory) const override {
            UE_LOG(LogKlawrCodeGenerator, Log, TEXT("Test Module for Export: %s"), *ModuleName);
            bool bCanExport = (ModuleType == EBuildModuleType::EngineRuntime || ModuleType == EBuildModuleType::GameRuntime);
            if(bCanExport) {
                // only export functions from selected modules
                auto config = FCodeGenerator::GetConig();
                bCanExport = !config.ExcludedModules.Contains(ModuleName)
                        && ((config.SupportedModules.Num() == 0) || config.SupportedModules.Contains(ModuleName));
            }
            UE_LOG(LogKlawrCodeGenerator, Log, TEXT("Export: %s"), bCanExport ? TEXT("true") : TEXT("false"));
            return bCanExport;
        }

        virtual bool SupportsTarget(const FString & TargetName) const override {
            return true;
        }

        virtual void Initialize(const FString & RootLocalPath, const FString & RootBuildPath, const FString & OutputDirectory, const FString & IncludeBase) override {
            UE_LOG(LogKlawrCodeGenerator, Log, TEXT("Using Klawr Code Generator."));
            CodeGenerator = new FCodeGenerator(RootLocalPath, RootBuildPath, OutputDirectory, IncludeBase);

			//TArray<UObject*> packages;
			//GetObjectsOfClass(UPackage::StaticClass(), packages);

			UE_LOG(LogKlawrCodeGenerator, Log, TEXT("Output directory: %s"), *OutputDirectory);

        }

        virtual void ExportClass(UClass * Class, const FString & SourceHeaderFilename, const FString & GeneratedHeaderFilename, bool bHasChanged) override {
			
			CodeGenerator->ExportClass(Class, SourceHeaderFilename, GeneratedHeaderFilename, bHasChanged);
            
        }

		virtual bool ShouldExportFromPackage(const UPackage* package)
		{
			auto config = FCodeGenerator::GetConig();
			FString packageName = package->GetName();
			packageName.RemoveFromStart(L"/Script/");

			if (packageName.Contains(L"/"))
			{
				FString left, right;
				packageName.Split(L"/", &left, &right);
				packageName = left;
			}

			if (config.ExcludedModules.Contains(packageName))
			{
				return false;
			}

			if ((config.SupportedModules.Num() > 0) && !config.SupportedModules.Contains(packageName))
			{
				return false;
			}

			return true;
		}

        virtual void FinishExport() override {
			auto config = FCodeGenerator::GetConig();

			// Export all structs
			TArray<UObject*> structs;
			GetObjectsOfClass(UScriptStruct::StaticClass(), structs);
			for (int i = 0; i < structs.Num(); i++)
			{
				UScriptStruct* Struct = CastChecked<UScriptStruct>(structs[i]);
				if (ShouldExportFromPackage(Struct->GetOutermost()))
				{
					CodeGenerator->ExportStruct(Struct);
				}
			}

			// Export all enums
			TArray<UObject*> enums;
			GetObjectsOfClass(UEnum::StaticClass(), enums);
			for (int i = 0; i < enums.Num(); i++)
			{
				UEnum* Enum = CastChecked<UEnum>(enums[i]);
				if (ShouldExportFromPackage(Enum->GetOutermost()))
				{
					CodeGenerator->ExportEnum(Enum);
				}
			}


            CodeGenerator->FinishExport();
        }

        virtual FString GetGeneratorName() const override {
            return TEXT("Klawr Code Generator Plugin");
        }
    };

} // namespace Klawr

IMPLEMENT_MODULE(Klawr::FCodeGeneratorPlugin, KlawrCodeGeneratorPlugin)
