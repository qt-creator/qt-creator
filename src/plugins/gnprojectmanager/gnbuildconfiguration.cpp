// Copyright (C) 2024 The Qt Company Ltd.
// Copyright (C) 2026 BogDan Vatra <bogdan@kde.org>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "gnbuildconfiguration.h"

#include "gnbuildstep.h"
#include "gnbuildsystem.h"
#include "gnkitaspect.h"
#include "gnpluginconstants.h"
#include "gnprojectmanagertr.h"
#include "gntools.h"

#include <coreplugin/editormanager/editormanager.h>

#include <projectexplorer/buildaspects.h>
#include <projectexplorer/buildinfo.h>
#include <projectexplorer/buildmanager.h>
#include <projectexplorer/buildstep.h>
#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/kit.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectconfiguration.h>
#include <projectexplorer/projectexplorer.h>

#include <utils/detailswidget.h>
#include <utils/layoutbuilder.h>
#include <utils/pathchooser.h>
#include <utils/qtcprocess.h>

#include <QFormLayout>
#include <QLineEdit>
#include <QPushButton>

using namespace ProjectExplorer;
using namespace Utils;

namespace GNProjectManager::Internal {

GNBuildConfiguration::GNBuildConfiguration(Target *target, Id id)
    : BuildConfiguration(target, id)
{
    setConfigWidgetDisplayName(Tr::tr("GN"));
    appendInitialBuildStep(Constants::GN_BUILD_STEP_ID);
    appendInitialCleanStep(Constants::GN_BUILD_STEP_ID);
}

void GNBuildConfiguration::build(const QString &target)
{
    auto gnBuildStep = qobject_cast<GNBuildStep *>(
        Utils::findOrDefault(buildSteps()->steps(), [](const BuildStep *bs) {
            return bs->id() == Constants::GN_BUILD_STEP_ID;
        }));

    QString originalBuildTarget;
    if (gnBuildStep) {
        originalBuildTarget = gnBuildStep->targetName();
        gnBuildStep->setBuildTarget(target);
    }

    BuildManager::buildList(buildSteps());

    if (gnBuildStep)
        gnBuildStep->setBuildTarget(originalBuildTarget);
}

FilePath GNBuildConfiguration::gnExecutable() const
{
    // Prefer kit's GN tool
    if (auto tool = GNKitAspect::gnTool(kit()))
        return tool->exe();

    // Fall back to PATH lookup
    const Environment env = environment();
    return env.searchInPath("gn");
}

QStringList GNBuildConfiguration::gnGenArgs() const
{
    QStringList args;
    if (!m_parameters.isEmpty())
        args << ProcessArgs::splitArgs(m_parameters, HostOsInfo::hostOs());
    return args;
}

const QString &GNBuildConfiguration::parameters() const
{
    return m_parameters;
}

void GNBuildConfiguration::setParameters(const QString &params)
{
    m_parameters = params;
    emit parametersChanged();
}

void GNBuildConfiguration::toMap(Store &map) const
{
    BuildConfiguration::toMap(map);
    map[Constants::BuildConfiguration::PARAMETERS_KEY] = m_parameters;
}

void GNBuildConfiguration::fromMap(const Store &map)
{
    BuildConfiguration::fromMap(map);
    m_parameters = map.value(Constants::BuildConfiguration::PARAMETERS_KEY).toString();
}

// GNBuildSettingsWidget

class GNBuildSettingsWidget : public QWidget
{
    Q_OBJECT

public:
    explicit GNBuildSettingsWidget(GNBuildConfiguration *buildCfg)
    {
        auto generateButton = new QPushButton(Tr::tr("Generate Project"));
        generateButton->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred);
        generateButton->setToolTip(Tr::tr("Run 'gn gen --ide=json' to (re)generate the project."));

        auto openArgsButton = new QPushButton(Tr::tr("Open args.gn"));
        openArgsButton->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred);
        openArgsButton->setToolTip(Tr::tr("Opens <build_dir>/args.gn in the editor"));

        auto parametersLineEdit = new QLineEdit;
        parametersLineEdit->setText(buildCfg->parameters());
        parametersLineEdit->setPlaceholderText(
            Tr::tr("Additional GN gen arguments (e.g. --args='is_debug=true')"));

        using namespace Layouting;
        Column {
            noMargin,
            Form {
                Tr::tr("Parameters:"), parametersLineEdit, br,
                buildCfg->buildDirectoryAspect(), br,
            },
            Row { generateButton, st, noMargin },
        }.attachTo(this);

        auto bs = static_cast<GNBuildSystem *>(buildCfg->buildSystem());

        connect(bs, &GNBuildSystem::updated, this, [=, this] {
            const auto args = bs->argsPath();
            if (m_openEditor && args.exists())
                Core::EditorManager::openEditor(args);
            m_openEditor = false;
        });

        connect(generateButton, &QPushButton::clicked, this, [=] { bs->generate(); });

        connect(openArgsButton, &QPushButton::clicked, this, [=, this] {
            const auto args = bs->argsPath();
            if (!args.exists()) {
                m_openEditor = true;
                bs->generate();
                return;
            }
            Core::EditorManager::openEditor(args);
        });
        connect(parametersLineEdit,
                &QLineEdit::editingFinished,
                this,
                [buildCfg, parametersLineEdit] {
                    buildCfg->setParameters(parametersLineEdit->text());
                });

        bs->triggerParsing();
    }

private:
    bool m_openEditor = false;
};

QWidget *GNBuildConfiguration::createConfigWidget()
{
    return new GNBuildSettingsWidget{this};
}

// GNBuildConfigurationFactory

class GNBuildConfigurationFactory final : public BuildConfigurationFactory
{
public:
    GNBuildConfigurationFactory()
    {
        registerBuildConfiguration<GNBuildConfiguration>(Constants::GN_BUILD_CONFIG_ID);
        setSupportedProjectType(Constants::GN_PROJECT_ID);
        setSupportedProjectMimeTypeName(Constants::Project::MIMETYPE);
        setBuildGenerator([](const Kit *, const FilePath &projectPath, bool forSetup) {
            QList<BuildInfo> result;

            // GN doens't have a way to do debug/release/profile builds
            BuildInfo buildInfo;
            buildInfo.buildSystemName = GNBuildSystem::name();
            buildInfo.typeName = "build";
            buildInfo.displayName = "Build";
            buildInfo.buildType = BuildConfiguration::Unknown;
            // GN is using ./out/kit by default
            buildInfo.buildDirectory = Constants::GN_BUILD_DIR;
            if (forSetup)
                buildInfo.projectName = projectPath.parentDir().fileName();
            buildInfo.enabledByDefault = true;
            result << buildInfo;

            return result;
        });
    }
};

void setupGNBuildConfiguration()
{
    static GNBuildConfigurationFactory theGNBuildConfigurationFactory;
}

} // namespace GNProjectManager::Internal

#include "gnbuildconfiguration.moc"
