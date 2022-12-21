// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "cppincludesfilter.h"

#include "cppeditorconstants.h"
#include "cppmodelmanager.h"

#include <cplusplus/CppDocument.h>
#include <coreplugin/editormanager/documentmodel.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/session.h>

using namespace Core;
using namespace ProjectExplorer;
using namespace Utils;

namespace CppEditor::Internal {

class CppIncludesIterator final : public BaseFileFilter::Iterator
{
public:
    CppIncludesIterator(CPlusPlus::Snapshot snapshot, const QSet<FilePath> &seedPaths)
        : m_snapshot(snapshot),
          m_paths(seedPaths)
    {
        toFront();
    }

    void toFront() override;
    bool hasNext() const override;
    Utils::FilePath next() override;
    Utils::FilePath filePath() const override;

private:
    void fetchMore();

    CPlusPlus::Snapshot m_snapshot;
    QSet<FilePath> m_paths;
    QSet<FilePath> m_queuedPaths;
    QSet<FilePath> m_allResultPaths;
    FilePaths m_resultQueue;
    FilePath m_currentPath;
};



void CppIncludesIterator::toFront()
{
    m_queuedPaths = m_paths;
    m_allResultPaths.clear();
    m_resultQueue.clear();
    fetchMore();
}

bool CppIncludesIterator::hasNext() const
{
    return !m_resultQueue.isEmpty();
}

FilePath CppIncludesIterator::next()
{
    if (m_resultQueue.isEmpty())
        return {};
    m_currentPath = m_resultQueue.takeFirst();
    if (m_resultQueue.isEmpty())
        fetchMore();
    return m_currentPath;
}

FilePath CppIncludesIterator::filePath() const
{
    return m_currentPath;
}

void CppIncludesIterator::fetchMore()
{
    while (!m_queuedPaths.isEmpty() && m_resultQueue.isEmpty()) {
        const FilePath filePath = *m_queuedPaths.begin();
        m_queuedPaths.remove(filePath);
        CPlusPlus::Document::Ptr doc = m_snapshot.document(filePath);
        if (!doc)
            continue;
        const FilePaths includedFiles = doc->includedFiles();
        for (const FilePath &includedPath : includedFiles ) {
            if (!m_allResultPaths.contains(includedPath)) {
                m_allResultPaths.insert(includedPath);
                m_queuedPaths.insert(includedPath);
                m_resultQueue.append(includedPath);
            }
        }
    }
}

CppIncludesFilter::CppIncludesFilter()
{
    setId(Constants::INCLUDES_FILTER_ID);
    setDisplayName(Constants::INCLUDES_FILTER_DISPLAY_NAME);
    setDescription(
        tr("Matches all files that are included by all C++ files in all projects. Append "
           "\"+<number>\" or \":<number>\" to jump to the given line number. Append another "
           "\"+<number>\" or \":<number>\" to jump to the column number as well."));
    setDefaultShortcutString("ai");
    setDefaultIncludedByDefault(true);
    setPriority(ILocatorFilter::Low);

    connect(ProjectExplorerPlugin::instance(), &ProjectExplorerPlugin::fileListChanged,
            this, &CppIncludesFilter::markOutdated);
    connect(CppModelManager::instance(), &CppModelManager::documentUpdated,
            this, &CppIncludesFilter::markOutdated);
    connect(CppModelManager::instance(), &CppModelManager::aboutToRemoveFiles,
            this, &CppIncludesFilter::markOutdated);
    connect(DocumentModel::model(), &QAbstractItemModel::rowsInserted,
            this, &CppIncludesFilter::markOutdated);
    connect(DocumentModel::model(), &QAbstractItemModel::rowsRemoved,
            this, &CppIncludesFilter::markOutdated);
    connect(DocumentModel::model(), &QAbstractItemModel::dataChanged,
            this, &CppIncludesFilter::markOutdated);
    connect(DocumentModel::model(), &QAbstractItemModel::modelReset,
            this, &CppIncludesFilter::markOutdated);
}

void CppIncludesFilter::prepareSearch(const QString &entry)
{
    Q_UNUSED(entry)
    if (m_needsUpdate) {
        m_needsUpdate = false;
        QSet<FilePath> seedPaths;
        for (Project *project : SessionManager::projects()) {
            const FilePaths allFiles = project->files(Project::SourceFiles);
            for (const FilePath &filePath : allFiles )
                seedPaths.insert(filePath);
        }
        const QList<DocumentModel::Entry *> entries = DocumentModel::entries();
        for (DocumentModel::Entry *entry : entries) {
            if (entry)
                seedPaths.insert(entry->filePath());
        }
        CPlusPlus::Snapshot snapshot = CppModelManager::instance()->snapshot();
        setFileIterator(new CppIncludesIterator(snapshot, seedPaths));
    }
    BaseFileFilter::prepareSearch(entry);
}

void CppIncludesFilter::refresh(QFutureInterface<void> &future)
{
    Q_UNUSED(future)
    QMetaObject::invokeMethod(this, &CppIncludesFilter::markOutdated, Qt::QueuedConnection);
}

void CppIncludesFilter::markOutdated()
{
    m_needsUpdate = true;
    setFileIterator(nullptr); // clean up
}

} // namespace CppEditor::Internal
