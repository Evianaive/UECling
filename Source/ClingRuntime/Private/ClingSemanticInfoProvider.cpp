#include "ClingSemanticInfoProvider.h"
#include "CppInterOp/CppInterOp.h"

namespace
{
EClingSemanticSymbolKind ToClingSymbolKind(CppImpl::Interpreter::SemanticTokenKind InKind)
{
	switch (InKind)
	{
	case CppImpl::Interpreter::SemanticTokenKind::Namespace:
		return EClingSemanticSymbolKind::Namespace;
	case CppImpl::Interpreter::SemanticTokenKind::Class:
		return EClingSemanticSymbolKind::Class;
	case CppImpl::Interpreter::SemanticTokenKind::Struct:
		return EClingSemanticSymbolKind::Struct;
	case CppImpl::Interpreter::SemanticTokenKind::Union:
		return EClingSemanticSymbolKind::Union;
	case CppImpl::Interpreter::SemanticTokenKind::Enum:
		return EClingSemanticSymbolKind::Enum;
	case CppImpl::Interpreter::SemanticTokenKind::Function:
	case CppImpl::Interpreter::SemanticTokenKind::Method:
	case CppImpl::Interpreter::SemanticTokenKind::Constructor:
		return EClingSemanticSymbolKind::Function;
	case CppImpl::Interpreter::SemanticTokenKind::Variable:
	case CppImpl::Interpreter::SemanticTokenKind::Field:
	case CppImpl::Interpreter::SemanticTokenKind::Parameter:
		return EClingSemanticSymbolKind::Variable;
	case CppImpl::Interpreter::SemanticTokenKind::TypeAlias:
		return EClingSemanticSymbolKind::TypeAlias;
	case CppImpl::Interpreter::SemanticTokenKind::Template:
		return EClingSemanticSymbolKind::Template;
	default:
		return EClingSemanticSymbolKind::Unknown;
	}
}
}

FClingSemanticInfoProvider::FClingSemanticInfoProvider() {}

void FClingSemanticInfoProvider::Refresh(CppImpl::CppInterpWrapper& InInterp)
{
	if (!InInterp.IsValid())
	{
		return;
	}

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
		if (!CurrentProvider)
		{
			return;
		}

		for (size_t i = 0; i < Size; ++i)
		{
			const FString Name = UTF8_TO_TCHAR(Names[i]);
			const FString TypeName = UTF8_TO_TCHAR(TypeNames[i]);

			if (TypeName == TEXT("Namespace"))
			{
				CurrentProvider->Namespaces.Add(Name);
				CurrentProvider->SymbolKinds.Add(Name, EClingSemanticSymbolKind::Namespace);
			}
			else if (TypeName == TEXT("CXXRecord") || TypeName == TEXT("Record"))
			{
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
						// CppInterOp 暂无 IsUnion 暴露，当前统一按 Struct 处理。
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
				if (!Name.IsEmpty() && TChar<WIDECHAR>::IsAlpha(Name[0]))
				{
					CurrentProvider->Functions.Add(Name);
					CurrentProvider->SymbolKinds.Add(Name, EClingSemanticSymbolKind::Function);
				}
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

	InInterp.GetUsingNamespaces(InInterp.GetGlobalScope(), [&InInterp](const CppImpl::TCppScope_t* Scopes, size_t Size)
	{
		if (!CurrentProvider)
		{
			return;
		}

		for (size_t i = 0; i < Size; ++i)
		{
			InInterp.GetName(Scopes[i], [](const char* Name)
			{
				if (!CurrentProvider)
				{
					return;
				}

				const FString NamespaceName = UTF8_TO_TCHAR(Name);
				CurrentProvider->Namespaces.Add(NamespaceName);
				CurrentProvider->SymbolKinds.Add(NamespaceName, EClingSemanticSymbolKind::Namespace);
			});
		}
	});

	InInterp.GetEnums(InInterp.GetGlobalScope(), [](const char* const* Names, size_t Size)
	{
		if (!CurrentProvider)
		{
			return;
		}

		for (size_t i = 0; i < Size; ++i)
		{
			const FString EnumName = UTF8_TO_TCHAR(Names[i]);
			CurrentProvider->Enums.Add(EnumName);
			CurrentProvider->SymbolKinds.Add(EnumName, EClingSemanticSymbolKind::Enum);
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
	SemanticTokens.Reset();

	// 预留：未来可通过 CppInterOp 新接口把 CompilerInstance 的语义 token
	// (line/column/length/kind/spelling) 同步到 SemanticTokens。
	// 注意：只有当底层 CppInterOp 实现并导出该 API 时才会启用。
#if defined(CPPINTEROP_HAS_SEMANTIC_TOKENS)
	if (InInterp.IsValid() && !CompiledCode.IsEmpty())
	{
		TArray<FClingSemanticToken> CollectedTokens;
		InInterp.GetSemanticTokensForCode([&CollectedTokens](const CppImpl::Interpreter::SemanticTokenInfo* Tokens, size_t Count)
		{
			if (!Tokens || Count == 0)
			{
				return;
			}

			CollectedTokens.Reserve(static_cast<int32>(Count));
			for (size_t i = 0; i < Count; ++i)
			{
				const CppImpl::Interpreter::SemanticTokenInfo& Src = Tokens[i];
				FClingSemanticToken Dst;
				Dst.Line = static_cast<int32>(Src.line);
				Dst.Column = static_cast<int32>(Src.column);
				Dst.Length = static_cast<int32>(Src.length);
				Dst.Spelling = Src.spelling ? UTF8_TO_TCHAR(Src.spelling) : TEXT("");
				Dst.Kind = ToClingSymbolKind(Src.kind);
				CollectedTokens.Add(MoveTemp(Dst));
			}
		}, TCHAR_TO_UTF8(*CompiledCode), "UEClingBuffer.cc", 0U, 0U);

		SetSemanticTokens(CollectedTokens);
	}
#else
	(void)CompiledCode;
#endif

	if (!bIsReady)
	{
		Refresh(InInterp);
	}
}

void FClingSemanticInfoProvider::SetSemanticTokens(const TArray<FClingSemanticToken>& InTokens)
{
	SemanticTokens = InTokens;
}

bool FClingSemanticInfoProvider::TryGetSemanticTokenAtLocation(int32 Line, int32 Column, FClingSemanticToken& OutToken) const
{
	for (const FClingSemanticToken& Token : SemanticTokens)
	{
		if (Token.Contains(Line, Column))
		{
			OutToken = Token;
			return true;
		}
	}
	return false;
}
