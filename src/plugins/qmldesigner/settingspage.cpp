// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "settingspage.h"

#include "designersettings.h"
#include "qmldesignerplugin.h"
#include "qmldesignertr.h"

#include <coreplugin/icore.h>

#include <qmljseditor/qmljseditorconstants.h>
#include <qmljstools/qmljstoolsconstants.h>

#include <qmlprojectmanager/qmlproject.h>

#include <utils/layoutbuilder.h>
#include <utils/pathchooser.h>
#include <utils/qtcassert.h>
#include <utils/environment.h>

#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QRadioButton>
#include <QSpinBox>
#include <QTextStream>
#include <QVBoxLayout>

namespace QmlDesigner {
namespace Internal {

static QStringList puppetModes()
{
    static QStringList puppetModeList{"", "all", "editormode", "rendermode", "previewmode",
                                      "bakelightsmode", "import3dmode"};
    return puppetModeList;
}

class SettingsPageWidget final : public Core::IOptionsPageWidget
{
    Q_DECLARE_TR_FUNCTIONS(QmlDesigner::Internal::SettingsPage)

public:
    explicit SettingsPageWidget();

    void apply() final;

    QHash<QByteArray, QVariant> newSettings() const;
    void readSettings();

private:
    QSpinBox *m_spinItemSpacing;
    QSpinBox *m_spinSnapMargin;
    QSpinBox *m_spinCanvasHeight;
    QSpinBox *m_spinCanvasWidth;
    QCheckBox *m_smoothRendering;
    QSpinBox *m_spinRootItemInitHeight;
    QSpinBox *m_spinRootItemInitWidth;
    QLineEdit *m_styleLineEdit;
    QComboBox *m_controls2StyleComboBox;
    QCheckBox *m_alwaysSaveSubcomponentsCheckBox;
    QCheckBox *m_designerWarningsInEditorCheckBox;
    QCheckBox *m_designerWarningsCheckBox;
    QCheckBox *m_designerWarningsUiQmlfiles;
    QRadioButton *m_useQsTrFunctionRadioButton;
    QRadioButton *m_useQsTrIdFunctionRadioButton;
    QRadioButton *m_useQsTranslateFunctionRadioButton;
    QCheckBox *m_designerAlwaysDesignModeCheckBox;
    QCheckBox *m_askBeforeDeletingAssetCheckBox;
    QCheckBox *m_askBeforeDeletingContentLibFileCheckBox;
    QCheckBox *m_alwaysAutoFormatUICheckBox;
    QCheckBox *m_featureTimelineEditorCheckBox;
    QCheckBox *m_featureDockWidgetContentMinSize;
    QGroupBox *m_debugGroupBox;
    QCheckBox *m_designerShowDebuggerCheckBox;
    QCheckBox *m_showPropertyEditorWarningsCheckBox;
    QComboBox *m_forwardPuppetOutputComboBox;
    QCheckBox *m_designerEnableDebuggerCheckBox;
    QCheckBox *m_showWarnExceptionsCheckBox;
    QComboBox *m_debugPuppetComboBox;
};

SettingsPageWidget::SettingsPageWidget()
{
    m_spinItemSpacing = new QSpinBox;
    m_spinItemSpacing->setMaximum(50);

    m_spinSnapMargin = new QSpinBox;
    m_spinSnapMargin->setMaximum(10);

    m_spinCanvasHeight = new QSpinBox;
    m_spinCanvasHeight->setMaximum(100000);
    m_spinCanvasHeight->setSingleStep(100);
    m_spinCanvasHeight->setValue(10000);

    m_spinCanvasWidth = new QSpinBox;
    m_spinCanvasWidth->setMaximum(100000);
    m_spinCanvasWidth->setSingleStep(100);
    m_spinCanvasWidth->setValue(10000);

    m_smoothRendering = new QCheckBox;
    m_smoothRendering->setToolTip(Tr::tr("Enable smooth rendering in the 2D view."));

    m_spinRootItemInitHeight = new QSpinBox;
    m_spinRootItemInitHeight->setMaximum(100000);
    m_spinRootItemInitHeight->setValue(480);

    m_spinRootItemInitWidth = new QSpinBox;
    m_spinRootItemInitWidth->setMaximum(100000);
    m_spinRootItemInitWidth->setValue(640);

    m_styleLineEdit = new QLineEdit;
    m_styleLineEdit->setPlaceholderText(Tr::tr("Default style"));

    auto resetStyle = new QPushButton(Tr::tr("Reset Style"));

    m_controls2StyleComboBox = new QComboBox;
    m_controls2StyleComboBox->addItems({ "Default", "Material", "Universal" });

    m_alwaysSaveSubcomponentsCheckBox = new QCheckBox(
        Tr::tr("Always save when leaving subcomponent in bread crumb"));

    m_designerWarningsInEditorCheckBox = new QCheckBox(
        Tr::tr("Warn about unsupported features of .ui.qml files in code editor"));
    m_designerWarningsInEditorCheckBox->setToolTip(
        Tr::tr("Also warns in the code editor about QML features that are not properly "
               "supported by the Qt Quick Designer."));

    m_designerWarningsCheckBox = new QCheckBox(
        Tr::tr("Warn about unsupported features in .ui.qml files"));
    m_designerWarningsCheckBox->setToolTip(Tr::tr(
        "Warns about QML features that are not properly supported by the Qt Design Studio."));

    m_designerWarningsUiQmlfiles = new QCheckBox(
        Tr::tr("Warn about using .qml files instead of .ui.qml files"));
    m_designerWarningsUiQmlfiles->setToolTip(Tr::tr(
        "Qt Quick Designer will propose to open .ui.qml files instead of opening a .qml file."));

    m_useQsTrFunctionRadioButton = new QRadioButton(Tr::tr("qsTr()"));
    m_useQsTrFunctionRadioButton->setChecked(true);
    m_useQsTrIdFunctionRadioButton = new QRadioButton(Tr::tr("qsTrId()"));
    m_useQsTranslateFunctionRadioButton = new QRadioButton(Tr::tr("qsTranslate()"));

    m_designerAlwaysDesignModeCheckBox = new QCheckBox(
        Tr::tr("Always open ui.qml files in Design mode"));
    m_askBeforeDeletingAssetCheckBox = new QCheckBox(
        Tr::tr("Ask for confirmation before deleting asset"));
    m_askBeforeDeletingContentLibFileCheckBox = new QCheckBox(
        Tr::tr("Ask for confirmation before deleting content library files"));
    m_alwaysAutoFormatUICheckBox = new QCheckBox(
        Tr::tr("Always auto-format ui.qml files in Design mode"));
    m_featureTimelineEditorCheckBox = new QCheckBox(Tr::tr("Enable Timeline editor"));
    m_featureDockWidgetContentMinSize = new QCheckBox(
        Tr::tr("Enable DockWidget content minimum size"));

    m_debugGroupBox = new QGroupBox(Tr::tr("Debugging"));
    m_designerShowDebuggerCheckBox = new QCheckBox(Tr::tr("Show the debugging view"));
    m_showPropertyEditorWarningsCheckBox = new QCheckBox(Tr::tr("Show property editor warnings"));

    m_forwardPuppetOutputComboBox = new QComboBox;

    m_designerEnableDebuggerCheckBox = new QCheckBox(Tr::tr("Enable the debugging view"));
    m_showWarnExceptionsCheckBox = new QCheckBox(Tr::tr("Show warn exceptions"));

    m_debugPuppetComboBox = new QComboBox;

    using namespace Layouting;

    Grid{m_designerShowDebuggerCheckBox,
         m_showPropertyEditorWarningsCheckBox,
         Form{Tr::tr("Forward QML Puppet output:"), m_forwardPuppetOutputComboBox},
         br,
         m_designerEnableDebuggerCheckBox,
         m_showWarnExceptionsCheckBox,
         Form{Tr::tr("Debug QML Puppet:"), m_debugPuppetComboBox}}
        .attachTo(m_debugGroupBox);

    Column{Row{Group{title(Tr::tr("Snapping")),
                     Form{Tr::tr("Parent component padding:"),
                          m_spinSnapMargin,
                          br,
                          Tr::tr("Sibling component spacing:"),
                          m_spinItemSpacing}},
               Group{title(Tr::tr("Canvas")),
                     Form{Tr::tr("Width:"),
                          m_spinCanvasWidth,
                          br,
                          Tr::tr("Height:"),
                          m_spinCanvasHeight,
                          br,
                          Tr::tr("Smooth rendering:"),
                          m_smoothRendering}},
               Group{title(Tr::tr("Root Component Init Size")),
                     Form{Tr::tr("Width:"),
                          m_spinRootItemInitWidth,
                          br,
                          Tr::tr("Height:"),
                          m_spinRootItemInitHeight}},
               Group{title(Tr::tr("Styling")),
                     Form{Tr::tr("Controls style:"),
                          m_styleLineEdit,
                          resetStyle,
                          br,
                          Tr::tr("Controls 2 style:"),
                          m_controls2StyleComboBox}}},
           Group{title(Tr::tr("Subcomponents")), Column{m_alwaysSaveSubcomponentsCheckBox}},
           Row{Group{title(Tr::tr("Warnings")),
                     Column{m_designerWarningsCheckBox,
                            m_designerWarningsInEditorCheckBox,
                            m_designerWarningsUiQmlfiles}},
               Group{title(Tr::tr("Internationalization")),
                     Column{m_useQsTrFunctionRadioButton,
                            m_useQsTrIdFunctionRadioButton,
                            m_useQsTranslateFunctionRadioButton}}},
           Group{title(Tr::tr("Features")),
                 Grid{m_designerAlwaysDesignModeCheckBox,
                      m_alwaysAutoFormatUICheckBox,
                      br,
                      m_askBeforeDeletingAssetCheckBox,
                      m_askBeforeDeletingContentLibFileCheckBox,
                      br,
                      m_featureTimelineEditorCheckBox,
                      br,
                      m_featureDockWidgetContentMinSize}},
           m_debugGroupBox,
           st}
        .attachTo(this);

    connect(m_designerEnableDebuggerCheckBox, &QCheckBox::toggled, [this](bool checked) {
        if (checked && ! m_designerShowDebuggerCheckBox->isChecked())
            m_designerShowDebuggerCheckBox->setChecked(true);
        }
    );
    connect(resetStyle, &QPushButton::clicked,
        m_styleLineEdit, &QLineEdit::clear);
    connect(m_controls2StyleComboBox, &QComboBox::currentTextChanged, [this] {
        m_styleLineEdit->setText(m_controls2StyleComboBox->currentText());
    });

    m_forwardPuppetOutputComboBox->addItems(puppetModes());
    m_debugPuppetComboBox->addItems(puppetModes());

    readSettings();

    Utils::installMarkSettingsDirtyTriggerRecursively(this);
}

QHash<QByteArray, QVariant> SettingsPageWidget::newSettings() const
{
    QHash<QByteArray, QVariant> settings;
    settings.insert(DesignerSettingsKey::ItemSpacing, m_spinItemSpacing->value());
    settings.insert(DesignerSettingsKey::ContainerPadding, m_spinSnapMargin->value());
    settings.insert(DesignerSettingsKey::CanvasWidth, m_spinCanvasWidth->value());
    settings.insert(DesignerSettingsKey::CanvasHeight, m_spinCanvasHeight->value());
    settings.insert(DesignerSettingsKey::RootElementInitWidth, m_spinRootItemInitWidth->value());
    settings.insert(DesignerSettingsKey::RootElementInitHeight, m_spinRootItemInitHeight->value());
    settings.insert(DesignerSettingsKey::WarnAboutQtQuickFeaturesInDesigner,
                    m_designerWarningsCheckBox->isChecked());
    settings.insert(DesignerSettingsKey::WarnAboutQmlFilesInsteadOfUiQmlFiles,
                    m_designerWarningsUiQmlfiles->isChecked());

    settings.insert(DesignerSettingsKey::WarnAboutQtQuickDesignerFeaturesInCodeEditor,
        m_designerWarningsInEditorCheckBox->isChecked());
    settings.insert(DesignerSettingsKey::ShowQtQuickDesignerDebugView,
        m_designerShowDebuggerCheckBox->isChecked());
    settings.insert(DesignerSettingsKey::EnableQtQuickDesignerDebugView,
        m_designerEnableDebuggerCheckBox->isChecked());

    int typeOfQsTrFunction;

    if (m_useQsTrFunctionRadioButton->isChecked())
        typeOfQsTrFunction = 0;
    else if (m_useQsTrIdFunctionRadioButton->isChecked())
        typeOfQsTrFunction = 1;
    else if (m_useQsTranslateFunctionRadioButton->isChecked())
        typeOfQsTrFunction = 2;
    else
        typeOfQsTrFunction = 0;

    settings.insert(DesignerSettingsKey::TypeOfQsTrFunction, typeOfQsTrFunction);
    settings.insert(DesignerSettingsKey::ControlsStyle, m_styleLineEdit->text());
    settings.insert(DesignerSettingsKey::ForwardPuppetOutput,
        m_forwardPuppetOutputComboBox->currentText());
    settings.insert(DesignerSettingsKey::DebugPuppet,
        m_debugPuppetComboBox->currentText());

    settings.insert(DesignerSettingsKey::AlwaysSaveInCrumbleBar,
        m_alwaysSaveSubcomponentsCheckBox->isChecked());
    settings.insert(DesignerSettingsKey::ShowPropertyEditorWarnings,
        m_showPropertyEditorWarningsCheckBox->isChecked());
    settings.insert(DesignerSettingsKey::WarnException,
        m_showWarnExceptionsCheckBox->isChecked());
    settings.insert(DesignerSettingsKey::EnableTimelineView,
                    m_featureTimelineEditorCheckBox->isChecked());
    settings.insert(DesignerSettingsKey::EnableDockWidgetContentMinSize,
                    m_featureDockWidgetContentMinSize->isChecked());
    settings.insert(DesignerSettingsKey::AlwaysDesignMode,
                    m_designerAlwaysDesignModeCheckBox->isChecked());
    settings.insert(DesignerSettingsKey::AskBeforeDeletingAsset,
                    m_askBeforeDeletingAssetCheckBox->isChecked());
    settings.insert(DesignerSettingsKey::AskBeforeDeletingContentLibFile,
                    m_askBeforeDeletingContentLibFileCheckBox->isChecked());
    settings.insert(DesignerSettingsKey::SmoothRendering, m_smoothRendering->isChecked());

    settings.insert(DesignerSettingsKey::ReformatUiQmlFiles,
                    m_alwaysAutoFormatUICheckBox->isChecked());

    return settings;
}

void SettingsPageWidget::readSettings()
{
    const DesignerSettings &settings = designerSettings();
    m_spinItemSpacing->setValue(settings.value(DesignerSettingsKey::ItemSpacing).toInt());
    m_spinSnapMargin->setValue(settings.value(
        DesignerSettingsKey::ContainerPadding).toInt());
    m_spinCanvasWidth->setValue(settings.value(
        DesignerSettingsKey::CanvasWidth).toInt());
    m_spinCanvasHeight->setValue(settings.value(
        DesignerSettingsKey::CanvasHeight).toInt());
    m_spinRootItemInitWidth->setValue(settings.value(
        DesignerSettingsKey::RootElementInitWidth).toInt());
    m_spinRootItemInitHeight->setValue(settings.value(
        DesignerSettingsKey::RootElementInitHeight).toInt());
    m_designerWarningsCheckBox->setChecked(settings.value(
        DesignerSettingsKey::WarnAboutQtQuickFeaturesInDesigner).toBool());
    m_designerWarningsUiQmlfiles->setChecked(settings.value(
        DesignerSettingsKey::WarnAboutQmlFilesInsteadOfUiQmlFiles).toBool());
    m_designerWarningsInEditorCheckBox->setChecked(settings.value(
        DesignerSettingsKey::WarnAboutQtQuickDesignerFeaturesInCodeEditor).toBool());
    m_designerShowDebuggerCheckBox->setChecked(settings.value(
        DesignerSettingsKey::ShowQtQuickDesignerDebugView).toBool());
    m_designerEnableDebuggerCheckBox->setChecked(settings.value(
        DesignerSettingsKey::EnableQtQuickDesignerDebugView).toBool());
    m_useQsTrFunctionRadioButton->setChecked(settings.value(
        DesignerSettingsKey::TypeOfQsTrFunction).toInt() == 0);
    m_useQsTrIdFunctionRadioButton->setChecked(settings.value(
        DesignerSettingsKey::TypeOfQsTrFunction).toInt() == 1);
    m_useQsTranslateFunctionRadioButton->setChecked(settings.value(
        DesignerSettingsKey::TypeOfQsTrFunction).toInt() == 2);
    m_styleLineEdit->setText(settings.value(
        DesignerSettingsKey::ControlsStyle).toString());

    m_forwardPuppetOutputComboBox->setCurrentText(settings.value(
        DesignerSettingsKey::ForwardPuppetOutput).toString());
    m_debugPuppetComboBox->setCurrentText(settings.value(
        DesignerSettingsKey::DebugPuppet).toString());

    m_alwaysSaveSubcomponentsCheckBox->setChecked(settings.value(
        DesignerSettingsKey::AlwaysSaveInCrumbleBar).toBool());
    m_showPropertyEditorWarningsCheckBox->setChecked(settings.value(
        DesignerSettingsKey::ShowPropertyEditorWarnings).toBool());
    m_showWarnExceptionsCheckBox->setChecked(settings.value(
        DesignerSettingsKey::WarnException).toBool());

    m_controls2StyleComboBox->setCurrentText(m_styleLineEdit->text());

    m_designerAlwaysDesignModeCheckBox->setChecked(settings.value(
        DesignerSettingsKey::AlwaysDesignMode).toBool());
    m_featureTimelineEditorCheckBox->setChecked(settings.value(
        DesignerSettingsKey::EnableTimelineView).toBool());
    m_featureDockWidgetContentMinSize->setChecked(
        settings.value(DesignerSettingsKey::EnableDockWidgetContentMinSize).toBool());

    m_askBeforeDeletingAssetCheckBox->setChecked(
        settings.value(DesignerSettingsKey::AskBeforeDeletingAsset).toBool());
    m_askBeforeDeletingContentLibFileCheckBox->setChecked(
        settings.value(DesignerSettingsKey::AskBeforeDeletingContentLibFile).toBool());

    m_debugGroupBox->setVisible(true);
    m_featureTimelineEditorCheckBox->setVisible(false);
    m_featureDockWidgetContentMinSize->setVisible(false);
    m_smoothRendering->setChecked(settings.value(DesignerSettingsKey::SmoothRendering).toBool());

    m_alwaysAutoFormatUICheckBox->setChecked(
        settings.value(DesignerSettingsKey::ReformatUiQmlFiles).toBool());
}

void SettingsPageWidget::apply()
{
    auto settings = newSettings();

    const auto restartNecessaryKeys = {DesignerSettingsKey::WarnException,
                                       DesignerSettingsKey::ForwardPuppetOutput,
                                       DesignerSettingsKey::DebugPuppet,
                                       DesignerSettingsKey::WarnException,
                                       DesignerSettingsKey::EnableTimelineView,
                                       DesignerSettingsKey::EnableDockWidgetContentMinSize};

    for (const char * const key : restartNecessaryKeys) {
        if (designerSettings().value(key) != settings.value(key)) {
            QMessageBox::information(Core::ICore::dialogParent(),
                                     Tr::tr("Restart Required"),
                                     Tr::tr("The made changes will take effect after a "
                                            "restart of the QML Puppet or %1.")
                                         .arg(QGuiApplication::applicationDisplayName()));
            break;
        }
    }

    designerSettings().insert(settings);
}

SettingsPage::SettingsPage()
{
    setId("B.QmlDesigner");
    setDisplayName(Tr::tr("Qt Quick Designer"));
    setCategory(QmlJSEditor::Constants::SETTINGS_CATEGORY_QML);
    setWidgetCreator([&] { return new SettingsPageWidget(); });
}

} // Internal
} // QmlDesigner
