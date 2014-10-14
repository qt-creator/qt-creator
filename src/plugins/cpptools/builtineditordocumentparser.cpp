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

#include "builtineditordocumentparser.h"
#include "cppsourceprocessor.h"
#include "editordocumenthandle.h"

#include <utils/qtcassert.h>

using namespace CPlusPlus;
using namespace CppTools;
using namespace CppTools::Internal;

BuiltinEditorDocumentParser::BuiltinEditorDocumentParser(const QString &filePath)
    : BaseEditorDocumentParser(filePath)
    , m_forceSnapshotInvalidation(false)
    , m_releaseSourceAndAST(true)
{
    qRegisterMetaType<CPlusPlus::Snapshot>("CPlusPlus::Snapshot");
}

void BuiltinEditorDocumentParser::update(WorkingCopy workingCopy)
{
    QMutexLocker locker(&m_mutex);

    if (filePath().isEmpty())
        return;

    bool invalidateSnapshot = false, invalidateConfig = false, editorDefinesChanged_ = false;

    CppModelManager *modelManager = CppModelManager::instance();
    QByteArray configFile = modelManager->codeModelConfiguration();
    ProjectPart::HeaderPaths headerPaths;
    QStringList precompiledHeaders;
    QString projectConfigFile;

    updateProjectPart();

    if (m_forceSnapshotInvalidation) {
        invalidateSnapshot = true;
        m_forceSnapshotInvalidation = false;
    }

    if (const ProjectPart::Ptr part = projectPart()) {
        configFile += part->toolchainDefines;
        configFile += part->projectDefines;
        headerPaths = part->headerPaths;
        projectConfigFile = part->projectConfigFile;
        if (usePrecompiledHeaders())
            precompiledHeaders = part->precompiledHeaders;
    }

    if (configFile != m_configFile) {
        m_configFile = configFile;
        invalidateSnapshot = true;
        invalidateConfig = true;
    }

    if (editorDefinesChanged()) {
        invalidateSnapshot = true;
        editorDefinesChanged_ = true;
        resetEditorDefinesChanged();
    }

    if (headerPaths != m_headerPaths) {
        m_headerPaths = headerPaths;
        invalidateSnapshot = true;
    }

    if (projectConfigFile != m_projectConfigFile) {
        m_projectConfigFile = projectConfigFile;
        invalidateSnapshot = true;
    }

    if (precompiledHeaders != m_precompiledHeaders) {
        m_precompiledHeaders = precompiledHeaders;
        invalidateSnapshot = true;
    }

    unsigned rev = 0;
    if (Document::Ptr doc = document())
        rev = doc->revision();
    else
        invalidateSnapshot = true;

    Snapshot globalSnapshot = modelManager->snapshot();

    if (invalidateSnapshot) {
        m_snapshot = Snapshot();
    } else {
        // Remove changed files from the snapshot
        QSet<QString> toRemove;
        foreach (const Document::Ptr &doc, m_snapshot) {
            QString fileName = doc->fileName();
            if (workingCopy.contains(fileName)) {
                if (workingCopy.get(fileName).second != doc->editorRevision())
                    addFileAndDependencies(&toRemove, fileName);
                continue;
            }
            Document::Ptr otherDoc = globalSnapshot.document(fileName);
            if (!otherDoc.isNull() && otherDoc->revision() != doc->revision())
                addFileAndDependencies(&toRemove, fileName);
        }

        if (!toRemove.isEmpty()) {
            invalidateSnapshot = true;
            foreach (const QString &fileName, toRemove)
                m_snapshot.remove(fileName);
        }
    }

    // Update the snapshot
    if (invalidateSnapshot) {
        const QString configurationFileName = modelManager->configurationFileName();
        if (invalidateConfig)
            m_snapshot.remove(configurationFileName);
        if (!m_snapshot.contains(configurationFileName))
            workingCopy.insert(configurationFileName, m_configFile);
        m_snapshot.remove(filePath());

        static const QString editorDefinesFileName
            = CppModelManager::editorConfigurationFileName();
        if (editorDefinesChanged_) {
            m_snapshot.remove(editorDefinesFileName);
            workingCopy.insert(editorDefinesFileName, editorDefines());
        }

        CppSourceProcessor sourceProcessor(m_snapshot, [&](const Document::Ptr &doc) {
            const QString fileName = doc->fileName();
            const bool isInEditor = fileName == filePath();
            Document::Ptr otherDoc = modelManager->document(fileName);
            unsigned newRev = otherDoc.isNull() ? 1U : otherDoc->revision() + 1;
            if (isInEditor)
                newRev = qMax(rev + 1, newRev);
            doc->setRevision(newRev);
            modelManager->emitDocumentUpdated(doc);
            if (m_releaseSourceAndAST)
                doc->releaseSourceAndAST();
        });
        Snapshot globalSnapshot = modelManager->snapshot();
        globalSnapshot.remove(filePath());
        sourceProcessor.setGlobalSnapshot(globalSnapshot);
        sourceProcessor.setWorkingCopy(workingCopy);
        sourceProcessor.setHeaderPaths(m_headerPaths);
        sourceProcessor.run(configurationFileName);
        if (!m_projectConfigFile.isEmpty())
            sourceProcessor.run(m_projectConfigFile);
        if (usePrecompiledHeaders()) {
            foreach (const QString &precompiledHeader, m_precompiledHeaders)
                sourceProcessor.run(precompiledHeader);
        }
        if (!editorDefines().isEmpty())
            sourceProcessor.run(editorDefinesFileName);
        sourceProcessor.run(filePath(), usePrecompiledHeaders() ? m_precompiledHeaders
                                                                    : QStringList());
        m_snapshot = sourceProcessor.snapshot();
        Snapshot newSnapshot = m_snapshot.simplified(document());
        for (Snapshot::const_iterator i = m_snapshot.begin(), ei = m_snapshot.end(); i != ei; ++i) {
            if (Client::isInjectedFile(i.key()))
                newSnapshot.insert(i.value());
        }
        m_snapshot = newSnapshot;
        m_snapshot.updateDependencyTable();

        emit finished(document(), m_snapshot);
    }
}

void BuiltinEditorDocumentParser::releaseResources()
{
    QMutexLocker locker(&m_mutex);
    m_snapshot = Snapshot();
    m_forceSnapshotInvalidation = true;
}

Document::Ptr BuiltinEditorDocumentParser::document() const
{
    QMutexLocker locker(&m_mutex);
    return m_snapshot.document(filePath());
}

Snapshot BuiltinEditorDocumentParser::snapshot() const
{
    QMutexLocker locker(&m_mutex);
    return m_snapshot;
}

ProjectPart::HeaderPaths BuiltinEditorDocumentParser::headerPaths() const
{
    QMutexLocker locker(&m_mutex);
    return m_headerPaths;
}

void BuiltinEditorDocumentParser::setReleaseSourceAndAST(bool onoff)
{
    QMutexLocker locker(&m_mutex);
    m_releaseSourceAndAST = onoff;
}

BuiltinEditorDocumentParser *BuiltinEditorDocumentParser::get(const QString &filePath)
{
    if (BaseEditorDocumentParser *b = BaseEditorDocumentParser::get(filePath))
        return qobject_cast<BuiltinEditorDocumentParser *>(b);
    return 0;
}

void BuiltinEditorDocumentParser::addFileAndDependencies(QSet<QString> *toRemove,
                                                         const QString &fileName) const
{
    toRemove->insert(fileName);
    if (fileName != filePath()) {
        QStringList deps = m_snapshot.filesDependingOn(fileName);
        toRemove->unite(QSet<QString>::fromList(deps));
    }
}
