/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
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

#include "helpmanager.h"
#include "bookmarkmanager.h"

#include <coreplugin/icore.h>

#include <QtCore/QDebug>
#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QMutexLocker>
#include <QtCore/QString>
#include <QtCore/QUrl>

#include <QtHelp/QHelpEngine>
#include <QtHelp/QHelpEngineCore>

using namespace Help;

bool HelpManager::m_guiNeedsSetup = true;
bool HelpManager::m_needsCollectionFile = true;

QMutex HelpManager::m_guiMutex;
QHelpEngine* HelpManager::m_guiEngine = 0;

QMutex HelpManager::m_coreMutex;
QHelpEngineCore* HelpManager::m_coreEngine = 0;

HelpManager* HelpManager::m_helpManager = 0;
BookmarkManager* HelpManager::m_bookmarkManager = 0;

HelpManager::HelpManager(QObject *parent)
    : QObject(parent)
{
    Q_ASSERT(!m_helpManager);
    m_helpManager = this;
}

HelpManager::~HelpManager()
{
    delete m_guiEngine;
    m_guiEngine = 0;

    delete m_coreEngine;
    m_coreEngine = 0;

    if (m_bookmarkManager) {
        m_bookmarkManager->saveBookmarks();
        delete m_bookmarkManager;
        m_bookmarkManager = 0;
    }
}

HelpManager& HelpManager::instance()
{
    Q_ASSERT(m_helpManager);
    return *m_helpManager;
}

void HelpManager::setupGuiHelpEngine()
{
    if (m_needsCollectionFile) {
        m_needsCollectionFile = false;
        (&helpEngine())->setCollectionFile(collectionFilePath());
    }

    if (m_guiNeedsSetup) {
        m_guiNeedsSetup = false;
        (&helpEngine())->setupData();
    }
}

bool HelpManager::guiEngineNeedsUpdate() const
{
    return m_guiNeedsSetup;
}

void HelpManager::handleHelpRequest(const QString &url)
{
    emit helpRequested(url);
}

void HelpManager::verifyDocumenation()
{
    QStringList nameSpacesToUnregister;
    QHelpEngineCore *engine = &helpEngineCore();
    const QStringList &registeredDocs = engine->registeredDocumentations();
    foreach (const QString &nameSpace, registeredDocs) {
        const QString &file = engine->documentationFileName(nameSpace);
        if (!QFileInfo(file).exists())
            nameSpacesToUnregister.append(nameSpace);
    }

    if (!nameSpacesToUnregister.isEmpty())
        unregisterDocumentation(nameSpacesToUnregister);
}

void HelpManager::registerDocumentation(const QStringList &files)
{
    QHelpEngineCore *engine = &helpEngineCore();
    foreach (const QString &file, files) {
        const QString &nameSpace = engine->namespaceName(file);
        if (nameSpace.isEmpty())
            continue;
        if (!engine->registeredDocumentations().contains(nameSpace)) {
            if (engine->registerDocumentation(file)) {
                m_guiNeedsSetup = true;
            } else {
                qWarning() << "Error registering namespace '" << nameSpace
                    << "' from file '" << file << "':" << engine->error();
            }
        }
    }
}

void HelpManager::unregisterDocumentation(const QStringList &nameSpaces)
{
    QHelpEngineCore *engine = &helpEngineCore();
    foreach (const QString &nameSpace, nameSpaces) {
        const QString &file = engine->documentationFileName(nameSpace);
        if (engine->unregisterDocumentation(nameSpace)) {
            m_guiNeedsSetup = true;
        } else {
            qWarning() << "Error unregistering namespace '" << nameSpace
                << "' from file '" << file << "': " << engine->error();
        }
    }
}

QHelpEngine &HelpManager::helpEngine()
{
    if (!m_guiEngine) {
        QMutexLocker _(&m_guiMutex);
        if (!m_guiEngine) {
            m_guiEngine = new QHelpEngine("");
            m_guiEngine->setAutoSaveFilter(false);
        }
    }
    return *m_guiEngine;
}

QString HelpManager::collectionFilePath()
{
    const QFileInfo fi(Core::ICore::instance()->settings()->fileName());
    const QDir directory(fi.absolutePath() + QLatin1String("/qtcreator"));
    if (!directory.exists())
        directory.mkpath(directory.absolutePath());
    return QDir::cleanPath(directory.absolutePath() + QLatin1String("/helpcollection.qhc"));
}

QHelpEngineCore& HelpManager::helpEngineCore()
{
    if (!m_coreEngine) {
        QMutexLocker _(&m_coreMutex);
        if (!m_coreEngine) {
            m_coreEngine = new QHelpEngineCore(collectionFilePath());
            m_coreEngine->setAutoSaveFilter(false);
            m_coreEngine->setCurrentFilter(tr("Unfiltered"));
            m_coreEngine->setupData();
        }
    }
    return *m_coreEngine;
}

BookmarkManager& HelpManager::bookmarkManager()
{
    if (!m_bookmarkManager) {
        m_bookmarkManager = new BookmarkManager;
        m_bookmarkManager->setupBookmarkModels();
    }
    return *m_bookmarkManager;
}
