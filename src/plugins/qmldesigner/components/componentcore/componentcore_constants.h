// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QtGlobal>

namespace QmlDesigner {

namespace ComponentCoreConstants {

inline constexpr char rootCategory[] = "";

inline constexpr char selectionCategory[] = "Selection";
inline constexpr char connectionsCategory[] = "Connections";
inline constexpr char arrangeCategory[] = "Arrange";
inline constexpr char qmlPreviewCategory[] = "QmlPreview";
inline constexpr char editCategory[] = "Edit";
inline constexpr char anchorsCategory[] = "Anchors";
inline constexpr char positionerCategory[] = "Position";
inline constexpr char groupCategory[] = "Group";
inline constexpr char snappingCategory[] = "Snapping";
inline constexpr char layoutCategory[] = "Layout";
inline constexpr char flowCategory[] = "Flow";
inline constexpr char flowEffectCategory[] = "FlowEffect";
inline constexpr char flowConnectionCategory[] = "FlowConnection";
inline constexpr char stackedContainerCategory[] = "StackedContainer";
inline constexpr char genericToolBarCategory[] = "GenericToolBar";
inline constexpr char eventListCategory[] = "QmlEventList";
inline constexpr char create2DCategory[] = "Create 2D";
inline constexpr char basicCategory[] = "Basic";
inline constexpr char controlsCategory[] = "Controls 2";
inline constexpr char viewsCategory[] = "Views";
inline constexpr char positioner2Category[] = "Positioner";
inline constexpr char create3DCategory[] = "Create 3D";
inline constexpr char camerasCategory[] = "Cameras";
inline constexpr char lightsCategory[] = "Lights";
inline constexpr char primitivesCategory[] = "Primitives";

inline constexpr char toFrontCommandId[] = "ToFront";
inline constexpr char toBackCommandId[] = "ToBack";
inline constexpr char raiseCommandId[] = "Raise";
inline constexpr char lowerCommandId[] = "Lower";
inline constexpr char resetZCommandId[] = "ResetZ";
inline constexpr char reverseCommandId[] = "Reverse";
inline constexpr char resetSizeCommandId[] = "ResetSize";
inline constexpr char resetPositionCommandId[] = "ResetPosition";
inline constexpr char copyFormatCommandId[] = "CopyFormat";
inline constexpr char applyFormatCommandId[] = "ApplyFormat";
inline constexpr char visiblityCommandId[] = "ToggleVisiblity";
inline constexpr char anchorsFillCommandId[] = "AnchorsFill";
inline constexpr char anchorsResetCommandId[] = "AnchorsReset";

inline constexpr char anchorParentTopAndBottomCommandId[] = "AnchorParentTopAndBottom";
inline constexpr char anchorParentLeftAndRightCommandId[] = "AnchorParentLeftAndRight";
inline constexpr char anchorParentTopCommandId[] = "AnchorParentTop";
inline constexpr char anchorParentRightCommandId[] = "AnchorParentRight";
inline constexpr char anchorParentBottomCommandId[] = "AnchorParentBottom";
inline constexpr char anchorParentLeftCommandId[] = "AnchorParentLeft";

inline constexpr char removePositionerCommandId[] = "RemovePositioner";
inline constexpr char createFlowActionAreaCommandId[] = "CreateFlowActionArea";
inline constexpr char setFlowStartCommandId[] = "SetFlowStart";
inline constexpr char selectFlowEffectCommandId[] = "SelectFlowEffect";
inline constexpr char layoutRowPositionerCommandId[] = "LayoutRowPositioner";
inline constexpr char layoutColumnPositionerCommandId[] = "LayoutColumnPositioner";
inline constexpr char layoutGridPositionerCommandId[] = "LayoutGridPositioner";
inline constexpr char layoutFlowPositionerCommandId[] = "LayoutFlowPositioner";
inline constexpr char removeLayoutCommandId[] = "RemoveLayout";
inline constexpr char layoutRowLayoutCommandId[] = "LayoutRowLayout";
inline constexpr char layoutColumnLayoutCommandId[] = "LayoutColumnLayout";
inline constexpr char layoutGridLayoutCommandId[] = "LayoutGridLayout";
inline constexpr char layoutFillWidthCommandId[] = "LayoutFillWidth";
inline constexpr char layoutFillHeightCommandId[] = "LayoutFillHeight";
inline constexpr char goIntoComponentCommandId[] = "GoIntoComponent";
inline constexpr char jumpToCodeCommandId[] = "JumpToCode";
inline constexpr char mergeTemplateCommandId[] = "MergeTemplate";
inline constexpr char goToImplementationCommandId[] = "GoToImplementation";
inline constexpr char makeComponentCommandId[] = "MakeComponent";
inline constexpr char importComponentCommandId[] = "ImportComponent";
inline constexpr char exportComponentCommandId[] = "ExportComponent";
inline constexpr char editMaterialCommandId[] = "EditMaterial";
inline constexpr char addToContentLibraryCommandId[] = "AddToContentLibrary";
inline constexpr char addItemToStackedContainerCommandId[] = "AddItemToStackedContainer";
inline constexpr char addTabBarToStackedContainerCommandId[] = "AddTabBarToStackedContainer";
inline constexpr char increaseIndexOfStackedContainerCommandId[]
    = "IncreaseIndexOfStackedContainer";
inline constexpr char decreaseIndexOfStackedContainerCommandId[]
    = "DecreaseIndexOfStackedContainer";
inline constexpr char flowAssignEffectCommandId[] = "AssignFlowEffect";
inline constexpr char flowAssignCustomEffectCommandId[] = "AssignFlowCustomEffect";
inline constexpr char addToGroupItemCommandId[] = "AddToGroupItem";
inline constexpr char fitRootToScreenCommandId[] = "FitRootToScreen";
inline constexpr char fitSelectionToScreenCommandId[] = "FitSelectionToScreen";
inline constexpr char editAnnotationsCommandId[] = "EditAnnotation";
inline constexpr char addMouseAreaFillCommandId[] = "AddMouseAreaFill";
inline constexpr char editIn3dViewCommandId[] = "editIn3dView";
inline constexpr char editInEffectComposerCommandId[] = "editInEffectComposer";

inline constexpr char openSignalDialogCommandId[] = "OpenSignalDialog";
inline constexpr char update3DAssetCommandId[] = "Update3DAsset";

inline constexpr char createImageCommandId[] = "CreateImage";
inline constexpr char createItemCommandId[] = "CreateItem";
inline constexpr char createMouseAreaCommandId[] = "CreateMouseArea";
inline constexpr char createRectangleCommandId[] = "CreateRectangle";
inline constexpr char createTextCommandId[] = "CreateText";
inline constexpr char createTextEditCommandId[] = "CreateTextEdit";
inline constexpr char createTextInputCommandId[] = "CreateTextInput";
inline constexpr char createGridViewCommandId[] = "CreateGridView";
inline constexpr char createListViewCommandId[] = "CreateListView";
inline constexpr char createPathViewCommandId[] = "CreatePathView";
inline constexpr char createColumnCommandId[] = "CreateColumn";
inline constexpr char createFlowCommandId[] = "CreateFlow";
inline constexpr char createGridCommandId[] = "CreateGrid";
inline constexpr char createRowCommandId[] = "CreateRow";
inline constexpr char createButtonCommandId[] = "CreateButton";
inline constexpr char createCheckBoxCommandId[] = "CreateCheckBox";
inline constexpr char createLabelCommandId[] = "CreateLabel";
inline constexpr char createSliderCommandId[] = "CreateSlider";
inline constexpr char createSpinBoxCommandId[] = "CreateSpinBox";
inline constexpr char createSwitchCommandId[] = "CreateSwitch";
inline constexpr char createTextFieldCommandId[] = "CreateTextField";
inline constexpr char createNodeCommandId[] = "CreateNode";
inline constexpr char createOrthographicCameraCommandId[] = "CreateOrthographicCamera";
inline constexpr char createPerspectiveCameraCommandId[] = "CreatePerspectiveCamera";
inline constexpr char createDirectionalLightCommandId[] = "CreateDirectionalLight";
inline constexpr char createPointLightCommandId[] = "CreatePointLight";
inline constexpr char createSpotLightCommandId[] = "CreateSpotLight";
inline constexpr char createConeCommandId[] = "CreateCone";
inline constexpr char createCubeCommandId[] = "CreateCube";
inline constexpr char createCylinderCommandId[] = "CreateCylinder";
inline constexpr char createModelCommandId[] = "CreateModel";
inline constexpr char createPlaneCommandId[] = "CreatePlane";
inline constexpr char createSphereCommandId[] = "CreateSphere";

inline constexpr char selectionCategoryDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu",
                                                                         "Selection");
inline constexpr char connectionsCategoryDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu",
                                                                           "Connections");
inline constexpr char flowConnectionCategoryDisplayName[] = QT_TRANSLATE_NOOP(
    "QmlDesignerContextMenu", "Connect");
inline constexpr char selectEffectDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu",
                                                                    "Select Effect");
inline constexpr char arrangeCategoryDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu",
                                                                       "Arrange");
inline constexpr char editCategoryDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu", "Edit");
inline constexpr char anchorsCategoryDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu",
                                                                       "Anchors");
inline constexpr char positionerCategoryDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu",
                                                                          "Positioner");
inline constexpr char groupCategoryDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu",
                                                                     "Group");
inline constexpr char snappingCategoryDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu",
                                                                        "Snapping");
inline constexpr char layoutCategoryDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu",
                                                                      "Layout");
inline constexpr char flowCategoryDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu", "Flow");
inline constexpr char flowEffectCategoryDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu",
                                                                          "Flow Effects");
inline constexpr char stackedContainerCategoryDisplayName[] = QT_TRANSLATE_NOOP(
    "QmlDesignerContextMenu", "Stacked Container");

inline constexpr char cutSelectionDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu", "Cut");
inline constexpr char copySelectionDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu", "Copy");
inline constexpr char pasteSelectionDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu",
                                                                      "Paste");
inline constexpr char deleteSelectionDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu",
                                                                       "Delete Selection");

inline constexpr char toFrontDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu",
                                                               "Bring to Front");
inline constexpr char toBackDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu",
                                                              "Send to Back");

inline constexpr char raiseDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu",
                                                             "Bring Forward");
inline constexpr char lowerDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu",
                                                             "Send Backward");

inline constexpr char undoDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu", "Undo");
inline constexpr char redoDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu", "Redo");

inline constexpr char visibilityDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu",
                                                                  "Visibility");

inline constexpr char resetSizeDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu",
                                                                 "Reset Size");
inline constexpr char resetPositionDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu",
                                                                     "Reset Position");
inline constexpr char copyFormatDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu",
                                                                  "Copy Formatting");
inline constexpr char applyFormatDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu",
                                                                   "Apply Formatting");

inline constexpr char enterComponentDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu",
                                                                      "Edit Component");
inline constexpr char JumpToCodeDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu",
                                                                  "Jump to the Code");
inline constexpr char mergeTemplateDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu",
                                                                     "Merge with Template");
inline constexpr char goToImplementationDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu",
                                                                          "Go to Implementation");
inline constexpr char makeComponentDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu",
                                                                     "Create Component");
inline constexpr char editMaterialDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu",
                                                                    "Edit Material");
inline constexpr char addToContentLibraryDisplayName[] = QT_TRANSLATE_NOOP(
    "QmlDesignerContextMenu", "Add to Content Library");
inline constexpr char importComponentDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu",
                                                                        "Import Component");
inline constexpr char exportComponentDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu",
                                                                        "Export Component");
inline constexpr char editAnnotationsDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu",
                                                                       "Edit Annotations");
inline constexpr char addMouseAreaFillDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu",
                                                                        "Add Mouse Area");
inline constexpr char editIn3dViewDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu",
                                                                    "Edit in 3D View");
inline constexpr char editInEffectComposerDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu",
                                                                            "Edit in Effect Composer");

inline constexpr char openSignalDialogDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu",
                                                                        "Open Signal Dialog");
inline constexpr char update3DAssetDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu",
                                                                     "Update 3D Asset");

inline constexpr char setIdDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu", "Set Id");

inline constexpr char resetZDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu",
                                                              "Reset z Property");

inline constexpr char reverseDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu", "Reverse");

inline constexpr char anchorsFillDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu",
                                                                   "Fill Parent");
inline constexpr char anchorsResetDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu",
                                                                    "No Anchors");

inline constexpr char anchorParentTopAndBottomDisplayName[] = QT_TRANSLATE_NOOP(
    "QmlDesignerContextMenu", "Top And Bottom");
inline constexpr char anchorParentLeftAndRightDisplayName[] = QT_TRANSLATE_NOOP(
    "QmlDesignerContextMenu", "Left And Right");
inline constexpr char anchorParentTopDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu",
                                                                       "Top");
inline constexpr char anchorParentRightDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu",
                                                                         "Right");
inline constexpr char anchorParentBottomDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu",
                                                                          "Bottom");
inline constexpr char anchorParentLeftDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu",
                                                                        "Left");

inline constexpr char layoutColumnPositionerDisplayName[] = QT_TRANSLATE_NOOP(
    "QmlDesignerContextMenu", "Column Positioner");
inline constexpr char layoutRowPositionerDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu",
                                                                           "Row Positioner");
inline constexpr char layoutGridPositionerDisplayName[] = QT_TRANSLATE_NOOP(
    "QmlDesignerContextMenu", "Grid Positioner");
inline constexpr char layoutFlowPositionerDisplayName[] = QT_TRANSLATE_NOOP(
    "QmlDesignerContextMenu", "Flow Positioner");
inline constexpr char removePositionerDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu",
                                                                        "Remove Positioner");
inline constexpr char createFlowActionAreaDisplayName[] = QT_TRANSLATE_NOOP(
    "QmlDesignerContextMenu", "Create Flow Action");
inline constexpr char setFlowStartDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu",
                                                                    "Set Flow Start");
inline constexpr char removeLayoutDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu",
                                                                    "Remove Layout");

inline constexpr char addToGroupItemDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu",
                                                                      "Group in GroupItem");
inline constexpr char removeGroupItemDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu",
                                                                       "Remove GroupItem");

inline constexpr char addItemToStackedContainerDisplayName[] = QT_TRANSLATE_NOOP(
    "QmlDesignerContextMenu", "Add Component");
inline constexpr char addTabBarToStackedContainerDisplayName[] = QT_TRANSLATE_NOOP(
    "QmlDesignerContextMenu", "Add Tab Bar");
inline constexpr char increaseIndexToStackedContainerDisplayName[] = QT_TRANSLATE_NOOP(
    "QmlDesignerContextMenu", "Increase Index");
inline constexpr char decreaseIndexToStackedContainerDisplayName[] = QT_TRANSLATE_NOOP(
    "QmlDesignerContextMenu", "Decrease Index");

inline constexpr char layoutColumnLayoutDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu",
                                                                          "Column Layout");
inline constexpr char layoutRowLayoutDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu",
                                                                       "Row Layout");
inline constexpr char layoutGridLayoutDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu",
                                                                        "Grid Layout");

inline constexpr char layoutFillWidthDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu",
                                                                       "Fill Width");
inline constexpr char layoutFillHeightDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu",
                                                                        "Fill Height");

inline constexpr char flowAssignEffectDisplayName[] = "Assign FlowEffect ";
inline constexpr char flowAssignCustomEffectDisplayName[] = "Assign Custom FlowEffect ";

inline constexpr char raiseToolTip[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu",
                                                         "Raise selected component.");
inline constexpr char lowerToolTip[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu",
                                                         "Lower selected component.");

inline constexpr char resetSizeToolTip[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu",
                                                             "Reset size and use implicit size.");
inline constexpr char resetPositionTooltip[] = QT_TRANSLATE_NOOP(
    "QmlDesignerContextMenu", "Reset position and use implicit position.");
inline constexpr char copyFormatTooltip[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu",
                                                              "Copy formatting.");
inline constexpr char applyFormatTooltip[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu",
                                                               "Apply formatting.");

inline constexpr char anchorsFillToolTip[] = QT_TRANSLATE_NOOP(
    "QmlDesignerContextMenu", "Fill selected component to parent.");
inline constexpr char anchorsResetToolTip[] = QT_TRANSLATE_NOOP(
    "QmlDesignerContextMenu", "Reset anchors for selected component.");

inline constexpr char layoutColumnLayoutToolTip[] = QT_TRANSLATE_NOOP(
    "QmlDesignerContextMenu", "Layout selected components in column layout.");
inline constexpr char layoutRowLayoutToolTip[] = QT_TRANSLATE_NOOP(
    "QmlDesignerContextMenu", "Layout selected components in row layout.");
inline constexpr char layoutGridLayoutToolTip[] = QT_TRANSLATE_NOOP(
    "QmlDesignerContextMenu", "Layout selected components in grid layout.");

inline constexpr char increaseIndexOfStackedContainerToolTip[] = QT_TRANSLATE_NOOP(
    "QmlDesignerContextMenu", "Increase index of stacked container.");
inline constexpr char decreaseIndexOfStackedContainerToolTip[] = QT_TRANSLATE_NOOP(
    "QmlDesignerContextMenu", "Decrease index of stacked container.");
inline constexpr char addItemToStackedContainerToolTip[] = QT_TRANSLATE_NOOP(
    "QmlDesignerContextMenu", "Add component to stacked container.");
inline constexpr char addFlowActionToolTip[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu",
                                                                 "Add flow action.");

inline constexpr char editListModelDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu",
                                                                     "Edit List Model...");

inline constexpr char create2DCategoryDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu", "Create 2D");
inline constexpr char basicCategoryDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu", "Basic");
inline constexpr char createImageDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu", "Image");
inline constexpr char createItemDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu", "Item");
inline constexpr char createMouseAreaDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu", "Mouse Area");
inline constexpr char createRectangleDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu", "Rectangle");
inline constexpr char createTextDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu", "Text");
inline constexpr char createTextEditDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu", "Text Edit");
inline constexpr char createTextInputDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu", "Text Input");
inline constexpr char controlsCategoryDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu", "Controls 2");
inline constexpr char createButtonDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu", "Button");
inline constexpr char createCheckBoxDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu", "Check Box");
inline constexpr char createLabelDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu", "Label");
inline constexpr char createSliderDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu", "Slider");
inline constexpr char createSpinBoxDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu", "Spin Box");
inline constexpr char createSwitchDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu", "Switch");
inline constexpr char createTextFieldDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu", "Text Field");
inline constexpr char viewsCategoryDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu", "Views");
inline constexpr char createGridViewDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu", "Grid View");
inline constexpr char createListViewDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu", "List View");
inline constexpr char createPathViewDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu", "Path View");
inline constexpr char positioner2CategoryDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu", "Positioner");
inline constexpr char createColumnDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu", "Column");
inline constexpr char createFlowDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu", "Flow");
inline constexpr char createGridDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu", "Grid");
inline constexpr char createRowDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu", "Row");
inline constexpr char create3DCategoryDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu", "Create 3D");
inline constexpr char createNodeDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu", "Node");
inline constexpr char camerasCategoryDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu", "Cameras");
inline constexpr char createOrthographicCameraDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu", "Orthographic Camera");
inline constexpr char createPerspectiveCameraDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu", "Perspective Camera");
inline constexpr char lightsCategoryDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu", "Lights");
inline constexpr char createDirectionalLightDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu", "Directional Light");
inline constexpr char createPointLightDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu", "Point Light");
inline constexpr char createSpotLightDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu", "Spot Light");
inline constexpr char primitivesCategoryDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu", "Primitives");
inline constexpr char createConeDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu", "Cone");
inline constexpr char createCubeDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu", "Cube");
inline constexpr char createCylinderDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu", "Cylinder");
inline constexpr char createModelDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu", "Model");
inline constexpr char createPlaneDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu", "Plane");
inline constexpr char createSphereDisplayName[] = QT_TRANSLATE_NOOP("QmlDesignerContextMenu", "Sphere");

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
    EditListModel,
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
    JumpToCode,
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
    Last,
    /******** Section *****************************/
    AddingAssetsSection = 7000,
    Add3DToContentLib,
    ImportComponent,
    ExportComponent,
    /******** Section *****************************/
    CreateSection = 8000,
    Create2DCategory,
    BasicCategory,
    CreateImage,
    CreateItem,
    CreateMouseArea,
    CreateRectangle,
    CreateText,
    CreateTextEdit,
    CreateTextInput,
    ControlsCategory,
    CreateButton,
    CreateCheckBox,
    CreateLabel,
    CreateSlider,
    CreateSpinBox,
    CreateSwitch,
    CreateTextField,
    ViewsCategory,
    CreateGridView,
    CreateListView,
    CreatePathView,
    Positioner2Category,
    CreateColumn,
    CreateFlow,
    CreateGrid,
    CreateRow,
    Create3DCategory,
    CreateNode,
    CamerasCategory,
    CreateOrthographicCamera,
    CreatePerspectiveCamera,
    LightsCategory,
    CreateDirectionalLight,
    CreatePointLight,
    CreateSpotLight,
    PrimitivesCategory,
    CreateCone,
    CreateCube,
    CreateCylinder,
    CreateModel,
    CreatePlane,
    CreateSphere,
};
};

inline constexpr char addImagesDisplayString[] = QT_TRANSLATE_NOOP("QmlDesignerAddResources",
                                                                   "Image Files");
inline constexpr char addFontsDisplayString[] = QT_TRANSLATE_NOOP("QmlDesignerAddResources",
                                                                  "Font Files");
inline constexpr char addSoundsDisplayString[] = QT_TRANSLATE_NOOP("QmlDesignerAddResources",
                                                                   "Sound Files");
inline constexpr char addVideosDisplayString[] = QT_TRANSLATE_NOOP("QmlDesignerAddResources",
                                                                   "Video Files");
inline constexpr char addShadersDisplayString[] = QT_TRANSLATE_NOOP("QmlDesignerAddResources",
                                                                    "Shader Files");
inline constexpr char add3DAssetsDisplayString[] = QT_TRANSLATE_NOOP("QmlDesignerAddResources",
                                                                     "3D Assets");
inline constexpr char addQt3DSPresentationsDisplayString[] = QT_TRANSLATE_NOOP(
    "QmlDesignerAddResources", "Qt 3D Studio Presentations");
inline constexpr char addCustomEffectDialogDisplayString[] = QT_TRANSLATE_NOOP(
    "QmlDesignerAddResources", "Effect Composer Files");

} //ComponentCoreConstants

} //QmlDesigner
