/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#ifndef ANALYZERSTARTPARAMETERS_H
#define ANALYZERSTARTPARAMETERS_H

#include "analyzerbase_global.h"
#include "analyzerconstants.h"

#include <coreplugin/id.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <ssh/sshconnection.h>
#include <utils/environment.h>

#include <QMetaType>

namespace Analyzer {

class ANALYZER_EXPORT AnalyzerRunnable
{
public:
    QString debuggee;
    QString debuggeeArgs;
};

class ANALYZER_EXPORT AnalyzerConnection
{
public:
    QSsh::SshConnectionParameters connParams;
    QString analyzerHost;
    QString analyzerSocket;
    quint16 analyzerPort = 0;
};

class ANALYZER_EXPORT AnalyzerStartParameters
    : public AnalyzerRunnable, public AnalyzerConnection
{
public:
};

} // namespace Analyzer

#endif // ANALYZERSTARTPARAMETERS_H
