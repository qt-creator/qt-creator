/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#ifndef MAEMOPACKAGECREATIONSTEP_H
#define MAEMOPACKAGECREATIONSTEP_H

#include <projectexplorer/buildstep.h>

QT_BEGIN_NAMESPACE
class QDateTime;
class QFile;
class QFileInfo;
class QProcess;
QT_END_NAMESPACE

namespace Qt4ProjectManager { class Qt4BuildConfiguration; }

namespace RemoteLinux {
namespace Internal {
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
        const Qt4ProjectManager::Qt4BuildConfiguration *bc,
        const QString &workingDir);

    QString projectName() const;
    const Qt4ProjectManager::Qt4BuildConfiguration *qt4BuildConfiguration() const;
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

    virtual bool createPackage(QProcess *buildProc, const QFutureInterface<bool> &fi)=0;
    virtual bool isMetaDataNewerThan(const QDateTime &packageDate) const=0;

    static QString nativePath(const QFile &file);
    bool packagingNeeded() const;

    const Qt4ProjectManager::Qt4BuildConfiguration *m_lastBuildConfig;
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

    virtual bool createPackage(QProcess *buildProc, const QFutureInterface<bool> &fi);
    virtual bool isMetaDataNewerThan(const QDateTime &packageDate) const;

    void ctor();
    static QString packagingCommand(const Qt4ProjectManager::Qt4BuildConfiguration *bc,
        const QString &commandName);
    bool copyDebianFiles(bool inSourceBuild);
    void checkProjectName();
    bool adaptRulesFile(const QString &templatePath, const QString &rulesFilePath);

    static const QString CreatePackageId;
};

class MaemoRpmPackageCreationStep : public AbstractMaemoPackageCreationStep
{
    Q_OBJECT
    friend class MaemoPackageCreationFactory;
public:
    MaemoRpmPackageCreationStep(ProjectExplorer::BuildStepList *bsl);

private:
    virtual bool createPackage(QProcess *buildProc, const QFutureInterface<bool> &fi);
    virtual bool isMetaDataNewerThan(const QDateTime &packageDate) const;

    MaemoRpmPackageCreationStep(ProjectExplorer::BuildStepList *buildConfig,
        MaemoRpmPackageCreationStep *other);

    void ctor();
    static QString rpmBuildDir(const Qt4ProjectManager::Qt4BuildConfiguration *bc);

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
    virtual bool createPackage(QProcess *buildProc, const QFutureInterface<bool> &fi);
    virtual bool isMetaDataNewerThan(const QDateTime &packageDate) const;
    virtual ProjectExplorer::BuildStepConfigWidget *createConfigWidget();

    MaemoTarPackageCreationStep(ProjectExplorer::BuildStepList *buildConfig,
        MaemoTarPackageCreationStep *other);

    void ctor();
    bool appendFile(QFile &tarFile, const QFileInfo &fileInfo,
        const QString &remoteFilePath, const QFutureInterface<bool> &fi);
    bool writeHeader(QFile &tarFile, const QFileInfo &fileInfo,
        const QString &remoteFilePath);

    static const QString CreatePackageId;
};

} // namespace Internal
} // namespace RemoteLinux

#endif // MAEMOPACKAGECREATIONSTEP_H
