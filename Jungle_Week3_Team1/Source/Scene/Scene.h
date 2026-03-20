#pragma once

#include "../../CoreMinimal.h"

class UObject;
class AActor;

class Scene : public UObject
{
public:
	Scene();
	virtual ~Scene();

	const TMap<uint32, AActor*>& GetAActorArray();
	AActor* GetAActor(uint32 InUUID);


private:
	TMap<uint32, AActor*> AActorMap;

};