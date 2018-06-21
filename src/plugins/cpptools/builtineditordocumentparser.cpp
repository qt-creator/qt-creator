/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "builtineditordocumentparser.h"
#include "cppsourceprocessor.h"

#include <projectexplorer/projectmacro.h>
#include <projectexplorer/projectexplorerconstants.h>

#include <utils/qtcassert.h>

using namespace CPlusPlus;
using namespace CppTools;
using namespace CppTools::Internal;

static QByteArray overwrittenToolchainDefines(const ProjectPart &projectPart)
{
    QByteArray defines;

    // MSVC's predefined macros like __FUNCSIG__ expand to itself.
    // We can't parse this, so redefine to the empty string literal.
    if (projectPart.toolchainType == ProjectExplorer::Constants::MSVC_TOOLCHAIN_TYPEID) {
        defines += "#define __FUNCSIG__ \"\"\n"
                   "#define __FUNCDNAME__ \"\"\n"
                   "#define __FUNCTION__ \"\"\n";
    }

    return defines;
}

BuiltinEditorDocumentParser::BuiltinEditorDocumentParser(const QString &filePath)
    : BaseEditorDocumentParser(filePath)
{
    qRegisterMetaType<CPlusPlus::Snapshot>("CPlusPlus::Snapshot");
}

void BuiltinEditorDocumentParser::updateImpl(const QFutureInterface<void> &future,
                                             const UpdateParams &updateParams)
{
    if (filePath().isEmpty())
        return;

    const Configuration baseConfig = configuration();
    const bool releaseSourceAndAST_ = releaseSourceAndAST();

    State baseState = state();
    ExtraState state = extraState();
    WorkingCopy workingCopy = updateParams.workingCopy;

    bool invalidateSnapshot = false, invalidateConfig = false, editorDefinesChanged_ = false;

    CppModelManager *modelManager = CppModelManager::instance();
    QByteArray configFile = modelManager->codeModelConfiguration();
    ProjectPartHeaderPaths headerPaths;
    QStringList precompiledHeaders;
    QString projectConfigFile;
    LanguageFeatures features = LanguageFeatures::defaultFeatures();

    baseState.projectPartInfo = determineProjectPart(filePath(),
                                                    baseConfig.preferredProjectPartId,
                                                    baseState.projectPartInfo,
                                                    updateParams.activeProject,
                                                    updateParams.languagePreference,
                                                    updateParams.projectsUpdated);
    emit projectPartInfoUpdated(baseState.projectPartInfo);

    if (state.forceSnapshotInvalidation) {
        invalidateSnapshot = true;
        state.forceSnapshotInvalidation = false;
    }

    if (const ProjectPart::Ptr part = baseState.projectPartInfo.projectPart) {
        configFile += ProjectExplorer::Macro::toByteArray(part->toolChainMacros);
        configFile += overwrittenToolchainDefines(*part.data());
        configFile += ProjectExplorer::Macro::toByteArray(part->projectMacros);
        if (!part->projectConfigFile.isEmpty())
            configFile += ProjectPart::readProjectConfigFile(part);
        headerPaths = part->headerPaths;
        projectConfigFile = part->projectConfigFile;
        if (baseConfig.usePrecompiledHeaders)
            precompiledHeaders = part->precompiledHeaders;
        features = part->languageFeatures;
    }

    if (configFile != state.configFile) {
        state.configFile = configFile;
        invalidateSnapshot = true;
        invalidateConfig = true;
    }

    if (baseConfig.editorDefines != baseState.editorDefines) {
        baseState.editorDefines = baseConfig.editorDefines;
        invalidateSnapshot = true;
        editorDefinesChanged_ = true;
    }

    if (headerPaths != state.headerPaths) {
        state.headerPaths = headerPaths;
        invalidateSnapshot = true;
    }

    if (projectConfigFile != state.projectConfigFile) {
        state.projectConfigFile = projectConfigFile;
        invalidateSnapshot = true;
    }

    if (precompiledHeaders != state.precompiledHeaders) {
        state.precompiledHeaders = precompiledHeaders;
        invalidateSnapshot = true;
    }

    unsigned rev = 0;
    if (Document::Ptr doc = state.snapshot.document(filePath()))
        rev = doc->revision();
    else
        invalidateSnapshot = true;

    Snapshot globalSnapshot = modelManager->snapshot();

    if (invalidateSnapshot) {
        state.snapshot = Snapshot();
    } else {
        // Remove changed files from the snapshot
        QSet<Utils::FileName> toRemove;
        foreach (const Document::Ptr &doc, state.snapshot) {
            const Utils::FileName fileName = Utils::FileName::fromString(doc->fileName());
            if (workingCopy.contains(fileName)) {
                if (workingCopy.get(fileName).second != doc->editorRevision())
                    addFileAndDependencies(&state.snapshot, &toRemove, fileName);
                continue;
            }
            Document::Ptr otherDoc = globalSnapshot.document(fileName);
            if (!otherDoc.isNull() && otherDoc->revision() != doc->revision())
                addFileAndDependencies(&state.snapshot, &toRemove, fileName);
        }

        if (!toRemove.isEmpty()) {
            invalidateSnapshot = true;
            foreach (const Utils::FileName &fileName, toRemove)
                state.snapshot.remove(fileName);
        }
    }

    // Update the snapshot
    if (invalidateSnapshot) {
        const QString configurationFileName = modelManager->configurationFileName();
        if (invalidateConfig)
            state.snapshot.remove(configurationFileName);
        if (!state.snapshot.contains(configurationFileName))
            workingCopy.insert(configurationFileName, state.configFile);
        state.snapshot.remove(filePath());

        static const QString editorDefinesFileName
            = CppModelManager::editorConfigurationFileName();
        if (editorDefinesChanged_) {
            state.snapshot.remove(editorDefinesFileName);
            workingCopy.insert(editorDefinesFileName, baseState.editorDefines);
        }

        CppSourceProcessor sourceProcessor(state.snapshot, [&](const Document::Ptr &doc) {
            const QString fileName = doc->fileName();
            const bool isInEditor = fileName == filePath();
            Document::Ptr otherDoc = modelManager->document(fileName);
            unsigned newRev = otherDoc.isNull() ? 1U : otherDoc->revision() + 1;
            if (isInEditor)
                newRev = qMax(rev + 1, newRev);
            doc->setRevision(newRev);
            modelManager->emitDocumentUpdated(doc);
            if (releaseSourceAndAST_)
                doc->releaseSourceAndAST();
        });
        sourceProcessor.setCancelChecker([future]() {
           return future.isCanceled();
        });

        Snapshot globalSnapshot = modelManager->snapshot();
        globalSnapshot.remove(filePath());
        sourceProcessor.setGlobalSnapshot(globalSnapshot);
        sourceProcessor.setWorkingCopy(workingCopy);
        sourceProcessor.setHeaderPaths(state.headerPaths);
        sourceProcessor.setLanguageFeatures(features);
        sourceProcessor.run(configurationFileName);
        if (baseConfig.usePrecompiledHeaders) {
            foreach (const QString &precompiledHeader, state.precompiledHeaders)
                sourceProcessor.run(precompiledHeader);
        }
        if (!baseState.editorDefines.isEmpty())
            sourceProcessor.run(editorDefinesFileName);
        sourceProcessor.run(filePath(), baseConfig.usePrecompiledHeaders ? state.precompiledHeaders
                                                                         : QStringList());
        state.snapshot = sourceProcessor.snapshot();
        Snapshot newSnapshot = state.snapshot.simplified(state.snapshot.document(filePath()));
        for (Snapshot::const_iterator i = state.snapshot.begin(), ei = state.snapshot.end(); i != ei; ++i) {
            if (Client::isInjectedFile(i.key().toString()))
                newSnapshot.insert(i.value());
        }
        state.snapshot = newSnapshot;
        state.snapshot.updateDependencyTable();
    }

    setState(baseState);
    setExtraState(state);

    if (invalidateSnapshot)
        emit finished(state.snapshot.document(filePath()), state.snapshot);
}

void BuiltinEditorDocumentParser::releaseResources()
{
    ExtraState s = extraState();
    s.snapshot = Snapshot();
    s.forceSnapshotInvalidation = true;
    setExtraState(s);
}

Document::Ptr BuiltinEditorDocumentParser::document() const
{
    return extraState().snapshot.document(filePath());
}

Snapshot BuiltinEditorDocumentParser::snapshot() const
{
    return extraState().snapshot;
}

ProjectPartHeaderPaths BuiltinEditorDocumentParser::headerPaths() const
{
    return extraState().headerPaths;
}

BuiltinEditorDocumentParser::Ptr BuiltinEditorDocumentParser::get(const QString &filePath)
{
    if (BaseEditorDocumentParser::Ptr b = BaseEditorDocumentParser::get(filePath))
        return b.objectCast<BuiltinEditorDocumentParser>();
    return BuiltinEditorDocumentParser::Ptr();
}

void BuiltinEditorDocumentParser::addFileAndDependencies(Snapshot *snapshot,
                                                         QSet<Utils::FileName> *toRemove,
                                                         const Utils::FileName &fileName) const
{
    QTC_ASSERT(snapshot, return);

    toRemove->insert(fileName);
    if (fileName != Utils::FileName::fromString(filePath())) {
        Utils::FileNameList deps = snapshot->filesDependingOn(fileName);
        toRemove->unite(QSet<Utils::FileName>::fromList(deps));
    }
}

BuiltinEditorDocumentParser::ExtraState BuiltinEditorDocumentParser::extraState() const
{
    QMutexLocker locker(&m_stateAndConfigurationMutex);
    return m_extraState;
}

void BuiltinEditorDocumentParser::setExtraState(const ExtraState &extraState)
{
    QMutexLocker locker(&m_stateAndConfigurationMutex);
    m_extraState = extraState;
}

bool BuiltinEditorDocumentParser::releaseSourceAndAST() const
{
    QMutexLocker locker(&m_stateAndConfigurationMutex);
    return m_releaseSourceAndAST;
}

void BuiltinEditorDocumentParser::setReleaseSourceAndAST(bool release)
{
    QMutexLocker locker(&m_stateAndConfigurationMutex);
    m_releaseSourceAndAST = release;
}
