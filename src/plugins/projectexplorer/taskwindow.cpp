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

#include "taskwindow.h"

#include "itaskhandler.h"
#include "projectexplorericons.h"
#include "session.h"
#include "task.h"
#include "taskhub.h"
#include "taskmodel.h"

#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/icore.h>
#include <coreplugin/icontext.h>
#include <extensionsystem/pluginmanager.h>
#include <utils/algorithm.h>
#include <utils/qtcassert.h>
#include <utils/itemviews.h>
#include <utils/utilsicons.h>

#include <QDir>
#include <QPainter>
#include <QStyledItemDelegate>
#include <QMenu>
#include <QToolButton>
#include <QScrollBar>

namespace {
const int ELLIPSIS_GRADIENT_WIDTH = 16;
const char SESSION_FILTER_CATEGORIES[] = "TaskWindow.Categories";
const char SESSION_FILTER_WARNINGS[] = "TaskWindow.IncludeWarnings";
}

namespace ProjectExplorer {
namespace Internal {

class TaskView : public Utils::ListView
{
public:
    TaskView(QWidget *parent = 0);
    ~TaskView();
    void resizeEvent(QResizeEvent *e);
};

class TaskWindowContext : public Core::IContext
{
public:
    TaskWindowContext(QWidget *widget);
};

class TaskDelegate : public QStyledItemDelegate
{
    Q_OBJECT

    friend class TaskView; // for using Positions::minimumSize()

public:
    TaskDelegate(QObject * parent = 0);
    ~TaskDelegate();
    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const;

    // TaskView uses this method if the size of the taskview changes
    void emitSizeHintChanged(const QModelIndex &index);

    void currentChanged(const QModelIndex &current, const QModelIndex &previous);

private:
    void generateGradientPixmap(int width, int height, QColor color, bool selected) const;

    mutable int m_cachedHeight;
    mutable QFont m_cachedFont;

    /*
      Collapsed:
      +----------------------------------------------------------------------------------------------------+
      | TASKICONAREA  TEXTAREA                                                           FILEAREA LINEAREA |
      +----------------------------------------------------------------------------------------------------+

      Expanded:
      +----------------------------------------------------------------------------------------------------+
      | TASKICONICON  TEXTAREA                                                           FILEAREA LINEAREA |
      |               more text -------------------------------------------------------------------------> |
      +----------------------------------------------------------------------------------------------------+
     */
    class Positions
    {
    public:
        Positions(const QStyleOptionViewItem &options, TaskModel *model) :
            m_totalWidth(options.rect.width()),
            m_maxFileLength(model->sizeOfFile(options.font)),
            m_maxLineLength(model->sizeOfLineNumber(options.font)),
            m_realFileLength(m_maxFileLength),
            m_top(options.rect.top()),
            m_bottom(options.rect.bottom())
        {
            int flexibleArea = lineAreaLeft() - textAreaLeft() - ITEM_SPACING;
            if (m_maxFileLength > flexibleArea / 2)
                m_realFileLength = flexibleArea / 2;
            m_fontHeight = QFontMetrics(options.font).height();
        }

        int top() const { return m_top + ITEM_MARGIN; }
        int left() const { return ITEM_MARGIN; }
        int right() const { return m_totalWidth - ITEM_MARGIN; }
        int bottom() const { return m_bottom; }
        int firstLineHeight() const { return m_fontHeight + 1; }
        static int minimumHeight() { return taskIconHeight() + 2 * ITEM_MARGIN; }

        int taskIconLeft() const { return left(); }
        static int taskIconWidth() { return TASK_ICON_SIZE; }
        static int taskIconHeight() { return TASK_ICON_SIZE; }
        int taskIconRight() const { return taskIconLeft() + taskIconWidth(); }
        QRect taskIcon() const { return QRect(taskIconLeft(), top(), taskIconWidth(), taskIconHeight()); }

        int textAreaLeft() const { return taskIconRight() + ITEM_SPACING; }
        int textAreaWidth() const { return textAreaRight() - textAreaLeft(); }
        int textAreaRight() const { return fileAreaLeft() - ITEM_SPACING; }
        QRect textArea() const { return QRect(textAreaLeft(), top(), textAreaWidth(), firstLineHeight()); }

        int fileAreaLeft() const { return fileAreaRight() - fileAreaWidth(); }
        int fileAreaWidth() const { return m_realFileLength; }
        int fileAreaRight() const { return lineAreaLeft() - ITEM_SPACING; }
        QRect fileArea() const { return QRect(fileAreaLeft(), top(), fileAreaWidth(), firstLineHeight()); }

        int lineAreaLeft() const { return lineAreaRight() - lineAreaWidth(); }
        int lineAreaWidth() const { return m_maxLineLength; }
        int lineAreaRight() const { return right(); }
        QRect lineArea() const { return QRect(lineAreaLeft(), top(), lineAreaWidth(), firstLineHeight()); }

    private:
        int m_totalWidth;
        int m_maxFileLength;
        int m_maxLineLength;
        int m_realFileLength;
        int m_top;
        int m_bottom;
        int m_fontHeight;

        static const int TASK_ICON_SIZE = 16;
        static const int ITEM_MARGIN = 2;
        static const int ITEM_SPACING = 2 * ITEM_MARGIN;
    };
};

TaskView::TaskView(QWidget *parent)
    : Utils::ListView(parent)
{
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);

    QFontMetrics fm(font());
    int vStepSize = fm.height() + 3;
    if (vStepSize < TaskDelegate::Positions::minimumHeight())
        vStepSize = TaskDelegate::Positions::minimumHeight();

    verticalScrollBar()->setSingleStep(vStepSize);
}

TaskView::~TaskView()
{ }

void TaskView::resizeEvent(QResizeEvent *e)
{
    Q_UNUSED(e)
    static_cast<TaskDelegate *>(itemDelegate())->emitSizeHintChanged(selectionModel()->currentIndex());
}

/////
// TaskWindow
/////

class TaskWindowPrivate
{
public:
    Internal::TaskModel *m_model;
    Internal::TaskFilterModel *m_filter;
    Internal::TaskView *m_listview;
    Internal::TaskWindowContext *m_taskWindowContext;
    QMenu *m_contextMenu;
    ITaskHandler *m_defaultHandler = nullptr;
    QToolButton *m_filterWarningsButton;
    QToolButton *m_categoriesButton;
    QMenu *m_categoriesMenu;
    QList<QAction *> m_actions;
};

static QToolButton *createFilterButton(const QIcon &icon, const QString &toolTip,
                                       QObject *receiver, std::function<void(bool)> lambda)
{
    auto button = new QToolButton;
    button->setIcon(icon);
    button->setToolTip(toolTip);
    button->setCheckable(true);
    button->setChecked(true);
    button->setAutoRaise(true);
    button->setEnabled(true);
    QObject::connect(button, &QToolButton::toggled, receiver, lambda);
    return button;
}

TaskWindow::TaskWindow() : d(new TaskWindowPrivate)
{
    d->m_model = new Internal::TaskModel(this);
    d->m_filter = new Internal::TaskFilterModel(d->m_model);
    d->m_listview = new Internal::TaskView;

    d->m_listview->setModel(d->m_filter);
    d->m_listview->setFrameStyle(QFrame::NoFrame);
    d->m_listview->setWindowTitle(displayName());
    d->m_listview->setSelectionMode(QAbstractItemView::SingleSelection);
    Internal::TaskDelegate *tld = new Internal::TaskDelegate(this);
    d->m_listview->setItemDelegate(tld);
    d->m_listview->setWindowIcon(Icons::WINDOW.icon());
    d->m_listview->setContextMenuPolicy(Qt::ActionsContextMenu);
    d->m_listview->setAttribute(Qt::WA_MacShowFocusRect, false);

    d->m_taskWindowContext = new Internal::TaskWindowContext(d->m_listview);

    Core::ICore::addContextObject(d->m_taskWindowContext);

    connect(d->m_listview->selectionModel(), &QItemSelectionModel::currentChanged,
            tld, &TaskDelegate::currentChanged);

    connect(d->m_listview->selectionModel(), &QItemSelectionModel::currentChanged,
            this, &TaskWindow::currentChanged);
    connect(d->m_listview, &QAbstractItemView::activated,
            this, &TaskWindow::triggerDefaultHandler);

    d->m_contextMenu = new QMenu(d->m_listview);

    d->m_listview->setContextMenuPolicy(Qt::ActionsContextMenu);

    d->m_filterWarningsButton = createFilterButton(
                Utils::Icons::WARNING_TOOLBAR.icon(),
                tr("Show Warnings"), this, [this](bool show) { setShowWarnings(show); });

    d->m_categoriesButton = new QToolButton;
    d->m_categoriesButton->setIcon(Utils::Icons::FILTER.icon());
    d->m_categoriesButton->setToolTip(tr("Filter by categories"));
    d->m_categoriesButton->setProperty("noArrow", true);
    d->m_categoriesButton->setAutoRaise(true);
    d->m_categoriesButton->setPopupMode(QToolButton::InstantPopup);

    d->m_categoriesMenu = new QMenu(d->m_categoriesButton);
    connect(d->m_categoriesMenu, &QMenu::aboutToShow, this, &TaskWindow::updateCategoriesMenu);

    d->m_categoriesButton->setMenu(d->m_categoriesMenu);

    TaskHub *hub = TaskHub::instance();
    connect(hub, &TaskHub::categoryAdded, this, &TaskWindow::addCategory);
    connect(hub, &TaskHub::taskAdded, this, &TaskWindow::addTask);
    connect(hub, &TaskHub::taskRemoved, this, &TaskWindow::removeTask);
    connect(hub, &TaskHub::taskLineNumberUpdated, this, &TaskWindow::updatedTaskLineNumber);
    connect(hub, &TaskHub::taskFileNameUpdated, this, &TaskWindow::updatedTaskFileName);
    connect(hub, &TaskHub::tasksCleared, this, &TaskWindow::clearTasks);
    connect(hub, &TaskHub::categoryVisibilityChanged, this, &TaskWindow::setCategoryVisibility);
    connect(hub, &TaskHub::popupRequested, this, &TaskWindow::popup);
    connect(hub, &TaskHub::showTask, this, &TaskWindow::showTask);
    connect(hub, &TaskHub::openTask, this, &TaskWindow::openTask);

    connect(d->m_filter, &TaskFilterModel::rowsRemoved,
            [this]() { emit setBadgeNumber(d->m_filter->rowCount()); });
    connect(d->m_filter, &TaskFilterModel::rowsInserted,
            [this]() { emit setBadgeNumber(d->m_filter->rowCount()); });
    connect(d->m_filter, &TaskFilterModel::modelReset,
            [this]() { emit setBadgeNumber(d->m_filter->rowCount()); });

    SessionManager *session = SessionManager::instance();
    connect(session, &SessionManager::aboutToSaveSession, this, &TaskWindow::saveSettings);
    connect(session, &SessionManager::sessionLoaded, this, &TaskWindow::loadSettings);
}

TaskWindow::~TaskWindow()
{
    Core::ICore::removeContextObject(d->m_taskWindowContext);
    delete d->m_filterWarningsButton;
    delete d->m_listview;
    delete d->m_filter;
    delete d->m_model;
    delete d;
}

static ITaskHandler *handler(QAction *action)
{
    QVariant prop = action->property("ITaskHandler");
    ITaskHandler *handler = qobject_cast<ITaskHandler *>(prop.value<QObject *>());
    QTC_CHECK(handler);
    return handler;
}

void TaskWindow::delayedInitialization()
{
    static bool alreadyDone = false;
    if (alreadyDone)
        return;

    alreadyDone = true;

    QList<ITaskHandler *> handlers = ExtensionSystem::PluginManager::getObjects<ITaskHandler>();
    foreach (ITaskHandler *h, handlers) {
        if (h->isDefaultHandler() && !d->m_defaultHandler)
            d->m_defaultHandler = h;

        QAction *action = h->createAction(this);
        QTC_ASSERT(action, continue);
        action->setProperty("ITaskHandler", qVariantFromValue(qobject_cast<QObject*>(h)));
        connect(action, &QAction::triggered, this, &TaskWindow::actionTriggered);
        d->m_actions << action;

        Core::Id id = h->actionManagerId();
        if (id.isValid()) {
            Core::Command *cmd = Core::ActionManager::instance()
                    ->registerAction(action, id, d->m_taskWindowContext->context(), true);
            action = cmd->action();
        }
        d->m_listview->addAction(action);
    }

    // Disable everything for now:
    currentChanged(QModelIndex());
}

QList<QWidget*> TaskWindow::toolBarWidgets() const
{
    return {d->m_filterWarningsButton, d->m_categoriesButton};
}

QWidget *TaskWindow::outputWidget(QWidget *)
{
    return d->m_listview;
}

void TaskWindow::clearTasks(Core::Id categoryId)
{
    d->m_model->clearTasks(categoryId);

    emit tasksChanged();
    emit tasksCleared();
    navigateStateChanged();
}

void TaskWindow::setCategoryVisibility(Core::Id categoryId, bool visible)
{
    if (!categoryId.isValid())
        return;

    QList<Core::Id> categories = d->m_filter->filteredCategories();

    if (visible)
        categories.removeOne(categoryId);
    else
        categories.append(categoryId);

    d->m_filter->setFilteredCategories(categories);
}

void TaskWindow::currentChanged(const QModelIndex &index)
{
    const Task task = index.isValid() ? d->m_filter->task(index) : Task();
    foreach (QAction *action, d->m_actions) {
        ITaskHandler *h = handler(action);
        action->setEnabled((task.isNull() || !h) ? false : h->canHandle(task));
    }
}

void TaskWindow::saveSettings()
{
    QStringList categories = Utils::transform(d->m_filter->filteredCategories(), &Core::Id::toString);
    SessionManager::setValue(QLatin1String(SESSION_FILTER_CATEGORIES), categories);
    SessionManager::setValue(QLatin1String(SESSION_FILTER_WARNINGS), d->m_filter->filterIncludesWarnings());
}

void TaskWindow::loadSettings()
{
    QVariant value = SessionManager::value(QLatin1String(SESSION_FILTER_CATEGORIES));
    if (value.isValid()) {
        QList<Core::Id> categories
                = Utils::transform(value.toStringList(), &Core::Id::fromString);
        d->m_filter->setFilteredCategories(categories);
    }
    value = SessionManager::value(QLatin1String(SESSION_FILTER_WARNINGS));
    if (value.isValid()) {
        bool includeWarnings = value.toBool();
        d->m_filter->setFilterIncludesWarnings(includeWarnings);
        d->m_filter->setFilterIncludesUnknowns(includeWarnings);
        d->m_filterWarningsButton->setDown(d->m_filter->filterIncludesWarnings());
    }
}

void TaskWindow::visibilityChanged(bool visible)
{
    if (visible)
        delayedInitialization();
}

void TaskWindow::addCategory(Core::Id categoryId, const QString &displayName, bool visible)
{
    d->m_model->addCategory(categoryId, displayName);
    if (!visible) {
        QList<Core::Id> filters = d->m_filter->filteredCategories();
        filters += categoryId;
        d->m_filter->setFilteredCategories(filters);
    }
}

void TaskWindow::addTask(const Task &task)
{
    d->m_model->addTask(task);

    emit tasksChanged();
    navigateStateChanged();

    if (task.type == Task::Error && d->m_filter->filterIncludesErrors()
            && !d->m_filter->filteredCategories().contains(task.category))
        flash();
}

void TaskWindow::removeTask(const Task &task)
{
    d->m_model->removeTask(task);

    emit tasksChanged();
    navigateStateChanged();
}

void TaskWindow::updatedTaskFileName(unsigned int id, const QString &fileName)
{
    d->m_model->updateTaskFileName(id, fileName);
    emit tasksChanged();
}

void TaskWindow::updatedTaskLineNumber(unsigned int id, int line)
{
    d->m_model->updateTaskLineNumber(id, line);
    emit tasksChanged();
}

void TaskWindow::showTask(unsigned int id)
{
    int sourceRow = d->m_model->rowForId(id);
    QModelIndex sourceIdx = d->m_model->index(sourceRow, 0);
    QModelIndex filterIdx = d->m_filter->mapFromSource(sourceIdx);
    d->m_listview->setCurrentIndex(filterIdx);
    popup(Core::IOutputPane::ModeSwitch);
}

void TaskWindow::openTask(unsigned int id)
{
    int sourceRow = d->m_model->rowForId(id);
    QModelIndex sourceIdx = d->m_model->index(sourceRow, 0);
    QModelIndex filterIdx = d->m_filter->mapFromSource(sourceIdx);
    triggerDefaultHandler(filterIdx);
}

void TaskWindow::triggerDefaultHandler(const QModelIndex &index)
{
    if (!index.isValid() || !d->m_defaultHandler)
        return;

    Task task(d->m_filter->task(index));
    if (task.isNull())
        return;

    if (d->m_defaultHandler->canHandle(task)) {
        d->m_defaultHandler->handle(task);
    } else {
        if (!task.file.exists())
            d->m_model->setFileNotFound(index, true);
    }
}

void TaskWindow::actionTriggered()
{
    auto action = qobject_cast<QAction *>(sender());
    if (!action || !action->isEnabled())
        return;
    ITaskHandler *h = handler(action);
    if (!h)
        return;

    QModelIndex index = d->m_listview->selectionModel()->currentIndex();
    Task task = d->m_filter->task(index);
    if (task.isNull())
        return;

    h->handle(task);
}

void TaskWindow::setShowWarnings(bool show)
{
    d->m_filter->setFilterIncludesWarnings(show);
    d->m_filter->setFilterIncludesUnknowns(show); // "Unknowns" are often associated with warnings
}

void TaskWindow::updateCategoriesMenu()
{
    typedef QMap<QString, Core::Id>::ConstIterator NameToIdsConstIt;

    d->m_categoriesMenu->clear();

    const QList<Core::Id> filteredCategories = d->m_filter->filteredCategories();

    QMap<QString, Core::Id> nameToIds;
    foreach (Core::Id categoryId, d->m_model->categoryIds())
        nameToIds.insert(d->m_model->categoryDisplayName(categoryId), categoryId);

    const NameToIdsConstIt cend = nameToIds.constEnd();
    for (NameToIdsConstIt it = nameToIds.constBegin(); it != cend; ++it) {
        const QString &displayName = it.key();
        const Core::Id categoryId = it.value();
        auto action = new QAction(d->m_categoriesMenu);
        action->setCheckable(true);
        action->setText(displayName);
        action->setChecked(!filteredCategories.contains(categoryId));
        connect(action, &QAction::triggered, this, [this, action, categoryId] {
            setCategoryVisibility(categoryId, action->isChecked());
        });
        d->m_categoriesMenu->addAction(action);
    }
}

int TaskWindow::taskCount(Core::Id category) const
{
    return d->m_model->taskCount(category);
}

int TaskWindow::errorTaskCount(Core::Id category) const
{
    return d->m_model->errorTaskCount(category);
}

int TaskWindow::warningTaskCount(Core::Id category) const
{
    return d->m_model->warningTaskCount(category);
}

int TaskWindow::priorityInStatusBar() const
{
    return 90;
}

void TaskWindow::clearContents()
{
    // clear all tasks in all displays
    // Yeah we are that special
    TaskHub::clearTasks();
}

bool TaskWindow::hasFocus() const
{
    return d->m_listview->window()->focusWidget() == d->m_listview;
}

bool TaskWindow::canFocus() const
{
    return d->m_filter->rowCount();
}

void TaskWindow::setFocus()
{
    if (d->m_filter->rowCount()) {
        d->m_listview->setFocus();
        if (d->m_listview->currentIndex() == QModelIndex())
            d->m_listview->setCurrentIndex(d->m_filter->index(0,0, QModelIndex()));
    }
}

bool TaskWindow::canNext() const
{
    return d->m_filter->rowCount();
}

bool TaskWindow::canPrevious() const
{
    return d->m_filter->rowCount();
}

void TaskWindow::goToNext()
{
    if (!canNext())
        return;
    QModelIndex startIndex = d->m_listview->currentIndex();
    QModelIndex currentIndex = startIndex;

    if (startIndex.isValid()) {
        do {
            int row = currentIndex.row() + 1;
            if (row == d->m_filter->rowCount())
                row = 0;
            currentIndex = d->m_filter->index(row, 0);
            if (d->m_filter->hasFile(currentIndex))
                break;
        } while (startIndex != currentIndex);
    } else {
        currentIndex = d->m_filter->index(0, 0);
    }
    d->m_listview->setCurrentIndex(currentIndex);
    triggerDefaultHandler(currentIndex);
}

void TaskWindow::goToPrev()
{
    if (!canPrevious())
        return;
    QModelIndex startIndex = d->m_listview->currentIndex();
    QModelIndex currentIndex = startIndex;

    if (startIndex.isValid()) {
        do {
            int row = currentIndex.row() - 1;
            if (row < 0)
                row = d->m_filter->rowCount() - 1;
            currentIndex = d->m_filter->index(row, 0);
            if (d->m_filter->hasFile(currentIndex))
                break;
        } while (startIndex != currentIndex);
    } else {
        currentIndex = d->m_filter->index(0, 0);
    }
    d->m_listview->setCurrentIndex(currentIndex);
    triggerDefaultHandler(currentIndex);
}

bool TaskWindow::canNavigate() const
{
    return true;
}

/////
// Delegate
/////

TaskDelegate::TaskDelegate(QObject *parent) :
    QStyledItemDelegate(parent),
    m_cachedHeight(0)
{ }

TaskDelegate::~TaskDelegate()
{
}

QSize TaskDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QStyleOptionViewItem opt = option;
    initStyleOption(&opt, index);

    auto view = qobject_cast<const QAbstractItemView *>(opt.widget);
    const bool selected = (view->selectionModel()->currentIndex() == index);
    QSize s;
    s.setWidth(option.rect.width());

    if (!selected && option.font == m_cachedFont && m_cachedHeight > 0) {
        s.setHeight(m_cachedHeight);
        return s;
    }

    QFontMetrics fm(option.font);
    int fontHeight = fm.height();
    int fontLeading = fm.leading();

    auto model = static_cast<TaskFilterModel *>(view->model())->taskModel();
    Positions positions(option, model);

    if (selected) {
        QString description = index.data(TaskModel::Description).toString();
        // Layout the description
        int leading = fontLeading;
        int height = 0;
        description.replace(QLatin1Char('\n'), QChar::LineSeparator);
        QTextLayout tl(description);
        tl.setFormats(index.data(TaskModel::Task_t).value<Task>().formats);
        tl.beginLayout();
        while (true) {
            QTextLine line = tl.createLine();
            if (!line.isValid())
                break;
            line.setLineWidth(positions.textAreaWidth());
            height += leading;
            line.setPosition(QPoint(0, height));
            height += static_cast<int>(line.height());
        }
        tl.endLayout();

        s.setHeight(height + leading + fontHeight + 3);
    } else {
        s.setHeight(fontHeight + 3);
    }
    if (s.height() < positions.minimumHeight())
        s.setHeight(positions.minimumHeight());

    if (!selected) {
        m_cachedHeight = s.height();
        m_cachedFont = option.font;
    }

    return s;
}

void TaskDelegate::emitSizeHintChanged(const QModelIndex &index)
{
    emit sizeHintChanged(index);
}

void TaskDelegate::currentChanged(const QModelIndex &current, const QModelIndex &previous)
{
    emit sizeHintChanged(current);
    emit sizeHintChanged(previous);
}

void TaskDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QStyleOptionViewItem opt = option;
    initStyleOption(&opt, index);
    painter->save();

    QFontMetrics fm(opt.font);
    QColor backgroundColor;
    QColor textColor;

    auto view = qobject_cast<const QAbstractItemView *>(opt.widget);
    bool selected = view->selectionModel()->currentIndex() == index;

    if (selected) {
        painter->setBrush(opt.palette.highlight().color());
        backgroundColor = opt.palette.highlight().color();
    } else {
        painter->setBrush(opt.palette.background().color());
        backgroundColor = opt.palette.background().color();
    }
    painter->setPen(Qt::NoPen);
    painter->drawRect(opt.rect);

    // Set Text Color
    if (selected)
        textColor = opt.palette.highlightedText().color();
    else
        textColor = opt.palette.text().color();

    painter->setPen(textColor);

    auto model = static_cast<TaskFilterModel *>(view->model())->taskModel();
    Positions positions(opt, model);

    // Paint TaskIconArea:
    QIcon icon = index.data(TaskModel::Icon).value<QIcon>();
    painter->drawPixmap(positions.left(), positions.top(),
                        icon.pixmap(positions.taskIconWidth(), positions.taskIconHeight()));

    // Paint TextArea:
    if (!selected) {
        // in small mode we lay out differently
        QString bottom = index.data(TaskModel::Description).toString().split(QLatin1Char('\n')).first();
        painter->setClipRect(positions.textArea());
        painter->drawText(positions.textAreaLeft(), positions.top() + fm.ascent(), bottom);
        if (fm.width(bottom) > positions.textAreaWidth()) {
            // draw a gradient to mask the text
            int gradientStart = positions.textAreaRight() - ELLIPSIS_GRADIENT_WIDTH + 1;
            QLinearGradient lg(gradientStart, 0, gradientStart + ELLIPSIS_GRADIENT_WIDTH, 0);
            lg.setColorAt(0, Qt::transparent);
            lg.setColorAt(1, backgroundColor);
            painter->fillRect(gradientStart, positions.top(), ELLIPSIS_GRADIENT_WIDTH, positions.firstLineHeight(), lg);
        }
    } else {
        // Description
        QString description = index.data(TaskModel::Description).toString();
        // Layout the description
        int leading = fm.leading();
        int height = 0;
        description.replace(QLatin1Char('\n'), QChar::LineSeparator);
        QTextLayout tl(description);
        tl.setFormats(index.data(TaskModel::Task_t).value<Task>().formats);
        tl.beginLayout();
        while (true) {
            QTextLine line = tl.createLine();
            if (!line.isValid())
                break;
            line.setLineWidth(positions.textAreaWidth());
            height += leading;
            line.setPosition(QPoint(0, height));
            height += static_cast<int>(line.height());
        }
        tl.endLayout();
        tl.draw(painter, QPoint(positions.textAreaLeft(), positions.top()));

        QColor mix;
        mix.setRgb( static_cast<int>(0.7 * textColor.red()   + 0.3 * backgroundColor.red()),
                static_cast<int>(0.7 * textColor.green() + 0.3 * backgroundColor.green()),
                static_cast<int>(0.7 * textColor.blue()  + 0.3 * backgroundColor.blue()));
        painter->setPen(mix);

        const QString directory = QDir::toNativeSeparators(index.data(TaskModel::File).toString());
        int secondBaseLine = positions.top() + fm.ascent() + height + leading;
        if (index.data(TaskModel::FileNotFound).toBool()
                && !directory.isEmpty()) {
            QString fileNotFound = tr("File not found: %1").arg(directory);
            painter->setPen(Qt::red);
            painter->drawText(positions.textAreaLeft(), secondBaseLine, fileNotFound);
        } else {
            painter->drawText(positions.textAreaLeft(), secondBaseLine, directory);
        }
    }
    painter->setPen(textColor);

    // Paint FileArea
    QString file = index.data(TaskModel::File).toString();
    const int pos = file.lastIndexOf(QLatin1Char('/'));
    if (pos != -1)
        file = file.mid(pos +1);
    const int realFileWidth = fm.width(file);
    painter->setClipRect(positions.fileArea());
    painter->drawText(qMin(positions.fileAreaLeft(), positions.fileAreaRight() - realFileWidth),
                      positions.top() + fm.ascent(), file);
    if (realFileWidth > positions.fileAreaWidth()) {
        // draw a gradient to mask the text
        int gradientStart = positions.fileAreaLeft() - 1;
        QLinearGradient lg(gradientStart + ELLIPSIS_GRADIENT_WIDTH, 0, gradientStart, 0);
        lg.setColorAt(0, Qt::transparent);
        lg.setColorAt(1, backgroundColor);
        painter->fillRect(gradientStart, positions.top(), ELLIPSIS_GRADIENT_WIDTH, positions.firstLineHeight(), lg);
    }

    // Paint LineArea
    int line = index.data(TaskModel::Line).toInt();
    int movedLine = index.data(TaskModel::MovedLine).toInt();
    QString lineText;

    if (line == -1) {
        // No line information at all
    } else if (movedLine == -1) {
        // removed the line, but we had line information, show the line in ()
        QFont f = painter->font();
        f.setItalic(true);
        painter->setFont(f);
        lineText = QLatin1Char('(') + QString::number(line) + QLatin1Char(')');
    }  else if (movedLine != line) {
        // The line was moved
        QFont f = painter->font();
        f.setItalic(true);
        painter->setFont(f);
        lineText = QString::number(movedLine);
    } else {
        lineText = QString::number(line);
    }

    painter->setClipRect(positions.lineArea());
    const int realLineWidth = fm.width(lineText);
    painter->drawText(positions.lineAreaRight() - realLineWidth, positions.top() + fm.ascent(), lineText);
    painter->setClipRect(opt.rect);

    // Separator lines
    painter->setPen(QColor::fromRgb(150,150,150));
    const QRectF borderRect = QRectF(opt.rect).adjusted(0.5, 0.5, -0.5, -0.5);
    painter->drawLine(borderRect.bottomLeft(), borderRect.bottomRight());
    painter->restore();
}

TaskWindowContext::TaskWindowContext(QWidget *widget)
  : Core::IContext(widget)
{
    setWidget(widget);
    setContext(Core::Context(Core::Constants::C_PROBLEM_PANE));
}

} // namespace Internal
} // namespace ProjectExplorer

#include "taskwindow.moc"
