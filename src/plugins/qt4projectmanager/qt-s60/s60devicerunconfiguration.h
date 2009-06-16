#ifndef S60DEVICERUNCONFIGURATION_H
#define S60DEVICERUNCONFIGURATION_H

#include <projectexplorer/runconfiguration.h>
#include <projectexplorer/applicationlauncher.h>

#include <QtGui/QWidget>
#include <QtGui/QLabel>
#include <QtGui/QLineEdit>

namespace Qt4ProjectManager {
namespace Internal {

class S60DeviceRunConfiguration : public ProjectExplorer::RunConfiguration
{
    Q_OBJECT
public:
    S60DeviceRunConfiguration(ProjectExplorer::Project *project, const QString &proFilePath);
    ~S60DeviceRunConfiguration();

    QString type() const;
    bool isEnabled() const;
    QWidget *configurationWidget();
    void save(ProjectExplorer::PersistentSettingsWriter &writer) const;
    void restore(const ProjectExplorer::PersistentSettingsReader &reader);

    QString executable() const;

signals:
    void targetInformationChanged();

private slots:
    void invalidateCachedTargetInformation();

private:
    void updateTarget();

    QString m_proFilePath;
    QString m_executable;
    bool m_cachedTargetInformationValid;
};

class S60DeviceRunConfigurationWidget : public QWidget
{
    Q_OBJECT
public:
    S60DeviceRunConfigurationWidget(S60DeviceRunConfiguration *runConfiguration,
                                      QWidget *parent = 0);

private slots:
    void nameEdited(const QString &text);
    void updateTargetInformation();

private:
    S60DeviceRunConfiguration *m_runConfiguration;
    QLineEdit *m_nameLineEdit;
    QLabel *m_executableLabel;
};

class S60DeviceRunConfigurationFactory : public ProjectExplorer::IRunConfigurationFactory
{
    Q_OBJECT
public:
    S60DeviceRunConfigurationFactory(QObject *parent);
    ~S60DeviceRunConfigurationFactory();
    bool canRestore(const QString &type) const;
    QStringList availableCreationTypes(ProjectExplorer::Project *pro) const;
    // used to translate the types to names to display to the user
    QString displayNameForType(const QString &type) const;
    QSharedPointer<ProjectExplorer::RunConfiguration> create(ProjectExplorer::Project *project, const QString &type);
};

class S60DeviceRunConfigurationRunner : public ProjectExplorer::IRunConfigurationRunner
{
    Q_OBJECT
public:
    S60DeviceRunConfigurationRunner(QObject *parent = 0);
    bool canRun(QSharedPointer<ProjectExplorer::RunConfiguration> runConfiguration, const QString &mode);
    ProjectExplorer::RunControl* run(QSharedPointer<ProjectExplorer::RunConfiguration> runConfiguration, const QString &mode);
    QString displayName() const { return "Run on Device"; }
    QWidget *configurationWidget(QSharedPointer<ProjectExplorer::RunConfiguration> runConfiguration) { return 0; }
};

class S60DeviceRunControl : public ProjectExplorer::RunControl
{
    Q_OBJECT
public:
    S60DeviceRunControl(QSharedPointer<ProjectExplorer::RunConfiguration> runConfiguration);
    ~S60DeviceRunControl() {}
    void start();
    void stop();
    bool isRunning() const;

private slots:
    void processExited(int exitCode);
    void slotAddToOutputWindow(const QString &line);
    void slotError(const QString & error);

private:
    ProjectExplorer::ApplicationLauncher m_applicationLauncher;
    QString m_executable;
};

} // namespace Internal
} // namespace Qt4ProjectManager

#endif // S60DEVICERUNCONFIGURATION_H
