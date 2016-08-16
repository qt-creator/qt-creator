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

namespace CppTools {
namespace Internal {

// Documentation inside.
class CppSourceProcessor: public CPlusPlus::Client
{
    Q_DISABLE_COPY(CppSourceProcessor)

public:
    typedef std::function<void (const CPlusPlus::Document::Ptr &)> DocumentCallback;

public:
    static QString cleanPath(const QString &path);

    CppSourceProcessor(const CPlusPlus::Snapshot &snapshot, DocumentCallback documentFinished);
    ~CppSourceProcessor();

    using CancelChecker = std::function<bool()>;
    void setCancelChecker(const CancelChecker &cancelChecker);

    void setWorkingCopy(const CppTools::WorkingCopy &workingCopy);
    void setHeaderPaths(const ProjectPartHeaderPaths &headerPaths);
    void setLanguageFeatures(CPlusPlus::LanguageFeatures languageFeatures);
    void setFileSizeLimitInMb(int fileSizeLimitInMb);
    void setTodo(const QSet<QString> &files);

    void run(const QString &fileName, const QStringList &initialIncludes = QStringList());
    void removeFromCache(const QString &fileName);
    void resetEnvironment();

    CPlusPlus::Snapshot snapshot() const { return m_snapshot; }
    const QSet<QString> &todo() const { return m_todo; }

    void setGlobalSnapshot(const CPlusPlus::Snapshot &snapshot) { m_globalSnapshot = snapshot; }

private:
    void addFrameworkPath(const ProjectPartHeaderPath &frameworkPath);

    CPlusPlus::Document::Ptr switchCurrentDocument(CPlusPlus::Document::Ptr doc);

    bool getFileContents(const QString &absoluteFilePath, QByteArray *contents,
                         unsigned *revision) const;
    bool checkFile(const QString &absoluteFilePath) const;
    QString resolveFile(const QString &fileName, IncludeType type);
    QString resolveFile_helper(const QString &fileName,
                               ProjectPartHeaderPaths::Iterator headerPathsIt);

    void mergeEnvironment(CPlusPlus::Document::Ptr doc);

    // Client interface
    void macroAdded(const CPlusPlus::Macro &macro) override;
    void passedMacroDefinitionCheck(unsigned bytesOffset, unsigned utf16charsOffset,
                                    unsigned line, const CPlusPlus::Macro &macro) override;
    void failedMacroDefinitionCheck(unsigned bytesOffset, unsigned utf16charOffset,
                                    const CPlusPlus::ByteArrayRef &name) override;
    void notifyMacroReference(unsigned bytesOffset, unsigned utf16charOffset,
                              unsigned line, const CPlusPlus::Macro &macro) override;
    void startExpandingMacro(unsigned bytesOffset, unsigned utf16charOffset,
                             unsigned line, const CPlusPlus::Macro &macro,
                             const QVector<CPlusPlus::MacroArgumentReference> &actuals) override;
    void stopExpandingMacro(unsigned bytesOffset, const CPlusPlus::Macro &macro) override;
    void markAsIncludeGuard(const QByteArray &macroName) override;
    void startSkippingBlocks(unsigned utf16charsOffset) override;
    void stopSkippingBlocks(unsigned utf16charsOffset) override;
    void sourceNeeded(unsigned line, const QString &fileName, IncludeType type,
                      const QStringList &initialIncludes) override;

private:
    CPlusPlus::Snapshot m_snapshot;
    CPlusPlus::Snapshot m_globalSnapshot;
    DocumentCallback m_documentFinished;
    CPlusPlus::Environment m_env;
    CPlusPlus::Preprocessor m_preprocess;
    ProjectPartHeaderPaths m_headerPaths;
    CPlusPlus::LanguageFeatures m_languageFeatures;
    CppTools::WorkingCopy m_workingCopy;
    QSet<QString> m_included;
    CPlusPlus::Document::Ptr m_currentDoc;
    QSet<QString> m_todo;
    QSet<QString> m_processed;
    QHash<QString, QString> m_fileNameCache;
    int m_fileSizeLimitInMb = -1;
    QTextCodec *m_defaultCodec;
};

} // namespace Internal
} // namespace CppTools
