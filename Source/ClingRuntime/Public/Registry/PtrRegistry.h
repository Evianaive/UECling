#pragma once

class CLINGRUNTIME_API FPtrRegistry
{
public:
	static void* Get(const FName& Name);
	template<typename T, typename TStr>
	static T* Get(TStr&& Str)
	{
		return static_cast<T*>(Get(Forward(Str)));
	}
	
	static void Register(const FName& Name, void* Ptr);
	template<typename T, typename TStr>
	static void Register(TStr&& Name, T* Ptr)
	{
		Register(Forward(Name),Ptr);
	}
private:
	static TMap<FName, void*> PtrMap;
};
