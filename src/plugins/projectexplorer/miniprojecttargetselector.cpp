#include "miniprojecttargetselector.h"

#include <utils/qtcassert.h>
#include <utils/styledbar.h>
#include <utils/stylehelper.h>

#include <coreplugin/icore.h>
#include <coreplugin/mainwindow.h>

#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/session.h>
#include <projectexplorer/project.h>
#include <projectexplorer/runconfiguration.h>
#include <projectexplorer/buildconfiguration.h>

#include <QtGui/QLayout>
#include <QtGui/QFormLayout>
#include <QtGui/QLabel>
#include <QtGui/QComboBox>
#include <QtGui/QListWidget>
#include <QtGui/QStatusBar>
#include <QtGui/QStackedWidget>

#include <QDebug>

using namespace ProjectExplorer;
using namespace ProjectExplorer::Internal;

MiniTargetWidget::MiniTargetWidget(Project *project, QWidget *parent) :
    QWidget(parent), m_project(project)
{
    m_buildComboBox = new QComboBox;
    m_buildComboBox->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred);
    m_runComboBox = new QComboBox;
    m_runComboBox->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred);

    int fontSize = font().pointSize();
    setStyleSheet(QString::fromLatin1("QWidget { font-size: %1pt; color: white; } "
                                      "QLabel#targetName {  font-size: %2pt; font-weight: bold; } "
                                      "QComboBox { background-color: transparent; margin: 0; border: none; } "
                                      "QComboBox QWidget { background-color: %3;  border: 1px solid lightgrey; } "
                                      "QComboBox::drop-down { border: none; }"
                                      "QComboBox::down-arrow { image: url(:/welcome/images/combobox_arrow.png); } "
                                      ).arg(fontSize-1).arg(fontSize).arg(Utils::StyleHelper::baseColor().name()));

    QGridLayout *gridLayout = new QGridLayout(this);

    m_targetName = new QLabel(tr("Target"));
    m_targetName->setObjectName(QLatin1String("targetName"));
    m_targetIcon = new QLabel();
    m_targetIcon->setPixmap(style()->standardIcon(QStyle::SP_ComputerIcon).pixmap(48, 48));

    Q_FOREACH(BuildConfiguration* bc, project->buildConfigurations())
            addBuildConfiguration(bc);

    Q_FOREACH(RunConfiguration* rc, project->runConfigurations())
            addRunConfiguration(rc);

    connect(project, SIGNAL(addedBuildConfiguration(ProjectExplorer::BuildConfiguration*)),
            SLOT(addBuildConfiguration(ProjectExplorer::BuildConfiguration*)));
    connect(project, SIGNAL(removedBuildConfiguration(ProjectExplorer::BuildConfiguration*)),
            SLOT(addBuildConfiguration(ProjectExplorer::BuildConfiguration*)));

    connect(project, SIGNAL(addedRunConfiguration(ProjectExplorer::RunConfiguration*)),
            SLOT(addRunConfiguration(ProjectExplorer::RunConfiguration*)));
    connect(project, SIGNAL(removedRunConfiguration(ProjectExplorer::RunConfiguration*)),
            SLOT(removeRunConfiguration(ProjectExplorer::RunConfiguration*)));

    connect(m_buildComboBox, SIGNAL(currentIndexChanged(int)), SLOT(setActiveBuildConfiguration(int)));
    connect(m_runComboBox, SIGNAL(currentIndexChanged(int)), SLOT(setActiveRunConfiguration(int)));

    connect(project, SIGNAL(activeBuildConfigurationChanged()), SLOT(setActiveBuildConfiguration()));
    connect(project, SIGNAL(activeRunConfigurationChanged()), SLOT(setActiveRunConfiguration()));

    QHBoxLayout *runHelperLayout = new QHBoxLayout;
    runHelperLayout->setMargin(0);
    runHelperLayout->setSpacing(0);
    QHBoxLayout *buildHelperLayout = new QHBoxLayout;
    buildHelperLayout->setMargin(0);
    buildHelperLayout->setSpacing(0);

    buildHelperLayout->addWidget(m_buildComboBox);
    runHelperLayout->addWidget(m_runComboBox);

    QFormLayout *formLayout = new QFormLayout;
    QLabel *lbl = new QLabel(tr("Build:"));
    lbl->setIndent(6);
    formLayout->addRow(lbl, buildHelperLayout);
    lbl = new QLabel(tr("Run:"));
    lbl->setIndent(6);
    formLayout->addRow(lbl, runHelperLayout);

    gridLayout->addWidget(m_targetName, 0, 0);
    gridLayout->addWidget(m_targetIcon, 0, 1, 2, 1, Qt::AlignTop|Qt::AlignHCenter);
    gridLayout->addLayout(formLayout, 1, 0);
}

void MiniTargetWidget::setActiveBuildConfiguration(int index)
{
    ProjectExplorer::BuildConfiguration* bc =
            m_buildComboBox->itemData(index).value<ProjectExplorer::BuildConfiguration*>();
    m_project->setActiveBuildConfiguration(bc);
    emit activeBuildConfigurationChanged(bc);
}

void MiniTargetWidget::setActiveRunConfiguration(int index)
{
    m_project->setActiveRunConfiguration(
            m_runComboBox->itemData(index).value<ProjectExplorer::RunConfiguration*>());
}
void MiniTargetWidget::setActiveBuildConfiguration()
{
    m_buildComboBox->setCurrentIndex(m_buildComboBox->findData(
            QVariant::fromValue(m_project->activeBuildConfiguration())));
}

void MiniTargetWidget::setActiveRunConfiguration()
{
    m_runComboBox->setCurrentIndex(m_runComboBox->findData(
            QVariant::fromValue(m_project->activeRunConfiguration())));
}

void MiniTargetWidget::addRunConfiguration(ProjectExplorer::RunConfiguration* runConfig)
{
    connect(runConfig, SIGNAL(displayNameChanged()), SLOT(updateDisplayName()));
    m_runComboBox->addItem(runConfig->displayName(), QVariant::fromValue(runConfig));
    if (m_project->activeRunConfiguration() == runConfig)
        m_runComboBox->setCurrentIndex(m_runComboBox->count()-1);
}

void MiniTargetWidget::removeRunConfiguration(ProjectExplorer::RunConfiguration* runConfig)
{
    m_runComboBox->removeItem(m_runComboBox->findData(QVariant::fromValue(runConfig)));
}

void MiniTargetWidget::addBuildConfiguration(ProjectExplorer::BuildConfiguration* buildConfig)
{
    connect(buildConfig, SIGNAL(displayNameChanged()), SLOT(updateDisplayName()));
    m_buildComboBox->addItem(buildConfig->displayName(), QVariant::fromValue(buildConfig));
    if (m_project->activeBuildConfiguration() == buildConfig)
        m_buildComboBox->setCurrentIndex(m_buildComboBox->count()-1);
}

void MiniTargetWidget::removeBuildConfiguration(ProjectExplorer::BuildConfiguration* buildConfig)
{
    m_buildComboBox->removeItem(m_buildComboBox->findData(QVariant::fromValue(buildConfig)));
}

void MiniTargetWidget::updateDisplayName()
{
    QObject *obj = sender();
    if (RunConfiguration* runConfig = qobject_cast<RunConfiguration*>(obj))
    {
        m_runComboBox->setItemText(m_runComboBox->findData(QVariant::fromValue(runConfig)),
                                   runConfig->displayName());
    } else if (BuildConfiguration* buildConfig = qobject_cast<BuildConfiguration*>(obj))
    {
        m_buildComboBox->setItemText(m_buildComboBox->findData(QVariant::fromValue(buildConfig)),
                                   buildConfig->displayName());
    }
}


MiniProjectTargetSelector::MiniProjectTargetSelector(QAction *targetSelectorAction, QWidget *parent) :
    QWidget(parent), m_projectAction(targetSelectorAction)
{
    setWindowFlags(Qt::Popup);
    setFocusPolicy(Qt::NoFocus);

    targetSelectorAction->setIcon(style()->standardIcon(QStyle::SP_ComputerIcon));
    targetSelectorAction->setProperty("titledAction", true);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setMargin(0);
    layout->setSpacing(0);

    Utils::StyledBar *bar = new Utils::StyledBar;
    bar->setSingleRow(true);
    layout->addWidget(bar);
    QHBoxLayout *toolLayout = new QHBoxLayout(bar);
    toolLayout->setMargin(0);
    toolLayout->setSpacing(0);

    QLabel *lbl = new QLabel(tr("Project"));
    lbl->setIndent(6);
    QFont f = lbl->font();
    f.setBold(true);
    lbl->setFont(f);
    m_projectsBox = new QComboBox;
    m_projectsBox->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred);
    m_projectsBox->setMaximumWidth(200);

    toolLayout->addWidget(lbl);
    toolLayout->addWidget(new Utils::StyledSeparator);
    toolLayout->addWidget(m_projectsBox);

    m_widgetStack = new QStackedWidget;
    m_widgetStack->setFocusPolicy(Qt::NoFocus);
    layout->addWidget(m_widgetStack);

    connect(m_projectsBox, SIGNAL(activated(int)), this, SLOT(emitStartupProjectChanged(int)));
}

void MiniProjectTargetSelector::setVisible(bool visible)
{
    if (visible) {
        resize(sizeHint());
        QStatusBar *statusBar = Core::ICore::instance()->statusBar();
        QPoint moveTo = statusBar->mapToGlobal(QPoint(0,0));
        moveTo -= QPoint(0, sizeHint().height());
        move(moveTo);
    }
    QWidget::setVisible(visible);
}

void MiniProjectTargetSelector::addProject(ProjectExplorer::Project* project)
{
    QTC_ASSERT(project, return);
    ProjectListWidget *targetList = new ProjectListWidget(project);
    targetList->setStyleSheet(QString::fromLatin1("QListWidget { background: %1; border: none; }")
                              .arg(Utils::StyleHelper::baseColor().name()));
    int pos = m_widgetStack->addWidget(targetList);

    m_projectsBox->addItem(project->displayName(), QVariant::fromValue(project));

    QListWidgetItem *lwi = new QListWidgetItem();
    targetList->addItem(lwi);
    MiniTargetWidget *targetWidget = new MiniTargetWidget(project);
    targetWidget->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Preferred);
    // width==0 size hint to avoid horizontal scrolling in list widget
    lwi->setSizeHint(QSize(0, targetWidget->sizeHint().height()));
    targetList->setItemWidget(lwi, targetWidget);
    targetList->setCurrentItem(lwi);

    connect(project, SIGNAL(activeBuildConfigurationChanged()), SLOT(updateAction()));

    if (project == ProjectExplorerPlugin::instance()->startupProject()) {
        m_projectsBox->setCurrentIndex(pos);
        m_widgetStack->setCurrentIndex(pos);
    }
}

void MiniProjectTargetSelector::removeProject(ProjectExplorer::Project* project)
{
    for (int i = 0; i < m_widgetStack->count(); ++i) {
        ProjectListWidget *plw = qobject_cast<ProjectListWidget*>(m_widgetStack->widget(i));
        if (plw && plw->project() == project) {
            m_projectsBox->removeItem(i);
            delete plw;
        }
    }
}


void MiniProjectTargetSelector::updateAction()
{
    Project *project = ProjectExplorerPlugin::instance()->startupProject();

    QString projectName = tr("No Project");
    QString buildConfig = tr("None");

    if (project) {
        projectName = project->displayName();
        if (BuildConfiguration* bc = project->activeBuildConfiguration())
            buildConfig = bc->displayName();
    }
    m_projectAction->setProperty("heading", projectName);
    m_projectAction->setProperty("subtitle", buildConfig);
    m_projectAction->setIcon(m_projectAction->icon()); // HACK TO FORCE UPDATE
}

void MiniProjectTargetSelector::emitStartupProjectChanged(int index)
{
    Project *project = m_projectsBox->itemData(index).value<Project*>();
    QTC_ASSERT(project, return;)
    emit startupProjectChanged(project);
}

void MiniProjectTargetSelector::changeStartupProject(ProjectExplorer::Project *project)
{
    for (int i = 0; i < m_widgetStack->count(); ++i) {
        ProjectListWidget *plw = qobject_cast<ProjectListWidget*>(m_widgetStack->widget(i));
        if (plw && plw->project() == project) {
            m_projectsBox->setCurrentIndex(i);
            m_widgetStack->setCurrentIndex(i);
        }
    }
    updateAction();
}
