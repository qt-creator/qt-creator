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

#ifndef HELPFINDSUPPORT_H
#define HELPFINDSUPPORT_H

#include "centralwidget.h"

#include <find/ifindsupport.h>

namespace Help {
namespace Internal {

class HelpFindSupport : public Find::IFindSupport
{
    Q_OBJECT

public:
    HelpFindSupport(CentralWidget *centralWidget);
    ~HelpFindSupport();

    bool isEnabled() const;
    bool supportsReplace() const { return false; }
    void resetIncrementalSearch() {}
    void clearResults() {}
    QString currentFindString() const;
    QString completedFindString() const;

    bool findIncremental(const QString &txt, QTextDocument::FindFlags findFlags);
    bool findStep(const QString &txt, QTextDocument::FindFlags findFlags);
    bool replaceStep(const QString &, const QString &,
        QTextDocument::FindFlags ) { return false; }
    int replaceAll(const QString &, const QString &,
        QTextDocument::FindFlags ) { return 0; }

private:
    bool find(const QString &ttf, QTextDocument::FindFlags findFlags, bool incremental);

    CentralWidget *m_centralWidget;
};

} // namespace Internal
} // namespace Help

#endif // HELPFINDSUPPORT_H
