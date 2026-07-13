// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "pysideuicextracompiler.h"

#include <utils/qtcprocess.h>

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

ProcessExtraCompiler::Parameters PySideUicExtraCompiler::parameters() const
{
    Parameters params;

    params.command.setExecutable(m_pySideUic);

    // TODO: Any reason not to share this with UicGenerator?
    params.postRunner = [targets = this->targets()](Process *process) {
        FileNameToContentsHash result;

        if (targets.size() != 1)
            return result;
        // As far as I can discover in the UIC sources, it writes out local 8-bit encoding. The
        // conversion below is to normalize both the encoding, and the line terminators.
        result[targets.first()] = process->readAllStandardOutput().toUtf8();
        return result;

    };

    return params;
}

FilePath PySideUicExtraCompiler::pySideUicPath() const
{
    return m_pySideUic;
}

} // Python::Internal
