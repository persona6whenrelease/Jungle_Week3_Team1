#pragma once
#pragma once

// #include "Object/ObjectFactory.h"
#include "SceneComponent.h"
// #include "Render/Scene/RenderBus.h"
// #include "Render/Common/RenderTypes.h"
#include "../Types/RayTypes.h"

#include "../Types/VertexTypes.h"
#include "../Render/RenderTypes.h"


class UPrimitiveComponent : public USceneComponent
{
private:
	FVector WorldAABBMinLocation;
	FVector WorldAABBMaxLocation;

protected:
	const FMeshData* MeshData = nullptr;
	FVector LocalExtents = { 0.5f, 0.5f, 0.5f };
	bool bIsVisible = true;

public:
	inline const FMeshData* GetMeshData() const { return MeshData; };

	inline void SetVisibility(bool bVisible) { bIsVisible = bVisible; }

	//Collision
	void UpdateWorldAABB();
	bool CheckAABB(const FRay& Ray);
	bool Raycast(const FRay& Ray, FHitResult& OutHitResult);
	bool IntersectTriangle(const FVector& RayOrigin, const FVector& RayDir, const FVector& V0, const FVector& V1, const FVector& V2, float& OutT);
	virtual bool RaycastMesh(const FRay& Ray, FHitResult& OutHitResult);
	inline bool IsVisible() const { return bIsVisible; }

	void UpdateWorldMatrix() override;

	virtual EPrimitiveType GetPrimitiveType() const = 0;
};

class UCubeComponent : public UPrimitiveComponent
{
private:

public:
	UCubeComponent();

	static constexpr EPrimitiveType PrimitiveType = EPrimitiveType::EPT_Cube;

	EPrimitiveType GetPrimitiveType() const override { return PrimitiveType; }
};

class USphereComponent : public UPrimitiveComponent
{
private:

public:
	USphereComponent();

	static constexpr EPrimitiveType PrimitiveType = EPrimitiveType::EPT_Sphere;

	EPrimitiveType GetPrimitiveType() const override { return PrimitiveType; }
};

class UPlaneComponent : public UPrimitiveComponent
{
private:

public:
	UPlaneComponent();

	static constexpr EPrimitiveType PrimitiveType = EPrimitiveType::EPT_Plane;

	EPrimitiveType GetPrimitiveType() const override { return PrimitiveType; }
};

class UGizmoComponent : public UPrimitiveComponent
{
private:

public:
	UGizmoComponent();

	static constexpr EPrimitiveType PrimitiveType = EPrimitiveType::EPT_TransGizmo;

	EPrimitiveType GetPrimitiveType() const override { return PrimitiveType; }
};