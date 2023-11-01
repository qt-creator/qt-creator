// Copyright (C) 2016 BogDan Vatra <bog_dan_ro@yahoo.com>
// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <projectexplorer/abstractprocessstep.h>
#include <projectexplorer/processparameters.h>

#include <QVersionNumber>

QT_BEGIN_NAMESPACE
class QAbstractItemModel;
QT_END_NAMESPACE

namespace Android {
namespace Internal {

class AndroidBuildApkStep : public ProjectExplorer::AbstractProcessStep
{
    Q_OBJECT

public:
    AndroidBuildApkStep(ProjectExplorer::BuildStepList *bc, Utils::Id id);

    void fromMap(const Utils::Store &map) override;
    void toMap(Utils::Store &map) const override;

    // signing
    Utils::FilePath keystorePath() const;
    void setKeystorePath(const Utils::FilePath &path);
    void setKeystorePassword(const QString &pwd);
    void setCertificateAlias(const QString &alias);
    void setCertificatePassword(const QString &pwd);

    QAbstractItemModel *keystoreCertificates();
    bool signPackage() const;
    void setSignPackage(bool b);

    Utils::BoolAspect buildAAB{this};
    Utils::BoolAspect openPackageLocation{this};
    Utils::BoolAspect verboseOutput{this};

    bool addDebugger() const;
    void setAddDebugger(bool debug);

    QString buildTargetSdk() const;
    void setBuildTargetSdk(const QString &sdk);

    QVersionNumber buildToolsVersion() const;
    void setBuildToolsVersion(const QVersionNumber &version);

    QVariant data(Utils::Id id) const override;

private:
    void showInGraphicalShell();

    bool init() override;
    void setupOutputFormatter(Utils::OutputFormatter *formatter) override;
    QWidget *createConfigWidget() override;
    bool verifyKeystorePassword();
    bool verifyCertificatePassword();

    Tasking::GroupItem runRecipe() final;
    void stdError(const QString &output);

    void reportWarningOrError(const QString &message, ProjectExplorer::Task::TaskType type);
    void updateBuildToolsVersionInJsonFile();

    bool m_signPackage = false;
    bool m_openPackageLocationForRun = false;
    bool m_addDebugger = true;
    QString m_buildTargetSdk;
    QVersionNumber m_buildToolsVersion;

    Utils::FilePath m_keystorePath;
    QString m_keystorePasswd;
    QString m_certificateAlias;
    QString m_certificatePasswd;
    Utils::FilePath m_packagePath;

    ProjectExplorer::ProcessParameters m_concealedParams;
    bool m_skipBuilding = false;
    Utils::FilePath m_inputFile;
};

class AndroidBuildApkStepFactory : public ProjectExplorer::BuildStepFactory
{
public:
    AndroidBuildApkStepFactory();
};

} // namespace Internal
} // namespace Android
