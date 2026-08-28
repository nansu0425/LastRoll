# Emissive 미지원 엔진에서 빛나는 투사체 만들기

> 게임잼 주차(2025-10-31 ~ 11-06) 작업. 플레이어 주위를 공전하는 `OrbitProjectile`(태양 메시)을 "빛나는" 오브젝트로 보이게 만든 작업입니다.

## 문제

공전 투사체를 광원처럼 빛나는 태양으로 연출하고 싶었는데, 당시 FutureEngine의 렌더링 파이프라인에는 그럴 수단이 없었습니다.

- Material에 **emissive 항이 없음** — Blinn-Phong 조명(ambient/diffuse/specular)만 계산하므로, 빛을 받지 못하는 방향의 면은 무조건 어두워짐
- **Bloom / HDR 파이프라인 없음** — scene color가 `B8G8R8A8_UNORM`(LDR)이고 post-process는 감마·비네트 수준. 1.0을 넘는 밝기를 저장할 수도, 화면 공간에서 번지게 할 수도 없음

즉 "스스로 밝게 빛나는 표면"과 "주변이 그 빛을 받는 효과"를 둘 다 다른 방법으로 만들어야 했습니다.

## 해결 — 세 가지 조합

### 1. 렌더링 파이프라인에 emissive 항 추가 (커밋 `b1f9a93`)

Material 데이터부터 셰이더까지 emissive를 관통시켰습니다.

- [`Material.h`](../Engine/Source/Texture/Public/Material.h) — MTL의 `Ke`에 대응하는 `FVector Ke` 필드와 `Get/SetEmissiveColor` 추가
- material constant buffer(`FMaterialConstants`, `b2` 슬롯)에 `Ke` 추가 — [`StaticMeshPass.cpp`](../Engine/Source/Render/RenderPass/Private/StaticMeshPass.cpp)가 material이 바뀔 때 함께 업로드
- [`UberLit.hlsl`](../Engine/Asset/Shader/UberLit.hlsl) — 조명 결과에 emissive를 반영:

```hlsl
// Treat Ke as additional light source for diffuse (so texture glows with Ke color)
float3 totalDiffuseLight = Illumination.Diffuse.rgb + Ke.rgb;
finalPixel.rgb = Illumination.Ambient.rgb * ambientColor.rgb + totalDiffuseLight * diffuseColor.rgb + Illumination.Specular.rgb * specularColor.rgb;
```

`Ke`를 최종 색에 그냥 더하는 대신 **diffuse light 항에 더했습니다**. emissive가 diffuse 텍스처 색과 곱해지므로, 단색으로 하얗게 떠버리지 않고 태양 텍스처의 무늬를 유지한 채 밝아집니다. 조명이 0인 면도 `Ke` 만큼의 diffuse 광량을 받은 것처럼 렌더링되어 "스스로 빛나는" 표면이 됩니다.

부수 작업으로, material constant buffer를 공유하는 다른 셰이더(`TexturePS.hlsl`, `TextureShader.hlsl`)에도 같은 위치에 `Ke` 필드를 선언해 **cbuffer 레이아웃을 동기화**했고 (한 쪽만 필드를 추가하면 뒤따르는 필드들이 16바이트씩 밀려 깨집니다), material 상수를 매 프레임 채우지 않는 pass(`BillboardPass`, `EditorIconPass`)에는 구조체 zero-init을 넣어 초기화되지 않은 `Ke`가 GPU로 올라가는 것을 막았습니다.

동기화 대상 레이아웃은 이렇습니다 — cbuffer 자체는 기존 코드이고, 이번 작업이 추가한 것은 `Ke` 줄입니다 (C++ 쪽 `FMaterialConstants`도 같은 위치에 `FVector4 Ke` 추가):

```hlsl
cbuffer MaterialConstants : register(b2)
{
    float4 Ka; // Ambient color
    float4 Kd; // Diffuse color
    float4 Ks; // Specular color
    float4 Ke; // Emissive color
    float Ns;  // Specular exponent
    float Ni;  // Index of refraction
    float D;   // Dissolve factor
    uint MaterialFlags; // Which textures are available (bitfield)
    float Time;
}
```

### 2. 공유 material을 오염시키지 않는 per-instance override

emissive는 이 투사체에만 적용돼야 합니다. 그런데 `MeshComponent->GetMaterial()`이 돌려주는 것은 **에셋(`UStaticMesh`)이 소유한 공유 material**이라, 거기에 `SetEmissiveColor`를 호출하면 같은 메시를 쓰는 모든 액터가 함께 빛나게 됩니다.

그래서 `UMaterial::Duplicate()`를 구현해 material 데이터를 복제한 뒤(텍스처 포인터는 공유 리소스이므로 얕은 복사), 복제본에만 emissive를 설정하고 컴포넌트의 override 슬롯에 꽂았습니다 — [`OrbitProjectile.cpp`](../Engine/Source/Actor/Private/OrbitProjectile.cpp):

```cpp
UMaterial* OverrideMaterial = Cast<UMaterial>(OriginalMaterial->Duplicate());
OverrideMaterial->SetEmissiveColor(EmissiveColor);   // (1.0, 0.8, 0.3) 주황빛
MeshComponent->SetMaterial(i, OverrideMaterial);      // 컴포넌트별 override
```

메시가 여러 material 섹션을 가질 수 있어 전체 슬롯을 순회합니다.

`Duplicate` 구현 ([`Material.cpp`](../Engine/Source/Texture/Private/Material.cpp)) — material 파라미터는 값 복사, 텍스처는 공유 리소스라 포인터만 복사합니다:

```cpp
UObject* UMaterial::Duplicate()
{
	// Create new Material instance
	UMaterial* NewMaterial = NewObject<UMaterial>();

	if (NewMaterial)
	{
		// Copy MaterialData (Ka, Kd, Ks, Ke, Ns, Ni, D, etc.)
		NewMaterial->MaterialData = this->MaterialData;

		// Copy Texture pointers (these point to shared texture resources)
		NewMaterial->DiffuseTexture = this->DiffuseTexture;
		NewMaterial->AmbientTexture = this->AmbientTexture;
		NewMaterial->SpecularTexture = this->SpecularTexture;
		NewMaterial->NormalTexture = this->NormalTexture;
		NewMaterial->AlphaTexture = this->AlphaTexture;
		NewMaterial->BumpTexture = this->BumpTexture;
	}

	return NewMaterial;
}
```

### 3. PointLight로 "주변을 비추는" 효과

emissive만으로는 표면이 밝아질 뿐, 주변 바닥·적이 빛을 받지 않아 광원처럼 보이지 않습니다. bloom이 있었다면 화면 공간 번짐이 이 인상을 만들어 줬겠지만 없으므로, **실제 동적 광원을 붙여서** 해결했습니다.

```cpp
PointLight->AttachToComponent(MeshComponent);
PointLight->SetLightColor(FVector(1.0f, 0.8f, 0.3f));  // emissive와 동일 색
PointLight->SetIntensity(2.0f);
PointLight->SetAttenuationRadius(100.0f);
PointLight->SetCastShadows(false);                      // 성능 (공전체 6면 섀도우 렌더링 회피)
```

투사체가 플레이어 주위를 공전하면 주황빛 조명이 체스판 바닥 위를 함께 돌면서, 표면 emissive(1)와 합쳐져 "빛나는 태양이 주변을 밝힌다"는 인상이 완성됩니다.

## 한계

- 화면 공간 halo(빛 번짐)는 없습니다. bloom 없이 표면 밝기 + 실제 조명으로 내는 근사이므로, 광원을 직접 볼 때의 눈부심 표현은 불가능합니다.
- emissive를 diffuse에 더하는 방식이라 diffuse 텍스처가 검은 픽셀인 곳은 빛나지 않습니다 (순수 additive emissive와 다른 점 — 이 게임에서는 태양 텍스처가 전체적으로 밝아 문제가 없었습니다).
- emissive 강도 스칼라가 따로 없어 세기는 `Ke` RGB 값으로만 조절합니다.
- MTL 파일의 `Ke` / `map_Ke` 파싱은 연결하지 않아, emissive를 켜는 경로는 C++ `SetEmissiveColor` 호출뿐입니다. 게임잼 범위에서는 이 투사체 하나면 충분했지만, 에셋 주도로 emissive를 쓰려면 파서 확장이 필요합니다.
