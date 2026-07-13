// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "pysideuicextracompiler.h"

#include <qtsupport/qtsupportutils.h>
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
    QTC_ASSERT(targets().size() == 1, return {});

    Parameters params;
    params.command.setExecutable(m_pySideUic);
    params.postRunner = [target = targets().first()](Process *process) {
        return FileNameToContentsHash{std::make_pair(target, QtSupport::uiHeaderFromUic(process))};
    };

    return params;
}

FilePath PySideUicExtraCompiler::pySideUicPath() const
{
    return m_pySideUic;
}

} // Python::Internal
