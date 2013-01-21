#include "vcmakestep.h"

#include "msbuildoutputparser.h"
#include "vcprojectbuildconfiguration.h"
#include "vcprojectbuildoptionspage.h"
#include "vcprojectfile.h"
#include "vcprojectmanager.h"
#include "vcprojectmanagerconstants.h"

#include <coreplugin/icore.h>
#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectconfiguration.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/target.h>
#include <projectexplorer/task.h>

#include <QComboBox>
#include <QFormLayout>
#include <QLabel>
#include <QSettings>

namespace VcProjectManager {
namespace Internal {

namespace {
const char MS_ID[] = "VcProjectManager.MakeStep";
const char CLEAN_KEY[] = "VcProjectManager.MakeStep.Clean";
const char BUILD_TARGETS_KEY[] = "VcProjectManager.MakeStep.BuildTargets";
const char ADDITIONAL_ARGUMENTS_KEY[] = "VcProjectManager.MakeStep.AdditionalArguments";
}

VcMakeStep::VcMakeStep(ProjectExplorer::BuildStepList *bsl)
    : AbstractProcessStep(bsl, Core::Id(MS_ID))
{
}

bool VcMakeStep::init()
{
    VcProjectBuildConfiguration *bc = vcProjectBuildConfiguration();

    if (!bc || m_msBuildCommand.isEmpty()) {
        m_tasks.append(ProjectExplorer::Task(ProjectExplorer::Task::Error,
                                             tr("Qt Creator didn't detected any proper build tool for .vcproj files."),
                                             Utils::FileName(), -1,
                                             Core::Id(ProjectExplorer::Constants::TASK_CATEGORY_BUILDSYSTEM)));
        return true;
    }

    m_processParams = processParameters();
    m_processParams->setCommand(m_msBuildCommand);
    m_processParams->setMacroExpander(bc->macroExpander());
    m_processParams->setEnvironment(bc->environment());
    m_processParams->setWorkingDirectory(bc->buildDirectory());

    ProjectExplorer::Project *project = bc->target()->project();
    VcProjectFile* document = static_cast<VcProjectFile *>(project->document());
    m_processParams->setArguments(document->filePath());

    if (!m_buildArguments.isEmpty()) {
        QStringListIterator it(m_buildArguments);
        QString arguments(document->filePath());
        arguments += QLatin1Char(' ');

        while (it.hasNext()) {
            arguments += it.next();
            arguments += QLatin1Char(' ');
        }

        m_processParams->setArguments(arguments);
    }

    setOutputParser(new MsBuildOutputParser);

    return AbstractProcessStep::init();
}

void VcMakeStep::run(QFutureInterface<bool> &fi)
{
    bool canContinue = true;

    foreach (const ProjectExplorer::Task &t, m_tasks) {
        addTask(t);
        canContinue = false;
    }

    if (!canContinue) {
        emit addOutput(tr("Configuration is faulty. Check the Issues view for details."), BuildStep::MessageOutput);
        fi.reportResult(false);
        return;
    }

    m_futureInterface = &fi;
    m_futureInterface->setProgressRange(0, 100);
    AbstractProcessStep::run(fi);
    m_futureInterface->setProgressValue(100);
    m_futureInterface->reportFinished();
    m_futureInterface = 0;
}

ProjectExplorer::BuildStepConfigWidget *VcMakeStep::createConfigWidget()
{
    return new VcMakeStepConfigWidget(this);
}

bool VcMakeStep::immutable() const
{
    return false;
}

VcProjectBuildConfiguration *VcMakeStep::vcProjectBuildConfiguration() const
{
    return static_cast<VcProjectBuildConfiguration *>(buildConfiguration());
}

QString VcMakeStep::msBuildCommand() const
{
    return m_msBuildCommand;
}

QString VcMakeStep::msBuildVersion() const
{
    return m_msBuildVersion;
}

void VcMakeStep::setMsBuildCommand(const QString &msBuild, const QString &version)
{
    m_msBuildCommand = msBuild;
    m_msBuildVersion = version;
}

QStringList VcMakeStep::buildArguments() const
{
    return m_buildArguments;
}

QString VcMakeStep::buildArgumentsToString() const
{
    QStringListIterator it(m_buildArguments);
    QString buildArguments;

    if (it.hasNext())
        buildArguments += it.next();

    while (it.hasNext())
        buildArguments += QLatin1Char(' ') + it.next();

    return buildArguments;
}

void VcMakeStep::addBuildArgument(const QString &argument)
{
    m_buildArguments.append(argument);
}

void VcMakeStep::removeBuildArgument(const QString &buildArgument)
{
    m_buildArguments.removeAll(buildArgument);
}

QVariantMap VcMakeStep::toMap() const
{
    return BuildStep::toMap();
}

bool VcMakeStep::fromMap(const QVariantMap &map)
{
    return BuildStep::fromMap(map);
}

void VcMakeStep::stdOutput(const QString &line)
{
    AbstractProcessStep::stdOutput(line);
}

VcMakeStep::VcMakeStep(ProjectExplorer::BuildStepList *parent, VcMakeStep *vcMakeStep) :
    AbstractProcessStep(parent, vcMakeStep)
{
}

/////////////////////////
// VcMakeStepConfigWidget
/////////////////////////
VcMakeStepConfigWidget::VcMakeStepConfigWidget(VcMakeStep *makeStep) :
    m_makeStep(makeStep)
{
    QFormLayout *mainLayout = new QFormLayout(this);
    mainLayout->setMargin(0);
    mainLayout->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);

    m_msBuildComboBox = new QComboBox();
    m_msBuildPath = new QLabel();
    mainLayout->addRow(tr("Command:"), m_msBuildPath);
    mainLayout->addRow(tr("MS Build:"), m_msBuildComboBox);
    setLayout(mainLayout);

    if (m_makeStep) {
        VcProjectBuildConfiguration *bc = m_makeStep->vcProjectBuildConfiguration();
        ProjectExplorer::Project *project = bc->target()->project();
        VcManager *vcManager = static_cast<VcManager *>(project->projectManager());
        QVector<MsBuildInformation *> msBuildInfos = vcManager->buildOptionsPage()->msBuilds();

        if (msBuildInfos.size()) {
            foreach (const MsBuildInformation *msBuild, msBuildInfos) {
                if (!msBuild->m_executable.isEmpty()) {
                    QFileInfo fileInfo(msBuild->m_executable);
                    QString buildName = fileInfo.fileName() + QLatin1Char(' ') + msBuild->m_version;
                    QVariant msBuildFullPath(msBuild->m_executable + QLatin1Char(';') + msBuild->m_version);
                    m_msBuildComboBox->addItem(buildName, msBuildFullPath);
                }
            }
        }
        else {
            m_msBuildPath->setText(tr("No Ms Build tools found."));
            m_msBuildComboBox->setEnabled(false);
        }

        // NOTE(Radovan): place Ms Build settings read from our .user file for selected MS Build for this project
        // and setting that ms build as ms build command in this make step
        if (m_makeStep->msBuildCommand().isEmpty() && m_msBuildComboBox->count()) {
            m_msBuildComboBox->setCurrentIndex(0);
            onMsBuildSelectionChanged(0);
        }

        connect(m_msBuildComboBox, SIGNAL(currentIndexChanged(int)),
                this, SLOT(onMsBuildSelectionChanged(int)));

        connect(vcManager->buildOptionsPage(), SIGNAL(msBuildInfomationsUpdated()),
                this, SLOT(onMsBuildInformationsUpdated()));
    }
}

QString VcMakeStepConfigWidget::displayName() const
{
    return tr("Vc Make Step Config Widget");
}

QString VcMakeStepConfigWidget::summaryText() const
{
    VcProjectBuildConfiguration *bc = m_makeStep->vcProjectBuildConfiguration();
    ProjectExplorer::Project *project = bc->target()->project();
    VcProjectFile* document = static_cast<VcProjectFile *>(project->document());
    QFileInfo fileInfo(m_makeStep->msBuildCommand());
    return QString("<b>MsBuild:</b> %1 %2 %3").arg(fileInfo.fileName())
            .arg(document->filePath())
            .arg(m_makeStep->buildArgumentsToString());
}

void VcMakeStepConfigWidget::onMsBuildSelectionChanged(int index)
{
    if (m_makeStep && m_msBuildComboBox && 0 <= index && index < m_msBuildComboBox->count()) {
        QStringList data = m_msBuildComboBox->itemData(index).toString().split(QLatin1Char(';'));
        m_makeStep->setMsBuildCommand(data.at(0),  // ms build full path
                                      data.at(1)); // ms build version
        m_msBuildPath->setText(m_makeStep->msBuildCommand());
    }
}

void VcMakeStepConfigWidget::onMsBuildInformationsUpdated()
{
    disconnect(m_msBuildComboBox, SIGNAL(currentIndexChanged(int)),
               this, SLOT(onMsBuildSelectionChanged(int)));

    m_msBuildComboBox->clear();
    VcProjectBuildConfiguration *bc = m_makeStep->vcProjectBuildConfiguration();
    ProjectExplorer::Project *project = bc->target()->project();
    VcManager *vcManager = static_cast<VcManager *>(project->projectManager());
    QVector<MsBuildInformation *> msBuildInfos = vcManager->buildOptionsPage()->msBuilds();
    bool msBuildExists = false;

    foreach (const MsBuildInformation *msBuild, msBuildInfos) {
        if (!msBuild->m_executable.isEmpty()) {
            QFileInfo fileInfo(msBuild->m_executable);
            QString buildName = fileInfo.fileName() + QLatin1Char(' ') + msBuild->m_version;
            QVariant msBuildFullPath(msBuild->m_executable + QLatin1Char(';') + msBuild->m_version);
            m_msBuildComboBox->addItem(buildName, msBuildFullPath);

            if (!m_makeStep->msBuildCommand().isEmpty() && msBuild->m_executable == m_makeStep->msBuildCommand()) {
                m_msBuildComboBox->setCurrentIndex(m_msBuildComboBox->count() - 1);
                msBuildExists = true;
            }
        }
    }

    if (!msBuildExists) {
        if (m_msBuildComboBox->count()) {
            onMsBuildSelectionChanged(m_msBuildComboBox->currentIndex());

            if (!m_msBuildComboBox->isEnabled())
                m_msBuildComboBox->setEnabled(true);
        }

        else {
            m_makeStep->setMsBuildCommand(QString(), QString());
            m_msBuildPath->setText(tr("No Ms Build tools found."));
            m_msBuildComboBox->setEnabled(false);
        }
    }

    connect(m_msBuildComboBox, SIGNAL(currentIndexChanged(int)),
            this, SLOT(onMsBuildSelectionChanged(int)));
}

////////////////////
// VcMakeStepFactory
////////////////////
VcMakeStepFactory::VcMakeStepFactory(QObject *parent)
    : IBuildStepFactory(parent)
{
}

VcMakeStepFactory::~VcMakeStepFactory()
{
}

bool VcMakeStepFactory::canCreate(ProjectExplorer::BuildStepList *parent, const Core::Id id) const
{
    if (parent->target()->project()->id() == Constants::VC_PROJECT_ID)
        return id == MS_ID;
    return false;
}

ProjectExplorer::BuildStep* VcMakeStepFactory::create(ProjectExplorer::BuildStepList *parent, const Core::Id id)
{
    if (!canCreate(parent, id))
        return 0;
    VcMakeStep *step = new VcMakeStep(parent);
    return step;
}

bool VcMakeStepFactory::canClone(ProjectExplorer::BuildStepList *parent, ProjectExplorer::BuildStep *product) const
{
    return canCreate(parent, product->id());
}

ProjectExplorer::BuildStep *VcMakeStepFactory::clone(ProjectExplorer::BuildStepList *parent, ProjectExplorer::BuildStep *product)
{
    if (!canClone(parent, product))
        return 0;
    return new VcMakeStep(parent, static_cast<VcMakeStep *>(product));
}

bool VcMakeStepFactory::canRestore(ProjectExplorer::BuildStepList *parent, const QVariantMap &map) const
{
    return canCreate(parent, ProjectExplorer::idFromMap(map));
}

ProjectExplorer::BuildStep *VcMakeStepFactory::restore(ProjectExplorer::BuildStepList *parent, const QVariantMap &map)
{
    if (!canRestore(parent, map))
        return 0;
    VcMakeStep *bs = new VcMakeStep(parent);
    if (bs->fromMap(map))
        return bs;
    delete bs;
    return 0;
}

QList<Core::Id> VcMakeStepFactory::availableCreationIds(ProjectExplorer::BuildStepList *parent) const
{
    if (parent->target() && parent->target()->project()) {
        if (parent->target()->project()->id() == Constants::VC_PROJECT_ID) {
            return QList<Core::Id>() << Core::Id(MS_ID);
        }
    }
    return QList<Core::Id>();
}

QString VcMakeStepFactory::displayNameForId(const Core::Id id) const
{
    if (id == MS_ID)
        return tr("Make", "Vc Project Make Step Factory id.");
    return QString();
}


} // namespace Internal
} // namespace VcProjectManager
