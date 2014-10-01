/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://www.qt.io/licensing.  For further information
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
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "clangindexer.h"
#include "clangsymbolsearcher.h"
#include "clangutils.h"
#include "indexer.h"
#include "liveunitsmanager.h"

#include <coreplugin/icore.h>
#include <coreplugin/progressmanager/progressmanager.h>
#include <cpptools/cppmodelmanagerinterface.h>
#include <projectexplorer/session.h>

#include <QDir>

using namespace ClangCodeModel;
using namespace ClangCodeModel::Internal;

ClangIndexingSupport::ClangIndexingSupport(ClangIndexer *indexer)
    : m_indexer(indexer)
{
}

ClangIndexingSupport::~ClangIndexingSupport()
{
}

QFuture<void> ClangIndexingSupport::refreshSourceFiles(const QStringList &sourceFiles)
{
    return m_indexer->refreshSourceFiles(sourceFiles);
}

CppTools::SymbolSearcher *ClangIndexingSupport::createSymbolSearcher(CppTools::SymbolSearcher::Parameters parameters, QSet<QString> fileNames)
{
    return new ClangSymbolSearcher(m_indexer, parameters, fileNames);
}

ClangIndexer::ClangIndexer()
    : QObject(0)
    , m_indexingSupport(new ClangIndexingSupport(this))
    , m_isLoadingSession(false)
    , m_clangIndexer(new Indexer(this))
{
    connect(m_clangIndexer, SIGNAL(indexingStarted(QFuture<void>)),
            this, SLOT(onIndexingStarted(QFuture<void>)));

    QObject *session = ProjectExplorer::SessionManager::instance();

    connect(session, SIGNAL(aboutToLoadSession(QString)),
            this, SLOT(onAboutToLoadSession(QString)));
    connect(session, SIGNAL(sessionLoaded(QString)),
            this, SLOT(onSessionLoaded(QString)));
    connect(session, SIGNAL(aboutToSaveSession()),
            this, SLOT(onAboutToSaveSession()));
}

ClangIndexer::~ClangIndexer()
{
    m_clangIndexer->cancel(true);
}

CppTools::CppIndexingSupport *ClangIndexer::indexingSupport()
{
    return m_indexingSupport.data();
}

QFuture<void> ClangIndexer::refreshSourceFiles(const QStringList &sourceFiles)
{
    typedef CppTools::ProjectPart ProjectPart;
    CppTools::CppModelManagerInterface *mmi = CppTools::CppModelManagerInterface::instance();
    LiveUnitsManager *lum = LiveUnitsManager::instance();

    if (m_clangIndexer->isBusy())
        m_clangIndexer->cancel(true);

    foreach (const QString &file, sourceFiles) {
        if (lum->isTracking(file))
            continue; // we get notified separately about open files.
        const QList<ProjectPart::Ptr> &parts = mmi->projectPart(file);
        if (!parts.isEmpty())
            m_clangIndexer->addFile(file, parts.at(0));
        else
            m_clangIndexer->addFile(file, ProjectPart::Ptr());
    }

    if (!m_isLoadingSession)
        m_clangIndexer->regenerate();

    return QFuture<void>();
}

void ClangIndexer::match(ClangSymbolSearcher *searcher) const
{
    m_clangIndexer->match(searcher);
}

void ClangIndexer::onAboutToLoadSession(const QString &sessionName)
{
    m_isLoadingSession = true;

    if (sessionName == QLatin1String("default"))
        return;

    QString path = Core::ICore::instance()->userResourcePath() + QLatin1String("/codemodel/");
    if (QFile::exists(path) || QDir().mkpath(path))
        m_clangIndexer->initialize(path + sessionName + QLatin1String(".qci"));
}

void ClangIndexer::onSessionLoaded(QString)
{
    m_isLoadingSession = false;
    m_clangIndexer->regenerate();
}

void ClangIndexer::onAboutToSaveSession()
{
    m_clangIndexer->finalize();
}

void ClangIndexer::indexNow(Unit::Ptr unit)
{
    typedef CppTools::ProjectPart ProjectPart;

    QString file = unit->fileName();
    CppTools::CppModelManagerInterface *mmi = CppTools::CppModelManagerInterface::instance();
    const QList<ProjectPart::Ptr> &parts = mmi->projectPart(file);
    ProjectPart::Ptr part;
    if (!parts.isEmpty())
        part = parts.at(0);
    if (!m_isLoadingSession)
        m_clangIndexer->runQuickIndexing(unit, part);
}

void ClangIndexer::onIndexingStarted(QFuture<void> indexingFuture)
{
    Core::ICore::instance()->progressManager()->addTask(indexingFuture,
                                                        tr("C++ Indexing"),
                                                        QLatin1String("Key.Temp.Indexing"));
}
