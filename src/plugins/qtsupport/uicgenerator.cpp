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

#include "uicgenerator.h"
#include "baseqtversion.h"
#include "qtkitinformation.h"

#include <projectexplorer/kitmanager.h>
#include <projectexplorer/target.h>
#include <projectexplorer/buildconfiguration.h>

#include <QFileInfo>
#include <QDir>
#include <QLoggingCategory>
#include <QProcess>
#include <QDateTime>

namespace QtSupport {

QLoggingCategory UicGenerator::m_log("qtc.uicgenerator");

UicGenerator::UicGenerator(const ProjectExplorer::Project *project, const Utils::FileName &source,
                           const Utils::FileNameList &targets, QObject *parent) :
    ProjectExplorer::ExtraCompiler(project, source, targets, parent)
{
    connect(&m_process, static_cast<void(QProcess::*)(int)>(&QProcess::finished),
            this, &UicGenerator::finishProcess);
}

void UicGenerator::finishProcess()
{
    if (!m_process.waitForFinished(3000)
            && m_process.exitStatus() != QProcess::NormalExit
            && m_process.exitCode() != 0) {

        qCDebug(m_log) << "finish process: failed" << m_process.readAllStandardError();
        m_process.kill();
        return;
    }

    // As far as I can discover in the UIC sources, it writes out local 8-bit encoding. The
    // conversion below is to normalize both the encoding, and the line terminators.
    QString normalized = QString::fromLocal8Bit(m_process.readAllStandardOutput());
    qCDebug(m_log) << "finish process: ok" << normalized.size() << "bytes.";
    setCompileTime(QDateTime::currentDateTime());
    setContent(targets()[0], normalized);
}

void UicGenerator::run(const QByteArray &sourceContent)
{
    if (m_process.state() != QProcess::NotRunning) {
        m_process.kill();
        m_process.waitForFinished(3000);
    }

    QtSupport::BaseQtVersion *version = 0;
    ProjectExplorer::Target *target;
    if ((target = project()->activeTarget()))
        version = QtSupport::QtKitInformation::qtVersion(target->kit());
    else
        version = QtSupport::QtKitInformation::qtVersion(ProjectExplorer::KitManager::defaultKit());

    if (!version)
        return;

    const QString generator = version->uicCommand();
    if (generator.isEmpty())
        return;

    m_process.setProcessEnvironment(buildEnvironment().toProcessEnvironment());

    qCDebug(m_log) << "  UicGenerator::run " << generator << " on "
                 << sourceContent.size() << " bytes";
    m_process.start(generator, QStringList(), QIODevice::ReadWrite);
    if (!m_process.waitForStarted())
        return;

    m_process.write(sourceContent);
    if (!m_process.waitForBytesWritten(3000)) {
        qCDebug(m_log) << "failed" << m_process.readAllStandardError();
        m_process.kill();
        return;
    }

    m_process.closeWriteChannel();
}

ProjectExplorer::FileType UicGeneratorFactory::sourceType() const
{
    return ProjectExplorer::FormType;
}

QString UicGeneratorFactory::sourceTag() const
{
    return QLatin1String("ui");
}

ProjectExplorer::ExtraCompiler *UicGeneratorFactory::create(const ProjectExplorer::Project *project,
                                                            const Utils::FileName &source,
                                                            const Utils::FileNameList &targets)
{
    return new UicGenerator(project, source, targets, this);
}

} // QtSupport
