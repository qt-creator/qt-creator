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
#include <utils/fancylineedit.h>
#include <utils/hostosinfo.h>
#include <utils/progressindicator.h>
#include <utils/qtcassert.h>
#include <utils/runextensions.h>
#include <utils/stylehelper.h>
#include <utils/utilsicons.h>

#include <QColor>
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

Q_DECLARE_METATYPE(Core::ILocatorFilter*)
Q_DECLARE_METATYPE(Core::LocatorFilterEntry)

namespace Core {
namespace Internal {

/* A model to represent the Locator results. */
class LocatorModel : public QAbstractListModel
{
public:
    LocatorModel(QObject *parent = 0)
        : QAbstractListModel(parent)
    {}

    void clear();
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    void addEntries(const QList<LocatorFilterEntry> &entries);

private:
    mutable QList<LocatorFilterEntry> mEntries;
};

class CompletionList : public QTreeView
{
public:
    CompletionList(QWidget *parent = 0);

    void resize();
    QSize preferredSize() const { return m_preferredSize; }

    void focusOutEvent (QFocusEvent *event) {
        if (event->reason() == Qt::ActiveWindowFocusReason)
            hide();
        QTreeView::focusOutEvent(event);
    }

    void next() {
        int index = currentIndex().row();
        ++index;
        if (index >= model()->rowCount(QModelIndex())) {
            // wrap
            index = 0;
        }
        setCurrentIndex(model()->index(index, 0));
    }

    void previous() {
        int index = currentIndex().row();
        --index;
        if (index < 0) {
            // wrap
            index = model()->rowCount(QModelIndex()) - 1;
        }
        setCurrentIndex(model()->index(index, 0));
    }

private:
    QSize m_preferredSize;
};

// =========== LocatorModel ===========

void LocatorModel::clear()
{
    beginResetModel();
    mEntries.clear();
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
    return parent.isValid() ? 0 : 2;
}

QVariant LocatorModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= mEntries.size())
        return QVariant();

    switch (role) {
    case Qt::DisplayRole:
        if (index.column() == 0)
            return mEntries.at(index.row()).displayName;
        else if (index.column() == 1)
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
        if (index.column() == 0) {
            LocatorFilterEntry &entry = mEntries[index.row()];
            if (!entry.fileIconResolved && !entry.fileName.isEmpty() && entry.displayIcon.isNull()) {
                entry.fileIconResolved = true;
                entry.displayIcon = FileIconProvider::icon(entry.fileName);
            }
            return entry.displayIcon;
        }
        break;
    case Qt::ForegroundRole:
        if (index.column() == 1)
            return QColor(Qt::darkGray);
        break;
    case Qt::UserRole:
        return qVariantFromValue(mEntries.at(index.row()));
    }

    return QVariant();
}

void LocatorModel::addEntries(const QList<LocatorFilterEntry> &entries)
{
    beginInsertRows(QModelIndex(), mEntries.size(), mEntries.size() + entries.size() - 1);
    mEntries.append(entries);
    endInsertRows();
}

// =========== CompletionList ===========

CompletionList::CompletionList(QWidget *parent)
    : QTreeView(parent)
{
    setRootIsDecorated(false);
    setUniformRowHeights(true);
    header()->hide();
    header()->setStretchLastSection(true);
    // This is too slow when done on all results
    //header()->setSectionResizeMode(QHeaderView::ResizeToContents);
    setWindowFlags(Qt::ToolTip);
    if (Utils::HostOsInfo::isMacHost()) {
        if (horizontalScrollBar())
            horizontalScrollBar()->setAttribute(Qt::WA_MacMiniSize);
        if (verticalScrollBar())
            verticalScrollBar()->setAttribute(Qt::WA_MacMiniSize);
    }
}

void CompletionList::resize()
{
    const QStyleOptionViewItem &option = viewOptions();
    const QSize shint = itemDelegate()->sizeHint(option, model()->index(0, 0));
    const QSize windowSize = ICore::mainWindow()->size();

    const int width = qMax(730, windowSize.width() * 2 / 3);
    m_preferredSize = QSize(width, shint.height() * 17 + frameWidth() * 2);
    header()->resizeSection(0, width / 2);
    QTreeView::resize(m_preferredSize);
}

// =========== LocatorWidget ===========

LocatorWidget::LocatorWidget(Locator *qop) :
    m_locatorPlugin(qop),
    m_locatorModel(new LocatorModel(this)),
    m_completionList(new CompletionList(this)),
    m_filterMenu(new QMenu(this)),
    m_refreshAction(new QAction(tr("Refresh"), this)),
    m_configureAction(new QAction(ICore::msgShowOptionsDialog(), this)),
    m_fileLineEdit(new Utils::FancyLineEdit)
{
    m_mainWindow = ICore::mainWindow();
    // Explicitly hide the completion list popup.
    m_completionList->hide();

    setAttribute(Qt::WA_Hover);
    setFocusProxy(m_fileLineEdit);
    setWindowTitle(tr("Locate..."));
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

    setWindowIcon(Utils::Icons::ZOOM.icon());
    const QPixmap pixmap = Utils::Icons::MAGNIFIER.pixmap();
    m_fileLineEdit->setFiltering(true);
    m_fileLineEdit->setButtonPixmap(Utils::FancyLineEdit::Left, pixmap);
    m_fileLineEdit->setButtonToolTip(Utils::FancyLineEdit::Left, tr("Options"));
    m_fileLineEdit->setFocusPolicy(Qt::ClickFocus);
    m_fileLineEdit->setButtonVisible(Utils::FancyLineEdit::Left, true);
    // We set click focus since otherwise you will always get two popups
    m_fileLineEdit->setButtonFocusPolicy(Utils::FancyLineEdit::Left, Qt::ClickFocus);
    m_fileLineEdit->setAttribute(Qt::WA_MacShowFocusRect, false);

    m_fileLineEdit->installEventFilter(this);
    this->installEventFilter(this);
    m_mainWindow->installEventFilter(this);

    m_completionList->setModel(m_locatorModel);
    m_completionList->resize();

    m_filterMenu->addAction(m_refreshAction);
    m_filterMenu->addAction(m_configureAction);

    m_fileLineEdit->setButtonMenu(Utils::FancyLineEdit::Left, m_filterMenu);

    connect(m_refreshAction, &QAction::triggered,
            m_locatorPlugin, [this]() { m_locatorPlugin->refresh(); });
    connect(m_configureAction, &QAction::triggered, this, &LocatorWidget::showConfigureDialog);
    connect(m_fileLineEdit, &QLineEdit::textChanged,
        this, &LocatorWidget::showPopup);
    connect(m_completionList, &QAbstractItemView::activated,
            this, &LocatorWidget::scheduleAcceptCurrentEntry);

    m_entriesWatcher = new QFutureWatcher<LocatorFilterEntry>(this);
    connect(m_entriesWatcher, &QFutureWatcher<LocatorFilterEntry>::resultsReadyAt,
            this, &LocatorWidget::addSearchResults);
    connect(m_entriesWatcher, &QFutureWatcher<LocatorFilterEntry>::finished,
            this, &LocatorWidget::handleSearchFinished);

    m_showPopupTimer.setInterval(100);
    m_showPopupTimer.setSingleShot(true);
    connect(&m_showPopupTimer, &QTimer::timeout, this, &LocatorWidget::showPopupNow);

    m_progressIndicator = new Utils::ProgressIndicator(Utils::ProgressIndicator::Small,
                                                       m_fileLineEdit);
    m_progressIndicator->raise();
    m_progressIndicator->hide();
    m_showProgressTimer.setSingleShot(true);
    m_showProgressTimer.setInterval(50); // don't show progress for < 50ms tasks
    connect(&m_showProgressTimer, &QTimer::timeout, [this]() { setProgressIndicatorVisible(true);});
}

void LocatorWidget::setPlaceholderText(const QString &text)
{
    m_fileLineEdit->setPlaceholderText(text);
}

void LocatorWidget::updateFilterList()
{
    typedef QMap<Id, QAction *> IdActionMap;

    m_filterMenu->clear();

    // update actions and menu
    IdActionMap actionCopy = m_filterActionMap;
    m_filterActionMap.clear();
    // register new actions, update existent
    foreach (ILocatorFilter *filter, m_locatorPlugin->filters()) {
        if (filter->shortcutString().isEmpty() || filter->isHidden())
            continue;
        Id filterId = filter->id();
        Id locatorId = filterId.withPrefix("Locator.");
        QAction *action = 0;
        Command *cmd = 0;
        if (!actionCopy.contains(filterId)) {
            // register new action
            action = new QAction(filter->displayName(), this);
            cmd = ActionManager::registerAction(action, locatorId);
            cmd->setAttribute(Command::CA_UpdateText);
            connect(action, &QAction::triggered, this, &LocatorWidget::filterSelected);
            action->setData(qVariantFromValue(filter));
        } else {
            action = actionCopy.take(filterId);
            action->setText(filter->displayName());
            cmd = ActionManager::command(locatorId);
        }
        m_filterActionMap.insert(filterId, action);
        m_filterMenu->addAction(cmd->action());
    }

    // unregister actions that are deleted now
    const IdActionMap::Iterator end = actionCopy.end();
    for (IdActionMap::Iterator it = actionCopy.begin(); it != end; ++it) {
        ActionManager::unregisterAction(it.value(), it.key().withPrefix("Locator."));
        delete it.value();
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
        case Qt::Key_Up:
        case Qt::Key_Down:
        case Qt::Key_PageUp:
        case Qt::Key_PageDown:
            showCompletionList();
            QApplication::sendEvent(m_completionList, event);
            return true;
        case Qt::Key_Home:
        case Qt::Key_End:
            if (Utils::HostOsInfo::isMacHost()
                    != (keyEvent->modifiers() == Qt::KeyboardModifiers(Qt::ControlModifier))) {
                showCompletionList();
                QApplication::sendEvent(m_completionList, event);
                return true;
            }
            break;
        case Qt::Key_Enter:
        case Qt::Key_Return:
            scheduleAcceptCurrentEntry();
            return true;
        case Qt::Key_Escape:
            m_completionList->hide();
            return true;
        case Qt::Key_Tab:
            m_completionList->next();
            return true;
        case Qt::Key_Backtab:
            m_completionList->previous();
            return true;
        case Qt::Key_Alt:
            if (keyEvent->modifiers() == Qt::AltModifier) {
                m_possibleToolTipRequest = true;
                return true;
            }
            break;
        case Qt::Key_P:
        case Qt::Key_N:
            if (keyEvent->modifiers() == Qt::KeyboardModifiers(Utils::HostOsInfo::controlModifier()))
            {
                if (keyEvent->key() == Qt::Key_P)
                    m_completionList->previous();
                else
                    m_completionList->next();
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
            if (m_completionList->isVisible()
                    && (keyEvent->key() == Qt::Key_Alt)
                    && (keyEvent->modifiers() == Qt::NoModifier)) {
                const QModelIndex index = m_completionList->currentIndex();
                if (index.isValid()) {
                    QToolTip::showText(m_completionList->pos() + m_completionList->visualRect(index).topRight(),
                                       m_locatorModel->data(index, Qt::ToolTipRole).toString());
                    return true;
                }
            }
        }
    } else if (obj == m_fileLineEdit && event->type() == QEvent::FocusOut) {
        QFocusEvent *fev = static_cast<QFocusEvent *>(event);
        if (fev->reason() != Qt::ActiveWindowFocusReason || !m_completionList->isActiveWindow())
            m_completionList->hide();
    } else if (obj == m_fileLineEdit && event->type() == QEvent::FocusIn) {
        QFocusEvent *fev = static_cast<QFocusEvent *>(event);
        if (fev->reason() != Qt::ActiveWindowFocusReason)
            showPopupNow();
    } else if (obj == m_mainWindow && event->type() == QEvent::Resize) {
        m_completionList->resize();
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

void LocatorWidget::showCompletionList()
{
    const int border = m_completionList->frameWidth();
    const QSize size = m_completionList->preferredSize();
    const QRect rect(mapToGlobal(QPoint(-border, -size.height() - border)), size);
    m_completionList->setGeometry(rect);
    m_completionList->show();
}

void LocatorWidget::showPopup()
{
    m_updateRequested = true;
    m_showPopupTimer.start();
}

void LocatorWidget::showPopupNow()
{
    m_showPopupTimer.stop();
    updateCompletionList(m_fileLineEdit->text());
    showCompletionList();
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
    const QList<ILocatorFilter *> filters = m_locatorPlugin->filters();
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
    if (m_acceptRequested) {
        acceptCurrentEntry();
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

void LocatorWidget::scheduleAcceptCurrentEntry()
{
    if (m_updateRequested) {
        // don't just accept the selected entry, since the list is not up to date
        // accept will be called after the update finished
        m_acceptRequested = true;
        // do not wait for the rest of the search to finish
        m_entriesWatcher->future().cancel();
    } else {
        acceptCurrentEntry();
    }
}

void LocatorWidget::acceptCurrentEntry()
{
    m_acceptRequested = false;
    if (!m_completionList->isVisible())
        return;
    const QModelIndex index = m_completionList->currentIndex();
    if (!index.isValid())
        return;
    const LocatorFilterEntry entry = m_locatorModel->data(index, Qt::UserRole).value<LocatorFilterEntry>();
    m_completionList->hide();
    m_fileLineEdit->clearFocus();
    entry.filter->accept(entry);
}

void LocatorWidget::show(const QString &text, int selectionStart, int selectionLength)
{
    if (!text.isEmpty())
        m_fileLineEdit->setText(text);
    m_fileLineEdit->setFocus();
    showPopupNow();
    ICore::raiseWindow(m_mainWindow);

    if (selectionStart >= 0) {
        m_fileLineEdit->setSelection(selectionStart, selectionLength);
        if (selectionLength == 0) // make sure the cursor is at the right position (Mac-vs.-rest difference)
            m_fileLineEdit->setCursorPosition(selectionStart);
    } else {
        m_fileLineEdit->selectAll();
    }
}

void LocatorWidget::filterSelected()
{
    QString searchText = tr("<type here>");
    QAction *action = qobject_cast<QAction *>(sender());
    QTC_ASSERT(action, return);
    ILocatorFilter *filter = action->data().value<ILocatorFilter *>();
    QTC_ASSERT(filter, return);
    QString currentText = m_fileLineEdit->text().trimmed();
    // add shortcut string at front or replace existing shortcut string
    if (!currentText.isEmpty()) {
        searchText = currentText;
        foreach (ILocatorFilter *otherfilter, m_locatorPlugin->filters()) {
            if (currentText.startsWith(otherfilter->shortcutString() + QLatin1Char(' '))) {
                searchText = currentText.mid(otherfilter->shortcutString().length() + 1);
                break;
            }
        }
    }
    show(filter->shortcutString() + QLatin1Char(' ') + searchText,
         filter->shortcutString().length() + 1,
         searchText.length());
    updateCompletionList(m_fileLineEdit->text());
    m_fileLineEdit->setFocus();
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
    if (selectFirst)
        m_completionList->setCurrentIndex(m_locatorModel->index(0, 0));
}

} // namespace Internal
} // namespace Core
