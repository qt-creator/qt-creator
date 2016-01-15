/****************************************************************************
**
** Copyright (C) 2016 Nicolas Arnaud-Cormos
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

#include "macrotextfind.h"

#include <utils/qtcassert.h>

using namespace Macros;
using namespace Macros::Internal;

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

Core::FindFlags MacroTextFind::supportedFindFlags() const
{
    QTC_ASSERT(m_currentFind, return 0);
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
    QTC_ASSERT(m_currentFind, return QString());
    return m_currentFind->currentFindString();
}

QString MacroTextFind::completedFindString() const
{
    QTC_ASSERT(m_currentFind, return QString());
    return m_currentFind->completedFindString();
}

void MacroTextFind::highlightAll(const QString &txt, Core::FindFlags findFlags)
{
    QTC_ASSERT(m_currentFind, return);
    m_currentFind->highlightAll(txt, findFlags);
}

Core::IFindSupport::Result MacroTextFind::findIncremental(const QString &txt, Core::FindFlags findFlags)
{
    QTC_ASSERT(m_currentFind, return IFindSupport::NotFound);
    Core::IFindSupport::Result result = m_currentFind->findIncremental(txt, findFlags);
    if (result == Found)
        emit incrementalFound(txt, findFlags);
    return result;
}

Core::IFindSupport::Result MacroTextFind::findStep(const QString &txt, Core::FindFlags findFlags)
{
    QTC_ASSERT(m_currentFind, return IFindSupport::NotFound);
    Core::IFindSupport::Result result = m_currentFind->findStep(txt, findFlags);
    if (result == Found)
        emit stepFound(txt, findFlags);
    return result;
}

void MacroTextFind::replace(const QString &before, const QString &after, Core::FindFlags findFlags)
{
    QTC_ASSERT(m_currentFind, return);
    m_currentFind->replace(before, after, findFlags);
    emit replaced(before, after, findFlags);
}

bool MacroTextFind::replaceStep(const QString &before, const QString &after, Core::FindFlags findFlags)
{
    QTC_ASSERT(m_currentFind, return false);
    bool result = m_currentFind->replaceStep(before, after, findFlags);
    emit stepReplaced(before, after, findFlags);
    return result;
}

int MacroTextFind::replaceAll(const QString &before, const QString &after, Core::FindFlags findFlags)
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
