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

#include "locator.h"
#include "locatorwidget.h"
#include "locatorconstants.h"
#include "locatorsearchutils.h"
#include "ilocatorfilter.h"

#include <coreplugin/icore.h>
#include <coreplugin/modemanager.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/fileiconprovider.h>
#include <coreplugin/icontext.h>
#include <coreplugin/mainwindow.h>
#include <utils/algorithm.h>
#include <utils/appmainwindow.h>
#include <utils/asconst.h>
#include <utils/fancylineedit.h>
#include <utils/highlightingitemdelegate.h>
#include <utils/hostosinfo.h>
#include <utils/itemviews.h>
#include <utils/progressindicator.h>
#include <utils/qtcassert.h>
#include <utils/runextensions.h>
#include <utils/stylehelper.h>
#include <utils/utilsicons.h>

#include <QApplication>
#include <QColor>
#include <QDesktopWidget>
#include <QFileInfo>
#include <QTimer>
#include <QEvent>
#include <QAction>
#include <QApplication>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QKeyEvent>
#include <QMenu>
#include <QScrollBar>
#include <QTreeView>
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

    LocatorModel(QObject *parent = 0)
        : QAbstractListModel(parent)
        , mBackgroundColor(Utils::creatorTheme()->color(Utils::Theme::TextColorHighlightBackground).name())
    {}

    void clear();
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    void addEntries(const QList<LocatorFilterEntry> &entries);

private:
    mutable QList<LocatorFilterEntry> mEntries;
    bool hasExtraInfo = false;
    QColor mBackgroundColor;
};

class CompletionDelegate : public HighlightingItemDelegate
{
public:
    CompletionDelegate(QObject *parent);

    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override;
};

class CompletionList : public Utils::TreeView
{
public:
    CompletionList(QWidget *parent = 0);

    void setModel(QAbstractItemModel *model);

    void resizeHeaders();

    void next();
    void previous();

    void showCurrentItemToolTip();

    void keyPressEvent(QKeyEvent *event);
    bool eventFilter(QObject *watched, QEvent *event);

private:
    QMetaObject::Connection m_updateSizeConnection;
};

class TopLeftLocatorPopup : public LocatorPopup
{
public:
    TopLeftLocatorPopup(LocatorWidget *locatorWidget)
        : LocatorPopup(locatorWidget, locatorWidget) {}

protected:
    void updateGeometry() override;
    void inputLostFocus() override;
};

class CenteredLocatorPopup : public LocatorPopup
{
public:
    CenteredLocatorPopup(LocatorWidget *locatorWidget, QWidget *parent)
        : LocatorPopup(locatorWidget, parent) {}

protected:
    void updateGeometry() override;
};

// =========== LocatorModel ===========

void LocatorModel::clear()
{
    beginResetModel();
    mEntries.clear();
    hasExtraInfo = false;
    endResetModel();
}

int LocatorModel::rowCount(const QModelIndex & parent) const
{
    if (parent.isValid())
        return 0;
    return mEntries.size();
}

int LocatorModel::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return hasExtraInfo ? ColumnCount : 1;
}

QVariant LocatorModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= mEntries.size())
        return QVariant();

    switch (role) {
    case Qt::DisplayRole:
        if (index.column() == DisplayNameColumn)
            return mEntries.at(index.row()).displayName;
        else if (index.column() == ExtraInfoColumn)
            return mEntries.at(index.row()).extraInfo;
        break;
    case Qt::ToolTipRole:
        if (mEntries.at(index.row()).extraInfo.isEmpty())
            return QVariant(mEntries.at(index.row()).displayName);
        else
            return QVariant(mEntries.at(index.row()).displayName
                            + QLatin1String("\n\n") + mEntries.at(index.row()).extraInfo);
        break;
    case Qt::DecorationRole:
        if (index.column() == DisplayNameColumn) {
            LocatorFilterEntry &entry = mEntries[index.row()];
            if (!entry.displayIcon && !entry.fileName.isEmpty())
                entry.displayIcon = FileIconProvider::icon(entry.fileName);
            return entry.displayIcon ? entry.displayIcon.value() : QIcon();
        }
        break;
    case Qt::ForegroundRole:
        if (index.column() == ExtraInfoColumn)
            return QColor(Qt::darkGray);
        break;
    case LocatorEntryRole:
        return qVariantFromValue(mEntries.at(index.row()));
    case int(HighlightingItemRole::StartColumn):
    case int(HighlightingItemRole::Length): {
        LocatorFilterEntry &entry = mEntries[index.row()];
        const int highlightColumn = entry.highlightInfo.dataType == LocatorFilterEntry::HighlightInfo::DisplayName
                                                                 ? DisplayNameColumn
                                                                 : ExtraInfoColumn;
        if (highlightColumn == index.column()) {
            const bool startIndexRole = role == int(HighlightingItemRole::StartColumn);
            return startIndexRole ? QVariant::fromValue(entry.highlightInfo.starts)
                                  : QVariant::fromValue(entry.highlightInfo.lengths);
        }
        break;
    }
    case int(HighlightingItemRole::Background):
        return mBackgroundColor;
    }

    return QVariant();
}

void LocatorModel::addEntries(const QList<LocatorFilterEntry> &entries)
{
    beginInsertRows(QModelIndex(), mEntries.size(), mEntries.size() + entries.size() - 1);
    mEntries.append(entries);
    endInsertRows();
    if (hasExtraInfo)
        return;
    if (Utils::anyOf(entries, [](const LocatorFilterEntry &e) { return !e.extraInfo.isEmpty();})) {
        beginInsertColumns(QModelIndex(), 1, 1);
        hasExtraInfo = true;
        endInsertColumns();
    }
}

// =========== CompletionList ===========

CompletionList::CompletionList(QWidget *parent)
    : Utils::TreeView(parent)
{
    setItemDelegate(new CompletionDelegate(this));
    setRootIsDecorated(false);
    setUniformRowHeights(true);
    header()->hide();
    header()->setStretchLastSection(true);
    // This is too slow when done on all results
    //header()->setSectionResizeMode(QHeaderView::ResizeToContents);
    if (Utils::HostOsInfo::isMacHost()) {
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
            const QStyleOptionViewItem &option = viewOptions();
            const QSize shint = itemDelegate()->sizeHint(option, model()->index(0, 0));
            setFixedHeight(shint.height() * 17 + frameWidth() * 2);
            disconnect(m_updateSizeConnection);
        }
    };

    if (model()) {
        disconnect(model(), 0, this, 0);
    }
    QTreeView::setModel(newModel);
    if (newModel) {
        connect(newModel, &QAbstractItemModel::columnsInserted,
                this, &CompletionList::resizeHeaders);
        m_updateSizeConnection = connect(newModel, &QAbstractItemModel::rowsInserted,
                                         this, updateSize);
    }
}

void LocatorPopup::updateGeometry()
{
    m_tree->resizeHeaders();
}

void TopLeftLocatorPopup::updateGeometry()
{
    QTC_ASSERT(parentWidget(), return);
    const QSize size = preferredSize();
    const int border = m_tree->frameWidth();
    const QRect rect(parentWidget()->mapToGlobal(QPoint(-border, -size.height() - border)), size);
    setGeometry(rect);
    LocatorPopup::updateGeometry();
}

void CenteredLocatorPopup::updateGeometry()
{
    QTC_ASSERT(parentWidget(), return);
    const QSize size = preferredSize();
    const QSize parentSize = parentWidget()->size();
    const QPoint pos = parentWidget()->mapToGlobal({(parentSize.width() - size.width()) / 2,
                                                    parentSize.height() / 2 - size.height()});
    QRect rect(pos, size);
    // invisible widget doesn't have the right screen set yet, so use the parent widget to
    // check for available geometry
    const QRect available = QApplication::desktop()->availableGeometry(parentWidget());
    if (rect.right() > available.right())
        rect.moveRight(available.right());
    if (rect.bottom() > available.bottom())
        rect.moveBottom(available.bottom());
    if (rect.top() < available.top())
        rect.moveTop(available.top());
    if (rect.left() < available.left())
        rect.moveLeft(available.left());
    setGeometry(rect);
    LocatorPopup::updateGeometry();
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
    if (event->type() == QEvent::ParentChange)
        updateWindow();
    // completion list resizes after first items are shown --> LayoutRequest
    else if (event->type() == QEvent::Show || event->type() == QEvent::LayoutRequest)
        QTimer::singleShot(0, this, &LocatorPopup::updateGeometry);
    return QWidget::event(event);
}

bool LocatorPopup::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == m_window && event->type() == QEvent::Resize)
        updateGeometry();
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
    if (Utils::HostOsInfo::isMacHost())
        m_tree->setFrameStyle(QFrame::NoFrame); // tool tip already includes a frame
    m_tree->setModel(locatorWidget->model());

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
                if (isVisible())
                    locatorWidget->scheduleAcceptEntry(index);
            });

    updateGeometry();
}

CompletionList *LocatorPopup::completionList() const
{
    return m_tree;
}

LocatorWidget *LocatorPopup::inputWidget() const
{
    return m_inputWidget;
}

void LocatorPopup::focusOutEvent(QFocusEvent *event) {
    if (event->reason() == Qt::ActiveWindowFocusReason)
        hide();
    QWidget::focusOutEvent(event);
}

void CompletionList::next() {
    int index = currentIndex().row();
    ++index;
    if (index >= model()->rowCount(QModelIndex())) {
        // wrap
        index = 0;
    }
    setCurrentIndex(model()->index(index, 0));
}

void CompletionList::previous() {
    int index = currentIndex().row();
    --index;
    if (index < 0) {
        // wrap
        index = model()->rowCount(QModelIndex()) - 1;
    }
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
        if (event->modifiers() == Qt::KeyboardModifiers(Utils::HostOsInfo::controlModifier())) {
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
    Utils::TreeView::keyPressEvent(event);
}

bool CompletionList::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == this && event->type() == QEvent::ShortcutOverride) {
        QKeyEvent *ke = static_cast<QKeyEvent *>(event);
        switch (ke->key()) {
        case Qt::Key_Escape:
            if (!ke->modifiers()) {
                event->accept();
                return true;
            }
            break;
        }
    }
    return Utils::TreeView::eventFilter(watched, event);
}

// =========== LocatorWidget ===========

LocatorWidget::LocatorWidget(Locator *locator) :
    m_locatorModel(new LocatorModel(this)),
    m_filterMenu(new QMenu(this)),
    m_refreshAction(new QAction(tr("Refresh"), this)),
    m_configureAction(new QAction(ICore::msgShowOptionsDialog(), this)),
    m_fileLineEdit(new Utils::FancyLineEdit)
{
    setAttribute(Qt::WA_Hover);
    setFocusProxy(m_fileLineEdit);
    resize(200, 90);
    QSizePolicy sizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred);
    sizePolicy.setHorizontalStretch(0);
    sizePolicy.setVerticalStretch(0);
    setSizePolicy(sizePolicy);
    setMinimumSize(QSize(200, 0));

    QHBoxLayout *layout = new QHBoxLayout(this);
    setLayout(layout);
    layout->setMargin(0);
    layout->addWidget(m_fileLineEdit);

    const QIcon icon = Utils::Icons::MAGNIFIER.icon();
    m_fileLineEdit->setFiltering(true);
    m_fileLineEdit->setButtonIcon(Utils::FancyLineEdit::Left, icon);
    m_fileLineEdit->setButtonToolTip(Utils::FancyLineEdit::Left, tr("Options"));
    m_fileLineEdit->setFocusPolicy(Qt::ClickFocus);
    m_fileLineEdit->setButtonVisible(Utils::FancyLineEdit::Left, true);
    // We set click focus since otherwise you will always get two popups
    m_fileLineEdit->setButtonFocusPolicy(Utils::FancyLineEdit::Left, Qt::ClickFocus);
    m_fileLineEdit->setAttribute(Qt::WA_MacShowFocusRect, false);

    m_fileLineEdit->installEventFilter(this);
    this->installEventFilter(this);

    m_filterMenu->addAction(m_refreshAction);
    m_filterMenu->addAction(m_configureAction);

    m_fileLineEdit->setButtonMenu(Utils::FancyLineEdit::Left, m_filterMenu);

    connect(m_refreshAction, &QAction::triggered,
            locator, [locator]() { locator->refresh(); });
    connect(m_configureAction, &QAction::triggered, this, &LocatorWidget::showConfigureDialog);
    connect(m_fileLineEdit, &QLineEdit::textChanged,
        this, &LocatorWidget::showPopupDelayed);

    m_entriesWatcher = new QFutureWatcher<LocatorFilterEntry>(this);
    connect(m_entriesWatcher, &QFutureWatcher<LocatorFilterEntry>::resultsReadyAt,
            this, &LocatorWidget::addSearchResults);
    connect(m_entriesWatcher, &QFutureWatcher<LocatorFilterEntry>::finished,
            this, &LocatorWidget::handleSearchFinished);

    m_showPopupTimer.setInterval(100);
    m_showPopupTimer.setSingleShot(true);
    connect(&m_showPopupTimer, &QTimer::timeout, this, &LocatorWidget::showPopupNow);

    m_progressIndicator = new Utils::ProgressIndicator(Utils::ProgressIndicatorSize::Small,
                                                       m_fileLineEdit);
    m_progressIndicator->raise();
    m_progressIndicator->hide();
    m_showProgressTimer.setSingleShot(true);
    m_showProgressTimer.setInterval(50); // don't show progress for < 50ms tasks
    connect(&m_showProgressTimer, &QTimer::timeout, [this]() { setProgressIndicatorVisible(true);});

    Command *locateCmd = ActionManager::command(Constants::LOCATE);
    if (QTC_GUARD(locateCmd)) {
        connect(locateCmd, &Command::keySequenceChanged, this, [this,locateCmd] {
            updatePlaceholderText(locateCmd);
        });
        updatePlaceholderText(locateCmd);
    }

    connect(locator, &Locator::filtersChanged, this, &LocatorWidget::updateFilterList);
    updateFilterList();
}

void LocatorWidget::updatePlaceholderText(Command *command)
{
    QTC_ASSERT(command, return);
    if (command->keySequence().isEmpty())
        m_fileLineEdit->setPlaceholderText(tr("Type to locate"));
    else
        m_fileLineEdit->setPlaceholderText(tr("Type to locate (%1)").arg(
                                        command->keySequence().toString(QKeySequence::NativeText)));
}

void LocatorWidget::updateFilterList()
{
    m_filterMenu->clear();
    const QList<ILocatorFilter *> filters = Locator::filters();
    for (ILocatorFilter *filter : filters) {
        Command *cmd = ActionManager::command(filter->actionId());
        if (cmd)
            m_filterMenu->addAction(cmd->action());
    }
    m_filterMenu->addSeparator();
    m_filterMenu->addAction(m_refreshAction);
    m_filterMenu->addAction(m_configureAction);
}

bool LocatorWidget::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == m_fileLineEdit && event->type() == QEvent::ShortcutOverride) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
        switch (keyEvent->key()) {
        case Qt::Key_P:
        case Qt::Key_N:
            if (keyEvent->modifiers() == Qt::KeyboardModifiers(Utils::HostOsInfo::controlModifier())) {
                event->accept();
                return true;
            }
        }
    } else if (obj == m_fileLineEdit && event->type() == QEvent::KeyPress) {
        if (m_possibleToolTipRequest)
            m_possibleToolTipRequest = false;
        if (QToolTip::isVisible())
            QToolTip::hideText();

        QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
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
            if (Utils::HostOsInfo::isMacHost()
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
            if (keyEvent->modifiers() == Qt::KeyboardModifiers(Utils::HostOsInfo::controlModifier())) {
                emit showPopup();
                emit handleKey(keyEvent);
                return true;
            }
            break;
        default:
            break;
        }
    } else if (obj == m_fileLineEdit && event->type() == QEvent::KeyRelease) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
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
        QFocusEvent *fev = static_cast<QFocusEvent *>(event);
        if (fev->reason() != Qt::ActiveWindowFocusReason)
            showPopupNow();
    } else if (obj == this && event->type() == QEvent::ParentChange) {
        emit parentChanged();
    } else if (obj == this && event->type() == QEvent::ShortcutOverride) {
        QKeyEvent *ke = static_cast<QKeyEvent *>(event);
        switch (ke->key()) {
        case Qt::Key_Escape:
            if (!ke->modifiers()) {
                event->accept();
                QTimer::singleShot(0, this, &LocatorWidget::setFocusToCurrentMode);
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

void LocatorWidget::setFocusToCurrentMode()
{
    ModeManager::setFocusToCurrentMode();
}

void LocatorWidget::showPopupDelayed()
{
    m_updateRequested = true;
    m_showPopupTimer.start();
}

void LocatorWidget::showPopupNow()
{
    m_showPopupTimer.stop();
    updateCompletionList(m_fileLineEdit->text());
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
        foreach (ILocatorFilter *filter, filters) {
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
    m_progressIndicator->setGeometry(m_fileLineEdit->button(Utils::FancyLineEdit::Right)->geometry().x()
                                     - iconSize.width(),
                                     (m_fileLineEdit->height() - iconSize.height()) / 2 /*center*/,
                                     iconSize.width(),
                                     iconSize.height());
    m_progressIndicator->show();
}

void LocatorWidget::updateCompletionList(const QString &text)
{
    m_updateRequested = true;
    if (m_entriesWatcher->future().isRunning()) {
        // Cancel the old future. We may not just block the UI thread to wait for the search to
        // actually cancel, so try again when the finshed signal of the watcher ends up in
        // updateEntries() (which will call updateCompletionList again with the
        // requestedCompletionText)
        m_requestedCompletionText = text;
        m_entriesWatcher->future().cancel();
        return;
    }

    m_showProgressTimer.start();
    m_needsClearResult = true;
    QString searchText;
    const QList<ILocatorFilter *> filters = filtersFor(text, searchText);

    foreach (ILocatorFilter *filter, filters)
        filter->prepareSearch(searchText);
    QFuture<LocatorFilterEntry> future = Utils::runAsync(&runSearch, filters, searchText);
    m_entriesWatcher->setFuture(future);
}

void LocatorWidget::handleSearchFinished()
{
    m_showProgressTimer.stop();
    setProgressIndicatorVisible(false);
    m_updateRequested = false;
    if (m_rowRequestedForAccept) {
        acceptEntry(m_rowRequestedForAccept.value());
        m_rowRequestedForAccept.reset();
        return;
    }
    if (m_entriesWatcher->future().isCanceled()) {
        const QString text = m_requestedCompletionText;
        m_requestedCompletionText.clear();
        updateCompletionList(text);
        return;
    }

    if (m_needsClearResult) {
        m_locatorModel->clear();
        m_needsClearResult = false;
    }
}

void LocatorWidget::scheduleAcceptEntry(const QModelIndex &index)
{
    if (m_updateRequested) {
        // don't just accept the selected entry, since the list is not up to date
        // accept will be called after the update finished
        m_rowRequestedForAccept = index.row();
        // do not wait for the rest of the search to finish
        m_entriesWatcher->future().cancel();
    } else {
        acceptEntry(index.row());
    }
}

void LocatorWidget::acceptEntry(int row)
{
    if (row < 0 || row >= m_locatorModel->rowCount())
        return;
    const QModelIndex index = m_locatorModel->index(row, 0);
    if (!index.isValid())
        return;
    const LocatorFilterEntry entry = m_locatorModel->data(index, LocatorEntryRole).value<LocatorFilterEntry>();
    Q_ASSERT(entry.filter != nullptr);
    QString newText;
    int selectionStart = -1;
    int selectionLength = 0;
    entry.filter->accept(entry, &newText, &selectionStart, &selectionLength);
    if (newText.isEmpty()) {
        emit hidePopup();
        m_fileLineEdit->clearFocus();
    } else {
        showText(newText, selectionStart, selectionLength);
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

void LocatorWidget::addSearchResults(int firstIndex, int endIndex)
{
    if (m_needsClearResult) {
        m_locatorModel->clear();
        m_needsClearResult = false;
    }
    const bool selectFirst = m_locatorModel->rowCount() == 0;
    QList<LocatorFilterEntry> entries;
    for (int i = firstIndex; i < endIndex; ++i)
        entries.append(m_entriesWatcher->resultAt(i));
    m_locatorModel->addEntries(entries);
    if (selectFirst) {
        emit selectRow(0);
        if (m_rowRequestedForAccept)
            m_rowRequestedForAccept = 0;
    }
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
    popup->layout()->addWidget(widget);
    popup->setWindowFlags(Qt::Popup);
    popup->setAttribute(Qt::WA_DeleteOnClose);
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
