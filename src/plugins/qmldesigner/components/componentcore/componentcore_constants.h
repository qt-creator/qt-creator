// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QtGlobal>

namespace QmlDesigner {

namespace ComponentCoreConstants {

const char rootCategory[] = "";

const char selectionCategory[] = "Selection";
const char connectionsCategory[] = "Connections";
const char arrangeCategory[] = "Arrange";
const char qmlPreviewCategory[] = "QmlPreview";
const char editCategory[] = "Edit";
const char anchorsCategory[] = "Anchors";
const char positionerCategory[] = "Position";
const char groupCategory[] = "Group";
const char snappingCategory[] = "Snapping";
const char layoutCategory[] = "Layout";
const char flowCategory[] = "Flow";
const char flowEffectCategory[] = "FlowEffect";
const char flowConnectionCategory[] = "FlowConnection";
const char stackedContainerCategory[] = "StackedContainer";
const char genericToolBarCategory[] = "GenericToolBar";
const char eventListCategory[] = "QmlEventList";

const char toFrontCommandId[] = "ToFront";
const char toBackCommandId[] = "ToBack";
const char raiseCommandId[] = "Raise";
const char lowerCommandId[] = "Lower";
const char resetZCommandId[] = "ResetZ";
const char reverseCommandId[] = "Reverse";
const char resetSizeCommandId[] = "ResetSize";
const char resetPositionCommandId[] = "ResetPosition";
const char copyFormatCommandId[] = "CopyFormat";
const char applyFormatCommandId[] = "ApplyFormat";
const char visiblityCommandId[] = "ToggleVisiblity";
const char anchorsFillCommandId[] = "AnchorsFill";
const char anchorsResetCommandId[] = "AnchorsReset";

const char anchorParentTopAndBottomCommandId[] = "AnchorParentTopAndBottom";
const char anchorParentLeftAndRightCommandId[] = "AnchorParentLeftAndRight";
const char anchorParentTopCommandId[] = "AnchorParentTop";
const char anchorParentRightCommandId[] = "AnchorParentRight";
const char anchorParentBottomCommandId[] = "AnchorParentBottom";
const char anchorParentLeftCommandId[] = "AnchorParentLeft";

const char removePositionerCommandId[] = "RemovePositioner";
const char createFlowActionAreaCommandId[] = "CreateFlowActionArea";
const char setFlowStartCommandId[] = "SetFlowStart";
const char selectFlowEffectCommandId[] = "SelectFlowEffect";
const char layoutRowPositionerCommandId[] = "LayoutRowPositioner";
const char layoutColumnPositionerCommandId[] = "LayoutColumnPositioner";
const char layoutGridPositionerCommandId[] = "LayoutGridPositioner";
const char layoutFlowPositionerCommandId[] = "LayoutFlowPositioner";
const char removeLayoutCommandId[] = "RemoveLayout";
const char layoutRowLayoutCommandId[] = "LayoutRowLayout";
const char layoutColumnLayoutCommandId[] = "LayoutColumnLayout";
const char layoutGridLayoutCommandId[] = "LayoutGridLayout";
const char layoutFillWidthCommandId[] = "LayoutFillWidth";
const char layoutFillHeightCommandId[] = "LayoutFillHeight";
const char goIntoComponentCommandId[] = "GoIntoComponent";
const char mergeTemplateCommandId[] = "MergeTemplate";
const char goToImplementationCommandId[] = "GoToImplementation";
const char makeComponentCommandId[] = "MakeComponent";
const char editMaterialCommandId[] = "EditMaterial";
const char addItemToStackedContainerCommandId[] = "AddItemToStackedContainer";
const char addTabBarToStackedContainerCommandId[] = "AddTabBarToStackedContainer";
const char increaseIndexOfStackedContainerCommandId[] = "IncreaseIndexOfStackedContainer";
const char decreaseIndexOfStackedContainerCommandId[] = "DecreaseIndexOfStackedContainer";
const char flowAssignEffectCommandId[] = "AssignFlowEffect";
const char flowAssignCustomEffectCommandId[] = "AssignFlowCustomEffect";
const char addToGroupItemCommandId[] = "AddToGroupItem";
const char fitRootToScreenCommandId[] = "FitRootToScreen";
const char fitSelectionToScreenCommandId[] = "FitSelectionToScreen";
const char editAnnotationsCommandId[] = "EditAnnotation";
const char addMouseAreaFillCommandId[] = "AddMouseAreaFill";

const char openSignalDialogCommandId[] = "OpenSignalDialog";
const char update3DAssetCommandId[] = "Update3DAsset";

const char selectionCategoryDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu", "Selection");
const char connectionsCategoryDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu", "Connections");
const char flowConnectionCategoryDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu", "Connect");
const char selectEffectDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu", "Select Effect");
const char arrangeCategoryDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu", "Arrange");
const char editCategoryDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu", "Edit");
const char anchorsCategoryDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu", "Anchors");
const char positionerCategoryDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu", "Positioner");
const char groupCategoryDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu", "Group");
const char snappingCategoryDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu", "Snapping");
const char layoutCategoryDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu", "Layout");
const char flowCategoryDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu", "Flow");
const char flowEffectCategoryDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu", "Flow Effects");
const char stackedContainerCategoryDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu", "Stacked Container");

const char cutSelectionDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu", "Cut");
const char copySelectionDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu", "Copy");
const char pasteSelectionDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu", "Paste");
const char deleteSelectionDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu", "Delete Selection");

const char toFrontDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu", "Bring to Front");
const char toBackDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu", "Send to Back");

const char raiseDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu", "Bring Forward");
const char lowerDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu", "Send Backward");

const char undoDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu", "Undo");
const char redoDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu", "Redo");

const char visibilityDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu", "Visibility");

const char resetSizeDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu", "Reset Size");
const char resetPositionDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu", "Reset Position");
const char copyFormatDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu", "Copy Formatting");
const char applyFormatDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu", "Apply Formatting");

const char enterComponentDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu", "Edit Component");
const char mergeTemplateDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu", "Merge with Template");
const char goToImplementationDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu", "Go to Implementation");
const char makeComponentDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu", "Create Component");
const char editMaterialDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu", "Edit Material");
const char editAnnotationsDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu", "Edit Annotations");
const char addMouseAreaFillDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu", "Add Mouse Area");

const char openSignalDialogDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu", "Open Signal Dialog");
const char update3DAssetDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu", "Update 3D Asset");

const char setIdDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu", "Set Id");

const char resetZDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu", "Reset z Property");

const char reverseDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu", "Reverse");

const char anchorsFillDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu", "Fill Parent");
const char anchorsResetDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu", "No Anchors");

const char anchorParentTopAndBottomDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu", "Top And Bottom");
const char anchorParentLeftAndRightDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu", "Left And Right");
const char anchorParentTopDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu", "Top");
const char anchorParentRightDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu", "Right");
const char anchorParentBottomDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu", "Bottom");
const char anchorParentLeftDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu", "Left");

const char layoutColumnPositionerDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu", "Column Positioner");
const char layoutRowPositionerDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu", "Row Positioner");
const char layoutGridPositionerDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu", "Grid Positioner");
const char layoutFlowPositionerDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu", "Flow Positioner");
const char removePositionerDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu", "Remove Positioner");
const char createFlowActionAreaDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu", "Create Flow Action");
const char setFlowStartDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu", "Set Flow Start");
const char removeLayoutDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu", "Remove Layout");

const char addToGroupItemDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu", "Group in GroupItem");
const char removeGroupItemDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu",
                                                            "Remove GroupItem");

const char addItemToStackedContainerDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu", "Add Component");
const char addTabBarToStackedContainerDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu", "Add Tab Bar");
const char increaseIndexToStackedContainerDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu", "Increase Index");
const char decreaseIndexToStackedContainerDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu", "Decrease Index");

const char layoutColumnLayoutDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu", "Column Layout");
const char layoutRowLayoutDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu", "Row Layout");
const char layoutGridLayoutDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu", "Grid Layout");

const char layoutFillWidthDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu", "Fill Width");
const char layoutFillHeightDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu", "Fill Height");

const char flowAssignEffectDisplayName[] = "Assign FlowEffect ";
const char flowAssignCustomEffectDisplayName[] = "Assign Custom FlowEffect ";

const char raiseToolTip[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu", "Raise selected component.");
const char lowerToolTip[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu", "Lower selected component.");

const char resetSizeToolTip[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu", "Reset size and use implicit size.");
const char resetPositionTooltip[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu", "Reset position and use implicit position.");
const char copyFormatTooltip[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu", "Copy formatting.");
const char applyFormatTooltip[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu", "Apply formatting.");

const char anchorsFillToolTip[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu", "Fill selected component to parent.");
const char anchorsResetToolTip[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu", "Reset anchors for selected component.");

const char layoutColumnLayoutToolTip[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu", "Layout selected components in column layout.");
const char layoutRowLayoutToolTip[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu", "Layout selected components in row layout.");
const char layoutGridLayoutToolTip[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu", "Layout selected components in grid layout.");

const char increaseIndexOfStackedContainerToolTip[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu", "Increase index of stacked container.");
const char decreaseIndexOfStackedContainerToolTip[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu", "Decrease index of stacked container.");
const char addItemToStackedContainerToolTip[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu", "Add component to stacked container.");
const char addFlowActionToolTip[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu", "Add flow action.");

const char editListModelDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu",
                                                          "Edit List Model...");
namespace Priorities {
enum PrioritiesEnum : int {
    Top = 0,
    FlowCategory,
    ComponentActions,
    /******** Section *****************************/
    ModifySection = 1000,
    ConnectionsCategory,
    SelectionCategory,
    ArrangeCategory,
    EditCategory,
    /******** Section *****************************/
    PositionSection = 2000,
    SnappingCategory,
    AnchorsCategory,
    LayoutCategory,
    PositionCategory,
    StackedContainerCategory,
    /******** Section *****************************/
    EventSection = 3000,
    TimelineCategory,
    EventListCategory,
    /******** Section *****************************/
    AdditionsSection = 4000,
    EditAnnotations,
    AddMouseArea,
    MergeWithTemplate,
    /******** Section *****************************/
    ViewOprionsSection = 5000,
    ResetView,
    Group,
    Visibility,
    ShowBoundingRect,
    /******** Section *****************************/
    CustomActionsSection = 6000,
    QmlPreviewCategory,
    SignalsDialog,
    Refactoring,
    GenericToolBar,
    Last
};
};

const char addImagesDisplayString[] = QT_TRANSLATE_NOOP("QmlDesignerAddResources", "Image Files");
const char addFontsDisplayString[] = QT_TRANSLATE_NOOP("QmlDesignerAddResources", "Font Files");
const char addSoundsDisplayString[] = QT_TRANSLATE_NOOP("QmlDesignerAddResources", "Sound Files");
const char addVideosDisplayString[] = QT_TRANSLATE_NOOP("QmlDesignerAddResources", "Video Files");
const char addShadersDisplayString[] = QT_TRANSLATE_NOOP("QmlDesignerAddResources", "Shader Files");
const char add3DAssetsDisplayString[] = QT_TRANSLATE_NOOP("QmlDesignerAddResources", "3D Assets");
const char addQt3DSPresentationsDisplayString[] = QT_TRANSLATE_NOOP("QmlDesignerAddResources",
                                                                    "Qt 3D Studio Presentations");
const char addCustomEffectDialogDisplayString[] = QT_TRANSLATE_NOOP("QmlDesignerAddResources",
                                                                    "Effect Maker Files");

} //ComponentCoreConstants

} //QmlDesigner
