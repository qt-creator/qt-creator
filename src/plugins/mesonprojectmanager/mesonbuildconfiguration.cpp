// Copyright (C) 2020 Alexis Jeandet.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "mesonbuildconfiguration.h"

#include "buildoptionsmodel.h"
#include "mesonbuildsystem.h"
#include "mesonpluginconstants.h"
#include "mesonprojectmanagertr.h"
#include "mesonwrapper.h"
#include "ninjabuildstep.h"

#include <coreplugin/find/itemviewfind.h>

#include <projectexplorer/buildaspects.h>
#include <projectexplorer/buildinfo.h>
#include <projectexplorer/buildmanager.h>
#include <projectexplorer/buildstep.h>
#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/kit.h>
#include <projectexplorer/namedwidget.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectconfiguration.h>
#include <projectexplorer/projectexplorer.h>

#include <utils/categorysortfiltermodel.h>
#include <utils/detailswidget.h>
#include <utils/headerviewstretcher.h>
#include <utils/itemviews.h>
#include <utils/layoutbuilder.h>
#include <utils/process.h>
#include <utils/progressindicator.h>
#include <utils/utilsicons.h>

#include <QLayout>
#include <QPushButton>

using namespace ProjectExplorer;
using namespace Utils;

namespace MesonProjectManager::Internal {

const QHash<QString, MesonBuildType> buildTypesByName = {
    {"plain", MesonBuildType::plain},
    {"debug", MesonBuildType::debug},
    {"debugoptimized", MesonBuildType::debugoptimized},
    {"release", MesonBuildType::release},
    {"minsize", MesonBuildType::minsize},
    {"custom", MesonBuildType::custom}
 };

static MesonBuildType mesonBuildType(const QString &typeName)
{
    return buildTypesByName.value(typeName, MesonBuildType::custom);
}

static FilePath shadowBuildDirectory(const FilePath &projectFilePath,
                                     const Kit *k,
                                     const QString &bcName,
                                     BuildConfiguration::BuildType buildType)
{
    if (projectFilePath.isEmpty())
        return {};

    const QString projectName = projectFilePath.parentDir().fileName();
    return MesonBuildConfiguration::buildDirectoryFromTemplate(
        Project::projectDirectory(projectFilePath), projectFilePath,
        projectName, k, bcName, buildType, "meson");
}

MesonBuildConfiguration::MesonBuildConfiguration(ProjectExplorer::Target *target, Id id)
    : BuildConfiguration(target, id)
{
    appendInitialBuildStep(Constants::MESON_BUILD_STEP_ID);
    appendInitialCleanStep(Constants::MESON_BUILD_STEP_ID);
    setInitializer([this, target](const ProjectExplorer::BuildInfo &info) {
        m_buildType = mesonBuildType(info.typeName);
        auto k = target->kit();
        if (info.buildDirectory.isEmpty()) {
            setBuildDirectory(shadowBuildDirectory(target->project()->projectFilePath(),
                                                   k,
                                                   info.displayName,
                                                   info.buildType));
        }
        m_buildSystem = new MesonBuildSystem{this};
    });
}

MesonBuildConfiguration::~MesonBuildConfiguration()
{
    delete m_buildSystem;
}

ProjectExplorer::BuildSystem *MesonBuildConfiguration::buildSystem() const
{
    return m_buildSystem;
}

void MesonBuildConfiguration::build(const QString &target)
{
    auto mesonBuildStep = qobject_cast<NinjaBuildStep *>(
        Utils::findOrDefault(buildSteps()->steps(), [](const ProjectExplorer::BuildStep *bs) {
            return bs->id() == Constants::MESON_BUILD_STEP_ID;
        }));

    QString originalBuildTarget;
    if (mesonBuildStep) {
        originalBuildTarget = mesonBuildStep->targetName();
        mesonBuildStep->setBuildTarget(target);
    }

    ProjectExplorer::BuildManager::buildList(buildSteps());

    if (mesonBuildStep)
        mesonBuildStep->setBuildTarget(originalBuildTarget);
}

static QString mesonBuildTypeName(MesonBuildType type)
{
    return buildTypesByName.key(type, "custom");
}

QStringList MesonBuildConfiguration::mesonConfigArgs()
{
    return Utils::ProcessArgs::splitArgs(m_parameters, HostOsInfo::hostOs())
        + QStringList{QString("-Dbuildtype=%1").arg(mesonBuildTypeName(m_buildType))};
}

const QString &MesonBuildConfiguration::parameters() const
{
    return m_parameters;
}

void MesonBuildConfiguration::setParameters(const QString &params)
{
    m_parameters = params;
    emit parametersChanged();
}

void MesonBuildConfiguration::toMap(Store &map) const
{
    ProjectExplorer::BuildConfiguration::toMap(map);
    map[Constants::BuildConfiguration::BUILD_TYPE_KEY] = mesonBuildTypeName(m_buildType);
    map[Constants::BuildConfiguration::PARAMETERS_KEY] = m_parameters;
}

void MesonBuildConfiguration::fromMap(const Store &map)
{
    ProjectExplorer::BuildConfiguration::fromMap(map);
    m_buildSystem = new MesonBuildSystem{this};
    m_buildType = mesonBuildType(
        map.value(Constants::BuildConfiguration::BUILD_TYPE_KEY).toString());
    m_parameters = map.value(Constants::BuildConfiguration::PARAMETERS_KEY).toString();
}

class MesonBuildSettingsWidget : public NamedWidget
{
public:
    explicit MesonBuildSettingsWidget(MesonBuildConfiguration *buildCfg)
        : NamedWidget(Tr::tr("Meson")), m_progressIndicator(ProgressIndicatorSize::Large)
    {
        auto configureButton = new QPushButton(Tr::tr("Apply Configuration Changes"));
        configureButton->setEnabled(false);
        configureButton->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred);

        auto wipeButton = new QPushButton(Tr::tr("Wipe Project"));
        wipeButton->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Preferred);
        wipeButton->setIcon(Utils::Icons::WARNING.icon());
        wipeButton->setToolTip(Tr::tr("Wipes build directory and reconfigures using previous command "
                                      "line options.\nUseful if build directory is corrupted or when "
                                      "rebuilding with a newer version of Meson."));

        auto container = new DetailsWidget;

        auto details = new QWidget;

        container->setState(DetailsWidget::NoSummary);
        container->setWidget(details);

        auto parametersLineEdit = new QLineEdit;

        auto optionsFilterLineEdit = new FancyLineEdit;

        auto optionsTreeView = new TreeView;
        optionsTreeView->setMinimumHeight(300);
        optionsTreeView->setFrameShape(QFrame::NoFrame);
        optionsTreeView->setSelectionBehavior(QAbstractItemView::SelectItems);
        optionsTreeView->setSortingEnabled(true);

        using namespace Layouting;
        Column {
            noMargin,
            Form {
                 Tr::tr("Parameters:"), parametersLineEdit, br,
                buildCfg->buildDirectoryAspect(), br
            },
            optionsFilterLineEdit,
            optionsTreeView,
        }.attachTo(details);

        Column {
               noMargin,
            container,
            Row { configureButton, wipeButton, noMargin }
        }.attachTo(this);

        parametersLineEdit->setText(buildCfg->parameters());
        optionsFilterLineEdit->setFiltering(true);

        optionsTreeView->sortByColumn(0, Qt::AscendingOrder);

        QFrame *findWrapper
            = Core::ItemViewFind::createSearchableWrapper(optionsTreeView,
                                                          Core::ItemViewFind::LightColored);
        findWrapper->setFrameStyle(QFrame::StyledPanel);
        m_progressIndicator.attachToWidget(findWrapper);
        m_progressIndicator.raise();
        m_progressIndicator.hide();
        details->layout()->addWidget(findWrapper);

        m_showProgressTimer.setSingleShot(true);
        m_showProgressTimer.setInterval(50); // don't show progress for < 50ms tasks
        connect(&m_showProgressTimer, &QTimer::timeout,
                this, [this] { m_progressIndicator.show(); });
        connect(&m_optionsModel, &BuidOptionsModel::configurationChanged, this, [configureButton] {
            configureButton->setEnabled(true);
        });
        m_optionsFilter.setSourceModel(&m_optionsModel);
        m_optionsFilter.setSortRole(Qt::DisplayRole);
        m_optionsFilter.setFilterKeyColumn(-1);

        optionsTreeView->setModel(&m_optionsFilter);
        optionsTreeView->setItemDelegate(new BuildOptionDelegate{optionsTreeView});

        MesonBuildSystem *bs = static_cast<MesonBuildSystem *>(buildCfg->buildSystem());
        connect(buildCfg->target(), &ProjectExplorer::Target::parsingFinished,
                this, [this, bs, optionsTreeView](bool success) {
            if (success) {
                m_optionsModel.setConfiguration(bs->buildOptions());
            } else {
                m_optionsModel.clear();
            }
            optionsTreeView->expandAll();
            optionsTreeView->resizeColumnToContents(0);
            optionsTreeView->setEnabled(true);
            m_showProgressTimer.stop();
            m_progressIndicator.hide();
        });

        connect(bs, &MesonBuildSystem::parsingStarted, this, [this, optionsTreeView] {
            if (!m_showProgressTimer.isActive()) {
                optionsTreeView->setEnabled(false);
                m_showProgressTimer.start();
            }
        });

        connect(&m_optionsModel, &BuidOptionsModel::dataChanged, this, [bs, this] {
            bs->setMesonConfigArgs(this->m_optionsModel.changesAsMesonArgs());
        });

        connect(&m_optionsFilter, &QAbstractItemModel::modelReset, this, [optionsTreeView] {
            optionsTreeView->expandAll();
            optionsTreeView->resizeColumnToContents(0);
        });

        connect(optionsFilterLineEdit, &QLineEdit::textChanged, &m_optionsFilter, [this](const QString &txt) {
            m_optionsFilter.setFilterRegularExpression(
                QRegularExpression(QRegularExpression::escape(txt),
                                   QRegularExpression::CaseInsensitiveOption));
        });

        connect(optionsTreeView,
                &Utils::TreeView::activated,
                optionsTreeView,
                [tree = optionsTreeView](const QModelIndex &idx) { tree->edit(idx); });

        connect(configureButton, &QPushButton::clicked,
                this, [this, bs, configureButton, optionsTreeView] {
            optionsTreeView->setEnabled(false);
            configureButton->setEnabled(false);
            m_showProgressTimer.start();
            bs->configure();
        });
        connect(wipeButton, &QPushButton::clicked,
                this, [this, bs, configureButton, optionsTreeView] {
            optionsTreeView->setEnabled(false);
            configureButton->setEnabled(false);
            m_showProgressTimer.start();
            bs->wipe();
        });
        connect(parametersLineEdit, &QLineEdit::editingFinished, this, [ buildCfg, parametersLineEdit] {
            buildCfg->setParameters(parametersLineEdit->text());
        });
        bs->triggerParsing();
    }

private:
    BuidOptionsModel m_optionsModel;
    CategorySortFilterModel m_optionsFilter;
    ProgressIndicator m_progressIndicator;
    QTimer m_showProgressTimer;
};

NamedWidget *MesonBuildConfiguration::createConfigWidget()
{
    return new MesonBuildSettingsWidget{this};
}

static BuildConfiguration::BuildType buildType(MesonBuildType type)
{
    switch (type) {
    case MesonBuildType::plain:
        return BuildConfiguration::Unknown;
    case MesonBuildType::debug:
        return BuildConfiguration::Debug;
    case MesonBuildType::debugoptimized:
        return BuildConfiguration::Profile;
    case MesonBuildType::release:
        return BuildConfiguration::Release;
    case MesonBuildType::minsize:
        return BuildConfiguration::Release;
    default:
        return BuildConfiguration::Unknown;
    }
}

static QString mesonBuildTypeDisplayName(MesonBuildType type)
{
    switch (type) {
    case MesonBuildType::plain:
        return {"Plain"};
    case MesonBuildType::debug:
        return {"Debug"};
    case MesonBuildType::debugoptimized:
        return {"Debug With Optimizations"};
    case MesonBuildType::release:
        return {"Release"};
    case MesonBuildType::minsize:
        return {"Minimum Size"};
    default:
        return {"Custom"};
    }
}

BuildInfo createBuildInfo(MesonBuildType type)
{
    BuildInfo bInfo;
    bInfo.typeName = mesonBuildTypeName(type);
    bInfo.displayName = mesonBuildTypeDisplayName(type);
    bInfo.buildType = buildType(type);
    return bInfo;
}

MesonBuildConfigurationFactory::MesonBuildConfigurationFactory()
{
    registerBuildConfiguration<MesonBuildConfiguration>(Constants::MESON_BUILD_CONFIG_ID);
    setSupportedProjectType(Constants::Project::ID);
    setSupportedProjectMimeTypeName(Constants::Project::MIMETYPE);
    setBuildGenerator(
        [](const ProjectExplorer::Kit *k, const Utils::FilePath &projectPath, bool forSetup) {
            QList<ProjectExplorer::BuildInfo> result;

            Utils::FilePath path = forSetup
                                       ? Project::projectDirectory(projectPath)
                                       : projectPath;
            for (const auto &bType : {MesonBuildType::debug,
                                      MesonBuildType::release,
                                      MesonBuildType::debugoptimized,
                                      MesonBuildType::minsize}) {
                auto bInfo = createBuildInfo(bType);
                if (forSetup)
                    bInfo.buildDirectory = shadowBuildDirectory(projectPath,
                                                                k,
                                                                bInfo.typeName,
                                                                bInfo.buildType);
                result << bInfo;
            }
            return result;
        });
}

} // MesonProjectManager::Internal
