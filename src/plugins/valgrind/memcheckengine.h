/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Author: Nicolas Arnaud-Cormos, KDAB (nicolas.arnaud-cormos@kdab.com)
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

#pragma once

#include "valgrindengine.h"

#include "valgrindrunner.h"
#include "xmlprotocol/threadedparser.h"

namespace Valgrind {
namespace Internal {

class MemcheckToolRunner : public ValgrindToolRunner
{
    Q_OBJECT

public:
    explicit MemcheckToolRunner(ProjectExplorer::RunControl *runControl,
                                bool withGdb = false);

    void start() override;
    void stop() override;

    QStringList suppressionFiles() const;

signals:
    void internalParserError(const QString &errorString);
    void parserError(const Valgrind::XmlProtocol::Error &error);
    void suppressionCount(const QString &name, qint64 count);

private:
    QString progressTitle() const override;
    QStringList toolArguments() const override;

    void startDebugger();
    void appendLog(const QByteArray &data);

    const bool m_withGdb;
    QHostAddress m_localServerAddress;
};

} // namespace Internal
} // namespace Valgrind
