/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "buildconfiguration.h"
#include "buildmanager.h"
#include "deployconfiguration.h"
#include "kit.h"
#include "kitmanager.h"
#include "miniprojecttargetselector.h"
#include "projectexplorer.h"
#include "projectexplorericons.h"
#include "project.h"
#include "projectmodels.h"
#include "runconfiguration.h"
#include "session.h"
#include "target.h"

#include <utils/algorithm.h>
#include <utils/itemviews.h>
#include <utils/stringutils.h>
#include <utils/styledbar.h>
#include <utils/stylehelper.h>
#include <utils/theme/theme.h>
#include <utils/utilsicons.h>

#include <coreplugin/icore.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/modemanager.h>

#include <QGuiApplication>
#include <QTimer>
#include <QLayout>
#include <QLabel>
#include <QList>
#include <QListWidget>
#include <QStatusBar>
#include <QKeyEvent>
#include <QPainter>
#include <QPixmap>
#include <QStyleFactory>
#include <QAction>
#include <QItemDelegate>

using namespace Utils;

namespace ProjectExplorer {
namespace Internal {

const int RunColumnWidth = 30;

static QIcon createCenteredIcon(const QIcon &icon, const QIcon &overlay)
{
    QPixmap targetPixmap;
    const qreal appDevicePixelRatio = qApp->devicePixelRatio();
    const auto deviceSpaceIconSize = static_cast<int>(Core::Constants::MODEBAR_ICON_SIZE * appDevicePixelRatio);
    targetPixmap = QPixmap(deviceSpaceIconSize, deviceSpaceIconSize);
    targetPixmap.setDevicePixelRatio(appDevicePixelRatio);
    targetPixmap.fill(Qt::transparent);
    QPainter painter(&targetPixmap); // painter in user space

    QPixmap pixmap = icon.pixmap(Core::Constants::MODEBAR_ICON_SIZE); // already takes app devicePixelRatio into account
    qreal pixmapDevicePixelRatio = pixmap.devicePixelRatio();
    painter.drawPixmap((Core::Constants::MODEBAR_ICON_SIZE - pixmap.width() / pixmapDevicePixelRatio) / 2,
                       (Core::Constants::MODEBAR_ICON_SIZE - pixmap.height() / pixmapDevicePixelRatio) / 2, pixmap);
    if (!overlay.isNull()) {
        pixmap = overlay.pixmap(Core::Constants::MODEBAR_ICON_SIZE); // already takes app devicePixelRatio into account
        pixmapDevicePixelRatio = pixmap.devicePixelRatio();
        painter.drawPixmap((Core::Constants::MODEBAR_ICON_SIZE - pixmap.width() / pixmapDevicePixelRatio) / 2,
                           (Core::Constants::MODEBAR_ICON_SIZE - pixmap.height() / pixmapDevicePixelRatio) / 2, pixmap);
    }

    return QIcon(targetPixmap);
}

class GenericItem : public TreeItem
{
public:
    GenericItem() = default;
    GenericItem(QObject *object) : m_object(object) {}
    QObject *object() const { return m_object; }
    QString rawDisplayName() const
    {
        if (const auto p = qobject_cast<Project *>(object()))
            return p->displayName();
        if (const auto t = qobject_cast<Target *>(object()))
            return t->displayName();
        return static_cast<ProjectConfiguration *>(object())->displayName();

    }
    QString displayName() const
    {
        if (const auto p = qobject_cast<Project *>(object())) {
            const auto hasSameProjectName = [this](TreeItem *ti) {
                return ti != this
                        && static_cast<GenericItem *>(ti)->rawDisplayName() == rawDisplayName();
            };
            QString displayName = p->displayName();
            if (parent()->findAnyChild(hasSameProjectName)) {
                displayName.append(" (").append(p->projectFilePath().toUserOutput())
                        .append(')');
            }
            return displayName;
        }
        return rawDisplayName();
    }

private:
    QVariant toolTip() const
    {
        if (qobject_cast<Project *>(object()))
            return {};
        if (const auto t = qobject_cast<Target *>(object()))
            return t->toolTip();
        return static_cast<ProjectConfiguration *>(object())->toolTip();
    }

    QVariant data(int column, int role) const override
    {
        if (column == 1 && role == Qt::ToolTipRole)
            return QCoreApplication::translate("RunConfigSelector", "Run Without Deployment");
        if (column != 0)
            return {};
        switch (role) {
        case Qt::DisplayRole:
            return displayName();
        case Qt::ToolTipRole:
            return toolTip();
        default:
            break;
        }
        return {};
    }

    QObject *m_object = nullptr;
};

static bool compareItems(const TreeItem *ti1, const TreeItem *ti2)
{
    const int result = caseFriendlyCompare(static_cast<const GenericItem *>(ti1)->rawDisplayName(),
                                           static_cast<const GenericItem *>(ti2)->rawDisplayName());
    if (result != 0)
        return result < 0;
    return ti1 < ti2;
}

class GenericModel : public TreeModel<GenericItem, GenericItem>
{
    Q_OBJECT
public:
    GenericModel(QObject *parent) : TreeModel(parent) { }

    void rebuild(const QList<QObject *> &objects)
    {
        clear();
        for (QObject * const e : objects)
            addItemForObject(e);
    }

    const GenericItem *addItemForObject(QObject *object)
    {
        const auto item = new GenericItem(object);
        rootItem()->insertOrderedChild(item, &compareItems);
        if (const auto project = qobject_cast<Project *>(object)) {
            connect(project, &Project::displayNameChanged,
                    this, &GenericModel::displayNameChanged);
        } else if (const auto target = qobject_cast<Target *>(object)) {
            connect(target, &Target::kitChanged, this, &GenericModel::displayNameChanged);
        } else {
            const auto pc = qobject_cast<ProjectConfiguration *>(object);
            QTC_CHECK(pc);
            connect(pc, &ProjectConfiguration::displayNameChanged,
                    this, &GenericModel::displayNameChanged);
            connect(pc, &ProjectConfiguration::toolTipChanged,
                    this, &GenericModel::updateToolTips);
        }
        return item;
    }

    GenericItem *itemForObject(const QObject *object) const
    {
        return findItemAtLevel<1>([object](const GenericItem *item) {
            return item->object() == object;
        });
    }

    void setColumnCount(int columns) { m_columnCount = columns; }

signals:
    void displayNameChanged();

private:
    void updateToolTips()
    {
        emit dataChanged(index(0, 0), index(rowCount() - 1, 0), {Qt::ToolTipRole});
    }
};

class SelectorView : public TreeView
{
    Q_OBJECT

public:
    SelectorView(QWidget *parent);

    void setMaxCount(int maxCount);
    int maxCount();

    int optimalWidth() const;
    void setOptimalWidth(int width);

    int padding();

    GenericModel *theModel() const { return static_cast<GenericModel *>(model()); }

protected:
    void resetOptimalWidth()
    {
        int width = 0;
        QFontMetrics fn(font());
        theModel()->forItemsAtLevel<1>([this, &width, &fn](const GenericItem *item) {
            width = qMax(fn.horizontalAdvance(item->displayName()) + padding(), width);
        });
        setOptimalWidth(width);
    }

private:
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;

    int m_maxCount = 0;
    int m_optimalWidth = 0;
};

class ProjectListView : public SelectorView
{
    Q_OBJECT

public:
    explicit ProjectListView(QWidget *parent = nullptr) : SelectorView(parent)
    {
        const auto model = new GenericModel(this);
        model->rebuild(transform<QList<QObject *>>(SessionManager::projects(),
                                                   [](Project *p) { return p; }));
        connect(SessionManager::instance(), &SessionManager::projectAdded,
                this, [this, model](Project *project) {
            const GenericItem *projectItem = model->addItemForObject(project);
            QFontMetrics fn(font());
            const int width = fn.horizontalAdvance(projectItem->displayName()) + padding();
            if (width > optimalWidth())
                setOptimalWidth(width);
            restoreCurrentIndex();
        });
        connect(SessionManager::instance(), &SessionManager::aboutToRemoveProject,
                this, [this, model](const Project *project) {
            GenericItem * const item = model->itemForObject(project);
            if (!item)
                return;
            model->destroyItem(item);
            resetOptimalWidth();
        });
        connect(SessionManager::instance(), &SessionManager::startupProjectChanged,
                this, [this, model](const Project *project) {
            const GenericItem * const item = model->itemForObject(project);
            if (item)
                setCurrentIndex(item->index());
        });
        connect(model, &GenericModel::displayNameChanged, this, [this, model] {
            model->rootItem()->sortChildren(&compareItems);
            resetOptimalWidth();
            restoreCurrentIndex();
        });
        setModel(model);
        connect(selectionModel(), &QItemSelectionModel::currentChanged,
                this, [model](const QModelIndex &index) {
            const GenericItem * const item = model->itemForIndex(index);
            if (item && item->object())
                SessionManager::setStartupProject(qobject_cast<Project *>(item->object()));
        });
    }

private:
    void restoreCurrentIndex()
    {
        const GenericItem * const itemForStartupProject
                = theModel()->itemForObject(SessionManager::startupProject());
        if (itemForStartupProject)
            setCurrentIndex(theModel()->indexForItem(itemForStartupProject));
    }
};

class GenericListWidget : public SelectorView
{
    Q_OBJECT

public:
    explicit GenericListWidget(QWidget *parent = nullptr) : SelectorView(parent)
    {
        const auto model = new GenericModel(this);
        connect(model, &GenericModel::displayNameChanged, this, [this, model] {
            const GenericItem * const activeItem = model->itemForIndex(currentIndex());
            model->rootItem()->sortChildren(&compareItems);
            resetOptimalWidth();
            if (activeItem)
                setCurrentIndex(activeItem->index());
        });
        setModel(model);
        connect(selectionModel(), &QItemSelectionModel::currentChanged,
                this, &GenericListWidget::rowChanged);
    }

signals:
    void changeActiveProjectConfiguration(QObject *pc);

public:
    void setProjectConfigurations(const QList<QObject *> &list, QObject *active)
    {
        theModel()->rebuild(list);
        resetOptimalWidth();
        setActiveProjectConfiguration(active);
    }

    void setActiveProjectConfiguration(QObject *active)
    {
        if (const GenericItem * const item = theModel()->itemForObject(active))
            setCurrentIndex(item->index());
    }

    void addProjectConfiguration(QObject *pc)
    {
        const auto activeItem = theModel()->itemForIndex(currentIndex());
        const auto item = theModel()->addItemForObject(pc);
        QFontMetrics fn(font());
        const int width = fn.horizontalAdvance(item->displayName()) + padding();
        if (width > optimalWidth())
            setOptimalWidth(width);
        if (activeItem)
            setCurrentIndex(activeItem->index());
    }

    void removeProjectConfiguration(QObject *pc)
    {
        const auto activeItem = theModel()->itemForIndex(currentIndex());
        if (GenericItem * const item = theModel()->itemForObject(pc)) {
            theModel()->destroyItem(item);
            resetOptimalWidth();
            if (activeItem && activeItem != item)
                setCurrentIndex(activeItem->index());
        }
    }

private:
    void mousePressEvent(QMouseEvent *event) override
    {
        const QModelIndex pressedIndex = indexAt(event->pos());
        if (pressedIndex.column() == 1) {
            m_pressedIndex = pressedIndex;
            return; // Clicking on the run button should not change the current index
        }
        m_pressedIndex = QModelIndex();
        TreeView::mousePressEvent(event);
    }

    void mouseReleaseEvent(QMouseEvent *event) override
    {
        const QModelIndex pressedIndex = m_pressedIndex;
        m_pressedIndex = QModelIndex();
        if (pressedIndex.isValid() && pressedIndex == indexAt(event->pos())) {
            const auto rc = qobject_cast<RunConfiguration *>(
                        theModel()->itemForIndex(pressedIndex)->object());
            QTC_ASSERT(rc, return);
            if (!BuildManager::isBuilding(rc->project()))
                ProjectExplorerPlugin::runRunConfiguration(rc, Constants::NORMAL_RUN_MODE, true);
            return;
        }
        TreeView::mouseReleaseEvent(event);
    }

    QObject *objectAt(const QModelIndex &index) const
    {
        return theModel()->itemForIndex(index)->object();
    }

    void rowChanged(const QModelIndex &index)
    {
        if (index.isValid())
            emit changeActiveProjectConfiguration(objectAt(index));
    }

    QModelIndex m_pressedIndex;
};

////////
// TargetSelectorDelegate
////////
class TargetSelectorDelegate : public QItemDelegate
{
public:
    TargetSelectorDelegate(SelectorView *parent) : QItemDelegate(parent), m_view(parent) { }
private:
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    void paint(QPainter *painter,
               const QStyleOptionViewItem &option,
               const QModelIndex &index) const override;
    SelectorView *m_view;
};

QSize TargetSelectorDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    Q_UNUSED(option)
    Q_UNUSED(index)
    return QSize(m_view->size().width(), 30);
}

void TargetSelectorDelegate::paint(QPainter *painter,
                                   const QStyleOptionViewItem &option,
                                   const QModelIndex &index) const
{
    painter->save();
    painter->setClipping(false);

    QColor textColor = creatorTheme()->color(Theme::MiniProjectTargetSelectorTextColor);
    if (option.state & QStyle::State_Selected) {
        QColor color;
        if (m_view->hasFocus()) {
            color = option.palette.highlight().color();
            textColor = option.palette.highlightedText().color();
        } else {
            color = option.palette.dark().color();
        }

        if (creatorTheme()->flag(Theme::FlatToolBars)) {
            painter->fillRect(option.rect, color);
        } else {
            painter->fillRect(option.rect, color.darker(140));
            static const QImage selectionGradient(":/projectexplorer/images/targetpanel_gradient.png");
            StyleHelper::drawCornerImage(selectionGradient, painter, option.rect.adjusted(0, 0, 0, -1), 5, 5, 5, 5);
            const QRectF borderRect = QRectF(option.rect).adjusted(0.5, 0.5, -0.5, -0.5);
            painter->setPen(QColor(255, 255, 255, 60));
            painter->drawLine(borderRect.topLeft(), borderRect.topRight());
            painter->setPen(QColor(255, 255, 255, 30));
            painter->drawLine(borderRect.bottomLeft() - QPointF(0, 1), borderRect.bottomRight() - QPointF(0, 1));
            painter->setPen(QColor(0, 0, 0, 80));
            painter->drawLine(borderRect.bottomLeft(), borderRect.bottomRight());
        }
    }

    QFontMetrics fm(option.font);
    QString text = index.data(Qt::DisplayRole).toString();
    painter->setPen(textColor);
    QString elidedText = fm.elidedText(text, Qt::ElideMiddle, option.rect.width() - 12);
    if (elidedText != text)
        const_cast<QAbstractItemModel *>(index.model())->setData(index, text, Qt::ToolTipRole);
    else
        const_cast<QAbstractItemModel *>(index.model())
            ->setData(index, index.model()->data(index, Qt::UserRole + 1).toString(), Qt::ToolTipRole);
    painter->drawText(option.rect.left() + 6, option.rect.top() + (option.rect.height() - fm.height()) / 2 + fm.ascent(), elidedText);
    if (index.column() == 1 && option.state & QStyle::State_MouseOver) {
        const QIcon icon = Utils::Icons::RUN_SMALL_TOOLBAR.icon();
        QRect iconRect(0, 0, 16, 16);
        iconRect.moveCenter(option.rect.center());
        icon.paint(painter, iconRect);
    }

    painter->restore();
}

////////
// ListWidget
////////
SelectorView::SelectorView(QWidget *parent) : TreeView(parent)
{
    setFocusPolicy(Qt::NoFocus);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setAlternatingRowColors(false);
    setUniformRowHeights(true);
    setIndentation(0);
    setFocusPolicy(Qt::WheelFocus);
    setItemDelegate(new TargetSelectorDelegate(this));
    setSelectionBehavior(SelectRows);
    setAttribute(Qt::WA_MacShowFocusRect, false);
    setHeaderHidden(true);
    const QColor bgColor = creatorTheme()->color(Theme::MiniProjectTargetSelectorBackgroundColor);
    const QString bgColorName = creatorTheme()->flag(Theme::FlatToolBars)
            ? bgColor.lighter(120).name() : bgColor.name();
    setStyleSheet(QString::fromLatin1("QAbstractItemView { background: %1; border-style: none; }").arg(bgColorName));
    setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
}

void SelectorView::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Left)
        focusPreviousChild();
    else if (event->key() == Qt::Key_Right)
        focusNextChild();
    else
        TreeView::keyPressEvent(event);
}

void SelectorView::keyReleaseEvent(QKeyEvent *event)
{
    if (event->key() != Qt::Key_Left && event->key() != Qt::Key_Right)
        TreeView::keyReleaseEvent(event);
}

void SelectorView::setMaxCount(int maxCount)
{
    m_maxCount = maxCount;
    updateGeometry();
}

int SelectorView::maxCount()
{
    return m_maxCount;
}

int SelectorView::optimalWidth() const
{
    return m_optimalWidth;
}

void SelectorView::setOptimalWidth(int width)
{
    m_optimalWidth = width;
    if (model()->columnCount() == 2)
        m_optimalWidth += RunColumnWidth;
    updateGeometry();
}

int SelectorView::padding()
{
    // there needs to be enough extra pixels to show a scrollbar
    return 2 * style()->pixelMetric(QStyle::PM_FocusFrameHMargin, nullptr, this)
            + style()->pixelMetric(QStyle::PM_ScrollBarExtent, nullptr, this)
            + 10;
}

/////////
// KitAreaWidget
/////////
class KitAreaWidget : public QWidget
{
    Q_OBJECT
public:
    explicit KitAreaWidget(QWidget *parent = nullptr)
        : QWidget(parent), m_layout(new QGridLayout(this))
    {
        m_layout->setContentsMargins(3, 3, 3, 3);
        connect(KitManager::instance(), &KitManager::kitUpdated, this, &KitAreaWidget::updateKit);
    }

    ~KitAreaWidget() override { setKit(nullptr); }

    void setKit(ProjectExplorer::Kit *k)
    {
        qDeleteAll(m_widgets);
        m_widgets.clear();

        if (!k)
            return;

        foreach (QLabel *l, m_labels)
            l->deleteLater();
        m_labels.clear();

        int row = 0;
        for (KitAspect *aspect : KitManager::kitAspects()) {
            if (k && k->isMutable(aspect->id())) {
                KitAspectWidget *widget = aspect->createConfigWidget(k);
                m_widgets << widget;
                QLabel *label = new QLabel(aspect->displayName());
                m_labels << label;

                m_layout->addWidget(label, row, 0);
                m_layout->addWidget(widget->mainWidget(), row, 1);
                m_layout->addWidget(widget->buttonWidget(), row, 2);

                ++row;
            }
        }
        m_kit = k;

        setHidden(m_widgets.isEmpty());
    }

private:
    void updateKit(ProjectExplorer::Kit *k)
    {
        if (!m_kit || m_kit != k)
            return;

        bool addedMutables = false;
        QList<Utils::Id> knownIdList = Utils::transform(m_widgets, &KitAspectWidget::kitInformationId);

        for (KitAspect *aspect : KitManager::kitAspects()) {
            const Utils::Id currentId = aspect->id();
            if (m_kit->isMutable(currentId) && !knownIdList.removeOne(currentId)) {
                addedMutables = true;
                break;
            }
        }
        const bool removedMutables = !knownIdList.isEmpty();

        if (addedMutables || removedMutables) {
            // Redo whole setup if the number of mutable settings did change
            setKit(m_kit);
        } else {
            // Refresh all widgets if the number of mutable settings did not change
            foreach (KitAspectWidget *w, m_widgets)
                w->refresh();
        }
    }

    QGridLayout *m_layout;
    Kit *m_kit = nullptr;
    QList<KitAspectWidget *> m_widgets;
    QList<QLabel *> m_labels;
};

/////////
// MiniProjectTargetSelector
/////////

QWidget *MiniProjectTargetSelector::createTitleLabel(const QString &text)
{
    auto *bar = new StyledBar(this);
    bar->setSingleRow(true);
    auto *toolLayout = new QVBoxLayout(bar);
    toolLayout->setContentsMargins(6, 0, 6, 0);
    toolLayout->setSpacing(0);

    QLabel *l = new QLabel(text);
    QFont f = l->font();
    f.setBold(true);
    l->setFont(f);
    toolLayout->addWidget(l);

    int panelHeight = l->fontMetrics().height() + 12;
    bar->ensurePolished(); // Required since manhattanstyle overrides height
    bar->setFixedHeight(panelHeight);
    return bar;
}

MiniProjectTargetSelector::MiniProjectTargetSelector(QAction *targetSelectorAction, QWidget *parent) :
    QWidget(parent),
    m_projectAction(targetSelectorAction)
{
    setProperty("panelwidget", true);
    setContentsMargins(QMargins(0, 1, 1, 8));
    setWindowFlags(Qt::Popup);

    targetSelectorAction->setIcon(creatorTheme()->flag(Theme::FlatSideBarIcons)
                                  ? Icons::DESKTOP_DEVICE.icon()
                                  : style()->standardIcon(QStyle::SP_ComputerIcon));
    targetSelectorAction->setProperty("titledAction", true);

    m_kitAreaWidget = new KitAreaWidget(this);

    m_summaryLabel = new QLabel(this);
    m_summaryLabel->setContentsMargins(3, 3, 3, 3);
    m_summaryLabel->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    QPalette pal = m_summaryLabel->palette();
    pal.setColor(QPalette::Window, Utils::StyleHelper().baseColor());
    m_summaryLabel->setPalette(pal);
    m_summaryLabel->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
    m_summaryLabel->setTextInteractionFlags(m_summaryLabel->textInteractionFlags() | Qt::LinksAccessibleByMouse);

    m_listWidgets.resize(LAST);
    m_titleWidgets.resize(LAST);
    m_listWidgets[PROJECT] = nullptr; //project is not a generic list widget

    m_titleWidgets[PROJECT] = createTitleLabel(tr("Project"));
    m_projectListWidget = new ProjectListView(this);
    connect(m_projectListWidget, &QAbstractItemView::doubleClicked,
            this, &MiniProjectTargetSelector::hide);

    QStringList titles;
    titles << tr("Kit") << tr("Build")
           << tr("Deploy") << tr("Run");

    for (int i = TARGET; i < LAST; ++i) {
        m_titleWidgets[i] = createTitleLabel(titles.at(i -1));
        m_listWidgets[i] = new GenericListWidget(this);
        connect(m_listWidgets[i], &QAbstractItemView::doubleClicked,
                this, &MiniProjectTargetSelector::hide);
    }
    m_listWidgets[RUN]->theModel()->setColumnCount(2);
    m_listWidgets[RUN]->viewport()->setAttribute(Qt::WA_Hover);

    // Validate state: At this point the session is still empty!
    Project *startup = SessionManager::startupProject();
    QTC_CHECK(!startup);
    QTC_CHECK(SessionManager::projects().isEmpty());

    connect(m_summaryLabel, &QLabel::linkActivated,
            this, &MiniProjectTargetSelector::switchToProjectsMode);

    SessionManager *sessionManager = SessionManager::instance();
    connect(sessionManager, &SessionManager::startupProjectChanged,
            this, &MiniProjectTargetSelector::changeStartupProject);

    connect(sessionManager, &SessionManager::projectAdded,
            this, &MiniProjectTargetSelector::projectAdded);
    connect(sessionManager, &SessionManager::projectRemoved,
            this, &MiniProjectTargetSelector::projectRemoved);
    connect(sessionManager, &SessionManager::projectDisplayNameChanged,
            this, &MiniProjectTargetSelector::updateActionAndSummary);

    // for icon changes:
    connect(ProjectExplorer::KitManager::instance(), &KitManager::kitUpdated,
            this, &MiniProjectTargetSelector::kitChanged);

    connect(m_listWidgets[TARGET], &GenericListWidget::changeActiveProjectConfiguration,
            this, [this](QObject *pc) {
                SessionManager::setActiveTarget(m_project, static_cast<Target *>(pc), SetActive::Cascade);
            });
    connect(m_listWidgets[BUILD], &GenericListWidget::changeActiveProjectConfiguration,
            this, [this](QObject *pc) {
                 SessionManager::setActiveBuildConfiguration(m_project->activeTarget(),
                                                             static_cast<BuildConfiguration *>(pc), SetActive::Cascade);
            });
    connect(m_listWidgets[DEPLOY], &GenericListWidget::changeActiveProjectConfiguration,
            this, [this](QObject *pc) {
                 SessionManager::setActiveDeployConfiguration(m_project->activeTarget(),
                                                              static_cast<DeployConfiguration *>(pc), SetActive::Cascade);
            });
    connect(m_listWidgets[RUN], &GenericListWidget::changeActiveProjectConfiguration,
            this, [this](QObject *pc) {
                 m_project->activeTarget()->setActiveRunConfiguration(static_cast<RunConfiguration *>(pc));
            });
}

bool MiniProjectTargetSelector::event(QEvent *event)
{
    if (event->type() == QEvent::LayoutRequest) {
        doLayout(true);
        return true;
    } else if (event->type() == QEvent::ShortcutOverride) {
        if (static_cast<QKeyEvent *>(event)->key() == Qt::Key_Escape) {
            event->accept();
            return true;
        }
    }
    return QWidget::event(event);
}

// does some fancy calculations to ensure proper widths for the list widgets
QVector<int> MiniProjectTargetSelector::listWidgetWidths(int minSize, int maxSize)
{
    QVector<int> result;
    result.resize(LAST);
    if (m_projectListWidget->isVisibleTo(this))
        result[PROJECT] = m_projectListWidget->optimalWidth();
    else
        result[PROJECT] = -1;

    for (int i = TARGET; i < LAST; ++i) {
        if (m_listWidgets[i]->isVisibleTo(this))
            result[i] = m_listWidgets[i]->optimalWidth();
        else
            result[i] = -1;
    }

    int totalWidth = 0;
    // Adjust to minimum width of title
    for (int i = PROJECT; i < LAST; ++i) {
        if (result[i] != -1) {
            // We want at least 100 pixels per column
            int width = qMax(m_titleWidgets[i]->sizeHint().width(), 100);
            if (result[i] < width)
                result[i] = width;
            totalWidth += result[i];
        }
    }

    if (totalWidth == 0) // All hidden
        return result;

    bool tooSmall;
    if (totalWidth < minSize)
        tooSmall = true;
    else if (totalWidth > maxSize)
        tooSmall = false;
    else
        return result;

    int widthToDistribute = tooSmall ? (minSize - totalWidth)
                                     : (totalWidth - maxSize);
    QVector<int> indexes;
    indexes.reserve(LAST);
    for (int i = PROJECT; i < LAST; ++i)
        if (result[i] != -1)
            indexes.append(i);

    if (tooSmall) {
        Utils::sort(indexes, [&result](int i, int j) {
            return result[i] < result[j];
        });
    } else {
        Utils::sort(indexes, [&result](int i, int j) {
            return result[i] > result[j];
        });
    }

    int i = 0;
    int first = result[indexes.first()]; // biggest or smallest

    // we resize the biggest columns until they are the same size as the second biggest
    // since it looks prettiest if all the columns are the same width
    while (true) {
        for (; i < indexes.size(); ++i) {
            if (result[indexes[i]] != first)
                break;
        }
        int next = tooSmall ? INT_MAX : 0;
        if (i < indexes.size())
            next = result[indexes[i]];

        int delta;
        if (tooSmall)
            delta = qMin(next - first, widthToDistribute / qMax(i, 1));
        else
            delta = qMin(first - next, widthToDistribute / qMax(i, 1));

        if (delta == 0)
            return result;

        if (tooSmall) {
            for (int j = 0; j < i; ++j)
                result[indexes[j]] += delta;
        } else {
            for (int j = 0; j < i; ++j)
                result[indexes[j]] -= delta;
        }

        widthToDistribute -= delta * i;
        if (widthToDistribute <= 0)
            return result;

        first = result[indexes.first()];
        i = 0; // TODO can we do better?
    }
}

void MiniProjectTargetSelector::doLayout(bool keepSize)
{
    // An unconfigured project shows empty build/deploy/run sections
    // if there's a configured project in the seesion
    // that could be improved
    static QStatusBar *statusBar = Core::ICore::statusBar();
    static auto *actionBar = Core::ICore::mainWindow()->findChild<QWidget*>(QLatin1String("actionbar"));
    Q_ASSERT(actionBar);

    m_kitAreaWidget->move(0, 0);

    int oldSummaryLabelY = m_summaryLabel->y();

    int kitAreaHeight = m_kitAreaWidget->isVisibleTo(this) ? m_kitAreaWidget->sizeHint().height() : 0;

    // 1. Calculate the summary label height
    int summaryLabelY = 1 + kitAreaHeight;

    int summaryLabelHeight = 0;
    int oldSummaryLabelHeight = m_summaryLabel->height();
    bool onlySummary = false;
    // Count the number of lines
    int visibleLineCount = m_projectListWidget->isVisibleTo(this) ? 0 : 1;
    for (int i = TARGET; i < LAST; ++i)
        visibleLineCount += m_listWidgets[i]->isVisibleTo(this) ? 0 : 1;

    if (visibleLineCount == LAST) {
        summaryLabelHeight = m_summaryLabel->sizeHint().height();
        onlySummary = true;
    } else {
        if (visibleLineCount < 3) {
            if (Utils::anyOf(SessionManager::projects(), &Project::needsConfiguration))
                visibleLineCount = 3;
        }
        if (visibleLineCount)
            summaryLabelHeight = m_summaryLabel->sizeHint().height();
    }

    if (keepSize && oldSummaryLabelHeight > summaryLabelHeight)
        summaryLabelHeight = oldSummaryLabelHeight;

    m_summaryLabel->move(0, summaryLabelY);

    // Height to be aligned with side bar button
    int alignedWithActionHeight = 210;
    if (actionBar->isVisible())
        alignedWithActionHeight = actionBar->height() - statusBar->height();
    int bottomMargin = 9;
    int heightWithoutKitArea = 0;

    if (!onlySummary) {
        // list widget height
        int maxItemCount = m_projectListWidget->maxCount();
        for (int i = TARGET; i < LAST; ++i)
            maxItemCount = qMax(maxItemCount, m_listWidgets[i]->maxCount());

        int titleWidgetsHeight = m_titleWidgets.first()->height();
        if (keepSize) {
            heightWithoutKitArea = height() - oldSummaryLabelY + 1;
        } else {
            // Clamp the size of the listwidgets to be
            // at least as high as the sidebar button
            // and at most twice as high
            heightWithoutKitArea = summaryLabelHeight
                    + qBound(alignedWithActionHeight,
                             maxItemCount * 30 + bottomMargin + titleWidgetsHeight,
                             alignedWithActionHeight * 2);
        }

        int titleY = summaryLabelY + summaryLabelHeight;
        int listY = titleY + titleWidgetsHeight;
        int listHeight = heightWithoutKitArea + kitAreaHeight - bottomMargin - listY + 1;

        // list widget widths
        int minWidth = qMax(m_summaryLabel->sizeHint().width(), 250);
        minWidth = qMax(minWidth, m_kitAreaWidget->sizeHint().width());
        if (keepSize) {
            // Do not make the widget smaller then it was before
            int oldTotalListWidgetWidth = m_projectListWidget->isVisibleTo(this) ?
                    m_projectListWidget->width() : 0;
            for (int i = TARGET; i < LAST; ++i)
                oldTotalListWidgetWidth += m_listWidgets[i]->width();
            minWidth = qMax(minWidth, oldTotalListWidgetWidth);
        }

        QVector<int> widths = listWidgetWidths(minWidth, 1000);

        const int runColumnWidth = widths[RUN] == -1 ? 0 : RunColumnWidth;
        int x = 0;
        for (int i = PROJECT; i < LAST; ++i) {
            int optimalWidth = widths[i];
            if (i == PROJECT) {
                m_projectListWidget->resize(optimalWidth, listHeight);
                m_projectListWidget->move(x, listY);
            } else {
                if (i == RUN)
                    optimalWidth += runColumnWidth;
                m_listWidgets[i]->resize(optimalWidth, listHeight);
                m_listWidgets[i]->move(x, listY);
            }
            m_titleWidgets[i]->resize(optimalWidth, titleWidgetsHeight);
            m_titleWidgets[i]->move(x, titleY);
            x += optimalWidth + 1; //1 extra pixel for the separators or the right border
        }

        m_listWidgets[RUN]->setColumnWidth(0, m_listWidgets[RUN]->size().width() - runColumnWidth
                                           - m_listWidgets[RUN]->padding());
        m_listWidgets[RUN]->setColumnWidth(1, runColumnWidth);
        m_summaryLabel->resize(x - 1, summaryLabelHeight);
        m_kitAreaWidget->resize(x - 1, kitAreaHeight);
        setFixedSize(x, heightWithoutKitArea + kitAreaHeight);
    } else {
        if (keepSize)
            heightWithoutKitArea = height() - oldSummaryLabelY + 1;
        else
            heightWithoutKitArea = qMax(summaryLabelHeight + bottomMargin, alignedWithActionHeight);
        m_summaryLabel->resize(m_summaryLabel->sizeHint().width(), heightWithoutKitArea - bottomMargin);
        m_kitAreaWidget->resize(m_kitAreaWidget->sizeHint());
        setFixedSize(m_summaryLabel->width() + 1, heightWithoutKitArea + kitAreaHeight); //1 extra pixel for the border
    }

    QPoint moveTo = statusBar->mapToGlobal(QPoint(0, 0));
    moveTo -= QPoint(0, height());
    move(moveTo);
}

void MiniProjectTargetSelector::projectAdded(Project *project)
{
    connect(project, &Project::addedTarget,
            this, &MiniProjectTargetSelector::handleNewTarget);
    connect(project, &Project::removedTarget,
            this, &MiniProjectTargetSelector::handleRemovalOfTarget);

    foreach (Target *t, project->targets())
        addedTarget(t);

    updateProjectListVisible();
    updateTargetListVisible();
    updateBuildListVisible();
    updateDeployListVisible();
    updateRunListVisible();
}

void MiniProjectTargetSelector::projectRemoved(Project *project)
{
    disconnect(project, &Project::addedTarget,
               this, &MiniProjectTargetSelector::handleNewTarget);
    disconnect(project, &Project::removedTarget,
               this, &MiniProjectTargetSelector::handleRemovalOfTarget);

    foreach (Target *t, project->targets())
        removedTarget(t);

    updateProjectListVisible();
    updateTargetListVisible();
    updateBuildListVisible();
    updateDeployListVisible();
    updateRunListVisible();
}

void MiniProjectTargetSelector::handleNewTarget(Target *target)
{
    addedTarget(target);
    updateTargetListVisible();
    updateBuildListVisible();
    updateDeployListVisible();
    updateRunListVisible();
}

void MiniProjectTargetSelector::handleRemovalOfTarget(Target *target)
{
    removedTarget(target);

    updateTargetListVisible();
    updateBuildListVisible();
    updateDeployListVisible();
    updateRunListVisible();
}

void MiniProjectTargetSelector::addedTarget(Target *target)
{
    if (target->project() != m_project)
        return;

    m_listWidgets[TARGET]->addProjectConfiguration(target);

    for (BuildConfiguration *bc : target->buildConfigurations())
        addedBuildConfiguration(bc, false);
    for (DeployConfiguration *dc : target->deployConfigurations())
        addedDeployConfiguration(dc, false);
    for (RunConfiguration *rc : target->runConfigurations())
        addedRunConfiguration(rc, false);
}

void MiniProjectTargetSelector::removedTarget(Target *target)
{
    if (target->project() != m_project)
        return;

    m_listWidgets[TARGET]->removeProjectConfiguration(target);

    for (BuildConfiguration *bc : target->buildConfigurations())
        removedBuildConfiguration(bc, false);
    for (DeployConfiguration *dc : target->deployConfigurations())
        removedDeployConfiguration(dc, false);
    for (RunConfiguration *rc : target->runConfigurations())
        removedRunConfiguration(rc, false);
}

void MiniProjectTargetSelector::addedBuildConfiguration(BuildConfiguration *bc, bool update)
{
    if (!m_project || bc->target() != m_project->activeTarget())
        return;

    m_listWidgets[BUILD]->addProjectConfiguration(bc);
    if (update)
        updateBuildListVisible();
}

void MiniProjectTargetSelector::removedBuildConfiguration(BuildConfiguration *bc, bool update)
{
    if (!m_project || bc->target() != m_project->activeTarget())
        return;

    m_listWidgets[BUILD]->removeProjectConfiguration(bc);
    if (update)
        updateBuildListVisible();
}

void MiniProjectTargetSelector::addedDeployConfiguration(DeployConfiguration *dc, bool update)
{
    if (!m_project || dc->target() != m_project->activeTarget())
        return;

    m_listWidgets[DEPLOY]->addProjectConfiguration(dc);
    if (update)
        updateDeployListVisible();
}

void MiniProjectTargetSelector::removedDeployConfiguration(DeployConfiguration *dc, bool update)
{
    if (!m_project || dc->target() != m_project->activeTarget())
        return;

    m_listWidgets[DEPLOY]->removeProjectConfiguration(dc);
    if (update)
        updateDeployListVisible();
}

void MiniProjectTargetSelector::addedRunConfiguration(RunConfiguration *rc, bool update)
{
    if (!m_project || rc->target() != m_project->activeTarget())
        return;

    m_listWidgets[RUN]->addProjectConfiguration(rc);
    if (update)
        updateRunListVisible();
}

void MiniProjectTargetSelector::removedRunConfiguration(RunConfiguration *rc, bool update)
{
    if (!m_project || rc->target() != m_project->activeTarget())
        return;

    m_listWidgets[RUN]->removeProjectConfiguration(rc);
    if (update)
        updateRunListVisible();
}

void MiniProjectTargetSelector::updateProjectListVisible()
{
    int count = SessionManager::projects().size();
    bool visible = count > 1;

    m_projectListWidget->setVisible(visible);
    m_projectListWidget->setMaxCount(count);
    m_titleWidgets[PROJECT]->setVisible(visible);

    updateSummary();
}

void MiniProjectTargetSelector::updateTargetListVisible()
{
    int maxCount = 0;
    for (Project *p : SessionManager::projects())
        maxCount = qMax(p->targets().size(), maxCount);

    bool visible = maxCount > 1;
    m_listWidgets[TARGET]->setVisible(visible);
    m_listWidgets[TARGET]->setMaxCount(maxCount);
    m_titleWidgets[TARGET]->setVisible(visible);
    updateSummary();
}

void MiniProjectTargetSelector::updateBuildListVisible()
{
    int maxCount = 0;
    for (Project *p : SessionManager::projects())
        foreach (Target *t, p->targets())
            maxCount = qMax(t->buildConfigurations().size(), maxCount);

    bool visible = maxCount > 1;
    m_listWidgets[BUILD]->setVisible(visible);
    m_listWidgets[BUILD]->setMaxCount(maxCount);
    m_titleWidgets[BUILD]->setVisible(visible);
    updateSummary();
}

void MiniProjectTargetSelector::updateDeployListVisible()
{
    int maxCount = 0;
    for (Project *p : SessionManager::projects())
        foreach (Target *t, p->targets())
            maxCount = qMax(t->deployConfigurations().size(), maxCount);

    bool visible = maxCount > 1;
    m_listWidgets[DEPLOY]->setVisible(visible);
    m_listWidgets[DEPLOY]->setMaxCount(maxCount);
    m_titleWidgets[DEPLOY]->setVisible(visible);
    updateSummary();
}

void MiniProjectTargetSelector::updateRunListVisible()
{
    int maxCount = 0;
    for (Project *p : SessionManager::projects())
        foreach (Target *t, p->targets())
            maxCount = qMax(t->runConfigurations().size(), maxCount);

    bool visible = maxCount > 1;
    m_listWidgets[RUN]->setVisible(visible);
    m_listWidgets[RUN]->setMaxCount(maxCount);
    m_titleWidgets[RUN]->setVisible(visible);
    updateSummary();
}

void MiniProjectTargetSelector::changeStartupProject(Project *project)
{
    if (m_project) {
        disconnect(m_project, &Project::activeTargetChanged,
                   this, &MiniProjectTargetSelector::activeTargetChanged);
    }
    m_project = project;
    if (m_project) {
        connect(m_project, &Project::activeTargetChanged,
                this, &MiniProjectTargetSelector::activeTargetChanged);
        activeTargetChanged(m_project->activeTarget());
    } else {
        activeTargetChanged(nullptr);
    }

    if (project) {
        QList<QObject *> list;
        foreach (Target *t, project->targets())
            list.append(t);
        m_listWidgets[TARGET]->setProjectConfigurations(list, project->activeTarget());
    } else {
        m_listWidgets[TARGET]->setProjectConfigurations(QList<QObject *>(), nullptr);
    }

    updateActionAndSummary();
}

void MiniProjectTargetSelector::activeTargetChanged(Target *target)
{
    if (m_target) {
        disconnect(m_target, &Target::kitChanged,
                   this, &MiniProjectTargetSelector::updateActionAndSummary);
        disconnect(m_target, &Target::iconChanged,
                   this, &MiniProjectTargetSelector::updateActionAndSummary);
        disconnect(m_target, &Target::activeBuildConfigurationChanged,
                   this, &MiniProjectTargetSelector::activeBuildConfigurationChanged);
        disconnect(m_target, &Target::activeDeployConfigurationChanged,
                   this, &MiniProjectTargetSelector::activeDeployConfigurationChanged);
        disconnect(m_target, &Target::activeRunConfigurationChanged,
                   this, &MiniProjectTargetSelector::activeRunConfigurationChanged);
    }

    m_target = target;

    m_kitAreaWidget->setKit(m_target ? m_target->kit() : nullptr);

    m_listWidgets[TARGET]->setActiveProjectConfiguration(m_target);

    if (m_buildConfiguration)
        disconnect(m_buildConfiguration, &ProjectConfiguration::displayNameChanged,
                   this, &MiniProjectTargetSelector::updateActionAndSummary);
    if (m_deployConfiguration)
        disconnect(m_deployConfiguration, &ProjectConfiguration::displayNameChanged,
                   this, &MiniProjectTargetSelector::updateActionAndSummary);

    if (m_runConfiguration)
        disconnect(m_runConfiguration, &ProjectConfiguration::displayNameChanged,
                   this, &MiniProjectTargetSelector::updateActionAndSummary);

    if (m_target) {
        QList<QObject *> bl;
        for (BuildConfiguration *bc : target->buildConfigurations())
            bl.append(bc);
        m_listWidgets[BUILD]->setProjectConfigurations(bl, target->activeBuildConfiguration());

        QList<QObject *> dl;
        for (DeployConfiguration *dc : target->deployConfigurations())
            dl.append(dc);
        m_listWidgets[DEPLOY]->setProjectConfigurations(dl, target->activeDeployConfiguration());

        QList<QObject *> rl;
        for (RunConfiguration *rc : target->runConfigurations())
            rl.append(rc);
        m_listWidgets[RUN]->setProjectConfigurations(rl, target->activeRunConfiguration());

        m_buildConfiguration = m_target->activeBuildConfiguration();
        if (m_buildConfiguration)
            connect(m_buildConfiguration, &ProjectConfiguration::displayNameChanged,
                    this, &MiniProjectTargetSelector::updateActionAndSummary);
        m_deployConfiguration = m_target->activeDeployConfiguration();
        if (m_deployConfiguration)
            connect(m_deployConfiguration, &ProjectConfiguration::displayNameChanged,
                    this, &MiniProjectTargetSelector::updateActionAndSummary);
        m_runConfiguration = m_target->activeRunConfiguration();
        if (m_runConfiguration)
            connect(m_runConfiguration, &ProjectConfiguration::displayNameChanged,
                    this, &MiniProjectTargetSelector::updateActionAndSummary);

        connect(m_target, &Target::kitChanged,
                this, &MiniProjectTargetSelector::updateActionAndSummary);
        connect(m_target, &Target::iconChanged,
                this, &MiniProjectTargetSelector::updateActionAndSummary);
        connect(m_target, &Target::activeBuildConfigurationChanged,
                this, &MiniProjectTargetSelector::activeBuildConfigurationChanged);
        connect(m_target, &Target::activeDeployConfigurationChanged,
                this, &MiniProjectTargetSelector::activeDeployConfigurationChanged);
        connect(m_target, &Target::activeRunConfigurationChanged,
                this, &MiniProjectTargetSelector::activeRunConfigurationChanged);
    } else {
        m_listWidgets[BUILD]->setProjectConfigurations(QList<QObject *>(), nullptr);
        m_listWidgets[DEPLOY]->setProjectConfigurations(QList<QObject *>(), nullptr);
        m_listWidgets[RUN]->setProjectConfigurations(QList<QObject *>(), nullptr);
        m_buildConfiguration = nullptr;
        m_deployConfiguration = nullptr;
        m_runConfiguration = nullptr;
    }
    updateActionAndSummary();
}

void MiniProjectTargetSelector::kitChanged(Kit *k)
{
    if (m_target && m_target->kit() == k)
        updateActionAndSummary();
}

void MiniProjectTargetSelector::activeBuildConfigurationChanged(BuildConfiguration *bc)
{
    if (m_buildConfiguration)
        disconnect(m_buildConfiguration, &ProjectConfiguration::displayNameChanged,
                   this, &MiniProjectTargetSelector::updateActionAndSummary);
    m_buildConfiguration = bc;
    if (m_buildConfiguration)
        connect(m_buildConfiguration, &ProjectConfiguration::displayNameChanged,
                this, &MiniProjectTargetSelector::updateActionAndSummary);
    m_listWidgets[BUILD]->setActiveProjectConfiguration(bc);
    updateActionAndSummary();
}

void MiniProjectTargetSelector::activeDeployConfigurationChanged(DeployConfiguration *dc)
{
    if (m_deployConfiguration)
        disconnect(m_deployConfiguration, &ProjectConfiguration::displayNameChanged,
                   this, &MiniProjectTargetSelector::updateActionAndSummary);
    m_deployConfiguration = dc;
    if (m_deployConfiguration)
        connect(m_deployConfiguration, &ProjectConfiguration::displayNameChanged,
                this, &MiniProjectTargetSelector::updateActionAndSummary);
    m_listWidgets[DEPLOY]->setActiveProjectConfiguration(dc);
    updateActionAndSummary();
}

void MiniProjectTargetSelector::activeRunConfigurationChanged(RunConfiguration *rc)
{
    if (m_runConfiguration)
        disconnect(m_runConfiguration, &ProjectConfiguration::displayNameChanged,
                   this, &MiniProjectTargetSelector::updateActionAndSummary);
    m_runConfiguration = rc;
    if (m_runConfiguration)
        connect(m_runConfiguration, &ProjectConfiguration::displayNameChanged,
                this, &MiniProjectTargetSelector::updateActionAndSummary);
    m_listWidgets[RUN]->setActiveProjectConfiguration(rc);
    updateActionAndSummary();
}

void MiniProjectTargetSelector::setVisible(bool visible)
{
    doLayout(false);
    QWidget::setVisible(visible);
    m_projectAction->setChecked(visible);
    if (visible) {
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
        if (auto *lw = qobject_cast<SelectorView *>(focusWidget())) {
            if (lw->currentIndex().row() < lw->model()->rowCount() -1)
                lw->setCurrentIndex(lw->model()->index(lw->currentIndex().row() + 1, 0));
            else
                lw->setCurrentIndex(lw->model()->index(0, 0));
        }
    }
}

void MiniProjectTargetSelector::keyPressEvent(QKeyEvent *ke)
{
    if (ke->key() == Qt::Key_Return
            || ke->key() == Qt::Key_Enter
            || ke->key() == Qt::Key_Space
            || ke->key() == Qt::Key_Escape) {
        hide();
    } else {
        QWidget::keyPressEvent(ke);
    }
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
            || ke->key() == Qt::Key_Space
            || ke->key() == Qt::Key_Escape)
        return;
    QWidget::keyReleaseEvent(ke);
}

void MiniProjectTargetSelector::delayedHide()
{
    QDateTime current = QDateTime::currentDateTime();
    if (m_earliestHidetime > current) {
        // schedule for later
        QTimer::singleShot(current.msecsTo(m_earliestHidetime) + 50, this, &MiniProjectTargetSelector::delayedHide);
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
    QString projectName = QLatin1String(" ");
    QString fileName; // contains the path if projectName is not unique
    QString targetName;
    QString targetToolTipText;
    QString buildConfig;
    QString deployConfig;
    QString runConfig;
    QIcon targetIcon = creatorTheme()->flag(Theme::FlatSideBarIcons)
            ? Icons::DESKTOP_DEVICE.icon()
            : style()->standardIcon(QStyle::SP_ComputerIcon);

    Project *project = SessionManager::startupProject();
    if (project) {
        projectName = project->displayName();
        for (Project *p : SessionManager::projects()) {
            if (p != project && p->displayName() == projectName) {
                fileName = project->projectFilePath().toUserOutput();
                break;
            }
        }

        if (Target *target = project->activeTarget()) {
            targetName = project->activeTarget()->displayName();

            if (BuildConfiguration *bc = target->activeBuildConfiguration())
                buildConfig = bc->displayName();

            if (DeployConfiguration *dc = target->activeDeployConfiguration())
                deployConfig = dc->displayName();

            if (RunConfiguration *rc = target->activeRunConfiguration())
                runConfig = rc->displayName();

            targetToolTipText = target->overlayIconToolTip();
            targetIcon = createCenteredIcon(target->icon(), target->overlayIcon());
        }
    }
    m_projectAction->setProperty("heading", projectName);
    if (project && project->needsConfiguration())
        m_projectAction->setProperty("subtitle", tr("Unconfigured"));
    else
        m_projectAction->setProperty("subtitle", buildConfig);
    m_projectAction->setIcon(targetIcon);
    QStringList lines;
    lines << tr("<b>Project:</b> %1").arg(projectName);
    if (!fileName.isEmpty())
        lines << tr("<b>Path:</b> %1").arg(fileName);
    if (!targetName.isEmpty())
        lines << tr("<b>Kit:</b> %1").arg(targetName);
    if (!buildConfig.isEmpty())
        lines << tr("<b>Build:</b> %1").arg(buildConfig);
    if (!deployConfig.isEmpty())
        lines << tr("<b>Deploy:</b> %1").arg(deployConfig);
    if (!runConfig.isEmpty())
        lines << tr("<b>Run:</b> %1").arg(runConfig);
    if (!targetToolTipText.isEmpty())
        lines << tr("%1").arg(targetToolTipText);
    QString toolTip = QString("<html><nobr>%1</html>")
            .arg(lines.join(QLatin1String("<br/>")));
    m_projectAction->setToolTip(toolTip);
    updateSummary();
}

void MiniProjectTargetSelector::updateSummary()
{
    QString summary;
    if (Project *startupProject = SessionManager::startupProject()) {
        if (!m_projectListWidget->isVisibleTo(this))
            summary.append(tr("Project: <b>%1</b><br/>").arg(startupProject->displayName()));
        if (Target *activeTarget = startupProject->activeTarget()) {
            if (!m_listWidgets[TARGET]->isVisibleTo(this))
                summary.append(tr("Kit: <b>%1</b><br/>").arg( activeTarget->displayName()));
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
                summary.append(QLatin1String("<br/>"));
            if (!m_listWidgets[BUILD]->isVisibleTo(this))
                summary.append(QLatin1String("<br/>"));
            if (!m_listWidgets[DEPLOY]->isVisibleTo(this))
                summary.append(QLatin1String("<br/>"));
            if (!m_listWidgets[RUN]->isVisibleTo(this))
                summary.append(QLatin1String("<br/>"));
        }
    }
    m_summaryLabel->setText(summary);
}

void MiniProjectTargetSelector::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.fillRect(rect(), Utils::StyleHelper().baseColor());
    painter.setPen(creatorTheme()->color(Theme::MiniProjectTargetSelectorBorderColor));
    // draw border on top and right
    QRectF borderRect = QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5);
    painter.drawLine(borderRect.topLeft(), borderRect.topRight());
    painter.drawLine(borderRect.topRight(), borderRect.bottomRight());
    if (creatorTheme()->flag(Theme::DrawTargetSelectorBottom)) {
        // draw thicker border on the bottom
        QRect bottomRect(0, rect().height() - 8, rect().width(), 8);
        static const QImage image(":/projectexplorer/images/targetpanel_bottom.png");
        StyleHelper::drawCornerImage(image, &painter, bottomRect, 1, 1, 1, 1);
    }
}

void MiniProjectTargetSelector::switchToProjectsMode()
{
    Core::ModeManager::activateMode(Constants::MODE_SESSION);
    hide();
}

} // namespace Internal
} // namespace ProjectExplorer

#include <miniprojecttargetselector.moc>
