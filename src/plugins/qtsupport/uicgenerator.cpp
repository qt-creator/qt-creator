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

#include <utils/qtcassert.h>

#include <QFileInfo>
#include <QDir>
#include <QLoggingCategory>
#include <QProcess>
#include <QDateTime>

using namespace ProjectExplorer;

namespace QtSupport {

UicGenerator::UicGenerator(const Project *project, const Utils::FilePath &source,
                           const Utils::FilePaths &targets, QObject *parent) :
    ProcessExtraCompiler(project, source, targets, parent)
{
    QTC_ASSERT(targets.count() == 1, return);
}

Utils::FilePath UicGenerator::command() const
{
    QtSupport::BaseQtVersion *version = nullptr;
    Target *target;
    if ((target = project()->activeTarget()))
        version = QtSupport::QtKitAspect::qtVersion(target->kit());
    else
        version = QtSupport::QtKitAspect::qtVersion(KitManager::defaultKit());

    if (!version)
        return Utils::FilePath();

    return Utils::FilePath::fromString(version->uicCommand());
}

QStringList UicGenerator::arguments() const
{
    return {"-p"};
}

FileNameToContentsHash UicGenerator::handleProcessFinished(QProcess *process)
{
    FileNameToContentsHash result;
    if (process->exitStatus() != QProcess::NormalExit && process->exitCode() != 0)
        return result;

    const Utils::FilePaths targetList = targets();
    if (targetList.size() != 1)
        return result;
    // As far as I can discover in the UIC sources, it writes out local 8-bit encoding. The
    // conversion below is to normalize both the encoding, and the line terminators.
    QByteArray content = QString::fromLocal8Bit(process->readAllStandardOutput()).toUtf8();
    content.prepend("#pragma once\n");
    result[targetList.first()] = content;
    return result;
}

void UicGenerator::handleProcessStarted(QProcess *process, const QByteArray &sourceContents)
{
    process->write(sourceContents);
    process->closeWriteChannel();
}

FileType UicGeneratorFactory::sourceType() const
{
    return FileType::Form;
}

QString UicGeneratorFactory::sourceTag() const
{
    return QLatin1String("ui");
}

ExtraCompiler *UicGeneratorFactory::create(const Project *project,
                                           const Utils::FilePath &source,
                                           const Utils::FilePaths &targets)
{
    annouceCreation(project, source, targets);

    return new UicGenerator(project, source, targets, this);
}

} // QtSupport
