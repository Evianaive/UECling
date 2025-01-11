
#include "Registry/PtrRegistry.h"

TMap<FName, void*> FPtrRegistry::PtrMap;

void* FPtrRegistry::Get(const FName& Name)
{
	if(auto Re = PtrMap.Find(Name))
		return *Re;
	return nullptr;
}

void FPtrRegistry::Register(const FName& Name, void* Ptr)
{
	PtrMap.Add(Name,Ptr);
}
