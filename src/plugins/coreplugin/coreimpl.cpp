/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
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
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#include "coreimpl.h"

#include <QtCore/QDir>
#include <QtCore/QCoreApplication>

namespace Core {
namespace Internal {

// The Core Singleton
static CoreImpl *m_instance = 0;

} // namespace Internal
} // namespace Core


using namespace Core;
using namespace Core::Internal;


ICore* ICore::instance()
{
    return m_instance;
}

CoreImpl::CoreImpl(MainWindow *mainwindow)
{
    m_instance = this;
    m_mainwindow = mainwindow;
}

QStringList CoreImpl::showNewItemDialog(const QString &title,
                                        const QList<IWizard *> &wizards,
                                        const QString &defaultLocation)
{
    return m_mainwindow->showNewItemDialog(title, wizards, defaultLocation);
}

void CoreImpl::showOptionsDialog(const QString &group, const QString &page)
{
    m_mainwindow->showOptionsDialog(group, page);
}

ActionManager *CoreImpl::actionManager() const
{
    return m_mainwindow->actionManager();
}

FileManager *CoreImpl::fileManager() const
{
    return m_mainwindow->fileManager();
}

UniqueIDManager *CoreImpl::uniqueIDManager() const
{
    return m_mainwindow->uniqueIDManager();
}

MessageManager *CoreImpl::messageManager() const
{
    return m_mainwindow->messageManager();
}

EditorManager *CoreImpl::editorManager() const
{
    return m_mainwindow->editorManager();
}

ProgressManager *CoreImpl::progressManager() const
{
    return m_mainwindow->progressManager();
}

ScriptManager *CoreImpl::scriptManager() const
{
    return m_mainwindow->scriptManager();
}

VariableManager *CoreImpl::variableManager() const
{
    return m_mainwindow->variableManager();
}

VCSManager *CoreImpl::vcsManager() const
{
    return m_mainwindow->vcsManager();
}

ModeManager *CoreImpl::modeManager() const
{
    return m_mainwindow->modeManager();
}

MimeDatabase *CoreImpl::mimeDatabase() const
{
    return m_mainwindow->mimeDatabase();
}

QSettings *CoreImpl::settings() const
{
    return m_mainwindow->settings();
}

QPrinter *CoreImpl::printer() const
{
    return m_mainwindow->printer();
}

QString CoreImpl::resourcePath() const
{
#if defined(Q_OS_MAC)
    return QDir::cleanPath(QCoreApplication::applicationDirPath()+QLatin1String("/../Resources"));
#else
    return QDir::cleanPath(QCoreApplication::applicationDirPath())+"/../share/qtcreator";
#endif
}

IContext *CoreImpl::currentContextObject() const
{
    return m_mainwindow->currentContextObject();
}


QMainWindow *CoreImpl::mainWindow() const
{
    return m_mainwindow;
}

// adds and removes additional active contexts, this context is appended to the
// currently active contexts. call updateContext after changing
void CoreImpl::addAdditionalContext(int context)
{
    m_mainwindow->addAdditionalContext(context);
}

void CoreImpl::removeAdditionalContext(int context)
{
    m_mainwindow->removeAdditionalContext(context);
}

bool CoreImpl::hasContext(int context) const
{
    return m_mainwindow->hasContext(context);
}

void CoreImpl::addContextObject(IContext *context)
{
    m_mainwindow->addContextObject(context);
}

void CoreImpl::removeContextObject(IContext *context)
{
    m_mainwindow->removeContextObject(context);
}

void CoreImpl::updateContext()
{
    return m_mainwindow->updateContext();
}

void CoreImpl::openFiles(const QStringList &arguments)
{
    m_mainwindow->openFiles(arguments);
}

