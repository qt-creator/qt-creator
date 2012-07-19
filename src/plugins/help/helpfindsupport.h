/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**************************************************************************/

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
