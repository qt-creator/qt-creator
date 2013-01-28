/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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

#ifndef HELPFINDSUPPORT_H
#define HELPFINDSUPPORT_H

#include "centralwidget.h"

#include <find/ifindsupport.h>

namespace Help {
namespace Internal {

class HelpViewer;

class HelpFindSupport : public Find::IFindSupport
{
    Q_OBJECT

public:
    HelpFindSupport(CentralWidget *centralWidget);
    ~HelpFindSupport();

    bool supportsReplace() const { return false; }
    Find::FindFlags supportedFindFlags() const;

    void resetIncrementalSearch() {}
    void clearResults() {}
    QString currentFindString() const;
    QString completedFindString() const;

    Result findIncremental(const QString &txt, Find::FindFlags findFlags);
    Result findStep(const QString &txt, Find::FindFlags findFlags);

private:
    bool find(const QString &ttf, Find::FindFlags findFlags, bool incremental);

    CentralWidget *m_centralWidget;
};

class HelpViewerFindSupport : public Find::IFindSupport
{
    Q_OBJECT
public:
    HelpViewerFindSupport(HelpViewer *viewer);

    bool supportsReplace() const { return false; }
    Find::FindFlags supportedFindFlags() const;
    void resetIncrementalSearch() {}
    void clearResults() {}
    QString currentFindString() const;
    QString completedFindString() const { return QString(); }

    Result findIncremental(const QString &txt, Find::FindFlags findFlags);
    Result findStep(const QString &txt, Find::FindFlags findFlags);

private:
    bool find(const QString &ttf, Find::FindFlags findFlags, bool incremental);
    HelpViewer *m_viewer;
};

} // namespace Internal
} // namespace Help

#endif // HELPFINDSUPPORT_H
