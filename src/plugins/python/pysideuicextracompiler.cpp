// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "pysideuicextracompiler.h"

#include <utils/process.h>

using namespace ProjectExplorer;
using namespace Utils;

namespace Python::Internal {

PySideUicExtraCompiler::PySideUicExtraCompiler(const FilePath &pySideUic,
                                               const Project *project,
                                               const FilePath &source,
                                               const FilePaths &targets,
                                               QObject *parent)
    : ProcessExtraCompiler(project, source, targets, parent)
    , m_pySideUic(pySideUic)
{
}

FilePath PySideUicExtraCompiler::pySideUicPath() const
{
    return m_pySideUic;
}

FilePath PySideUicExtraCompiler::command() const
{
    return m_pySideUic;
}

FileNameToContentsHash PySideUicExtraCompiler::handleProcessFinished(Process *process)
{
    FileNameToContentsHash result;
    if (process->exitStatus() != QProcess::NormalExit && process->exitCode() != 0)
        return result;

    const FilePaths targetList = targets();
    if (targetList.size() != 1)
        return result;
    // As far as I can discover in the UIC sources, it writes out local 8-bit encoding. The
    // conversion below is to normalize both the encoding, and the line terminators.
    result[targetList.first()] = QString::fromLocal8Bit(process->readAllRawStandardOutput()).toUtf8();
    return result;
}

} // Python::Internal
