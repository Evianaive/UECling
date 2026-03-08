#pragma once

class CLINGRUNTIME_API TestStatic
{
public:
	static void LogSomeThing(int* Int = nullptr);
	static int Add(int A, int B);
	static void TestNewDelete();
	static void TestNames();
	static void TestNameTypes();
};
