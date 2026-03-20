#include "Actor.h"

IMPLEMENT_OBJECT(AActor, UObject)

void AActor::Tick(float DeltaTime)
{
	for (int32 i = 0; i < Components.size(); ++i)
	{
		Components[i]->TickComponent(DeltaTime);
	}
}

void AActor::Release()
{
	for (size_t i = 0; i < Components.size(); ++i)
	{
		UObjectFactory::DestroyObject(Components[i]);
	}
	Components.clear();
}
