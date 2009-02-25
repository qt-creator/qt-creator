/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
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
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#include <qglobal.h>

namespace QuickOpen {
struct FilterEntry;
}

QT_BEGIN_NAMESPACE
unsigned int qHash(const QuickOpen::FilterEntry &entry);
QT_END_NAMESPACE

#include "quickopentoolwindow.h"
#include "quickopenplugin.h"
#include "quickopenconstants.h"

#include <extensionsystem/pluginmanager.h>
#include <coreplugin/icore.h>
#include <coreplugin/modemanager.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/fileiconprovider.h>
#include <utils/fancylineedit.h>
#include <utils/qtcassert.h>

#include <QtCore/QFileInfo>
#include <QtCore/QFile>
#include <QtCore/QTimer>
#include <QtCore/QSettings>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QContextMenuEvent>
#include <QtGui/QHBoxLayout>
#include <QtGui/QHeaderView>
#include <QtGui/QKeyEvent>
#include <QtGui/QLabel>
#include <QtGui/QLineEdit>
#include <QtGui/QMenu>
#include <QtGui/QMouseEvent>
#include <QtGui/QPushButton>
#include <QtGui/QScrollBar>
#include <QtGui/QTreeView>

Q_DECLARE_METATYPE(QuickOpen::IQuickOpenFilter*);
Q_DECLARE_METATYPE(QuickOpen::FilterEntry);

namespace QuickOpen {
namespace Internal {

/*! A model to represent the QuickOpen results. */
class QuickOpenModel : public QAbstractListModel
{
public:
    QuickOpenModel(QObject *parent = 0)
        : QAbstractListModel(parent)
//        , mDisplayCount(64)
    {}

    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;

    void setEntries(const QList<FilterEntry> &entries);
    //void setDisplayCount(int count);

private:
    mutable QList<FilterEntry> mEntries;
    //int mDisplayCount;
};

class CompletionList : public QTreeView
{
public:
    CompletionList(QWidget *parent = 0);

    void updatePreferredSize();
    QSize preferredSize() const { return m_preferredSize; }

private:
    QSize m_preferredSize;
};

} // namespace Internal
} // namespace QuickOpen

using namespace QuickOpen;
using namespace QuickOpen::Internal;

QT_BEGIN_NAMESPACE
uint qHash(const FilterEntry &entry)
{
    if (entry.internalData.canConvert(QVariant::String))
        return qHash(entry.internalData.toString());
    return qHash(entry.internalData.constData());
}
QT_END_NAMESPACE


// =========== QuickOpenModel ===========

int QuickOpenModel::rowCount(const QModelIndex & /* parent */) const
{
    return mEntries.size();
}

int QuickOpenModel::columnCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : 2;
}

/*!
 * When asked for the icon via Qt::DecorationRole, the QuickOpenModel lazily
 * resolves and caches the Greehouse-specific file icon when
 * FilterEntry::resolveFileIcon is true. FilterEntry::internalData is assumed
 * to be the filename.
 */
QVariant QuickOpenModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= mEntries.size())
        return QVariant();

    if (role == Qt::DisplayRole) {
        if (index.column() == 0) {
            return mEntries.at(index.row()).displayName;
        } else if (index.column() == 1) {
            return mEntries.at(index.row()).extraInfo;
        }
    } else if (role == Qt::DecorationRole && index.column() == 0) {
        FilterEntry &entry = mEntries[index.row()];
        if (entry.resolveFileIcon && entry.displayIcon.isNull()) {
            entry.resolveFileIcon = false;
            entry.displayIcon =
                 Core::FileIconProvider::instance()->icon(QFileInfo(entry.internalData.toString()));
        }
        return entry.displayIcon;
    } else if (role == Qt::ForegroundRole && index.column() == 1) {
        return Qt::darkGray;
    } else if (role == Qt::UserRole) {
        return qVariantFromValue(mEntries.at(index.row()));
    }

    return QVariant();
}

void QuickOpenModel::setEntries(const QList<FilterEntry> &entries)
{
    mEntries = entries;
    reset();
}
#if 0
void QuickOpenModel::setDisplayCount(int count)
{
    // TODO: This method is meant to be used for increasing the number of items displayed at the
    // user's request. There is however no way yet for the user to request this.
    if (count == mDisplayCount)
        return;

    const int displayedOld = qMin(mDisplayCount, mEntries.size());
    const int displayedNew = qMin(count, mEntries.size());

    if (displayedNew < displayedOld)
        beginRemoveRows(QModelIndex(), displayedNew - 1, displayedOld - 1);
    else if (displayedNew > displayedOld)
        beginInsertRows(QModelIndex(), displayedOld - 1, displayedNew - 1);

    mDisplayCount = count;

    if (displayedNew < displayedOld)
        endRemoveRows();
    else if (displayedNew > displayedOld)
        endInsertRows();
}
#endif

// =========== CompletionList ===========

CompletionList::CompletionList(QWidget *parent)
    : QTreeView(parent)
{
    setRootIsDecorated(false);
    setUniformRowHeights(true);
    setMaximumWidth(900);
    header()->hide();
    header()->setStretchLastSection(true);
    // This is too slow when done on all results
    //header()->setResizeMode(QHeaderView::ResizeToContents);
    setWindowFlags(Qt::ToolTip);
}

void CompletionList::updatePreferredSize()
{
    //header()->setStretchLastSection(false);
    //updateGeometries();

    const QStyleOptionViewItem &option = viewOptions();
    const QSize shint = itemDelegate()->sizeHint(option, model()->index(0, 0));
#if 0
    const int visibleItems = model()->rowCount();

    // TODO: Look into enabling preferred height as well. Current problem is that this doesn't
    // work nicely from the user perspective if the list is popping up instead of down.
    //const int h = shint.height() * visibleItems;

    const QScrollBar *vscroll = verticalScrollBar();
    int preferredWidth = header()->length() + frameWidth() * 2
                         + (vscroll->isVisibleTo(this) ? vscroll->width() : 0);
    const int diff = preferredWidth - width();

    // Avoid resizing the list widget when there are no results or when the preferred size is
    // only a little smaller than our current size
    if (visibleItems == 0 || (diff > -100 && diff < 0))
        preferredWidth = width();
#endif

    m_preferredSize = QSize(600, //qMax(600, preferredWidth),
                            shint.height() * 17 + frameWidth() * 2);
    //header()->setStretchLastSection(true);
}


// =========== QuickOpenToolWindow ===========

QuickOpenToolWindow::QuickOpenToolWindow(QuickOpenPlugin *qop) :
     m_quickOpenPlugin(qop),
     m_quickOpenModel(new QuickOpenModel(this)),
     m_completionList(new CompletionList(this)),
     m_filterMenu(new QMenu(this)),
     m_refreshAction(new QAction(tr("Refresh"), this)),
     m_configureAction(new QAction(tr("Configure..."), this)),
     m_fileLineEdit(new Core::Utils::FancyLineEdit)
{
    // Explicitly hide the completion list popup.
    m_completionList->hide();

    setWindowTitle("Locate...");
    resize(200, 90);
    QSizePolicy sizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
    sizePolicy.setHorizontalStretch(0);
    sizePolicy.setVerticalStretch(0);
    setSizePolicy(sizePolicy);
    setMinimumSize(QSize(200, 0));

    QHBoxLayout *layout = new QHBoxLayout(this);
    setLayout(layout);
    layout->setMargin(0);
    layout->addWidget(m_fileLineEdit);

    setWindowIcon(QIcon(":/quickopen/images/quickopen.png"));
    QPixmap image(Core::Constants::ICON_MAGNIFIER);
    m_fileLineEdit->setPixmap(image);
    m_fileLineEdit->setUseLayoutDirection(true);
    m_fileLineEdit->setHintText(tr("Type to locate"));
    m_fileLineEdit->setFocusPolicy(Qt::ClickFocus);
    m_fileLineEdit->setAttribute(Qt::WA_MacShowFocusRect, false);

    m_fileLineEdit->installEventFilter(this);
    this->installEventFilter(this);

    m_completionList->setModel(m_quickOpenModel);
    m_completionList->header()->resizeSection(0, 300);
    m_completionList->updatePreferredSize();
    m_completionList->resize(m_completionList->preferredSize());

    m_filterMenu->addAction(m_refreshAction);
    m_filterMenu->addAction(m_configureAction);

    m_fileLineEdit->setMenu( m_filterMenu);

    connect(m_refreshAction, SIGNAL(triggered()), m_quickOpenPlugin, SLOT(refresh()));
    connect(m_configureAction, SIGNAL(triggered()), this, SLOT(showConfigureDialog()));
    connect(m_fileLineEdit, SIGNAL(textEdited(const QString&)),
        this, SLOT(textEdited(const QString&)));
    connect(m_completionList, SIGNAL(activated(QModelIndex)),
            this, SLOT(acceptCurrentEntry()));
}

bool QuickOpenToolWindow::isShowingTypeHereMessage() const
{
    return m_fileLineEdit->isShowingHintText();
}

void QuickOpenToolWindow::updateFilterList()
{
    m_filterMenu->clear();
    foreach (IQuickOpenFilter *filter, m_quickOpenPlugin->filters()) {
        if (!filter->shortcutString().isEmpty() && !filter->isHidden()) {
            QAction *action = m_filterMenu->addAction(filter->trName(), this, SLOT(filterSelected()));
            action->setData(qVariantFromValue(filter));
        }
    }
    m_filterMenu->addSeparator();
    m_filterMenu->addAction(m_refreshAction);
    m_filterMenu->addAction(m_configureAction);
}

bool QuickOpenToolWindow::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == m_fileLineEdit && event->type() == QEvent::KeyPress) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
        switch (keyEvent->key()) {
        case Qt::Key_Up:
        case Qt::Key_Down:
        case Qt::Key_PageUp:
        case Qt::Key_PageDown:
            showCompletionList();
            QApplication::sendEvent(m_completionList, event);
            return true;
        case Qt::Key_Enter:
        case Qt::Key_Return:
            acceptCurrentEntry();
            return true;
        case Qt::Key_Escape:
            m_completionList->hide();
            return true;
        default:
            break;
        }
    } else if (obj == m_fileLineEdit && event->type() == QEvent::FocusOut) {
        m_completionList->hide();
    } else if (obj == m_fileLineEdit && event->type() == QEvent::FocusIn) {
        updateCompletionList(m_fileLineEdit->typedText());
        showCompletionList();
    } else if (obj == this && event->type() == QEvent::ShortcutOverride) {
        QKeyEvent *ke = static_cast<QKeyEvent *>(event);
        if (ke->key() == Qt::Key_Escape && !ke->modifiers()) {
            event->accept();
            QTimer::singleShot(0, Core::ModeManager::instance(), SLOT(setFocusToCurrentMode()));
            return true;
        }
    }
    return QWidget::eventFilter(obj, event);
}

void QuickOpenToolWindow::showCompletionList()
{
    const int border = m_completionList->frameWidth();
    const QSize size = m_completionList->preferredSize();
    const QRect rect(mapToGlobal(QPoint(-border, -size.height() - border)), size);
    m_completionList->setGeometry(rect);
    m_completionList->show();
}

void QuickOpenToolWindow::textEdited(const QString &text)
{
    updateCompletionList(text);
    showCompletionList();
}

QList<IQuickOpenFilter*> QuickOpenToolWindow::filtersFor(const QString &text, QString &searchText)
{
    QList<IQuickOpenFilter*> filters = m_quickOpenPlugin->filters();
    int whiteSpace = text.indexOf(" ");
    QString prefix;
    if (whiteSpace >= 0)
        prefix = text.left(whiteSpace);
    if (!prefix.isEmpty()) {
        prefix = prefix.toLower();
        foreach (IQuickOpenFilter *filter, filters) {
            if (prefix == filter->shortcutString()) {
                searchText = text.mid(whiteSpace+1);
                return QList<IQuickOpenFilter*>() << filter;
            }
        }
    }
    searchText = text;
    QList<IQuickOpenFilter*> activeFilters;
    foreach (IQuickOpenFilter *filter, filters)
        if (filter->isIncludedByDefault())
            activeFilters << filter;
    return activeFilters;
}

void QuickOpenToolWindow::updateCompletionList(const QString &text)
{
    QString searchText;
    const QList<IQuickOpenFilter*> filters = filtersFor(text, searchText);
    QSet<FilterEntry> alreadyAdded;
    const bool checkDuplicates = (filters.size() > 1);
    QList<FilterEntry> entries;
    foreach (IQuickOpenFilter *filter, filters) {
        foreach (const FilterEntry &entry, filter->matchesFor(searchText)) {
            if (checkDuplicates && alreadyAdded.contains(entry))
                continue;
            entries.append(entry);
            if (checkDuplicates)
                alreadyAdded.insert(entry);
        }
    }
    m_quickOpenModel->setEntries(entries);
    if (m_quickOpenModel->rowCount() > 0) {
        m_completionList->setCurrentIndex(m_quickOpenModel->index(0, 0));
    }
#if 0
    m_completionList->updatePreferredSize();
#endif
}

void QuickOpenToolWindow::acceptCurrentEntry()
{
    if (!m_completionList->isVisible())
        return;
    const QModelIndex index = m_completionList->currentIndex();
    if (!index.isValid())
        return;
    const FilterEntry entry = m_quickOpenModel->data(index, Qt::UserRole).value<FilterEntry>();
    m_completionList->hide();
    entry.filter->accept(entry);
}

void QuickOpenToolWindow::show(const QString &text, int selectionStart, int selectionLength)
{
    m_fileLineEdit->hideHintText();
    m_fileLineEdit->setText(text);
    setFocus();
    if (selectionStart >= 0)
        m_fileLineEdit->setSelection(selectionStart, selectionLength);
    else
        m_fileLineEdit->deselect();
}

void QuickOpenToolWindow::filterSelected()
{
    QString searchText = "<type here>";
    QAction *action = qobject_cast<QAction*>(sender());
    QTC_ASSERT(action, return);
    IQuickOpenFilter *filter = action->data().value<IQuickOpenFilter*>();
    QTC_ASSERT(filter, return);
    QString currentText = m_fileLineEdit->text().trimmed();
    // add shortcut string at front or replace existing shortcut string
    if (!currentText.isEmpty()) {
        searchText = currentText;
        foreach (IQuickOpenFilter *otherfilter, m_quickOpenPlugin->filters()) {
            if (currentText.startsWith(otherfilter->shortcutString() + " ")) {
                searchText = currentText.mid(otherfilter->shortcutString().length()+1);
                break;
            }
        }
    }
    show(filter->shortcutString() + " " + searchText,
         filter->shortcutString().length() + 1,
         searchText.length());
    updateCompletionList(m_fileLineEdit->text());
    m_fileLineEdit->setFocus();
}

void QuickOpenToolWindow::focusInEvent(QFocusEvent *e)
{
    m_fileLineEdit->setFocus(e->reason());
    if (e->reason() != Qt::MouseFocusReason) {
        m_fileLineEdit->selectAll();
    }
    QWidget::focusInEvent(e);
}

void QuickOpenToolWindow::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
}

void QuickOpenToolWindow::showConfigureDialog()
{
    Core::ICore::instance()->showOptionsDialog(Constants::QUICKOPEN_CATEGORY,
          Constants::FILTER_OPTIONS_PAGE);
}
