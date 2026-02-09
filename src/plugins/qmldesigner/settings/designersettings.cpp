// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "designersettings.h"

#include "qmldesignertr.h"

#include <utils/layoutbuilder.h>
#include <utils/qtcsettings.h>

#include <QGuiApplication>
#include <QMessageBox>

using namespace Utils;

namespace QmlDesigner {

DesignerSettings::DesignerSettings()
    : m_settings(&userSettings())
{
    setAutoApply(false);

    setSettingsGroups("QML", "Designer");

    itemSpacing.setSettingsKey("ItemSpacing");
    itemSpacing.setDefaultValue(6);
    itemSpacing.setRange(0, 50);
    itemSpacing.setLabelText(Tr::tr("Sibling component spacing:"));

    containerPadding.setSettingsKey("ContainerPadding");
    containerPadding.setDefaultValue(8);
    containerPadding.setRange(0, 10);
    containerPadding.setLabelText(Tr::tr("Parent component padding:"));

    canvasWidth.setSettingsKey("CanvasWidth");
    canvasWidth.setDefaultValue(40000);
    canvasWidth.setLabelText(Tr::tr("Width:"));
    canvasWidth.setRange(0, 100000);
    canvasWidth.setSingleStep(100);
    canvasWidth.setValue(10000);

    canvasHeight.setSettingsKey("CanvasHeight");
    canvasHeight.setDefaultValue(40000);
    canvasHeight.setLabelText(Tr::tr("Height:"));
    canvasHeight.setRange(0, 100000);
    canvasHeight.setSingleStep(100);
    canvasHeight.setValue(10000);

    rootElementInitWidth.setSettingsKey("RootElementInitWidth");
    rootElementInitWidth.setDefaultValue(640);
    rootElementInitWidth.setRange(0, 100000);
    rootElementInitWidth.setLabelText(Tr::tr("Width:"));

    rootElementInitHeight.setSettingsKey("RootElementInitHeight");
    rootElementInitHeight.setDefaultValue(480);
    rootElementInitHeight.setRange(0, 100000);
    rootElementInitHeight.setLabelText(Tr::tr("Height:"));

    warningForFeaturesInDesigner.setSettingsKey("WarnAboutQtQuickFeaturesInDesigner");
    warningForFeaturesInDesigner.setDefaultValue(true);
    warningForFeaturesInDesigner.setLabelText(
        Tr::tr("Warn about unsupported features in .ui.qml files"));
    warningForFeaturesInDesigner.setToolTip(Tr::tr(
        "Warns about QML features that are not properly supported by the Qt Design Studio."));

    warningForQmlFilesInsteadOfUiQmlFiles.setSettingsKey("WarnAboutQmlFilesInsteadOfUiQmlFiles");
    warningForQmlFilesInsteadOfUiQmlFiles.setDefaultValue(true);
    warningForQmlFilesInsteadOfUiQmlFiles.setLabelText(
        Tr::tr("Warn about using .qml files instead of .ui.qml files"));
    warningForQmlFilesInsteadOfUiQmlFiles.setToolTip(Tr::tr(
        "Qt Quick Designer will propose to open .ui.qml files instead of opening a .qml file."));

    warningForDesignerFeaturesInEditor.setSettingsKey("WarnAboutQtQuickDesignerFeaturesInCodeEditor");
    warningForDesignerFeaturesInEditor.setDefaultValue(false);
    warningForDesignerFeaturesInEditor.setLabelText(
        Tr::tr("Warn about unsupported features of .ui.qml files in code editor"));
    warningForDesignerFeaturesInEditor.setToolTip(
        Tr::tr("Also warns in the code editor about QML features that are not properly "
               "supported by the Qt Quick Designer."));

    showDebugView.setSettingsKey("ShowQtQuickDesignerDebugView");
    showDebugView.setDefaultValue(false);

    enableDebugView.setSettingsKey("EnableQtQuickDesignerDebugView");
    enableDebugView.setDefaultValue(false);
    enableDebugView.setLabelText(Tr::tr("Enable the debugging view"));

    edit3DViewBackgroundColor.setSettingsKey("Edit3DViewBackgroundColor");
    edit3DViewBackgroundColor.setDefaultValue(QStringList{"#222222", "#999999"});

    edit3DViewGridLineColor.setSettingsKey("Edit3DViewGridLineColor");
    edit3DViewGridLineColor.setDefaultValue("#cccccc");

    edit3DViewSnapAbsolute.setSettingsKey("Edit3DViewSnapAbsolute");
    edit3DViewSnapAbsolute.setDefaultValue(true);

    edit3DViewSnapEnabled.setSettingsKey("Edit3DViewSnapEnabled");
    edit3DViewSnapEnabled.setDefaultValue(false);

    edit3DViewSnapPosition.setSettingsKey("Edit3DViewSnapPosition");
    edit3DViewSnapPosition.setDefaultValue(true);

    edit3DViewSnapPositionInterval.setSettingsKey("Edit3DViewSnapPositionInterval");
    edit3DViewSnapPositionInterval.setDefaultValue(50.0);

    edit3DViewSnapRotation.setSettingsKey("Edit3DViewSnapRotation");
    edit3DViewSnapRotation.setDefaultValue(true);

    edit3DViewSnapRotationInterval.setSettingsKey("Edit3DViewSnapRotationInterval");
    edit3DViewSnapRotationInterval.setDefaultValue(5.0);

    edit3DViewSnapScale.setSettingsKey("Edit3DViewSnapScale");
    edit3DViewSnapScale.setDefaultValue(true);

    edit3DViewSnapScaleInterval.setSettingsKey("Edit3DViewSnapScaleInterval");
    edit3DViewSnapScaleInterval.setDefaultValue(10.0);

    alwaysSaveInCrumbleBar.setSettingsKey("AlwaysSaveInCrumbleBar");
    alwaysSaveInCrumbleBar.setDefaultValue(false);
    alwaysSaveInCrumbleBar.setLabelText(
        Tr::tr("Always save when leaving subcomponent in bread crumb"));

    controlsStyle.setSettingsKey("ControlsStyle");
    //controlsStyle.setDefaultValue(); CONTROLS_STYLE);
    controlsStyle.setPlaceHolderText(Tr::tr("Default style"));
    controlsStyle.setDisplayStyle(StringAspect::LineEditDisplay);

    controls2Style.setSettingsKey("Controls2Style");
    controls2Style.setLabelText(Tr::tr("Controls 2 style:"));
    controls2Style.setDisplayStyle(SelectionAspect::DisplayStyle::ComboBox);
    controls2Style.addOption({"Default"});
    controls2Style.addOption({"Material"});
    controls2Style.addOption({"Universal"});
    controls2Style.addOnVolatileValueChanged(this, [this] {
        controlsStyle.setVolatileValue(controlsStyle.volatileValue());
    });

    typeOfQsTrFunction.setSettingsKey("TypeOfQsTrFunction");
    typeOfQsTrFunction.setDefaultValue(0);
    typeOfQsTrFunction.addOption(Tr::tr("qsTr()"));
    typeOfQsTrFunction.addOption(Tr::tr("qsTrId()"));
    typeOfQsTrFunction.addOption(Tr::tr("qsTranslate()"));

    showPropertyEditorWarnings.setSettingsKey("ShowPropertyEditorWarnings");
    showPropertyEditorWarnings.setDefaultValue(true);
    showPropertyEditorWarnings.setLabelText(Tr::tr("Show property editor warnings"));

    enableModelExceptionOutput.setSettingsKey("WarnException");
    enableModelExceptionOutput.setDefaultValue(false);
    enableModelExceptionOutput.setLabelText(Tr::tr("Show warn exceptions"));

    puppetKillTimeout.setSettingsKey("PuppetKillTimeout");
    puppetKillTimeout.setDefaultValue(30000);

    static QStringList puppetModeList{"", "all", "editormode", "rendermode", "previewmode",
                                      "bakelightsmode", "import3dmode"};

    debugPuppet.setSettingsKey("DebugPuppet");
    debugPuppet.setDefaultValue(QString());
    debugPuppet.setDisplayStyle(SelectionAspect::DisplayStyle::ComboBox);
    debugPuppet.setLabelText(Tr::tr("Debug QML Puppet:"));
    for (const QString &mode : puppetModeList)
        debugPuppet.addOption(mode);

    forwardPuppetOutput.setSettingsKey("ForwardPuppetOutput");
    forwardPuppetOutput.setDefaultValue(QString());
    forwardPuppetOutput.setDisplayStyle(SelectionAspect::DisplayStyle::ComboBox);
    forwardPuppetOutput.setLabelText(Tr::tr("Forward QML Puppet output:"));
    for (const QString &mode : puppetModeList)
        forwardPuppetOutput.addOption(mode);

    navigatorShowReferenceNodes.setSettingsKey("NavigatorShowReferenceNodes");
    navigatorShowReferenceNodes.setDefaultValue(false);

    navigatorShowOnlyVisibleItems.setSettingsKey("NavigatorShowOnlyVisibleItems");
    navigatorShowOnlyVisibleItems.setDefaultValue(false);

    navigatorReverseItemOrder.setSettingsKey("NavigatorReverseItemOrder");
    navigatorReverseItemOrder.setDefaultValue(false);

    navigatorColorizeIcons.setSettingsKey("NavigatorColorizeIcons");
    navigatorColorizeIcons.setDefaultValue(false);

    reformatUiQmlFiles.setSettingsKey("ReformatUiQmlFiles");
    reformatUiQmlFiles.setDefaultValue(true);
    reformatUiQmlFiles.setLabelText(Tr::tr("Always auto-format ui.qml files in Design mode"));

    // The settings can be used to turn off the feature, if there are serious issues
    ignoreDevicePixelRatio.setSettingsKey("IgnoreDevicePixelRaio");
    ignoreDevicePixelRatio.setDefaultValue(false);

    showDebugSettings.setSettingsKey("ShowDebugSettings");
    showDebugSettings.setDefaultValue(false);
    showDebugSettings.setLabelText(Tr::tr("Show the debugging view"));

    enableTimelineView.setSettingsKey("EnableTimelineView");
    enableTimelineView.setDefaultValue(true);
    enableTimelineView.setLabelText(Tr::tr("Enable Timeline editor"));

    enableDockWidgetContentMinSize.setSettingsKey("EnableDockWidgetContentMinSize");
    enableDockWidgetContentMinSize.setDefaultValue(true);
    enableDockWidgetContentMinSize.setLabelText(
        Tr::tr("Enable DockWidget content minimum size"));

    colorPaletteRecent.setSettingsKey("ColorPaletteRecent");
    colorPaletteRecent.setDefaultValue(QStringList());

    colorPaletteFavorite.setSettingsKey("ColorPaletteFavorite");
    colorPaletteFavorite.setDefaultValue(QStringList());

    alwaysDesignMode.setSettingsKey("AlwaysDesignMode");
    alwaysDesignMode.setDefaultValue(true);
    alwaysDesignMode.setLabelText(Tr::tr("Always open ui.qml files in Design mode"));

    disableItemLibraryUpdateTimer.setSettingsKey("DisableItemLibraryUpdateTimer");
    disableItemLibraryUpdateTimer.setDefaultValue(false);

    askBeforeDeletingAsset.setSettingsKey("AskBeforeDeletingAsset");
    askBeforeDeletingAsset.setDefaultValue(true);
    askBeforeDeletingAsset.setLabelText(
        Tr::tr("Ask for confirmation before deleting asset"));

    askBeforeDeletingContentLibFile.setSettingsKey("AskBeforeDeletingContentLibFile");
    askBeforeDeletingContentLibFile.setDefaultValue(true);
    askBeforeDeletingContentLibFile.setLabelText(
        Tr::tr("Ask for confirmation before deleting content library files"));

    smoothRendering.setSettingsKey("SmoothRendering");
    smoothRendering.setDefaultValue(true);
    smoothRendering.setLabelText(Tr::tr("Smooth rendering:"));
    smoothRendering.setToolTip(Tr::tr("Enable smooth rendering in the 2D view."));

    editorZoomFactor.setSettingsKey("EditorZoomFactor");
    editorZoomFactor.setDefaultValue(1.0);

    actionsMergeTemplateEnabled.setSettingsKey("ActionsMergeTemplateEnabled");
    actionsMergeTemplateEnabled.setDefaultValue(false);

    downloadableBundlesLocation.setSettingsKey("DownloadableBundlesLocation");
    downloadableBundlesLocation.setDefaultValue("https://cdn.qt.io/designstudio/bundles");

    contentLibraryNewFlagExpirationInDays.setSettingsKey("ContentLibraryNewFlagExpirationInDays");
    contentLibraryNewFlagExpirationInDays.setDefaultValue(3);

    groqApiKey.setSettingsKey("GroqApiKey");
    groqApiKey.setDefaultValue({});

    setLayouter([this] {
        using namespace Layouting;

        return Column {
            Row {
                Group {
                    title(Tr::tr("Snapping")),
                    Form {
                        containerPadding, br,
                        itemSpacing
                    },
                },
                Group {
                    title(Tr::tr("Canvas")),
                    Form {
                        canvasWidth, br,
                        canvasHeight, br,
                        smoothRendering
                    },
                },
                Group {
                    title(Tr::tr("Root Component Init Size")),
                    Form {
                        rootElementInitWidth, br,
                        rootElementInitHeight
                    },
                },
                Group {
                    title(Tr::tr("Styling")),
                    Form {
                        Tr::tr("Controls style:"),
                        controlsStyle,
                        PushButton {
                            text(Tr::tr("Reset Style")),
                            onClicked(this, [this] {
                                controlsStyle.setVolatileValue({});
                            }),
                        },
                        br,
                        controls2Style,
                    },
                },
            },
            Row {
                Group {
                    title(Tr::tr("Subcomponents")),
                    Column {
                        alwaysSaveInCrumbleBar,
                        st
                    },
                },
            },
            Row {
                Group {
                    title(Tr::tr("Warnings")),
                    Column {
                        warningForFeaturesInDesigner,
                        warningForDesignerFeaturesInEditor,
                        warningForQmlFilesInsteadOfUiQmlFiles,
                    }
                },
                Group {
                    title(Tr::tr("Internationalization")),
                    Column {
                        typeOfQsTrFunction,
                    }
                }
            },
            Group {
                title(Tr::tr("Features")),
                Grid {
                    alwaysDesignMode, reformatUiQmlFiles, br,
                    askBeforeDeletingAsset, askBeforeDeletingContentLibFile, br,
                    enableTimelineView, br,
                    enableDockWidgetContentMinSize,
                },
            },
            Group {
                title(Tr::tr("Debugging")),
                Grid {
                    showDebugSettings,
                    showPropertyEditorWarnings,
                    Row { forwardPuppetOutput }, br,
                    enableDebugView,
                    enableModelExceptionOutput,
                    Row { debugPuppet }
                }
            },
            st
        };
    });

    readSettings();
}

DesignerSettings::~DesignerSettings() = default;

void DesignerSettings::apply()
{
    bool restartNecessary = enableModelExceptionOutput.isDirty()
            || forwardPuppetOutput.isDirty()
            || debugPuppet.isDirty()
            || enableTimelineView.isDirty()
            || enableDockWidgetContentMinSize.isDirty();

    if (restartNecessary) {
        QMessageBox::information(Utils::dialogParent(),
                                 Tr::tr("Restart Required"),
                                 Tr::tr("The made changes will take effect after a "
                                        "restart of the QML Puppet or %1.")
                                 .arg(QGuiApplication::applicationDisplayName()));
    }

    AspectContainer::apply();
}

DesignerSettings &designerSettings()
{
    static DesignerSettings theDesignerSettings;
    return theDesignerSettings;
}

} // namespace QmlDesigner
