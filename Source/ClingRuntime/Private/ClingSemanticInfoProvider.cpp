#include "ClingSemanticInfoProvider.h"
#include "CppInterOp/CppInterOp.h"

FClingSemanticInfoProvider::FClingSemanticInfoProvider() {}

void FClingSemanticInfoProvider::Refresh(CppImpl::CppInterpWrapper& InInterp)
{
	if (!InInterp.IsValid()) return;
	
	Namespaces.Reset();
	Classes.Reset();
	Structs.Reset();
	Unions.Reset();
	Enums.Reset();
	Functions.Reset();
	Typedefs.Reset();
	Templates.Reset();
	Others.Reset();
	SymbolKinds.Reset();

	static thread_local FClingSemanticInfoProvider* CurrentProvider = nullptr;
	CurrentProvider = this;

	InInterp.GetAllCppNamesWithTypeName(InInterp.GetGlobalScope(), [&InInterp](const char* const* Names, const char* const* TypeNames, size_t Size)
	{
		if (!CurrentProvider) return;

		for (size_t i = 0; i < Size; ++i)
		{
			FString Name = UTF8_TO_TCHAR(Names[i]);
			FString TypeName = UTF8_TO_TCHAR(TypeNames[i]);

				if (TypeName == TEXT("Namespace"))
				{
					CurrentProvider->Namespaces.Add(Name);
					CurrentProvider->SymbolKinds.Add(Name, EClingSemanticSymbolKind::Namespace);
				}
				else if (TypeName == TEXT("CXXRecord") || TypeName == TEXT("Record"))
				{
				// 尝试区分 Class 和 Struct
				CppImpl::TCppScope_t Scope = InInterp.GetNamed(TCHAR_TO_UTF8(*Name), InInterp.GetGlobalScope());
				if (Scope)
				{
						if (InInterp.IsClass(Scope))
						{
							CurrentProvider->Classes.Add(Name);
							CurrentProvider->SymbolKinds.Add(Name, EClingSemanticSymbolKind::Class);
						}
						else
						{
						// 如果不是 Class 且是 Record，通常是 Struct 或 Union
						// 鉴于 CppInterOp 没暴露 IsUnion，我们先归类到 Structs
							CurrentProvider->Structs.Add(Name);
							CurrentProvider->SymbolKinds.Add(Name, EClingSemanticSymbolKind::Struct);
						}
					}
					else
					{
						CurrentProvider->Classes.Add(Name);
						CurrentProvider->SymbolKinds.Add(Name, EClingSemanticSymbolKind::Class);
					}
				}
				else if (TypeName == TEXT("Function") || TypeName == TEXT("CXXMethod"))
			{
					if (TChar<WIDECHAR>::IsAlpha(Name[0]))
					{
						CurrentProvider->Functions.Add(Name);
						CurrentProvider->SymbolKinds.Add(Name, EClingSemanticSymbolKind::Function);
					}
				// CurrentProvider->Functions.Add(Name);
			}
				else if (TypeName == TEXT("Typedef") || TypeName == TEXT("TypeAlias"))
				{
					CurrentProvider->Typedefs.Add(Name);
					CurrentProvider->SymbolKinds.Add(Name, EClingSemanticSymbolKind::TypeAlias);
				}
				else if (TypeName.Contains(TEXT("Template")))
				{
					CurrentProvider->Templates.Add(Name);
					CurrentProvider->SymbolKinds.Add(Name, EClingSemanticSymbolKind::Template);
				}
				else
				{
					CurrentProvider->Others.Add(Name);
				}
		}
	});

	// 获取 Using Namespaces
	InInterp.GetUsingNamespaces(InInterp.GetGlobalScope(), [&InInterp](const CppImpl::TCppScope_t* Scopes, size_t Size)
	{
		if (!CurrentProvider) return;
		for (size_t i = 0; i < Size; ++i)
		{
			InInterp.GetName(Scopes[i], [](const char* Name)
			{
				if (CurrentProvider)
				{
					CurrentProvider->Namespaces.Add(UTF8_TO_TCHAR(Name));
					CurrentProvider->SymbolKinds.Add(UTF8_TO_TCHAR(Name), EClingSemanticSymbolKind::Namespace);
				}
			});
		}
	});

	// 直接获取全局枚举，不需要通过类型名分析
	InInterp.GetEnums(InInterp.GetGlobalScope(), [](const char* const* Names, size_t Size)
	{
		if (!CurrentProvider) return;
		for (size_t i = 0; i < Size; ++i)
		{
			CurrentProvider->Enums.Add(UTF8_TO_TCHAR(Names[i]));
			CurrentProvider->SymbolKinds.Add(UTF8_TO_TCHAR(Names[i]), EClingSemanticSymbolKind::Enum);
		}
	});

	CurrentProvider = nullptr;
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

TSet<FString> FClingSemanticInfoProvider::GetAllKnownSymbols() const
{
	TSet<FString> Combined;
	Combined.Append(Namespaces);
	Combined.Append(Classes);
	Combined.Append(Structs);
	Combined.Append(Unions);
	Combined.Append(Enums);
	Combined.Append(Functions);
	Combined.Append(Typedefs);
	Combined.Append(Templates);
	Combined.Append(Others);
	return Combined;
}

EClingSemanticSymbolKind FClingSemanticInfoProvider::GetSymbolKind(const FString& Symbol) const
{
	if (const EClingSemanticSymbolKind* Kind = SymbolKinds.Find(Symbol))
	{
		return *Kind;
	}
	return EClingSemanticSymbolKind::Unknown;
}

void FClingSemanticInfoProvider::RefreshSemanticHighlightKinds(CppImpl::CppInterpWrapper& InInterp, const FString& CompiledCode)
{
	(void)CompiledCode;
	// 预留：未来可调用 CppInterOp 新增接口（例如 GetSemanticTokens / GetIdentifierKinds）
	// 将编译器返回的精确符号类型填充到 SymbolKinds 中。
	// 当前回退策略：使用 Refresh 已经抓取的解释器符号分类。
	if (!bIsReady)
	{
		Refresh(InInterp);
	}
}
