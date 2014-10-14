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

#ifndef CLANGCOMPLETER_H
#define CLANGCOMPLETER_H

#include "clang_global.h"
#include "diagnostic.h"
#include "sourcelocation.h"
#include "utils.h"

#include <QList>
#include <QMap>
#include <QMutex>
#include <QPair>
#include <QSharedPointer>
#include <QString>
#include <QStringList>
#include <QVariant>

namespace ClangCodeModel {

class SourceMarker;

class CLANG_EXPORT CodeCompletionResult
{
public:
    enum Kind {
        Other = 0,
        FunctionCompletionKind,
        ConstructorCompletionKind,
        DestructorCompletionKind,
        VariableCompletionKind,
        ClassCompletionKind,
        EnumCompletionKind,
        EnumeratorCompletionKind,
        NamespaceCompletionKind,
        PreProcessorCompletionKind,
        SignalCompletionKind,
        SlotCompletionKind,
        ObjCMessageCompletionKind,
        KeywordCompletionKind,
        ClangSnippetKind
    };

    enum Availability {
        Available,
        Deprecated,
        NotAvailable,
        NotAccessible
    };

public:
    CodeCompletionResult();
    CodeCompletionResult(unsigned priority);

    unsigned priority() const
    { return m_priority; }

    bool isValid() const
    { return !m_text.isEmpty(); }

    QString text() const
    { return m_text; }
    void setText(const QString &text)
    { m_text = text; }

    QString hint() const
    { return m_hint; }
    void setHint(const QString &hint)
    { m_hint = hint; }

    QString snippet() const
    { return m_snippet; }
    void setSnippet(const QString &snippet)
    { m_snippet = snippet; }

    Kind completionKind() const
    { return m_completionKind; }
    void setCompletionKind(Kind type)
    { m_completionKind = type; }

    int compare(const CodeCompletionResult &other) const
    {
        if (m_priority < other.m_priority)
            return -1;
        else if (m_priority > other.m_priority)
            return 1;

        if (m_completionKind < other.m_completionKind)
            return -1;
        else if (m_completionKind > other.m_completionKind)
            return 1;

        if (m_text < other.m_text)
            return -1;
        else if (m_text > other.m_text)
            return 1;

        if (m_hint < other.m_hint)
            return -1;
        else if (m_hint > other.m_hint)
            return 1;

        if (!m_hasParameters && other.m_hasParameters)
            return -1;
        else if (m_hasParameters && !other.m_hasParameters)
            return 1;

        if (m_availability < other.m_availability)
            return -1;
        else if (m_availability > other.m_availability)
            return 1;

        return 0;
    }

    bool hasParameters() const
    { return m_hasParameters; }
    void setHasParameters(bool hasParameters)
    { m_hasParameters = hasParameters; }

    Availability availability() const
    { return m_availability; }
    void setAvailability(Availability availability)
    { m_availability = availability; }

private:
    unsigned m_priority;
    Kind m_completionKind;
    QString m_text;
    QString m_hint;
    QString m_snippet;
    Availability m_availability;
    bool m_hasParameters;
};

inline CLANG_EXPORT uint qHash(const CodeCompletionResult &ccr)
{ return ccr.completionKind() ^ qHash(ccr.text()); }

inline CLANG_EXPORT bool operator==(const CodeCompletionResult &ccr1, const CodeCompletionResult &ccr2)
{ return ccr1.compare(ccr2) == 0; }

inline CLANG_EXPORT bool operator<(const CodeCompletionResult &ccr1, const CodeCompletionResult &ccr2)
{
    return ccr1.compare(ccr2) < 0;
}

class CLANG_EXPORT ClangCompleter
{
    Q_DISABLE_COPY(ClangCompleter)

    class PrivateData;

public: // data structures
    typedef QSharedPointer<ClangCompleter> Ptr;

public: // methods
    ClangCompleter();
    ~ClangCompleter();

    QString fileName() const;
    void setFileName(const QString &fileName);

    QStringList options() const;
    void setOptions(const QStringList &options) const;

    bool isSignalSlotCompletion() const;
    void setSignalSlotCompletion(bool isSignalSlot);

    bool reparse(const Internal::UnsavedFiles &unsavedFiles);

    /**
     * Do code-completion at the specified position.
     *
     * \param line The line number on which to do code-completion. The first
     * line of a file has line number 1.
     * \param column The column number where to do code-completion. Column
     * numbers start with 1.
     */
    QList<CodeCompletionResult> codeCompleteAt(unsigned line,
                                               unsigned column,
                                               const Internal::UnsavedFiles &unsavedFiles);

    bool objcEnabled() const;

    QMutex *mutex() const;

private: // instance fields
    QScopedPointer<PrivateData> d;
};

} // namespace Clang

Q_DECLARE_METATYPE(ClangCodeModel::CodeCompletionResult)

#endif // CLANGCOMPLETER_H
