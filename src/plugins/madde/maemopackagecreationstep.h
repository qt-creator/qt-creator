/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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

#ifndef MAEMOPACKAGECREATIONSTEP_H
#define MAEMOPACKAGECREATIONSTEP_H

#include <remotelinux/abstractpackagingstep.h>
#include <utils/environment.h>
#include <utils/fileutils.h>

QT_BEGIN_NAMESPACE
class QDateTime;
class QFile;
class QProcess;
QT_END_NAMESPACE

namespace Qt4ProjectManager { class Qt4BuildConfiguration; }
namespace RemoteLinux { class RemoteLinuxDeployConfiguration; }

namespace Madde {
namespace Internal {

class AbstractMaemoPackageCreationStep : public RemoteLinux::AbstractPackagingStep
{
    Q_OBJECT
public:
    virtual ~AbstractMaemoPackageCreationStep();

    QString versionString(QString *error) const;
    bool setVersionString(const QString &version, QString *error);

    static void preparePackagingProcess(QProcess *proc,
        const Qt4ProjectManager::Qt4BuildConfiguration *bc,
        const QString &workingDir);

    const Qt4ProjectManager::Qt4BuildConfiguration *qt4BuildConfiguration() const;

    static const QLatin1String DefaultVersionNumber;


protected:
    AbstractMaemoPackageCreationStep(ProjectExplorer::BuildStepList *bsl, const Core::Id id);
    AbstractMaemoPackageCreationStep(ProjectExplorer::BuildStepList *buildConfig,
                             AbstractMaemoPackageCreationStep *other);

    bool callPackagingCommand(QProcess *proc, const QStringList &arguments);
    QString replaceDots(const QString &name) const;
    virtual bool init();

private slots:
    void handleBuildOutput();

private:
    virtual void run(QFutureInterface<bool> &fi);
    virtual ProjectExplorer::BuildStepConfigWidget *createConfigWidget();

    virtual QString packageFileName() const;

    virtual bool createPackage(QProcess *buildProc, const QFutureInterface<bool> &fi) = 0;
    virtual bool isMetaDataNewerThan(const QDateTime &packageDate) const = 0;

    bool isPackagingNeeded() const;

    const Qt4ProjectManager::Qt4BuildConfiguration *m_lastBuildConfig;
    bool m_packagingNeeded;
    Utils::Environment m_environment;
    QString m_qmakeCommand;
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

    virtual bool init();
    virtual bool createPackage(QProcess *buildProc, const QFutureInterface<bool> &fi);
    virtual bool isMetaDataNewerThan(const QDateTime &packageDate) const;

    void ctor();
    static QString packagingCommand(const QString &maddeRoot, const QString &commandName);
    bool copyDebianFiles(bool inSourceBuild);
    void checkProjectName();
    bool adaptRulesFile(const QString &templatePath, const QString &rulesFilePath);

    QString m_maddeRoot;
    QString m_projectDirectory;
    QString m_pkgFileName;
    QString m_packageName;
    QString m_templatesDirPath;
    bool m_debugBuild;

    static const Core::Id CreatePackageId;
};

} // namespace Internal
} // namespace Madde

#endif // MAEMOPACKAGECREATIONSTEP_H
