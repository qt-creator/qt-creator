/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef QMAKEPROJECTIMPORTER_H
#define QMAKEPROJECTIMPORTER_H

#include "qmakestep.h"

#include <projectexplorer/projectimporter.h>

namespace QtSupport { class BaseQtVersion; }

namespace QmakeProjectManager {

class QmakeProject;

namespace Internal {

// Documentation inside.
class QmakeProjectImporter : public ProjectExplorer::ProjectImporter
{
public:
    QmakeProjectImporter(const QString &path);

    QList<ProjectExplorer::BuildInfo *> import(const Utils::FileName &importPath, bool silent = false);
    QStringList importCandidates(const Utils::FileName &projectFilePath);
    ProjectExplorer::Target *preferredTarget(const QList<ProjectExplorer::Target *> &possibleTargets);

    void cleanupKit(ProjectExplorer::Kit *k);

    void makePermanent(ProjectExplorer::Kit *k);

private:
    ProjectExplorer::Kit *createTemporaryKit(QtSupport::BaseQtVersion *version,
                                             bool temporaryVersion,
                                             const Utils::FileName &parsedSpec,
                                             const QmakeProjectManager::QMakeStepConfig::TargetArchConfig &archConfig, const QMakeStepConfig::OsType &osType);
};

} // namespace Internal
} // namespace QmakeProjectManager

#endif // QMAKEPROJECTIMPORTER_H
