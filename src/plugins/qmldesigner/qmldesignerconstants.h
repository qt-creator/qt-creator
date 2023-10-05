// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

namespace QmlDesigner {
namespace Constants {

const char C_BACKSPACE[] = "QmlDesigner.Backspace";
const char C_DELETE[]    = "QmlDesigner.Delete";
const char C_DUPLICATE[] = "QmlDesigner.Duplicate";

// Context
const char C_QMLDESIGNER[]          = "QmlDesigner::QmlDesignerMain";
const char C_QMLFORMEDITOR[]        = "QmlDesigner::FormEditor";
const char C_QMLEDITOR3D[]          = "QmlDesigner::Editor3D";
const char C_QMLEFFECTMAKER[]       = "QmlDesigner::EffectMaker";
const char C_QMLNAVIGATOR[]         = "QmlDesigner::Navigator";
const char C_QMLTEXTEDITOR[]        = "QmlDesigner::TextEditor";
const char C_QMLMATERIALBROWSER[]   = "QmlDesigner::MaterialBrowser";
const char C_QMLASSETSLIBRARY[]     = "QmlDesigner::AssetsLibrary";
const char C_QMLCOLLECTIONEDITOR[]  = "QmlDesigner::CollectionEditor";

// Special context for preview menu, shared b/w designer and text editor
const char C_QT_QUICK_TOOLS_MENU[] = "QmlDesigner::ToolsMenu";

// Actions
const char SWITCH_TEXT_DESIGN[]   = "QmlDesigner.SwitchTextDesign";
const char RESTORE_DEFAULT_VIEW[] = "QmlDesigner.RestoreDefaultView";
const char TOGGLE_LEFT_SIDEBAR[] = "QmlDesigner.ToggleLeftSideBar";
const char TOGGLE_RIGHT_SIDEBAR[] = "QmlDesigner.ToggleRightSideBar";
const char TOGGLE_STATES_EDITOR[] = "QmlDesigner.ToggleStatesEditor";
const char GO_INTO_COMPONENT[] = "QmlDesigner.GoIntoComponent";
const char EXPORT_AS_IMAGE[] = "QmlDesigner.ExportAsImage";
const char TAKE_SCREENSHOT[] = "QmlDesigner.TakeScreenshot";
const char FORMEDITOR_REFRESH[] = "QmlDesigner.FormEditor.Refresh";
const char FORMEDITOR_SNAPPING[] = "QmlDesigner.FormEditor.Snapping";
const char FORMEDITOR_NO_SNAPPING[] = "QmlDesigner.FormEditor.NoSnapping";
const char FORMEDITOR_NO_SNAPPING_AND_ANCHORING[] = "QmlDesigner.FormEditor.NoSnappingAndAnchoring";
const char FORMEDITOR_NO_SHOW_BOUNDING_RECTANGLE[] = "QmlDesigner.FormEditor.ShowBoundingRectangle";
const char EDIT3D_SELECTION_MODE[] = "QmlDesigner.Editor3D.SelectionModeToggle";
const char EDIT3D_MOVE_TOOL[]      = "QmlDesigner.Editor3D.MoveTool";
const char EDIT3D_ROTATE_TOOL[]    = "QmlDesigner.Editor3D.RotateTool";
const char EDIT3D_SCALE_TOOL[]     = "QmlDesigner.Editor3D.ScaleTool";
const char EDIT3D_FIT_SELECTED[]   = "QmlDesigner.Editor3D.FitSelected";
const char EDIT3D_ALIGN_CAMERAS[]  = "QmlDesigner.Editor3D.AlignCameras";
const char EDIT3D_ALIGN_VIEW[]     = "QmlDesigner.Editor3D.AlignView";
const char EDIT3D_EDIT_CAMERA[]    = "QmlDesigner.Editor3D.EditCameraToggle";
const char EDIT3D_ORIENTATION[]    = "QmlDesigner.Editor3D.OrientationToggle";
const char EDIT3D_EDIT_LIGHT[]     = "QmlDesigner.Editor3D.EditLightToggle";
const char EDIT3D_EDIT_SHOW_GRID[] = "QmlDesigner.Editor3D.ToggleGrid";
const char EDIT3D_EDIT_SELECT_BACKGROUND_COLOR[] = "QmlDesigner.Editor3D.SelectBackgroundColor";
const char EDIT3D_EDIT_SELECT_GRID_COLOR[] = "QmlDesigner.Editor3D.SelectGridColor";
const char EDIT3D_EDIT_RESET_BACKGROUND_COLOR[] = "QmlDesigner.Editor3D.ResetBackgroundColor";
const char EDIT3D_EDIT_SYNC_ENV_BACKGROUND[] = "QmlDesigner.Editor3D.SyncEnvBackground";
const char EDIT3D_EDIT_SHOW_SELECTION_BOX[] = "QmlDesigner.Editor3D.ToggleSelectionBox";
const char EDIT3D_EDIT_SHOW_ICON_GIZMO[] = "QmlDesigner.Editor3D.ToggleIconGizmo";
const char EDIT3D_EDIT_SHOW_CAMERA_FRUSTUM[] = "QmlDesigner.Editor3D.ToggleCameraFrustum";
const char EDIT3D_EDIT_SHOW_PARTICLE_EMITTER[] = "QmlDesigner.Editor3D.ToggleParticleEmitter";
const char EDIT3D_RESET_VIEW[]     = "QmlDesigner.Editor3D.ResetView";
const char EDIT3D_PARTICLE_MODE[]     = "QmlDesigner.Editor3D.ParticleViewModeToggle";
const char EDIT3D_PARTICLES_PLAY[]    = "QmlDesigner.Editor3D.ParticlesPlay";
const char EDIT3D_PARTICLES_SEEKER[]    = "QmlDesigner.Editor3D.ParticlesSeeker";
const char EDIT3D_PARTICLES_RESTART[] = "QmlDesigner.Editor3D.ParticlesRestart";
const char EDIT3D_VISIBILITY_TOGGLES[] = "QmlDesigner.Editor3D.VisibilityToggles";
const char EDIT3D_BACKGROUND_COLOR_ACTIONS[] = "QmlDesigner.Editor3D.BackgroundColorActions";
const char EDIT3D_BAKE_LIGHTS[] = "QmlDesigner.Editor3D.BakeLights";
const char EDIT3D_SNAP_TOGGLE[] = "QmlDesigner.Editor3D.SnapToggle";
const char EDIT3D_SNAP_CONFIG[] = "QmlDesigner.Editor3D.SnapConfig";

const char QML_DESIGNER_SUBFOLDER[] = "/designer/";
const char COMPONENT_BUNDLES_FOLDER[] = "/ComponentBundles";
const char COMPONENT_BUNDLES_ASSET_REF_FILE[] = "_asset_ref.json";
const char QUICK_3D_ASSETS_FOLDER[] = "/Quick3DAssets";
const char QUICK_3D_ASSET_LIBRARY_ICON_SUFFIX[] = "_libicon";
const char QUICK_3D_ASSET_IMPORT_DATA_NAME[] = "_importdata.json";
const char QUICK_3D_ASSET_IMPORT_DATA_OPTIONS_KEY[] = "import_options";
const char QUICK_3D_ASSET_IMPORT_DATA_SOURCE_KEY[] = "source_scene";
const char DEFAULT_ASSET_IMPORT_FOLDER[] = "/asset_imports";
const char MATERIAL_LIB_ID[] = "__materialLibrary__";

const char MIME_TYPE_ITEM_LIBRARY_INFO[] = "application/vnd.qtdesignstudio.itemlibraryinfo";
const char MIME_TYPE_ASSETS[]            = "application/vnd.qtdesignstudio.assets";
const char MIME_TYPE_MATERIAL[]          = "application/vnd.qtdesignstudio.material";
const char MIME_TYPE_TEXTURE[]           = "application/vnd.qtdesignstudio.texture";
const char MIME_TYPE_BUNDLE_EFFECT[]     = "application/vnd.qtdesignstudio.bundleeffect";
const char MIME_TYPE_BUNDLE_MATERIAL[]   = "application/vnd.qtdesignstudio.bundlematerial";
const char MIME_TYPE_BUNDLE_TEXTURE[]    = "application/vnd.qtdesignstudio.bundletexture";
const char MIME_TYPE_ASSET_IMAGE[]       = "application/vnd.qtdesignstudio.asset.image";
const char MIME_TYPE_ASSET_FONT[]        = "application/vnd.qtdesignstudio.asset.font";
const char MIME_TYPE_ASSET_SHADER[]      = "application/vnd.qtdesignstudio.asset.shader";
const char MIME_TYPE_ASSET_SOUND[]       = "application/vnd.qtdesignstudio.asset.sound";
const char MIME_TYPE_ASSET_VIDEO[]       = "application/vnd.qtdesignstudio.asset.video";
const char MIME_TYPE_ASSET_TEXTURE3D[]   = "application/vnd.qtdesignstudio.asset.texture3d";
const char MIME_TYPE_MODELNODE_LIST[]    = "application/vnd.qtdesignstudio.modelnode.list";
const char MIME_TYPE_ASSET_EFFECT[]      = "application/vnd.qtdesignstudio.asset.effect";

// Menus
const char M_VIEW_WORKSPACES[] = "QmlDesigner.Menu.View.Workspaces";

const int MODELNODE_PREVIEW_IMAGE_DIMENSIONS = 150;

const char EVENT_TIMELINE_ADDED[] = "timelineAdded";
const char EVENT_TRANSITION_ADDED[] = "transitionAdded";
const char EVENT_STATE_ADDED[] = "stateAdded";
const char EVENT_STATE_ADDED_AND_CLONED[] = "stateAddedAndCloned";
const char EVENT_STATE_CLONED[] = "stateCloned";
const char EVENT_STATE_EXTENDED[] = "stateExtended";
const char EVENT_CONNECTION_ADDED[] = "connectionAdded";
const char EVENT_PROPERTY_ADDED[] = "propertyAdded";
const char EVENT_ANNOTATION_ADDED[] = "annotationAdded";
const char EVENT_RESOURCE_IMPORTED[] = "resourceImported";
const char EVENT_ACTION_EXECUTED[] = "actionExecuted";
const char EVENT_HELP_REQUESTED[] = "helpRequested";
const char EVENT_IMPORT_ADDED[] = "importAdded:";
const char EVENT_BINDINGEDITOR_OPENED[] = "bindingEditorOpened";
const char EVENT_RICHTEXT_OPENED[] = "richtextEditorOpened";
const char EVENT_FORMEDITOR_TIME[] = "formEditor";
const char EVENT_3DEDITOR_TIME[] = "3DEditor";
const char EVENT_TIMELINE_TIME[] = "timeline";
const char EVENT_TRANSITIONEDITOR_TIME[] = "transitionEditor";
const char EVENT_CURVEDITOR_TIME[] = "curveEditor";
const char EVENT_STATESEDITOR_TIME[] = "statesEditor";
const char EVENT_TEXTEDITOR_TIME[] = "textEditor";
const char EVENT_TEXTUREEDITOR_TIME[] = "textureEditor";
const char EVENT_PROPERTYEDITOR_TIME[] = "propertyEditor";
const char EVENT_ASSETSLIBRARY_TIME[] = "assetsLibrary";
const char EVENT_EFFECTMAKER_TIME[] = "effectMaker";
const char EVENT_ITEMLIBRARY_TIME[] = "itemLibrary";
const char EVENT_TRANSLATIONVIEW_TIME[] = "translationView";
const char EVENT_NAVIGATORVIEW_TIME[] = "navigatorView";
const char EVENT_DESIGNMODE_TIME[] = "designMode";
const char EVENT_MATERIALEDITOR_TIME[] = "materialEditor";
const char EVENT_MATERIALBROWSER_TIME[] = "materialBrowser";
const char EVENT_CONTENTLIBRARY_TIME[] = "contentLibrary";
const char EVENT_INSIGHT_TIME[] = "insight";
const char EVENT_TOOLBAR_MODE_CHANGE[] = "ToolBarTriggerModeChange";
const char EVENT_TOOLBAR_PROJECT_SETTINGS[] = "ToolBarTriggerProjectSettings";
const char EVENT_TOOLBAR_RUN_PROJECT[] = "ToolBarRunProject";
const char EVENT_TOOLBAR_GO_FORWARD[] = "ToolBarGoForward";
const char EVENT_TOOLBAR_GO_BACKWARD[] = "ToolBarGoBackward";
const char EVENT_TOOLBAR_OPEN_FILE[] = "ToolBarOpenFile";
const char EVENT_TOOLBAR_CLOSE_DOCUMENT[] = "ToolBarCloseCurrentDocument";
const char EVENT_TOOLBAR_SHARE_APPLICATION[] = "ToolBarShareApplication";
const char EVENT_TOOLBAR_SET_CURRENT_WORKSPACE[] = "ToolBarSetCurrentWorkspace";
const char EVENT_TOOLBAR_EDIT_GLOBAL_ANNOTATION[] = "ToolBarEditGlobalAnnotation";
const char EVENT_STATUSBAR_SHOW_ZOOM[] = "StatusBarShowZoomMenu";
const char EVENT_STATUSBAR_SET_STYLE[] = "StatusBarSetCurrentStyle";

const char PROPERTY_EDITOR_CLASSNAME_PROPERTY[] = "__classNamePrivateInternal";

// Copy/Paste Headers
const char HEADER_3DPASTE_CONTENT[] = "// __QmlDesigner.Editor3D.Paste__ \n";

const char OBJECT_NAME_ASSET_LIBRARY[] = "QQuickWidgetAssetLibrary";
const char OBJECT_NAME_CONTENT_LIBRARY[] = "QQuickWidgetContentLibrary";
const char OBJECT_NAME_BUSY_INDICATOR[] = "QQuickWidgetBusyIndicator";
const char OBJECT_NAME_COMPONENT_LIBRARY[] = "QQuickWidgetComponentLibrary";
const char OBJECT_NAME_EFFECT_MAKER[] = "QQuickWidgetEffectMaker";
const char OBJECT_NAME_MATERIAL_BROWSER[] = "QQuickWidgetMaterialBrowser";
const char OBJECT_NAME_MATERIAL_EDITOR[] = "QQuickWidgetMaterialEditor";
const char OBJECT_NAME_PROPERTY_EDITOR[] = "QQuickWidgetPropertyEditor";
const char OBJECT_NAME_COLLECTION_EDITOR[] = "QQuickWidgetQDSCollectionEditor";
const char OBJECT_NAME_STATES_EDITOR[] = "QQuickWidgetStatesEditor";
const char OBJECT_NAME_TEXTURE_EDITOR[] = "QQuickWidgetTextureEditor";
const char OBJECT_NAME_TOP_TOOLBAR[] = "QQuickWidgetTopToolbar";
const char OBJECT_NAME_STATUSBAR[] = "QQuickWidgetStatusbar";
const char OBJECT_NAME_TOP_FEEDBACK[] = "QQuickWidgetQDSFeedback";
const char OBJECT_NAME_NEW_DIALOG[] = "QQuickWidgetQDSNewDialog";
const char OBJECT_NAME_SPLASH_SCREEN[] = "QQuickWidgetSplashScreen";
const char OBJECT_NAME_WELCOME_PAGE[] = "QQuickWidgetQDSWelcomePage";
const char OBJECT_NAME_CONNECTION_EDITOR[] = "QQuickWidgetConnectionEditor";

const char ENVIRONMENT_SHOW_QML_ERRORS[] = "QMLDESIGNER_SHOW_QML_ERRORS";

namespace Internal {
    enum { debug = 0 };
}

} // Constants
} // QmlDesigner
