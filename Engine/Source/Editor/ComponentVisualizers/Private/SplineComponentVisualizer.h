// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "ComponentVisualizer.h"
#include "Components/SplineComponent.h"

/** Base class for clickable spline editing proxies */
struct HSplineVisProxy : public HComponentVisProxy
{
	DECLARE_HIT_PROXY();

	HSplineVisProxy(const UActorComponent* InComponent)
	: HComponentVisProxy(InComponent, HPP_Wireframe)
	{}
};

/** Proxy for a spline key */
struct HSplineKeyProxy : public HSplineVisProxy
{
	DECLARE_HIT_PROXY();

	HSplineKeyProxy(const UActorComponent* InComponent, int32 InKeyIndex) 
		: HSplineVisProxy(InComponent)
		, KeyIndex(InKeyIndex)
	{}

	int32 KeyIndex;
};

/** Proxy for a spline segment */
struct HSplineSegmentProxy : public HSplineVisProxy
{
	DECLARE_HIT_PROXY();

	HSplineSegmentProxy(const UActorComponent* InComponent, int32 InSegmentIndex)
		: HSplineVisProxy(InComponent)
		, SegmentIndex(InSegmentIndex)
	{}

	int32 SegmentIndex;
};

/** SplineComponent visualizer/edit functionality */
class FSplineComponentVisualizer : public FComponentVisualizer
{
public:
	FSplineComponentVisualizer();
	virtual ~FSplineComponentVisualizer();

	// Begin FComponentVisualizer interface
	virtual void OnRegister() override;
	virtual void DrawVisualization(const UActorComponent* Component, const FSceneView* View, FPrimitiveDrawInterface* PDI) override;
	virtual bool VisProxyHandleClick(FLevelEditorViewportClient* InViewportClient, HComponentVisProxy* VisProxy, const FViewportClick& Click) override;
	virtual void EndEditing() override;
	virtual bool GetWidgetLocation(const FEditorViewportClient* ViewportClient, FVector& OutLocation) const override;
	virtual bool GetCustomInputCoordinateSystem(const FEditorViewportClient* ViewportClient, FMatrix& OutMatrix) const override;
	virtual bool HandleInputDelta(FEditorViewportClient* ViewportClient, FViewport* Viewport, FVector& DeltaTranslate, FRotator& DeltaRotate, FVector& DeltaScale) override;
	virtual bool HandleInputKey(FEditorViewportClient* ViewportClient, FViewport* Viewport, FKey Key, EInputEvent Event) override;
	virtual TSharedPtr<SWidget> GenerateContextMenu() const override;
	// End FComponentVisualizer interface

	/** Get the spline component we are currently editing */
	USplineComponent* GetEditedSplineComponent() const;

private:

	void NotifyComponentModified();

	void OnDeleteKey();
	bool CanDeleteKey() const;

	void OnDuplicateKey();
	bool IsKeySelectionValid() const;

	void OnAddKey();
	bool CanAddKey() const;

	void OnResetToAutomaticTangent(EInterpCurveMode Mode);
	bool CanResetToAutomaticTangent(EInterpCurveMode Mode) const;

	void OnSetKeyType(EInterpCurveMode Mode);
	bool IsKeyTypeSet(EInterpCurveMode Mode) const;

	/** Generate the submenu containing the available point types */
	void GenerateSplinePointTypeSubMenu(FMenuBuilder& MenuBuilder) const;

	/** Generate the submenu containing the available auto tangent types */
	void GenerateTangentTypeSubMenu(FMenuBuilder& MenuBuilder) const;

	/** Output log commands */
	TSharedPtr<FUICommandList> SplineComponentVisualizerActions;

	/** Actor that owns the currently edited spline */
	TWeakObjectPtr<AActor> SplineOwningActor;

	/** Name of property on the actor that references the spline we are editing */
	FName SplineCompPropName;

	/** Index of key we have selected */
	int32 SelectedKeyIndex;

	/** Index of segment we have selected */
	int32 SelectedSegmentIndex;

	/** Position on spline we have selected */
	FVector SelectedSplinePosition;

	/** Whether we currently allow duplication when dragging */
	bool bAllowDuplication;
};
