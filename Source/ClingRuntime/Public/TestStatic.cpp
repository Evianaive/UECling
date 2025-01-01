
#include "TestStatic.h"

void TestStatic::LogSomeThing(int* Int)
{
	static int Test = 1;
	if(Int)
	{
		Test+=*Int;
		UE_LOG(LogTemp,Log,TEXT("Something is loged %i"),Test);
	}
	else
		UE_LOG(LogTemp,Log,TEXT("Something is loged %i"),Test++);
}

int TestStatic::Add(int A, int B)
{
	return A+B;
}
