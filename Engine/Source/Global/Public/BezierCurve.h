#pragma once

#include "Global/Types.h"
#include "Global/Function.h"

namespace json { class JSON; }
using JSON = json::JSON;

/**
 * @brief Cubic Bezier 곡선을 나타내는 구조체
 *
 * 4개의 제어점(P0, P1, P2, P3)으로 정의되는 3차 베지어 곡선입니다.
 * 카메라 흔들림의 감쇠 패턴, 애니메이션 타이밍 등에 사용됩니다.
 *
 * 수식: B(t) = (1-t)³P0 + 3(1-t)²tP1 + 3(1-t)t²P2 + t³P3, where t ∈ [0, 1]
 *
 * 사용 예시:
 * ```cpp
 * FCubicBezierCurve Curve = FCubicBezierCurve::CreateEaseInOut();
 * float DecayValue = Curve.Evaluate(0.5f);  // t=0.5에서의 곡선 값
 * ```
 */
struct FCubicBezierCurve
{
public:
	/**
	 * @brief 4개의 제어점 (P0, P1, P2, P3)
	 *
	 * P0: 시작점 (일반적으로 (0, 0))
	 * P1: 첫 번째 제어점 (시작 방향 결정)
	 * P2: 두 번째 제어점 (끝 방향 결정)
	 * P3: 끝점 (일반적으로 (1, 1))
	 */
	FVector2 P[4];

	/**
	 * @brief 기본 생성자 - 선형 곡선 (0,0) → (1,1)
	 */
	FCubicBezierCurve()
	{
		P[0] = FVector2(0.0f, 0.0f);
		P[1] = FVector2(0.33f, 0.33f);
		P[2] = FVector2(0.66f, 0.66f);
		P[3] = FVector2(1.0f, 1.0f);
	}

	/**
	 * @brief 제어점을 명시적으로 지정하는 생성자
	 */
	FCubicBezierCurve(const FVector2& P0, const FVector2& P1, const FVector2& P2, const FVector2& P3)
	{
		P[0] = P0;
		P[1] = P1;
		P[2] = P2;
		P[3] = P3;
	}

	/**
	 * @brief Cubic Bezier 곡선의 특정 t 값에서의 점을 계산
	 *
	 * @param t 파라미터 [0.0, 1.0]
	 * @return 곡선 상의 점 (x, y)
	 *
	 * @note Bernstein 다항식 사용:
	 *       B(t) = (1-t)³P0 + 3(1-t)²tP1 + 3(1-t)t²P2 + t³P3
	 */
	FVector2 Evaluate(float t) const;

	/**
	 * @brief X 값을 입력받아 대응하는 Y 값을 반환 (Newton-Raphson 방법)
	 *
	 * @param x 입력 X 좌표 [0.0, 1.0]
	 * @param iterations Newton-Raphson 반복 횟수 (기본값: 8)
	 * @return 대응하는 Y 좌표
	 *
	 * @note 카메라 흔들림 감쇠에 사용: x는 정규화된 시간(ElapsedTime/Duration),
	 *       반환값은 진폭 배율(0.0=완전감쇠, 1.0=최대진폭)
	 */
	float SampleY(float x, int32 iterations = 8) const;

	/**
	 * @brief JSON 직렬화/역직렬화
	 *
	 * @param bInIsLoading true면 로드, false면 저장
	 * @param InOutHandle JSON 핸들
	 */
	void Serialize(bool bInIsLoading, JSON& InOutHandle);

	// ===== Preset 곡선 팩토리 메서드 =====

	/**
	 * @brief 선형 곡선 (변화 없음)
	 * @return (0,0)-(0.33,0.33)-(0.66,0.66)-(1,1)
	 */
	static FCubicBezierCurve CreateLinear();

	/**
	 * @brief Ease-In 곡선 (천천히 시작)
	 * @return (0,0)-(0.42,0)-(1,1)-(1,1)
	 */
	static FCubicBezierCurve CreateEaseIn();

	/**
	 * @brief Ease-Out 곡선 (천천히 끝남)
	 * @return (0,0)-(0,0)-(0.58,1)-(1,1)
	 */
	static FCubicBezierCurve CreateEaseOut();

	/**
	 * @brief Ease-In-Out 곡선 (S자 형태)
	 * @return (0,0)-(0.42,0)-(0.58,1)-(1,1)
	 */
	static FCubicBezierCurve CreateEaseInOut();

	/**
	 * @brief Bounce 곡선 (튕기는 효과)
	 * @return (0,0)-(0.5,1.5)-(0.8,0.9)-(1,1)
	 */
	static FCubicBezierCurve CreateBounce();

private:
	/**
	 * @brief X 좌표에서 t 파라미터를 찾기 (Newton-Raphson)
	 *
	 * @param x 목표 X 좌표
	 * @param iterations 반복 횟수
	 * @return 대응하는 t 값
	 */
	float SolveForT(float x, int32 iterations) const;

	/**
	 * @brief Cubic Bezier의 X 좌표만 계산
	 */
	float BezierX(float t) const;

	/**
	 * @brief Cubic Bezier의 Y 좌표만 계산
	 */
	float BezierY(float t) const;

	/**
	 * @brief Cubic Bezier의 X에 대한 1차 미분 (dX/dt)
	 */
	float BezierXDerivative(float t) const;
};
