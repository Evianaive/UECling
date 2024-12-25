#pragma once

namespace cling
{
	class Interpreter;
}

/**
 * Executor for "Cling" commands
 */
class FClingCommandExecutor : public IConsoleCommandExecutor
{
public:
	// FClingCommandExecutor(IPythonScriptPlugin* InPythonScriptPlugin);
	FClingCommandExecutor(cling::Interpreter* InInterpreter);
	static FName StaticName();
	virtual FName GetName() const override;
	virtual FText GetDisplayName() const override;
	virtual FText GetDescription() const override;
	virtual FText GetHintText() const override;
	virtual void GetAutoCompleteSuggestions(const TCHAR* Input, TArray<FString>& Out) override;
	virtual void GetExecHistory(TArray<FString>& Out) override;
	virtual bool Exec(const TCHAR* Input) override;
	virtual bool AllowHotKeyClose() const override;
	virtual bool AllowMultiLine() const override;
	virtual FInputChord GetHotKey() const override;
	virtual FInputChord GetIterateExecutorHotKey() const override;

private:
	cling::Interpreter* Interpreter;
};
