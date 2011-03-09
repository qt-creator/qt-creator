/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of Qt Creator.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef MAEMOPACKAGECREATIONSTEP_H
#define MAEMOPACKAGECREATIONSTEP_H

#include <projectexplorer/buildstep.h>

QT_BEGIN_NAMESPACE
class QDateTime;
class QFile;
class QProcess;
QT_END_NAMESPACE

namespace Qt4ProjectManager {
class Qt4BuildConfiguration;

namespace Internal {
class MaemoDeployableListModel;
class AbstractQt4MaemoTarget;
class AbstractDebBasedQt4MaemoTarget;
class AbstractRpmBasedQt4MaemoTarget;
class Qt4MaemoDeployConfiguration;

class AbstractMaemoPackageCreationStep : public ProjectExplorer::BuildStep
{
    Q_OBJECT
public:
    virtual ~AbstractMaemoPackageCreationStep();

    virtual QString packageFilePath() const;

    QString versionString(QString *error) const;
    bool setVersionString(const QString &version, QString *error);

    static void preparePackagingProcess(QProcess *proc,
        const Qt4BuildConfiguration *bc, const QString &workingDir);

    QString projectName() const;
    const Qt4BuildConfiguration *qt4BuildConfiguration() const;
    AbstractQt4MaemoTarget *maemoTarget() const;
    AbstractDebBasedQt4MaemoTarget *debBasedMaemoTarget() const;
    AbstractRpmBasedQt4MaemoTarget *rpmBasedMaemoTarget() const;
    Qt4MaemoDeployConfiguration *deployConfig() const;

    static const QLatin1String DefaultVersionNumber;

signals:
    void packageFilePathChanged();
    void qtVersionChanged();

protected:
    AbstractMaemoPackageCreationStep(ProjectExplorer::BuildStepList *bsl,
        const QString &id);
    AbstractMaemoPackageCreationStep(ProjectExplorer::BuildStepList *buildConfig,
                             AbstractMaemoPackageCreationStep *other);

    void raiseError(const QString &shortMsg,
        const QString &detailedMsg = QString());
    bool callPackagingCommand(QProcess *proc, const QStringList &arguments);
    static QString replaceDots(const QString &name);
    QString buildDirectory() const;

private slots:
    void handleBuildOutput();
    void handleBuildConfigChanged();

private:
    void ctor();
    virtual bool init();
    virtual void run(QFutureInterface<bool> &fi);
    virtual ProjectExplorer::BuildStepConfigWidget *createConfigWidget();

    virtual bool createPackage(QProcess *buildProc)=0;
    virtual bool isMetaDataNewerThan(const QDateTime &packageDate) const=0;

    static QString nativePath(const QFile &file);
    bool packagingNeeded() const;

    const Qt4BuildConfiguration *m_lastBuildConfig;
};


class MaemoDebianPackageCreationStep : public AbstractMaemoPackageCreationStep
{
    Q_OBJECT
    friend class MaemoPackageCreationFactory;
public:
    MaemoDebianPackageCreationStep(ProjectExplorer::BuildStepList *bsl);

    static void ensureShlibdeps(QByteArray &rulesContent);

private:
    MaemoDebianPackageCreationStep(ProjectExplorer::BuildStepList *buildConfig,
        MaemoDebianPackageCreationStep *other);

    virtual bool createPackage(QProcess *buildProc);
    virtual bool isMetaDataNewerThan(const QDateTime &packageDate) const;

    void ctor();
    static QString packagingCommand(const Qt4BuildConfiguration *bc,
        const QString &commandName);
    bool copyDebianFiles(bool inSourceBuild);
    void addSedCmdToRulesFile(QByteArray &rulesFileContent, int &insertPos,
        const QString &desktopFilePath, const QByteArray &oldString,
        const QByteArray &newString);
    void addWorkaroundForHarmattanBug(QByteArray &rulesFileContent,
        int &insertPos, const MaemoDeployableListModel *model,
        const QString &desktopFileDir);
    void checkProjectName();
    void adaptRulesFile(const QString &rulesFilePath);

    static const QString CreatePackageId;
};

class MaemoRpmPackageCreationStep : public AbstractMaemoPackageCreationStep
{
    Q_OBJECT
    friend class MaemoPackageCreationFactory;
public:
    MaemoRpmPackageCreationStep(ProjectExplorer::BuildStepList *bsl);

private:
    virtual bool createPackage(QProcess *buildProc);
    virtual bool isMetaDataNewerThan(const QDateTime &packageDate) const;

    MaemoRpmPackageCreationStep(ProjectExplorer::BuildStepList *buildConfig,
        MaemoRpmPackageCreationStep *other);

    void ctor();
    static QString rpmBuildDir(const Qt4BuildConfiguration *bc);

    static const QString CreatePackageId;
};

class MaemoTarPackageCreationStep : public AbstractMaemoPackageCreationStep
{
    Q_OBJECT
    friend class MaemoPackageCreationFactory;
public:
    MaemoTarPackageCreationStep(ProjectExplorer::BuildStepList *bsl);

    virtual QString packageFilePath() const;
private:
    virtual bool createPackage(QProcess *buildProc);
    virtual bool isMetaDataNewerThan(const QDateTime &packageDate) const;
    virtual ProjectExplorer::BuildStepConfigWidget *createConfigWidget();

    MaemoTarPackageCreationStep(ProjectExplorer::BuildStepList *buildConfig,
        MaemoTarPackageCreationStep *other);

    void ctor();

    static const QString CreatePackageId;
};

} // namespace Internal
} // namespace Qt4ProjectManager

#endif // MAEMOPACKAGECREATIONSTEP_H
