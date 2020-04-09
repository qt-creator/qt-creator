/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "systemsettings.h"
#include "coreconstants.h"
#include "editormanager/editormanager_p.h"
#include "fileutils.h"
#include "icore.h"
#include "iversioncontrol.h"
#include "patchtool.h"
#include "vcsmanager.h"

#include <app/app_version.h>
#include <utils/checkablemessagebox.h>
#include <utils/consoleprocess.h>
#include <utils/environment.h>
#include <utils/hostosinfo.h>
#include <utils/unixutils.h>

#include <QCoreApplication>
#include <QMessageBox>
#include <QSettings>

#include "ui_systemsettings.h"

using namespace Utils;

namespace Core {
namespace Internal {

class SystemSettingsWidget : public IOptionsPageWidget
{
    Q_DECLARE_TR_FUNCTIONS(Core::Internal::SystemSettingsWidget)

public:
    SystemSettingsWidget()
    {
        m_ui.setupUi(this);
        m_ui.terminalOpenArgs->setToolTip(
            tr("Command line arguments used for \"%1\".").arg(FileUtils::msgTerminalHereAction()));

        m_ui.reloadBehavior->setCurrentIndex(EditorManager::reloadSetting());
        if (HostOsInfo::isAnyUnixHost()) {
            const QVector<TerminalCommand> availableTerminals = ConsoleProcess::availableTerminalEmulators();
            for (const TerminalCommand &term : availableTerminals)
                m_ui.terminalComboBox->addItem(term.command, QVariant::fromValue(term));
            updateTerminalUi(ConsoleProcess::terminalEmulator(ICore::settings()));
            connect(m_ui.terminalComboBox,
                    QOverload<int>::of(&QComboBox::currentIndexChanged),
                    this,
                    [this](int index) {
                        updateTerminalUi(
                            m_ui.terminalComboBox->itemData(index).value<TerminalCommand>());
                    });
        } else {
            m_ui.terminalLabel->hide();
            m_ui.terminalComboBox->hide();
            m_ui.terminalOpenArgs->hide();
            m_ui.terminalExecuteArgs->hide();
            m_ui.resetTerminalButton->hide();
        }

        if (HostOsInfo::isAnyUnixHost() && !HostOsInfo::isMacHost()) {
            m_ui.externalFileBrowserEdit->setText(UnixUtils::fileBrowser(ICore::settings()));
        } else {
            m_ui.externalFileBrowserLabel->hide();
            m_ui.externalFileBrowserEdit->hide();
            m_ui.resetFileBrowserButton->hide();
            m_ui.helpExternalFileBrowserButton->hide();
        }

        const QString patchToolTip = tr("Command used for reverting diff chunks.");
        m_ui.patchCommandLabel->setToolTip(patchToolTip);
        m_ui.patchChooser->setToolTip(patchToolTip);
        m_ui.patchChooser->setExpectedKind(PathChooser::ExistingCommand);
        m_ui.patchChooser->setHistoryCompleter(QLatin1String("General.PatchCommand.History"));
        m_ui.patchChooser->setPath(PatchTool::patchCommand());
        m_ui.autoSaveCheckBox->setChecked(EditorManagerPrivate::autoSaveEnabled());
        m_ui.autoSaveCheckBox->setToolTip(tr("Automatically creates temporary copies of "
                                                "modified files. If %1 is restarted after "
                                                "a crash or power failure, it asks whether to "
                                                "recover the auto-saved content.")
                                             .arg(Constants::IDE_DISPLAY_NAME));
        m_ui.autoSaveInterval->setValue(EditorManagerPrivate::autoSaveInterval());
        m_ui.autoSuspendCheckBox->setChecked(EditorManagerPrivate::autoSuspendEnabled());
        m_ui.autoSuspendMinDocumentCount->setValue(EditorManagerPrivate::autoSuspendMinDocumentCount());
        m_ui.warnBeforeOpeningBigFiles->setChecked(
                    EditorManagerPrivate::warnBeforeOpeningBigFilesEnabled());
        m_ui.bigFilesLimitSpinBox->setValue(EditorManagerPrivate::bigFileSizeLimit());
        m_ui.maxRecentFilesSpinBox->setMinimum(1);
        m_ui.maxRecentFilesSpinBox->setMaximum(99);
        m_ui.maxRecentFilesSpinBox->setValue(EditorManagerPrivate::maxRecentFiles());

        if (HostOsInfo::isAnyUnixHost()) {
            connect(m_ui.resetTerminalButton, &QAbstractButton::clicked,
                    this, &SystemSettingsWidget::resetTerminal);
            if (!HostOsInfo::isMacHost()) {
                connect(m_ui.resetFileBrowserButton, &QAbstractButton::clicked,
                        this, &SystemSettingsWidget::resetFileBrowser);
                connect(m_ui.helpExternalFileBrowserButton, &QAbstractButton::clicked,
                        this, &SystemSettingsWidget::showHelpForFileBrowser);
            }
        }

        if (HostOsInfo::isMacHost()) {
            Qt::CaseSensitivity defaultSensitivity
                    = OsSpecificAspects::fileNameCaseSensitivity(HostOsInfo::hostOs());
            if (defaultSensitivity == Qt::CaseSensitive) {
                m_ui.fileSystemCaseSensitivityChooser->addItem(tr("Case Sensitive (Default)"),
                                                                  Qt::CaseSensitive);
            } else {
                m_ui.fileSystemCaseSensitivityChooser->addItem(tr("Case Sensitive"),
                                                                  Qt::CaseSensitive);
            }
            if (defaultSensitivity == Qt::CaseInsensitive) {
                m_ui.fileSystemCaseSensitivityChooser->addItem(tr("Case Insensitive (Default)"),
                                                                  Qt::CaseInsensitive);
            } else {
                m_ui.fileSystemCaseSensitivityChooser->addItem(tr("Case Insensitive"),
                                                                  Qt::CaseInsensitive);
            }
            if (HostOsInfo::fileNameCaseSensitivity() == Qt::CaseSensitive)
                m_ui.fileSystemCaseSensitivityChooser->setCurrentIndex(0);
            else
                m_ui.fileSystemCaseSensitivityChooser->setCurrentIndex(1);
        } else {
            m_ui.fileSystemCaseSensitivityWidget->hide();
        }

        updatePath();

        connect(VcsManager::instance(), &VcsManager::configurationChanged,
                this, &SystemSettingsWidget::updatePath);
    }

private:
    void apply() final;

    void showHelpForFileBrowser();
    void resetFileBrowser();
    void resetTerminal();
    void updateTerminalUi(const Utils::TerminalCommand &term);
    void updatePath();

    void variableHelpDialogCreator(const QString &helpText);
    Ui::SystemSettings m_ui;
    QPointer<QMessageBox> m_dialog;
};

void SystemSettingsWidget::apply()
{
    EditorManager::setReloadSetting(IDocument::ReloadSetting(m_ui.reloadBehavior->currentIndex()));
    if (HostOsInfo::isAnyUnixHost()) {
        ConsoleProcess::setTerminalEmulator(ICore::settings(),
                                            {m_ui.terminalComboBox->lineEdit()->text(),
                                             m_ui.terminalOpenArgs->text(),
                                             m_ui.terminalExecuteArgs->text()});
        if (!HostOsInfo::isMacHost()) {
            UnixUtils::setFileBrowser(ICore::settings(),
                                      m_ui.externalFileBrowserEdit->text());
        }
    }
    PatchTool::setPatchCommand(m_ui.patchChooser->filePath().toString());
    EditorManagerPrivate::setAutoSaveEnabled(m_ui.autoSaveCheckBox->isChecked());
    EditorManagerPrivate::setAutoSaveInterval(m_ui.autoSaveInterval->value());
    EditorManagerPrivate::setAutoSuspendEnabled(m_ui.autoSuspendCheckBox->isChecked());
    EditorManagerPrivate::setAutoSuspendMinDocumentCount(m_ui.autoSuspendMinDocumentCount->value());
    EditorManagerPrivate::setWarnBeforeOpeningBigFilesEnabled(
                m_ui.warnBeforeOpeningBigFiles->isChecked());
    EditorManagerPrivate::setBigFileSizeLimit(m_ui.bigFilesLimitSpinBox->value());
    EditorManagerPrivate::setMaxRecentFiles(m_ui.maxRecentFilesSpinBox->value());

    if (HostOsInfo::isMacHost()) {
        Qt::CaseSensitivity defaultSensitivity
                = OsSpecificAspects::fileNameCaseSensitivity(HostOsInfo::hostOs());
        Qt::CaseSensitivity selectedSensitivity = Qt::CaseSensitivity(
                m_ui.fileSystemCaseSensitivityChooser->currentData().toInt());
        if (defaultSensitivity == selectedSensitivity)
            HostOsInfo::unsetOverrideFileNameCaseSensitivity();
        else
            HostOsInfo::setOverrideFileNameCaseSensitivity(selectedSensitivity);
    }
}

void SystemSettingsWidget::resetTerminal()
{
    if (HostOsInfo::isAnyUnixHost())
        m_ui.terminalComboBox->setCurrentIndex(0);
}

void SystemSettingsWidget::updateTerminalUi(const TerminalCommand &term)
{
    m_ui.terminalComboBox->lineEdit()->setText(term.command);
    m_ui.terminalOpenArgs->setText(term.openArgs);
    m_ui.terminalExecuteArgs->setText(term.executeArgs);
}

void SystemSettingsWidget::resetFileBrowser()
{
    if (HostOsInfo::isAnyUnixHost() && !HostOsInfo::isMacHost())
        m_ui.externalFileBrowserEdit->setText(UnixUtils::defaultFileBrowser());
}

void SystemSettingsWidget::updatePath()
{
    Environment env = Environment::systemEnvironment();
    QStringList toAdd = VcsManager::additionalToolsPath();
    env.appendOrSetPath(toAdd.join(HostOsInfo::pathListSeparator()));
    m_ui.patchChooser->setEnvironment(env);
}

void SystemSettingsWidget::variableHelpDialogCreator(const QString &helpText)
{
    if (m_dialog) {
        if (m_dialog->text() != helpText)
            m_dialog->setText(helpText);

        m_dialog->show();
        ICore::raiseWindow(m_dialog);
        return;
    }
    QMessageBox *mb = new QMessageBox(QMessageBox::Information,
                                  tr("Variables"),
                                  helpText,
                                  QMessageBox::Close,
                                  this);
    mb->setWindowModality(Qt::NonModal);
    m_dialog = mb;
    mb->show();
}

void SystemSettingsWidget::showHelpForFileBrowser()
{
    if (HostOsInfo::isAnyUnixHost() && !HostOsInfo::isMacHost())
        variableHelpDialogCreator(UnixUtils::fileBrowserHelpText());
}

SystemSettings::SystemSettings()
{
    setId(Constants::SETTINGS_ID_SYSTEM);
    setDisplayName(SystemSettingsWidget::tr("System"));
    setCategory(Constants::SETTINGS_CATEGORY_CORE);
    setWidgetCreator([] { return new SystemSettingsWidget; });
}

} // namespace Internal
} // namespace Core
