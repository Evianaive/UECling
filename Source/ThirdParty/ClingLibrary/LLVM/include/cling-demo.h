#pragma once
#if defined _WIN32 || defined _WIN64
#define EXAMPLELIBRARY_IMPORT __declspec(dllimport)
#elif defined __linux__
#define EXAMPLELIBRARY_IMPORT __attribute__((visibility("default")))
#else
#define EXAMPLELIBRARY_IMPORT
#endif
#include "cling/Interpreter/Interpreter.h"

EXAMPLELIBRARY_IMPORT cling::Interpreter* CreateInterp(int argc,
	const char* const* argv,const char* llvmdir);

EXAMPLELIBRARY_IMPORT cling::Interpreter* CreateChildInterp(
	const cling::Interpreter& parentInterpreter,
	int argc,  const char* const* argv,  const char* llvmdir);

class EXAMPLELIBRARY_IMPORT FCodeComplete
{
public:
	FCodeComplete(const cling::Interpreter* interpreter,
			const std::string& line, size_t& cursor
			);
	std::vector<std::string> completions;
};
// EXAMPLELIBRARY_IMPORT void CodeComplete(
//   const cling::Interpreter* interpreter,
//   const std::string& line, size_t& cursor,
//   std::vector<std::string>& completions)

EXAMPLELIBRARY_IMPORT void LoadHeader(
	cling::Interpreter* interpreter,
	const std::string& filename,
	cling::Transaction** T = nullptr);

EXAMPLELIBRARY_IMPORT void Execute(cling::Interpreter* interpreter, const std::string& String);

EXAMPLELIBRARY_IMPORT void Process(cling::Interpreter* interpreter, const std::string& String,
	cling::Value* V = nullptr,
	cling::Transaction** T = nullptr,
	bool disableValuePrinting = true);

EXAMPLELIBRARY_IMPORT void Decalre(cling::Interpreter* interpreter, const std::string& String,
	cling::Transaction** T = nullptr);

EXAMPLELIBRARY_IMPORT void AddIncludePath(cling::Interpreter* interpreter, const std::string& String);

EXAMPLELIBRARY_IMPORT void ProcessCommand(cling::Interpreter* interpreter, const std::string& String,
	cling::Value* V = nullptr,
	bool disableValuePrinting = true);