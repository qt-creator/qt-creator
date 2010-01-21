#ifndef MINIPROJECTTARGETSELECTOR_H
#define MINIPROJECTTARGETSELECTOR_H

#include <QtGui/QListWidget>


QT_BEGIN_NAMESPACE
class QComboBox;
class QLabel;
class QStackedWidget;
QT_END_NAMESPACE

namespace ProjectExplorer {
class Project;
class RunConfiguration;
class BuildConfiguration;

namespace Internal {

// helper classes

class ProjectListWidget : public QListWidget
{
    Q_OBJECT
private:
    ProjectExplorer::Project* m_project;

public:
    ProjectListWidget(ProjectExplorer::Project *project, QWidget *parent = 0)
        : QListWidget(parent), m_project(project)
    {
        setFocusPolicy(Qt::NoFocus);
        setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    }

    ProjectExplorer::Project *project() const
    {
        return m_project;
    }

};

class MiniTargetWidget : public QWidget
{
    Q_OBJECT
public:
    // TODO: Pass target instead of project
    MiniTargetWidget(Project *project, QWidget *parent = 0);


private slots:
    void addRunConfiguration(ProjectExplorer::RunConfiguration *runConfig);
    void removeRunConfiguration(ProjectExplorer::RunConfiguration *buildConfig);
    void addBuildConfiguration(ProjectExplorer::BuildConfiguration *buildConfig);
    void removeBuildConfiguration(ProjectExplorer::BuildConfiguration *buildConfig);

    void setActiveBuildConfiguration(int index);
    void setActiveRunConfiguration(int index);
    void setActiveBuildConfiguration();
    void setActiveRunConfiguration();

    void updateDisplayName();

signals:
    void activeBuildConfigurationChanged(ProjectExplorer::BuildConfiguration *buildConfig);

private:
    QLabel *m_targetName;
    QLabel *m_targetIcon;
    QComboBox *m_runComboBox;
    QComboBox *m_buildComboBox;
    ProjectExplorer::Project* m_project;
};

// main class

class MiniProjectTargetSelector : public QWidget
{
    Q_OBJECT
public:
    MiniProjectTargetSelector(QAction *projectAction,QWidget *parent = 0);
    void setVisible(bool visible);

signals:
    void startupProjectChanged(ProjectExplorer::Project *project);

private slots:
    void addProject(ProjectExplorer::Project *project);
    void removeProject(ProjectExplorer::Project *project);
    void emitStartupProjectChanged(int index);
    void changeStartupProject(ProjectExplorer::Project *project);
    void updateAction();

private:
    QAction *m_projectAction;
    QComboBox *m_projectsBox;
    QStackedWidget *m_widgetStack;
};

};
};
#endif // MINIPROJECTTARGETSELECTOR_H
