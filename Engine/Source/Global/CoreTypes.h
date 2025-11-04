#pragma once
#include "Global/Vector.h"
#include "Global/Matrix.h"
#include "Global/Quaternion.h"
#include "Global/Types.h"
#include "Core/Public/Name.h"
#include <Texture/Public/Material.h>

//struct BatchLineContants
//{
//	float CellSize;
//	//FMatrix BoundingBoxModel;
//	uint32 ZGridStartIndex; // 인덱스 버퍼에서, z방향쪽 그리드가 시작되는 인덱스
//	uint32 BoundingBoxStartIndex; // 인덱스 버퍼에서, 바운딩박스가 시작되는 인덱스
//};

struct FCameraConstants
{
	FCameraConstants() : NearClip(0), FarClip(0)
	{
		View = FMatrix::Identity();
		Projection = FMatrix::Identity();
	}

	FMatrix View;
	FMatrix Projection;
	FVector ViewWorldLocation;
	float NearClip;
	float FarClip;
};

/**
 * @brief 카메라 POV 정보 (카메라 모디파이어 체인에서 사용)
 */
struct FMinimalViewInfo
{
	// 변환
	FVector Location;
	FQuaternion Rotation;

	// 투영 파라미터
	float FOV;                      // 수직 시야각 (도 단위)
	float AspectRatio;              // 너비 / 높이
	float NearClipPlane;
	float FarClipPlane;
	float OrthoWidth;               // 직교 투영용
	bool bUsePerspectiveProjection; // true: 원근 투영, false: 직교 투영

	// 기본 생성자
	FMinimalViewInfo()
		: Location(FVector::ZeroVector())
		, Rotation(FQuaternion::Identity())
		, FOV(90.0f)
		, AspectRatio(16.0f / 9.0f)
		, NearClipPlane(1.0f)
		, FarClipPlane(10000.0f)
		, OrthoWidth(1000.0f)
		, bUsePerspectiveProjection(true)
	{
	}

	// FCameraConstants로 변환
	FCameraConstants ToCameraConstants() const
	{
		FCameraConstants Result;

		// View 행렬 구성 (월드 → 뷰 공간)
		FMatrix Translation = FMatrix::TranslationMatrixInverse(Location);

		// 회전 쿼터니언에서 기저 벡터 얻기
		// 월드 공간 축 (엔진 좌표계: X-전방, Y-우측, Z-상단)
		FVector WorldForward(1, 0, 0);
		FVector WorldRight(0, 1, 0);
		FVector WorldUp(0, 0, 1);

		// 카메라 회전으로 월드 축을 회전하여 카메라 축 얻기
		FVector Forward = Rotation.RotateVector(WorldForward);
		FVector Right = Rotation.RotateVector(WorldRight);
		FVector Up = Rotation.RotateVector(WorldUp);

		// 기저 벡터로 회전 행렬 구성
		FMatrix RotationMatrix(Right, Up, Forward);
		RotationMatrix = RotationMatrix.Transpose();

		Result.View = Translation * RotationMatrix;

		// Projection 행렬 구성
		if (bUsePerspectiveProjection)
		{
			// 원근 투영 (수동 계산, 왼손 좌표계)
			float FovYRad = FOV * (PI / 180.0f);
			float f = 1.0f / tan(FovYRad / 2.0f);

			Result.Projection = FMatrix::Identity();
			Result.Projection.Data[0][0] = f / AspectRatio;
			Result.Projection.Data[1][1] = f;
			Result.Projection.Data[2][2] = FarClipPlane / (FarClipPlane - NearClipPlane);
			Result.Projection.Data[2][3] = 1.0f;
			Result.Projection.Data[3][2] = (-NearClipPlane * FarClipPlane) / (FarClipPlane - NearClipPlane);
			Result.Projection.Data[3][3] = 0.0f;
		}
		else
		{
			// 직교 투영
			float Width = OrthoWidth;
			float Height = OrthoWidth / AspectRatio;
			Result.Projection = FMatrix::CreateOrthoLH(
				-Width / 2.0f, Width / 2.0f,
				-Height / 2.0f, Height / 2.0f,
				NearClipPlane, FarClipPlane
			);
		}

		Result.ViewWorldLocation = Location;
		Result.NearClip = NearClipPlane;
		Result.FarClip = FarClipPlane;

		return Result;
	}
};

#define HAS_DIFFUSE_MAP	 (1 << 0)
#define HAS_AMBIENT_MAP	 (1 << 1)
#define HAS_SPECULAR_MAP (1 << 2)
#define HAS_NORMAL_MAP	 (1 << 3)
#define HAS_ALPHA_MAP	 (1 << 4)
#define HAS_BUMP_MAP	 (1 << 5)

struct FMaterialConstants
{
	FVector4 Ka;
	FVector4 Kd;
	FVector4 Ks;
	FVector4 Ke;
	float Ns;
	float Ni;
	float D;
	uint32 MaterialFlags;
	float Time; // Time in seconds
};

struct FVertex
{
	FVector Position;
	FVector4 Color;
};

struct FNormalVertex
{
	FVector Position;
	FVector Normal;
	FVector4 Color;
	FVector2 TexCoord;
	FVector4 Tangent;  // XYZ: Tangent, W: Handedness(+1/-1)
};

struct FRay
{
	FVector4 Origin;
	FVector4 Direction;
};

/**
 * @brief Render State Settings for Actor's Component
 */
struct FRenderState
{
	ECullMode CullMode = ECullMode::None;
	EFillMode FillMode = EFillMode::Solid;
};

/**
 * @brief 변환 정보를 담는 구조체
 */
struct FTransform
{
	FVector Location = FVector(0.0f, 0.0f, 0.0f);
	FVector Rotation = FVector(0.0f, 0.0f, 0.0f);
	FVector Scale = FVector(1.0f, 1.0f, 1.0f);

	FTransform() = default;

	FTransform(const FVector& InLocation, const FVector& InRotation = FVector::ZeroVector(),
		const FVector& InScale = FVector::OneVector())
		: Location(InLocation), Rotation(InRotation), Scale(InScale)
	{
	}
};

/**
 * @brief 2차원 좌표의 정보를 담는 구조체
 */
struct FPoint
{
	INT X = 0;
	INT Y = 0;
	constexpr FPoint(LONG InX, LONG InY) : X(InX), Y(InY)
	{
	}
};

/**
 * @brief 윈도우를 비롯한 2D 화면의 정보를 담는 구조체
 */
struct FRect
{
	LONG Left = 0;
	LONG Top = 0;
	LONG Width = 0;
	LONG Height = 0;

	LONG GetRight() const { return Left + Width; }
	LONG GetBottom() const { return Top + Height; }
};

struct FAmbientLightInfo
{
	FVector4 Color;
	float Intensity;
	FVector Padding;
};

struct FDirectionalLightInfo
{
	FVector4 Color;
	FVector Direction;
	float Intensity;

	// Shadow parameters
	// FMatrix LightViewProjection;
	uint32 CastShadow;           // 0 or 1
	uint32 ShadowModeIndex;
	float ShadowBias;
	float ShadowSlopeBias;
	float ShadowSharpen;
	float Resolution;
	FVector2 Padding;
};

//StructuredBuffer: 16-byte alignment required (FVector4 alignment)
struct FPointLightInfo
{
	FVector4 Color;
	FVector Position;
	float Intensity;
	float Range;
	float DistanceFalloffExponent;

	// Shadow parameters
	uint32 CastShadow;
	uint32 ShadowModeIndex;
	float ShadowBias;
	float ShadowSlopeBias;
	float ShadowSharpen;
	float Resolution;
};

//StructuredBuffer padding 없어도됨
struct FSpotLightInfo
{
	// Point Light와 공유하는 속성 (필드 순서 맞춤)
	FVector4 Color;
	FVector Position;
	float Intensity;
	float Range;
	float DistanceFalloffExponent;

	// SpotLight 고유 속성
	float InnerConeAngle;
	float OuterConeAngle;
	float AngleFalloffExponent;
	FVector Direction;

	// Shadow parameters
	FMatrix LightViewProjection;
	uint32 CastShadow;
	uint32 ShadowModeIndex;
	float ShadowBias;
	float ShadowSlopeBias;
	float ShadowSharpen;
	float Resolution;
	FVector2 Padding;
};

struct FGlobalLightConstant
{
	FAmbientLightInfo Ambient;
	FDirectionalLightInfo Directional;
};
