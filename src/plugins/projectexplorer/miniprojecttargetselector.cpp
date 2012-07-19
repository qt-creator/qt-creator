/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**************************************************************************/

#include "miniprojecttargetselector.h"
#include "target.h"

#include <utils/qtcassert.h>
#include <utils/styledbar.h>
#include <utils/stylehelper.h>

#include <coreplugin/idocument.h>
#include <coreplugin/icore.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/modemanager.h>

#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/session.h>
#include <projectexplorer/project.h>
#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/deployconfiguration.h>
#include <projectexplorer/profilemanager.h>
#include <projectexplorer/projectmodels.h>
#include <projectexplorer/runconfiguration.h>

#include <QTimer>
#include <QLayout>
#include <QLabel>
#include <QListWidget>
#include <QStatusBar>
#include <QKeyEvent>
#include <QPainter>
#include <QAction>
#include <QItemDelegate>
#include <QMainWindow>
#include <QApplication>

static QIcon createCenteredIcon(const QIcon &icon, const QIcon &overlay)
{
    QPixmap targetPixmap;
    targetPixmap = QPixmap(Core::Constants::TARGET_ICON_SIZE, Core::Constants::TARGET_ICON_SIZE);
    targetPixmap.fill(Qt::transparent);
    QPainter painter(&targetPixmap);

    QPixmap pixmap = icon.pixmap(Core::Constants::TARGET_ICON_SIZE);
    painter.drawPixmap((Core::Constants::TARGET_ICON_SIZE - pixmap.width())/2,
                       (Core::Constants::TARGET_ICON_SIZE - pixmap.height())/2, pixmap);
    if (!overlay.isNull()) {
        pixmap = overlay.pixmap(Core::Constants::TARGET_ICON_SIZE);
        painter.drawPixmap((Core::Constants::TARGET_ICON_SIZE - pixmap.width())/2,
                           (Core::Constants::TARGET_ICON_SIZE - pixmap.height())/2, pixmap);
    }
    return QIcon(targetPixmap);
}

using namespace ProjectExplorer;
using namespace ProjectExplorer::Internal;

static bool projectLesserThan(Project *p1, Project *p2)
{
    int result = caseFriendlyCompare(p1->displayName(), p2->displayName());
    if (result != 0)
        return result < 0;
    else
        return p1 < p2;
}

////////
// TargetSelectorDelegate
////////
class TargetSelectorDelegate : public QItemDelegate
{
public:
    TargetSelectorDelegate(QObject *parent) : QItemDelegate(parent) { }
private:
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const;
    void paint(QPainter *painter,
               const QStyleOptionViewItem &option,
               const QModelIndex &index) const;
    mutable QImage selectionGradient;
};

QSize TargetSelectorDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    Q_UNUSED(option)
    Q_UNUSED(index)
    return QSize(190, 30);
}

void TargetSelectorDelegate::paint(QPainter *painter,
                                   const QStyleOptionViewItem &option,
                                   const QModelIndex &index) const
{
    painter->save();
    painter->setClipping(false);

    if (selectionGradient.isNull())
        selectionGradient.load(QLatin1String(":/projectexplorer/images/targetpanel_gradient.png"));

    if (option.state & QStyle::State_Selected) {
        QColor color =(option.state & QStyle::State_HasFocus) ?
                      option.palette.highlight().color() :
                      option.palette.dark().color();
        painter->fillRect(option.rect, color.darker(140));
        Utils::StyleHelper::drawCornerImage(selectionGradient, painter, option.rect.adjusted(0, 0, 0, -1), 5, 5, 5, 5);
        painter->setPen(QColor(255, 255, 255, 60));
        painter->drawLine(option.rect.topLeft(), option.rect.topRight());
        painter->setPen(QColor(255, 255, 255, 30));
        painter->drawLine(option.rect.bottomLeft() - QPoint(0,1), option.rect.bottomRight() -  QPoint(0,1));
        painter->setPen(QColor(0, 0, 0, 80));
        painter->drawLine(option.rect.bottomLeft(), option.rect.bottomRight());
    }

    QFontMetrics fm(option.font);
    QString text = index.data(Qt::DisplayRole).toString();
    painter->setPen(QColor(255, 255, 255, 160));
    QString elidedText = fm.elidedText(text, Qt::ElideMiddle, option.rect.width() - 12);
    if (elidedText != text)
        const_cast<QAbstractItemModel *>(index.model())->setData(index, text, Qt::ToolTipRole);
    else
        const_cast<QAbstractItemModel *>(index.model())->setData(index, QString(), Qt::ToolTipRole);
    painter->drawText(option.rect.left() + 6, option.rect.top() + (option.rect.height() - fm.height()) / 2 + fm.ascent(), elidedText);

    painter->restore();
}

////////
// ListWidget
////////
ListWidget::ListWidget(QWidget *parent)
    : QListWidget(parent), m_maxCount(0)
{
    setFocusPolicy(Qt::NoFocus);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setAlternatingRowColors(false);
    setFocusPolicy(Qt::WheelFocus);
    setItemDelegate(new TargetSelectorDelegate(this));
    setAttribute(Qt::WA_MacShowFocusRect, false);
    setStyleSheet(QString::fromLatin1("QListWidget { background: #464646; border-style: none; }"));
    setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
}


QSize ListWidget::sizeHint() const
{
    int height = m_maxCount * 30;
    int width = 190;

    // We try to keep the height of the popup equal to the actionbar
    QSize size(width, height);
    static QStatusBar *statusBar = Core::ICore::statusBar();
    static QWidget *actionBar = Core::ICore::mainWindow()->findChild<QWidget*>(QLatin1String("actionbar"));
    Q_ASSERT(actionBar);

    QMargins popupMargins = window()->contentsMargins();
    int alignedWithActionHeight
            = actionBar->height() - statusBar->height() - (popupMargins.top() + popupMargins.bottom());
    size.setHeight(qBound(alignedWithActionHeight, height, 2 * alignedWithActionHeight));
    return size;
}

void ListWidget::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Left)
        focusPreviousChild();
    else if (event->key() == Qt::Key_Right)
        focusNextChild();
    else
        QListWidget::keyPressEvent(event);
}

void ListWidget::keyReleaseEvent(QKeyEvent *event)
{
    if (event->key() != Qt::Key_Left && event->key() != Qt::Key_Right)
        QListWidget::keyReleaseEvent(event);
}

void ListWidget::setMaxCount(int maxCount)
{
    // Note: the current assumption is that, this is not called while the listwidget is visible
    // Otherwise we would need to add code to MiniProjectTargetSelector reacting to the
    // updateGeometry (which then would jump ugly)
    m_maxCount = maxCount;
    updateGeometry();
}

////////
// ProjectListWidget
////////
ProjectListWidget::ProjectListWidget(SessionManager *sessionManager, QWidget *parent)
    : ListWidget(parent), m_sessionManager(sessionManager), m_ignoreIndexChange(false)
{
    connect(m_sessionManager, SIGNAL(projectAdded(ProjectExplorer::Project*)),
            this, SLOT(addProject(ProjectExplorer::Project*)));
    connect(m_sessionManager, SIGNAL(aboutToRemoveProject(ProjectExplorer::Project*)),
            this, SLOT(removeProject(ProjectExplorer::Project*)));
    connect(m_sessionManager, SIGNAL(startupProjectChanged(ProjectExplorer::Project*)),
            this, SLOT(changeStartupProject(ProjectExplorer::Project*)));
    connect(m_sessionManager, SIGNAL(projectDisplayNameChanged(ProjectExplorer::Project*)),
            this, SLOT(projectDisplayNameChanged(ProjectExplorer::Project*)));
    connect(this, SIGNAL(currentRowChanged(int)),
            this, SLOT(setProject(int)));
}

QListWidgetItem *ProjectListWidget::itemForProject(Project *project)
{
    for (int i = 0; i < count(); ++i) {
        QListWidgetItem *currentItem = item(i);
        if (currentItem->data(Qt::UserRole).value<Project*>() == project)
            return currentItem;
    }
    return 0;
}

QString ProjectListWidget::fullName(ProjectExplorer::Project *project)
{
    return tr("%1 (%2)").arg(project->displayName(), project->document()->fileName());
}

void ProjectListWidget::addProject(Project *project)
{
    m_ignoreIndexChange = true;

    int pos = count();
    for (int i=0; i < count(); ++i) {
        Project *p = item(i)->data(Qt::UserRole).value<Project*>();
        if (projectLesserThan(project, p)) {
            pos = i;
            break;
        }
    }

    bool useFullName = false;
    for (int i = 0; i < count(); ++i) {
        Project *p = item(i)->data(Qt::UserRole).value<Project*>();
        if (p->displayName() == project->displayName()) {
            useFullName = true;
            item(i)->setText(fullName(p));
        }
    }

    QString displayName = useFullName ? fullName(project) : project->displayName();
    QListWidgetItem *item = new QListWidgetItem();
    item->setData(Qt::UserRole, QVariant::fromValue(project));
    item->setText(displayName);
    insertItem(pos, item);

    if (project == ProjectExplorerPlugin::instance()->startupProject()) {
        setCurrentItem(item);
    }

    m_ignoreIndexChange = false;
}

void ProjectListWidget::removeProject(Project *project)
{
    m_ignoreIndexChange = true;

    QListWidgetItem *listItem = itemForProject(project);
    delete listItem;

    // Update display names
    QString name = project->displayName();
    int countDisplayName = 0;
    int otherIndex = -1;
    for (int i = 0; i < count(); ++i) {
        Project *p = item(i)->data(Qt::UserRole).value<Project *>();
        if (p->displayName() == name) {
            ++countDisplayName;
            otherIndex = i;
        }
    }
    if (countDisplayName == 1) {
        Project *p = item(otherIndex)->data(Qt::UserRole).value<Project *>();
        item(otherIndex)->setText(p->displayName());
    }

    m_ignoreIndexChange = false;

}

void ProjectListWidget::projectDisplayNameChanged(Project *project)
{
    m_ignoreIndexChange = true;

    int oldPos = 0;
    bool useFullName = false;
    for (int i = 0; i < count(); ++i) {
        Project *p = item(i)->data(Qt::UserRole).value<Project*>();
        if (p == project) {
            oldPos = i;
        } else if (p->displayName() == project->displayName()) {
            useFullName = true;
            item(i)->setText(fullName(p));
        }
    }

    bool isCurrentItem = (oldPos == currentRow());
    QListWidgetItem *projectItem = takeItem(oldPos);

    int pos = count();
    for (int i = 0; i < count(); ++i) {
        Project *p = item(i)->data(Qt::UserRole).value<Project*>();
        if (projectLesserThan(project, p)) {
            pos = i;
            break;
        }
    }

    QString displayName = useFullName ? fullName(project) : project->displayName();
    projectItem->setText(displayName);
    insertItem(pos, projectItem);
    if (isCurrentItem)
        setCurrentRow(pos);

    m_ignoreIndexChange = false;
}

void ProjectListWidget::setProject(int index)
{
    if (m_ignoreIndexChange)
        return;
    if (index < 0)
        return;
    Project *p = item(index)->data(Qt::UserRole).value<Project *>();
    m_sessionManager->setStartupProject(p);
}

void ProjectListWidget::changeStartupProject(Project *project)
{
    setCurrentItem(itemForProject(project));
}

/////////
// GenericListWidget
/////////

GenericListWidget::GenericListWidget(QWidget *parent)
    : ListWidget(parent), m_ignoreIndexChange(false)
{
    connect(this, SIGNAL(currentRowChanged(int)),
            this, SLOT(rowChanged(int)));
}

void GenericListWidget::setProjectConfigurations(const QList<ProjectConfiguration *> &list, ProjectConfiguration *active)
{
    m_ignoreIndexChange = true;
    clear();
    for (int i = 0; i < count(); ++i) {
        ProjectConfiguration *p = item(i)->data(Qt::UserRole).value<ProjectConfiguration *>();
        disconnect(p, SIGNAL(displayNameChanged()),
                   this, SLOT(displayNameChanged()));
    }
    foreach (ProjectConfiguration *pc, list)
        addProjectConfiguration(pc);
    setActiveProjectConfiguration(active);
    m_ignoreIndexChange = false;
}

void GenericListWidget::setActiveProjectConfiguration(ProjectConfiguration *active)
{
    QListWidgetItem *item = itemForProjectConfiguration(active);
    setCurrentItem(item);
}

void GenericListWidget::addProjectConfiguration(ProjectExplorer::ProjectConfiguration *pc)
{
    m_ignoreIndexChange = true;
    QListWidgetItem *lwi = new QListWidgetItem();
    lwi->setText(pc->displayName());
    lwi->setData(Qt::UserRole, QVariant::fromValue(pc));

    // Figure out pos
    int pos = count();
    for (int i = 0; i < count(); ++i) {
        ProjectConfiguration *p = item(i)->data(Qt::UserRole).value<ProjectConfiguration *>();
        if (pc->displayName() < p->displayName()) {
            pos = i;
            break;
        }
    }
    insertItem(pos, lwi);

    connect(pc, SIGNAL(displayNameChanged()),
            this, SLOT(displayNameChanged()));
    m_ignoreIndexChange = false;
}

void GenericListWidget::removeProjectConfiguration(ProjectExplorer::ProjectConfiguration *pc)
{
    m_ignoreIndexChange = true;
    disconnect(pc, SIGNAL(displayNameChanged()),
               this, SLOT(displayNameChanged()));
    delete itemForProjectConfiguration(pc);
    m_ignoreIndexChange = false;
}

void GenericListWidget::rowChanged(int index)
{
    if (m_ignoreIndexChange)
        return;
    if (index < 0)
        return;
    emit changeActiveProjectConfiguration(item(index)->data(Qt::UserRole).value<ProjectConfiguration *>());
}

void GenericListWidget::displayNameChanged()
{
    m_ignoreIndexChange = true;
    ProjectConfiguration *activeProjectConfiguration = 0;
    if (currentItem())
        activeProjectConfiguration = currentItem()->data(Qt::UserRole).value<ProjectConfiguration *>();

    ProjectConfiguration *pc = qobject_cast<ProjectConfiguration *>(sender());
    int index = -1;
    int i = 0;
    for (; i < count(); ++i) {
        QListWidgetItem *lwi = item(i);
        if (lwi->data(Qt::UserRole).value<ProjectConfiguration *>() == pc) {
            index = i;
            break;
        }
    }
    if (index == -1)
        return;
    QListWidgetItem *lwi = takeItem(i);
    lwi->setText(pc->displayName());
    int pos = count();
    for (int i = 0; i < count(); ++i) {
        ProjectConfiguration *p = item(i)->data(Qt::UserRole).value<ProjectConfiguration *>();
        if (pc->displayName() < p->displayName()) {
            pos = i;
            break;
        }
    }
    insertItem(pos, lwi);
    if (activeProjectConfiguration)
        setCurrentItem(itemForProjectConfiguration(activeProjectConfiguration));
    m_ignoreIndexChange = false;
}

QListWidgetItem *GenericListWidget::itemForProjectConfiguration(ProjectConfiguration *pc)
{
    for (int i = 0; i < count(); ++i) {
        QListWidgetItem *lwi = item(i);
        if (lwi->data(Qt::UserRole).value<ProjectConfiguration *>() == pc) {
            return lwi;
        }
    }
    return 0;
}

QWidget *createTitleLabel(const QString &text)
{
    Utils::StyledBar *bar = new Utils::StyledBar;
    bar->setSingleRow(true);
    QVBoxLayout *toolLayout = new QVBoxLayout(bar);
    toolLayout->setMargin(0);
    toolLayout->setSpacing(0);

    QLabel *l = new QLabel(text);
    l->setIndent(6);
    QFont f = l->font();
    f.setBold(true);
    l->setFont(f);
    toolLayout->addWidget(l);

    int panelHeight = l->fontMetrics().height() + 12;
    bar->ensurePolished(); // Required since manhattanstyle overrides height
    bar->setFixedHeight(panelHeight);
    return bar;
}

class OnePixelGreyLine : public QWidget
{
public:
    OnePixelGreyLine(QWidget *parent)
        : QWidget(parent)
    {
        setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
        setMinimumWidth(1);
        setMaximumWidth(1);
    }
    void paintEvent(QPaintEvent *e)
    {
        Q_UNUSED(e);
        QPainter p(this);
        p.fillRect(contentsRect(), QColor(160, 160, 160, 255));
    }
};

MiniProjectTargetSelector::MiniProjectTargetSelector(QAction *targetSelectorAction, SessionManager *sessionManager, QWidget *parent) :
    QWidget(parent), m_projectAction(targetSelectorAction), m_sessionManager(sessionManager),
    m_project(0),
    m_target(0),
    m_buildConfiguration(0),
    m_deployConfiguration(0),
    m_runConfiguration(0),
    m_hideOnRelease(false)
{
    QPalette p = palette();
    p.setColor(QPalette::Text, QColor(255, 255, 255, 160));
    setPalette(p);
    setProperty("panelwidget", true);
    setContentsMargins(QMargins(0, 1, 1, 8));
    setWindowFlags(Qt::Popup);

    targetSelectorAction->setIcon(style()->standardIcon(QStyle::SP_ComputerIcon));
    targetSelectorAction->setProperty("titledAction", true);

    QGridLayout *grid = new QGridLayout(this);
    grid->setMargin(0);
    grid->setSpacing(0);

    m_summaryLabel = new QLabel(this);
    m_summaryLabel->setMargin(3);
    m_summaryLabel->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    m_summaryLabel->setStyleSheet(QString::fromLatin1("background: #464646;"));
    m_summaryLabel->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
    m_summaryLabel->setTextInteractionFlags(m_summaryLabel->textInteractionFlags() | Qt::LinksAccessibleByMouse);

    grid->addWidget(m_summaryLabel, 0, 0, 1, 2 * LAST - 1);

    m_listWidgets.resize(LAST);
    m_titleWidgets.resize(LAST);
    m_separators.resize(LAST);
    m_listWidgets[PROJECT] = 0; //project is not a generic list widget

    m_titleWidgets[PROJECT] = createTitleLabel(tr("Project"));
    grid->addWidget(m_titleWidgets[PROJECT], 1, 0, 1, 2);
    m_projectListWidget = new ProjectListWidget(m_sessionManager, this);
    grid->addWidget(m_projectListWidget, 2, 0);
    m_separators[PROJECT] = new OnePixelGreyLine(this);
    grid->addWidget(m_separators[PROJECT], 2, 1);

    QStringList titles;
    titles << tr("Target") << tr("Build")
           << tr("Deploy") << tr("Run");

    for (int i = TARGET; i < LAST; ++i) {
        m_titleWidgets[i] = createTitleLabel(titles.at(i -1));
        grid->addWidget(m_titleWidgets[i], 1, 2 * i, 1, 2);
        m_listWidgets[i] = new GenericListWidget(this);
        grid->addWidget(m_listWidgets[i], 2, 2 *i);
        m_separators[i] = new OnePixelGreyLine(this);
        grid->addWidget(m_separators[i], 2, 1 + 2 * i);
    }

    changeStartupProject(m_sessionManager->startupProject());
    if (m_sessionManager->startupProject())
        activeTargetChanged(m_sessionManager->startupProject()->activeTarget());

    connect(m_summaryLabel, SIGNAL(linkActivated(QString)),
            this, SLOT(switchToProjectsMode()));

    connect(m_sessionManager, SIGNAL(startupProjectChanged(ProjectExplorer::Project*)),
            this, SLOT(changeStartupProject(ProjectExplorer::Project*)));

    connect(m_sessionManager, SIGNAL(projectAdded(ProjectExplorer::Project*)),
            this, SLOT(projectAdded(ProjectExplorer::Project*)));
    connect(m_sessionManager, SIGNAL(projectRemoved(ProjectExplorer::Project*)),
            this, SLOT(projectRemoved(ProjectExplorer::Project*)));
    connect(m_sessionManager, SIGNAL(projectDisplayNameChanged(ProjectExplorer::Project*)),
            this, SLOT(updateActionAndSummary()));

    // for icon changes:
    connect(ProjectExplorer::ProfileManager::instance(), SIGNAL(profileUpdated(ProjectExplorer::Profile*)),
            this, SLOT(profileChanged(ProjectExplorer::Profile*)));

    connect(m_listWidgets[TARGET], SIGNAL(changeActiveProjectConfiguration(ProjectExplorer::ProjectConfiguration*)),
            this, SLOT(setActiveTarget(ProjectExplorer::ProjectConfiguration*)));
    connect(m_listWidgets[BUILD], SIGNAL(changeActiveProjectConfiguration(ProjectExplorer::ProjectConfiguration*)),
            this, SLOT(setActiveBuildConfiguration(ProjectExplorer::ProjectConfiguration*)));
    connect(m_listWidgets[DEPLOY], SIGNAL(changeActiveProjectConfiguration(ProjectExplorer::ProjectConfiguration*)),
            this, SLOT(setActiveDeployConfiguration(ProjectExplorer::ProjectConfiguration*)));
    connect(m_listWidgets[RUN], SIGNAL(changeActiveProjectConfiguration(ProjectExplorer::ProjectConfiguration*)),
            this, SLOT(setActiveRunConfiguration(ProjectExplorer::ProjectConfiguration*)));
}

void MiniProjectTargetSelector::setActiveTarget(ProjectExplorer::ProjectConfiguration *pc)
{
    m_project->setActiveTarget(static_cast<Target *>(pc));
}

void MiniProjectTargetSelector::setActiveBuildConfiguration(ProjectExplorer::ProjectConfiguration *pc)
{
    m_target->setActiveBuildConfiguration(static_cast<BuildConfiguration *>(pc));
}

void MiniProjectTargetSelector::setActiveDeployConfiguration(ProjectExplorer::ProjectConfiguration *pc)
{
    m_target->setActiveDeployConfiguration(static_cast<DeployConfiguration *>(pc));
}

void MiniProjectTargetSelector::setActiveRunConfiguration(ProjectExplorer::ProjectConfiguration *pc)
{
    m_target->setActiveRunConfiguration(static_cast<RunConfiguration *>(pc));
}

void MiniProjectTargetSelector::projectAdded(ProjectExplorer::Project *project)
{
    connect(project, SIGNAL(addedTarget(ProjectExplorer::Target*)),
            this, SLOT(addedTarget(ProjectExplorer::Target*)));

    connect(project, SIGNAL(removedTarget(ProjectExplorer::Target*)),
            this, SLOT(removedTarget(ProjectExplorer::Target*)));

    foreach (Target *t, project->targets())
        addedTarget(t);

    updateProjectListVisible();
    updateTargetListVisible();
    updateBuildListVisible();
    updateDeployListVisible();
    updateRunListVisible();
}

void MiniProjectTargetSelector::projectRemoved(ProjectExplorer::Project *project)
{
    disconnect(project, SIGNAL(addedTarget(ProjectExplorer::Target*)),
               this, SLOT(addedTarget(ProjectExplorer::Target*)));

    disconnect(project, SIGNAL(removedTarget(ProjectExplorer::Target*)),
               this, SLOT(removedTarget(ProjectExplorer::Target*)));

    foreach (Target *t, project->targets())
        removedTarget(t);

    updateProjectListVisible();
    updateTargetListVisible();
    updateBuildListVisible();
    updateDeployListVisible();
    updateRunListVisible();
}

void MiniProjectTargetSelector::addedTarget(ProjectExplorer::Target *target)
{
    connect(target, SIGNAL(addedBuildConfiguration(ProjectExplorer::BuildConfiguration*)),
            this, SLOT(addedBuildConfiguration(ProjectExplorer::BuildConfiguration*)));
    connect(target, SIGNAL(removedBuildConfiguration(ProjectExplorer::BuildConfiguration*)),
            this, SLOT(removedBuildConfiguration(ProjectExplorer::BuildConfiguration*)));

    connect(target, SIGNAL(addedDeployConfiguration(ProjectExplorer::DeployConfiguration*)),
            this, SLOT(addedDeployConfiguration(ProjectExplorer::DeployConfiguration*)));
    connect(target, SIGNAL(removedDeployConfiguration(ProjectExplorer::DeployConfiguration*)),
            this, SLOT(removedDeployConfiguration(ProjectExplorer::DeployConfiguration*)));

    connect(target, SIGNAL(addedRunConfiguration(ProjectExplorer::RunConfiguration*)),
            this, SLOT(addedRunConfiguration(ProjectExplorer::RunConfiguration*)));
    connect(target, SIGNAL(removedRunConfiguration(ProjectExplorer::RunConfiguration*)),
            this, SLOT(removedRunConfiguration(ProjectExplorer::RunConfiguration*)));

    if (target->project() == m_project)
        m_listWidgets[TARGET]->addProjectConfiguration(target);

    foreach (BuildConfiguration *bc, target->buildConfigurations())
        addedBuildConfiguration(bc);
    foreach (DeployConfiguration *dc, target->deployConfigurations())
        addedDeployConfiguration(dc);
    foreach (RunConfiguration *rc, target->runConfigurations())
        addedRunConfiguration(rc);

    updateTargetListVisible();
    updateBuildListVisible();
    updateDeployListVisible();
    updateRunListVisible();
}

void MiniProjectTargetSelector::removedTarget(ProjectExplorer::Target *target)
{
    disconnect(target, SIGNAL(addedBuildConfiguration(ProjectExplorer::BuildConfiguration*)),
               this, SLOT(addedBuildConfiguration(ProjectExplorer::BuildConfiguration*)));
    disconnect(target, SIGNAL(removedBuildConfiguration(ProjectExplorer::BuildConfiguration*)),
               this, SLOT(removedBuildConfiguration(ProjectExplorer::BuildConfiguration*)));

    disconnect(target, SIGNAL(addedDeployConfiguration(ProjectExplorer::DeployConfiguration*)),
               this, SLOT(addedDeployConfiguration(ProjectExplorer::DeployConfiguration*)));
    disconnect(target, SIGNAL(removedDeployConfiguration(ProjectExplorer::DeployConfiguration*)),
               this, SLOT(removedDeployConfiguration(ProjectExplorer::DeployConfiguration*)));

    disconnect(target, SIGNAL(addedRunConfiguration(ProjectExplorer::RunConfiguration*)),
               this, SLOT(addedRunConfiguration(ProjectExplorer::RunConfiguration*)));
    disconnect(target, SIGNAL(removedRunConfiguration(ProjectExplorer::RunConfiguration*)),
               this, SLOT(removedRunConfiguration(ProjectExplorer::RunConfiguration*)));

    if (target->project() == m_project)
        m_listWidgets[TARGET]->removeProjectConfiguration(target);

    foreach (BuildConfiguration *bc, target->buildConfigurations())
        removedBuildConfiguration(bc);
    foreach (DeployConfiguration *dc, target->deployConfigurations())
        removedDeployConfiguration(dc);
    foreach (RunConfiguration *rc, target->runConfigurations())
        removedRunConfiguration(rc);

    updateTargetListVisible();
    updateBuildListVisible();
    updateDeployListVisible();
    updateRunListVisible();
}

void MiniProjectTargetSelector::addedBuildConfiguration(ProjectExplorer::BuildConfiguration *bc)
{
    if (bc->target() == m_target)
        m_listWidgets[BUILD]->addProjectConfiguration(bc);
    updateBuildListVisible();
}

void MiniProjectTargetSelector::removedBuildConfiguration(ProjectExplorer::BuildConfiguration *bc)
{
    if (bc->target() == m_target)
        m_listWidgets[BUILD]->removeProjectConfiguration(bc);
    updateBuildListVisible();
}

void MiniProjectTargetSelector::addedDeployConfiguration(ProjectExplorer::DeployConfiguration *dc)
{
    if (dc->target() == m_target)
        m_listWidgets[DEPLOY]->addProjectConfiguration(dc);
    updateDeployListVisible();
}

void MiniProjectTargetSelector::removedDeployConfiguration(ProjectExplorer::DeployConfiguration *dc)
{
    if (dc->target() == m_target)
        m_listWidgets[DEPLOY]->removeProjectConfiguration(dc);
    updateDeployListVisible();
}

void MiniProjectTargetSelector::addedRunConfiguration(ProjectExplorer::RunConfiguration *rc)
{
    if (rc->target() == m_target)
        m_listWidgets[RUN]->addProjectConfiguration(rc);
    updateRunListVisible();
}

void MiniProjectTargetSelector::removedRunConfiguration(ProjectExplorer::RunConfiguration *rc)
{
    if (rc->target() == m_target)
        m_listWidgets[RUN]->removeProjectConfiguration(rc);
    updateRunListVisible();
}

void MiniProjectTargetSelector::updateProjectListVisible()
{
    bool visible = m_sessionManager->projects().size() > 1;

    m_projectListWidget->setVisible(visible);
    m_projectListWidget->setMaxCount(m_sessionManager->projects().size());
    m_titleWidgets[PROJECT]->setVisible(visible);

    updateSummary();
    updateSeparatorVisible();
}

void MiniProjectTargetSelector::updateTargetListVisible()
{
    int maxCount = 0;
    foreach (Project *p, m_sessionManager->projects())
        maxCount = qMax(p->targets().size(), maxCount);

    bool visible = maxCount > 1;
    m_listWidgets[TARGET]->setVisible(visible);
    m_listWidgets[TARGET]->setMaxCount(maxCount);
    m_titleWidgets[TARGET]->setVisible(visible);
    updateSummary();
    updateSeparatorVisible();
}

void MiniProjectTargetSelector::updateBuildListVisible()
{
    int maxCount = 0;
    foreach (Project *p, m_sessionManager->projects())
        foreach (Target *t, p->targets())
            maxCount = qMax(t->buildConfigurations().size(), maxCount);

    bool visible = maxCount > 1;
    m_listWidgets[BUILD]->setVisible(visible);
    m_listWidgets[BUILD]->setMaxCount(maxCount);
    m_titleWidgets[BUILD]->setVisible(visible);
    updateSummary();
    updateSeparatorVisible();
}

void MiniProjectTargetSelector::updateDeployListVisible()
{
    int maxCount = 0;
    foreach (Project *p, m_sessionManager->projects())
        foreach (Target *t, p->targets())
            maxCount = qMax(t->deployConfigurations().size(), maxCount);

    bool visible = maxCount > 1;
    m_listWidgets[DEPLOY]->setVisible(visible);
    m_listWidgets[DEPLOY]->setMaxCount(maxCount);
    m_titleWidgets[DEPLOY]->setVisible(visible);
    updateSummary();
    updateSeparatorVisible();
}

void MiniProjectTargetSelector::updateRunListVisible()
{
    int maxCount = 0;
    foreach (Project *p, m_sessionManager->projects())
        foreach (Target *t, p->targets())
            maxCount = qMax(t->runConfigurations().size(), maxCount);

    bool visible = maxCount > 1;
    m_listWidgets[RUN]->setVisible(visible);
    m_listWidgets[RUN]->setMaxCount(maxCount);
    m_titleWidgets[RUN]->setVisible(visible);
    updateSummary();
    updateSeparatorVisible();
}

void MiniProjectTargetSelector::changeStartupProject(ProjectExplorer::Project *project)
{
    if (m_project) {
        disconnect(m_project, SIGNAL(activeTargetChanged(ProjectExplorer::Target*)),
                   this, SLOT(activeTargetChanged(ProjectExplorer::Target*)));
    }
    m_project = project;
    if (m_project) {
        connect(m_project, SIGNAL(activeTargetChanged(ProjectExplorer::Target*)),
                this, SLOT(activeTargetChanged(ProjectExplorer::Target*)));
        activeTargetChanged(m_project->activeTarget());
    } else {
        activeTargetChanged(0);
    }

    if (project) {
        QList<ProjectConfiguration *> list;
        foreach (Target *t, project->targets())
            list.append(t);
        m_listWidgets[TARGET]->setProjectConfigurations(list, project->activeTarget());
    } else {
        m_listWidgets[TARGET]->setProjectConfigurations(QList<ProjectConfiguration *>(), 0);
    }

    updateActionAndSummary();
}

void MiniProjectTargetSelector::activeTargetChanged(ProjectExplorer::Target *target)
{
    if (m_target) {
        disconnect(m_target, SIGNAL(displayNameChanged()),
                   this, SLOT(updateActionAndSummary()));
        disconnect(m_target, SIGNAL(toolTipChanged()),
                   this, SLOT(updateActionAndSummary()));
        disconnect(m_target, SIGNAL(iconChanged()),
                   this, SLOT(updateActionAndSummary()));
        disconnect(m_target, SIGNAL(activeBuildConfigurationChanged(ProjectExplorer::BuildConfiguration*)),
                   this, SLOT(activeBuildConfigurationChanged(ProjectExplorer::BuildConfiguration*)));
        disconnect(m_target, SIGNAL(activeDeployConfigurationChanged(ProjectExplorer::DeployConfiguration*)),
                   this, SLOT(activeDeployConfigurationChanged(ProjectExplorer::DeployConfiguration*)));
        disconnect(m_target, SIGNAL(activeRunConfigurationChanged(ProjectExplorer::RunConfiguration*)),
                   this, SLOT(activeRunConfigurationChanged(ProjectExplorer::RunConfiguration*)));
    }

    m_target = target;

    m_listWidgets[TARGET]->setActiveProjectConfiguration(m_target);

    if (m_buildConfiguration)
        disconnect(m_buildConfiguration, SIGNAL(displayNameChanged()),
                   this, SLOT(updateActionAndSummary()));
    if (m_deployConfiguration)
        disconnect(m_deployConfiguration, SIGNAL(displayNameChanged()),
                   this, SLOT(updateActionAndSummary()));

    if (m_runConfiguration)
        disconnect(m_runConfiguration, SIGNAL(displayNameChanged()),
                   this, SLOT(updateActionAndSummary()));

    if (m_target) {
        QList<ProjectConfiguration *> bl;
        foreach (BuildConfiguration *bc, target->buildConfigurations())
            bl.append(bc);
        m_listWidgets[BUILD]->setProjectConfigurations(bl, target->activeBuildConfiguration());

        QList<ProjectConfiguration *> dl;
        foreach (DeployConfiguration *dc, target->deployConfigurations())
            dl.append(dc);
        m_listWidgets[DEPLOY]->setProjectConfigurations(dl, target->activeDeployConfiguration());

        QList<ProjectConfiguration *> rl;
        foreach (RunConfiguration *rc, target->runConfigurations())
            rl.append(rc);
        m_listWidgets[RUN]->setProjectConfigurations(rl, target->activeRunConfiguration());

        m_buildConfiguration = m_target->activeBuildConfiguration();
        if (m_buildConfiguration)
            connect(m_buildConfiguration, SIGNAL(displayNameChanged()),
                    this, SLOT(updateActionAndSummary()));
        m_deployConfiguration = m_target->activeDeployConfiguration();
        if (m_deployConfiguration)
            connect(m_deployConfiguration, SIGNAL(displayNameChanged()),
                    this, SLOT(updateActionAndSummary()));
        m_runConfiguration = m_target->activeRunConfiguration();
        if (m_runConfiguration)
            connect(m_runConfiguration, SIGNAL(displayNameChanged()),
                    this, SLOT(updateActionAndSummary()));

        connect(m_target, SIGNAL(displayNameChanged()),
                this, SLOT(updateActionAndSummary()));
        connect(m_target, SIGNAL(toolTipChanged()),
                this, SLOT(updateActionAndSummary()));
        connect(m_target, SIGNAL(iconChanged()),
                this, SLOT(updateActionAndSummary()));
        connect(m_target, SIGNAL(activeBuildConfigurationChanged(ProjectExplorer::BuildConfiguration*)),
                this, SLOT(activeBuildConfigurationChanged(ProjectExplorer::BuildConfiguration*)));
        connect(m_target, SIGNAL(activeDeployConfigurationChanged(ProjectExplorer::DeployConfiguration*)),
                this, SLOT(activeDeployConfigurationChanged(ProjectExplorer::DeployConfiguration*)));
        connect(m_target, SIGNAL(activeRunConfigurationChanged(ProjectExplorer::RunConfiguration*)),
                this, SLOT(activeRunConfigurationChanged(ProjectExplorer::RunConfiguration*)));
    } else {
        m_listWidgets[BUILD]->setProjectConfigurations(QList<ProjectConfiguration *>(), 0);
        m_listWidgets[DEPLOY]->setProjectConfigurations(QList<ProjectConfiguration *>(), 0);
        m_listWidgets[RUN]->setProjectConfigurations(QList<ProjectConfiguration *>(), 0);
        m_buildConfiguration = 0;
        m_deployConfiguration = 0;
        m_runConfiguration = 0;
    }
    updateActionAndSummary();
}

void MiniProjectTargetSelector::profileChanged(Profile *profile)
{
    if (m_target && m_target->profile() == profile)
        updateActionAndSummary();
}

void MiniProjectTargetSelector::activeBuildConfigurationChanged(ProjectExplorer::BuildConfiguration *bc)
{
    if (m_buildConfiguration)
        disconnect(m_buildConfiguration, SIGNAL(displayNameChanged()),
                   this, SLOT(updateActionAndSummary()));
    m_buildConfiguration = bc;
    if (m_buildConfiguration)
        connect(m_buildConfiguration, SIGNAL(displayNameChanged()),
                this, SLOT(updateActionAndSummary()));
    m_listWidgets[BUILD]->setActiveProjectConfiguration(bc);
    updateActionAndSummary();
}

void MiniProjectTargetSelector::activeDeployConfigurationChanged(ProjectExplorer::DeployConfiguration *dc)
{
    if (m_deployConfiguration)
        disconnect(m_deployConfiguration, SIGNAL(displayNameChanged()),
                   this, SLOT(updateActionAndSummary()));
    m_deployConfiguration = dc;
    if (m_deployConfiguration)
        connect(m_deployConfiguration, SIGNAL(displayNameChanged()),
                this, SLOT(updateActionAndSummary()));
    m_listWidgets[DEPLOY]->setActiveProjectConfiguration(dc);
    updateActionAndSummary();
}

void MiniProjectTargetSelector::activeRunConfigurationChanged(ProjectExplorer::RunConfiguration *rc)
{
    if (m_runConfiguration)
        disconnect(m_runConfiguration, SIGNAL(displayNameChanged()),
                   this, SLOT(updateActionAndSummary()));
    m_runConfiguration = rc;
    if (m_runConfiguration)
        connect(m_runConfiguration, SIGNAL(displayNameChanged()),
                this, SLOT(updateActionAndSummary()));
    m_listWidgets[RUN]->setActiveProjectConfiguration(rc);
    updateActionAndSummary();
}

void MiniProjectTargetSelector::updateSeparatorVisible()
{
    QVector<bool> visibility;
    visibility.resize(LAST);
    visibility[PROJECT] = m_projectListWidget->isVisibleTo(this);
    for (int i = TARGET; i < LAST; ++i)
        visibility[i] = m_listWidgets[i]->isVisibleTo(this);
    int lastVisible = visibility.lastIndexOf(true);
    if (lastVisible != -1)
        visibility[lastVisible] = false;

    for (int i = PROJECT; i < LAST; ++i)
        m_separators[i]->setVisible(visibility[i]);
}

void MiniProjectTargetSelector::setVisible(bool visible)
{
    if (visible) {
        QStatusBar *statusBar = Core::ICore::statusBar();
        QPoint moveTo = statusBar->mapToGlobal(QPoint(0,0));
        moveTo -= QPoint(0, sizeHint().height());
        move(moveTo);
        if (!focusWidget() || !focusWidget()->isVisibleTo(this)) { // Does the second part actually work?
            if (m_projectListWidget->isVisibleTo(this))
                m_projectListWidget->setFocus();
            for (int i = TARGET; i < LAST; ++i) {
                if (m_listWidgets[i]->isVisibleTo(this)) {
                    m_listWidgets[i]->setFocus();
                    break;
                }
            }
        }
    }

    QWidget::setVisible(visible);
    m_projectAction->setChecked(visible);
}

void MiniProjectTargetSelector::toggleVisible()
{
    setVisible(!isVisible());
}

void MiniProjectTargetSelector::nextOrShow()
{
    if (!isVisible()) {
        show();
    } else {
        m_hideOnRelease = true;
        m_earliestHidetime = QDateTime::currentDateTime().addMSecs(800);
        if (ListWidget *lw = qobject_cast<ListWidget *>(focusWidget())) {
            if (lw->currentRow() < lw->count() -1)
                lw->setCurrentRow(lw->currentRow() + 1);
            else
                lw->setCurrentRow(0);
        }
    }
}

void MiniProjectTargetSelector::keyPressEvent(QKeyEvent *ke)
{
    if (ke->key() == Qt::Key_Return
            || ke->key() == Qt::Key_Enter
            || ke->key() == Qt::Key_Space)
        hide();
    QWidget::keyPressEvent(ke);
}

void MiniProjectTargetSelector::keyReleaseEvent(QKeyEvent *ke)
{
    if (m_hideOnRelease) {
        if (ke->modifiers() == 0
                /*HACK this is to overcome some event inconsistencies between platforms*/
                || (ke->modifiers() == Qt::AltModifier
                    && (ke->key() == Qt::Key_Alt || ke->key() == -1))) {
            delayedHide();
            m_hideOnRelease = false;
        }
    }
    if (ke->key() == Qt::Key_Return
            || ke->key() == Qt::Key_Enter
            || ke->key() == Qt::Key_Space)
        return;
    QWidget::keyReleaseEvent(ke);
}

QSize MiniProjectTargetSelector::sizeHint() const
{
    static QStatusBar *statusBar = Core::ICore::statusBar();
    static QWidget *actionBar = Core::ICore::mainWindow()->findChild<QWidget*>(QLatin1String("actionbar"));
    Q_ASSERT(actionBar);

    // At least the size of the actionbar
    int alignedWithActionHeight
            = actionBar->height() - statusBar->height();
    QSize s = QWidget::sizeHint();
    if (s.height() < alignedWithActionHeight)
        s.setHeight(alignedWithActionHeight);
    return s;
}

void MiniProjectTargetSelector::delayedHide()
{
    QDateTime current = QDateTime::currentDateTime();
    if (m_earliestHidetime > current) {
        // schedule for later
        QTimer::singleShot(current.msecsTo(m_earliestHidetime) + 50, this, SLOT(delayedHide()));
    } else {
        hide();
    }
}

// This is a workaround for the problem that Windows
// will let the mouse events through when you click
// outside a popup to close it. This causes the popup
// to open on mouse release if you hit the button, which
//
//
// A similar case can be found in QComboBox
void MiniProjectTargetSelector::mousePressEvent(QMouseEvent *e)
{
    setAttribute(Qt::WA_NoMouseReplay);
    QWidget::mousePressEvent(e);
}

void MiniProjectTargetSelector::updateActionAndSummary()
{
    QString projectName;
    QString targetName;
    QString targetToolTipText;
    QString buildConfig;
    QString deployConfig;
    QString runConfig;
    QIcon targetIcon = style()->standardIcon(QStyle::SP_ComputerIcon);

    Project *project = ProjectExplorerPlugin::instance()->startupProject();
    if (project) {
        projectName = project->displayName();

        if (Target *target = project->activeTarget()) {
            targetName = project->activeTarget()->displayName();

            if (BuildConfiguration *bc = target->activeBuildConfiguration())
                buildConfig = bc->displayName();

            if (DeployConfiguration *dc = target->activeDeployConfiguration())
                deployConfig = dc->displayName();

            if (RunConfiguration *rc = target->activeRunConfiguration())
                runConfig = rc->displayName();

            targetToolTipText = target->toolTip();
            targetIcon = createCenteredIcon(target->icon(), target->overlayIcon());
        }
    }
    m_projectAction->setProperty("heading", projectName);
    if (project && project->needsConfiguration()) {
        m_projectAction->setProperty("subtitle", tr("Unconfigured"));
    } else {
        m_projectAction->setProperty("subtitle", buildConfig);
    }
    m_projectAction->setIcon(targetIcon);
    QStringList lines;
    lines << tr("<b>Project:</b> %1").arg(projectName);
    if (!targetName.isEmpty())
        lines << tr("<b>Target:</b> %1").arg(targetName);
    if (!buildConfig.isEmpty())
        lines << tr("<b>Build:</b> %1").arg(buildConfig);
    if (!deployConfig.isEmpty())
        lines << tr("<b>Deploy:</b> %1").arg(deployConfig);
    if (!runConfig.isEmpty())
        lines << tr("<b>Run:</b> %1").arg(runConfig);
    if (!targetToolTipText.isEmpty())
        lines << tr("%1").arg(targetToolTipText);
    QString toolTip = tr("<html><nobr>%1</html>")
            .arg(lines.join(QLatin1String("<br/>")));
    m_projectAction->setToolTip(toolTip);
    updateSummary();
}

void MiniProjectTargetSelector::updateSummary()
{
    // Count the number of lines
    int visibleLineCount = m_projectListWidget->isVisibleTo(this) ? 0 : 1;
    for (int i = TARGET; i < LAST; ++i)
        visibleLineCount += m_listWidgets[i]->isVisibleTo(this) ? 0 : 1;

    if (visibleLineCount == LAST) {
        m_summaryLabel->setMinimumHeight(0);
        m_summaryLabel->setMaximumHeight(800);
    } else {
        if (visibleLineCount < 3) {
            foreach (Project *p, m_sessionManager->projects()) {
                if (p->needsConfiguration()) {
                    visibleLineCount = 3;
                    break;
                }
            }
        }

        int height = visibleLineCount * QFontMetrics(m_summaryLabel->font()).height() + m_summaryLabel->margin() *2;
        m_summaryLabel->setMinimumHeight(height);
        m_summaryLabel->setMaximumHeight(height);
    }

    QString summary;
    if (Project *startupProject = m_sessionManager->startupProject()) {
        if (!m_projectListWidget->isVisibleTo(this))
            summary.append(tr("Project: <b>%1</b><br/>").arg(startupProject->displayName()));
        if (Target *activeTarget = m_sessionManager->startupProject()->activeTarget()) {
            if (!m_listWidgets[TARGET]->isVisibleTo(this))
                summary.append(tr("Target: <b>%1</b><br/>").arg( activeTarget->displayName()));
            if (!m_listWidgets[BUILD]->isVisibleTo(this) && activeTarget->activeBuildConfiguration())
                summary.append(tr("Build: <b>%1</b><br/>").arg(
                                   activeTarget->activeBuildConfiguration()->displayName()));
            if (!m_listWidgets[DEPLOY]->isVisibleTo(this) && activeTarget->activeDeployConfiguration())
                summary.append(tr("Deploy: <b>%1</b><br/>").arg(
                                   activeTarget->activeDeployConfiguration()->displayName()));
            if (!m_listWidgets[RUN]->isVisibleTo(this) && activeTarget->activeRunConfiguration())
                summary.append(tr("Run: <b>%1</b><br/>").arg(
                                   activeTarget->activeRunConfiguration()->displayName()));
        } else if (startupProject->needsConfiguration()) {
            summary = tr("<style type=text/css>"
                         "a:link {color: rgb(128, 128, 255, 240);}</style>"
                         "The project <b>%1</b> is not yet configured<br/><br/>"
                         "You can configure it in the <a href=\"projectmode\">Projects mode</a><br/>")
                    .arg(startupProject->displayName());
        } else {
            if (!m_listWidgets[TARGET]->isVisibleTo(this))
                summary.append("<br/>");
            if (!m_listWidgets[BUILD]->isVisibleTo(this))
                summary.append("<br/>");
            if (!m_listWidgets[DEPLOY]->isVisibleTo(this))
                summary.append("<br/>");
            if (!m_listWidgets[RUN]->isVisibleTo(this))
                summary.append("<br/>");
        }
    }
    m_summaryLabel->setText(summary);
}

void MiniProjectTargetSelector::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setPen(Utils::StyleHelper::borderColor());
    painter.drawLine(rect().topLeft(), rect().topRight());
    painter.drawLine(rect().topRight(), rect().bottomRight());

    QRect bottomRect(0, rect().height() - 8, rect().width(), 8);
    static QImage image(QLatin1String(":/projectexplorer/images/targetpanel_bottom.png"));
    Utils::StyleHelper::drawCornerImage(image, &painter, bottomRect, 1, 1, 1, 1);
}

void MiniProjectTargetSelector::switchToProjectsMode()
{
    Core::ICore::instance()->modeManager()->activateMode(Constants::MODE_SESSION);
    hide();
}
