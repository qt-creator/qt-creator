/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "generalsettings.h"
#include "coreconstants.h"
#include "icore.h"
#include "infobar.h"
#include "patchtool.h"
#include "vcsmanager.h"
#include "editormanager/editormanager_p.h"

#include <utils/checkablemessagebox.h>
#include <utils/consoleprocess.h>
#include <utils/environment.h>
#include <utils/hostosinfo.h>
#include <utils/stylehelper.h>
#include <utils/unixutils.h>

#include <QCoreApplication>
#include <QDir>
#include <QLibraryInfo>
#include <QMessageBox>
#include <QSettings>
#include <QTextStream>

#include "ui_generalsettings.h"

using namespace Utils;

namespace Core {
namespace Internal {

GeneralSettings::GeneralSettings()
    : m_page(0), m_dialog(0)
{
    setId(Constants::SETTINGS_ID_ENVIRONMENT);
    setDisplayName(tr("General"));
    setCategory(Constants::SETTINGS_CATEGORY_CORE);
    setDisplayCategory(QCoreApplication::translate("Core", Constants::SETTINGS_TR_CATEGORY_CORE));
    setCategoryIcon(QLatin1String(Constants::SETTINGS_CATEGORY_CORE_ICON));
}

static bool hasQmFilesForLocale(const QString &locale, const QString &creatorTrPath)
{
    static const QString qtTrPath = QLibraryInfo::location(QLibraryInfo::TranslationsPath);

    const QString trFile = QLatin1String("/qt_") + locale + QLatin1String(".qm");
    return QFile::exists(qtTrPath + trFile) || QFile::exists(creatorTrPath + trFile);
}

void GeneralSettings::fillLanguageBox() const
{
    const QString currentLocale = language();

    m_page->languageBox->addItem(tr("<System Language>"), QString());
    // need to add this explicitly, since there is no qm file for English
    m_page->languageBox->addItem(QLatin1String("English"), QLatin1String("C"));
    if (currentLocale == QLatin1String("C"))
        m_page->languageBox->setCurrentIndex(m_page->languageBox->count() - 1);

    const QString creatorTrPath = ICore::resourcePath() + QLatin1String("/translations");
    const QStringList languageFiles = QDir(creatorTrPath).entryList(QStringList(QLatin1String("qtcreator*.qm")));

    foreach (const QString &languageFile, languageFiles) {
        int start = languageFile.indexOf(QLatin1Char('_'))+1;
        int end = languageFile.lastIndexOf(QLatin1Char('.'));
        const QString locale = languageFile.mid(start, end-start);
        // no need to show a language that creator will not load anyway
        if (hasQmFilesForLocale(locale, creatorTrPath)) {
            QLocale tmpLocale(locale);
            QString languageItem = QLocale::languageToString(tmpLocale.language()) + QLatin1String(" (")
                                   + QLocale::countryToString(tmpLocale.country()) + QLatin1Char(')');
            m_page->languageBox->addItem(languageItem, locale);
            if (locale == currentLocale)
                m_page->languageBox->setCurrentIndex(m_page->languageBox->count() - 1);
        }
    }
}

QWidget *GeneralSettings::widget()
{
    if (!m_widget) {
        m_page = new Ui::GeneralSettings();
        m_widget = new QWidget;
        m_page->setupUi(m_widget);

        fillLanguageBox();

        m_page->colorButton->setColor(StyleHelper::requestedBaseColor());
        m_page->reloadBehavior->setCurrentIndex(EditorManager::reloadSetting());
        if (HostOsInfo::isAnyUnixHost()) {
            const QStringList availableTerminals = ConsoleProcess::availableTerminalEmulators();
            const QString currentTerminal = ConsoleProcess::terminalEmulator(ICore::settings(), false);
            m_page->terminalComboBox->addItems(availableTerminals);
            m_page->terminalComboBox->lineEdit()->setText(currentTerminal);
            m_page->terminalComboBox->lineEdit()->setPlaceholderText(ConsoleProcess::defaultTerminalEmulator());
        } else {
            m_page->terminalLabel->hide();
            m_page->terminalComboBox->hide();
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
        m_page->autoSaveInterval->setValue(EditorManagerPrivate::autoSaveInterval());
        m_page->warnBeforeOpeningBigFiles->setChecked(
                    EditorManagerPrivate::warnBeforeOpeningBigFilesEnabled());
        m_page->bigFilesLimitSpinBox->setValue(EditorManagerPrivate::bigFileSizeLimit());
        m_page->resetWarningsButton->setEnabled(canResetWarnings());

        connect(m_page->resetColorButton, SIGNAL(clicked()),
                this, SLOT(resetInterfaceColor()));
        connect(m_page->resetWarningsButton, SIGNAL(clicked()),
                this, SLOT(resetWarnings()));
        if (HostOsInfo::isAnyUnixHost()) {
            connect(m_page->resetTerminalButton, SIGNAL(clicked()), this, SLOT(resetTerminal()));
            if (!HostOsInfo::isMacHost()) {
                connect(m_page->resetFileBrowserButton, SIGNAL(clicked()), this, SLOT(resetFileBrowser()));
                connect(m_page->helpExternalFileBrowserButton, SIGNAL(clicked()),
                        this, SLOT(showHelpForFileBrowser()));
            }
        }

        updatePath();

        connect(VcsManager::instance(), SIGNAL(configurationChanged(const IVersionControl*)),
                this, SLOT(updatePath()));
    }
    return m_widget;
}

void GeneralSettings::apply()
{
    if (!m_page) // wasn't shown, can't be changed
        return;
    int currentIndex = m_page->languageBox->currentIndex();
    setLanguage(m_page->languageBox->itemData(currentIndex, Qt::UserRole).toString());
    // Apply the new base color if accepted
    StyleHelper::setBaseColor(m_page->colorButton->color());
    EditorManager::setReloadSetting(IDocument::ReloadSetting(m_page->reloadBehavior->currentIndex()));
    if (HostOsInfo::isAnyUnixHost()) {
        ConsoleProcess::setTerminalEmulator(ICore::settings(),
                                            m_page->terminalComboBox->lineEdit()->text());
        if (!HostOsInfo::isMacHost()) {
            UnixUtils::setFileBrowser(ICore::settings(),
                                      m_page->externalFileBrowserEdit->text());
        }
    }
    PatchTool::setPatchCommand(m_page->patchChooser->path());
    EditorManagerPrivate::setAutoSaveEnabled(m_page->autoSaveCheckBox->isChecked());
    EditorManagerPrivate::setAutoSaveInterval(m_page->autoSaveInterval->value());
    EditorManagerPrivate::setWarnBeforeOpeningBigFilesEnabled(
                m_page->warnBeforeOpeningBigFiles->isChecked());
    EditorManagerPrivate::setBigFileSizeLimit(m_page->bigFilesLimitSpinBox->value());
    m_page->themeWidget->apply();
}

void GeneralSettings::finish()
{
    delete m_widget;
    delete m_page;
    m_page = 0;
}

void GeneralSettings::resetInterfaceColor()
{
    m_page->colorButton->setColor(StyleHelper::DEFAULT_BASE_COLOR);
}

void GeneralSettings::resetWarnings()
{
    InfoBar::clearGloballySuppressed();
    CheckableMessageBox::resetAllDoNotAskAgainQuestions(ICore::settings());
    m_page->warnBeforeOpeningBigFiles->setChecked(true);
    m_page->resetWarningsButton->setEnabled(false);
}

void GeneralSettings::resetTerminal()
{
    if (HostOsInfo::isAnyUnixHost())
        m_page->terminalComboBox->lineEdit()->clear();
}

void GeneralSettings::resetFileBrowser()
{
    if (HostOsInfo::isAnyUnixHost() && !HostOsInfo::isMacHost())
        m_page->externalFileBrowserEdit->setText(UnixUtils::defaultFileBrowser());
}

void GeneralSettings::updatePath()
{
    Environment env = Environment::systemEnvironment();
    QStringList toAdd = VcsManager::additionalToolsPath();
    env.appendOrSetPath(toAdd.join(HostOsInfo::pathListSeparator()));
    m_page->patchChooser->setEnvironment(env);
}

bool GeneralSettings::canResetWarnings() const
{
    return InfoBar::anyGloballySuppressed()
        || CheckableMessageBox::hasSuppressedQuestions(ICore::settings())
        || !EditorManagerPrivate::warnBeforeOpeningBigFilesEnabled();
}

void GeneralSettings::variableHelpDialogCreator(const QString &helpText)
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


void GeneralSettings::showHelpForFileBrowser()
{
    if (HostOsInfo::isAnyUnixHost() && !HostOsInfo::isMacHost())
        variableHelpDialogCreator(UnixUtils::fileBrowserHelpText());
}

void GeneralSettings::resetLanguage()
{
    // system language is default
    m_page->languageBox->setCurrentIndex(0);
}

QString GeneralSettings::language() const
{
    QSettings *settings = ICore::settings();
    return settings->value(QLatin1String("General/OverrideLanguage")).toString();
}

void GeneralSettings::setLanguage(const QString &locale)
{
    QSettings *settings = ICore::settings();
    if (settings->value(QLatin1String("General/OverrideLanguage")).toString() != locale)
        QMessageBox::information(ICore::mainWindow(), tr("Restart required"),
                                 tr("The language change will take effect after a restart of Qt Creator."));

    if (locale.isEmpty())
        settings->remove(QLatin1String("General/OverrideLanguage"));
    else
        settings->setValue(QLatin1String("General/OverrideLanguage"), locale);
}

} // namespace Internal
} // namespace Core
