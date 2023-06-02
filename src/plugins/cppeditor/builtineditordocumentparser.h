// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "baseeditordocumentparser.h"
#include "cppeditor_global.h"

#include <cplusplus/CppDocument.h>

namespace CppEditor {

class CPPEDITOR_EXPORT BuiltinEditorDocumentParser : public BaseEditorDocumentParser
{
    Q_OBJECT

public:
    BuiltinEditorDocumentParser(const Utils::FilePath &filePath, int fileSizeLimitInMb = -1);

    bool releaseSourceAndAST() const;
    void setReleaseSourceAndAST(bool release);

    CPlusPlus::Document::Ptr document() const;
    CPlusPlus::Snapshot snapshot() const;
    ProjectExplorer::HeaderPaths headerPaths() const;

    void releaseResources();

signals:
    void finished(CPlusPlus::Document::Ptr document, CPlusPlus::Snapshot snapshot);

public:
    using Ptr = QSharedPointer<BuiltinEditorDocumentParser>;
    static Ptr get(const Utils::FilePath &filePath);

private:
    void updateImpl(const QPromise<void> &promise, const UpdateParams &updateParams) override;
    void addFileAndDependencies(CPlusPlus::Snapshot *snapshot,
                                QSet<Utils::FilePath> *toRemove,
                                const Utils::FilePath &fileName) const;

    struct ExtraState {
        QByteArray configFile;

        ProjectExplorer::HeaderPaths headerPaths;
        QString projectConfigFile;
        Utils::FilePaths includedFiles;
        Utils::FilePaths precompiledHeaders;

        CPlusPlus::Snapshot snapshot;
        bool forceSnapshotInvalidation = false;
    };
    ExtraState extraState() const;
    void setExtraState(const ExtraState &extraState);

    bool m_releaseSourceAndAST = true;
    ExtraState m_extraState;

    const int m_fileSizeLimitInMb = -1;
};

} // CppEditor
