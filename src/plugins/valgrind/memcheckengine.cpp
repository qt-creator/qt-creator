/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
** Author: Nicolas Arnaud-Cormos, KDAB (nicolas.arnaud-cormos@kdab.com)
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

#include "memcheckengine.h"

#include "valgrindsettings.h"

#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/taskhub.h>

#include <valgrind/xmlprotocol/error.h>
#include <valgrind/xmlprotocol/status.h>

#include <utils/qtcassert.h>

using namespace Analyzer;
using namespace ProjectExplorer;
using namespace Valgrind::XmlProtocol;

namespace Valgrind {
namespace Internal {

MemcheckRunControl::MemcheckRunControl(const AnalyzerStartParameters &sp,
        ProjectExplorer::RunConfiguration *runConfiguration)
    : ValgrindRunControl(sp, runConfiguration)
{
    connect(&m_parser, SIGNAL(error(Valgrind::XmlProtocol::Error)),
            SIGNAL(parserError(Valgrind::XmlProtocol::Error)));
    connect(&m_parser, SIGNAL(suppressionCount(QString,qint64)),
            SIGNAL(suppressionCount(QString,qint64)));
    connect(&m_parser, SIGNAL(internalError(QString)),
            SIGNAL(internalParserError(QString)));
    connect(&m_parser, SIGNAL(status(Valgrind::XmlProtocol::Status)),
            SLOT(status(Valgrind::XmlProtocol::Status)));

    m_progress->setProgressRange(0, XmlProtocol::Status::Finished + 1);
}

QString MemcheckRunControl::progressTitle() const
{
    return tr("Analyzing Memory");
}

Valgrind::ValgrindRunner *MemcheckRunControl::runner()
{
    return &m_runner;
}

bool MemcheckRunControl::startEngine()
{
    m_runner.setParser(&m_parser);

    // Clear about-to-be-outdated tasks.
    TaskHub::clearTasks(Analyzer::Constants::ANALYZERTASK_ID);

    appendMessage(tr("Analyzing memory of %1").arg(executable()) + QLatin1Char('\n'),
                        Utils::NormalMessageFormat);
    return ValgrindRunControl::startEngine();
}

void MemcheckRunControl::stopEngine()
{
    disconnect(&m_parser, SIGNAL(internalError(QString)),
               this, SIGNAL(internalParserError(QString)));
    ValgrindRunControl::stopEngine();
}

QStringList MemcheckRunControl::toolArguments() const
{
    QStringList arguments;
    arguments << QLatin1String("--gen-suppressions=all");

    QTC_ASSERT(m_settings, return arguments);

    if (m_settings->trackOrigins())
        arguments << QLatin1String("--track-origins=yes");

    if (m_settings->showReachable())
        arguments << QLatin1String("--show-reachable=yes");

    QString leakCheckValue;
    switch (m_settings->leakCheckOnFinish()) {
    case ValgrindBaseSettings::LeakCheckOnFinishNo:
        leakCheckValue = QLatin1String("no");
        break;
    case ValgrindBaseSettings::LeakCheckOnFinishYes:
        leakCheckValue = QLatin1String("full");
        break;
    case ValgrindBaseSettings::LeakCheckOnFinishSummaryOnly:
    default:
        leakCheckValue = QLatin1String("summary");
        break;
    }
    arguments << QLatin1String("--leak-check=") + leakCheckValue;

    foreach (const QString &file, m_settings->suppressionFiles())
        arguments << QString::fromLatin1("--suppressions=%1").arg(file);

    arguments << QString::fromLatin1("--num-callers=%1").arg(m_settings->numCallers());
    return arguments;
}

QStringList MemcheckRunControl::suppressionFiles() const
{
    return m_settings->suppressionFiles();
}

void MemcheckRunControl::status(const Status &status)
{
    m_progress->setProgressValue(status.state() + 1);
}

} // namespace Internal
} // namespace Valgrind
