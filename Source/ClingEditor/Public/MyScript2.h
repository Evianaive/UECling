
#ifdef WITH_CLING
#else
#include "TestStatic.h"
#endif

void MyScript2()
{
	UE_LOG(LogTemp,Log,TEXT("this is myscript2 test4"));
	TestStatic::LogSomeThing();
	TestStatic::Add(1,5);
	TestStatic::Add(4,6);
	TestStatic::Add(1,5);
};

#ifdef WITH_CLING
MyScript2();
#endif