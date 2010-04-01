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

#include "helpindexfilter.h"
#include "helpmanager.h"

#include <extensionsystem/pluginmanager.h>
#include <coreplugin/icore.h>
#include <coreplugin/modemanager.h>

#include <QtGui/QIcon>

#include <QtHelp/QHelpEngine>
#include <QtHelp/QHelpIndexModel>

#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlDriver>
#include <QtSql/QSqlError>
#include <QtSql/QSqlQuery>

using namespace Locator;
using namespace Help;
using namespace Help::Internal;

Q_DECLARE_METATYPE(ILocatorFilter*);

static const char linksForKeyQuery[] = "SELECT d.Title, f.Name, e.Name, "
    "d.Name, a.Anchor FROM IndexTable a, FileNameTable d, FolderTable e, "
    "NamespaceTable f WHERE a.FileId=d.FileId AND d.FolderId=e.Id AND "
    "a.NamespaceId=f.Id AND a.Name='%1'";

// -- HelpIndexFilter::HelpFileReader

class HelpIndexFilter::HelpFileReader
{
    struct dbCleaner {
        dbCleaner(const QString &dbName)
            : name(dbName) {}
        ~dbCleaner() {
            QSqlDatabase::removeDatabase(name);
        }
        QString name;
    };

public:
    HelpFileReader();
    ~HelpFileReader();

public:
    void updateHelpFiles();
    QMap<QString, QUrl> linksForKey(const QString &key);
    QList<FilterEntry> matchesFor(const QString &entry, ILocatorFilter *locator,
        int maxHits = INT_MAX);

private:
    QIcon m_icon;
    bool m_initialized;
    QStringList m_helpFiles;
};

HelpIndexFilter::HelpFileReader::HelpFileReader()
    : m_initialized(false)
{
    m_icon = QIcon(QLatin1String(":/help/images/bookmark.png"));
}

HelpIndexFilter::HelpFileReader::~HelpFileReader()
{
}

void HelpIndexFilter::HelpFileReader::updateHelpFiles()
{
    m_helpFiles.clear();
    const QLatin1String id("HelpIndexFilter::HelpFileReader::helpFiles");
    {
        QSqlDatabase db = QSqlDatabase::addDatabase(QLatin1String("QSQLITE"), id);
        if (db.driver()
            && db.driver()->lastError().type() == QSqlError::NoError) {
            db.setDatabaseName(HelpManager::collectionFilePath());
            if (db.open()) {
                QSqlQuery query = QSqlQuery(db);
                query.exec(QLatin1String("SELECT a.FilePath FROM NamespaceTable a"));
                while (query.next())
                    m_helpFiles.append(query.value(0).toString());
            }
        }
    }
    QSqlDatabase::removeDatabase(id);
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

QMap<QString, QUrl>HelpIndexFilter::HelpFileReader::linksForKey(const QString &key)
{
    if (!m_initialized) {
        updateHelpFiles();
        m_initialized = true;
    }

    QMap<QString, QUrl> links;
    const QLatin1String sqlite("QSQLITE");
    const QLatin1String name("HelpIndexFilter::HelpFileReader::linksForKey");

    dbCleaner cleaner(name);
    QSqlDatabase db = QSqlDatabase::addDatabase(sqlite, name);
    if (db.driver() && db.driver()->lastError().type() == QSqlError::NoError) {
        foreach(const QString &file, m_helpFiles) {
            if (!QFile::exists(file))
                continue;
            db.setDatabaseName(file);
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

QList<FilterEntry> HelpIndexFilter::HelpFileReader::matchesFor(const QString &id,
    ILocatorFilter *locator, int maxHits)
{
    if (!m_initialized) {
        updateHelpFiles();
        m_initialized = true;
    }

    QList<FilterEntry> entries;
    const QLatin1String sqlite("QSQLITE");
    const QLatin1String name("HelpIndexFilter::HelpFileReader::matchesFor");

    dbCleaner cleaner(name);
    QSqlDatabase db = QSqlDatabase::addDatabase(sqlite, name);
    if (db.driver() && db.driver()->lastError().type() == QSqlError::NoError) {
        foreach(const QString &file, m_helpFiles) {
            if (!QFile::exists(file))
                continue;
            db.setDatabaseName(file);
            if (db.open()) {
                QSqlQuery query = QSqlQuery(db);
                query.setForwardOnly(true);
                query.exec(QString::fromLatin1("SELECT DISTINCT Name FROM "
                    "IndexTable WHERE Name LIKE '%%1%'").arg(id));
                while (query.next()) {
                    const QString &key = query.value(0).toString();
                    if (!key.isEmpty()) {
                        entries.append(FilterEntry(locator, key, QVariant(),
                            m_icon));
                        if (entries.count() == maxHits)
                            return entries;
                    }
                }
            }
        }
    }
    return entries;
}

// -- HelpIndexFilter

HelpIndexFilter::HelpIndexFilter()
    : m_fileReader(new HelpFileReader)
{
    setIncludedByDefault(false);
    setShortcutString(QString(QLatin1Char('?')));

    connect(&HelpManager::helpEngineCore(), SIGNAL(setupFinished()), this,
        SLOT(updateHelpFiles()));
}

HelpIndexFilter::~HelpIndexFilter()
{
    delete m_fileReader;
}

void HelpIndexFilter::updateHelpFiles()
{
    m_fileReader->updateHelpFiles();
}

QString HelpIndexFilter::displayName() const
{
    return tr("Help index");
}

QString HelpIndexFilter::id() const
{
    return QLatin1String("HelpIndexFilter");
}

ILocatorFilter::Priority HelpIndexFilter::priority() const
{
    return Medium;
}

QList<FilterEntry> HelpIndexFilter::matchesFor(const QString &entry)
{
    if (entry.length() < 2)
        return m_fileReader->matchesFor(entry, this, 300);
    return m_fileReader->matchesFor(entry, this);
}

void HelpIndexFilter::accept(FilterEntry selection) const
{
    const QString &key = selection.displayName;
    const QMap<QString, QUrl> &links = m_fileReader->linksForKey(key);
    if (links.size() == 1) {
        emit linkActivated(links.begin().value());
    } else if (!links.isEmpty()) {
        emit linksActivated(links, key);
    }
}

void HelpIndexFilter::refresh(QFutureInterface<void> &future)
{
    Q_UNUSED(future)
    // Nothing to refresh
}
