#ifndef VCPROJECTMANAGER_INTERNAL_VCMAKESTEP_H
#define VCPROJECTMANAGER_INTERNAL_VCMAKESTEP_H

#include <projectexplorer/abstractprocessstep.h>

class QComboBox;
class QLabel;

namespace VcProjectManager {
namespace Internal {

class VcProjectBuildConfiguration;
struct MsBuildInformation;

class VcMakeStep : public ProjectExplorer::AbstractProcessStep
{
    Q_OBJECT
    friend class VcMakeStepFactory;

public:
    explicit VcMakeStep(ProjectExplorer::BuildStepList *bsl);

    bool init();
    void run(QFutureInterface<bool> &fi);
    ProjectExplorer::BuildStepConfigWidget *createConfigWidget();
    bool immutable() const;

    VcProjectBuildConfiguration *vcProjectBuildConfiguration() const;
    QString msBuildCommand() const;
    QString msBuildVersion() const;
    void setMsBuildCommand(const QString &msBuild, const QString &version);
    QStringList buildArguments() const;
    QString buildArgumentsToString() const;
    void addBuildArgument(const QString &argument);
    void removeBuildArgument(const QString &buildArgument);

    QVariantMap toMap() const;
    bool fromMap(const QVariantMap &map);

protected:
    void stdOutput(const QString &line);

private:
    explicit VcMakeStep(ProjectExplorer::BuildStepList *parent, VcMakeStep *vcMakeStep);

    QList<ProjectExplorer::Task> m_tasks;
    QFutureInterface<bool> *m_futureInterface;
    ProjectExplorer::ProcessParameters *m_processParams;
    QString m_msBuildCommand;
    QString m_msBuildVersion;
    QStringList m_buildArguments;
};

class VcMakeStepConfigWidget : public ProjectExplorer::BuildStepConfigWidget
{
    Q_OBJECT
public:
    VcMakeStepConfigWidget(VcMakeStep *makeStep);
    QString displayName() const;
    QString summaryText() const;

private slots:
    void onMsBuildSelectionChanged(int index);
    void onMsBuildInformationsUpdated();

private:
    VcMakeStep *m_makeStep;
    QComboBox *m_msBuildComboBox;
    QLabel *m_msBuildPath;
};

class VcMakeStepFactory : public ProjectExplorer::IBuildStepFactory
{
    Q_OBJECT

public:
    explicit VcMakeStepFactory(QObject *parent = 0);
    ~VcMakeStepFactory();

    bool canCreate(ProjectExplorer::BuildStepList *parent, const Core::Id id) const;
    ProjectExplorer::BuildStep *create(ProjectExplorer::BuildStepList *parent, const Core::Id id);
    bool canClone(ProjectExplorer::BuildStepList *parent, ProjectExplorer::BuildStep *product) const;
    ProjectExplorer::BuildStep *clone(ProjectExplorer::BuildStepList *parent, ProjectExplorer::BuildStep *product);
    bool canRestore(ProjectExplorer::BuildStepList *parent, const QVariantMap &map) const;
    ProjectExplorer::BuildStep *restore(ProjectExplorer::BuildStepList *parent, const QVariantMap &map);

    QList<Core::Id> availableCreationIds(ProjectExplorer::BuildStepList *parent) const;
    QString displayNameForId(const Core::Id id) const;
};

} // namespace Internal
} // namespace VcProjectManager

#endif // VCPROJECTMANAGER_INTERNAL_VCMAKESTEP_H
