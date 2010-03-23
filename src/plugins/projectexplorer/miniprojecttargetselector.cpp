/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
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
#include <QtGui/QItemDelegate>

#include <QtGui/QApplication>

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

class TargetSelectorDelegate : public QItemDelegate
{
public:
    TargetSelectorDelegate(QObject *parent) : QItemDelegate(parent) { }
private:
    void paint(QPainter *painter,
               const QStyleOptionViewItem &option,
               const QModelIndex &index) const;
    mutable QImage selectionGradient;
};

void TargetSelectorDelegate::paint(QPainter *painter,
                                   const QStyleOptionViewItem &option,
                                   const QModelIndex &) const
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
    painter->restore();
}

ProjectListWidget::ProjectListWidget(ProjectExplorer::Project *project, QWidget *parent)
    : QListWidget(parent), m_project(project)
{
    setFocusPolicy(Qt::NoFocus);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setAlternatingRowColors(false);
    setFocusPolicy(Qt::WheelFocus);
    setItemDelegate(new TargetSelectorDelegate(this));
    setAttribute(Qt::WA_MacShowFocusRect, false);

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

    // We try to keep the height of the popup equal to the actionbar
    QSize size(QListWidget::sizeHint().width(), height);
    static QStatusBar *statusBar = Core::ICore::instance()->statusBar();
    static QWidget *actionBar = Core::ICore::instance()->mainWindow()->findChild<QWidget*>("actionbar");
    Q_ASSERT(actionBar);

    QMargins popupMargins = window()->contentsMargins();
    if (actionBar)
        size.setHeight(qMax(actionBar->height() - statusBar->height() -
                            (popupMargins.top() + popupMargins.bottom()), height));
    return size;
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
    MiniTargetWidget *mtw = qobject_cast<MiniTargetWidget *>(itemWidget(item(index)));
    if (!mtw)
        return;
    m_project->setActiveTarget(mtw->target());
}

MiniTargetWidget::MiniTargetWidget(Target *target, QWidget *parent) :
    QWidget(parent), m_target(target)
{
    Q_ASSERT(m_target);

    if (hasBuildConfiguration()) {
        m_buildComboBox = new QComboBox;
        m_buildComboBox->setProperty("alignarrow", true);
        m_buildComboBox->setProperty("hideborder", true);
        m_buildComboBox->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred);
        m_buildComboBox->setToolTip(tr("Select active build configuration"));
    } else {
        m_buildComboBox = 0;
    }
 
    m_runComboBox = new QComboBox;
    m_runComboBox ->setProperty("alignarrow", true);
    m_runComboBox ->setProperty("hideborder", true);
    m_runComboBox->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred);
    m_runComboBox->setToolTip(tr("Select active run configuration"));
    int fontSize = font().pointSize();
    setStyleSheet(QString::fromLatin1("QLabel { font-size: %2pt; color: white; } "
                                      "#target { font: bold %1pt;} "
                                      "#buildLabel{ font: bold; color: rgba(255, 255, 255, 160)} "
                                      "#runLabel { font: bold ; color: rgba(255, 255, 255, 160)} "
                                      ).arg(fontSize).arg(fontSize - 2));

    QGridLayout *gridLayout = new QGridLayout(this);

    m_targetName = new QLabel(m_target->displayName());
    m_targetName->setObjectName(QString::fromUtf8("target"));
    m_targetIcon = new QLabel();
    updateIcon();
    if (hasBuildConfiguration()) {
        Q_FOREACH(BuildConfiguration* bc, m_target->buildConfigurations())
                addBuildConfiguration(bc);

        connect(m_target, SIGNAL(addedBuildConfiguration(ProjectExplorer::BuildConfiguration*)),
                SLOT(addBuildConfiguration(ProjectExplorer::BuildConfiguration*)));
        connect(m_target, SIGNAL(removedBuildConfiguration(ProjectExplorer::BuildConfiguration*)),
                SLOT(removeBuildConfiguration(ProjectExplorer::BuildConfiguration*)));

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
    formLayout->setLabelAlignment(Qt::AlignRight);
    QLabel *lbl;
    if (hasBuildConfiguration()) {
        lbl = new QLabel(tr("Build:"));
        lbl->setObjectName(QString::fromUtf8("buildLabel"));
        lbl->setIndent(10);
        formLayout->addRow(lbl, buildHelperLayout);
    }
    lbl = new QLabel(tr("Run:"));
    lbl->setObjectName(QString::fromUtf8("runLabel"));
    lbl->setIndent(10);
    formLayout->addRow(lbl, runHelperLayout);

    gridLayout->addWidget(m_targetName, 0, 0);
    gridLayout->addWidget(m_targetIcon, 0, 1, 2, 1, Qt::AlignTop|Qt::AlignHCenter);
    gridLayout->addLayout(formLayout, 1, 0);
}

void MiniTargetWidget::updateIcon()
{
    m_targetIcon->setPixmap(createCenteredIcon(m_target->icon(), QIcon()).pixmap(Core::Constants::TARGET_ICON_SIZE));
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

    m_runComboBox->setEnabled(m_runComboBox->count()>1);
}

void MiniTargetWidget::removeRunConfiguration(ProjectExplorer::RunConfiguration* rc)
{
    m_runComboBox->removeItem(m_runComboBox->findData(QVariant::fromValue(rc)));
    m_runComboBox->setEnabled(m_runComboBox->count()>1);
}

void MiniTargetWidget::addBuildConfiguration(ProjectExplorer::BuildConfiguration* bc)
{
    QTC_ASSERT(m_buildComboBox, return);
    connect(bc, SIGNAL(displayNameChanged()), SLOT(updateDisplayName()));
    m_buildComboBox->addItem(bc->displayName(), QVariant::fromValue(bc));
    if (m_target->activeBuildConfiguration() == bc)
        m_buildComboBox->setCurrentIndex(m_buildComboBox->count()-1);

    m_buildComboBox->setEnabled(m_buildComboBox->count() > 1);
}

void MiniTargetWidget::removeBuildConfiguration(ProjectExplorer::BuildConfiguration* bc)
{
    QTC_ASSERT(m_buildComboBox, return);
    m_buildComboBox->removeItem(m_buildComboBox->findData(QVariant::fromValue(bc)));
    m_buildComboBox->setEnabled(m_buildComboBox->count() > 1);
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
    setProperty("panelwidget", true);
    setContentsMargins(QMargins(0, 1, 1, 8));
    setWindowFlags(Qt::Popup);

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

    int panelHeight = lbl->fontMetrics().height() + 12;
    bar->ensurePolished(); // Required since manhattanstyle overrides height
    bar->setFixedHeight(panelHeight);

    m_projectsBox = new QComboBox;
    m_projectsBox->setToolTip(tr("Select active project"));
    m_projectsBox->setFocusPolicy(Qt::TabFocus);
    f.setBold(false);
    m_projectsBox->setFont(f);
    m_projectsBox->ensurePolished();
    m_projectsBox->setFixedHeight(panelHeight);
    m_projectsBox->setProperty("hideborder", true);
    m_projectsBox->setObjectName(QString::fromUtf8("ProjectsBox"));
    m_projectsBox->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred);
    m_projectsBox->setMaximumWidth(200);

    toolLayout->addWidget(lbl);
    toolLayout->addWidget(new Utils::StyledSeparator);
    toolLayout->addWidget(m_projectsBox);

    m_widgetStack = new QStackedWidget;
    m_widgetStack->setFocusPolicy(Qt::NoFocus);
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

void MiniProjectTargetSelector::addProject(ProjectExplorer::Project* project)
{
    QTC_ASSERT(project, return);
    ProjectListWidget *targetList = new ProjectListWidget(project);
    targetList->setStyleSheet(QString::fromLatin1("QListWidget { background: %1; border: none; }")
                              .arg(QColor(70, 70, 70).name()));
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

    m_projectsBox->setEnabled(m_projectsBox->count() > 1);
}

void MiniProjectTargetSelector::removeProject(ProjectExplorer::Project* project)
{
    int index = indexFor(project);
    if (index < 0)
        return;
    ProjectListWidget *plw = qobject_cast<ProjectListWidget*>(m_widgetStack->widget(index));
    m_projectsBox->removeItem(index);
    m_projectsBox->setEnabled(m_projectsBox->count() > 1);
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
    connect(target, SIGNAL(overlayIconChanged()), this, SLOT(updateAction()));
    ProjectListWidget *plw = qobject_cast<ProjectListWidget*>(m_widgetStack->widget(index));
    QListWidgetItem *lwi = new QListWidgetItem();

    // Sort on insert:
    for (int idx = 0; idx <= plw->count(); ++idx) {
        QListWidgetItem *itm(plw->item(idx));
        MiniTargetWidget *mtw(qobject_cast<MiniTargetWidget *>(plw->itemWidget(itm)));
        if (!mtw && idx < plw->count())
            continue;
        if (idx == plw->count() ||
            mtw->target()->displayName() > target->displayName()) {
            plw->insertItem(idx, lwi);
            break;
        }
    }

    MiniTargetWidget *targetWidget = new MiniTargetWidget(target);
    connect(targetWidget, SIGNAL(changed()), this, SLOT(updateAction()));
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
    disconnect(target, SIGNAL(toolTipChanged()), this, SLOT(updateAction()));
    disconnect(target, SIGNAL(iconChanged()), this, SLOT(updateAction()));
    disconnect(target, SIGNAL(overlayIconChanged()), this, SLOT(updateAction()));
}

void MiniProjectTargetSelector::updateAction()
{
    Project *project = ProjectExplorerPlugin::instance()->startupProject();

    QString projectName;;
    QString targetName;
    QString targetToolTipText;
    QString buildConfig;
    QString runConfig;
    QIcon targetIcon = style()->standardIcon(QStyle::SP_ComputerIcon);

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
            targetIcon = createCenteredIcon(target->icon(), target->overlayIcon());
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
