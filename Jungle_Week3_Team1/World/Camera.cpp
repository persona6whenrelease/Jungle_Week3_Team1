#include "Camera.h"
#include "SceneComponent.h"

void UCamera::SetRelativeLocation(const FVector newlocation) {
	USceneComponent::SetRelativeLocation(newlocation);
	bViewDirty = true;
}

void UCamera::SetRelativeRotation(const FQuat newrotation) {
	USceneComponent::SetRelativeRotation(newrotation);
	bViewDirty = true;
}

const FMatrix& UCamera::GetViewMatrix() {
	if (bUpdateFlag || bViewDirty) {
		RebuildView();
		bViewDirty = false;
		bUpdateFlag = false;
	}
	return CachedView;
}

const FMatrix& UCamera::GetProjectionMatrix() {
	if (bUpdateFlag || bProjectionDirty) {
		RebuildProjection();
		bProjectionDirty = false;
	}
	return CachedProjection;
}

const FMatrix& UCamera::GetViewProjection() {
	// GetViewMatrix/GetProjectionMatrix handle their own dirty flags now
	CachedVP = GetViewMatrix() * GetProjectionMatrix();
	return CachedVP;
}

void UCamera::BuildLookAtRotation() {
	//FVector diff = (CameraState.InitLook - CameraState.Location).Normalized();

	//RelativeRotation.X = asinf(diff.Y);
	//RelativeRotation.Y = atan2f(diff.X, diff.Z) * (180.0f / M_PI);
	//RelativeRotation.Z = 0.0f;

	//CameraState.Rotation = RelativeRotation;
}

void UCamera::LookAt(const FVector& target) {
	FVector Position = GetWorldLocation();
	FVector diff = (target - Position).GetSafeNormal();

	FMatrix LookAtMatrix = FMatrix::MakeFromX(diff);
	RelativeRotation = FTransform(LookAtMatrix).GetRotation();

	SetRelativeRotation(RelativeRotation);
	bViewDirty = true;
}

void UCamera::OnResize(int w, int h) {
	AspectRatio = ((float)w) / ((float)h);
	CameraState.AspectRatio = AspectRatio;
	bProjectionDirty = true;
}

void UCamera::SetFOV(float newFOV) {
	FOV = newFOV;
	bProjectionDirty = true;
}

float UCamera::GetFOV() const {
	return FOV;
}

float UCamera::GetNearPlane() const {
	return NearZ;
}

float UCamera::GetFarPlane() const {
	return FarZ;
}

void UCamera::SyncStateLookAt()
{
	//BuildLookAtRotation();
}

void UCamera::ApplyCameraState()
{
	SetProjectionMode(CameraState.bIsOrthogonal ? EProjectionMode::Orthographic : EProjectionMode::Perspective);
	SetFOV(CameraState.FOV);

	AspectRatio = CameraState.AspectRatio;
	NearZ = CameraState.NearZ;
	FarZ = CameraState.FarZ;
	OrthoWidth = CameraState.OrthoWidth;
	bProjectionDirty = true;

	//SetWorldLocation(CameraState.Location);
	//SetRelativeRotation(CameraState.Rotation);
}

//	Camera 상태 갱신을 이로 통일
void UCamera::SetCameraState(const FCameraState& NewState)
{
	CameraState = NewState;
	ApplyCameraState();
}

void UCamera::RebuildView() {
	FVector Eye = GetWorldLocation();
	FVector Forward = GetForwardVector();
	FVector Up = GetUpVector();

	FVector Target = Eye + Forward;
	CachedView = FMatrix::MakeViewLookAtLH(Eye, Target, Up);
}


void UCamera::SetProjectionMode(EProjectionMode mode) {
	ProjectionMode = mode;

	bProjectionDirty = true;
}

void UCamera::RebuildProjection() {
	if (ProjectionMode == EProjectionMode::Perspective)
	{
		CachedProjection = FMatrix::MakePerspectiveFovLH(FOV, AspectRatio, NearZ, FarZ);
	}
	else if (ProjectionMode == EProjectionMode::Orthographic)
	{
		float OrthoHeight = OrthoWidth / AspectRatio;
		CachedProjection = FMatrix::MakeOrthographicLH(OrthoWidth, OrthoHeight, NearZ, FarZ);
	}
}

FRay UCamera::DeprojectScreenToWorld(float MouseX, float MouseY, float ScreenWidth, float ScreenHeight) {
	float ndcX = (2.0f * MouseX) / ScreenWidth - 1.0f;
	float ndcY = 1.0f - (2.0f * MouseY) / ScreenHeight;

	FVector ndcNear(ndcX, ndcY, 0.0f);
	FVector ndcFar(ndcX, ndcY, 1.0f);

	FMatrix viewProj = GetViewMatrix() * GetProjectionMatrix();
	FMatrix inverseViewProjection = viewProj.GetInverse();

	FVector worldNear = inverseViewProjection.TransformPosition(ndcNear);
	FVector worldFar = inverseViewProjection.TransformPosition(ndcFar);

	FRay ray;
	ray.Origin = worldNear;

	ray.Direction = (worldFar - worldNear).GetSafeNormal();

	return ray;
}
