// Copyright (C) 2016 Nicolas Arnaud-Cormos
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "macrotextfind.h"

#include <utils/qtcassert.h>

using namespace Macros;
using namespace Macros::Internal;
using namespace Utils;

MacroTextFind::MacroTextFind(Core::IFindSupport *currentFind):
    Core::IFindSupport(),
    m_currentFind(currentFind)
{
}

bool MacroTextFind::supportsReplace() const
{
    QTC_ASSERT(m_currentFind, return false);
    return m_currentFind->supportsReplace();
}

FindFlags MacroTextFind::supportedFindFlags() const
{
    QTC_ASSERT(m_currentFind, return {});
    return m_currentFind->supportedFindFlags();
}

void MacroTextFind::resetIncrementalSearch()
{
    QTC_ASSERT(m_currentFind, return);
    m_currentFind->resetIncrementalSearch();
    emit incrementalSearchReseted();
}

void MacroTextFind::clearHighlights()
{
    QTC_ASSERT(m_currentFind, return);
    m_currentFind->clearHighlights();
}

QString MacroTextFind::currentFindString() const
{
    QTC_ASSERT(m_currentFind, return {});
    return m_currentFind->currentFindString();
}

QString MacroTextFind::completedFindString() const
{
    QTC_ASSERT(m_currentFind, return {});
    return m_currentFind->completedFindString();
}

void MacroTextFind::highlightAll(const QString &txt, FindFlags findFlags)
{
    QTC_ASSERT(m_currentFind, return);
    m_currentFind->highlightAll(txt, findFlags);
}

Core::IFindSupport::Result MacroTextFind::findIncremental(const QString &txt, FindFlags findFlags)
{
    QTC_ASSERT(m_currentFind, return IFindSupport::NotFound);
    Core::IFindSupport::Result result = m_currentFind->findIncremental(txt, findFlags);
    if (result == Found)
        emit incrementalFound(txt, findFlags);
    return result;
}

Core::IFindSupport::Result MacroTextFind::findStep(const QString &txt, FindFlags findFlags)
{
    QTC_ASSERT(m_currentFind, return IFindSupport::NotFound);
    Core::IFindSupport::Result result = m_currentFind->findStep(txt, findFlags);
    if (result == Found)
        emit stepFound(txt, findFlags);
    return result;
}

void MacroTextFind::replace(const QString &before, const QString &after, FindFlags findFlags)
{
    QTC_ASSERT(m_currentFind, return);
    m_currentFind->replace(before, after, findFlags);
    emit replaced(before, after, findFlags);
}

bool MacroTextFind::replaceStep(const QString &before, const QString &after, FindFlags findFlags)
{
    QTC_ASSERT(m_currentFind, return false);
    bool result = m_currentFind->replaceStep(before, after, findFlags);
    emit stepReplaced(before, after, findFlags);
    return result;
}

int MacroTextFind::replaceAll(const QString &before, const QString &after, FindFlags findFlags)
{
    QTC_ASSERT(m_currentFind, return 0);
    int result = m_currentFind->replaceAll(before, after, findFlags);
    emit allReplaced(before, after, findFlags);
    return result;
}

void MacroTextFind::defineFindScope()
{
    QTC_ASSERT(m_currentFind, return);
    m_currentFind->defineFindScope();
}

void MacroTextFind::clearFindScope()
{
    QTC_ASSERT(m_currentFind, return);
    m_currentFind->clearFindScope();
}
