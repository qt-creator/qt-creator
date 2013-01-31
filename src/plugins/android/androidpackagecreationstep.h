/**************************************************************************
**
** Copyright (c) 2013 BogDan Vatra <bog_dan_ro@yahoo.com>
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

#ifndef ANDROIDPACKAGECREATIONSTEP_H
#define ANDROIDPACKAGECREATIONSTEP_H

#include "javaparser.h"

#include <projectexplorer/abi.h>
#include <projectexplorer/buildstep.h>

#include <QAbstractItemModel>

QT_BEGIN_NAMESPACE
class QProcess;
QT_END_NAMESPACE

namespace Android {
namespace Internal {

class AndroidPackageCreationStep : public ProjectExplorer::BuildStep
{
    Q_OBJECT
    friend class AndroidPackageCreationFactory;

public:
    AndroidPackageCreationStep(ProjectExplorer::BuildStepList *bsl);

    static bool removeDirectory(const QString &dirPath);
    static void stripAndroidLibs(const QStringList &files, ProjectExplorer::Abi::Architecture architecture);

    static const QLatin1String DefaultVersionNumber;

    void checkRequiredLibraries();
    void initCheckRequiredLibrariesForRun();
    void checkRequiredLibrariesForRun();

    Utils::FileName keystorePath();
    void setKeystorePath(const Utils::FileName &path);
    void setKeystorePassword(const QString &pwd);
    void setCertificateAlias(const QString &alias);
    void setCertificatePassword(const QString &pwd);
    void setOpenPackageLocation(bool open);
    QAbstractItemModel *keystoreCertificates();

protected:
    bool fromMap(const QVariantMap &map);
    QVariantMap toMap() const;

private slots:
    void handleBuildStdOutOutput();
    void handleBuildStdErrOutput();
    void keystorePassword();
    void certificatePassword();
    void showInGraphicalShell();
    void setQtLibs(const QStringList &qtLibs);
    void setPrebundledLibs(const QStringList &prebundledLibs);

signals:
    void updateRequiredLibrariesModels();

private:
    AndroidPackageCreationStep(ProjectExplorer::BuildStepList *buildConfig,
                             AndroidPackageCreationStep *other);

    void ctor();
    virtual bool init();
    virtual void run(QFutureInterface<bool> &fi);
    virtual ProjectExplorer::BuildStepConfigWidget *createConfigWidget();
    virtual bool immutable() const { return true; }
    bool createPackage();
    bool runCommand(QProcess *buildProc, const QString &program, const QStringList &arguments);
    void raiseError(const QString &shortMsg,
                    const QString &detailedMsg = QString());

    static const Core::Id CreatePackageId;

private:
    Utils::FileName m_keystorePath;
    QString m_keystorePasswd;
    QString m_certificateAlias;
    QString m_certificatePasswd;
    bool    m_openPackageLocation;
    JavaParser m_outputParser;

    // members to pass data from init() to run()
    Utils::FileName m_androidDir;
    Utils::FileName m_gdbServerSource;
    Utils::FileName m_gdbServerDestination;
    bool m_debugBuild;
    Utils::FileName m_antToolPath;
    Utils::FileName m_apkPathUnsigned;
    Utils::FileName m_apkPathSigned;
    Utils::FileName m_keystorePathForRun;
    QString m_certificatePasswdForRun;
    Utils::FileName m_jarSigner;
    Utils::FileName m_zipAligner;
    // more for checkLibraries
    Utils::FileName m_appPath;
    Utils::FileName m_readElf;
    QStringList m_qtLibs;
    QStringList m_availableQtLibs;
    QStringList m_prebundledLibs;
};

} // namespace Internal
} // namespace Android

#endif // ANDROIDPACKAGECREATIONSTEP_H
