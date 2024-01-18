// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "uicgenerator.h"

#include "baseqtversion.h"
#include "qtkitaspect.h"

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/extracompiler.h>
#include <projectexplorer/kitmanager.h>
#include <projectexplorer/target.h>

#include <utils/process.h>
#include <utils/qtcassert.h>

#include <QDateTime>
#include <QLoggingCategory>

using namespace ProjectExplorer;
using namespace Utils;

namespace QtSupport::Internal {

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
    FilePath command() const final;
    QStringList arguments() const final { return {"-p"}; }
    FileNameToContentsHash handleProcessFinished(Process *process) final;
};

FilePath UicGenerator::command() const
{
    Target *target = project()->activeTarget();
    Kit *kit = target ? target->kit() : KitManager::defaultKit();
    QtVersion *version = QtKitAspect::qtVersion(kit);

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

// UicGeneratorFactory

class UicGeneratorFactory final : public ExtraCompilerFactory
{
public:
    UicGeneratorFactory() = default;

    FileType sourceType() const final { return FileType::Form; }

    QString sourceTag() const final { return QLatin1String("ui"); }

    ExtraCompiler *create(const Project *project,
                          const FilePath &source,
                          const FilePaths &targets) final
    {
        return new UicGenerator(project, source, targets, this);
    }
};

void setupUicGenerator()
{
    static UicGeneratorFactory theUicGeneratorFactory;
}

} // QtSupport::Internal
