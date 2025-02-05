// Copyright (c) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "haskellproject.h"

#include "haskellconstants.h"
#include "haskelltr.h"

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/buildinfo.h>
#include <projectexplorer/buildsystem.h>
#include <projectexplorer/buildtargetinfo.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectmanager.h>
#include <projectexplorer/projectnodes.h>
#include <projectexplorer/target.h>
#include <projectexplorer/treescanner.h>

#include <utils/algorithm.h>
#include <utils/detailswidget.h>
#include <utils/filepath.h>

#include <QFile>
#include <QHBoxLayout>
#include <QLabel>
#include <QTextStream>
#include <QVBoxLayout>

using namespace ProjectExplorer;
using namespace Utils;

namespace Haskell::Internal {

static QVector<QString> parseExecutableNames(const FilePath &projectFilePath)
{
    static const QString EXECUTABLE = "executable";
    static const int EXECUTABLE_LEN = EXECUTABLE.length();
    QVector<QString> result;
    QFile file(projectFilePath.toUrlishString());
    if (file.open(QFile::ReadOnly)) {
        QTextStream stream(&file);
        while (!stream.atEnd()) {
            const QString line = stream.readLine().trimmed();
            if (line.length() > EXECUTABLE_LEN && line.startsWith(EXECUTABLE)
                    && line.at(EXECUTABLE_LEN).isSpace())
                result.append(line.mid(EXECUTABLE_LEN + 1).trimmed());
        }
    }
    return result;
}

class HaskellBuildSystem final : public BuildSystem
{
public:
    HaskellBuildSystem(BuildConfiguration *bc);

    void triggerParsing() final;

    QString name() const final { return QLatin1String("haskell"); }

private:
    void updateApplicationTargets();

private:
    ParseGuard m_parseGuard;
    TreeScanner m_scanner;
};

HaskellBuildSystem::HaskellBuildSystem(BuildConfiguration *bc)
    : BuildSystem(bc)
{
    connect(&m_scanner, &TreeScanner::finished, this, [this] {
        auto root = std::make_unique<ProjectNode>(projectDirectory());
        root->setDisplayName(project()->displayName());
        std::vector<std::unique_ptr<FileNode>> nodePtrs
            = Utils::transform<std::vector>(m_scanner.release().takeAllFiles(), [](FileNode *fn) {
                  return std::unique_ptr<FileNode>(fn);
              });
        root->addNestedNodes(std::move(nodePtrs));
        setRootProjectNode(std::move(root));

        updateApplicationTargets();

        m_parseGuard.markAsSuccess();
        m_parseGuard = {};

        emitBuildSystemUpdated();
    });

    connect(project(), &Project::projectFileIsDirty, this, &BuildSystem::requestDelayedParse);
    requestDelayedParse();
}

void HaskellBuildSystem::triggerParsing()
{
    m_parseGuard = guardParsingRun();
    m_scanner.asyncScanForFiles(project()->projectDirectory());
}

void HaskellBuildSystem::updateApplicationTargets()
{
    const QVector<QString> executables = parseExecutableNames(projectFilePath());
    const Utils::FilePath projFilePath = projectFilePath();
    const QList<BuildTargetInfo> appTargets
        = Utils::transform<QList>(executables, [projFilePath](const QString &executable) {
              BuildTargetInfo bti;
              bti.displayName = executable;
              bti.buildKey = executable;
              bti.targetFilePath = FilePath::fromString(executable);
              bti.projectFilePath = projFilePath;
              bti.isQtcRunnable = true;
              return bti;
          });
    setApplicationTargets(appTargets);
    target()->updateDefaultRunConfigurations();
}

class HaskellProject final : public Project
{
public:
    explicit HaskellProject(const FilePath &fileName)
        : Project(Constants::C_HASKELL_PROJECT_MIMETYPE, fileName)
    {
        setId(Constants::C_HASKELL_PROJECT_ID);
        setDisplayName(fileName.toFileInfo().completeBaseName());
        setBuildSystemCreator<HaskellBuildSystem>();
    }
};

class HaskellBuildConfiguration final : public BuildConfiguration
{
public:
    HaskellBuildConfiguration(Target *target, Utils::Id id)
        : BuildConfiguration(target, id)
    {
        setConfigWidgetDisplayName(Tr::tr("General"));
        setInitializer([this](const BuildInfo &info) {
            setBuildDirectory(info.buildDirectory);
            setBuildType(info.buildType);
            setDisplayName(info.displayName);
        });
        appendInitialBuildStep(Constants::C_STACK_BUILD_STEP_ID);
    }

    QWidget *createConfigWidget() final;

    BuildType buildType() const final
    {
        return m_buildType;
    }

    void setBuildType(BuildType type)
    {
        m_buildType = type;
    }

private:
    BuildType m_buildType = BuildType::Release;
};

class HaskellBuildConfigurationWidget final : public QWidget
{
public:
    HaskellBuildConfigurationWidget(HaskellBuildConfiguration *bc)
    {
        setLayout(new QVBoxLayout);
        layout()->setContentsMargins(0, 0, 0, 0);
        auto box = new Utils::DetailsWidget;
        box->setState(Utils::DetailsWidget::NoSummary);
        layout()->addWidget(box);
        auto details = new QWidget;
        box->setWidget(details);
        details->setLayout(new QHBoxLayout);
        details->layout()->setContentsMargins(0, 0, 0, 0);
        details->layout()->addWidget(new QLabel(Tr::tr("Build directory:")));

        auto buildDirectoryInput = new Utils::PathChooser;
        buildDirectoryInput->setExpectedKind(Utils::PathChooser::Directory);
        buildDirectoryInput->setFilePath(bc->buildDirectory());
        details->layout()->addWidget(buildDirectoryInput);

        connect(bc,
                &BuildConfiguration::buildDirectoryChanged,
                buildDirectoryInput,
                [bc, buildDirectoryInput] {
                    buildDirectoryInput->setFilePath(bc->buildDirectory());
                });
        connect(buildDirectoryInput,
                &Utils::PathChooser::textChanged,
                bc,
                [bc, buildDirectoryInput](const QString &) {
                    bc->setBuildDirectory(buildDirectoryInput->unexpandedFilePath());
                });
    }
};

QWidget *HaskellBuildConfiguration::createConfigWidget()
{
    return new HaskellBuildConfigurationWidget(this);
}

class HaskellBuildConfigurationFactory final : public BuildConfigurationFactory
{
public:
    HaskellBuildConfigurationFactory()
    {
        registerBuildConfiguration<HaskellBuildConfiguration>("Haskell.BuildConfiguration");

        setSupportedProjectType(Constants::C_HASKELL_PROJECT_ID);
        setSupportedProjectMimeTypeName(Constants::C_HASKELL_PROJECT_MIMETYPE);

        setBuildGenerator([](const Kit *k, const Utils::FilePath &projectPath, bool forSetup)  {
            BuildInfo info;
            info.typeName = Tr::tr("Release");
            if (forSetup) {
                info.displayName = info.typeName;
                info.buildDirectory = projectPath.parentDir().pathAppended(".stack-work");
            }
            info.kitId = k->id();
            info.buildType = BuildConfiguration::BuildType::Release;
            return QList<BuildInfo>{info};
        });
    }
};

void setupHaskellProject()
{
    static const HaskellBuildConfigurationFactory theHaskellBuildConfigurationFactory;
    ProjectManager::registerProjectType<HaskellProject>(Constants::C_HASKELL_PROJECT_MIMETYPE);
}

} // Haskell::Internal
