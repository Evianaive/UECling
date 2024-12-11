
#include "TestStatic.h"

void TestStatic::LogSomeThing()
{
	static int Test = 1;
	UE_LOG(LogTemp,Log,TEXT("Something is loged %i"),Test++);
}