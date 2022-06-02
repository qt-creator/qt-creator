/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
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

#include "pysideuicextracompiler.h"

#include <utils/qtcprocess.h>

using namespace ProjectExplorer;

namespace Python {
namespace Internal {

PySideUicExtraCompiler::PySideUicExtraCompiler(const Utils::FilePath &pySideUic,
                                               const Project *project,
                                               const Utils::FilePath &source,
                                               const Utils::FilePaths &targets,
                                               QObject *parent)
    : ProcessExtraCompiler(project, source, targets, parent)
    , m_pySideUic(pySideUic)
{
}

Utils::FilePath PySideUicExtraCompiler::pySideUicPath() const
{
    return m_pySideUic;
}

Utils::FilePath PySideUicExtraCompiler::command() const
{
    return m_pySideUic;
}

FileNameToContentsHash PySideUicExtraCompiler::handleProcessFinished(
    Utils::QtcProcess *process)
{
    FileNameToContentsHash result;
    if (process->exitStatus() != QProcess::NormalExit && process->exitCode() != 0)
        return result;

    const Utils::FilePaths targetList = targets();
    if (targetList.size() != 1)
        return result;
    // As far as I can discover in the UIC sources, it writes out local 8-bit encoding. The
    // conversion below is to normalize both the encoding, and the line terminators.
    result[targetList.first()] = QString::fromLocal8Bit(process->readAllStandardOutput()).toUtf8();
    return result;
}

} // namespace Internal
} // namespace Python
