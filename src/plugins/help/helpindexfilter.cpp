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

// -- HelpIndexFilter::HelpFileReader

class HelpIndexFilter::HelpFileReader
{
public:
    HelpFileReader();
    ~HelpFileReader();

public:
    void updateHelpFiles();
    QList<FilterEntry> matchesFor(const QString &entry, ILocatorFilter *locator);

private:
    bool m_initialized;
    QStringList m_helpFiles;
};

HelpIndexFilter::HelpFileReader::HelpFileReader()
    : m_initialized(false)
{
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

QList<FilterEntry> HelpIndexFilter::HelpFileReader::matchesFor(const QString &id,
    ILocatorFilter *locator)
{
    if (!m_initialized) {
        updateHelpFiles();
        m_initialized = true;
    }

    QList<FilterEntry> entries;
    const QLatin1String sqlite("QSQLITE");
    const QLatin1String name("HelpIndexFilter::HelpFileReader::matchesFor");
    foreach(const QString &file, m_helpFiles) {
        if (!QFile::exists(file))
            continue;
        {
            QSqlDatabase db = QSqlDatabase::addDatabase(sqlite, name);
            if (db.driver()
                && db.driver()->lastError().type() == QSqlError::NoError) {
                db.setDatabaseName(file);
                if (db.open()) {
                    QSqlQuery query = QSqlQuery(db);
                    query.setForwardOnly(true);
                    query.exec(QString::fromLatin1("SELECT DISTINCT Name FROM "
                        "IndexTable WHERE Name LIKE '%%1%'").arg(id));
                    while (query.next()) {
                        const QString &key = query.value(0).toString();
                        if (!key.isEmpty()) {
                            // NOTE: do not use an icon since it is really slow
                            entries.append(FilterEntry(locator, key, QVariant(),
                                QIcon()));
                        }
                    }
                }
            }
        }
        QSqlDatabase::removeDatabase(name);
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
        return QList<FilterEntry>();
    return m_fileReader->matchesFor(entry, this);
}

void HelpIndexFilter::accept(FilterEntry selection) const
{
    const QHelpEngineCore &engine = HelpManager::helpEngineCore();
    QMap<QString, QUrl> links = engine.linksForIdentifier(selection.displayName);
    if (links.size() == 1) {
        emit linkActivated(links.begin().value());
    } else if (!links.isEmpty()) {
        emit linksActivated(links, selection.displayName);
    }
}

void HelpIndexFilter::refresh(QFutureInterface<void> &future)
{
    Q_UNUSED(future)
    // Nothing to refresh
}
