/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Author: Nicolas Arnaud-Cormos, KDAB (nicolas.arnaud-cormos@kdab.com)
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

#ifndef MEMCHECKENGINE_H
#define MEMCHECKENGINE_H

#include "valgrindengine.h"

#include "memcheck/memcheckrunner.h"
#include "xmlprotocol/threadedparser.h"

namespace Valgrind {
namespace Internal {

class MemcheckEngine : public ValgrindEngine
{
    Q_OBJECT

public:
    MemcheckEngine(Analyzer::IAnalyzerTool *tool, const Analyzer::AnalyzerStartParameters &sp,
        ProjectExplorer::RunConfiguration *runConfiguration);

    bool start();
    void stop();

    QStringList suppressionFiles() const;

signals:
    void internalParserError(const QString &errorString);
    void parserError(const Valgrind::XmlProtocol::Error &error);
    void suppressionCount(const QString &name, qint64 count);

protected:
    virtual QString progressTitle() const;
    virtual QStringList toolArguments() const;
    virtual ValgrindRunner *runner();

private slots:
    void receiveLogMessage(const QByteArray &);
    void status(const Valgrind::XmlProtocol::Status &status);

private:
    XmlProtocol::ThreadedParser m_parser;
    Memcheck::MemcheckRunner m_runner;
};

} // namespace Internal
} // namespace Valgrind

#endif // MEMCHECKENGINE_H
