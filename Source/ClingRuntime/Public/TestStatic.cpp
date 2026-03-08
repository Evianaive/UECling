
#include "TestStatic.h"

#include "ClingRuntime.h"
#include "CppInterOp/CppInterOp.h"

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

struct NewTest
{
	NewTest(){
		UE_LOG(LogTemp,Log,TEXT("New"));
	}
	int32 A;
	float B;
};

void TestStatic::TestNewDelete()
{
	UE_DEBUG_BREAK();
	auto* P = new NewTest();
	delete P;
}

void TestStatic::TestNames()
{
	auto& Module = FClingRuntimeModule::Get();
	CppImpl::CppInterpWrapper& Wrapper = Module.GetInterp();
	Wrapper.GetAllCppNames(Wrapper.GetGlobalScope(),
		[](const char* const* Names, size_t Size)
		{
			for (int i=0;i<Size;i++)
			{
				UE_LOG(LogTemp,Log,TEXT("%hs"),Names[i]);	
			}			
		}
	);
}

void TestStatic::TestNameTypes()
{
	auto& Module = FClingRuntimeModule::Get();
	CppImpl::CppInterpWrapper& Wrapper = Module.GetInterp();
	Wrapper.GetAllCppNamesWithType(Wrapper.GetGlobalScope(),
	[](const char* const* Names, const int* Type,size_t Size)
	{
		for (int i=0;i<Size;i++)
			{
				UE_LOG(LogTemp,Log,TEXT("%hs:%i"),Names[i],Type[i]);
			}
		}
	);
}

