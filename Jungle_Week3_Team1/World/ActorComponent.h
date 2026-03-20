#pragma once
#include "../CoreMinimal.h"
#include "../Object/Object.h"

class AActor;

class UActorComponent : public UObject
{
	friend class AActor;
	DECLARE_OBJECT(UActorComponent, UObject)

protected:
	virtual void TickComponent(float DeltaTime) {};
	// AActor* OwningActor = nullptr;

private:
	bool bIsActive = true;
	bool bAutoActivate = true;
	bool bCanEverTick = true;

	AActor* Owner = nullptr;

public:
	AActor* GetOwner() const { return Owner; }

	template<class T>
	T* GetOwner() const
	{
		return dynamic_cast<T*>(GetOwner());
	}

	virtual void BeginPlay();
	virtual void EndPlay() {};

	virtual void Activate();
	virtual void Deactivate();

	void ExcuteTick(float DeltaTime);
	void SetActive(bool bNewActive);
	inline void SetAutoActivate(bool bNewAutoActivate) { bAutoActivate = bNewAutoActivate; }
	inline void SetComponentTickEnabled(bool bEnabled) { bCanEverTick = bEnabled; }


	inline bool IsActive() { return bIsActive; }
};

