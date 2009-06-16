#ifndef S60EMULATORRUNCONFIGURATION_H
#define S60EMULATORRUNCONFIGURATION_H

#include <projectexplorer/runconfiguration.h>
#include <projectexplorer/applicationlauncher.h>

#include <QtGui/QWidget>
#include <QtGui/QLabel>
#include <QtGui/QLineEdit>

namespace Qt4ProjectManager {
namespace Internal {

class S60EmulatorRunConfiguration : public ProjectExplorer::RunConfiguration
{
    Q_OBJECT
public:
    S60EmulatorRunConfiguration(ProjectExplorer::Project *project, const QString &proFilePath);
    ~S60EmulatorRunConfiguration();

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

class S60EmulatorRunConfigurationWidget : public QWidget
{
    Q_OBJECT
public:
    S60EmulatorRunConfigurationWidget(S60EmulatorRunConfiguration *runConfiguration,
                                      QWidget *parent = 0);

private slots:
    void nameEdited(const QString &text);
    void updateTargetInformation();

private:
    S60EmulatorRunConfiguration *m_runConfiguration;
    QLineEdit *m_nameLineEdit;
    QLabel *m_executableLabel;
};

class S60EmulatorRunConfigurationFactory : public ProjectExplorer::IRunConfigurationFactory
{
    Q_OBJECT
public:
    S60EmulatorRunConfigurationFactory(QObject *parent);
    ~S60EmulatorRunConfigurationFactory();
    bool canRestore(const QString &type) const;
    QStringList availableCreationTypes(ProjectExplorer::Project *pro) const;
    // used to translate the types to names to display to the user
    QString displayNameForType(const QString &type) const;
    QSharedPointer<ProjectExplorer::RunConfiguration> create(ProjectExplorer::Project *project, const QString &type);
};

class S60EmulatorRunConfigurationRunner : public ProjectExplorer::IRunConfigurationRunner
{
    Q_OBJECT
public:
    S60EmulatorRunConfigurationRunner(QObject *parent = 0);
    bool canRun(QSharedPointer<ProjectExplorer::RunConfiguration> runConfiguration, const QString &mode);
    ProjectExplorer::RunControl* run(QSharedPointer<ProjectExplorer::RunConfiguration> runConfiguration, const QString &mode);
    QString displayName() const { return "Run in Emulator"; }
    QWidget *configurationWidget(QSharedPointer<ProjectExplorer::RunConfiguration> runConfiguration) { return 0; }
};

class S60EmulatorRunControl : public ProjectExplorer::RunControl
{
    Q_OBJECT
public:
    S60EmulatorRunControl(QSharedPointer<ProjectExplorer::RunConfiguration> runConfiguration);
    ~S60EmulatorRunControl() {}
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

#endif // S60EMULATORRUNCONFIGURATION_H
