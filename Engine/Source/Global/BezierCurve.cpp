#include "pch.h"
#include "Global/Public/BezierCurve.h"
#include <json.hpp>
#include <cmath>

FVector2 FCubicBezierCurve::Evaluate(float t) const
{
	// Clamp t to [0, 1]
	t = Clamp(t, 0.0f, 1.0f);

	// Bernstein polynomials for Cubic Bezier
	// B(t) = (1-t)³P0 + 3(1-t)²tP1 + 3(1-t)t²P2 + t³P3
	const float OneMinusT = 1.0f - t;
	const float OneMinusT2 = OneMinusT * OneMinusT;
	const float OneMinusT3 = OneMinusT2 * OneMinusT;
	const float t2 = t * t;
	const float t3 = t2 * t;

	const float b0 = OneMinusT3;
	const float b1 = 3.0f * OneMinusT2 * t;
	const float b2 = 3.0f * OneMinusT * t2;
	const float b3 = t3;

	FVector2 Result;
	Result.X = b0 * P[0].X + b1 * P[1].X + b2 * P[2].X + b3 * P[3].X;
	Result.Y = b0 * P[0].Y + b1 * P[1].Y + b2 * P[2].Y + b3 * P[3].Y;

	return Result;
}

float FCubicBezierCurve::SampleY(float x, int32 iterations) const
{
	// Clamp x to [0, 1]
	x = Clamp(x, 0.0f, 1.0f);

	// Newton-Raphson 방법으로 X에 대응하는 t 찾기
	float t = SolveForT(x, iterations);

	// t로부터 Y 계산
	return BezierY(t);
}

void FCubicBezierCurve::Serialize(bool bInIsLoading, JSON& InOutHandle)
{
	if (bInIsLoading)
	{
		// Load control points
		if (InOutHandle.hasKey("P0"))
		{
			P[0].X = static_cast<float>(InOutHandle["P0"]["x"].ToFloat());
			P[0].Y = static_cast<float>(InOutHandle["P0"]["y"].ToFloat());
		}
		if (InOutHandle.hasKey("P1"))
		{
			P[1].X = static_cast<float>(InOutHandle["P1"]["x"].ToFloat());
			P[1].Y = static_cast<float>(InOutHandle["P1"]["y"].ToFloat());
		}
		if (InOutHandle.hasKey("P2"))
		{
			P[2].X = static_cast<float>(InOutHandle["P2"]["x"].ToFloat());
			P[2].Y = static_cast<float>(InOutHandle["P2"]["y"].ToFloat());
		}
		if (InOutHandle.hasKey("P3"))
		{
			P[3].X = static_cast<float>(InOutHandle["P3"]["x"].ToFloat());
			P[3].Y = static_cast<float>(InOutHandle["P3"]["y"].ToFloat());
		}
	}
	else
	{
		// Save control points
		JSON P0Handle;
		P0Handle["x"] = P[0].X;
		P0Handle["y"] = P[0].Y;
		InOutHandle["P0"] = P0Handle;

		JSON P1Handle;
		P1Handle["x"] = P[1].X;
		P1Handle["y"] = P[1].Y;
		InOutHandle["P1"] = P1Handle;

		JSON P2Handle;
		P2Handle["x"] = P[2].X;
		P2Handle["y"] = P[2].Y;
		InOutHandle["P2"] = P2Handle;

		JSON P3Handle;
		P3Handle["x"] = P[3].X;
		P3Handle["y"] = P[3].Y;
		InOutHandle["P3"] = P3Handle;
	}
}

// ===== Preset Factory Methods =====

FCubicBezierCurve FCubicBezierCurve::CreateLinear()
{
	return FCubicBezierCurve(
		FVector2(0.0f, 0.0f),
		FVector2(0.33f, 0.33f),
		FVector2(0.66f, 0.66f),
		FVector2(1.0f, 1.0f)
	);
}

FCubicBezierCurve FCubicBezierCurve::CreateEaseIn()
{
	return FCubicBezierCurve(
		FVector2(0.0f, 0.0f),
		FVector2(0.42f, 0.0f),
		FVector2(1.0f, 1.0f),
		FVector2(1.0f, 1.0f)
	);
}

FCubicBezierCurve FCubicBezierCurve::CreateEaseOut()
{
	return FCubicBezierCurve(
		FVector2(0.0f, 0.0f),
		FVector2(0.0f, 0.0f),
		FVector2(0.58f, 1.0f),
		FVector2(1.0f, 1.0f)
	);
}

FCubicBezierCurve FCubicBezierCurve::CreateEaseInOut()
{
	return FCubicBezierCurve(
		FVector2(0.0f, 0.0f),
		FVector2(0.42f, 0.0f),
		FVector2(0.58f, 1.0f),
		FVector2(1.0f, 1.0f)
	);
}

FCubicBezierCurve FCubicBezierCurve::CreateBounce()
{
	return FCubicBezierCurve(
		FVector2(0.0f, 0.0f),
		FVector2(0.5f, 1.5f),
		FVector2(0.8f, 0.9f),
		FVector2(1.0f, 1.0f)
	);
}

// ===== Private Helper Methods =====

float FCubicBezierCurve::SolveForT(float x, int32 iterations) const
{
	// Newton-Raphson 방법으로 X에 대응하는 t를 찾는다
	// f(t) = BezierX(t) - x = 0을 풀기

	// 초기 추정: linear approximation
	float t = x;

	for (int32 i = 0; i < iterations; ++i)
	{
		float currentX = BezierX(t);
		float derivative = BezierXDerivative(t);

		// 미분값이 0에 가까우면 더 이상 개선 불가
		if (std::abs(derivative) < 1e-6f)
			break;

		// Newton-Raphson: t_new = t_old - f(t) / f'(t)
		float error = currentX - x;
		t -= error / derivative;

		// t를 [0, 1] 범위로 clamp
		t = Clamp(t, 0.0f, 1.0f);

		// 수렴 체크 (오차가 충분히 작으면 종료)
		if (std::abs(error) < 1e-6f)
			break;
	}

	return t;
}

float FCubicBezierCurve::BezierX(float t) const
{
	const float OneMinusT = 1.0f - t;
	const float OneMinusT2 = OneMinusT * OneMinusT;
	const float OneMinusT3 = OneMinusT2 * OneMinusT;
	const float t2 = t * t;
	const float t3 = t2 * t;

	const float b0 = OneMinusT3;
	const float b1 = 3.0f * OneMinusT2 * t;
	const float b2 = 3.0f * OneMinusT * t2;
	const float b3 = t3;

	return b0 * P[0].X + b1 * P[1].X + b2 * P[2].X + b3 * P[3].X;
}

float FCubicBezierCurve::BezierY(float t) const
{
	const float OneMinusT = 1.0f - t;
	const float OneMinusT2 = OneMinusT * OneMinusT;
	const float OneMinusT3 = OneMinusT2 * OneMinusT;
	const float t2 = t * t;
	const float t3 = t2 * t;

	const float b0 = OneMinusT3;
	const float b1 = 3.0f * OneMinusT2 * t;
	const float b2 = 3.0f * OneMinusT * t2;
	const float b3 = t3;

	return b0 * P[0].Y + b1 * P[1].Y + b2 * P[2].Y + b3 * P[3].Y;
}

float FCubicBezierCurve::BezierXDerivative(float t) const
{
	// dX/dt = -3(1-t)²P0 + 3(1-t)²P1 - 6t(1-t)P1 + 6t(1-t)P2 - 3t²P2 + 3t²P3
	// 간소화: dX/dt = 3[(1-t)²(P1-P0) + 2t(1-t)(P2-P1) + t²(P3-P2)]

	const float OneMinusT = 1.0f - t;
	const float OneMinusT2 = OneMinusT * OneMinusT;
	const float t2 = t * t;

	const float term1 = OneMinusT2 * (P[1].X - P[0].X);
	const float term2 = 2.0f * t * OneMinusT * (P[2].X - P[1].X);
	const float term3 = t2 * (P[3].X - P[2].X);

	return 3.0f * (term1 + term2 + term3);
}
