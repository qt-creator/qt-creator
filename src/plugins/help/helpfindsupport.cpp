// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "helpfindsupport.h"
#include "helpviewer.h"

#include <utils/qtcassert.h>

using namespace Core;
using namespace Help::Internal;
using namespace Utils;

HelpViewerFindSupport::HelpViewerFindSupport(HelpViewer *viewer)
    : m_viewer(viewer)
{
}

FindFlags HelpViewerFindSupport::supportedFindFlags() const
{
    return FindBackward | FindCaseSensitively;
}

QString HelpViewerFindSupport::currentFindString() const
{
    QTC_ASSERT(m_viewer, return QString());
    return m_viewer->selectedText();
}

IFindSupport::Result HelpViewerFindSupport::findIncremental(const QString &txt,
    FindFlags findFlags)
{
    QTC_ASSERT(m_viewer, return NotFound);
    findFlags &= ~FindBackward;
    return find(txt, findFlags, true) ? Found : NotFound;
}

IFindSupport::Result HelpViewerFindSupport::findStep(const QString &txt,
    FindFlags findFlags)
{
    QTC_ASSERT(m_viewer, return NotFound);
    return find(txt, findFlags, false) ? Found : NotFound;
}

bool HelpViewerFindSupport::find(const QString &txt,
    FindFlags findFlags, bool incremental)
{
    QTC_ASSERT(m_viewer, return false);
    bool wrapped = false;
    bool found = m_viewer->findText(txt, findFlags, incremental, false, &wrapped);
    if (wrapped)
        showWrapIndicator(m_viewer);
    return found;
}
