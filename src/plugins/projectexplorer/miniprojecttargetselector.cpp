/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "miniprojecttargetselector.h"

#include <utils/qtcassert.h>
#include <utils/styledbar.h>
#include <utils/stylehelper.h>

#include <coreplugin/icore.h>
#include <coreplugin/mainwindow.h>
#include <coreplugin/coreconstants.h>

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
#include <QtGui/QKeyEvent>
#include <QtGui/QPainter>

#include <QtGui/QApplication>

using namespace ProjectExplorer;
using namespace ProjectExplorer::Internal;

ProjectListWidget::ProjectListWidget(ProjectExplorer::Project *project, QWidget *parent)
    : QListWidget(parent), m_project(project)
{
    setFocusPolicy(Qt::NoFocus);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setAlternatingRowColors(false);
    setFocusPolicy(Qt::WheelFocus);

    connect(this, SIGNAL(currentRowChanged(int)), SLOT(setTarget(int)));
}

ProjectExplorer::Project *ProjectListWidget::project() const
{
    return m_project;
}

QSize ProjectListWidget::sizeHint() const
{
    int height = 0;
    for (int itemPos = 0; itemPos < count(); ++itemPos)
        height += item(itemPos)->sizeHint().height();

    return QListWidget::sizeHint().expandedTo(QSize(0, height));
}

void ProjectListWidget::setRunComboPopup()
{
    QWidget *w = itemWidget(currentItem());
    MiniTargetWidget *mtw = qobject_cast<MiniTargetWidget*>(w);
    if (mtw->runSettingsComboBox())
        mtw->runSettingsComboBox()->showPopup();
}

void ProjectListWidget::setBuildComboPopup()
{
    QWidget *w = itemWidget(currentItem());
    MiniTargetWidget *mtw = qobject_cast<MiniTargetWidget*>(w);
    if (mtw->buildSettingsComboBox())
        mtw->buildSettingsComboBox()->showPopup();
}

void ProjectListWidget::setTarget(int index)
{
    QList<Target *> targets(m_project->targets());
    if (index >= 0 && index < targets.count())
        m_project->setActiveTarget(targets.at(index));
}

MiniTargetWidget::MiniTargetWidget(Target *target, QWidget *parent) :
    QWidget(parent), m_target(target)
{
    Q_ASSERT(m_target);

    if (hasBuildConfiguration()) {
        m_buildComboBox = new QComboBox;
        m_buildComboBox->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred);
        m_buildComboBox->setToolTip(tr("Make active and press 'b' to select."));
    } else {
        m_buildComboBox = 0;
    }
 
    m_runComboBox = new QComboBox;
    m_runComboBox->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred);
    m_runComboBox->setToolTip(tr("Make active and press 'r' to select."));

    int fontSize = font().pointSize();
    setStyleSheet(QString::fromLatin1("QWidget { font-size: %1pt; color: white; } "
                                      "QLabel#targetName {  font-size: %2pt; font-weight: bold; } "
                                      "QComboBox { background-color: transparent; margin: 0; border: none; } "
                                      "QComboBox QWidget { background-color: %3;  border: 1px solid lightgrey; } "
                                      "QComboBox::drop-down { border: none; }"
                                      "QComboBox::down-arrow { image: url(:/welcome/images/combobox_arrow.png); } "
                                      ).arg(fontSize-1).arg(fontSize).arg(Utils::StyleHelper::baseColor().name()));

    QGridLayout *gridLayout = new QGridLayout(this);

    m_targetName = new QLabel(m_target->displayName());
    m_targetName->setObjectName(QLatin1String("target"));
    m_targetIcon = new QLabel();
    updateIcon();
    if (hasBuildConfiguration()) {
        Q_FOREACH(BuildConfiguration* bc, m_target->buildConfigurations())
                addBuildConfiguration(bc);

        connect(m_target, SIGNAL(addedBuildConfiguration(ProjectExplorer::BuildConfiguration*)),
                SLOT(addBuildConfiguration(ProjectExplorer::BuildConfiguration*)));
        connect(m_target, SIGNAL(removedBuildConfiguration(ProjectExplorer::BuildConfiguration*)),
                SLOT(addBuildConfiguration(ProjectExplorer::BuildConfiguration*)));

        connect(m_target, SIGNAL(activeBuildConfigurationChanged(ProjectExplorer::BuildConfiguration*)),
                SLOT(setActiveBuildConfiguration()));
        connect(m_buildComboBox, SIGNAL(currentIndexChanged(int)), SLOT(setActiveBuildConfiguration(int)));
    }

    Q_FOREACH(RunConfiguration* rc, m_target->runConfigurations())
            addRunConfiguration(rc);

    connect(m_target, SIGNAL(addedRunConfiguration(ProjectExplorer::RunConfiguration*)),
            SLOT(addRunConfiguration(ProjectExplorer::RunConfiguration*)));
    connect(m_target, SIGNAL(removedRunConfiguration(ProjectExplorer::RunConfiguration*)),
            SLOT(removeRunConfiguration(ProjectExplorer::RunConfiguration*)));

    connect(m_runComboBox, SIGNAL(currentIndexChanged(int)), SLOT(setActiveRunConfiguration(int)));

    connect(m_target, SIGNAL(activeRunConfigurationChanged(ProjectExplorer::RunConfiguration*)),
            SLOT(setActiveRunConfiguration()));
    connect(m_target, SIGNAL(iconChanged()), this, SLOT(updateIcon()));

    QHBoxLayout *buildHelperLayout = 0;
    if (hasBuildConfiguration()) {
        buildHelperLayout= new QHBoxLayout;
        buildHelperLayout->setMargin(0);
        buildHelperLayout->setSpacing(0);
        buildHelperLayout->addWidget(m_buildComboBox);
    }

    QHBoxLayout *runHelperLayout = new QHBoxLayout;
    runHelperLayout->setMargin(0);
    runHelperLayout->setSpacing(0);
    runHelperLayout->addWidget(m_runComboBox);

    QFormLayout *formLayout = new QFormLayout;
    QLabel *lbl;
    if (hasBuildConfiguration()) {
        lbl = new QLabel(tr("Build:"));
        lbl->setIndent(6);
        formLayout->addRow(lbl, buildHelperLayout);
    }
    lbl = new QLabel(tr("Run:"));
    lbl->setIndent(6);
    formLayout->addRow(lbl, runHelperLayout);

    gridLayout->addWidget(m_targetName, 0, 0);
    gridLayout->addWidget(m_targetIcon, 0, 1, 2, 1, Qt::AlignTop|Qt::AlignHCenter);
    gridLayout->addLayout(formLayout, 1, 0);
}

void MiniTargetWidget::updateIcon()
{
    QPixmap targetPixmap;
    QPixmap targetPixmapCandidate = m_target->icon().pixmap(Core::Constants::TARGET_ICON_SIZE, Core::Constants::TARGET_ICON_SIZE);
    QSize actualSize = m_target->icon().actualSize(QSize(Core::Constants::TARGET_ICON_SIZE, Core::Constants::TARGET_ICON_SIZE));
    if (actualSize == QSize(Core::Constants::TARGET_ICON_SIZE, Core::Constants::TARGET_ICON_SIZE)) {
        targetPixmap = targetPixmapCandidate;
    } else {
        targetPixmap = QPixmap(Core::Constants::TARGET_ICON_SIZE, Core::Constants::TARGET_ICON_SIZE);
        targetPixmap.fill(Qt::transparent);
        QPainter painter(&targetPixmap);
        painter.drawPixmap((Core::Constants::TARGET_ICON_SIZE - actualSize.width())/2,
                           (Core::Constants::TARGET_ICON_SIZE - actualSize.height())/2,
                           targetPixmapCandidate);
    }
    m_targetIcon->setPixmap(targetPixmap);
}

ProjectExplorer::Target *MiniTargetWidget::target() const
{
    return m_target;
}

void MiniTargetWidget::setActiveBuildConfiguration(int index)
{
    ProjectExplorer::BuildConfiguration* bc =
            m_buildComboBox->itemData(index).value<ProjectExplorer::BuildConfiguration*>();
    m_target->setActiveBuildConfiguration(bc);
    emit changed();
}

void MiniTargetWidget::setActiveRunConfiguration(int index)
{
    m_target->setActiveRunConfiguration(
            m_runComboBox->itemData(index).value<ProjectExplorer::RunConfiguration*>());
    updateIcon();
    emit changed();
}
void MiniTargetWidget::setActiveBuildConfiguration()
{
    QTC_ASSERT(m_buildComboBox, return)
    m_buildComboBox->setCurrentIndex(m_buildComboBox->findData(
            QVariant::fromValue(m_target->activeBuildConfiguration())));
}

void MiniTargetWidget::setActiveRunConfiguration()
{
    m_runComboBox->setCurrentIndex(m_runComboBox->findData(
            QVariant::fromValue(m_target->activeRunConfiguration())));
}

void MiniTargetWidget::addRunConfiguration(ProjectExplorer::RunConfiguration* rc)
{
    connect(rc, SIGNAL(displayNameChanged()), SLOT(updateDisplayName()));
    m_runComboBox->addItem(rc->displayName(), QVariant::fromValue(rc));
    if (m_target->activeRunConfiguration() == rc)
        m_runComboBox->setCurrentIndex(m_runComboBox->count()-1);
}

void MiniTargetWidget::removeRunConfiguration(ProjectExplorer::RunConfiguration* rc)
{
    m_runComboBox->removeItem(m_runComboBox->findData(QVariant::fromValue(rc)));
}

void MiniTargetWidget::addBuildConfiguration(ProjectExplorer::BuildConfiguration* bc)
{
    QTC_ASSERT(m_buildComboBox, return);
    connect(bc, SIGNAL(displayNameChanged()), SLOT(updateDisplayName()));
    m_buildComboBox->addItem(bc->displayName(), QVariant::fromValue(bc));
    if (m_target->activeBuildConfiguration() == bc)
        m_buildComboBox->setCurrentIndex(m_buildComboBox->count()-1);
}

void MiniTargetWidget::removeBuildConfiguration(ProjectExplorer::BuildConfiguration* bc)
{
    QTC_ASSERT(m_buildComboBox, return);
    m_buildComboBox->removeItem(m_buildComboBox->findData(QVariant::fromValue(bc)));
}

void MiniTargetWidget::updateDisplayName()
{
    QObject *obj = sender();
    RunConfiguration *rc = qobject_cast<RunConfiguration*>(obj);
    BuildConfiguration *bc = qobject_cast<BuildConfiguration*>(obj);
    if (rc) {
        m_runComboBox->setItemText(m_runComboBox->findData(QVariant::fromValue(rc)),
                                   rc->displayName());
    } else if (bc) {
        m_buildComboBox->setItemText(m_buildComboBox->findData(QVariant::fromValue(bc)),
                                     bc->displayName());
    }
    emit changed();
}

bool MiniTargetWidget::hasBuildConfiguration() const
{
    return (m_target->buildConfigurationFactory() != 0);
}

MiniProjectTargetSelector::MiniProjectTargetSelector(QAction *targetSelectorAction, QWidget *parent) :
    QWidget(parent), m_projectAction(targetSelectorAction)
{
    setWindowFlags(Qt::Popup);
    setFocusPolicy(Qt::WheelFocus);

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
    m_projectsBox->setObjectName(QLatin1String("ProjectsBox"));
    m_projectsBox->setFocusPolicy(Qt::WheelFocus);
    m_projectsBox->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred);
    m_projectsBox->setMaximumWidth(200);
    m_projectsBox->installEventFilter(this);

    toolLayout->addWidget(lbl);
    toolLayout->addWidget(new Utils::StyledSeparator);
    toolLayout->addWidget(m_projectsBox);

    m_widgetStack = new QStackedWidget;
    m_widgetStack->setFocusPolicy(Qt::WheelFocus);
    m_widgetStack->installEventFilter(this);
    layout->addWidget(m_widgetStack);

    connect(m_projectsBox, SIGNAL(activated(int)),
            SLOT(emitStartupProjectChanged(int)));
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
    if (m_widgetStack->currentWidget())
        m_widgetStack->currentWidget()->setFocus();

    m_projectAction->setChecked(visible);
}

void MiniProjectTargetSelector::addProject(ProjectExplorer::Project* project)
{
    QTC_ASSERT(project, return);
    ProjectListWidget *targetList = new ProjectListWidget(project);
    targetList->installEventFilter(this);
    targetList->setStyleSheet(QString::fromLatin1("QListWidget { background: %1; border: none; }")
                              .arg(Utils::StyleHelper::baseColor().name()));
    int pos = m_widgetStack->addWidget(targetList);

    m_projectsBox->addItem(project->displayName(), QVariant::fromValue(project));

    connect(project, SIGNAL(activeTargetChanged(ProjectExplorer::Target*)),
            SLOT(updateAction()));

    connect(project, SIGNAL(addedTarget(ProjectExplorer::Target*)),
            SLOT(addTarget(ProjectExplorer::Target*)));
    connect(project, SIGNAL(removedTarget(ProjectExplorer::Target*)),
            SLOT(removeTarget(ProjectExplorer::Target*)));

    if (project == ProjectExplorerPlugin::instance()->startupProject()) {
        m_projectsBox->setCurrentIndex(pos);
        m_widgetStack->setCurrentIndex(pos);
    }

    foreach (Target *t, project->targets())
        addTarget(t, t == project->activeTarget());
}

void MiniProjectTargetSelector::removeProject(ProjectExplorer::Project* project)
{
    int index = indexFor(project);
    if (index < 0)
        return;
    ProjectListWidget *plw = qobject_cast<ProjectListWidget*>(m_widgetStack->widget(index));
    m_projectsBox->removeItem(index);
    delete plw;
}

void MiniProjectTargetSelector::addTarget(ProjectExplorer::Target *target, bool activeTarget)
{
    QTC_ASSERT(target, return);

    int index = indexFor(target->project());
    if (index < 0)
        return;

    connect(target, SIGNAL(toolTipChanged()), this, SLOT(updateAction()));
    connect(target, SIGNAL(iconChanged()), this, SLOT(updateAction()));
    ProjectListWidget *plw = qobject_cast<ProjectListWidget*>(m_widgetStack->widget(index));

    QListWidgetItem *lwi = new QListWidgetItem();
    plw->addItem(lwi);
    MiniTargetWidget *targetWidget = new MiniTargetWidget(target);
    connect(targetWidget, SIGNAL(changed()), this, SLOT(updateAction()));
    targetWidget->installEventFilter(this);
    if (targetWidget->buildSettingsComboBox())
        targetWidget->buildSettingsComboBox()->installEventFilter(this);
    if (targetWidget->runSettingsComboBox())
        targetWidget->runSettingsComboBox()->installEventFilter(this);
    targetWidget->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Preferred);
    // width==0 size hint to avoid horizontal scrolling in list widget
    lwi->setSizeHint(QSize(0, targetWidget->sizeHint().height()));
    plw->setItemWidget(lwi, targetWidget);

    if (activeTarget)
        plw->setCurrentItem(lwi, QItemSelectionModel::SelectCurrent);
}

void MiniProjectTargetSelector::removeTarget(ProjectExplorer::Target *target)
{
    QTC_ASSERT(target, return);

    int index = indexFor(target->project());
    if (index < 0)
        return;

    ProjectListWidget *plw = qobject_cast<ProjectListWidget*>(m_widgetStack->widget(index));
    for (int i = 0; i < plw->count(); ++i) {
        QListWidgetItem *itm(plw->item(i));
        MiniTargetWidget *mtw(qobject_cast<MiniTargetWidget *>(plw->itemWidget(itm)));
        if (!mtw)
            continue;
        if (target != mtw->target())
            continue;
        delete plw->takeItem(i);
        delete mtw;
    }
}

void MiniProjectTargetSelector::updateAction()
{
    Project *project = ProjectExplorerPlugin::instance()->startupProject();

    QString projectName = tr("No Project");
    QString targetName;
    QString targetToolTipText;
    QIcon targetIcon;
    QString buildConfig;
    QString runConfig;

    if (project) {
        projectName = project->displayName();

        if (Target *target = project->activeTarget()) {
            if (project->targets().count() > 1) {
                targetName = project->activeTarget()->displayName();
            }
            if (BuildConfiguration *bc = target->activeBuildConfiguration()) {
                buildConfig = bc->displayName();
            }

            if (RunConfiguration *rc = target->activeRunConfiguration()) {
                runConfig = rc->displayName();
            }
            targetToolTipText = target->toolTip();
            targetIcon = target->icon();
        }
    }
    m_projectAction->setProperty("heading", projectName);
    m_projectAction->setProperty("subtitle", buildConfig);
    m_projectAction->setIcon(targetIcon);
    QString toolTip = tr("<html><b>Project:</b> %1<br/>%2%3<b>Run:</b> %4%5</html>");
    QString targetTip = targetName.isEmpty() ? QLatin1String("")
        : tr("<b>Target:</b> %1<br/>").arg(targetName);
    QString buildTip = buildConfig.isEmpty() ? QLatin1String("")
        : tr("<b>Build:</b> %2<br/>").arg(buildConfig);
    QString targetToolTip = targetToolTipText.isEmpty() ? QLatin1String("")
        : tr("<br/>%1").arg(targetToolTipText);
    m_projectAction->setToolTip(toolTip.arg(projectName, targetTip, buildTip, runConfig, targetToolTip));
}

int MiniProjectTargetSelector::indexFor(ProjectExplorer::Project *project) const
{
    for (int i = 0; i < m_widgetStack->count(); ++i) {
        ProjectListWidget *plw = qobject_cast<ProjectListWidget*>(m_widgetStack->widget(i));
        if (plw && plw->project() == project)
            return i;
    }
    return -1;
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

bool MiniProjectTargetSelector::eventFilter(QObject *o, QEvent *ev)
{
    switch(ev->type()) 
    {
    case QEvent::KeyPress: {

        QKeyEvent *kev = static_cast<QKeyEvent*>(ev);

        if (kev->key() == Qt::Key_Tab) {
            if(o == m_projectsBox) {
                if (m_widgetStack->currentWidget())
                    m_widgetStack->currentWidget()->setFocus();
                return true;
            } else {
                m_projectsBox->setFocus();
                return true;
            }
        }

        if (o == m_widgetStack->currentWidget()) {
            if (kev->key() == Qt::Key_Return) {
                hide();
                return true;
            }

            ProjectListWidget *plw = qobject_cast<ProjectListWidget*>(m_widgetStack->currentWidget());
            if (kev->key() == Qt::Key_B)
            {
                plw->setBuildComboPopup();
                return true;
            }
            if (kev->key() == Qt::Key_R)
            {
                plw->setRunComboPopup();
                return true;
            }
        }
    }
    default:
      return false;

    }
    return false;
}
