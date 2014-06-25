/**************************************************************************
**
** Copyright (c) 2014 BogDan Vatra <bog_dan_ro@yahoo.com>
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
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

#ifndef QMAKEANDROIDBUILDAPKSTEP_H
#define QMAKEANDROIDBUILDAPKSTEP_H

#include <android/androidbuildapkstep.h>

namespace QmakeProjectManager {
namespace Internal {


class QmakeAndroidBuildApkStepFactory : public ProjectExplorer::IBuildStepFactory
{
    Q_OBJECT
public:
    explicit QmakeAndroidBuildApkStepFactory(QObject *parent = 0);

    QList<Core::Id> availableCreationIds(ProjectExplorer::BuildStepList *parent) const;
    QString displayNameForId(const Core::Id id) const;

    bool canCreate(ProjectExplorer::BuildStepList *parent,
                   const Core::Id id) const;
    ProjectExplorer::BuildStep *create(ProjectExplorer::BuildStepList *parent, const Core::Id id);

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
    QString proFilePathForInputFile() const;
    void setProFilePathForInputFile(const QString &path);

protected:
    friend class QmakeAndroidBuildApkStepFactory;
    QmakeAndroidBuildApkStep(ProjectExplorer::BuildStepList *bc,
        QmakeAndroidBuildApkStep *other);

    Utils::FileName androidPackageSourceDir() const;

protected:
    void ctor();
    bool init();
    ProjectExplorer::BuildStepConfigWidget *createConfigWidget();
    bool fromMap(const QVariantMap &map);
    QVariantMap toMap() const;

signals:
    // also on purpose emitted if the possible values of this changed
    void inputFileChanged();

private slots:
    void updateInputFile();

private:
    QString m_proFilePathForInputFile;
};

} // namespace Internal
} // namespace QmakeProjectManager

#endif // QMAKEANDROIDBUILDAPKSTEP_H
