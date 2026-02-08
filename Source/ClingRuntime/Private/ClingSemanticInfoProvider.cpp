#include "ClingSemanticInfoProvider.h"
#include "CppInterOp/CppInterOp.h"

FClingSemanticInfoProvider::FClingSemanticInfoProvider() {}

void FClingSemanticInfoProvider::Refresh(void* InInterp)
{
	if (!InInterp) return;

	void* StoreInterp = Cpp::GetInterpreter();
	Cpp::ActivateInterpreter(InInterp);
	
	Namespaces.Reset();
	Classes.Reset();
	Structs.Reset();
	Unions.Reset();
	Enums.Reset();
	Functions.Reset();
	Typedefs.Reset();
	Templates.Reset();
	Others.Reset();

	static thread_local FClingSemanticInfoProvider* CurrentProvider = nullptr;
	CurrentProvider = this;

	Cpp::GetAllCppNamesWithTypeName(Cpp::GetGlobalScope(), [](const char* const* Names, const char* const* TypeNames, size_t Size)
	{
		if (!CurrentProvider) return;

		for (size_t i = 0; i < Size; ++i)
		{
			FString Name = UTF8_TO_TCHAR(Names[i]);
			FString TypeName = UTF8_TO_TCHAR(TypeNames[i]);

			if (TypeName == TEXT("Namespace"))
			{
				CurrentProvider->Namespaces.Add(Name);
			}
			else if (TypeName == TEXT("CXXRecord") || TypeName == TEXT("Record"))
			{
				// 尝试区分 Class 和 Struct
				Cpp::TCppScope_t Scope = Cpp::GetNamed(TCHAR_TO_UTF8(*Name), Cpp::GetGlobalScope());
				if (Scope)
				{
					if (Cpp::IsClass(Scope))
					{
						CurrentProvider->Classes.Add(Name);
					}
					else
					{
						// 如果不是 Class 且是 Record，通常是 Struct 或 Union
						// 鉴于 CppInterOp 没暴露 IsUnion，我们先归类到 Structs
						CurrentProvider->Structs.Add(Name);
					}
				}
				else
				{
					CurrentProvider->Classes.Add(Name);
				}
			}
			else if (TypeName == TEXT("Function") || TypeName == TEXT("CXXMethod"))
			{
				if (TChar<WIDECHAR>::IsAlpha(Name[0]))
				{
					CurrentProvider->Functions.Add(Name);
				}
				// CurrentProvider->Functions.Add(Name);
			}
			else if (TypeName == TEXT("Typedef") || TypeName == TEXT("TypeAlias"))
			{
				CurrentProvider->Typedefs.Add(Name);
			}
			else if (TypeName.Contains(TEXT("Template")))
			{
				CurrentProvider->Templates.Add(Name);
			}
			else
			{
				CurrentProvider->Others.Add(Name);
			}
		}
	});

	// 获取 Using Namespaces
	Cpp::GetUsingNamespaces(Cpp::GetGlobalScope(), [](const Cpp::TCppScope_t* Scopes, size_t Size)
	{
		if (!CurrentProvider) return;
		for (size_t i = 0; i < Size; ++i)
		{
			Cpp::GetName(Scopes[i], [](const char* Name)
			{
				if (CurrentProvider)
				{
					CurrentProvider->Namespaces.Add(UTF8_TO_TCHAR(Name));
				}
			});
		}
	});

	// 直接获取全局枚举，不需要通过类型名分析
	Cpp::GetEnums(Cpp::GetGlobalScope(), [](const char* const* Names, size_t Size)
	{
		if (!CurrentProvider) return;
		for (size_t i = 0; i < Size; ++i)
		{
			CurrentProvider->Enums.Add(UTF8_TO_TCHAR(Names[i]));
		}
	});

	CurrentProvider = nullptr;
	Cpp::ActivateInterpreter(StoreInterp);
	bIsReady = true;
}

TSet<FString> FClingSemanticInfoProvider::GetAllKnownTypes() const
{
	TSet<FString> Combined;
	Combined.Append(Classes);
	Combined.Append(Structs);
	Combined.Append(Unions);
	Combined.Append(Enums);
	Combined.Append(Typedefs);
	Combined.Append(Templates);
	return Combined;
}
