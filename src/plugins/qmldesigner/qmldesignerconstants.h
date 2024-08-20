// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <qmldesignercoreconstants.h>

namespace QmlDesigner {
namespace Constants {

// Context
inline constexpr char qmlDesignerContextId[] = "QmlDesigner::QmlDesignerMain";
inline constexpr char qmlFormEditorContextId[] = "QmlDesigner::FormEditor";
inline constexpr char qml3DEditorContextId[] = "QmlDesigner::Editor3D";
inline constexpr char qmlNavigatorContextId[] = "QmlDesigner::Navigator";
inline constexpr char qmlMaterialBrowserContextId[] = "QmlDesigner::MaterialBrowser";
inline constexpr char qmlAssetsLibraryContextId[] = "QmlDesigner::AssetsLibrary";

// Special context for preview menu, shared b/w designer and text editor
inline constexpr char qtQuickToolsMenuContextId[] = "QmlDesigner::ToolsMenu";

// Actions
inline constexpr char EDIT3D_SELECTION_MODE[] = "QmlDesigner.Editor3D.SelectionModeToggle";
inline constexpr char EDIT3D_MOVE_TOOL[] = "QmlDesigner.Editor3D.MoveTool";
inline constexpr char EDIT3D_ROTATE_TOOL[] = "QmlDesigner.Editor3D.RotateTool";
inline constexpr char EDIT3D_SCALE_TOOL[] = "QmlDesigner.Editor3D.ScaleTool";
inline constexpr char EDIT3D_FIT_SELECTED[] = "QmlDesigner.Editor3D.FitSelected";
inline constexpr char EDIT3D_ALIGN_CAMERAS[] = "QmlDesigner.Editor3D.AlignCameras";
inline constexpr char EDIT3D_ALIGN_VIEW[] = "QmlDesigner.Editor3D.AlignView";
inline constexpr char EDIT3D_EDIT_CAMERA[] = "QmlDesigner.Editor3D.EditCameraToggle";
inline constexpr char EDIT3D_ORIENTATION[] = "QmlDesigner.Editor3D.OrientationToggle";
inline constexpr char EDIT3D_EDIT_LIGHT[] = "QmlDesigner.Editor3D.EditLightToggle";
inline constexpr char EDIT3D_EDIT_SHOW_GRID[] = "QmlDesigner.Editor3D.ToggleGrid";
inline constexpr char EDIT3D_EDIT_SHOW_LOOKAT[] = "QmlDesigner.Editor3D.ToggleLookAt";
inline constexpr char EDIT3D_EDIT_SELECT_BACKGROUND_COLOR[]
    = "QmlDesigner.Editor3D.SelectBackgroundColor";
inline constexpr char EDIT3D_EDIT_SELECT_GRID_COLOR[] = "QmlDesigner.Editor3D.SelectGridColor";
inline constexpr char EDIT3D_EDIT_RESET_BACKGROUND_COLOR[]
    = "QmlDesigner.Editor3D.ResetBackgroundColor";
inline constexpr char EDIT3D_EDIT_SYNC_ENV_BACKGROUND[] = "QmlDesigner.Editor3D.SyncEnvBackground";
inline constexpr char EDIT3D_EDIT_SHOW_SELECTION_BOX[] = "QmlDesigner.Editor3D.ToggleSelectionBox";
inline constexpr char EDIT3D_EDIT_SHOW_ICON_GIZMO[] = "QmlDesigner.Editor3D.ToggleIconGizmo";
inline constexpr char EDIT3D_EDIT_SHOW_CAMERA_FRUSTUM[]
    = "QmlDesigner.Editor3D.ToggleCameraFrustum";
inline constexpr char EDIT3D_EDIT_SHOW_PARTICLE_EMITTER[]
    = "QmlDesigner.Editor3D.ToggleParticleEmitter";
inline constexpr char EDIT3D_RESET_VIEW[] = "QmlDesigner.Editor3D.ResetView";
inline constexpr char EDIT3D_PARTICLE_MODE[] = "QmlDesigner.Editor3D.ParticleViewModeToggle";
inline constexpr char EDIT3D_PARTICLES_PLAY[] = "QmlDesigner.Editor3D.ParticlesPlay";
inline constexpr char EDIT3D_PARTICLES_SEEKER[] = "QmlDesigner.Editor3D.ParticlesSeeker";
inline constexpr char EDIT3D_PARTICLES_RESTART[] = "QmlDesigner.Editor3D.ParticlesRestart";
inline constexpr char EDIT3D_SPLIT_VIEW[] = "QmlDesigner.Editor3D.SplitViewToggle";
inline constexpr char EDIT3D_VISIBILITY_TOGGLES[] = "QmlDesigner.Editor3D.VisibilityToggles";
inline constexpr char EDIT3D_BACKGROUND_COLOR_ACTIONS[]
    = "QmlDesigner.Editor3D.BackgroundColorActions";
inline constexpr char EDIT3D_BAKE_LIGHTS[] = "QmlDesigner.Editor3D.BakeLights";
inline constexpr char EDIT3D_SNAP_TOGGLE[] = "QmlDesigner.Editor3D.SnapToggle";
inline constexpr char EDIT3D_SNAP_CONFIG[] = "QmlDesigner.Editor3D.SnapConfig";
inline constexpr char EDIT3D_CAMERA_SPEED_CONFIG[] = "QmlDesigner.Editor3D.CameraSpeedConfig";

inline constexpr char BUNDLE_JSON_FILENAME[] = "bundle.json";
inline constexpr char BUNDLE_SUFFIX[] = "qdsbundle";
inline constexpr char COMPONENT_BUNDLES_EFFECT_BUNDLE_TYPE[] = "Effects";
inline constexpr char COMPONENT_BUNDLES_USER_EFFECT_BUNDLE_TYPE[] = "UserEffects";
inline constexpr char COMPONENT_BUNDLES_USER_3D_BUNDLE_TYPE[] = "User3D";
inline constexpr char COMPONENT_BUNDLES_ASSET_REF_FILE[] = "_asset_ref.json";
inline constexpr char QUICK_3D_ASSET_LIBRARY_ICON_SUFFIX[] = "_libicon";
inline constexpr char QUICK_3D_ASSET_IMPORT_DATA_NAME[] = "_importdata.json";
inline constexpr char QUICK_3D_ASSET_IMPORT_DATA_OPTIONS_KEY[] = "import_options";
inline constexpr char QUICK_3D_ASSET_IMPORT_DATA_SOURCE_KEY[] = "source_scene";
inline constexpr char OLD_EFFECTS_IMPORT_FOLDER[] = "/asset_imports/Effects";
inline constexpr char OLD_COMPONENT_BUNDLES_EFFECT_BUNDLE_TYPE[] = "EffectBundle";
inline constexpr char MATERIAL_LIB_ID[] = "__materialLibrary__";

inline constexpr char MIME_TYPE_ITEM_LIBRARY_INFO[]
    = "application/vnd.qtdesignstudio.itemlibraryinfo";
inline constexpr char MIME_TYPE_ASSETS[] = "application/vnd.qtdesignstudio.assets";
inline constexpr char MIME_TYPE_MATERIAL[] = "application/vnd.qtdesignstudio.material";
inline constexpr char MIME_TYPE_TEXTURE[] = "application/vnd.qtdesignstudio.texture";
inline constexpr char MIME_TYPE_BUNDLE_ITEM[] = "application/vnd.qtdesignstudio.bundleitem";
inline constexpr char MIME_TYPE_BUNDLE_MATERIAL[] = "application/vnd.qtdesignstudio.bundlematerial";
inline constexpr char MIME_TYPE_BUNDLE_TEXTURE[] = "application/vnd.qtdesignstudio.bundletexture";
inline constexpr char MIME_TYPE_ASSET_IMAGE[] = "application/vnd.qtdesignstudio.asset.image";
inline constexpr char MIME_TYPE_ASSET_FONT[] = "application/vnd.qtdesignstudio.asset.font";
inline constexpr char MIME_TYPE_ASSET_SHADER[] = "application/vnd.qtdesignstudio.asset.shader";
inline constexpr char MIME_TYPE_ASSET_SOUND[] = "application/vnd.qtdesignstudio.asset.sound";
inline constexpr char MIME_TYPE_ASSET_VIDEO[] = "application/vnd.qtdesignstudio.asset.video";
inline constexpr char MIME_TYPE_ASSET_TEXTURE3D[]
    = "application/vnd.qtdesignstudio.asset.texture3d";
inline constexpr char MIME_TYPE_MODELNODE_LIST[] = "application/vnd.qtdesignstudio.modelnode.list";
inline constexpr char MIME_TYPE_ASSET_EFFECT[] = "application/vnd.qtdesignstudio.asset.effect";

// Menus
inline constexpr char M_VIEW_WORKSPACES[] = "QmlDesigner.Menu.View.Workspaces";


inline constexpr char EVENT_TIMELINE_ADDED[] = "timelineAdded";
inline constexpr char EVENT_TRANSITION_ADDED[] = "transitionAdded";
inline constexpr char EVENT_STATE_ADDED[] = "stateAdded";
inline constexpr char EVENT_STATE_ADDED_AND_CLONED[] = "stateAddedAndCloned";
inline constexpr char EVENT_STATE_CLONED[] = "stateCloned";
inline constexpr char EVENT_STATE_EXTENDED[] = "stateExtended";
inline constexpr char EVENT_CONNECTION_ADDED[] = "connectionAdded";
inline constexpr char EVENT_PROPERTY_ADDED[] = "propertyAdded";
inline constexpr char EVENT_ANNOTATION_ADDED[] = "annotationAdded";
inline constexpr char EVENT_RESOURCE_IMPORTED[] = "resourceImported";
inline constexpr char EVENT_ACTION_EXECUTED[] = "actionExecuted";
inline constexpr char EVENT_HELP_REQUESTED[] = "helpRequested";
inline constexpr char EVENT_IMPORT_ADDED[] = "importAdded:";
inline constexpr char EVENT_BINDINGEDITOR_OPENED[] = "bindingEditorOpened";
inline constexpr char EVENT_RICHTEXT_OPENED[] = "richtextEditorOpened";
inline constexpr char EVENT_FORMEDITOR_TIME[] = "formEditor";
inline constexpr char EVENT_3DEDITOR_TIME[] = "3DEditor";
inline constexpr char EVENT_TIMELINE_TIME[] = "timeline";
inline constexpr char EVENT_TRANSITIONEDITOR_TIME[] = "transitionEditor";
inline constexpr char EVENT_CURVEDITOR_TIME[] = "curveEditor";
inline constexpr char EVENT_STATESEDITOR_TIME[] = "statesEditor";
inline constexpr char EVENT_TEXTEDITOR_TIME[] = "textEditor";
inline constexpr char EVENT_TEXTUREEDITOR_TIME[] = "textureEditor";
inline constexpr char EVENT_PROPERTYEDITOR_TIME[] = "propertyEditor";
inline constexpr char EVENT_ASSETSLIBRARY_TIME[] = "assetsLibrary";
inline constexpr char EVENT_EFFECTCOMPOSER_NODE[] = "effectComposerNode";
inline constexpr char EVENT_EFFECTCOMPOSER_TIME[] = "effectComposerTime";
inline constexpr char EVENT_ITEMLIBRARY_TIME[] = "itemLibrary";
inline constexpr char EVENT_TRANSLATIONVIEW_TIME[] = "translationView";
inline constexpr char EVENT_NAVIGATORVIEW_TIME[] = "navigatorView";
inline constexpr char EVENT_DESIGNMODE_TIME[] = "designMode";
inline constexpr char EVENT_MATERIALEDITOR_TIME[] = "materialEditor";
inline constexpr char EVENT_MATERIALBROWSER_TIME[] = "materialBrowser";
inline constexpr char EVENT_CONTENTLIBRARY_TIME[] = "contentLibrary";
inline constexpr char EVENT_INSIGHT_TIME[] = "insight";
inline constexpr char EVENT_MODELEDITOR_TIME[] = "modelEditor";
inline constexpr char EVENT_TOOLBAR_MODE_CHANGE[] = "ToolBarTriggerModeChange";
inline constexpr char EVENT_TOOLBAR_PROJECT_SETTINGS[] = "ToolBarTriggerProjectSettings";
inline constexpr char EVENT_TOOLBAR_RUN_PROJECT[] = "ToolBarRunProject";
inline constexpr char EVENT_TOOLBAR_GO_FORWARD[] = "ToolBarGoForward";
inline constexpr char EVENT_TOOLBAR_GO_BACKWARD[] = "ToolBarGoBackward";
inline constexpr char EVENT_TOOLBAR_OPEN_FILE[] = "ToolBarOpenFile";
inline constexpr char EVENT_TOOLBAR_CLOSE_DOCUMENT[] = "ToolBarCloseCurrentDocument";
inline constexpr char EVENT_TOOLBAR_SHARE_APPLICATION[] = "ToolBarShareApplication";
inline constexpr char EVENT_TOOLBAR_SET_CURRENT_WORKSPACE[] = "ToolBarSetCurrentWorkspace";
inline constexpr char EVENT_TOOLBAR_EDIT_GLOBAL_ANNOTATION[] = "ToolBarEditGlobalAnnotation";
inline constexpr char EVENT_STATUSBAR_SHOW_ZOOM[] = "StatusBarShowZoomMenu";
inline constexpr char EVENT_STATUSBAR_SET_STYLE[] = "StatusBarSetCurrentStyle";

inline constexpr char PROPERTY_EDITOR_CLASSNAME_PROPERTY[] = "__classNamePrivateInternal";

// Copy/Paste Headers
inline constexpr char HEADER_3DPASTE_CONTENT[] = "// __QmlDesigner.Editor3D.Paste__ \n";

inline constexpr char OBJECT_NAME_ASSET_LIBRARY[] = "QQuickWidgetAssetLibrary";
inline constexpr char OBJECT_NAME_CONTENT_LIBRARY[] = "QQuickWidgetContentLibrary";
inline constexpr char OBJECT_NAME_BUSY_INDICATOR[] = "QQuickWidgetBusyIndicator";
inline constexpr char OBJECT_NAME_COMPONENT_LIBRARY[] = "QQuickWidgetComponentLibrary";
inline constexpr char OBJECT_NAME_EFFECT_COMPOSER[] = "QQuickWidgetEffectComposer";
inline constexpr char OBJECT_NAME_MATERIAL_BROWSER[] = "QQuickWidgetMaterialBrowser";
inline constexpr char OBJECT_NAME_MATERIAL_EDITOR[] = "QQuickWidgetMaterialEditor";
inline constexpr char OBJECT_NAME_PROPERTY_EDITOR[] = "QQuickWidgetPropertyEditor";
inline constexpr char OBJECT_NAME_STATES_EDITOR[] = "QQuickWidgetStatesEditor";
inline constexpr char OBJECT_NAME_TEXTURE_EDITOR[] = "QQuickWidgetTextureEditor";
inline constexpr char OBJECT_NAME_TOP_TOOLBAR[] = "QQuickWidgetTopToolbar";
inline constexpr char OBJECT_NAME_STATUSBAR[] = "QQuickWidgetStatusbar";
inline constexpr char OBJECT_NAME_TOP_FEEDBACK[] = "QQuickWidgetQDSFeedback";
inline constexpr char OBJECT_NAME_NEW_DIALOG[] = "QQuickWidgetQDSNewDialog";
inline constexpr char OBJECT_NAME_SPLASH_SCREEN[] = "QQuickWidgetSplashScreen";
inline constexpr char OBJECT_NAME_WELCOME_PAGE[] = "QQuickWidgetQDSWelcomePage";
inline constexpr char OBJECT_NAME_CONNECTION_EDITOR[] = "QQuickWidgetConnectionEditor";

inline constexpr char ENVIRONMENT_SHOW_QML_ERRORS[] = "QMLDESIGNER_SHOW_QML_ERRORS";

namespace Internal {
    enum { debug = 0 };
}

} // Constants
} // QmlDesigner
