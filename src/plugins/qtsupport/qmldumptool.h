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

#pragma once

#include "qtsupport_global.h"

#include <utils/buildablehelperlibrary.h>

namespace ProjectExplorer { class Kit; }
namespace Utils { class Environment; }

namespace ProjectExplorer {
    class Project;
    class ToolChain;
}

namespace QtSupport {
class BaseQtVersion;

class QTSUPPORT_EXPORT QmlDumpTool : public Utils::BuildableHelperLibrary
{
public:
    static QString toolForVersion(const BaseQtVersion *version, bool debugDump);
    static QString toolForQtPaths(const QString &qtInstallBins,
                                 bool debugDump);

    static void pathAndEnvironment(const ProjectExplorer::Kit *k, bool preferDebug,
                                   QString *path, Utils::Environment *env);
};

} // namespace
