#include "pch.h"
#include "Global/Quaternion.h"

FQuaternion FQuaternion::FromAxisAngle(const FVector& Axis, float AngleRad)
{
	FVector N = Axis;
	N.Normalize();
	float s = sinf(AngleRad * 0.5f);
	return FQuaternion(
		N.X * s,
		N.Y * s,
		N.Z * s,
		cosf(AngleRad * 0.5f)
	);
}

FQuaternion FQuaternion::FromEuler(const FVector& EulerDeg)
{
	// EulerDeg: (X=Roll, Y=Pitch, Z=Yaw) in degrees
	// UE Standard: Yaw → Pitch → Roll (intrinsic rotations)
	FVector Radians = FVector::GetDegreeToRadian(EulerDeg);

	float cr = cosf(Radians.X * 0.5f); // Roll
	float sr = sinf(Radians.X * 0.5f);
	float cp = cosf(Radians.Y * 0.5f); // Pitch
	float sp = sinf(Radians.Y * 0.5f);
	float cy = cosf(Radians.Z * 0.5f); // Yaw
	float sy = sinf(Radians.Z * 0.5f);

	// UE Standard Yaw → Pitch → Roll quaternion composition
	// Reference: UE5 Quat.h MakeFromEuler
	return FQuaternion(
		cr * sp * sy - sr * cp * cy, // X
		-cr * sp * cy - sr * cp * sy, // Y
		cr * cp * sy - sr * sp * cy, // Z
		cr * cp * cy + sr * sp * sy  // W
	);
}

FQuaternion FQuaternion::FromRotationMatrix(const FMatrix& M)
{
    FQuaternion Q;
    float trace = M.Data[0][0] + M.Data[1][1] + M.Data[2][2];
    if (trace > 0)
    {
        float s = 0.5f / sqrtf(trace + 1.0f);
        Q.W = 0.25f / s;
        Q.X = (M.Data[2][1] - M.Data[1][2]) * s;
        Q.Y = (M.Data[0][2] - M.Data[2][0]) * s;
        Q.Z = (M.Data[1][0] - M.Data[0][1]) * s;
    }
    else
    {
        if (M.Data[0][0] > M.Data[1][1] && M.Data[0][0] > M.Data[2][2])
        {
            float s = 2.0f * sqrtf(1.0f + M.Data[0][0] - M.Data[1][1] - M.Data[2][2]);
            Q.W = (M.Data[2][1] - M.Data[1][2]) / s;
            Q.X = 0.25f * s;
            Q.Y = (M.Data[0][1] + M.Data[1][0]) / s;
            Q.Z = (M.Data[0][2] + M.Data[2][0]) / s;
        }
        else if (M.Data[1][1] > M.Data[2][2])
        {
            float s = 2.0f * sqrtf(1.0f + M.Data[1][1] - M.Data[0][0] - M.Data[2][2]);
            Q.W = (M.Data[0][2] - M.Data[2][0]) / s;
            Q.X = (M.Data[0][1] + M.Data[1][0]) / s;
            Q.Y = 0.25f * s;
            Q.Z = (M.Data[1][2] + M.Data[2][1]) / s;
        }
        else
        {
            float s = 2.0f * sqrtf(1.0f + M.Data[2][2] - M.Data[0][0] - M.Data[1][1]);
            Q.W = (M.Data[1][0] - M.Data[0][1]) / s;
            Q.X = (M.Data[0][2] + M.Data[2][0]) / s;
            Q.Y = (M.Data[1][2] + M.Data[2][1]) / s;
            Q.Z = 0.25f * s;
        }
    }
    return Q;
}

FVector FQuaternion::ToEuler() const
{
	// UE Standard conversion: Quaternion → (Roll, Pitch, Yaw)
	// Reference: UE5 UnrealMath.cpp FQuat4f::Rotator()
	FVector Euler;

	// Gimbal Lock singularity detection (Pitch ±90°)
	const float SingularityTest = Z * X - W * Y;
	const float YawY = 2.f * (W * Z + X * Y);
	const float YawX = (1.f - 2.f * (Y * Y + Z * Z));

	const float SINGULARITY_THRESHOLD = 0.4999995f;
	const float RAD_TO_DEG = (180.f) / PI;

	if (SingularityTest < -SINGULARITY_THRESHOLD) // South pole singularity
	{
		Euler.Y = -90.f; // Pitch = -90°
		Euler.Z = atan2f(-X, W) * RAD_TO_DEG * -2.f;
		Euler.X = 0.f; // Roll is lost (Gimbal Lock)
	}
	else if (SingularityTest > SINGULARITY_THRESHOLD) // North pole singularity
	{
		Euler.Y = 90.f; // Pitch = 90°
		Euler.Z = atan2f(X, W) * RAD_TO_DEG * 2.f;
		Euler.X = 0.f; // Roll is lost (Gimbal Lock)
	}
	else // Normal case
	{
		Euler.Y = asinf(2.f * SingularityTest) * RAD_TO_DEG; // Pitch
		Euler.Z = atan2f(YawY, YawX) * RAD_TO_DEG; // Yaw
		Euler.X = atan2f(-2.f * (W * X + Y * Z), (1.f - 2.f * (X * X + Y * Y))) * RAD_TO_DEG; // Roll
	}

	return Euler;
}

FMatrix FQuaternion::ToRotationMatrix() const
{
    FMatrix M;

    const float X2 = X * X;
    const float Y2 = Y * Y;
    const float Z2 = Z * Z;
    const float XY = X * Y;
    const float XZ = X * Z;
    const float YZ = Y * Z;
    const float WX = W * X;
    const float WY = W * Y;
    const float WZ = W * Z;

    M.Data[0][0] = 1.0f - 2.0f * (Y2 + Z2);
    M.Data[0][1] = 2.0f * (XY - WZ);
    M.Data[0][2] = 2.0f * (XZ + WY);
    M.Data[0][3] = 0.0f;

    M.Data[1][0] = 2.0f * (XY + WZ);
    M.Data[1][1] = 1.0f - 2.0f * (X2 + Z2);
    M.Data[1][2] = 2.0f * (YZ - WX);
    M.Data[1][3] = 0.0f;

    M.Data[2][0] = 2.0f * (XZ - WY);
    M.Data[2][1] = 2.0f * (YZ + WX);
    M.Data[2][2] = 1.0f - 2.0f * (X2 + Y2);
    M.Data[2][3] = 0.0f;

    M.Data[3][0] = 0.0f;
    M.Data[3][1] = 0.0f;
    M.Data[3][2] = 0.0f;
    M.Data[3][3] = 1.0f;

    return M;
}

FQuaternion FQuaternion::operator*(const FQuaternion& Q) const
{
	return FQuaternion(
		W * Q.X + X * Q.W + Y * Q.Z - Z * Q.Y,
		W * Q.Y - X * Q.Z + Y * Q.W + Z * Q.X,
		W * Q.Z + X * Q.Y - Y * Q.X + Z * Q.W,
		W * Q.W - X * Q.X - Y * Q.Y - Z * Q.Z
	);
}

void FQuaternion::Normalize()
{
	float mag = sqrtf(X * X + Y * Y + Z * Z + W * W);
	if (mag > 0.0001f)
	{
		X /= mag;
		Y /= mag;
		Z /= mag;
		W /= mag;
	}
}

FQuaternion FQuaternion::MakeFromDirection(const FVector& Direction)
{
	const FVector& ForwardVector = FVector::ForwardVector();
	FVector Dir = Direction.GetNormalized();

	float Dot = ForwardVector.Dot(Dir);
	if (Dot == 1.f) { return Identity(); }

	if (Dot == -1.f)
	{
		// 180도 회전
		FVector RotAxis = FVector::UpVector().Cross(ForwardVector);
		if (RotAxis.IsZero()) // Forward가 UP 벡터와 평행하면 다른 축 사용
		{
			RotAxis = FVector::ForwardVector().Cross(ForwardVector);
		}
		return FromAxisAngle(RotAxis.GetNormalized(), PI);
	}

	float AngleRad = acos(Dot);

	// 두 벡터에 수직인 회전축 계산 후 쿼터니언 생성
	FVector Axis = Dir.Cross(ForwardVector);
	Axis.Normalize();
	return FromAxisAngle(Axis, AngleRad);
}

FVector FQuaternion::RotateVector(const FQuaternion& q, const FVector& v)
{
	FQuaternion p(v.X, v.Y, v.Z, 0.0f);
	FQuaternion r = q * p * q.Inverse();
	return FVector(r.X, r.Y, r.Z);
}

FVector FQuaternion::RotateVector(const FVector& V) const
{
	const FVector Q(X, Y, Z);
	const FVector TT = V.Cross(Q) * 2.f;
	const FVector Result = V + (TT * W) + TT.Cross(Q);
	return Result;
}
FVector FQuaternion::GetForward() const
{
	return RotateVector(FVector(1, 0, 0));
}
FVector FQuaternion::GetRight() const
{
	return RotateVector(FVector(0, 1, 0));
}
FVector FQuaternion::GetUp() const
{
	return RotateVector(FVector(0, 0, 1));
}
float FQuaternion::Dot(const FQuaternion& Q) const
{
	return X * Q.X + Y * Q.Y + Z * Q.Z + W * Q.W;
}

FQuaternion FQuaternion::Slerp(const FQuaternion& Q1, const FQuaternion& Q2, float T)
{
	// T를 0~1 범위로 클램프
	T = Clamp(T, 0.0f, 1.0f);

	FQuaternion Start = Q1;
	FQuaternion End = Q2;

	// 내적 계산
	float CosHalfTheta = Start.Dot(End);

	// 최단 경로를 위해 부호 조정 (Dot < 0이면 반대 방향)
	if (CosHalfTheta < 0.0f)
	{
		End.X = -End.X;
		End.Y = -End.Y;
		End.Z = -End.Z;
		End.W = -End.W;
		CosHalfTheta = -CosHalfTheta;
	}

	// 쿼터니언이 거의 같으면 선형 보간
	if (CosHalfTheta >= 0.9995f)
	{
		return FQuaternion(
			Start.X + T * (End.X - Start.X),
			Start.Y + T * (End.Y - Start.Y),
			Start.Z + T * (End.Z - Start.Z),
			Start.W + T * (End.W - Start.W)
		);
	}

	// 구면 선형 보간
	float HalfTheta = acosf(CosHalfTheta);
	float SinHalfTheta = sqrtf(1.0f - CosHalfTheta * CosHalfTheta);

	// 각도가 180도에 가까우면 (SinHalfTheta가 0에 가까우면)
	if (fabsf(SinHalfTheta) < 0.001f)
	{
		// 50:50 혼합
		return FQuaternion(
			Start.X * 0.5f + End.X * 0.5f,
			Start.Y * 0.5f + End.Y * 0.5f,
			Start.Z * 0.5f + End.Z * 0.5f,
			Start.W * 0.5f + End.W * 0.5f
		);
	}

	// 보간 계수 계산
	float RatioA = sinf((1.0f - T) * HalfTheta) / SinHalfTheta;
	float RatioB = sinf(T * HalfTheta) / SinHalfTheta;

	FQuaternion Result(
		Start.X * RatioA + End.X * RatioB,
		Start.Y * RatioA + End.Y * RatioB,
		Start.Z * RatioA + End.Z * RatioB,
		Start.W * RatioA + End.W * RatioB
	);

	Result.Normalize();
	return Result;
}

FQuaternion FQuaternion::SlerpFullPath(const FQuaternion& Q1, const FQuaternion& Q2, float T)
{
	// 최단 경로가 아닌 전체 경로로 보간 (360도 회전용)
	T = Clamp(T, 0.0f, 1.0f);

	FQuaternion Start = Q1;
	FQuaternion End = Q2;

	float CosHalfTheta = Start.Dot(End);

	// 쿼터니언이 거의 같으면 선형 보간
	if (fabsf(CosHalfTheta) >= 0.9995f)
	{
		return FQuaternion(
			Start.X + T * (End.X - Start.X),
			Start.Y + T * (End.Y - Start.Y),
			Start.Z + T * (End.Z - Start.Z),
			Start.W + T * (End.W - Start.W)
		);
	}

	float HalfTheta = acosf(Clamp(CosHalfTheta, -1.0f, 1.0f));
	float SinHalfTheta = sqrtf(1.0f - CosHalfTheta * CosHalfTheta);

	if (fabsf(SinHalfTheta) < 0.001f)
	{
		return FQuaternion(
			Start.X * 0.5f + End.X * 0.5f,
			Start.Y * 0.5f + End.Y * 0.5f,
			Start.Z * 0.5f + End.Z * 0.5f,
			Start.W * 0.5f + End.W * 0.5f
		);
	}

	float RatioA = sinf((1.0f - T) * HalfTheta) / SinHalfTheta;
	float RatioB = sinf(T * HalfTheta) / SinHalfTheta;

	FQuaternion Result(
		Start.X * RatioA + End.X * RatioB,
		Start.Y * RatioA + End.Y * RatioB,
		Start.Z * RatioA + End.Z * RatioB,
		Start.W * RatioA + End.W * RatioB
	);

	Result.Normalize();
	return Result;
}