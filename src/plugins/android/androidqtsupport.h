/**************************************************************************
**
** Copyright (c) 2014 BogDan Vatra <bog_dan_ro@yahoo.com>
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef ANDROIDSUPPORT_H
#define ANDROIDSUPPORT_H

#include <QObject>
#include <QList>

#include <projectexplorer/project.h>

#include <utils/fileutils.h>

#include "android_global.h"

namespace ProjectExplorer {
    class Target;
    class DeployConfiguration;
    class ProcessParameters;
}

namespace Android {

class ANDROID_EXPORT AndroidQtSupport : public QObject
{
    Q_OBJECT

public:
    enum BuildType {
        DebugBuild,
        ReleaseBuildUnsigned,
        ReleaseBuildSigned
    };

public:
    virtual bool canHandle(const ProjectExplorer::Target *target) const = 0;
    virtual QStringList soLibSearchPath(const ProjectExplorer::Target *target) const = 0;
    virtual QStringList projectTargetApplications(const ProjectExplorer::Target *target) const = 0;
    virtual Utils::FileName apkPath(ProjectExplorer::Target *target, BuildType buildType) const = 0;

    virtual void resetBuild(const ProjectExplorer::Target *target) = 0;
};

} // namespace QtSupport


#endif // ANDROIDSUPPORT_H
