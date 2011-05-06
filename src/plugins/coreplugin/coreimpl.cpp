/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#include "coreimpl.h"
#include "mainwindow.h"

#include <QtCore/QDir>
#include <QtCore/QCoreApplication>
#include <QtCore/QDebug>

#include <QtGui/QStatusBar>

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

CoreImpl::~CoreImpl()
{
    m_instance = 0;
}

void CoreImpl::showNewItemDialog(const QString &title,
                                        const QList<IWizard *> &wizards,
                                        const QString &defaultLocation)
{
    m_mainwindow->showNewItemDialog(title, wizards, defaultLocation);
}

bool CoreImpl::showOptionsDialog(const QString &group, const QString &page, QWidget *parent)
{
    return m_mainwindow->showOptionsDialog(group, page, parent);
}

bool CoreImpl::showWarningWithOptions(const QString &title, const QString &text,
                                      const QString &details,
                                      const QString &settingsCategory,
                                      const QString &settingsId,
                                      QWidget *parent)
{
    return m_mainwindow->showWarningWithOptions(title, text,
                                                details, settingsCategory,
                                                settingsId, parent);
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

VcsManager *CoreImpl::vcsManager() const
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

HelpManager *CoreImpl::helpManager() const
{
    return m_mainwindow->helpManager();
}

QSettings *CoreImpl::settings(QSettings::Scope scope) const
{
    return m_mainwindow->settings(scope);
}

SettingsDatabase *CoreImpl::settingsDatabase() const
{
    return m_mainwindow->settingsDatabase();
}

QPrinter *CoreImpl::printer() const
{
    return m_mainwindow->printer();
}

QString CoreImpl::userInterfaceLanguage() const
{
    return qApp->property("qtc_locale").toString();
}

#ifdef Q_OS_MAC
#  define SHARE_PATH "/../Resources"
#else
#  define SHARE_PATH "/../share/qtcreator"
#endif

QString CoreImpl::resourcePath() const
{
    return QDir::cleanPath(QCoreApplication::applicationDirPath() + QLatin1String(SHARE_PATH));
}

QString CoreImpl::userResourcePath() const
{
    // Create qtcreator dir if it doesn't yet exist
    const QString configDir = QFileInfo(settings(QSettings::UserScope)->fileName()).path();
    const QString urp = configDir + "/qtcreator";

    QFileInfo fi(urp + QLatin1Char('/'));
    if (!fi.exists()) {
        QDir dir;
        if (!dir.mkpath(urp))
            qWarning() << "could not create" << urp;
    }

    return urp;
}

IContext *CoreImpl::currentContextObject() const
{
    return m_mainwindow->currentContextObject();
}


QMainWindow *CoreImpl::mainWindow() const
{
    return m_mainwindow;
}

QStatusBar *CoreImpl::statusBar() const
{
    return m_mainwindow->statusBar();
}

void CoreImpl::updateAdditionalContexts(const Context &remove, const Context &add)
{
    m_mainwindow->updateAdditionalContexts(remove, add);
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

void CoreImpl::openFiles(const QStringList &arguments, ICore::OpenFilesFlags flags)
{
    m_mainwindow->openFiles(arguments, flags);
}

void CoreImpl::emitNewItemsDialogRequested()
{
    emit newItemsDialogRequested();
}
