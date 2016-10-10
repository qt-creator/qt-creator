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

#include "builddirmanager.h"
#include "cmakeprojectconstants.h"

#include <projectexplorer/target.h>

#include <utils/fileutils.h>

using namespace Utils;

namespace CMakeProjectManager {
namespace Internal {

CMakeFile::CMakeFile(BuildDirManager *bdm, const FileName &fileName) : m_buildDirManager(bdm)
{
    setId("Cmake.ProjectFile");
    setMimeType(QLatin1String(Constants::CMAKEPROJECTMIMETYPE));
    setFilePath(fileName);
}

Core::IDocument::ReloadBehavior CMakeFile::reloadBehavior(ChangeTrigger state, ChangeType type) const
{
    Q_UNUSED(state)
    Q_UNUSED(type)
    return BehaviorSilent;
}

bool CMakeFile::reload(QString *errorString, Core::IDocument::ReloadFlag flag, Core::IDocument::ChangeType type)
{
    Q_UNUSED(errorString);
    Q_UNUSED(flag);

    if (type != TypePermissions)
        m_buildDirManager->handleCmakeFileChange();
    return true;
}

} // namespace Internal
} // namespace CMakeProjectManager
