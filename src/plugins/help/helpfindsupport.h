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

    bool isEnabled() const;
    bool supportsReplace() const { return false; }
    Find::FindFlags supportedFindFlags() const;

    void resetIncrementalSearch() {}
    void clearResults() {}
    QString currentFindString() const;
    QString completedFindString() const;

    Result findIncremental(const QString &txt, Find::FindFlags findFlags);
    Result findStep(const QString &txt, Find::FindFlags findFlags);
    void replace(const QString &, const QString &,
        Find::FindFlags ) { }
    bool replaceStep(const QString &, const QString &,
        Find::FindFlags ) { return false; }
    int replaceAll(const QString &, const QString &,
        Find::FindFlags ) { return 0; }

private:
    bool find(const QString &ttf, Find::FindFlags findFlags, bool incremental);

    CentralWidget *m_centralWidget;
};

class HelpViewerFindSupport : public Find::IFindSupport
{
    Q_OBJECT
public:
    HelpViewerFindSupport(HelpViewer *viewer);

    bool isEnabled() const { return true; }
    bool supportsReplace() const { return false; }
    Find::FindFlags supportedFindFlags() const;
    void resetIncrementalSearch() {}
    void clearResults() {}
    QString currentFindString() const;
    QString completedFindString() const { return QString(); }

    Result findIncremental(const QString &txt, Find::FindFlags findFlags);
    Result findStep(const QString &txt, Find::FindFlags findFlags);
    void replace(const QString &, const QString &,
        Find::FindFlags ) { }
    bool replaceStep(const QString &, const QString &,
        Find::FindFlags ) { return false; }
    int replaceAll(const QString &, const QString &,
        Find::FindFlags ) { return 0; }

private:
    bool find(const QString &ttf, Find::FindFlags findFlags, bool incremental);
    HelpViewer *m_viewer;
};

} // namespace Internal
} // namespace Help

#endif // HELPFINDSUPPORT_H
