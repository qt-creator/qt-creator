/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
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
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "helpfindsupport.h"
#include "helpviewer.h"

#include <utils/qtcassert.h>

using namespace Core;
using namespace Help::Internal;

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
