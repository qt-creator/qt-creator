/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
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
** If you are unsure which license is appropriate for your use, please
** contact the sales department at qt-sales@nokia.com.
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
