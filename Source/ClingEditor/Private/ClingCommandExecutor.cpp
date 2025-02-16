
#include "ClingCommandExecutor.h"

#include "cling-demo.h"
#include "Toolkits/GlobalEditorCommonCommands.h"

#define LOCTEXT_NAMESPACE "ClingCommandExecutor"

FClingCommandExecutor::FClingCommandExecutor(cling::Interpreter* InInterpreter)
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
	if(FString(Input).StartsWith(".r"))
	{
		if(RestartInterpreter.IsBound())
			Interpreter = RestartInterpreter.Execute();
	}
	else
	{
		::ProcessCommand(Interpreter,TCHAR_TO_ANSI(Input),nullptr);	
	}
	//::Process(Interpreter,TCHAR_TO_ANSI(Input),nullptr);
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
