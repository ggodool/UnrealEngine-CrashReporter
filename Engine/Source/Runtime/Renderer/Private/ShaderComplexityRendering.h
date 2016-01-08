// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
ShaderComplexityRendering.h: Declarations used for the shader complexity viewmode.
=============================================================================*/

#pragma once

template <bool bQuadComplexity>
class TComplexityAccumulatePS : public FDebugViewModePS
{
	DECLARE_SHADER_TYPE(TComplexityAccumulatePS,Global);
public:

	static bool ShouldCache(EShaderPlatform Platform)
	{
		return AllowDebugViewModeShader(Platform) || !bQuadComplexity; // Shader complexity is always available
	}

	TComplexityAccumulatePS(const ShaderMetaType::CompiledShaderInitializerType& Initializer):
		FDebugViewModePS(Initializer)
	{
		NormalizedComplexity.Bind(Initializer.ParameterMap,TEXT("NormalizedComplexity"));
		ShowQuadOverdraw.Bind(Initializer.ParameterMap,TEXT("bShowQuadOverdraw"));
		QuadBufferUAV.Bind(Initializer.ParameterMap,TEXT("RWQuadBuffer"));
	}

	TComplexityAccumulatePS() {}

	virtual bool Serialize(FArchive& Ar) override
	{
		bool bShaderHasOutdatedParameters = FGlobalShader::Serialize(Ar);
		Ar << NormalizedComplexity;
		Ar << ShowQuadOverdraw;
		Ar << QuadBufferUAV;
		return bShaderHasOutdatedParameters;
	}

	static void ModifyCompilationEnvironment(EShaderPlatform Platform, FShaderCompilerEnvironment& OutEnvironment)
	{
		OutEnvironment.SetDefine(TEXT("OUTPUT_QUAD_OVERDRAW"), AllowDebugViewModeShader(Platform) ? TEXT("1") : TEXT("0"));
	}

	virtual void SetParameters(
		FRHICommandList& RHICmdList, 
		const FShader* OriginalVS, 
		const FShader* OriginalPS, 
		const FSceneView& View
		) override;

	virtual void SetMesh(
		FRHICommandList& RHICmdList, 
		const FVertexFactory* VertexFactory,
		const FSceneView& View,
		const FPrimitiveSceneProxy* Proxy,
		const FMeshBatchElement& BatchElement, 
		const FMeshDrawingRenderState& DrawRenderState
		) override {}

	virtual void SetMesh(FRHICommandList& RHICmdList, const FSceneView& View) override {}

private:

	FShaderParameter NormalizedComplexity;
	FShaderParameter ShowQuadOverdraw;
	FShaderResourceParameter QuadBufferUAV;
};

typedef TComplexityAccumulatePS<false> TShaderComplexityAccumulatePS;
typedef TComplexityAccumulatePS<true> TQuadComplexityAccumulatePS;

