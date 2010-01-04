/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "generalsettings.h"
#include "coreconstants.h"

#include <utils/stylehelper.h>
#include <utils/qtcolorbutton.h>
#include <utils/consoleprocess.h>
#include <utils/unixutils.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/icore.h>

#include <QtGui/QMessageBox>

#include <QtCore/QCoreApplication>
#include <QtCore/QTextStream>

#include "ui_generalsettings.h"

using namespace Utils;
using namespace Core::Internal;


GeneralSettings::GeneralSettings():
    m_dialog(0)
{
}

QString GeneralSettings::id() const
{
    return QLatin1String("A.General");
}

QString GeneralSettings::trName() const
{
    return tr("General");
}

QString GeneralSettings::category() const
{
    return QLatin1String(Core::Constants::SETTINGS_CATEGORY_CORE);
}

QString GeneralSettings::trCategory() const
{
    return QCoreApplication::translate("Core", Core::Constants::SETTINGS_TR_CATEGORY_CORE);
}

QWidget *GeneralSettings::createPage(QWidget *parent)
{
    m_page = new Ui::GeneralSettings();
    QWidget *w = new QWidget(parent);
    m_page->setupUi(w);

    QSettings* settings = Core::ICore::instance()->settings();
    m_page->colorButton->setColor(StyleHelper::baseColor());
    m_page->externalEditorEdit->setText(EditorManager::instance()->externalEditor());
    m_page->reloadBehavior->setCurrentIndex(EditorManager::instance()->reloadBehavior());
#ifdef Q_OS_UNIX
    m_page->terminalEdit->setText(ConsoleProcess::terminalEmulator(settings));
#else
    m_page->terminalLabel->hide();
    m_page->terminalEdit->hide();
    m_page->resetTerminalButton->hide();
#endif

#if defined(Q_OS_UNIX) && !defined(Q_OS_MAC)
    m_page->externalFileBrowserEdit->setText(UnixUtils::fileBrowser(settings));
#else
    m_page->externalFileBrowserLabel->hide();
    m_page->externalFileBrowserEdit->hide();
    m_page->resetFileBrowserButton->hide();
#endif

    connect(m_page->resetButton, SIGNAL(clicked()),
            this, SLOT(resetInterfaceColor()));
    connect(m_page->resetEditorButton, SIGNAL(clicked()),
            this, SLOT(resetExternalEditor()));
    connect(m_page->helpExternalEditorButton, SIGNAL(clicked()),
            this, SLOT(showHelpForExternalEditor()));
#ifdef Q_OS_UNIX
    connect(m_page->resetTerminalButton, SIGNAL(clicked()),
            this, SLOT(resetTerminal()));
#ifndef Q_OS_MAC
    connect(m_page->resetFileBrowserButton, SIGNAL(clicked()),
            this, SLOT(resetFileBrowser()));
    connect(m_page->helpExternalFileBrowserButton, SIGNAL(clicked()),
            this, SLOT(showHelpForFileBrowser()));
#endif
#endif

    if (m_searchKeywords.isEmpty()) {
        QTextStream(&m_searchKeywords) << m_page->colorLabel->text() << ' '
                << m_page->terminalLabel->text() << ' ' << m_page->editorLabel->text()
                << ' '<< m_page->modifiedLabel->text();
        m_searchKeywords.remove(QLatin1Char('&'));
    }
    return w;
}

bool GeneralSettings::matches(const QString &s) const
{
    return m_searchKeywords.contains(s, Qt::CaseInsensitive);
}

void GeneralSettings::apply()
{
    // Apply the new base color if accepted
    StyleHelper::setBaseColor(m_page->colorButton->color());
    EditorManager::instance()->setExternalEditor(m_page->externalEditorEdit->text());
    EditorManager::instance()->setReloadBehavior(IFile::ReloadBehavior(m_page->reloadBehavior->currentIndex()));
#ifdef Q_OS_UNIX
	ConsoleProcess::setTerminalEmulator(Core::ICore::instance()->settings(),
                                        m_page->terminalEdit->text());
#ifndef Q_OS_MAC
        Utils::UnixUtils::setFileBrowser(Core::ICore::instance()->settings(), m_page->externalFileBrowserEdit->text());
#endif
#endif
}

void GeneralSettings::finish()
{
    delete m_page;
}

void GeneralSettings::resetInterfaceColor()
{
    m_page->colorButton->setColor(0x666666);
}

void GeneralSettings::resetExternalEditor()
{
    m_page->externalEditorEdit->setText(EditorManager::instance()->defaultExternalEditor());
}

#ifdef Q_OS_UNIX
void GeneralSettings::resetTerminal()
{
    m_page->terminalEdit->setText(ConsoleProcess::defaultTerminalEmulator() + QLatin1String(" -e"));
}

#ifndef Q_OS_MAC
void GeneralSettings::resetFileBrowser()
{
    m_page->externalFileBrowserEdit->setText(UnixUtils::defaultFileBrowser());
}
#endif
#endif


void GeneralSettings::variableHelpDialogCreator(const QString& helpText)
{
    if (m_dialog) {
        m_dialog->show();
        m_dialog->raise();
        m_dialog->activateWindow();
        return;
    }
    QMessageBox *mb = new QMessageBox(QMessageBox::Information,
                                  tr("Variables"),
                                  helpText,
                                  QMessageBox::Close,
                                  m_page->helpExternalEditorButton);
    mb->setWindowModality(Qt::NonModal);
    m_dialog = mb;
    mb->show();
}


void GeneralSettings::showHelpForExternalEditor()
{
    variableHelpDialogCreator(EditorManager::instance()->externalEditorHelpText());
}

#if defined(Q_OS_UNIX) && !defined(Q_OS_MAC)
void GeneralSettings::showHelpForFileBrowser()
{
    variableHelpDialogCreator(UnixUtils::fileBrowserHelpText());
}
#endif
