#include "s60devicerunconfiguration.h"

#include "qt4project.h"
#include "qtversionmanager.h"
#include "profilereader.h"
#include "s60manager.h"
#include "s60devices.h"

#include <coreplugin/icore.h>
#include <coreplugin/messagemanager.h>
#include <utils/qtcassert.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/project.h>

using namespace ProjectExplorer;
using namespace Qt4ProjectManager::Internal;

// ======== S60DeviceRunConfiguration
S60DeviceRunConfiguration::S60DeviceRunConfiguration(Project *project, const QString &proFilePath)
    : RunConfiguration(project),
    m_proFilePath(proFilePath),
    m_cachedTargetInformationValid(false)
{
    if (!m_proFilePath.isEmpty())
        setName(tr("%1 on Device").arg(QFileInfo(m_proFilePath).completeBaseName()));
    else
        setName(tr("QtS60DeviceRunConfiguration"));

    connect(project, SIGNAL(activeBuildConfigurationChanged()),
            this, SLOT(invalidateCachedTargetInformation()));
}

S60DeviceRunConfiguration::~S60DeviceRunConfiguration()
{
}

QString S60DeviceRunConfiguration::type() const
{
    return "Qt4ProjectManager.DeviceRunConfiguration";
}

bool S60DeviceRunConfiguration::isEnabled() const
{
    Qt4Project *pro = qobject_cast<Qt4Project*>(project());
    QTC_ASSERT(pro, return false);
    ToolChain::ToolChainType type = pro->toolChainType(pro->activeBuildConfiguration());
    return type == ToolChain::GCCE; //TODO || type == ToolChain::ARMV5
}

QWidget *S60DeviceRunConfiguration::configurationWidget()
{
    return new S60DeviceRunConfigurationWidget(this);
}

void S60DeviceRunConfiguration::save(PersistentSettingsWriter &writer) const
{
    const QDir projectDir = QFileInfo(project()->file()->fileName()).absoluteDir();
    writer.saveValue("ProFile", projectDir.relativeFilePath(m_proFilePath));
    RunConfiguration::save(writer);
}

void S60DeviceRunConfiguration::restore(const PersistentSettingsReader &reader)
{
    RunConfiguration::restore(reader);
    const QDir projectDir = QFileInfo(project()->file()->fileName()).absoluteDir();
    m_proFilePath = projectDir.filePath(reader.restoreValue("ProFile").toString());
}

QString S60DeviceRunConfiguration::executable() const
{
    const_cast<S60DeviceRunConfiguration *>(this)->updateTarget();
    return m_executable;
}

void S60DeviceRunConfiguration::updateTarget()
{
    if (m_cachedTargetInformationValid)
        return;
    Qt4Project *pro = static_cast<Qt4Project *>(project());
    Qt4PriFileNode * priFileNode = static_cast<Qt4Project *>(project())->rootProjectNode()->findProFileFor(m_proFilePath);
    if (!priFileNode) {
        m_executable = QString::null;
        m_cachedTargetInformationValid = true;
        emit targetInformationChanged();
        return;
    }
    QtVersion *qtVersion = pro->qtVersion(pro->activeBuildConfiguration());
    ProFileReader *reader = priFileNode->createProFileReader();
    reader->setCumulative(false);
    reader->setQtVersion(qtVersion);

    // Find out what flags we pass on to qmake, this code is duplicated in the qmake step
    QtVersion::QmakeBuildConfig defaultBuildConfiguration = qtVersion->defaultBuildConfig();
    QtVersion::QmakeBuildConfig projectBuildConfiguration = QtVersion::QmakeBuildConfig(pro->qmakeStep()->value(pro->activeBuildConfiguration(), "buildConfiguration").toInt());
    QStringList addedUserConfigArguments;
    QStringList removedUserConfigArguments;
    if ((defaultBuildConfiguration & QtVersion::BuildAll) && !(projectBuildConfiguration & QtVersion::BuildAll))
        removedUserConfigArguments << "debug_and_release";
    if (!(defaultBuildConfiguration & QtVersion::BuildAll) && (projectBuildConfiguration & QtVersion::BuildAll))
        addedUserConfigArguments << "debug_and_release";
    if ((defaultBuildConfiguration & QtVersion::DebugBuild) && !(projectBuildConfiguration & QtVersion::DebugBuild))
        addedUserConfigArguments << "release";
    if (!(defaultBuildConfiguration & QtVersion::DebugBuild) && (projectBuildConfiguration & QtVersion::DebugBuild))
        addedUserConfigArguments << "debug";

    reader->setUserConfigCmdArgs(addedUserConfigArguments, removedUserConfigArguments);

    if (!reader->readProFile(m_proFilePath)) {
        delete reader;
        Core::ICore::instance()->messageManager()->printToOutputPane(tr("Could not parse %1. The QtS60 Device run configuration %2 can not be started.").arg(m_proFilePath).arg(name()));
        return;
    }

    QString baseDir = S60Manager::instance()->devices()->deviceForId(
            S60Manager::instance()->deviceIdFromDetectionSource(qtVersion->autodetectionSource())).epocRoot;
    QString qmakeBuildConfig = "urel";
    if (projectBuildConfiguration & QtVersion::DebugBuild)
        qmakeBuildConfig = "udeb";
    baseDir += "/epoc32/release/winscw/" + qmakeBuildConfig;

    m_executable = QDir::toNativeSeparators(
            QDir::cleanPath(baseDir + QLatin1Char('/') + reader->value("TARGET")));
    m_executable += QLatin1String(".exe");

    delete reader;
    m_cachedTargetInformationValid = true;
    emit targetInformationChanged();
}

void S60DeviceRunConfiguration::invalidateCachedTargetInformation()
{
    m_cachedTargetInformationValid = false;
    emit targetInformationChanged();
}

// ======== S60DeviceRunConfigurationWidget

S60DeviceRunConfigurationWidget::S60DeviceRunConfigurationWidget(S60DeviceRunConfiguration *runConfiguration,
                                                                     QWidget *parent)
    : QWidget(parent),
    m_runConfiguration(runConfiguration)
{
    QFormLayout *toplayout = new QFormLayout();
    toplayout->setMargin(0);
    setLayout(toplayout);

    QLabel *nameLabel = new QLabel(tr("Name:"));
    m_nameLineEdit = new QLineEdit(m_runConfiguration->name());
    nameLabel->setBuddy(m_nameLineEdit);
    toplayout->addRow(nameLabel, m_nameLineEdit);

    m_executableLabel = new QLabel(m_runConfiguration->executable());
    toplayout->addRow(tr("Executable:"), m_executableLabel);

    connect(m_nameLineEdit, SIGNAL(textEdited(QString)),
        this, SLOT(nameEdited(QString)));
    connect(m_runConfiguration, SIGNAL(targetInformationChanged()),
            this, SLOT(updateTargetInformation()));
}

void S60DeviceRunConfigurationWidget::nameEdited(const QString &text)
{
    m_runConfiguration->setName(text);
}

void S60DeviceRunConfigurationWidget::updateTargetInformation()
{
    m_executableLabel->setText(m_runConfiguration->executable());
}

// ======== S60DeviceRunConfigurationFactory

S60DeviceRunConfigurationFactory::S60DeviceRunConfigurationFactory(QObject *parent)
    : IRunConfigurationFactory(parent)
{
}

S60DeviceRunConfigurationFactory::~S60DeviceRunConfigurationFactory()
{
}

bool S60DeviceRunConfigurationFactory::canRestore(const QString &type) const
{
    return type == "Qt4ProjectManager.DeviceRunConfiguration";
}

QStringList S60DeviceRunConfigurationFactory::availableCreationTypes(Project *pro) const
{
    Qt4Project *qt4project = qobject_cast<Qt4Project *>(pro);
    if (qt4project) {
        QStringList applicationProFiles;
        QList<Qt4ProFileNode *> list = qt4project->applicationProFiles();
        foreach (Qt4ProFileNode * node, list) {
            applicationProFiles.append("QtS60DeviceRunConfiguration." + node->path());
        }
        return applicationProFiles;
    } else {
        return QStringList();
    }
}

QString S60DeviceRunConfigurationFactory::displayNameForType(const QString &type) const
{
    QString fileName = type.mid(QString("QtS60DeviceRunConfiguration.").size());
    return tr("%1 on Device").arg(QFileInfo(fileName).completeBaseName());
}

QSharedPointer<RunConfiguration> S60DeviceRunConfigurationFactory::create(Project *project, const QString &type)
{
    Qt4Project *p = qobject_cast<Qt4Project *>(project);
    Q_ASSERT(p);
    if (type.startsWith("QtS60DeviceRunConfiguration.")) {
        QString fileName = type.mid(QString("QtS60DeviceRunConfiguration.").size());
        return QSharedPointer<RunConfiguration>(new S60DeviceRunConfiguration(p, fileName));
    }
    Q_ASSERT(type == "Qt4ProjectManager.DeviceRunConfiguration");
    // The right path is set in restoreSettings
    QSharedPointer<RunConfiguration> rc(new S60DeviceRunConfiguration(p, QString::null));
    return rc;
}

// ======== S60DeviceRunConfigurationRunner

S60DeviceRunConfigurationRunner::S60DeviceRunConfigurationRunner(QObject *parent)
    : IRunConfigurationRunner(parent)
{
}

bool S60DeviceRunConfigurationRunner::canRun(QSharedPointer<RunConfiguration> runConfiguration, const QString &mode)
{
    return (mode == ProjectExplorer::Constants::RUNMODE)
            && (!runConfiguration.dynamicCast<S60DeviceRunConfiguration>().isNull());
}

RunControl* S60DeviceRunConfigurationRunner::run(QSharedPointer<RunConfiguration> runConfiguration, const QString &mode)
{
    QSharedPointer<S60DeviceRunConfiguration> rc = runConfiguration.dynamicCast<S60DeviceRunConfiguration>();
    Q_ASSERT(!rc.isNull());
    Q_ASSERT(mode == ProjectExplorer::Constants::RUNMODE);

    S60DeviceRunControl *runControl = new S60DeviceRunControl(rc);
    return runControl;
}

// ======== S60DeviceRunControl

S60DeviceRunControl::S60DeviceRunControl(QSharedPointer<RunConfiguration> runConfiguration)
    : RunControl(runConfiguration)
{
    connect(&m_applicationLauncher, SIGNAL(applicationError(QString)),
            this, SLOT(slotError(QString)));
    connect(&m_applicationLauncher, SIGNAL(appendOutput(QString)),
            this, SLOT(slotAddToOutputWindow(QString)));
    connect(&m_applicationLauncher, SIGNAL(processExited(int)),
            this, SLOT(processExited(int)));
    connect(&m_applicationLauncher, SIGNAL(bringToForegroundRequested(qint64)),
            this, SLOT(bringApplicationToForeground(qint64)));
}

void S60DeviceRunControl::start()
{
    QSharedPointer<S60DeviceRunConfiguration> rc = runConfiguration().dynamicCast<S60DeviceRunConfiguration>();
    Q_ASSERT(!rc.isNull());

    // stuff like the EPOCROOT and EPOCDEVICE env variable
    Environment env = Environment::systemEnvironment();
    static_cast<Qt4Project *>(rc->project())->toolChain(rc->project()->activeBuildConfiguration())->addToEnvironment(env);
    m_applicationLauncher.setEnvironment(env.toStringList());

    m_executable = rc->executable();

    m_applicationLauncher.start(ApplicationLauncher::Gui,
                                m_executable, QStringList());
    emit started();

    emit addToOutputWindow(this, tr("Starting %1...").arg(QDir::toNativeSeparators(m_executable)));
}

void S60DeviceRunControl::stop()
{
    m_applicationLauncher.stop();
}

bool S60DeviceRunControl::isRunning() const
{
    return m_applicationLauncher.isRunning();
}

void S60DeviceRunControl::slotError(const QString & err)
{
    emit error(this, err);
    emit finished();
}

void S60DeviceRunControl::slotAddToOutputWindow(const QString &line)
{
    if (line.contains("Qt"))
        emit addToOutputWindowInline(this, line);
}

void S60DeviceRunControl::processExited(int exitCode)
{
    emit addToOutputWindow(this, tr("%1 exited with code %2").arg(QDir::toNativeSeparators(m_executable)).arg(exitCode));
    emit finished();
}
