/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
**
** Non-Open Source Usage
**
** Licensees may use this file in accordance with the Qt Beta Version
** License Agreement, Agreement version 2.2 provided with the Software or,
** alternatively, in accordance with the terms contained in a written
** agreement between you and Nokia.
**
** GNU General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU General
** Public License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the packaging
** of this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
**
** http://www.fsf.org/licensing/licenses/info/GPLv2.html and
** http://www.gnu.org/copyleft/gpl.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt GPL Exception
** version 1.3, included in the file GPL_EXCEPTION.txt in this package.
**
***************************************************************************/

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

QString HelpFindSupport::currentFindString() const
{
    QTC_ASSERT(m_centralWidget, return QString());
    HelpViewer *viewer = m_centralWidget->currentHelpViewer();
    if (!viewer)
        return QString();
#if !defined(QT_NO_WEBKIT)
    return viewer->selectedText();
#else
    return viewer->textCursor().selectedText();
#endif
}

QString HelpFindSupport::completedFindString() const
{
    return QString();
}

bool HelpFindSupport::findIncremental(const QString &txt, QTextDocument::FindFlags findFlags)
{
    QTC_ASSERT(m_centralWidget, return false);
    findFlags &= ~QTextDocument::FindBackward;
    return m_centralWidget->find(txt, findFlags, true);
}

bool HelpFindSupport::findStep(const QString &txt, QTextDocument::FindFlags findFlags)
{
    QTC_ASSERT(m_centralWidget, return false);
    return m_centralWidget->find(txt, findFlags, false);
}
