/**************************************************************************
**
** Copyright (C) 2015 BogDan Vatra <bog_dan_ro@yahoo.com>
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

#ifndef QMAKEANDROIDBUILDAPKSTEP_H
#define QMAKEANDROIDBUILDAPKSTEP_H

#include <android/androidbuildapkstep.h>

namespace QmakeAndroidSupport {
namespace Internal {

class QmakeAndroidBuildApkStepFactory : public ProjectExplorer::IBuildStepFactory
{
    Q_OBJECT
public:
    explicit QmakeAndroidBuildApkStepFactory(QObject *parent = 0);

    QList<Core::Id> availableCreationIds(ProjectExplorer::BuildStepList *parent) const;
    QString displayNameForId(Core::Id id) const;

    bool canCreate(ProjectExplorer::BuildStepList *parent, Core::Id id) const;
    ProjectExplorer::BuildStep *create(ProjectExplorer::BuildStepList *parent, Core::Id id);

    bool canRestore(ProjectExplorer::BuildStepList *parent, const QVariantMap &map) const;
    ProjectExplorer::BuildStep *restore(ProjectExplorer::BuildStepList *parent, const QVariantMap &map);

    bool canClone(ProjectExplorer::BuildStepList *parent,
                  ProjectExplorer::BuildStep *product) const;
    ProjectExplorer::BuildStep *clone(ProjectExplorer::BuildStepList *parent,
                                      ProjectExplorer::BuildStep *product);
private:
    bool canHandle(ProjectExplorer::Target *t) const;

};

class QmakeAndroidBuildApkStep : public Android::AndroidBuildApkStep
{
    Q_OBJECT
public:
    QmakeAndroidBuildApkStep(ProjectExplorer::BuildStepList *bc);
    Utils::FileName proFilePathForInputFile() const;
    void setProFilePathForInputFile(const QString &path);


protected:
    friend class QmakeAndroidBuildApkStepFactory;
    QmakeAndroidBuildApkStep(ProjectExplorer::BuildStepList *bc,
        QmakeAndroidBuildApkStep *other);

    Utils::FileName androidPackageSourceDir() const;

protected:
    void ctor();
    bool init();
    void run(QFutureInterface<bool> &fi);
    void processStarted();
    ProjectExplorer::BuildStepConfigWidget *createConfigWidget();
    bool fromMap(const QVariantMap &map);
    QVariantMap toMap() const;

private:
    void setupProcessParameters(ProjectExplorer::ProcessParameters *pp,
                                ProjectExplorer::BuildConfiguration *bc,
                                const QStringList &arguments, const QString &command);
    QString m_command;
    QString m_argumentsPasswordConcealed;
    bool m_skipBuilding;
};

} // namespace Internal
} // namespace QmakeAndroidSupport

#endif // QMAKEANDROIDBUILDAPKSTEP_H
