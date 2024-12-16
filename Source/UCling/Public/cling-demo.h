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

EXAMPLELIBRARY_IMPORT void CreateInterp2(int argc,
                const char* const* argv,const char* llvmdir);

EXAMPLELIBRARY_IMPORT void LoadHeader(cling::Interpreter* interpreter, const std::string& filename,
			   cling::Transaction** T = nullptr);


EXAMPLELIBRARY_IMPORT void Process(cling::Interpreter* interpreter, const std::string& String,
			   cling::Value* V,
			   cling::Transaction** T = nullptr,
			   bool disableValuePrinting = false);

EXAMPLELIBRARY_IMPORT void Decalre(cling::Interpreter* interpreter, const std::string& String,
			   cling::Transaction** T = nullptr);

EXAMPLELIBRARY_IMPORT void AddIncludePath(cling::Interpreter* interpreter, const std::string& String);

EXAMPLELIBRARY_IMPORT void ProcessCommand(cling::Interpreter* interpreter, const std::string& String);