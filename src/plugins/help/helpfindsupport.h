/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
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
** contact the sales department at http://qt.nokia.com/contact.
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
    IFindSupport::FindFlags supportedFindFlags() const;

    void resetIncrementalSearch() {}
    void clearResults() {}
    QString currentFindString() const;
    QString completedFindString() const;

    Result findIncremental(const QString &txt, Find::IFindSupport::FindFlags findFlags);
    Result findStep(const QString &txt, Find::IFindSupport::FindFlags findFlags);
    bool replaceStep(const QString &, const QString &,
        Find::IFindSupport::FindFlags ) { return false; }
    int replaceAll(const QString &, const QString &,
        Find::IFindSupport::FindFlags ) { return 0; }

private:
    bool find(const QString &ttf, Find::IFindSupport::FindFlags findFlags, bool incremental);

    CentralWidget *m_centralWidget;
};

class HelpViewerFindSupport : public Find::IFindSupport
{
    Q_OBJECT
public:
    HelpViewerFindSupport(HelpViewer *viewer);

    bool isEnabled() const { return true; }
    bool supportsReplace() const { return false; }
    IFindSupport::FindFlags supportedFindFlags() const;
    void resetIncrementalSearch() {}
    void clearResults() {}
    QString currentFindString() const;
    QString completedFindString() const { return QString(); }

    Result findIncremental(const QString &txt, Find::IFindSupport::FindFlags findFlags);
    Result findStep(const QString &txt, Find::IFindSupport::FindFlags findFlags);
    bool replaceStep(const QString &, const QString &,
        Find::IFindSupport::FindFlags ) { return false; }
    int replaceAll(const QString &, const QString &,
        Find::IFindSupport::FindFlags ) { return 0; }

private:
    bool find(const QString &ttf, Find::IFindSupport::FindFlags findFlags, bool incremental);
    HelpViewer *m_viewer;
};

} // namespace Internal
} // namespace Help

#endif // HELPFINDSUPPORT_H
