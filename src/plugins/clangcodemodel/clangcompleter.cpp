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

#include "clangcompleter.h"
#include "sourcemarker.h"
#include "unsavedfiledata.h"
#include "utils_p.h"
#include "completionproposalsbuilder.h"
#include "raii/scopedclangoptions.h"
#include "unit.h"

#include <QDebug>
#include <QFile>
#include <QMutex>
#include <QMutexLocker>
#include <QTime>

#include <clang-c/Index.h>

//#define TIME_COMPLETION

class ClangCodeModel::ClangCompleter::PrivateData
{
public:
    PrivateData()
        : m_mutex(QMutex::Recursive)
        , m_unit(Internal::Unit::create())
        , m_isSignalSlotCompletion(false)
    {
    }

    ~PrivateData()
    {
    }

    bool parseFromFile(const Internal::UnsavedFiles &unsavedFiles)
    {
        Q_ASSERT(!m_unit->isLoaded());
        if (m_unit->fileName().isEmpty())
            return false;

        unsigned opts = clang_defaultEditingTranslationUnitOptions();
#if defined(CINDEX_VERSION) && (CINDEX_VERSION > 5)
        opts |= CXTranslationUnit_CacheCompletionResults;
        opts |= CXTranslationUnit_IncludeBriefCommentsInCodeCompletion;
#endif
        m_unit->setManagementOptions(opts);

        m_unit->setUnsavedFiles(unsavedFiles);
        m_unit->parse();
        return m_unit->isLoaded();
    }

public:
    QMutex m_mutex;
    Internal::Unit::Ptr m_unit;
    bool m_isSignalSlotCompletion;
};

using namespace ClangCodeModel;
using namespace ClangCodeModel::Internal;

/**
 * @brief Constructs with highest possible priority
 */
CodeCompletionResult::CodeCompletionResult()
    : m_priority(SHRT_MAX)
    , m_completionKind(Other)
    , m_availability(Available)
    , m_hasParameters(false)
{}

/**
 * @brief Constructs with given priority
 * @param priority Will be reversed, because clang's highest priority is 0,
 * but inside QtCreator it is the lowest priority
 */
CodeCompletionResult::CodeCompletionResult(unsigned priority)
    : m_priority(SHRT_MAX - priority)
    , m_completionKind(Other)
    , m_availability(Available)
    , m_hasParameters(false)
{
}

ClangCompleter::ClangCompleter()
    : d(new PrivateData)
{
}

ClangCompleter::~ClangCompleter()
{
}

QString ClangCompleter::fileName() const
{
    return d->m_unit->fileName();
}

void ClangCompleter::setFileName(const QString &fileName)
{
    if (d->m_unit->fileName() != fileName) {
        d->m_unit = Internal::Unit::create(fileName);
    }
}

QStringList ClangCompleter::options() const
{
    return d->m_unit->compilationOptions();
}

void ClangCompleter::setOptions(const QStringList &options) const
{
    if (d->m_unit->compilationOptions() != options) {
        d->m_unit->setCompilationOptions(options);
        d->m_unit->unload();
    }
}

bool ClangCompleter::isSignalSlotCompletion() const
{
    return d->m_isSignalSlotCompletion;
}

void ClangCompleter::setSignalSlotCompletion(bool isSignalSlot)
{
    d->m_isSignalSlotCompletion = isSignalSlot;
}

bool ClangCompleter::reparse(const UnsavedFiles &unsavedFiles)
{
    if (!d->m_unit->isLoaded())
        return d->parseFromFile(unsavedFiles);

    d->m_unit->setUnsavedFiles(unsavedFiles);
    d->m_unit->reparse();
    return d->m_unit->isLoaded();
}

QList<CodeCompletionResult> ClangCompleter::codeCompleteAt(unsigned line,
                                                           unsigned column,
                                                           const UnsavedFiles &unsavedFiles)
{
#ifdef TIME_COMPLETION
    QTime t;t.start();
#endif // TIME_COMPLETION

    if (!d->m_unit->isLoaded())
        if (!d->parseFromFile(unsavedFiles))
            return QList<CodeCompletionResult>();

    ScopedCXCodeCompleteResults results;
    d->m_unit->setUnsavedFiles(unsavedFiles);
    d->m_unit->codeCompleteAt(line, column, results);

    QList<CodeCompletionResult> completions;
    if (results) {
        const quint64 contexts = clang_codeCompleteGetContexts(results);
        CompletionProposalsBuilder builder(completions, contexts, d->m_isSignalSlotCompletion);
        for (unsigned i = 0; i < results.size(); ++i)
            builder(results.completionAt(i));
    }

#ifdef TIME_COMPLETION
    qDebug() << "Completion timing:" << completions.size() << "results in" << t.elapsed() << "ms.";
#endif // TIME_COMPLETION

    return completions;
}

bool ClangCompleter::objcEnabled() const
{
    static const QString objcppOption = QLatin1String("-ObjC++");
    static const QString objcOption = QLatin1String("-ObjC");

    QStringList options = d->m_unit->compilationOptions();
    return options.contains(objcOption) || options.contains(objcppOption);
}

QMutex *ClangCompleter::mutex() const
{
    return &d->m_mutex;
}
