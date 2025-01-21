// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "cocoprojectwidget.h"

#include "buildsettings.h"
#include "cococommon.h"
#include "cocopluginconstants.h"
#include "cocotr.h"
#include "globalsettings.h"

#include <coreplugin/icore.h>
#include <projectexplorer/buildsystem.h>

#include <QFileDialog>
#include <QMessageBox>

using namespace ProjectExplorer;
using namespace Core;

namespace Coco::Internal {

CocoProjectWidget::CocoProjectWidget(Project *project, BuildConfiguration *buildConfig)
    : m_project{project}
    , m_buildConfigurationName{buildConfig->displayName()}
{
    using namespace Layouting;
    using namespace Utils;

    m_configerrorLabel.setVisible(false);
    m_configerrorLabel.setIconType(InfoLabel::Error);
    Label docLink(
        QString(
            "<a href=\"https://doc.qt.io/coco/coveragescanner-command-line-arguments.html\">%1</a>")
            .arg(Tr::tr("Documentation")));
    docLink.setOpenExternalLinks(true);
    m_optionEdit.setDisplayStyle(StringAspect::TextEditDisplay);
    m_tweaksEdit.setDisplayStyle(StringAspect::TextEditDisplay);
    m_revertButton.setText(Tr::tr("Revert"));
    QFont bold;
    bold.setBold(true);
    m_saveButton.setFont(bold);

    m_coverageGroupbox
        = {groupChecker(m_coverageGroupBoxEnabled.groupChecker()),
           Column{
               Row{Tr::tr("CoverageScanner Options"), st, docLink},
               m_optionEdit,
               Row{PushButton{
                       text(Tr::tr("Exclude File...")),
                       onClicked(this, [this] { onExcludeFileButtonClicked(); })},
                   PushButton{
                       text(Tr::tr("Exclude Directory...")),
                       onClicked(this, [this] { onExcludeDirButtonClicked(); })},
                   m_tweaksButton,
                   st},
               m_tweaksDescriptionLabel,
               m_tweaksEdit,
               Row{Tr::tr("These settings are stored in"), m_fileNameLabel, st},
               Group{title(Tr::tr("Changed Build Settings")), Column{m_changesText}}}};

    Column{
        m_configerrorLabel,
        m_coverageGroupbox,
        Row{m_messageLabel, st, &m_revertButton, &m_saveButton}}
        .attachTo(this);

    m_buildSettings = BuildSettings::createdFor(buildConfig);
    m_buildSettings->connectToProject(this);

    readSelectionDir();
    reloadSettings();

    m_fileNameLabel.setValue(m_buildSettings->featureFilePath());
    m_tweaksDescriptionLabel.setText(
        Tr::tr("Code for the end of the file \"%1\" to override the built-in declarations."
               " Only needed in special cases.")
            .arg(m_buildSettings->featureFilenName()));
    setTweaksVisible(m_buildSettings->hasTweaks());
    clearMessageLabel();

    connect(&m_coverageGroupBoxEnabled, &BoolAspect::changed, this, &CocoProjectWidget::onCoverageGroupBoxClicked);

    connect(&m_optionEdit, &StringAspect::changed, this, &CocoProjectWidget::onTextChanged);
    connect(&m_tweaksEdit, &StringAspect::changed, this, &CocoProjectWidget::onTextChanged);
    m_tweaksButton.onClicked(this, [this] { onTweaksButtonClicked(); });

    connect(&m_revertButton, &QPushButton::clicked, this, &CocoProjectWidget::onRevertButtonClicked);
    connect(&m_saveButton, &QPushButton::clicked, this, &CocoProjectWidget::onSaveButtonClicked);

    connect(&cocoSettings(), &CocoSettings::updateCocoDir, this, &CocoProjectWidget::reloadSettings);
}

// Read the build settings again and show them in the widget.
void CocoProjectWidget::reloadSettings()
{
    m_buildSettings->read();
    m_coverageGroupBoxEnabled.setValue(m_buildSettings->validSettings(), Utils::BaseAspect::BeQuiet);
    m_coverageGroupbox.setTitle(
        Tr::tr("Enable code coverage for build configuration \"%1\"").arg(m_buildConfigurationName));

    m_optionEdit.setValue(m_buildSettings->options().join('\n'), Utils::BaseAspect::BeQuiet);
    m_tweaksEdit.setValue(m_buildSettings->tweaks().join('\n'), Utils::BaseAspect::BeQuiet);

    setState(configDone);
    displayChanges();

    const bool valid = cocoSettings().isValid();
    m_configerrorLabel.setVisible(!valid);
    if (!valid) {
        m_configerrorLabel.setText(
            Tr::tr("Coco is not installed correctly: \"%1\"").arg(cocoSettings().errorMessage()));
    }
}

void CocoProjectWidget::showEvent(QShowEvent *event)
{
    Q_UNUSED(event)
    reloadSettings();
}

void CocoProjectWidget::buildSystemUpdated(ProjectExplorer::BuildSystem *bs)
{
    QString newBuildConfigurationName = bs->buildConfiguration()->displayName();

    if (m_buildConfigurationName != newBuildConfigurationName) {
        m_buildConfigurationName = newBuildConfigurationName;
        logSilently(Tr::tr("Build Configuration changed to %1.").arg(newBuildConfigurationName));
        reloadSettings();
    } else if (m_configState == configRunning)
        setState(configDone);
}

void CocoProjectWidget::configurationErrorOccurred(const QString &error)
{
    Q_UNUSED(error)

    if (m_configState == configEdited) {
        setMessageLabel(Utils::InfoLabel::Information, Tr::tr("Re-configuring stopped by user."));
        setState(configStopped);
    } else {
        // The variable error seems to contain no usable information.
        setMessageLabel(
            Utils::InfoLabel::Error,
            Tr::tr("Error when configuring with <i>%1</i>. "
                   "Check General Messages for more information.")
                .arg(m_buildSettings->featureFilenName()));
        setState(configDone);
    }
}

void CocoProjectWidget::setState(ConfigurationState state)
{
    m_configState = state;

    switch (m_configState) {
    case configDone:
        m_saveButton.setText(m_buildSettings->saveButtonText());
        m_saveButton.setEnabled(false);
        m_revertButton.setEnabled(false);
        break;
    case configEdited:
        m_saveButton.setText(m_buildSettings->saveButtonText());
        m_saveButton.setEnabled(true);
        m_revertButton.setEnabled(true);
        break;
    case configRunning:
        m_saveButton.setText(Tr::tr("Stop Re-configuring"));
        m_saveButton.setEnabled(true);
        m_revertButton.setEnabled(false);
        break;
    case configStopped:
        m_saveButton.setText(Tr::tr("Re-configure"));
        m_saveButton.setEnabled(true);
        m_revertButton.setEnabled(false);
        break;
    }
}

void CocoProjectWidget::readSelectionDir()
{
    QVariantMap settings = m_project->namedSettings(Constants::SETTINGS_NAME_KEY).toMap();

    if (settings.contains(Constants::SELECTION_DIR_KEY))
        m_selectionDirectory = settings[Constants::SELECTION_DIR_KEY].toString();
    else
        m_selectionDirectory = m_buildSettings->projectDirectory();
}

void CocoProjectWidget::writeSelectionDir(const QString &path)
{
    m_selectionDirectory = path;

    QVariantMap settings;
    settings[Constants::SELECTION_DIR_KEY] = path;

    m_project->setNamedSettings(Constants::SETTINGS_NAME_KEY, settings);
}

void CocoProjectWidget::setTweaksVisible(bool on)
{
    if (on)
        m_tweaksButton.setText(Tr::tr("Override <<"));
    else
        m_tweaksButton.setText(Tr::tr("Override >>"));

    m_tweaksDescriptionLabel.setVisible(on);
    m_tweaksEdit.setVisible(on);
}

void CocoProjectWidget::setMessageLabel(const Utils::InfoLabel::InfoType type, const QString &text)
{
    m_messageLabel.setText(text);
    m_messageLabel.setIconType(type);
}

void CocoProjectWidget::clearMessageLabel()
{
    m_messageLabel.setText("");
    m_messageLabel.setIconType(Utils::InfoLabel::None);
}

void Internal::CocoProjectWidget::onCoverageGroupBoxClicked()
{
    bool checked = m_coverageGroupBoxEnabled();

    displayChanges();

    if (!checked) {
        m_buildSettings->setCoverage(false);
        setState(configEdited);
        return;
    }

    if (!cocoSettings().isValid()) {
        m_coverageGroupBoxEnabled.setValue(false, Utils::BaseAspect::BeQuiet);

        QMessageBox box;
        box.setIcon(QMessageBox::Critical);
        box.setText(Tr::tr("The Coco installation path is not set correctly."));
        box.addButton(QMessageBox::Cancel);
        QPushButton *editButton = box.addButton(Tr::tr("Edit"), QMessageBox::AcceptRole);
        box.exec();

        if (box.clickedButton() == editButton)
            Core::ICore::showOptionsDialog(Constants::COCO_SETTINGS_PAGE_ID);

        m_coverageGroupBoxEnabled.setValue(cocoSettings().isValid(), Utils::BaseAspect::BeQuiet);
    } else
        m_buildSettings->setCoverage(checked);

    setState(configEdited);
}

void CocoProjectWidget::onSaveButtonClicked()
{
    if (m_configState == configRunning) {
        logSilently(Tr::tr("Stop re-configuring"));
        m_buildSettings->stopReconfigure();
        setState(configEdited);
        return;
    }

    QString options = m_optionEdit();
    QString tweaks = m_tweaksEdit();
    clearMessageLabel();

    logSilently(Tr::tr("Write file \"%1\"").arg(m_buildSettings->featureFilePath()));
    m_buildSettings->write(options, tweaks);

    if (m_buildSettings->needsReconfigure()) {
        logSilently(Tr::tr("Re-configure"));
        setState(configRunning);
        m_buildSettings->reconfigure();
    } else
        setState(configDone);
}

void CocoProjectWidget::onRevertButtonClicked()
{
    clearMessageLabel();
    logSilently(Tr::tr("Reload file \"%1\"").arg(m_buildSettings->featureFilePath()));
    reloadSettings();
}

void CocoProjectWidget::onTextChanged()
{
    setState(configEdited);
}

void CocoProjectWidget::displayChanges()
{
    m_changesText.setValue(m_buildSettings->configChanges());
}

void CocoProjectWidget::addCocoOption(QString option)
{
    m_optionEdit.setValue(m_optionEdit() + "\n" + option);
}

void CocoProjectWidget::onExcludeFileButtonClicked()
{
    QString fileName = QFileDialog::getOpenFileName(
        this, Tr::tr("File to Exclude from Instrumentation"), m_selectionDirectory);
    if (fileName.isEmpty())
        return;

    const auto fileNameInfo = Utils::FilePath::fromString(fileName);
    addCocoOption("--cs-exclude-file-abs-wildcard=" + maybeQuote("*/" + fileNameInfo.fileName()));

    writeSelectionDir(fileNameInfo.path());
}

void CocoProjectWidget::onExcludeDirButtonClicked()
{
    QString path = QFileDialog::getExistingDirectory(
        this, Tr::tr("Directory to Exclude from Instrumentation"), m_selectionDirectory);
    if (path.isEmpty())
        return;

    const QString projectDir = m_buildSettings->projectDirectory();
    if (path.startsWith(projectDir))
        // Make it a relative path with "*/" at the beginnig.
        path = "*/" + path.arg(path.mid(projectDir.size()));

    addCocoOption("--cs-exclude-file-abs-wildcard=" + maybeQuote(path));

    writeSelectionDir(path);
}

void Internal::CocoProjectWidget::onTweaksButtonClicked()
{
    setTweaksVisible(!m_tweaksEdit.isVisible());
}

} // namespace Coco::Internal
