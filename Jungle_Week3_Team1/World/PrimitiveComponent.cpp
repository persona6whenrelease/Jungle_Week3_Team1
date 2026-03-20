#include "PrimitiveComponent.h"
#include "../Types/RayTypes.h"
#include "Mesh/MeshManager.h"


void UPrimitiveComponent::UpdateWorldAABB()
{
	// 현재 x축 방향으로의 투영
	FVector LExt = LocalExtents;

	FMatrix worldMatrix = GetWorldMatrix();

	float NewEx = std::abs(worldMatrix.M[0][0]) * LExt.X + std::abs(worldMatrix.M[1][0]) * LExt.Y + std::abs(worldMatrix.M[2][0]) * LExt.Z;
	float NewEy = std::abs(worldMatrix.M[0][1]) * LExt.X + std::abs(worldMatrix.M[1][1]) * LExt.Y + std::abs(worldMatrix.M[2][1]) * LExt.Z;
	float NewEz = std::abs(worldMatrix.M[0][2]) * LExt.X + std::abs(worldMatrix.M[1][2]) * LExt.Y + std::abs(worldMatrix.M[2][2]) * LExt.Z;

	FVector WorldCenter = GetWorldLocation();
	WorldAABBMinLocation = WorldCenter - FVector(NewEx, NewEy, NewEz);
	WorldAABBMaxLocation = WorldCenter + FVector(NewEx, NewEy, NewEz);
}

bool UPrimitiveComponent::CheckAABB(const FRay& Ray)
{
	float tMin = -INFINITY;
	float tMax = INFINITY;

	for (int i = 0; i < 3; ++i)
	{
		float invDir = 1.0f / (i == 0 ? Ray.Direction.X : (i == 1 ? Ray.Direction.Y : Ray.Direction.Z));
		float origin = (i == 0 ? Ray.Origin.X : (i == 1 ? Ray.Origin.Y : Ray.Origin.Z));
		float minBound = (i == 0 ? WorldAABBMinLocation.X : (i == 1 ? WorldAABBMinLocation.Y : WorldAABBMinLocation.Z));
		float maxBound = (i == 0 ? WorldAABBMaxLocation.X : (i == 1 ? WorldAABBMaxLocation.Y : WorldAABBMaxLocation.Z));

		float t1 = (minBound - origin) * invDir;
		float t2 = (maxBound - origin) * invDir;

		if (t1 > t2) std::swap(t1, t2);

		tMin = std::max(tMin, t1);
		tMax = std::min(tMax, t2);

		if (tMin > tMax) return false;
	}

	return tMax >= 0;
}

bool UPrimitiveComponent::Raycast(const FRay& Ray, FHitResult& OutHitResult)
{

	if (!CheckAABB(Ray))
	{
		return false;
	}

	return RaycastMesh(Ray, OutHitResult);
}

bool UPrimitiveComponent::IntersectTriangle(const FVector& RayOrigin, const FVector& RayDir, const FVector& V0,
	const FVector& V1, const FVector& V2, float& OutT)
{
	FVector edge1 = V1 - V0;
	FVector edge2 = V2 - V0;
	FVector pvec = FVector::CrossProduct(RayDir, edge2);
	float det = FVector::DotProduct(edge1, pvec);

	if (std::abs(det) < 0.0001f) return false;

	float invDet = 1.0f / det;
	FVector tvec = RayOrigin - V0;
	float u = FVector::DotProduct(tvec, pvec) * invDet;

	if (u < 0.0f || u > 1.0f) return false;

	FVector qvec = FVector::CrossProduct(tvec, edge1);
	float v = FVector::DotProduct(RayDir, qvec) * invDet;

	if (v < 0.0f || u + v > 1.0f) return false;

	OutT = FVector::DotProduct(edge2, qvec) * invDet;
	return OutT > 0.0f;
}

bool UPrimitiveComponent::RaycastMesh(const FRay& Ray, FHitResult& OutHitResult)
{
	if (!MeshData || MeshData->Indices.empty())
	{
		return false;
	}

	FMatrix invWorld = CachedWorldMatrix.GetInverse();
	FVector localOrigin = invWorld.TransformPosition(Ray.Origin);
	FVector localDirection = invWorld.TransformVector(Ray.Direction).GetSafeNormal();

	bool bHit = false;
	float closestT = FLT_MAX;

	for (size_t i = 0; i < MeshData->Indices.size(); i += 3)
	{
		FVector v0 = MeshData->Vertices[MeshData->Indices[i]].Position;
		FVector v1 = MeshData->Vertices[MeshData->Indices[i + 1]].Position;
		FVector v2 = MeshData->Vertices[MeshData->Indices[i + 2]].Position;

		float t = 0.0f;

		if (IntersectTriangle(localOrigin, localDirection, v0, v1, v2, t))
		{
			if (t > 0.0f && t < closestT)
			{
				closestT = t;
				bHit = true;
				OutHitResult.FaceIndex = i;
			}
		}
	}

	OutHitResult.bHit = bHit;

	if (bHit)
	{
		FVector localHitPoint = localOrigin + (localDirection * closestT);
		FVector worldHitPoint = CachedWorldMatrix.TransformPosition(localHitPoint);
		OutHitResult.Distance = FVector::Dist(Ray.Origin, worldHitPoint);
		OutHitResult.HitComponent = this;
		return true;
	}

	return false;
}

void UPrimitiveComponent::UpdateWorldMatrix()
{
	USceneComponent::UpdateWorldMatrix();

	UpdateWorldAABB();
}


UCubeComponent::UCubeComponent()
{
	MeshData = &FMeshManager::GetCube();
}

