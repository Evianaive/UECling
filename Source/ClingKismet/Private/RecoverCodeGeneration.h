
#pragma once

class FRecoverCodeGeneration
{
public:
	// Generate cpp type name of property
	static FString GetCppTypeOfProperty(const FProperty* Property);

	// Generate define of this struct if it is a blueprint struct!
	static void GenerateStructCode(const UScriptStruct* Struct, FString& InOutDeclare);

	// Find Struct in the property
	static void RecursivelyGenerateDeclareOfNonNativeProp(const FProperty* Property, FString& InOutDeclare);

	// Get CppType Of Pin
	static void GetCppTypeFromPin(FString& InOutString, const FEdGraphPinType& PinType, const UEdGraphPin* Pin = nullptr, UClass* SelfClass = UObject::StaticClass());
};
