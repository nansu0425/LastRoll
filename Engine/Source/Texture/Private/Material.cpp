#include "pch.h"
#include "Texture/Public/Material.h"
#include "Texture/Public/Texture.h"
#include "Texture/Public/TextureRenderProxy.h"

IMPLEMENT_CLASS(UMaterial, UObject)

UMaterial::~UMaterial()
{
}

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
