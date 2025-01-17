﻿// Copyright Epic Games, Inc. All Rights Reserved.

#include "Importers/CurveLinearColorAtlasImporter.h"
#include "Importers/TextureImporter.h"
#include "Curves/CurveLinearColor.h"

#include "JsonGlobals.h"
#include "Curves/CurveLinearColorAtlas.h"
#include "Dom/JsonObject.h"
#include "Factories/CurveLinearColorAtlasFactory.h"
#include "Utilities/AssetUtilities.h"

bool UCurveLinearColorAtlasImporter::ImportData() {
	try {
		TSharedPtr<FJsonObject> Properties = JsonObject->GetObjectField("Properties");
		
		// Create UTextureImporters to add UTexture2D properties
		const UTextureImporter* Importer = new UTextureImporter(FString(), FString(), JsonObject, Package, OutermostPkg);

		float Width = 256;
		float Height = 256;

		UCurveLinearColorAtlas* Object = NewObject<UCurveLinearColorAtlas>(Package, UCurveLinearColorAtlas::StaticClass(), *FileName, RF_Public | RF_Standalone);
		Object->Source.Init(Width, Height, 1, 1, TSF_RGBA16F);
		const int32 TextureDataSize = Object->Source.CalcMipSize(0);
		Object->SrcData.AddUninitialized(TextureDataSize);
		uint32* TextureData = (uint32*)Object->Source.LockMip(0);
		FFloat16Color InitColor(FLinearColor::White);
		for (uint32 y = 0; y < Object->TextureSize; y++) {
			for (uint32 x = 0; x < Object->TextureSize; x++) {
				Object->SrcData[x * Object->TextureSize + y] = InitColor;
			}
		}
		FMemory::Memcpy(TextureData, Object->SrcData.GetData(), TextureDataSize);
		Object->Source.UnlockMip(0);

		Object->UpdateResource();

		bool bHasAnyDirtyTextures;
		bool bIsDirty;
		bool bShowDebugColorsForNullGradients;
		uint32 TextureSize;

		if (Properties->TryGetBoolField("bHasAnyDirtyTextures", bHasAnyDirtyTextures)) Object->bHasAnyDirtyTextures = bHasAnyDirtyTextures;
		if (Properties->TryGetBoolField("bIsDirty", bIsDirty)) Object->bIsDirty = bIsDirty;
		if (Properties->TryGetBoolField("bShowDebugColorsForNullGradients", bShowDebugColorsForNullGradients)) Object->bShowDebugColorsForNullGradients = bShowDebugColorsForNullGradients;

		if (Properties->TryGetNumberField(TEXT("TextureSize"), TextureSize))
			Object->TextureSize = Properties->GetNumberField("TextureSize");

		UProperty* TextureSizeProperty = FindField<UProperty>(Object->GetClass(), "TextureSize");
		FPropertyChangedEvent TextureSizePropertyPropertyChangedEvent(TextureSizeProperty, EPropertyChangeType::ValueSet);
		Object->PostEditChangeProperty(TextureSizePropertyPropertyChangedEvent);

		// Add gradient curves
		UProperty* GradientCurvesProperty = FindField<UProperty>(Object->GetClass(), "GradientCurves");
		FPropertyChangedEvent PropertyChangedEvent(GradientCurvesProperty, EPropertyChangeType::ArrayAdd);

		const TArray<TSharedPtr<FJsonValue>> GradientCurves = Properties->GetArrayField("GradientCurves");
		TArray<UCurveLinearColor*> CurvesLocal;

		CurvesLocal = LoadObject(GradientCurves, CurvesLocal);
		Object->GradientCurves = CurvesLocal;
		Object->PostEditChangeProperty(PropertyChangedEvent);

		// Handle edit changes, and add it to the content browser
		if (!HandleAssetCreation(Object)) return false;
	} catch (const char* Exception) {
		UE_LOG(LogJson, Error, TEXT("%s"), *FString(Exception));
		return false;
	}

	return true;
}