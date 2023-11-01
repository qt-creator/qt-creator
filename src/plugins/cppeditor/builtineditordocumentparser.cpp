// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "builtineditordocumentparser.h"

#include "cppsourceprocessor.h"

#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/projectmacro.h>

#include <utils/algorithm.h>
#include <utils/qtcassert.h>

#include <QPromise>

using namespace CPlusPlus;
using namespace Utils;

namespace CppEditor {

static QByteArray overwrittenToolchainDefines(const ProjectPart &projectPart)
{
    QByteArray defines;

    // MSVC's predefined macros like __FUNCSIG__ expand to itself.
    // We can't parse this, so redefine to the empty string literal.
    if (projectPart.toolchainType == ProjectExplorer::Constants::MSVC_TOOLCHAIN_TYPEID) {
        defines += "#define __FUNCSIG__ \"void __cdecl "
                   "someLegalAndLongishFunctionNameThatWorksAroundQTCREATORBUG-24580(void)\"\n"
                   "#define __FUNCDNAME__ "
                   "\"?someLegalAndLongishFunctionNameThatWorksAroundQTCREATORBUG-24580@@YAXXZ\"\n"
                   "#define __FUNCTION__ "
                   "\"someLegalAndLongishFunctionNameThatWorksAroundQTCREATORBUG-24580\"\n";
    }

    return defines;
}

BuiltinEditorDocumentParser::BuiltinEditorDocumentParser(const FilePath &filePath,
                                                         int fileSizeLimitInMb)
    : BaseEditorDocumentParser(filePath)
    , m_fileSizeLimitInMb(fileSizeLimitInMb)
{
    qRegisterMetaType<CPlusPlus::Snapshot>("CPlusPlus::Snapshot");
}

void BuiltinEditorDocumentParser::updateImpl(const QPromise<void> &promise,
                                             const UpdateParams &updateParams)
{
    if (filePath().isEmpty())
        return;

    const Configuration baseConfig = configuration();
    const bool releaseSourceAndAST_ = releaseSourceAndAST();

    State baseState = state();
    ExtraState state = extraState();
    WorkingCopy workingCopy = updateParams.workingCopy;

    bool invalidateSnapshot = false, invalidateConfig = false;

    QByteArray configFile = CppModelManager::codeModelConfiguration();
    ProjectExplorer::HeaderPaths headerPaths;
    FilePaths includedFiles;
    FilePaths precompiledHeaders;
    QString projectConfigFile;
    LanguageFeatures features = LanguageFeatures::defaultFeatures();

    baseState.projectPartInfo = determineProjectPart(filePath().toString(),
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

    if (const ProjectPart::ConstPtr part = baseState.projectPartInfo.projectPart) {
        configFile += ProjectExplorer::Macro::toByteArray(part->toolChainMacros);
        configFile += overwrittenToolchainDefines(*part.data());
        configFile += ProjectExplorer::Macro::toByteArray(part->projectMacros);
        if (!part->projectConfigFile.isEmpty())
            configFile += ProjectPart::readProjectConfigFile(part->projectConfigFile);
        headerPaths = part->headerPaths;
        projectConfigFile = part->projectConfigFile;
        includedFiles = Utils::transform(part->includedFiles, &FilePath::fromString);
        if (baseConfig.usePrecompiledHeaders)
            precompiledHeaders = Utils::transform(part->precompiledHeaders, &FilePath::fromString);
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
    }

    if (headerPaths != state.headerPaths) {
        state.headerPaths = headerPaths;
        invalidateSnapshot = true;
    }

    if (projectConfigFile != state.projectConfigFile) {
        state.projectConfigFile = projectConfigFile;
        invalidateSnapshot = true;
    }

    if (includedFiles != state.includedFiles) {
        state.includedFiles = includedFiles;
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

    Snapshot globalSnapshot = CppModelManager::snapshot();

    if (invalidateSnapshot) {
        state.snapshot = Snapshot();
        if (!baseState.editorDefines.isEmpty()) {
            workingCopy.insert(CppModelManager::editorConfigurationFileName(),
                               baseState.editorDefines);
        }
    } else {
        // Remove changed files from the snapshot
        QSet<Utils::FilePath> toRemove;
        for (const Document::Ptr &doc : std::as_const(state.snapshot)) {
            const Utils::FilePath filePath = doc->filePath();
            if (const auto entry = workingCopy.get(filePath)) {
                if (entry->second != doc->editorRevision())
                    addFileAndDependencies(&state.snapshot, &toRemove, filePath);
                continue;
            }
            Document::Ptr otherDoc = globalSnapshot.document(filePath);
            if (!otherDoc.isNull() && otherDoc->revision() != doc->revision())
                addFileAndDependencies(&state.snapshot, &toRemove, filePath);
        }

        if (!toRemove.isEmpty()) {
            invalidateSnapshot = true;
            for (const Utils::FilePath &fileName : std::as_const(toRemove))
                state.snapshot.remove(fileName);
        }
    }

    // Update the snapshot
    if (invalidateSnapshot) {
        const FilePath configurationFileName = CppModelManager::configurationFileName();
        if (invalidateConfig)
            state.snapshot.remove(configurationFileName);
        if (!state.snapshot.contains(configurationFileName))
            workingCopy.insert(configurationFileName, state.configFile);
        state.snapshot.remove(filePath());

        Internal::CppSourceProcessor sourceProcessor(state.snapshot, [&](const Document::Ptr &doc) {
            const bool isInEditor = doc->filePath() == filePath();
            Document::Ptr otherDoc = CppModelManager::document(doc->filePath());
            unsigned newRev = otherDoc.isNull() ? 1U : otherDoc->revision() + 1;
            if (isInEditor)
                newRev = qMax(rev + 1, newRev);
            doc->setRevision(newRev);
            CppModelManager::emitDocumentUpdated(doc);
            if (releaseSourceAndAST_)
                doc->releaseSourceAndAST();
        });
        sourceProcessor.setFileSizeLimitInMb(m_fileSizeLimitInMb);
        sourceProcessor.setCancelChecker([&promise] { return promise.isCanceled(); });

        Snapshot globalSnapshot = CppModelManager::snapshot();
        globalSnapshot.remove(filePath());
        sourceProcessor.setGlobalSnapshot(globalSnapshot);
        sourceProcessor.setWorkingCopy(workingCopy);
        sourceProcessor.setHeaderPaths(state.headerPaths);
        sourceProcessor.setLanguageFeatures(features);
        sourceProcessor.run(configurationFileName);
        if (baseConfig.usePrecompiledHeaders) {
            for (const FilePath &precompiledHeader : std::as_const(state.precompiledHeaders))
                sourceProcessor.run(precompiledHeader);
        }
        if (!baseState.editorDefines.isEmpty())
            sourceProcessor.run(CppModelManager::editorConfigurationFileName());
        FilePaths includedFiles = state.includedFiles;
        if (baseConfig.usePrecompiledHeaders)
            includedFiles << state.precompiledHeaders;
        FilePath::removeDuplicates(includedFiles);
        sourceProcessor.run(filePath(), includedFiles);
        state.snapshot = sourceProcessor.snapshot();
        Snapshot newSnapshot = state.snapshot.simplified(state.snapshot.document(filePath()));
        for (Snapshot::const_iterator i = state.snapshot.begin(), ei = state.snapshot.end();
             i != ei;
             ++i) {
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

ProjectExplorer::HeaderPaths BuiltinEditorDocumentParser::headerPaths() const
{
    return extraState().headerPaths;
}

BuiltinEditorDocumentParser::Ptr BuiltinEditorDocumentParser::get(const FilePath &filePath)
{
    if (BaseEditorDocumentParser::Ptr b = BaseEditorDocumentParser::get(filePath))
        return b.objectCast<BuiltinEditorDocumentParser>();
    return BuiltinEditorDocumentParser::Ptr();
}

void BuiltinEditorDocumentParser::addFileAndDependencies(Snapshot *snapshot,
                                                         QSet<Utils::FilePath> *toRemove,
                                                         const Utils::FilePath &fileName) const
{
    QTC_ASSERT(snapshot, return);

    toRemove->insert(fileName);
    if (fileName != filePath()) {
        Utils::FilePaths deps = snapshot->filesDependingOn(fileName);
        toRemove->unite(Utils::toSet(deps));
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

} // namespace CppEditor
