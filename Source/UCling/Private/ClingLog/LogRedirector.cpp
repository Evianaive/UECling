#include "LogRedirector.h"
#include "ClingLog/ClingLog.h"
#include <iostream>

class FUELogStreambuf : public std::streambuf
{
protected:
	std::string buffer;

	virtual int overflow(int c) override
	{
		// if (c == '\n') {
		// 	if (!buffer.empty()) {
		// 		UE_LOG(LogTemp, Log, TEXT("Overflow:%s"), *FString(buffer.c_str()));
		// 	}
		// 	buffer.clear();
		// } else {
		buffer += static_cast<char>(c);
		// }
		return c;
	}

	virtual int sync() override
	{
		if (!buffer.empty() && buffer.ends_with("\n"))
		{
			UE_LOG(LogCling, Log, TEXT("%s"), *FString(buffer.c_str()));
			buffer.clear();
		}
		return 0;
	}
};


void FLogRedirector::RedirectToUELog()
{
	static FUELogStreambuf ueLogBuf;
	std::cout.rdbuf(&ueLogBuf);
	std::cerr.rdbuf(&ueLogBuf);
}