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
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef CPPSOURCEPROCESSOR_H
#define CPPSOURCEPROCESSOR_H

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

    void setDumpFileNameWhileParsing(bool onoff)
    { m_dumpFileNameWhileParsing = onoff; }

    void setRevision(unsigned revision);
    void setWorkingCopy(const CppTools::WorkingCopy &workingCopy);
    void setHeaderPaths(const ProjectPart::HeaderPaths &headerPaths);
    void setTodo(const QSet<QString> &files);

    void run(const QString &fileName, const QStringList &initialIncludes = QStringList());
    void removeFromCache(const QString &fileName);
    void resetEnvironment();

    CPlusPlus::Snapshot snapshot() const { return m_snapshot; }
    const QSet<QString> &todo() const { return m_todo; }

    void setGlobalSnapshot(const CPlusPlus::Snapshot &snapshot) { m_globalSnapshot = snapshot; }

private:
    void addFrameworkPath(const ProjectPart::HeaderPath &frameworkPath);

    CPlusPlus::Document::Ptr switchCurrentDocument(CPlusPlus::Document::Ptr doc);

    bool getFileContents(const QString &absoluteFilePath, QByteArray *contents,
                         unsigned *revision) const;
    bool checkFile(const QString &absoluteFilePath) const;
    QString resolveFile(const QString &fileName, IncludeType type);
    QString resolveFile_helper(const QString &fileName, IncludeType type);

    void mergeEnvironment(CPlusPlus::Document::Ptr doc);

    // Client interface
    void macroAdded(const CPlusPlus::Macro &macro) Q_DECL_OVERRIDE;
    void passedMacroDefinitionCheck(unsigned bytesOffset, unsigned utf16charsOffset,
                                    unsigned line, const CPlusPlus::Macro &macro) Q_DECL_OVERRIDE;
    void failedMacroDefinitionCheck(unsigned bytesOffset, unsigned utf16charOffset,
                                    const CPlusPlus::ByteArrayRef &name) Q_DECL_OVERRIDE;
    void notifyMacroReference(unsigned bytesOffset, unsigned utf16charOffset,
                              unsigned line, const CPlusPlus::Macro &macro) Q_DECL_OVERRIDE;
    void startExpandingMacro(unsigned bytesOffset, unsigned utf16charOffset,
                             unsigned line, const CPlusPlus::Macro &macro,
                             const QVector<CPlusPlus::MacroArgumentReference> &actuals) Q_DECL_OVERRIDE;
    void stopExpandingMacro(unsigned bytesOffset, const CPlusPlus::Macro &macro) Q_DECL_OVERRIDE;
    void markAsIncludeGuard(const QByteArray &macroName) Q_DECL_OVERRIDE;
    void startSkippingBlocks(unsigned utf16charsOffset) Q_DECL_OVERRIDE;
    void stopSkippingBlocks(unsigned utf16charsOffset) Q_DECL_OVERRIDE;
    void sourceNeeded(unsigned line, const QString &fileName, IncludeType type,
                      const QStringList &initialIncludes) Q_DECL_OVERRIDE;

private:
    CPlusPlus::Snapshot m_snapshot;
    CPlusPlus::Snapshot m_globalSnapshot;
    DocumentCallback m_documentFinished;
    bool m_dumpFileNameWhileParsing;
    CPlusPlus::Environment m_env;
    CPlusPlus::Preprocessor m_preprocess;
    ProjectPart::HeaderPaths m_headerPaths;
    CppTools::WorkingCopy m_workingCopy;
    QSet<QString> m_included;
    CPlusPlus::Document::Ptr m_currentDoc;
    QSet<QString> m_todo;
    QSet<QString> m_processed;
    unsigned m_revision;
    QHash<QString, QString> m_fileNameCache;
    QTextCodec *m_defaultCodec;
};

} // namespace Internal
} // namespace CppTools

#endif // CPPSOURCEPROCESSOR_H
