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

#include "cmakefile.h"
#include "cmakeproject.h"
#include "cmakeprojectconstants.h"

#include <utils/fileutils.h>

using namespace Utils;

namespace CMakeProjectManager {
namespace Internal {

CMakeFile::CMakeFile(const FileName &fileName)
{
    setId("Cmake.ProjectFile");
    setMimeType(QLatin1String(Constants::CMAKEPROJECTMIMETYPE));
    setFilePath(fileName);
}

bool CMakeFile::save(QString *errorString, const QString &fileName, bool autoSave)
{
    // Once we have an texteditor open for this file, we probably do
    // need to implement this, don't we.
    Q_UNUSED(errorString)
    Q_UNUSED(fileName)
    Q_UNUSED(autoSave)
    return false;
}

bool CMakeFile::isModified() const
{
    return false;
}

bool CMakeFile::isSaveAsAllowed() const
{
    return false;
}

Core::IDocument::ReloadBehavior CMakeFile::reloadBehavior(ChangeTrigger state, ChangeType type) const
{
    Q_UNUSED(state)
    Q_UNUSED(type)
    return BehaviorSilent;
}

bool CMakeFile::reload(QString *errorString, ReloadFlag flag, ChangeType type)
{
    Q_UNUSED(errorString)
    Q_UNUSED(flag)
    Q_UNUSED(type)
    return true;
}
} // namespace Internal
} // namespace CMakeProjectManager
