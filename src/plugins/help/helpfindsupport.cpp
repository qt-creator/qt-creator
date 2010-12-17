/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "helpfindsupport.h"
#include "helpviewer.h"

#include <utils/qtcassert.h>

using namespace Help::Internal;

HelpFindSupport::HelpFindSupport(CentralWidget *centralWidget)
    : m_centralWidget(centralWidget)
{
}

HelpFindSupport::~HelpFindSupport()
{
}

bool HelpFindSupport::isEnabled() const
{
    return true;
}

Find::FindFlags HelpFindSupport::supportedFindFlags() const
{
    return Find::FindBackward | Find::FindCaseSensitively
        | Find::FindWholeWords;
}

QString HelpFindSupport::currentFindString() const
{
    QTC_ASSERT(m_centralWidget, return QString());
    HelpViewer *viewer = m_centralWidget->currentHelpViewer();
    if (!viewer)
        return QString();
    return viewer->selectedText();
}

QString HelpFindSupport::completedFindString() const
{
    return QString();
}

Find::IFindSupport::Result HelpFindSupport::findIncremental(const QString &txt,
    Find::FindFlags findFlags)
{
    QTC_ASSERT(m_centralWidget, return NotFound);
    findFlags &= ~Find::FindBackward;
    return m_centralWidget->find(txt, findFlags, true) ? Found : NotFound;
}

Find::IFindSupport::Result HelpFindSupport::findStep(const QString &txt,
    Find::FindFlags findFlags)
{
    QTC_ASSERT(m_centralWidget, return NotFound);
    return m_centralWidget->find(txt, findFlags, false) ? Found : NotFound;
}

// -- HelpViewerFindSupport

HelpViewerFindSupport::HelpViewerFindSupport(HelpViewer *viewer)
    : m_viewer(viewer)
{
}

Find::FindFlags HelpViewerFindSupport::supportedFindFlags() const
{
    return Find::FindBackward | Find::FindCaseSensitively
        | Find::FindWholeWords;
}

QString HelpViewerFindSupport::currentFindString() const
{
    QTC_ASSERT(m_viewer, return QString());
    return m_viewer->selectedText();
}

Find::IFindSupport::Result HelpViewerFindSupport::findIncremental(const QString &txt,
    Find::FindFlags findFlags)
{
    QTC_ASSERT(m_viewer, return NotFound);
    findFlags &= ~Find::FindBackward;
    return find(txt, findFlags, true) ? Found : NotFound;
}

Find::IFindSupport::Result HelpViewerFindSupport::findStep(const QString &txt,
    Find::FindFlags findFlags)
{
    QTC_ASSERT(m_viewer, return NotFound);
    return find(txt, findFlags, false) ? Found : NotFound;
}

bool HelpViewerFindSupport::find(const QString &txt,
    Find::FindFlags findFlags, bool incremental)
{
    QTC_ASSERT(m_viewer, return false);
    return m_viewer->findText(txt, findFlags, incremental, false);
}
