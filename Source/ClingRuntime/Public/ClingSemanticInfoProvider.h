#pragma once

#include "CoreMinimal.h"

#include "ClingSemanticInfoProvider.generated.h"

/**
 * 为 C++ 代码高亮和语义检查提供符号信息
 */
USTRUCT()
struct CLINGRUNTIME_API FClingSemanticInfoProvider
{
	GENERATED_BODY()
public:
	FClingSemanticInfoProvider();
	
	/** 从解释器刷新符号信息 */
	void Refresh(void* InInterp);
	
	UPROPERTY()
	bool bIsReady{false};
	bool IsReady() const { return bIsReady; }

	const TSet<FString>& GetNamespaces() const { return Namespaces; }
	const TSet<FString>& GetClasses() const { return Classes; }
	const TSet<FString>& GetStructs() const { return Structs; }
	const TSet<FString>& GetUnions() const { return Unions; }
	const TSet<FString>& GetEnums() const { return Enums; }
	const TSet<FString>& GetFunctions() const { return Functions; }
	const TSet<FString>& GetTypedefs() const { return Typedefs; }
	const TSet<FString>& GetTemplates() const { return Templates; }
	const TSet<FString>& GetOthers() const { return Others; }

	/** 获取所有已知的类型（Class, Struct, Union, Enum, Typedef） */
	TSet<FString> GetAllKnownTypes() const;

private:
	// Todo Change to FName
	UPROPERTY()
	TSet<FString> Namespaces;
	UPROPERTY()
	TSet<FString> Classes;
	UPROPERTY()
	TSet<FString> Structs;
	UPROPERTY()
	TSet<FString> Unions;
	UPROPERTY()
	TSet<FString> Enums;
	UPROPERTY()
	TSet<FString> Functions;
	UPROPERTY()
	TSet<FString> Typedefs;
	UPROPERTY()
	TSet<FString> Templates;
	UPROPERTY()
	TSet<FString> Others;
};
