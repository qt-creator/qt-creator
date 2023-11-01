// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "uicgenerator.h"

#include "baseqtversion.h"
#include "qtkitaspect.h"

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/kitmanager.h>
#include <projectexplorer/target.h>

#include <utils/process.h>
#include <utils/qtcassert.h>

#include <QDateTime>
#include <QLoggingCategory>

using namespace ProjectExplorer;
using namespace Utils;

namespace QtSupport {

class UicGenerator final : public ProcessExtraCompiler
{
public:
    UicGenerator(const Project *project, const FilePath &source,
                 const FilePaths &targets, QObject *parent)
        : ProcessExtraCompiler(project, source, targets, parent)
    {
        QTC_ASSERT(targets.count() == 1, return);
    }

protected:
    FilePath command() const override;
    QStringList arguments() const override { return {"-p"}; }
    FileNameToContentsHash handleProcessFinished(Process *process) override;
};

FilePath UicGenerator::command() const
{
    QtSupport::QtVersion *version = nullptr;
    Target *target;
    if ((target = project()->activeTarget()))
        version = QtSupport::QtKitAspect::qtVersion(target->kit());
    else
        version = QtSupport::QtKitAspect::qtVersion(KitManager::defaultKit());

    if (!version)
        return {};

    return version->uicFilePath();
}

FileNameToContentsHash UicGenerator::handleProcessFinished(Process *process)
{
    FileNameToContentsHash result;
    if (process->exitStatus() != QProcess::NormalExit && process->exitCode() != 0)
        return result;

    const FilePaths targetList = targets();
    if (targetList.size() != 1)
        return result;
    // As far as I can discover in the UIC sources, it writes out local 8-bit encoding. The
    // conversion below is to normalize both the encoding, and the line terminators.
    QByteArray content = QString::fromLocal8Bit(process->readAllRawStandardOutput()).toUtf8();
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
                                           const FilePath &source,
                                           const FilePaths &targets)
{
    return new UicGenerator(project, source, targets, this);
}

} // QtSupport
