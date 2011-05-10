/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
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
** If you have questions regarding the use of this file, please contact
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#include "miniprojecttargetselector.h"
#include "buildconfigurationmodel.h"
#include "runconfigurationmodel.h"
#include "target.h"

#include <utils/qtcassert.h>
#include <utils/styledbar.h>
#include <utils/stylehelper.h>

#include <coreplugin/ifile.h>
#include <coreplugin/icore.h>
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
#include <QtGui/QAction>
#include <QtGui/QItemDelegate>
#include <QtGui/QMainWindow>

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
    int width = 0;
    for (int itemPos = 0; itemPos < count(); ++itemPos) {
        height += item(itemPos)->sizeHint().height();
        width = qMax(width, item(itemPos)->sizeHint().width());
    }

    // We try to keep the height of the popup equal to the actionbar
    QSize size(width, height);
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
        m_buildComboBox->setModel(new BuildConfigurationModel(m_target, this));
    } else {
        m_buildComboBox = 0;
    }
 
    m_runComboBox = new QComboBox;
    m_runComboBox ->setProperty("alignarrow", true);
    m_runComboBox ->setProperty("hideborder", true);
    m_runComboBox->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred);
    m_runComboBox->setToolTip(tr("Select active run configuration"));
    RunConfigurationModel *model = new RunConfigurationModel(m_target, this);
    m_runComboBox->setModel(model);
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

        BuildConfigurationModel *model = static_cast<BuildConfigurationModel *>(m_buildComboBox->model());
        m_buildComboBox->setCurrentIndex(model->indexFor(m_target->activeBuildConfiguration()).row());

        connect(m_target, SIGNAL(addedBuildConfiguration(ProjectExplorer::BuildConfiguration*)),
                SLOT(addBuildConfiguration(ProjectExplorer::BuildConfiguration*)));
        connect(m_target, SIGNAL(removedBuildConfiguration(ProjectExplorer::BuildConfiguration*)),
                SLOT(removeBuildConfiguration(ProjectExplorer::BuildConfiguration*)));

        connect(m_target, SIGNAL(activeBuildConfigurationChanged(ProjectExplorer::BuildConfiguration*)),
                SLOT(setActiveBuildConfiguration()));
        connect(m_buildComboBox, SIGNAL(currentIndexChanged(int)), SLOT(setActiveBuildConfiguration(int)));
    }

    m_runComboBox->setEnabled(m_target->runConfigurations().count() > 1);
    m_runComboBox->setCurrentIndex(model->indexFor(m_target->activeRunConfiguration()).row());

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
    int indent = 10;
    if (hasBuildConfiguration()) {
        lbl = new QLabel(tr("Build:"));
        lbl->setObjectName(QString::fromUtf8("buildLabel"));
        lbl->setMinimumWidth(lbl->fontMetrics().width(lbl->text()) + indent + 4);
        lbl->setIndent(indent);

        formLayout->addRow(lbl, buildHelperLayout);
    }
    lbl = new QLabel(tr("Run:"));
    lbl->setObjectName(QString::fromUtf8("runLabel"));
    lbl->setMinimumWidth(lbl->fontMetrics().width(lbl->text()) + indent + 4);
    lbl->setIndent(indent);
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
    BuildConfigurationModel *model = static_cast<BuildConfigurationModel *>(m_buildComboBox->model());
    m_target->setActiveBuildConfiguration(model->buildConfigurationAt(index));
    emit changed();
}

void MiniTargetWidget::setActiveRunConfiguration(int index)
{
    RunConfigurationModel *model = static_cast<RunConfigurationModel *>(m_runComboBox->model());
    m_target->setActiveRunConfiguration(model->runConfigurationAt(index));
    updateIcon();
    emit changed();
}

void MiniTargetWidget::setActiveBuildConfiguration()
{
    QTC_ASSERT(m_buildComboBox, return);
    BuildConfigurationModel *model = static_cast<BuildConfigurationModel *>(m_buildComboBox->model());
    m_buildComboBox->setCurrentIndex(model->indexFor(m_target->activeBuildConfiguration()).row());
}

void MiniTargetWidget::setActiveRunConfiguration()
{
    RunConfigurationModel *model = static_cast<RunConfigurationModel *>(m_runComboBox->model());
    m_runComboBox->setCurrentIndex(model->indexFor(m_target->activeRunConfiguration()).row());
}

void MiniTargetWidget::addRunConfiguration(ProjectExplorer::RunConfiguration* rc)
{
    Q_UNUSED(rc);
    m_runComboBox->setEnabled(m_target->runConfigurations().count()>1);
}

void MiniTargetWidget::removeRunConfiguration(ProjectExplorer::RunConfiguration* rc)
{
    Q_UNUSED(rc);
    m_runComboBox->setEnabled(m_target->runConfigurations().count()>1);
}

void MiniTargetWidget::addBuildConfiguration(ProjectExplorer::BuildConfiguration* bc)
{
    Q_UNUSED(bc);
    connect(bc, SIGNAL(displayNameChanged()), SIGNAL(changed()), Qt::UniqueConnection);
    m_buildComboBox->setEnabled(m_target->buildConfigurations().count() > 1);
}

void MiniTargetWidget::removeBuildConfiguration(ProjectExplorer::BuildConfiguration* bc)
{
    Q_UNUSED(bc);
    QTC_ASSERT(m_buildComboBox, return);
    m_buildComboBox->setEnabled(m_target->buildConfigurations().count() > 1);
}

bool MiniTargetWidget::hasBuildConfiguration() const
{
    return (m_target->buildConfigurationFactory() != 0);
}

MiniProjectTargetSelector::MiniProjectTargetSelector(QAction *targetSelectorAction, QWidget *parent) :
    QWidget(parent), m_projectAction(targetSelectorAction), m_ignoreIndexChange(false)
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
    m_projectsBox->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    m_projectsBox->setSizeAdjustPolicy(QComboBox::AdjustToContents);

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
        QSize sh = sizeHint();
        resize(sh);
        QStatusBar *statusBar = Core::ICore::instance()->statusBar();
        QPoint moveTo = statusBar->mapToGlobal(QPoint(0,0));
        moveTo -= QPoint(0, sh.height());
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

QString MiniProjectTargetSelector::fullName(ProjectExplorer::Project *project)
{
    return project->displayName() + " (" + project->file()->fileName() + QLatin1Char(')');
}

void MiniProjectTargetSelector::addProject(ProjectExplorer::Project *project)
{
    QTC_ASSERT(project, return);
    ProjectListWidget *targetList = new ProjectListWidget(project);
    targetList->setStyleSheet(QString::fromLatin1("QListWidget { background: %1; border: none; }")
                              .arg(QColor(70, 70, 70).name()));

    m_ignoreIndexChange = true;

    QString sortName = fullName(project);
    int pos = 0;
    for (int i=0; i < m_projectsBox->count(); ++i) {
        Project *p = m_projectsBox->itemData(i).value<Project*>();
        QString itemSortName = fullName(p);
        if (itemSortName > sortName)
            pos = i;
    }

    m_widgetStack->insertWidget(pos, targetList);

    bool useFullName = false;
    for (int i = 0; i < m_projectsBox->count(); ++i) {
        Project *p = m_projectsBox->itemData(i).value<Project*>();
        if (p->displayName() == project->displayName()) {
            useFullName = true;
            m_projectsBox->setItemText(i, fullName(p));
        }
    }

    QString displayName = useFullName ? fullName(project) : project->displayName();
    m_projectsBox->insertItem(pos, displayName, QVariant::fromValue(project));

    connect(project, SIGNAL(activeTargetChanged(ProjectExplorer::Target*)),
            SLOT(updateAction()));

    connect(project, SIGNAL(addedTarget(ProjectExplorer::Target*)),
            SLOT(addTarget(ProjectExplorer::Target*)));
    connect(project, SIGNAL(removedTarget(ProjectExplorer::Target*)),
            SLOT(removeTarget(ProjectExplorer::Target*)));
    connect(project, SIGNAL(activeTargetChanged(ProjectExplorer::Target*)),
            SLOT(changeActiveTarget(ProjectExplorer::Target*)));

    if (project == ProjectExplorerPlugin::instance()->startupProject()) {
        m_projectsBox->setCurrentIndex(pos);
        m_widgetStack->setCurrentIndex(pos);
    }

    m_ignoreIndexChange = false;

    foreach (Target *t, project->targets())
        addTarget(t, t == project->activeTarget());

    m_projectsBox->setEnabled(m_projectsBox->count() > 1);
    m_projectsBox->parentWidget()->layout()->activate();
}

void MiniProjectTargetSelector::removeProject(ProjectExplorer::Project* project)
{
    int index = indexFor(project);
    if (index < 0)
        return;
    ProjectListWidget *plw = qobject_cast<ProjectListWidget*>(m_widgetStack->widget(index));

    // We don't want to emit a startUpProject changed, even if we remove the current startup project
    // The ProjectExplorer takes care of setting a new startup project.
    m_ignoreIndexChange = true;
    m_projectsBox->removeItem(index);
    m_projectsBox->setEnabled(m_projectsBox->count() > 1);
    delete plw;

    // Update display names
    QString name = project->displayName();
    int count = 0;
    int otherIndex;
    for (int i = 0; i < m_projectsBox->count(); ++i) {
        Project *p = m_projectsBox->itemData(i).value<Project*>();
        if (p->displayName() == name) {
            ++count;
            otherIndex = i;
        }
    }
    if (count == 1) {
        Project *p = m_projectsBox->itemData(otherIndex).value<Project*>();
        m_projectsBox->setItemText(otherIndex, p->displayName());
    }

    m_ignoreIndexChange = false;
    m_projectsBox->parentWidget()->layout()->activate();
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
    lwi->setSizeHint(targetWidget->sizeHint());
    plw->setItemWidget(lwi, targetWidget);

    if (activeTarget)
        plw->setCurrentItem(lwi, QItemSelectionModel::SelectCurrent);

    m_widgetStack->updateGeometry();
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

    m_widgetStack->updateGeometry();
}

void MiniProjectTargetSelector::changeActiveTarget(ProjectExplorer::Target *target)
{
    int index = indexFor(target->project());
    if (index < 0)
        return;
    ProjectListWidget *plw = qobject_cast<ProjectListWidget*>(m_widgetStack->widget(index));

    for (int i = 0; i < plw->count(); ++i) {
        QListWidgetItem *itm = plw->item(i);
        MiniTargetWidget *mtw = qobject_cast<MiniTargetWidget*>(plw->itemWidget(itm));
        if (mtw->target() == target) {
            plw->setCurrentItem(itm);
            break;
        }
    }
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

    const int extrawidth = 110; // Size of margins + icon width
    // Some fudge numbers to ensure the menu does not grow unbounded
    int maxLength = fontMetrics().averageCharWidth() * 140;

    if (project) {
        projectName = project->displayName();

        if (Target *target = project->activeTarget()) {
            if (project->targets().count() > 1) {
                targetName = project->activeTarget()->displayName();
            }
            if (BuildConfiguration *bc = target->activeBuildConfiguration()) {
                buildConfig = bc->displayName();
                int minimumWidth = fontMetrics().width(bc->displayName() + tr("Build:")) + extrawidth;
                m_widgetStack->setMinimumWidth(qMin(maxLength, qMax(minimumWidth, m_widgetStack->minimumWidth())));
            }

            if (RunConfiguration *rc = target->activeRunConfiguration()) {
                runConfig = rc->displayName();
                int minimumWidth = fontMetrics().width(rc->displayName() + tr("Run:")) + extrawidth;
                m_widgetStack->setMinimumWidth(qMin(maxLength, qMax(minimumWidth, m_widgetStack->minimumWidth())));
            }
            targetToolTipText = target->toolTip();
            targetIcon = createCenteredIcon(target->icon(), target->overlayIcon());
        }
    }
    m_projectAction->setProperty("heading", projectName);
    m_projectAction->setProperty("subtitle", buildConfig);
    m_projectAction->setIcon(targetIcon);
    QString toolTip = tr("<html><nobr><b>Project:</b> %1<br/>%2%3<b>Run:</b> %4%5</html>");
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
    if (m_ignoreIndexChange)
        return;
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
