// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "uicgenerator.h"
#include "baseqtversion.h"
#include "qtkitinformation.h"

#include <projectexplorer/kitmanager.h>
#include <projectexplorer/target.h>
#include <projectexplorer/buildconfiguration.h>

#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>

#include <QFileInfo>
#include <QDir>
#include <QLoggingCategory>
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
    QtSupport::QtVersion *version = nullptr;
    Target *target;
    if ((target = project()->activeTarget()))
        version = QtSupport::QtKitAspect::qtVersion(target->kit());
    else
        version = QtSupport::QtKitAspect::qtVersion(KitManager::defaultKit());

    if (!version)
        return Utils::FilePath();

    return version->uicFilePath();
}

QStringList UicGenerator::arguments() const
{
    return {"-p"};
}

FileNameToContentsHash UicGenerator::handleProcessFinished(Utils::QtcProcess *process)
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
    return new UicGenerator(project, source, targets, this);
}

} // QtSupport
