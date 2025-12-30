#pragma once
namespace Cpp
{
	using TCppIndex_t = size_t;
	using TCppScope_t = void*;
	using TCppConstScope_t = const void*;
	using TCppType_t = void*;
	using TCppFunction_t = void*;
	using TCppConstFunction_t = const void*;
	using TCppFuncAddr_t = void*;
	using TInterp_t = void*;
	using TCppObject_t = void*;
}
DECLARE_DELEGATE_RetVal(Cpp::TInterp_t,FRestartInterpreter)
/**
 * Executor for "Cling" commands
 */
class FClingCommandExecutor : public IConsoleCommandExecutor
{
public:
	// FClingCommandExecutor(IPythonScriptPlugin* InPythonScriptPlugin);
	FClingCommandExecutor(Cpp::TInterp_t InInterpreter);
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
#if ENGINE_MAJOR_VERSION==5 && ENGINE_MINOR_VERSION>=5
	virtual void GetSuggestedCompletions(const TCHAR* Input, TArray<FConsoleSuggestion>& Out) override;
#endif
	FRestartInterpreter RestartInterpreter;
private:
	Cpp::TInterp_t Interpreter;
};
