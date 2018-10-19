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

SystemSettings::SystemSettings()
    : m_page(nullptr), m_dialog(nullptr)
{
    setId(Constants::SETTINGS_ID_SYSTEM);
    setDisplayName(tr("System"));
    setCategory(Constants::SETTINGS_CATEGORY_CORE);

    connect(VcsManager::instance(), &VcsManager::configurationChanged,
            this, &SystemSettings::updatePath);
}

QWidget *SystemSettings::widget()
{
    if (!m_widget) {
        m_page = new Ui::SystemSettings();
        m_widget = new QWidget;
        m_page->setupUi(m_widget);
        m_page->terminalOpenArgs->setToolTip(
            tr("Command line arguments used for \"%1\".").arg(FileUtils::msgTerminalAction()));

        m_page->reloadBehavior->setCurrentIndex(EditorManager::reloadSetting());
        if (HostOsInfo::isAnyUnixHost()) {
            const QVector<TerminalCommand> availableTerminals = ConsoleProcess::availableTerminalEmulators();
            for (const TerminalCommand &term : availableTerminals)
                m_page->terminalComboBox->addItem(term.command, qVariantFromValue(term));
            updateTerminalUi(ConsoleProcess::terminalEmulator(ICore::settings()));
            connect(m_page->terminalComboBox,
                    QOverload<int>::of(&QComboBox::currentIndexChanged),
                    this,
                    [this](int index) {
                        updateTerminalUi(
                            m_page->terminalComboBox->itemData(index).value<TerminalCommand>());
                    });
        } else {
            m_page->terminalLabel->hide();
            m_page->terminalComboBox->hide();
            m_page->terminalOpenArgs->hide();
            m_page->terminalExecuteArgs->hide();
            m_page->resetTerminalButton->hide();
        }

        if (HostOsInfo::isAnyUnixHost() && !HostOsInfo::isMacHost()) {
            m_page->externalFileBrowserEdit->setText(UnixUtils::fileBrowser(ICore::settings()));
        } else {
            m_page->externalFileBrowserLabel->hide();
            m_page->externalFileBrowserEdit->hide();
            m_page->resetFileBrowserButton->hide();
            m_page->helpExternalFileBrowserButton->hide();
        }

        const QString patchToolTip = tr("Command used for reverting diff chunks.");
        m_page->patchCommandLabel->setToolTip(patchToolTip);
        m_page->patchChooser->setToolTip(patchToolTip);
        m_page->patchChooser->setExpectedKind(PathChooser::ExistingCommand);
        m_page->patchChooser->setHistoryCompleter(QLatin1String("General.PatchCommand.History"));
        m_page->patchChooser->setPath(PatchTool::patchCommand());
        m_page->autoSaveCheckBox->setChecked(EditorManagerPrivate::autoSaveEnabled());
        m_page->autoSaveCheckBox->setToolTip(tr("Automatically creates temporary copies of "
                                                "modified files. If %1 is restarted after "
                                                "a crash or power failure, it asks whether to "
                                                "recover the auto-saved content.")
                                             .arg(Constants::IDE_DISPLAY_NAME));
        m_page->autoSaveInterval->setValue(EditorManagerPrivate::autoSaveInterval());
        m_page->autoSuspendCheckBox->setChecked(EditorManagerPrivate::autoSuspendEnabled());
        m_page->autoSuspendMinDocumentCount->setValue(EditorManagerPrivate::autoSuspendMinDocumentCount());
        m_page->warnBeforeOpeningBigFiles->setChecked(
                    EditorManagerPrivate::warnBeforeOpeningBigFilesEnabled());
        m_page->bigFilesLimitSpinBox->setValue(EditorManagerPrivate::bigFileSizeLimit());

        if (HostOsInfo::isAnyUnixHost()) {
            connect(m_page->resetTerminalButton, &QAbstractButton::clicked,
                    this, &SystemSettings::resetTerminal);
            if (!HostOsInfo::isMacHost()) {
                connect(m_page->resetFileBrowserButton, &QAbstractButton::clicked,
                        this, &SystemSettings::resetFileBrowser);
                connect(m_page->helpExternalFileBrowserButton, &QAbstractButton::clicked,
                        this, &SystemSettings::showHelpForFileBrowser);
            }
        }

        if (HostOsInfo::isMacHost()) {
            Qt::CaseSensitivity defaultSensitivity
                    = OsSpecificAspects::fileNameCaseSensitivity(HostOsInfo::hostOs());
            if (defaultSensitivity == Qt::CaseSensitive) {
                m_page->fileSystemCaseSensitivityChooser->addItem(tr("Case Sensitive (Default)"),
                                                                  Qt::CaseSensitive);
            } else {
                m_page->fileSystemCaseSensitivityChooser->addItem(tr("Case Sensitive"),
                                                                  Qt::CaseSensitive);
            }
            if (defaultSensitivity == Qt::CaseInsensitive) {
                m_page->fileSystemCaseSensitivityChooser->addItem(tr("Case Insensitive (Default)"),
                                                                  Qt::CaseInsensitive);
            } else {
                m_page->fileSystemCaseSensitivityChooser->addItem(tr("Case Insensitive"),
                                                                  Qt::CaseInsensitive);
            }
            if (HostOsInfo::fileNameCaseSensitivity() == Qt::CaseSensitive)
                m_page->fileSystemCaseSensitivityChooser->setCurrentIndex(0);
            else
                m_page->fileSystemCaseSensitivityChooser->setCurrentIndex(1);
        } else {
            m_page->fileSystemCaseSensitivityWidget->hide();
        }

        updatePath();
    }
    return m_widget;
}

void SystemSettings::apply()
{
    if (!m_page) // wasn't shown, can't be changed
        return;
    EditorManager::setReloadSetting(IDocument::ReloadSetting(m_page->reloadBehavior->currentIndex()));
    if (HostOsInfo::isAnyUnixHost()) {
        ConsoleProcess::setTerminalEmulator(ICore::settings(),
                                            {m_page->terminalComboBox->lineEdit()->text(),
                                             m_page->terminalOpenArgs->text(),
                                             m_page->terminalExecuteArgs->text()});
        if (!HostOsInfo::isMacHost()) {
            UnixUtils::setFileBrowser(ICore::settings(),
                                      m_page->externalFileBrowserEdit->text());
        }
    }
    PatchTool::setPatchCommand(m_page->patchChooser->path());
    EditorManagerPrivate::setAutoSaveEnabled(m_page->autoSaveCheckBox->isChecked());
    EditorManagerPrivate::setAutoSaveInterval(m_page->autoSaveInterval->value());
    EditorManagerPrivate::setAutoSuspendEnabled(m_page->autoSuspendCheckBox->isChecked());
    EditorManagerPrivate::setAutoSuspendMinDocumentCount(m_page->autoSuspendMinDocumentCount->value());
    EditorManagerPrivate::setWarnBeforeOpeningBigFilesEnabled(
                m_page->warnBeforeOpeningBigFiles->isChecked());
    EditorManagerPrivate::setBigFileSizeLimit(m_page->bigFilesLimitSpinBox->value());

    if (HostOsInfo::isMacHost()) {
        Qt::CaseSensitivity defaultSensitivity
                = OsSpecificAspects::fileNameCaseSensitivity(HostOsInfo::hostOs());
        Qt::CaseSensitivity selectedSensitivity = Qt::CaseSensitivity(
                m_page->fileSystemCaseSensitivityChooser->currentData().toInt());
        if (defaultSensitivity == selectedSensitivity)
            HostOsInfo::unsetOverrideFileNameCaseSensitivity();
        else
            HostOsInfo::setOverrideFileNameCaseSensitivity(selectedSensitivity);
    }
}

void SystemSettings::finish()
{
    delete m_widget;
    delete m_page;
    m_page = nullptr;
}

void SystemSettings::resetTerminal()
{
    if (HostOsInfo::isAnyUnixHost())
        m_page->terminalComboBox->setCurrentIndex(0);
}

void SystemSettings::updateTerminalUi(const TerminalCommand &term)
{
    m_page->terminalComboBox->lineEdit()->setText(term.command);
    m_page->terminalOpenArgs->setText(term.openArgs);
    m_page->terminalExecuteArgs->setText(term.executeArgs);
}

void SystemSettings::resetFileBrowser()
{
    if (HostOsInfo::isAnyUnixHost() && !HostOsInfo::isMacHost())
        m_page->externalFileBrowserEdit->setText(UnixUtils::defaultFileBrowser());
}

void SystemSettings::updatePath()
{
    if (!m_page)
       return;

    Environment env = Environment::systemEnvironment();
    QStringList toAdd = VcsManager::additionalToolsPath();
    env.appendOrSetPath(toAdd.join(HostOsInfo::pathListSeparator()));
    m_page->patchChooser->setEnvironment(env);
}

void SystemSettings::variableHelpDialogCreator(const QString &helpText)
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
                                  m_widget);
    mb->setWindowModality(Qt::NonModal);
    m_dialog = mb;
    mb->show();
}


void SystemSettings::showHelpForFileBrowser()
{
    if (HostOsInfo::isAnyUnixHost() && !HostOsInfo::isMacHost())
        variableHelpDialogCreator(UnixUtils::fileBrowserHelpText());
}

} // namespace Internal
} // namespace Core
