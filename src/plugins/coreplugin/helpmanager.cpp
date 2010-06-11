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

#include "icore.h"

#include <QtCore/QDebug>
#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QStringList>

#include <QtHelp/QHelpEngineCore>

#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlDriver>
#include <QtSql/QSqlError>
#include <QtSql/QSqlQuery>

namespace Core {

HelpManager *HelpManager::m_instance = 0;

static const char linksForKeyQuery[] = "SELECT d.Title, f.Name, e.Name, "
    "d.Name, a.Anchor FROM IndexTable a, FileNameTable d, FolderTable e, "
    "NamespaceTable f WHERE a.FileId=d.FileId AND d.FolderId=e.Id AND "
    "a.NamespaceId=f.Id AND a.Name='%1'";

// -- DbCleaner

struct DbCleaner {
    DbCleaner(const QString &dbName)
        : name(dbName) {}
    ~DbCleaner() {
        QSqlDatabase::removeDatabase(name);
    }
    QString name;
};

// -- HelpManager

HelpManager::HelpManager(QObject *parent)
    : QObject(parent)
    , m_needsSetup(true)
    , m_helpEngine(0)
{
    Q_ASSERT(!m_instance);
    m_instance = this;

    connect(Core::ICore::instance(), SIGNAL(coreOpened()), this,
        SLOT(setupHelpManager()));
}

HelpManager::~HelpManager()
{
    delete m_helpEngine;
    m_helpEngine = 0;

    m_instance = 0;
}

HelpManager* HelpManager::instance()
{
    Q_ASSERT(m_instance);
    return m_instance;
}

QString HelpManager::collectionFilePath()
{
    const QFileInfo fi(Core::ICore::instance()->settings()->fileName());
    const QDir directory(fi.absolutePath() + QLatin1String("/qtcreator"));
    if (!directory.exists())
        directory.mkpath(directory.absolutePath());
    return QDir::cleanPath(directory.absolutePath() + QLatin1String("/helpcollection.qhc"));
}

void HelpManager::registerDocumentation(const QStringList &files)
{
    if (m_needsSetup) {
        m_filesToRegister.append(files);
        return;
    }

    bool docsChanged = false;
    foreach (const QString &file, files) {
        const QString &nameSpace = m_helpEngine->namespaceName(file);
        if (nameSpace.isEmpty())
            continue;
        if (!m_helpEngine->registeredDocumentations().contains(nameSpace)) {
            if (m_helpEngine->registerDocumentation(file)) {
                docsChanged = true;
            } else {
                qWarning() << "Error registering namespace '" << nameSpace
                    << "' from file '" << file << "':" << m_helpEngine->error();
            }
        }
    }
    if (docsChanged)
        emit documentationChanged();
}

void HelpManager::unregisterDocumentation(const QStringList &nameSpaces)
{
    if (m_needsSetup) {
        m_nameSpacesToUnregister.append(nameSpaces);
        return;
    }

    bool docsChanged = false;
    foreach (const QString &nameSpace, nameSpaces) {
        if (m_helpEngine->unregisterDocumentation(nameSpace)) {
            docsChanged = true;
        } else {
            qWarning() << "Error unregistering namespace '" << nameSpace
                << "' from file '" << m_helpEngine->documentationFileName(nameSpace)
                << "': " << m_helpEngine->error();
        }
    }
    if (docsChanged)
        emit documentationChanged();
}

QUrl buildQUrl(const QString &nameSpace, const QString &folder,
    const QString &relFileName, const QString &anchor)
{
    QUrl url;
    url.setScheme(QLatin1String("qthelp"));
    url.setAuthority(nameSpace);
    url.setPath(folder + QLatin1Char('/') + relFileName);
    url.setFragment(anchor);
    return url;
}

// This should go into Qt 4.8 once we start using it for Qt Creator
QMap<QString, QUrl> HelpManager::linksForKeyword(const QString &key) const
{
    QMap<QString, QUrl> links;
    if (m_needsSetup)
        return links;

    const QLatin1String sqlite("QSQLITE");
    const QLatin1String name("HelpManager::linksForKeyword");

    DbCleaner cleaner(name);
    QSqlDatabase db = QSqlDatabase::addDatabase(sqlite, name);
    if (db.driver() && db.driver()->lastError().type() == QSqlError::NoError) {
        const QStringList &registeredDocs = m_helpEngine->registeredDocumentations();
        foreach (const QString &nameSpace, registeredDocs) {
            db.setDatabaseName(m_helpEngine->documentationFileName(nameSpace));
            if (db.open()) {
                QSqlQuery query = QSqlQuery(db);
                query.setForwardOnly(true);
                query.exec(QString::fromLatin1(linksForKeyQuery).arg(key));
                while (query.next()) {
                    QString title = query.value(0).toString();
                    if (title.isEmpty()) // generate a title + corresponding path
                        title = key + QLatin1String(" : ") + query.value(3).toString();
                    links.insertMulti(title, buildQUrl(query.value(1).toString(),
                        query.value(2).toString(), query.value(3).toString(),
                        query.value(4).toString()));
                }
            }
        }
    }
    return links;
}

QMap<QString, QUrl> HelpManager::linksForIdentifier(const QString &id) const
{
    if (m_needsSetup)
        return QMap<QString, QUrl>();
    return m_helpEngine->linksForIdentifier(id);
}

// This should go into Qt 4.8 once we start using it for Qt Creator
QStringList HelpManager::findKeywords(const QString &key, int maxHits) const
{
    QStringList keywords;
    if (m_needsSetup)
        return keywords;

    const QLatin1String sqlite("QSQLITE");
    const QLatin1String name("HelpManager::findKeywords");

    DbCleaner cleaner(name);
    QSqlDatabase db = QSqlDatabase::addDatabase(sqlite, name);
    if (db.driver() && db.driver()->lastError().type() == QSqlError::NoError) {
        const QStringList &registeredDocs = m_helpEngine->registeredDocumentations();
        foreach (const QString &nameSpace, registeredDocs) {
            db.setDatabaseName(m_helpEngine->documentationFileName(nameSpace));
            if (db.open()) {
                QSqlQuery query = QSqlQuery(db);
                query.setForwardOnly(true);
                query.exec(QString::fromLatin1("SELECT DISTINCT Name FROM "
                    "IndexTable WHERE Name LIKE '%%1%'").arg(key));
                while (query.next()) {
                    const QString &key = query.value(0).toString();
                    if (!key.isEmpty()) {
                        keywords.append(key);
                        if (keywords.count() == maxHits)
                            return keywords;
                    }
                }
            }
        }
    }
    return keywords;
}

QUrl HelpManager::findFile(const QUrl &url) const
{
    if (m_needsSetup)
        return QUrl();
    return m_helpEngine->findFile(url);
}

void HelpManager::handleHelpRequest(const QString &url)
{
    emit helpRequested(QUrl(url));
}

QStringList HelpManager::registeredNamespaces() const
{
    if (m_needsSetup)
        return QStringList();
    return m_helpEngine->registeredDocumentations();
}

QString HelpManager::namespaceFromFile(const QString &file) const
{
    if (m_needsSetup)
        return QString();
    return m_helpEngine->namespaceName(file);
}

QString HelpManager::fileFromNamespace(const QString &nameSpace) const
{
    if (m_needsSetup)
        return QString();
    return m_helpEngine->documentationFileName(nameSpace);
}

// -- private slots

void HelpManager::setupHelpManager()
{
    if (!m_needsSetup)
        return;
    m_needsSetup = false;

    m_helpEngine = new QHelpEngineCore(collectionFilePath(), this);
    m_helpEngine->setAutoSaveFilter(false);
    m_helpEngine->setCurrentFilter(tr("Unfiltered"));
    m_helpEngine->setupData();

    verifyDocumenation();

    if (!m_nameSpacesToUnregister.isEmpty()) {
        unregisterDocumentation(m_nameSpacesToUnregister);
        m_nameSpacesToUnregister.clear();
    }

    // this might come from the installer
    const QLatin1String key("AddedDocs");
    const QString &addedDocs = m_helpEngine->customValue(key).toString();
    if (!addedDocs.isEmpty()) {
        m_helpEngine->removeCustomValue(key);
        m_filesToRegister += addedDocs.split(QLatin1Char(';'));
    }

    if (!m_filesToRegister.isEmpty()) {
        registerDocumentation(m_filesToRegister);
        m_filesToRegister.clear();
    }

    emit setupFinished();
}

// -- private

void HelpManager::verifyDocumenation()
{
    QStringList nameSpacesToUnregister;
    const QStringList &registeredDocs = m_helpEngine->registeredDocumentations();
    foreach (const QString &nameSpace, registeredDocs) {
        const QString &file = m_helpEngine->documentationFileName(nameSpace);
        if (!QFileInfo(file).exists())
            nameSpacesToUnregister.append(nameSpace);
    }

    if (!nameSpacesToUnregister.isEmpty())
        unregisterDocumentation(nameSpacesToUnregister);
}

}   // Core
