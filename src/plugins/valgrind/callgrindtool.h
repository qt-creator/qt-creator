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

#ifndef CALLGRINDTOOL_H
#define CALLGRINDTOOL_H

#include <analyzerbase/ianalyzertool.h>

namespace Valgrind {
namespace Internal {

const char CallgrindToolId[] = "Callgrind";
const char CallgrindLocalActionId[] = "Callgrind.Local";
const char CallgrindRemoteActionId[] = "Callgrind.Remote";

class ValgrindRunControl;
const char CALLGRIND_RUN_MODE[] = "CallgrindTool.CallgrindRunMode";

class CallgrindToolPrivate;

class CallgrindTool : public QObject
{
    Q_OBJECT

public:
    CallgrindTool(QObject *parent);
    ~CallgrindTool();

    ValgrindRunControl *createRunControl(const Analyzer::AnalyzerStartParameters &sp,
        ProjectExplorer::RunConfiguration *runConfiguration = 0);
    QWidget *createWidgets();

public slots:
    void handleShowCostsOfFunction();

private:
    CallgrindToolPrivate *d;
};

} // namespace Internal
} // namespace Valgrind

#endif // CALLGRINDTOOL_H
