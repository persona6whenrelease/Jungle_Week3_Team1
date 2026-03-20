#include "ObjectFactory.h"

uint32 UObjectFactory::NextUUID = 0;
bool UObjectFactory::bIsLoading = false;

void UObjectFactory::DestroyObject(UObject* Obj)
{
	if (!Obj) return;
	Obj->bPendingKill = true;
}

void UObjectFactory::FlushPendingKill()
{
	for (int i = 0; i < GUObjectArray.size(); i++)
	{
		if (GUObjectArray[i]->bPendingKill)
		{
			UObject* tmp = GUObjectArray[i];
			GUObjectArray[i] = GUObjectArray[GUObjectArray.size() - 1];
			GUObjectArray.pop_back();
			delete tmp;
			i--;
		}
	}
}

