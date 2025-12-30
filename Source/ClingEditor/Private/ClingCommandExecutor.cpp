
#include "ClingCommandExecutor.h"
#include "CppInterOp/CppInterOp.h"
#include "Toolkits/GlobalEditorCommonCommands.h"

#define LOCTEXT_NAMESPACE "ClingCommandExecutor"

FClingCommandExecutor::FClingCommandExecutor(Cpp::TInterp_t InInterpreter)
	:Interpreter(InInterpreter)
{	
}

FName FClingCommandExecutor::StaticName()
{
	static const FName CmdExecName = TEXT("Cling");
	return CmdExecName;
}

FName FClingCommandExecutor::GetName() const
{
	return StaticName();
}

FText FClingCommandExecutor::GetDisplayName() const
{
	return LOCTEXT("ClingCommandExecutorDisplayName", "Cling");
}

FText FClingCommandExecutor::GetDescription() const
{
	return LOCTEXT("ClingCommandExecutorDescription", "Execute Cpp scripts");
}

FText FClingCommandExecutor::GetHintText() const
{
	return LOCTEXT("ClingCommandExecutorHintText", "Enter Cpp Script");
}

void FClingCommandExecutor::GetAutoCompleteSuggestions(const TCHAR* Input, TArray<FString>& Out)
{
}

void FClingCommandExecutor::GetExecHistory(TArray<FString>& Out)
{
	IConsoleManager::Get().GetConsoleHistory(TEXT("Cling"), Out);
}

bool FClingCommandExecutor::Exec(const TCHAR* Input)
{
	SCOPED_NAMED_EVENT(Cling_EXEC, FColor::Red);
	IConsoleManager::Get().AddConsoleHistoryEntry(TEXT("Cling"), Input);
	UE_LOG(LogTemp, Log, TEXT("%s"), Input);
	FString InputString = Input;
	if(InputString.StartsWith(".r"))
	{
		if(RestartInterpreter.IsBound())
			Interpreter = RestartInterpreter.Execute();
	}
	// else if(InputString.StartsWith(".pch"))
	// {
	// 	FString Left,Right;
	// 	InputString.Split(" ",&Left,&Right);
	// 	while(Right.StartsWith(" "))
	// 		Right.RemoveAt(0);
	// 	Right.Split(" ",&Left,&Right);
	// 	::GeneratePCH(Interpreter,
	// 		StringCast<ANSICHAR>(*Left).Get(),
	// 		StringCast<ANSICHAR>(*Right).Get());
	// }
	// else
	// {
	// 	::ProcessCommand(Interpreter,TCHAR_TO_ANSI(Input),nullptr);	
	// }
	
	Cpp::Process(TCHAR_TO_ANSI(Input));
	return true;
}

bool FClingCommandExecutor::AllowHotKeyClose() const
{
	return true;
}

bool FClingCommandExecutor::AllowMultiLine() const
{
	return true;
}

FInputChord FClingCommandExecutor::GetHotKey() const
{
#if WITH_EDITOR
	return FGlobalEditorCommonCommands::Get().OpenConsoleCommandBox->GetActiveChord(EMultipleKeyBindingIndex::Primary).Get();
#else
	return FInputChord();
#endif
}

FInputChord FClingCommandExecutor::GetIterateExecutorHotKey() const
{
#if WITH_EDITOR
	return FGlobalEditorCommonCommands::Get().SelectNextConsoleExecutor->GetActiveChord(EMultipleKeyBindingIndex::Primary).Get();
#else
	return FInputChord();
#endif
}
#if ENGINE_MAJOR_VERSION==5 && ENGINE_MINOR_VERSION>=5
void FClingCommandExecutor::GetSuggestedCompletions(const TCHAR* Input, TArray<FConsoleSuggestion>& Out)
{
}
#endif
#undef LOCTEXT_NAMESPACE
