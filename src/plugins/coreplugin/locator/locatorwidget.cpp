// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "locatorwidget.h"

#include "ilocatorfilter.h"
#include "locatorconstants.h"
#include "locatormanager.h"
#include "../actionmanager/actionmanager.h"
#include "../coreplugintr.h"
#include "../editormanager/editormanager.h"
#include "../icore.h"
#include "../modemanager.h"

#include <utils/algorithm.h>
#include <utils/execmenu.h>
#include <utils/fancylineedit.h>
#include <utils/fsengine/fileiconprovider.h>
#include <utils/highlightingitemdelegate.h>
#include <utils/hostosinfo.h>
#include <utils/itemviews.h>
#include <utils/progressindicator.h>
#include <utils/qtcassert.h>
#include <utils/tooltip/tooltip.h>
#include <utils/utilsicons.h>
#include <QAction>
#include <QApplication>
#include <QColor>
#include <QEvent>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QKeyEvent>
#include <QMainWindow>
#include <QMenu>
#include <QScrollBar>
#include <QToolTip>

Q_DECLARE_METATYPE(Core::LocatorFilterEntry)

using namespace Utils;

const int LocatorEntryRole = int(HighlightingItemRole::User);

namespace Core {
namespace Internal {

/* A model to represent the Locator results. */
class LocatorModel : public QAbstractListModel
{
public:

    enum Columns {
        DisplayNameColumn,
        ExtraInfoColumn,
        ColumnCount
    };

    LocatorModel(QObject *parent = nullptr)
        : QAbstractListModel(parent)
        , m_backgroundColor(Utils::creatorTheme()->color(Theme::TextColorHighlightBackground))
        , m_foregroundColor(Utils::creatorTheme()->color(Theme::TextColorNormal))
    {}

    void clear();
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    void addEntries(const LocatorFilterEntries &entries);

private:
    mutable LocatorFilterEntries m_entries;
    bool m_hasExtraInfo = false;
    QColor m_backgroundColor;
    QColor m_foregroundColor;
};

class CompletionDelegate : public HighlightingItemDelegate
{
public:
    CompletionDelegate(QObject *parent);

    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override;
};

class CompletionList : public TreeView
{
public:
    CompletionList(QWidget *parent = nullptr);

    void setModel(QAbstractItemModel *model) override;

    void resizeHeaders();

    void next();
    void previous();

    void showCurrentItemToolTip();

    void keyPressEvent(QKeyEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    QMetaObject::Connection m_updateSizeConnection;
};

class TopLeftLocatorPopup final : public LocatorPopup
{
public:
    TopLeftLocatorPopup(LocatorWidget *locatorWidget)
        : LocatorPopup(locatorWidget, locatorWidget) {
        doUpdateGeometry();
    }

protected:
    void doUpdateGeometry() override;
    void inputLostFocus() override;
};

class CenteredLocatorPopup final : public LocatorPopup
{
public:
    CenteredLocatorPopup(LocatorWidget *locatorWidget, QWidget *parent)
        : LocatorPopup(locatorWidget, parent) {
        doUpdateGeometry();
    }

protected:
    void doUpdateGeometry() override;
};

// =========== LocatorModel ===========

void LocatorModel::clear()
{
    beginResetModel();
    m_entries.clear();
    m_hasExtraInfo = false;
    endResetModel();
}

int LocatorModel::rowCount(const QModelIndex & parent) const
{
    if (parent.isValid())
        return 0;
    return m_entries.size();
}

int LocatorModel::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return m_hasExtraInfo ? ColumnCount : 1;
}

QVariant LocatorModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_entries.size())
        return QVariant();

    switch (role) {
    case Qt::DisplayRole:
        if (index.column() == DisplayNameColumn)
            return m_entries.at(index.row()).displayName;
        else if (index.column() == ExtraInfoColumn)
            return m_entries.at(index.row()).extraInfo;
        break;
    case Qt::ToolTipRole: {
        const LocatorFilterEntry &entry = m_entries.at(index.row());
        QString toolTip = entry.displayName;
        if (!entry.extraInfo.isEmpty())
            toolTip += "\n\n" + entry.extraInfo;
        if (!entry.toolTip.isEmpty())
            toolTip += "\n\n" + entry.toolTip;
        return QVariant(toolTip);
    }
    case Qt::DecorationRole:
        if (index.column() == DisplayNameColumn) {
            LocatorFilterEntry &entry = m_entries[index.row()];
            if (!entry.displayIcon && !entry.filePath.isEmpty())
                entry.displayIcon = FileIconProvider::icon(entry.filePath);
            return entry.displayIcon ? entry.displayIcon.value() : QIcon();
        }
        break;
    case Qt::ForegroundRole:
        if (index.column() == ExtraInfoColumn)
            return QColor(Qt::darkGray);
        break;
    case LocatorEntryRole:
        return QVariant::fromValue(m_entries.at(index.row()));
    case int(HighlightingItemRole::StartColumn):
    case int(HighlightingItemRole::Length): {
        const LocatorFilterEntry &entry = m_entries[index.row()];
        auto highlights = [&](LocatorFilterEntry::HighlightInfo::DataType type){
            const bool startIndexRole = role == int(HighlightingItemRole::StartColumn);
            return startIndexRole ? QVariant::fromValue(entry.highlightInfo.starts(type))
                                  : QVariant::fromValue(entry.highlightInfo.lengths(type));
        };
        switch (index.column()) {
        case DisplayNameColumn: return highlights(LocatorFilterEntry::HighlightInfo::DisplayName);
        case ExtraInfoColumn: return highlights(LocatorFilterEntry::HighlightInfo::ExtraInfo);
        }
        break;
    }
    case int(HighlightingItemRole::DisplayExtra): {
        if (index.column() == LocatorFilterEntry::HighlightInfo::DisplayName) {
            LocatorFilterEntry &entry = m_entries[index.row()];
            if (!entry.displayExtra.isEmpty())
                return QString("   (" + entry.displayExtra + ')');
        }
        break;
    }
    case int(HighlightingItemRole::DisplayExtraForeground):
        return QColor(Qt::darkGray);
    case int(HighlightingItemRole::Background):
        return m_backgroundColor;
    case int(HighlightingItemRole::Foreground):
        return m_foregroundColor;
    }

    return QVariant();
}

void LocatorModel::addEntries(const QList<LocatorFilterEntry> &entries)
{
    beginInsertRows(QModelIndex(), m_entries.size(), m_entries.size() + entries.size() - 1);
    m_entries.append(entries);
    endInsertRows();
    if (m_hasExtraInfo)
        return;
    if (Utils::anyOf(entries, [](const LocatorFilterEntry &e) { return !e.extraInfo.isEmpty();})) {
        beginInsertColumns(QModelIndex(), 1, 1);
        m_hasExtraInfo = true;
        endInsertColumns();
    }
}

// =========== CompletionList ===========

CompletionList::CompletionList(QWidget *parent)
    : TreeView(parent)
{
    // on macOS and Windows the popup doesn't really get focus, so fake the selection color
    // which would then just be a very light gray, but should look as if it had focus
    QPalette p = palette();
    p.setBrush(QPalette::Inactive,
               QPalette::Highlight,
               p.brush(QPalette::Normal, QPalette::Highlight));
    setPalette(p);

    setItemDelegate(new CompletionDelegate(this));
    setRootIsDecorated(false);
    header()->hide();
    header()->setStretchLastSection(true);
    // This is too slow when done on all results
    //header()->setSectionResizeMode(QHeaderView::ResizeToContents);
    if (HostOsInfo::isMacHost()) {
        if (horizontalScrollBar())
            horizontalScrollBar()->setAttribute(Qt::WA_MacMiniSize);
        if (verticalScrollBar())
            verticalScrollBar()->setAttribute(Qt::WA_MacMiniSize);
    }
    installEventFilter(this);
}

void CompletionList::setModel(QAbstractItemModel *newModel)
{
    const auto updateSize = [this] {
        if (model() && model()->rowCount() > 0) {
            const QSize shint = sizeHintForIndex(model()->index(0, 0));
            setFixedHeight(shint.height() * 17 + frameWidth() * 2);
            disconnect(m_updateSizeConnection);
        }
    };

    if (model()) {
        disconnect(model(), nullptr, this, nullptr);
    }
    QTreeView::setModel(newModel);
    if (newModel) {
        connect(newModel, &QAbstractItemModel::columnsInserted,
                this, &CompletionList::resizeHeaders);
        m_updateSizeConnection = connect(newModel, &QAbstractItemModel::rowsInserted,
                                         this, updateSize);
    }
}

void LocatorPopup::doUpdateGeometry()
{
    m_tree->resizeHeaders();
}

void TopLeftLocatorPopup::doUpdateGeometry()
{
    QTC_ASSERT(parentWidget(), return);
    const QSize size = preferredSize();
    const int border = m_tree->frameWidth();
    const QRect rect(parentWidget()->mapToGlobal(QPoint(-border, -size.height() - border)), size);
    setGeometry(rect);
    LocatorPopup::doUpdateGeometry();
}

void CenteredLocatorPopup::doUpdateGeometry()
{
    QTC_ASSERT(parentWidget(), return);
    const QSize size = preferredSize();
    const QSize parentSize = parentWidget()->size();
    const QPoint local((parentSize.width() - size.width()) / 2,
                       (parentSize.height() - size.height()) / 2);
    const QPoint pos = parentWidget()->mapToGlobal(local);
    QRect rect(pos, size);
    // invisible widget doesn't have the right screen set yet, so use the parent widget to
    // check for available geometry
    const QRect available = parentWidget()->screen()->availableGeometry();
    if (rect.right() > available.right())
        rect.moveRight(available.right());
    if (rect.bottom() > available.bottom())
        rect.moveBottom(available.bottom());
    if (rect.top() < available.top())
        rect.moveTop(available.top());
    if (rect.left() < available.left())
        rect.moveLeft(available.left());
    setGeometry(rect);
    LocatorPopup::doUpdateGeometry();
}

void LocatorPopup::updateWindow()
{
    QWidget *w = parentWidget() ? parentWidget()->window() : nullptr;
    if (m_window != w) {
        if (m_window)
            m_window->removeEventFilter(this);
        m_window = w;
        if (m_window)
            m_window->installEventFilter(this);
    }
}

bool LocatorPopup::event(QEvent *event)
{
    if (event->type() == QEvent::ParentChange) {
        updateWindow();
    } else if (event->type() == QEvent::Show) {
        // make sure the popup has correct position before it becomes visible
        doUpdateGeometry();
    } else if (event->type() == QEvent::LayoutRequest) {
        // completion list resizes after first items are shown --> LayoutRequest
        QMetaObject::invokeMethod(this, &LocatorPopup::doUpdateGeometry, Qt::QueuedConnection);
    } else if (event->type() == QEvent::ShortcutOverride) {
        // if we (the popup) has focus, we need to handle escape manually (Windows)
        auto ke = static_cast<QKeyEvent *>(event);
        if (ke->modifiers() == Qt::NoModifier && ke->key() == Qt::Key_Escape)
            event->accept();
    } else if (event->type() == QEvent::KeyPress) {
        // if we (the popup) has focus, we need to handle escape manually (Windows)
        auto ke = static_cast<QKeyEvent *>(event);
        if (ke->modifiers() == Qt::NoModifier && ke->key() == Qt::Key_Escape)
            hide();
    }
    return QWidget::event(event);
}

bool LocatorPopup::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == m_tree && event->type() == QEvent::FocusOut) {
        // if the tree had focus and another application is brought to foreground,
        // we need to hide the popup because it otherwise stays on top of
        // everything else (even other applications) (Windows)
        auto fe = static_cast<QFocusEvent *>(event);
        if (fe->reason() == Qt::ActiveWindowFocusReason && !QApplication::activeWindow())
            hide();
    } else if (watched == m_window && event->type() == QEvent::Resize) {
        doUpdateGeometry();
    }
    return QWidget::eventFilter(watched, event);
}

QSize LocatorPopup::preferredSize()
{
    static const int MIN_WIDTH = 730;
    const QSize windowSize = m_window ? m_window->size() : QSize(MIN_WIDTH, 0);

    const int width = qMax(MIN_WIDTH, windowSize.width() * 2 / 3);
    return QSize(width, sizeHint().height());
}

void TopLeftLocatorPopup::inputLostFocus()
{
    if (!isActiveWindow())
        hide();
}

void LocatorPopup::inputLostFocus()
{
}

void CompletionList::resizeHeaders()
{
    header()->resizeSection(0, width() / 2);
    header()->resizeSection(1, 0); // last section is auto resized because of stretchLastSection
}

LocatorPopup::LocatorPopup(LocatorWidget *locatorWidget, QWidget *parent)
    : QWidget(parent),
      m_tree(new CompletionList(this)),
      m_inputWidget(locatorWidget)
{
    if (HostOsInfo::isMacHost())
        m_tree->setFrameStyle(QFrame::NoFrame); // tool tip already includes a frame
    m_tree->setModel(locatorWidget->model());
    m_tree->setTextElideMode(Qt::ElideMiddle);
    m_tree->installEventFilter(this);

    auto layout = new QVBoxLayout;
    layout->setSizeConstraint(QLayout::SetMinimumSize);
    setLayout(layout);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->addWidget(m_tree);

    connect(locatorWidget, &LocatorWidget::parentChanged, this, &LocatorPopup::updateWindow);
    connect(locatorWidget, &LocatorWidget::showPopup, this, &LocatorPopup::show);
    connect(locatorWidget, &LocatorWidget::hidePopup, this, &LocatorPopup::close);
    connect(locatorWidget, &LocatorWidget::lostFocus, this, &LocatorPopup::inputLostFocus,
            Qt::QueuedConnection);
    connect(locatorWidget, &LocatorWidget::selectRow, m_tree, [this](int row) {
        m_tree->setCurrentIndex(m_tree->model()->index(row, 0));
    });
    connect(locatorWidget, &LocatorWidget::showCurrentItemToolTip,
            m_tree, &CompletionList::showCurrentItemToolTip);
    connect(locatorWidget, &LocatorWidget::handleKey, this, [this](QKeyEvent *keyEvent) {
        QApplication::sendEvent(m_tree, keyEvent);
    }, Qt::DirectConnection); // must be handled directly before event is deleted
    connect(m_tree, &QAbstractItemView::activated, locatorWidget,
            [this, locatorWidget](const QModelIndex &index) {
        if (!index.isValid() || !isVisible())
            return;
        locatorWidget->acceptEntry(index.row());
    });
}

CompletionList *LocatorPopup::completionList() const
{
    return m_tree;
}

LocatorWidget *LocatorPopup::inputWidget() const
{
    return m_inputWidget;
}

void LocatorPopup::focusOutEvent(QFocusEvent *event)
{
    if (event->reason() == Qt::ActiveWindowFocusReason)
        hide();
    QWidget::focusOutEvent(event);
}

void CompletionList::next()
{
    int index = currentIndex().row();
    ++index;
    if (index >= model()->rowCount(QModelIndex()))
        index = 0; // wrap
    setCurrentIndex(model()->index(index, 0));
}

void CompletionList::previous()
{
    int index = currentIndex().row();
    --index;
    if (index < 0)
        index = model()->rowCount(QModelIndex()) - 1; // wrap
    setCurrentIndex(model()->index(index, 0));
}

void CompletionList::showCurrentItemToolTip()
{
    QTC_ASSERT(model(), return);
    if (!isVisible())
        return;
    const QModelIndex index = currentIndex();
    if (index.isValid()) {
        QToolTip::showText(mapToGlobal(pos() + visualRect(index).topRight()),
                           model()->data(index, Qt::ToolTipRole).toString());
    }
}

void CompletionList::keyPressEvent(QKeyEvent *event)
{
    switch (event->key()) {
    case Qt::Key_Tab:
    case Qt::Key_Down:
        next();
        return;
    case Qt::Key_Backtab:
    case Qt::Key_Up:
        previous();
        return;
    case Qt::Key_P:
    case Qt::Key_N:
        if (event->modifiers() == Qt::KeyboardModifiers(HostOsInfo::controlModifier())) {
            if (event->key() == Qt::Key_P)
                previous();
            else
                next();
            return;
        }
        break;
    case Qt::Key_Return:
    case Qt::Key_Enter:
        // emit activated even if current index is not valid
        // if there are no results yet, this allows activating the first entry when it is available
        // (see LocatorWidget::addSearchResults)
        if (event->modifiers() == 0) {
            emit activated(currentIndex());
            return;
        }
        break;
    }
    TreeView::keyPressEvent(event);
}

bool CompletionList::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == this && event->type() == QEvent::ShortcutOverride) {
        auto ke = static_cast<QKeyEvent *>(event);
        switch (ke->key()) {
        case Qt::Key_Escape:
            if (!ke->modifiers()) {
                event->accept();
                return true;
            }
            break;
        case Qt::Key_P:
        case Qt::Key_N:
            if (ke->modifiers() == Qt::KeyboardModifiers(HostOsInfo::controlModifier())) {
                event->accept();
                return true;
            }
            break;
        }
    }
    return TreeView::eventFilter(watched, event);
}

// =========== LocatorWidget ===========

LocatorWidget::LocatorWidget(Locator *locator)
    : m_locatorModel(new LocatorModel(this))
    , m_filterMenu(new QMenu(this))
    , m_centeredPopupAction(new QAction(Tr::tr("Open as Centered Popup"), this))
    , m_refreshAction(new QAction(Tr::tr("Refresh"), this))
    , m_configureAction(new QAction(ICore::msgShowOptionsDialog(), this))
    , m_fileLineEdit(new FancyLineEdit)
{
    setAttribute(Qt::WA_Hover);
    setFocusProxy(m_fileLineEdit);
    resize(200, 90);
    QSizePolicy sizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred);
    sizePolicy.setHorizontalStretch(0);
    sizePolicy.setVerticalStretch(0);
    setSizePolicy(sizePolicy);
    setMinimumSize(QSize(200, 0));

    auto layout = new QHBoxLayout(this);
    setLayout(layout);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_fileLineEdit);

    const QIcon icon = Utils::Icons::MAGNIFIER.icon();
    m_fileLineEdit->setFiltering(true);
    m_fileLineEdit->setButtonIcon(FancyLineEdit::Left, icon);
    m_fileLineEdit->setButtonToolTip(FancyLineEdit::Left, Tr::tr("Options"));
    m_fileLineEdit->setFocusPolicy(Qt::ClickFocus);
    m_fileLineEdit->setButtonVisible(FancyLineEdit::Left, true);
    // We set click focus since otherwise you will always get two popups
    m_fileLineEdit->setButtonFocusPolicy(FancyLineEdit::Left, Qt::ClickFocus);
    m_fileLineEdit->setAttribute(Qt::WA_MacShowFocusRect, false);

    m_fileLineEdit->installEventFilter(this);
    this->installEventFilter(this);

    m_centeredPopupAction->setCheckable(true);
    m_centeredPopupAction->setChecked(Locator::useCenteredPopupForShortcut());

    connect(m_filterMenu, &QMenu::aboutToShow, this, [this] {
        m_centeredPopupAction->setChecked(Locator::useCenteredPopupForShortcut());
    });
    Utils::addToolTipsToMenu(m_filterMenu);

    connect(m_centeredPopupAction, &QAction::toggled, locator, [locator](bool toggled) {
        if (toggled != Locator::useCenteredPopupForShortcut()) {
            Locator::setUseCenteredPopupForShortcut(toggled);
            QMetaObject::invokeMethod(locator, [] { LocatorManager::show({}); });
        }
    });

    m_filterMenu->addAction(m_centeredPopupAction);
    m_filterMenu->addAction(m_refreshAction);
    m_filterMenu->addAction(m_configureAction);

    m_fileLineEdit->setButtonMenu(FancyLineEdit::Left, m_filterMenu);

    connect(m_refreshAction, &QAction::triggered, locator, [locator] {
        locator->refresh(Locator::filters());
    });
    connect(m_configureAction, &QAction::triggered, this, &LocatorWidget::showConfigureDialog);
    connect(m_fileLineEdit, &QLineEdit::textChanged, this, &LocatorWidget::showPopupNow);

    m_progressIndicator = new ProgressIndicator(ProgressIndicatorSize::Small, m_fileLineEdit);
    m_progressIndicator->raise();
    m_progressIndicator->hide();
    m_showProgressTimer.setSingleShot(true);
    m_showProgressTimer.setInterval(50); // don't show progress for < 50ms tasks
    connect(&m_showProgressTimer, &QTimer::timeout,
            this, [this] { setProgressIndicatorVisible(true); });

    Command *locateCmd = ActionManager::command(Constants::LOCATE);
    if (QTC_GUARD(locateCmd)) {
        connect(locateCmd, &Command::keySequenceChanged, this, [this, locateCmd] {
            updatePlaceholderText(locateCmd);
        });
        updatePlaceholderText(locateCmd);
    }

    connect(qApp, &QApplication::focusChanged, this, &LocatorWidget::updatePreviousFocusWidget);

    connect(locator, &Locator::filtersChanged, this, &LocatorWidget::updateFilterList);
    updateFilterList();
}

LocatorWidget::~LocatorWidget() = default;

void LocatorWidget::updatePlaceholderText(Command *command)
{
    QTC_ASSERT(command, return);
    if (command->keySequence().isEmpty())
        m_fileLineEdit->setPlaceholderText(Tr::tr("Type to locate"));
    else
        m_fileLineEdit->setPlaceholderText(Tr::tr("Type to locate (%1)").arg(
                                        command->keySequence().toString(QKeySequence::NativeText)));
}

void LocatorWidget::updateFilterList()
{
    m_filterMenu->clear();
    const QList<ILocatorFilter *> filters = Utils::sorted(
        Locator::filters(), [](ILocatorFilter *a, ILocatorFilter *b) {
            return a->displayName() < b->displayName();
        });
    for (ILocatorFilter *filter : filters) {
        if (filter->shortcutString().isEmpty() || filter->isHidden())
            continue;
        QAction *action = m_filterMenu->addAction(filter->displayName());
        action->setEnabled(filter->isEnabled());
        const QString description = filter->description();
        action->setToolTip(description.isEmpty() ? QString()
                                                 : ("<html>" + description.toHtmlEscaped()));
        connect(filter, &ILocatorFilter::enabledChanged, action, &QAction::setEnabled);
        connect(action, &QAction::triggered, this, [this, filter] {
            Locator::showFilter(filter, this);
        });
    }
    m_filterMenu->addSeparator();
    m_filterMenu->addAction(m_centeredPopupAction);
    m_filterMenu->addAction(m_refreshAction);
    m_filterMenu->addAction(m_configureAction);
}

bool LocatorWidget::isInMainWindow() const
{
    return window() == ICore::mainWindow();
}

void LocatorWidget::updatePreviousFocusWidget(QWidget *previous, QWidget *current)
{
    const auto isInLocator = [this](QWidget *w) { return w == this || isAncestorOf(w); };
    if (isInLocator(current) && !isInLocator(previous))
        m_previousFocusWidget = previous;
}

static void resetFocus(QPointer<QWidget> previousFocus, bool isInMainWindow)
{
    if (previousFocus) {
        previousFocus->setFocus();
        ICore::raiseWindow(previousFocus);
    } else if (isInMainWindow) {
        ModeManager::setFocusToCurrentMode();
    }
}

bool LocatorWidget::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == m_fileLineEdit && event->type() == QEvent::ShortcutOverride) {
        auto keyEvent = static_cast<QKeyEvent *>(event);
        switch (keyEvent->key()) {
        case Qt::Key_P:
        case Qt::Key_N:
            if (keyEvent->modifiers() == Qt::KeyboardModifiers(HostOsInfo::controlModifier())) {
                event->accept();
                return true;
            }
        }
    } else if (obj == m_fileLineEdit && event->type() == QEvent::KeyPress) {
        if (m_possibleToolTipRequest)
            m_possibleToolTipRequest = false;
        if (QToolTip::isVisible())
            QToolTip::hideText();

        auto keyEvent = static_cast<QKeyEvent *>(event);

        if (keyEvent->matches(QKeySequence::MoveToStartOfBlock)
            || keyEvent->matches(QKeySequence::SelectStartOfBlock)
            || keyEvent->matches(QKeySequence::MoveToStartOfLine)
            || keyEvent->matches(QKeySequence::SelectStartOfLine)) {
            const int filterEndIndex = currentText().indexOf(' ');
            if (filterEndIndex > 0 && filterEndIndex < currentText().length() - 1) {
                const bool startsWithShortcutString
                    = Utils::anyOf(Locator::filters(),
                                   [shortcutString = currentText().left(filterEndIndex)](
                                       const ILocatorFilter *filter) {
                                       return filter->isEnabled() && !filter->isHidden()
                                              && filter->shortcutString() == shortcutString;
                                   });
                if (startsWithShortcutString) {
                    const int cursorPosition = m_fileLineEdit->cursorPosition();
                    const int patternStart = filterEndIndex + 1;
                    const bool mark = keyEvent->matches(QKeySequence::SelectStartOfBlock)
                                      || keyEvent->matches(QKeySequence::SelectStartOfLine);
                    if (cursorPosition == patternStart) {
                        m_fileLineEdit->home(mark);
                    } else {
                        const int diff = m_fileLineEdit->cursorPosition() - patternStart;
                        if (diff < 0)
                            m_fileLineEdit->cursorForward(mark, qAbs(diff));
                        else
                            m_fileLineEdit->cursorBackward(mark, diff);
                    }
                    return true;
                }
            }
        }

        switch (keyEvent->key()) {
        case Qt::Key_PageUp:
        case Qt::Key_PageDown:
        case Qt::Key_Down:
        case Qt::Key_Tab:
        case Qt::Key_Up:
        case Qt::Key_Backtab:
            emit showPopup();
            emit handleKey(keyEvent);
            return true;
        case Qt::Key_Home:
        case Qt::Key_End:
            if (HostOsInfo::isMacHost()
                    != (keyEvent->modifiers() == Qt::KeyboardModifiers(Qt::ControlModifier))) {
                emit showPopup();
                emit handleKey(keyEvent);
                return true;
            }
            break;
        case Qt::Key_Enter:
        case Qt::Key_Return:
            emit handleKey(keyEvent);
            return true;
        case Qt::Key_Escape:
            emit hidePopup();
            return true;
        case Qt::Key_Alt:
            if (keyEvent->modifiers() == Qt::AltModifier) {
                m_possibleToolTipRequest = true;
                return true;
            }
            break;
        case Qt::Key_P:
        case Qt::Key_N:
            if (keyEvent->modifiers() == Qt::KeyboardModifiers(HostOsInfo::controlModifier())) {
                emit showPopup();
                emit handleKey(keyEvent);
                return true;
            }
            break;
        default:
            break;
        }
    } else if (obj == m_fileLineEdit && event->type() == QEvent::KeyRelease) {
        auto keyEvent = static_cast<QKeyEvent *>(event);
        if (m_possibleToolTipRequest) {
            m_possibleToolTipRequest = false;
            if ((keyEvent->key() == Qt::Key_Alt)
                    && (keyEvent->modifiers() == Qt::NoModifier)) {
                emit showCurrentItemToolTip();
                return true;
            }
        }
    } else if (obj == m_fileLineEdit && event->type() == QEvent::FocusOut) {
        emit lostFocus();
    } else if (obj == m_fileLineEdit && event->type() == QEvent::FocusIn) {
        auto fev = static_cast<QFocusEvent *>(event);
        if (fev->reason() != Qt::ActiveWindowFocusReason)
            showPopupNow();
    } else if (obj == this && event->type() == QEvent::ParentChange) {
        emit parentChanged();
    } else if (obj == this && event->type() == QEvent::ShortcutOverride) {
        auto ke = static_cast<QKeyEvent *>(event);
        switch (ke->key()) {
        case Qt::Key_Escape:
            if (!ke->modifiers()) {
                event->accept();
                QMetaObject::invokeMethod(this, [focus = m_previousFocusWidget,
                                          isInMainWindow = isInMainWindow()] {
                    resetFocus(focus, isInMainWindow);
                }, Qt::QueuedConnection);
                return true;
            }
            break;
        case Qt::Key_Alt:
            if (ke->modifiers() == Qt::AltModifier) {
                event->accept();
                return true;
            }
            break;
        default:
            break;
        }
    }
    return QWidget::eventFilter(obj, event);
}

void LocatorWidget::showPopupNow()
{
    runMatcher(m_fileLineEdit->text());
    emit showPopup();
}

QList<ILocatorFilter *> LocatorWidget::filtersFor(const QString &text, QString &searchText)
{
    const int length = text.size();
    int firstNonSpace;
    for (firstNonSpace = 0; firstNonSpace < length; ++firstNonSpace) {
        if (!text.at(firstNonSpace).isSpace())
            break;
    }
    const int whiteSpace = text.indexOf(QChar::Space, firstNonSpace);
    const QList<ILocatorFilter *> filters = Utils::filtered(Locator::filters(),
                                                            &ILocatorFilter::isEnabled);
    if (whiteSpace >= 0) {
        const QString prefix = text.mid(firstNonSpace, whiteSpace - firstNonSpace).toLower();
        QList<ILocatorFilter *> prefixFilters;
        for (ILocatorFilter *filter : filters) {
            if (prefix == filter->shortcutString()) {
                searchText = text.mid(whiteSpace).trimmed();
                prefixFilters << filter;
            }
        }
        if (!prefixFilters.isEmpty())
            return prefixFilters;
    }
    searchText = text.trimmed();
    return Utils::filtered(filters, &ILocatorFilter::isIncludedByDefault);
}

void LocatorWidget::setProgressIndicatorVisible(bool visible)
{
    if (!visible) {
        m_progressIndicator->hide();
        return;
    }
    const QSize iconSize = m_progressIndicator->sizeHint();
    m_progressIndicator->setGeometry(m_fileLineEdit->button(FancyLineEdit::Right)->geometry().x()
                                     - iconSize.width(),
                                     (m_fileLineEdit->height() - iconSize.height()) / 2 /*center*/,
                                     iconSize.width(),
                                     iconSize.height());
    m_progressIndicator->show();
}

void LocatorWidget::runMatcher(const QString &text)
{
    QString searchText;
    const QList<ILocatorFilter *> filters = filtersFor(text, searchText);

    LocatorMatcherTasks tasks;
    for (ILocatorFilter *filter : filters)
        tasks += filter->matchers();

    m_locatorMatcher.reset(new LocatorMatcher);
    m_locatorMatcher->setTasks(tasks);
    m_locatorMatcher->setInputData(searchText);
    m_rowRequestedForAccept.reset();

    std::shared_ptr<std::atomic_bool> needsClearResult = std::make_shared<std::atomic_bool>(true);
    connect(m_locatorMatcher.get(), &LocatorMatcher::done, this, [this, needsClearResult] {
        m_showProgressTimer.stop();
        setProgressIndicatorVisible(false);
        m_locatorMatcher.release()->deleteLater();
        if (m_rowRequestedForAccept) {
            acceptEntry(m_rowRequestedForAccept.value());
            m_rowRequestedForAccept.reset();
            return;
        }
        if (needsClearResult->exchange(false))
            m_locatorModel->clear();
    });
    connect(m_locatorMatcher.get(), &LocatorMatcher::serialOutputDataReady,
            this, [this, needsClearResult](const LocatorFilterEntries &serialOutputData) {
        if (needsClearResult->exchange(false))
            m_locatorModel->clear();
        const bool selectFirst = m_locatorModel->rowCount() == 0;
        m_locatorModel->addEntries(serialOutputData);
        if (selectFirst) {
            emit selectRow(0);
            if (m_rowRequestedForAccept)
                m_rowRequestedForAccept = 0;
        }
    });

    m_showProgressTimer.start();
    m_locatorMatcher->start();
}

void LocatorWidget::acceptEntry(int row)
{
    if (m_locatorMatcher) {
        m_rowRequestedForAccept = row;
        return;
    }
    if (row < 0 || row >= m_locatorModel->rowCount())
        return;
    const QModelIndex index = m_locatorModel->index(row, 0);
    if (!index.isValid())
        return;
    const LocatorFilterEntry entry
        = m_locatorModel->data(index, LocatorEntryRole).value<LocatorFilterEntry>();

    if (!entry.acceptor) {
        // Opening editors can open dialogs (e.g. the ssh prompt, or showing erros), so delay until
        // we have hidden the popup with emit hidePopup below and Qt actually processed that
        QMetaObject::invokeMethod(EditorManager::instance(),
            [entry] { EditorManager::openEditor(entry); }, Qt::QueuedConnection);
    }
    QWidget *focusBeforeAccept = QApplication::focusWidget();
    const AcceptResult result = entry.acceptor ? entry.acceptor() : AcceptResult();
    if (result.newText.isEmpty()) {
        emit hidePopup();
        if (QApplication::focusWidget() == focusBeforeAccept)
            resetFocus(m_previousFocusWidget, isInMainWindow());
    } else {
        showText(result.newText, result.selectionStart, result.selectionLength);
    }
}

void LocatorWidget::showText(const QString &text, int selectionStart, int selectionLength)
{
    if (!text.isEmpty())
        m_fileLineEdit->setText(text);
    m_fileLineEdit->setFocus();
    showPopupNow();
    ICore::raiseWindow(window());

    if (selectionStart >= 0) {
        m_fileLineEdit->setSelection(selectionStart, selectionLength);
        if (selectionLength == 0) // make sure the cursor is at the right position (Mac-vs.-rest difference)
            m_fileLineEdit->setCursorPosition(selectionStart);
    } else {
        m_fileLineEdit->selectAll();
    }
}

QString LocatorWidget::currentText() const
{
    return m_fileLineEdit->text();
}

QAbstractItemModel *LocatorWidget::model() const
{
    return m_locatorModel;
}

void LocatorWidget::showConfigureDialog()
{
    ICore::showOptionsDialog(Constants::FILTER_OPTIONS_PAGE);
}

LocatorWidget *createStaticLocatorWidget(Locator *locator)
{
    auto widget = new LocatorWidget(locator);
    auto popup = new TopLeftLocatorPopup(widget); // owned by widget
    popup->setWindowFlags(Qt::ToolTip);
    return widget;
}

LocatorPopup *createLocatorPopup(Locator *locator, QWidget *parent)
{
    auto widget = new LocatorWidget(locator);
    auto popup = new CenteredLocatorPopup(widget, parent);
    auto layout = qobject_cast<QVBoxLayout *>(popup->layout());
    if (QTC_GUARD(layout))
        layout->insertWidget(0, widget);
    else
        popup->layout()->addWidget(widget);
    popup->setWindowFlags(Qt::Popup);
    return popup;
}

CompletionDelegate::CompletionDelegate(QObject *parent)
    : HighlightingItemDelegate(0, parent)
{
}

QSize CompletionDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    return HighlightingItemDelegate::sizeHint(option, index) + QSize(0, 2);
}

} // namespace Internal
} // namespace Core
