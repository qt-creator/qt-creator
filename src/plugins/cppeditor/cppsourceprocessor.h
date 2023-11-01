// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "cppmodelmanager.h"
#include "cppworkingcopy.h"

#include <cplusplus/PreprocessorEnvironment.h>
#include <cplusplus/pp-engine.h>

#include <QHash>
#include <QPointer>
#include <QSet>
#include <QStringList>

#include <functional>

QT_BEGIN_NAMESPACE
class QTextCodec;
QT_END_NAMESPACE

namespace CppEditor::Internal {

// Documentation inside.
class CppSourceProcessor: public CPlusPlus::Client
{
    Q_DISABLE_COPY(CppSourceProcessor)

public:
    using DocumentCallback = std::function<void (const CPlusPlus::Document::Ptr &)>;

public:
    CppSourceProcessor(const CPlusPlus::Snapshot &snapshot, DocumentCallback documentFinished);
    ~CppSourceProcessor() override;

    using CancelChecker = std::function<bool()>;
    void setCancelChecker(const CancelChecker &cancelChecker);

    void setWorkingCopy(const WorkingCopy &workingCopy);
    void setHeaderPaths(const ProjectExplorer::HeaderPaths &headerPaths);
    void setLanguageFeatures(CPlusPlus::LanguageFeatures languageFeatures);
    void setFileSizeLimitInMb(int fileSizeLimitInMb);
    void setTodo(const QSet<QString> &files);

    void run(const Utils::FilePath &filePath, const Utils::FilePaths &initialIncludes = {});
    void removeFromCache(const Utils::FilePath &filePath);
    void resetEnvironment();

    CPlusPlus::Snapshot snapshot() const { return m_snapshot; }
    const QSet<QString> &todo() const { return m_todo; }

    void setGlobalSnapshot(const CPlusPlus::Snapshot &snapshot) { m_globalSnapshot = snapshot; }

private:
    void addFrameworkPath(const ProjectExplorer::HeaderPath &frameworkPath);

    CPlusPlus::Document::Ptr switchCurrentDocument(CPlusPlus::Document::Ptr doc);

    bool getFileContents(const Utils::FilePath &absoluteFilePath, QByteArray *contents,
                         unsigned *revision) const;
    bool checkFile(const Utils::FilePath &absoluteFilePath) const;
    Utils::FilePath resolveFile(const Utils::FilePath &filePath, IncludeType type);
    Utils::FilePath resolveFile_helper(const Utils::FilePath &filePath,
                                       ProjectExplorer::HeaderPaths::Iterator headerPathsIt);

    void mergeEnvironment(CPlusPlus::Document::Ptr doc);

    // Client interface
    void macroAdded(const CPlusPlus::Macro &macro) override;
    void passedMacroDefinitionCheck(int bytesOffset, int utf16charsOffset,
                                    int line, const CPlusPlus::Macro &macro) override;
    void failedMacroDefinitionCheck(int bytesOffset, int utf16charOffset,
                                    const CPlusPlus::ByteArrayRef &name) override;
    void notifyMacroReference(int bytesOffset, int utf16charOffset,
                              int line, const CPlusPlus::Macro &macro) override;
    void startExpandingMacro(int bytesOffset, int utf16charOffset,
                             int line, const CPlusPlus::Macro &macro,
                             const QVector<CPlusPlus::MacroArgumentReference> &actuals) override;
    void stopExpandingMacro(int bytesOffset, const CPlusPlus::Macro &macro) override;
    void markAsIncludeGuard(const QByteArray &macroName) override;
    void startSkippingBlocks(int utf16charsOffset) override;
    void stopSkippingBlocks(int utf16charsOffset) override;
    void sourceNeeded(int line, const Utils::FilePath &filePath, IncludeType type,
                      const Utils::FilePaths &initialIncludes) override;

private:
    CPlusPlus::Snapshot m_snapshot;
    CPlusPlus::Snapshot m_globalSnapshot;
    DocumentCallback m_documentFinished;
    CPlusPlus::Environment m_env;
    CPlusPlus::Preprocessor m_preprocess;
    ProjectExplorer::HeaderPaths m_headerPaths;
    CPlusPlus::LanguageFeatures m_languageFeatures;
    WorkingCopy m_workingCopy;
    QSet<Utils::FilePath> m_included;
    CPlusPlus::Document::Ptr m_currentDoc;
    QSet<QString> m_todo;
    QSet<Utils::FilePath> m_processed;
    QHash<Utils::FilePath, Utils::FilePath> m_fileNameCache;
    int m_fileSizeLimitInMb = -1;
    QTextCodec *m_defaultCodec;
};

} // CppEditor::Internal
