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

#ifndef MAKEFILEPARSE_H
#define MAKEFILEPARSE_H

#include <utils/fileutils.h>
#include <qtsupport/baseqtversion.h>
#include <qmakeprojectmanager/qmakestep.h>

QT_BEGIN_NAMESPACE
class QLogggingCategory;
QT_END_NAMESPACE

namespace QmakeProjectManager {
namespace Internal {

struct QMakeAssignment
{
    QString variable;
    QString op;
    QString value;
};

class MakeFileParse
{
public:
    MakeFileParse(const QString &makefile);

    enum MakefileState { MakefileMissing, CouldNotParse, Okay };

    MakefileState makeFileState() const;
    Utils::FileName qmakePath() const;
    QString srcProFile() const;
    QMakeStepConfig config() const;

    QString unparsedArguments() const;

    QtSupport::BaseQtVersion::QmakeBuildConfigs
        effectiveBuildConfig(QtSupport::BaseQtVersion::QmakeBuildConfigs defaultBuildConfig) const;

    static const QLoggingCategory &logging();

private:
    void parseArgs(const QString &args, QList<QMakeAssignment> *assignments, QList<QMakeAssignment> *afterAssignments);
    void parseAssignments(QList<QMakeAssignment> *assignments);

    class QmakeBuildConfig
    {
    public:
        QmakeBuildConfig()
            : explicitDebug(false),
              explicitRelease(false),
              explicitBuildAll(false),
              explicitNoBuildAll(false)
        {}
        bool explicitDebug;
        bool explicitRelease;
        bool explicitBuildAll;
        bool explicitNoBuildAll;
    };

    MakefileState m_state;
    Utils::FileName m_qmakePath;
    QString m_srcProFile;

    QmakeBuildConfig m_qmakeBuildConfig;
    QMakeStepConfig m_config;
    QString m_unparsedArguments;
};

} // namespace Internal
} // namespace QmakeProjectManager


#endif // MAKEFILEPARSE_H
