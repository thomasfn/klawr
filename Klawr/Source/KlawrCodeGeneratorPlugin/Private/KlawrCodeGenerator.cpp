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
#include "KlawrCodeFormatter.h"
#include "pugixml.hpp"
#include "KlawrNativeWrapperGenerator.h"
#include "KlawrCSharpWrapperGenerator.h"
#include "KlawrCSharpStructWrapperGenerator.h"
#include "KlawrCSharpEnumWrapperGenerator.h"

namespace Klawr {


	const TArray<FName> FCodeGenerator::SpecialStructs  =
	{
		FName("Vector2D"),
		FName("Vector"),
		FName("Vector4"),
		FName("Quat"),
		FName("Rotator"),
		FName("Transform"),
		FName("LinearColor"),
		FName("Color")
	};

const FString FCodeGenerator::ClrHostManagedAssemblyName = TEXT("Klawr.ClrHost.Managed");

FCodeGenerator::FCodeGenerator(const FString& InRootLocalPath, const FString& InRootBuildPath, const FString& InOutputDirectory, const FString& InIncludeBase)
	: GeneratedCodePath(InOutputDirectory), RootLocalPath(InRootLocalPath), RootBuildPath(InRootBuildPath), IncludeBase(InIncludeBase)
{
	 
}

FString FCodeGenerator::GetPropertyCPPType(const UProperty* Property)
{
	static const FString enumDecl(TEXT("enum "));
	static const FString structDecl(TEXT("struct "));
	static const FString classDecl(TEXT("class "));
	static const FString enumAsByteDecl(TEXT("TEnumAsByte<enum "));
	static const FString subclassOfDecl(TEXT("TSubclassOf<class "));
	static const FString space(TEXT(" "));

	FString cppTypeName = Property->GetCPPType(NULL, CPPF_ArgumentOrReturnValue);
	
	if (cppTypeName.StartsWith(enumDecl) || cppTypeName.StartsWith(structDecl) || cppTypeName.StartsWith(classDecl))
	{
		// strip enum/struct/class keyword
		cppTypeName = cppTypeName.Mid(cppTypeName.Find(space) + 1);
	}
	else if (cppTypeName.StartsWith(enumAsByteDecl))
	{
		// strip enum keyword
		cppTypeName = TEXT("TEnumAsByte<") + cppTypeName.Mid(cppTypeName.Find(space) + 1);
	}
	else if (cppTypeName.StartsWith(subclassOfDecl))
	{
		// strip class keyword
		cppTypeName = TEXT("TSubclassOf<") + cppTypeName.Mid(cppTypeName.Find(space) + 1);
		// Passing around TSubclassOf<UObject> doesn't really provide any more type safety than
		// passing around UClass (so far only UObjects can cross the native/managed boundary),
		// and TSubclassOf<> lacks the reference equality UClass has.
		if (cppTypeName.StartsWith(TEXT("TSubclassOf<UObject>")))
		{
			cppTypeName = TEXT("UClass*");
		}
	}
	// some type names end with a space, strip it away
	cppTypeName.RemoveFromEnd(space);
	return cppTypeName;
}

bool FCodeGenerator::CanExportClass(const UClass* Class)
{
	bool bCanExport = 
		// skip classes that don't export DLL symbols
		Class->HasAnyClassFlags(CLASS_RequiredAPI | CLASS_MinimalAPI)
		// these have a custom wrappers, so no need to generate code for them
		&& (Class != UObject::StaticClass())
		&& (Class != UClass::StaticClass());

	if (bCanExport)
	{
		// check for exportable functions
		bool bHasMembersToExport = false;
		TFieldIterator<UFunction> funcIt(Class, EFieldIteratorFlags::ExcludeSuper);
		for ( ; funcIt; ++funcIt)
		{
			if (CanExportFunction(Class, *funcIt))
			{
				bHasMembersToExport = true;
				break;
			}
		}
		// check for exportable properties
		if (!bHasMembersToExport)
		{
			TFieldIterator<UProperty> propertyIt(Class, EFieldIteratorFlags::ExcludeSuper);
			for ( ; propertyIt; ++propertyIt)
			{
				if (CanExportProperty(Class, *propertyIt))
				{
					bHasMembersToExport = true;
					break;
				}
			}
		}
		bCanExport = bHasMembersToExport;
	}
	return bCanExport;
}

bool FCodeGenerator::CanExportStruct(const UScriptStruct* Struct)
{
	if (SpecialStructs.Contains(Struct->GetFName())) return false;

	if (Struct->GetName() == L"HitResult")
	{
		UE_LOG(LogKlawrCodeGenerator, Log, TEXT("BLAH"));
	}

	// ALL properties MUST be exportable for the struct to be exportable
	TFieldIterator<UProperty> propertyIt(Struct);
	for (; propertyIt; ++propertyIt)
	{
		UProperty* property = *propertyIt;
		if (!CanExportProperty(Struct, property))
		{
			return false;
		}
	}
	return true;
}

bool FCodeGenerator::CanExportEnum(const UEnum* Enum)
{
	return true;
}

bool FCodeGenerator::CanExportFunction(const UClass* Class, const UFunction* Function)
{
	// functions from base classes should only be exported when those classes are processed
	if (Function->GetOwnerClass() != Class)
	{
		return false;
	}

	// delegates and non-public functions are not supported yet
	if (Function->HasAnyFunctionFlags(FUNC_Delegate | FUNC_Private | FUNC_Protected))
	{
		return false;
	}

	// don't expose functions that aren't directly accessible in a Blueprint graph
	if (Function->GetBoolMetaData(TEXT("BlueprintInternalUseOnly")))
	{
		return false;
	}

	// check all parameter types for this function are supported
	for (TFieldIterator<UProperty> ParamIt(Function); ParamIt; ++ParamIt)
	{
		// arrays that are class members are supported, but arrays that are parameters are not
		if (ParamIt->IsA<UArrayProperty>() || !IsPropertyTypeSupported(*ParamIt))
		{
			return false;
		}
	}

	return true;
}

bool FCodeGenerator::IsPropertyTypeSupported(const UProperty* Property)
{
	bool bSupported = true;

	if (Property->ArrayDim > 1 ||
		Property->IsA<UDelegateProperty>() ||
		Property->IsA<UMulticastDelegateProperty>() ||
		Property->IsA<UWeakObjectProperty>() ||
		Property->IsA<UInterfaceProperty>())
	{
		bSupported = false;
	}
	else if (Property->IsA<UArrayProperty>())
	{
		auto arrayProp = Cast<UArrayProperty>(Property);
		FString elementType = GetPropertyCPPType(arrayProp->Inner);
		if (elementType.StartsWith("TSubclassOf<"))
		{
			// TArray<TSubclassOf<UWhatever>> is not currently supported
			bSupported = false;
		}
		else
		{
			bSupported = IsPropertyTypeSupported(arrayProp->Inner);

			// For now, we don't support arrays of structs
			if (arrayProp->Inner->IsA<UStructProperty>())
			{
				bSupported = false;
			}
		}
	}
	else if (Property->IsA<UStructProperty>())
	{
		auto StructProp = CastChecked<UStructProperty>(Property);
		bSupported = IsStructPropertyTypeSupported(StructProp);
	}
	else if (Property->IsA<UStrProperty>())
	{
		if (!Property->HasAnyPropertyFlags(CPF_ReturnParm | CPF_ConstParm) && 
			Property->HasAnyPropertyFlags(CPF_OutParm | CPF_ReferenceParm))
		{
			// Non-const FString references can't be easily marshaled from/to a C# string.
			// A few modifications to the generator will be required. For each non-const 
			// FString parameter the generated native wrapper function will need to take a TCHAR* 
			// to a preallocated buffer and the buffer size, then the corresponding value from the
			// FDispatchParams struct will need to be copied to the preallocated buffer after the
			// UFunction is called. On the managed side the managed wrapper delegate will need to
			// use the StringBuilder type for each non-const FString reference parameter, and the
			// corresponding StringBuilder instance of the appropriate size will need to be created
			// before the delegate can be invoked. The need to preallocate the buffer on the managed
			// side is what makes this a bit tedious, since the user is unlikely to know what a
			// suitable buffer size would be!
			bSupported = false;
		}
	}
	else if (!Property->IsA<UIntProperty>() &&
		!Property->IsA<UFloatProperty>() &&
		!Property->IsA<UBoolProperty>() &&
		!Property->IsA<UObjectPropertyBase>() &&
		!Property->IsA<UNameProperty>())
	{
		bSupported = false;
	}
	else if (Property->IsA<ULazyObjectProperty>())
	{
		bSupported = false;
	}
	
	// Check that the referenced object is known
	if (Property->IsA<UObjectProperty>())
	{
		static FString pointer(TEXT("*"));
		FString typeName = FCodeGenerator::GetPropertyCPPType(Property);
		typeName.RemoveFromEnd(pointer);

		UClass* cl = FindObject<UClass>(nullptr, *typeName);
		if (cl)
		{
			if (!IKlawrCodeGeneratorPlugin::Get().ShouldExportFromPackage(cl->GetOutermost()))
			{
				bSupported = false;
			}
		}
	}

	return bSupported;
}

bool FCodeGenerator::IsPropertyTypePointer(const UProperty* Property)
{
	return Property->IsA<UObjectPropertyBase>() || Property->IsA<ULazyObjectProperty>();
}

bool FCodeGenerator::IsStructPropertyTypeSupported(const UStructProperty* Property)
{
	if (!IKlawrCodeGeneratorPlugin::Get().ShouldExportFromPackage(Property->Struct->GetOutermost()))
	{
		return false;
	}

	const FName typeName = Property->Struct->GetFName();
	return SpecialStructs.Contains(typeName) || CanExportStruct(Property->Struct);
}

bool FCodeGenerator::CanExportProperty(const UScriptStruct* Struct, const UProperty* Property)
{
	// only public, editable properties can be exported
	/*if (!Property->HasAnyFlags(RF_Public) ||
		(Property->GetPropertyFlags() & CPF_Protected) ||
		!(Property->GetPropertyFlags() & CPF_Edit))
	{
		return false;
	}*/

	if (Property->IsA<UWeakObjectProperty>()) return true;

	return IsPropertyTypeSupported(Property);
}

bool FCodeGenerator::CanExportProperty(const UClass* Class, const UProperty* Property)
{
	// properties from base classes should only be exported when those classes are processed
	if (Property->GetOwnerClass() != Class)
	{
		return false;
	}

	// property must be DLL exported (well, not really, will remove this later)
	if (!(Class->ClassFlags & CLASS_RequiredAPI))
	{
		return false;
	}

	// only public, editable properties can be exported
	if (!Property->HasAnyFlags(RF_Public) ||
		(Property->GetPropertyFlags() & CPF_Protected) ||
		!(Property->GetPropertyFlags() & CPF_Edit))
	{
		return false;
	}

	return IsPropertyTypeSupported(Property);
}

void FCodeGenerator::ExportClass(UClass* Class, const FString& SourceHeaderFilename, const FString& GeneratedHeaderFilename, bool bHasChanged){
	if (Class->HasAnyClassFlags(CLASS_Deprecated))
    {
		return;
	}

    auto config = GetConig();

    if(config.Excluded.Contains(FPaths::GetCleanFilename(SourceHeaderFilename))) {
        return;
    }

	if (AllExportedClasses.Contains(Class))
	{
		// already processed
		return;
	}

    if(!Class) {
        UE_LOG(LogKlawrCodeGenerator, Log, TEXT("Error exporting class: %s"), *SourceHeaderFilename);
        return;
    }

	UE_LOG(LogKlawrCodeGenerator, Log, TEXT("Exporting class %s %s"), *Class->GetName(), *SourceHeaderFilename);

	// even if a class can't be properly exported generate a C# wrapper for it, because it may 
	// still be used as a function parameter in a function that is exported by another class
	AllExportedClasses.Add(Class);
	
	FCodeFormatter nativeGlueCode(TEXT('\t'), 1);
	FCodeFormatter managedGlueCode(TEXT(' '), 4);
	FNativeWrapperGenerator nativeWrapperGenerator(Class, nativeGlueCode);
	FCSharpWrapperGenerator csharpWrapperGenerator(
		Class, 
		FCSharpWrapperGenerator::GetWrapperSuperClass(Class, AllExportedClasses), 
		managedGlueCode
	);

	bool bCanExport = CanExportClass(Class);
	if (bCanExport)
	{
		ClassesWithNativeWrappers.Add(Class);
		nativeWrapperGenerator.GenerateHeader();
	}

	// some internal classes like UObjectProperty don't have an associated source header file,
	// no native wrapper functions are generated for those types, and perhaps we shouldn't generate
	// any C# wrappers for those either?
	if (!SourceHeaderFilename.IsEmpty())
	{
		// NOTE: Ideally when no native wrapper functions are generated for a class the corresponding
		//       header need not be included, however the class may still be referenced in the
		//       wrapper functions of other classes and if the header isn't included the compilation
		//       will fail. The compilation error is usually a cryptic error C2664, and occurs while 
		//       attempting to upcast a pointer to the class into a UObject*, this fails because the
		//       class is only forward declared at that point and as such the compiler is unaware that
		//       the class is derived from UObject.
		AllSourceClassHeaders.Add(SourceHeaderFilename);
	}
	
	csharpWrapperGenerator.GenerateHeader();
		
	if (bCanExport)
	{
		// export functions
		TFieldIterator<UFunction> funcIt(Class, EFieldIteratorFlags::ExcludeSuper);
		for ( ; funcIt; ++funcIt)
		{
			UFunction* function = *funcIt;
			if (CanExportFunction(Class, function))
			{
				// The inheritance hierarchy is mirrored in the C# wrapper classes, so there's no need to
				// redefine functions from a base class (assuming that base class has also been exported).
				// However, this also means that functions that are defined in UObject (which isn't exported)
				// are not available in the C# wrapper classes, but that's not a problem for now.
				if (function->GetOwnerClass() == Class)
				{
					nativeWrapperGenerator.GenerateFunctionWrapper(function);
					csharpWrapperGenerator.GenerateFunctionWrapper(function);
				}
			}
		}

		// export properties
		TFieldIterator<UProperty> propertyIt(Class, EFieldIteratorFlags::ExcludeSuper);
		for ( ; propertyIt; ++propertyIt)
		{
			UProperty* property = *propertyIt;
			if (CanExportProperty(Class, property))
			{
				UE_LOG(
					LogKlawrCodeGenerator, Log, 
					TEXT("  %s %s"), *property->GetClass()->GetName(), *property->GetName()
				);
				
				// Only wrap properties that are actually in this class, inheritance will take care of the 
				// properties from base classes.
				if (property->GetOwnerClass() == Class)
				{
					nativeWrapperGenerator.GeneratePropertyWrapper(property);
					csharpWrapperGenerator.GeneratePropertyWrapper(property);
				}
			}
		}

		if (nativeWrapperGenerator.GetPropertyCount() != csharpWrapperGenerator.GetPropertyCount())
		{
            UE_LOG(LogKlawrCodeGenerator, Log, TEXT("ERROR: Native and C# property wrapper count doesn't match!"));
            return;
		}

		if (nativeWrapperGenerator.GetFunctionCount() != csharpWrapperGenerator.GetFunctionCount())
		{
            UE_LOG(LogKlawrCodeGenerator, Log, TEXT("ERROR: Native and C# function wrapper count doesn't match!"));
            return;
        }

		nativeWrapperGenerator.GenerateFooter();
				
		const FString nativeGlueFilename = GeneratedCodePath / (Class->GetName() + TEXT(".klawr.h"));
		AllScriptHeaders.Add(nativeGlueFilename);
		WriteToFile(nativeGlueFilename, nativeGlueCode.Content);
	}
    
    csharpWrapperGenerator.GenerateFooter();

	const FString managedGlueFilename = GeneratedCodePath / (Class->GetName() + TEXT(".cs"));
	AllManagedWrapperFiles.Add(managedGlueFilename);
	WriteToFile(managedGlueFilename, managedGlueCode.Content);
}

void FCodeGenerator::ExportStruct(UScriptStruct* Struct) {
	auto config = GetConig();


	if (AllExportedStructs.Contains(Struct) || !CanExportStruct(Struct))
	{
		// already processed
		return;
	}

	if (!Struct) {
		return;
	}

	UE_LOG(LogKlawrCodeGenerator, Log, TEXT("Exporting struct %s"), *Struct->GetName());

	// even if a class can't be properly exported generate a C# wrapper for it, because it may 
	// still be used as a function parameter in a function that is exported by another class
	AllExportedStructs.Add(Struct);

	// FCodeFormatter nativeGlueCode(TEXT('\t'), 1);
	FCodeFormatter managedGlueCode(TEXT(' '), 4);
	// FNativeWrapperGenerator nativeWrapperGenerator(Class, nativeGlueCode);
	FCSharpStructWrapperGenerator csharpWrapperGenerator(
		Struct,
		managedGlueCode
	);

	csharpWrapperGenerator.GenerateHeader();

	// export properties
	TFieldIterator<UProperty> propertyIt(Struct);
	for (; propertyIt; ++propertyIt)
	{
		UProperty* property = *propertyIt;

		UE_LOG(
			LogKlawrCodeGenerator, Log,
			TEXT("  %s %s"), *property->GetClass()->GetName(), *property->GetName()
		);

		csharpWrapperGenerator.GeneratePropertyWrapper(property);
	}

	// nativeWrapperGenerator.GenerateFooter();

	// const FString nativeGlueFilename = GeneratedCodePath / (Class->GetName() + TEXT(".klawr.h"));
	// AllScriptHeaders.Add(nativeGlueFilename);
	// WriteToFile(nativeGlueFilename, nativeGlueCode.Content);

	csharpWrapperGenerator.GenerateFooter();

	const FString managedGlueFilename = GeneratedCodePath / (Struct->GetName() + TEXT(".cs"));
	AllManagedWrapperFiles.Add(managedGlueFilename);
	WriteToFile(managedGlueFilename, managedGlueCode.Content);
}

void FCodeGenerator::ExportEnum(UEnum* Enum) {
	auto config = GetConig();


	if (AllExportedEnums.Contains(Enum) || !CanExportEnum(Enum))
	{
		// already processed
		return;
	}

	if (!Enum) {
		return;
	}

	UE_LOG(LogKlawrCodeGenerator, Log, TEXT("Exporting enum %s"), *Enum->GetName());

	// even if a class can't be properly exported generate a C# wrapper for it, because it may 
	// still be used as a function parameter in a function that is exported by another class
	AllExportedEnums.Add(Enum);

	// FCodeFormatter nativeGlueCode(TEXT('\t'), 1);
	FCodeFormatter managedGlueCode(TEXT(' '), 4);
	// FNativeWrapperGenerator nativeWrapperGenerator(Class, nativeGlueCode);
	FCSharpEnumWrapperGenerator csharpWrapperGenerator(
		Enum,
		managedGlueCode
	);

	csharpWrapperGenerator.GenerateHeader();

	// export properties
	for (int i = 0; i < Enum->GetMaxEnumValue(); i++)
	{
		const FName name = Enum->GetNameByIndex(i);
		if (name != NAME_None)
		{
			csharpWrapperGenerator.GenerateName(name.ToString(), i);
		}
	}

	// nativeWrapperGenerator.GenerateFooter();

	// const FString nativeGlueFilename = GeneratedCodePath / (Class->GetName() + TEXT(".klawr.h"));
	// AllScriptHeaders.Add(nativeGlueFilename);
	// WriteToFile(nativeGlueFilename, nativeGlueCode.Content);

	csharpWrapperGenerator.GenerateFooter();

	const FString managedGlueFilename = GeneratedCodePath / (Enum->GetName() + TEXT(".cs"));
	AllManagedWrapperFiles.Add(managedGlueFilename);
	WriteToFile(managedGlueFilename, managedGlueCode.Content);
}

bool FCodeGenerator::GenerateManagedWrapperProject(){
    auto config = GetConig();

    const FString resourcesBasePath = config.WrapperProjectTemplatePath;
	const FString projectBasePath = config.WrapperProjectCopyPath;

    UE_LOG(LogKlawrCodeGenerator, Log, TEXT("Start generating wrapper project! From: %s To: %s"), *resourcesBasePath, *projectBasePath);
	IPlatformFile& platformFile = FPlatformFileManager::Get().GetPlatformFile();

    if(!FPaths::DirectoryExists(*projectBasePath)) {
        platformFile.CreateDirectory(*projectBasePath);
    }

	if (!platformFile.CopyDirectoryTree(*projectBasePath, *resourcesBasePath, true))
	{
        UE_LOG(LogKlawrCodeGenerator, Log, TEXT("Failed to copy wrapper template! From: %s To: %s"), *resourcesBasePath, *projectBasePath);
        return false;
	}

	const FString projectName("Klawr.UnrealEngine.csproj");
	const FString projectTemplateFilename = resourcesBasePath / projectName;
	const FString projectOutputFilename = projectBasePath / projectName;

	// load the template .csproj
	pugi::xml_document xmlDoc;
	pugi::xml_parse_result result = xmlDoc.load_file(*projectTemplateFilename, pugi::parse_default | pugi::parse_declaration | pugi::parse_comments);

	if (!result)
	{
        UE_LOG(LogKlawrCodeGenerator, Log, TEXT("Failed to load %s"), *projectTemplateFilename);
        return false;
	}
	else
	{
		// add a reference to the CLR host managed assembly
		auto referencesNode = xmlDoc.first_element_by_path(TEXT("/Project/ItemGroup/Reference")).parent();
		if (referencesNode)
		{
			FString assemblyPath =
				FPaths::RootDir()
				/ TEXT("Plugins/Klawr/Klawr/Source/ThirdParty/Klawr/ClrHostManaged/bin/$(Configuration)")
				/ ClrHostManagedAssemblyName + TEXT(".dll");
			FPaths::MakePathRelativeTo(assemblyPath, *projectOutputFilename);
			FPaths::MakePlatformFilename(assemblyPath);

			auto refNode = referencesNode.append_child(TEXT("Reference"));
			refNode.append_attribute(TEXT("Include")) = *ClrHostManagedAssemblyName;
			refNode.append_child(TEXT("HintPath")).text() = *assemblyPath;
		}

		// include all the generated C# wrapper classes in the project file
		auto sourceNode = xmlDoc.first_element_by_path(TEXT("/Project/ItemGroup/Compile")).parent();
		if (sourceNode)
		{
			FString linkFilename;
			for (FString managedGlueFilename : AllManagedWrapperFiles)
			{
				FPaths::MakePathRelativeTo(managedGlueFilename, *projectOutputFilename);
				FPaths::MakePlatformFilename(managedGlueFilename);
				// group all generated .cs files under a virtual folder in the project file
				linkFilename = TEXT("Generated\\");
				linkFilename += FPaths::GetCleanFilename(managedGlueFilename);

				auto compileNode = sourceNode.append_child(TEXT("Compile"));
				compileNode.append_attribute(TEXT("Include")) = *managedGlueFilename;
				compileNode.append_child(TEXT("Link")).text() = *linkFilename;
			}
		}

		// add a post-build event to copy the C# wrapper assembly to the engine binaries dir
		FString assemblyDestPath = FPlatformProcess::BaseDir();
		FPaths::NormalizeFilename(assemblyDestPath);
		FPaths::CollapseRelativeDirectories(assemblyDestPath);
		FPaths::MakePlatformFilename(assemblyDestPath);
		
		FString postBuildCmd = FString::Printf(
			TEXT("xcopy \"$(TargetPath)\" \"%s\" /Y"), *assemblyDestPath
		);

		xmlDoc
			.child(TEXT("Project"))
				.append_child(TEXT("PropertyGroup"))
					.append_child(TEXT("PostBuildEvent")).text() = *postBuildCmd;

		// preserve the BOM and line endings of the template .csproj when writing out the new file
		unsigned int outputFlags = pugi::format_default | pugi::format_write_bom | pugi::format_save_file_text;

		if (!xmlDoc.save_file(*projectOutputFilename, TEXT("  "), outputFlags))
		{
            UE_LOG(LogKlawrCodeGenerator, Log, TEXT("Failed to save %s"), *projectOutputFilename);
            return false;
		}
	}
    return true;
}

void FCodeGenerator::BuildManagedWrapperProject()
{
    UE_LOG(LogKlawrCodeGenerator, Log, TEXT("Start building wrapper project"));
    auto config = GetConig();

    FString buildFilename = config.WrapperProjectCopyPath / TEXT("Build.bat");
	FPaths::CollapseRelativeDirectories(buildFilename);
	FPaths::MakePlatformFilename(buildFilename);
	int32 returnCode = 0;
	FString stdOut;
	FString stdError;

	FPlatformProcess::ExecProcess(*buildFilename, TEXT(""), &returnCode, &stdOut, &stdError);

	if (returnCode != 0){
        UE_LOG(LogKlawrCodeGenerator, Log, TEXT("Failed to build Klawr.UnrealEngine assembly! STDOUT: %s STDERROR: %s ERRORCODE: %d"), *stdOut, *stdError, returnCode);
        return;
	}

    UE_LOG(LogKlawrCodeGenerator, Log, TEXT("finish building wrapper project"));
}

void FCodeGenerator::FinishExport()
{
    GlueAllNativeWrapperFiles();
    if(!GenerateManagedWrapperProject()){
        return;
    };
	BuildManagedWrapperProject();
}

void FCodeGenerator::GlueAllNativeWrapperFiles()
{
    UE_LOG(LogKlawrCodeGenerator, Log, TEXT("Start GlueAllNativeWrapperFiles"));
    
    // generate the file that will be included by ScriptPlugin.cpp
	FString glueFilename = GeneratedCodePath / TEXT("KlawrGeneratedNativeWrappers.inl");
	FCodeFormatter generatedGlue(TEXT('\t'), 1);

	generatedGlue 
		<< TEXT("// This file is autogenerated, DON'T EDIT it, if you do your changes will be lost!")
		<< FCodeFormatter::LineTerminator();

	// include all source header files
	generatedGlue << TEXT("// The native classes which will be made scriptable:");
	for (const auto& headerFilename : AllSourceClassHeaders)
	{
		// re-base to make sure we're including the right files on a remote machine
		FString newFilename(RebaseToBuildPath(headerFilename));
		generatedGlue << FString::Printf(TEXT("#include \"%s\""), *newFilename);
	}

	// include all script glue headers
	generatedGlue << TEXT("// The autogenerated native wrappers:");
	for (const auto& headerFilename : AllScriptHeaders)
	{
		FString newFilename(FPaths::GetCleanFilename(headerFilename));
		generatedGlue << FString::Printf(TEXT("#include \"%s\""), *newFilename);
	}
	
	// generate a function that feeds all the native wrapper functions to the CLR host
	generatedGlue
		<< FCodeFormatter::LineTerminator()
		<< TEXT("namespace Klawr {")
		<< TEXT("namespace NativeGlue {")
		<< FCodeFormatter::LineTerminator()
		<< TEXT("void RegisterWrapperClasses()")
		<< FCodeFormatter::OpenBrace();

	for (auto wrappedClass : ClassesWithNativeWrappers)
	{
		const FString ClassName = wrappedClass->GetName();
		const FString ClassNameCPP = FString::Printf(
			TEXT("%s%s"), wrappedClass->GetPrefixCPP(), *ClassName
		);
		generatedGlue << FString::Printf(
			TEXT("IClrHost::Get()->AddClass(TEXT(\"%s\"), %s_WrapperFunctions, ")
			TEXT("sizeof(%s_WrapperFunctions) / sizeof(%s_WrapperFunctions[0]));"),
			*ClassNameCPP, *ClassName, *ClassName, *ClassName
		);
	}

	generatedGlue 
		<< FCodeFormatter::CloseBrace()
		<< FCodeFormatter::LineTerminator()
		<< TEXT("}} // namespace Klawr::NativeGlue");

	WriteToFile(glueFilename, generatedGlue.Content);

    UE_LOG(LogKlawrCodeGenerator, Log, TEXT("finish GlueAllNativeWrapperFiles"));
}

void FCodeGenerator::WriteToFile(const FString& Path, const FString& Content)
{
	FString diskContent;
	FFileHelper::LoadFileToString(diskContent, *Path);

	const bool bContentChanged = (diskContent.Len() == 0) || FCString::Strcmp(*diskContent, *Content);
	if (bContentChanged)
	{
		if (!FFileHelper::SaveStringToFile(Content, *Path))
		{
			UE_LOG(LogKlawrCodeGenerator, Warning, TEXT("Failed to save '%s'"), *Path);
		}
	}
}

FString FCodeGenerator::RebaseToBuildPath(const FString& Filename) const
{
	FString rebasedFilename(Filename);
	FPaths::MakePathRelativeTo(rebasedFilename, *IncludeBase);
	return rebasedFilename;
}

} // namespace Klawr
