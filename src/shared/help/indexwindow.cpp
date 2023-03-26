// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "indexwindow.h"

#include "topicchooser.h"

#include <helptr.h>
#include <helpviewer.h>
#include <localhelpmanager.h>
#include <openpagesmanager.h>

#include <utils/fancylineedit.h>
#include <utils/hostosinfo.h>
#include <utils/navigationtreeview.h>
#include <utils/qtcassert.h>
#include <utils/styledbar.h>

#include <QAbstractItemModel>
#include <QLayout>
#include <QLabel>
#include <QLineEdit>
#include <QKeyEvent>
#include <QMenu>
#include <QContextMenuEvent>

#include <QHelpEngine>
#include <QHelpIndexModel>

#include <QHelpLink>

using namespace Help::Internal;

IndexWindow::IndexWindow()
    : m_searchLineEdit(0)
    , m_indexWidget(0)
    , m_isOpenInNewPageActionVisible(true)
{
    QVBoxLayout *layout = new QVBoxLayout(this);

    m_searchLineEdit = new Utils::FancyLineEdit();
    m_searchLineEdit->setPlaceholderText(QString());
    m_searchLineEdit->setFiltering(true);
    setFocusProxy(m_searchLineEdit);
    connect(m_searchLineEdit, &QLineEdit::textChanged,
            this, &IndexWindow::filterIndices);
    m_searchLineEdit->installEventFilter(this);
    m_searchLineEdit->setAttribute(Qt::WA_MacShowFocusRect, false);

    QLabel *l = new QLabel(Tr::tr("&Look for:"));
    l->setBuddy(m_searchLineEdit);
    layout->addWidget(l);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    Utils::StyledBar *toolbar = new Utils::StyledBar(this);
    toolbar->setSingleRow(false);
    QLayout *tbLayout = new QHBoxLayout();
    tbLayout->setSpacing(6);
    tbLayout->setContentsMargins(4, 4, 4, 4);
    tbLayout->addWidget(l);
    tbLayout->addWidget(m_searchLineEdit);
    toolbar->setLayout(tbLayout);
    layout->addWidget(toolbar);

    QHelpIndexModel *indexModel = LocalHelpManager::helpEngine().indexModel();
    m_filteredIndexModel = new IndexFilterModel(this);
    m_filteredIndexModel->setSourceModel(indexModel);
    m_indexWidget = new Utils::NavigationTreeView(this);
    m_indexWidget->setModel(m_filteredIndexModel);
    m_indexWidget->setRootIsDecorated(false);
    m_indexWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_indexWidget->installEventFilter(this);
    connect(indexModel, &QHelpIndexModel::indexCreationStarted,
            this, &IndexWindow::disableSearchLineEdit);
    connect(indexModel, &QHelpIndexModel::indexCreated,
            this, &IndexWindow::enableSearchLineEdit);
    connect(m_indexWidget, &Utils::NavigationTreeView::activated,
            this, [this](const QModelIndex &index) { open(index); });
    connect(m_searchLineEdit, &QLineEdit::returnPressed,
            m_indexWidget, [this]() { open(m_indexWidget->currentIndex()); });
    layout->addWidget(m_indexWidget);

    m_indexWidget->viewport()->installEventFilter(this);
}

IndexWindow::~IndexWindow()
{
}

void IndexWindow::setOpenInNewPageActionVisible(bool visible)
{
    m_isOpenInNewPageActionVisible = visible;
}

void IndexWindow::filterIndices(const QString &filter)
{
    const QString wildcard = filter.contains(QLatin1Char('*')) ? filter : QString();
    const QModelIndex bestMatch = m_filteredIndexModel->filter(filter, wildcard);
    if (!bestMatch.isValid())
        return;
    m_indexWidget->setCurrentIndex(bestMatch);
    m_indexWidget->scrollTo(bestMatch);
}

bool IndexWindow::eventFilter(QObject *obj, QEvent *e)
{
    if (obj == m_searchLineEdit && e->type() == QEvent::KeyPress) {
        QKeyEvent *ke = static_cast<QKeyEvent*>(e);
        QModelIndex idx = m_indexWidget->currentIndex();
        switch (ke->key()) {
        case Qt::Key_Up:
            idx = m_indexWidget->model()->index(idx.row()-1,
                idx.column(), idx.parent());
            if (idx.isValid())
                m_indexWidget->setCurrentIndex(idx);
            break;
        case Qt::Key_Down:
            idx = m_indexWidget->model()->index(idx.row()+1,
                idx.column(), idx.parent());
            if (idx.isValid())
                m_indexWidget->setCurrentIndex(idx);
            break;
        default: ; // stop complaining
        }
    } else if (obj == m_searchLineEdit
            && e->type() == QEvent::FocusIn
            && static_cast<QFocusEvent *>(e)->reason() != Qt::MouseFocusReason) {
        m_searchLineEdit->selectAll();
        m_searchLineEdit->setFocus();
    } else if (obj == m_indexWidget && e->type() == QEvent::ContextMenu) {
        QContextMenuEvent *ctxtEvent = static_cast<QContextMenuEvent*>(e);
        QModelIndex idx = m_indexWidget->indexAt(ctxtEvent->pos());
        if (idx.isValid()) {
            QMenu menu;
            QAction *curTab = menu.addAction(Tr::tr("Open Link"));
            QAction *newTab = 0;
            if (m_isOpenInNewPageActionVisible)
                newTab = menu.addAction(Tr::tr("Open Link as New Page"));
            menu.move(m_indexWidget->mapToGlobal(ctxtEvent->pos()));

            QAction *action = menu.exec();
            if (curTab == action)
                open(idx);
            else if (newTab && newTab == action)
                open(idx, true/*newPage*/);
        }
    } else if (m_indexWidget && obj == m_indexWidget->viewport()
        && e->type() == QEvent::MouseButtonRelease) {
        QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(e);
        QModelIndex idx = m_indexWidget->indexAt(mouseEvent->pos());
        if (idx.isValid()) {
            Qt::MouseButtons button = mouseEvent->button();
            if (((button == Qt::LeftButton) && (mouseEvent->modifiers() & Qt::ControlModifier))
                || (button == Qt::MiddleButton)) {
                open(idx);
            }
        }
    }

    return QWidget::eventFilter(obj, e);
}

void IndexWindow::enableSearchLineEdit()
{
    m_searchLineEdit->setDisabled(false);
    filterIndices(m_searchLineEdit->text());
}

void IndexWindow::disableSearchLineEdit()
{
    m_searchLineEdit->setDisabled(true);
}

void IndexWindow::open(const QModelIndex &index, bool newPage)
{
    const QString keyword = m_filteredIndexModel->data(index, Qt::DisplayRole).toString();
    const QMultiMap<QString, QUrl> links = LocalHelpManager::linksForKeyword(keyword);

    emit linksActivated(links, keyword, newPage);
}

Qt::DropActions IndexFilterModel::supportedDragActions() const
{
    if (!sourceModel())
        return Qt::IgnoreAction;
    return sourceModel()->supportedDragActions();
}

QModelIndex IndexFilterModel::index(int row, int column, const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    QTC_ASSERT(row < m_toSource.size(), return {});
    return createIndex(row, column);
}

QModelIndex IndexFilterModel::parent(const QModelIndex &child) const
{
    Q_UNUSED(child)
    return QModelIndex();
}

int IndexFilterModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) // our items don't have children
        return 0;
    return m_toSource.size();
}

int IndexFilterModel::columnCount(const QModelIndex &parent) const
{
    if (!sourceModel())
        return 0;
    return sourceModel()->columnCount(mapToSource(parent));
}

QVariant IndexFilterModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    // we don't show header
    Q_UNUSED(section)
    Q_UNUSED(orientation)
    Q_UNUSED(role)
    return {};
}

bool IndexFilterModel::hasChildren(const QModelIndex &parent) const
{
    if (parent.isValid()) // our items don't have children
        return false;
    return m_toSource.count();
}

void IndexFilterModel::setSourceModel(QAbstractItemModel *sm)
{
    QAbstractItemModel *previousModel = sourceModel();
    if (previousModel) {
        disconnect(previousModel, &QAbstractItemModel::dataChanged,
                   this, &IndexFilterModel::sourceDataChanged);
        disconnect(previousModel, &QAbstractItemModel::rowsInserted,
                   this, &IndexFilterModel::sourceRowsInserted);
        disconnect(previousModel, &QAbstractItemModel::rowsRemoved,
                   this, &IndexFilterModel::sourceRowsRemoved);
        disconnect(previousModel, &QAbstractItemModel::modelReset,
                   this, &IndexFilterModel::sourceModelReset);
    }
    QAbstractProxyModel::setSourceModel(sm);
    if (sm) {
        connect(sm, &QAbstractItemModel::dataChanged,
                this, &IndexFilterModel::sourceDataChanged);
        connect(sm, &QAbstractItemModel::rowsInserted,
                this, &IndexFilterModel::sourceRowsInserted);
        connect(sm, &QAbstractItemModel::rowsRemoved,
                this, &IndexFilterModel::sourceRowsRemoved);
        connect(sm, &QAbstractItemModel::modelReset,
                this, &IndexFilterModel::sourceModelReset);
    }
    filter(m_filter, m_wildcard);
}

QModelIndex IndexFilterModel::sibling(int row, int column, const QModelIndex &idx) const
{
    return QAbstractItemModel::sibling(row, column, idx);
}

Qt::ItemFlags IndexFilterModel::flags(const QModelIndex &index) const
{
    Q_UNUSED(index)
    return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

IndexFilterModel::IndexFilterModel(QObject *parent)
    : QAbstractProxyModel(parent)
{
}

QModelIndex IndexFilterModel::filter(const QString &filter, const QString &wildcard)
{
    beginResetModel();

    m_filter = filter;
    m_wildcard = wildcard;
    m_toSource.clear();

    if (!sourceModel()) {
        endResetModel();
        return {};
    }

    // adapted copy from QHelpIndexModel

    if (filter.isEmpty() && wildcard.isEmpty()) {
        const int count = sourceModel()->rowCount();
        m_toSource.reserve(count);
        for (int i = 0; i < count; ++i)
            m_toSource.append(i);
        endResetModel();
        if (m_toSource.isEmpty())
            return {};
        return index(0, 0);
    }

    const QStringList indices = static_cast<QHelpIndexModel *>(sourceModel())->stringList();

    const auto match = [this, &indices, &filter] (std::function<bool(const QString &)> matcher) {
        int goodMatch = -1;
        int perfectMatch = -1;
        int i = 0;
        for (const QString &index : indices) {
            if (matcher(index)) {
                m_toSource.append(i);
                if (perfectMatch == -1 && index.startsWith(filter, Qt::CaseInsensitive)) {
                    if (goodMatch == -1)
                        goodMatch = m_toSource.size() - 1;
                    if (filter.length() == index.length()){
                        perfectMatch = m_toSource.size() - 1;
                    }
                } else if (perfectMatch > -1 && index == filter) {
                    perfectMatch = m_toSource.size() - 1;
                }
            }
            ++i;
        }
        return perfectMatch >= 0 ? perfectMatch : goodMatch;
    };

    const auto matchSimpleOrRegExp = [&] () {
        if (wildcard.isEmpty()) {
            return match([&filter] (const QString &index) {
                return index.contains(filter, Qt::CaseInsensitive);
            });
        }
        const QRegularExpression regExp(QRegularExpression::wildcardToRegularExpression(wildcard),
                                        QRegularExpression::CaseInsensitiveOption);
        return match([&regExp] (const QString &index) {
            return index.contains(regExp);
        });
    };
    const int matchedIndex = matchSimpleOrRegExp();

    endResetModel();
    if (matchedIndex == -1)
        return {};
    return index(matchedIndex, 0);
}

QModelIndex IndexFilterModel::mapToSource(const QModelIndex &proxyIndex) const
{
    if (!sourceModel() || !proxyIndex.isValid()
            || proxyIndex.parent().isValid() || proxyIndex.row() >= m_toSource.size()) {
        return {};
    }
    return sourceModel()->index(m_toSource.at(proxyIndex.row()), proxyIndex.column());
}

QModelIndex IndexFilterModel::mapFromSource(const QModelIndex &sourceIndex) const
{
    if (!sourceIndex.isValid() || sourceIndex.parent().isValid())
        return {};
    const int i = m_toSource.indexOf(sourceIndex.row());
    if (i < 0)
        return {};
    return index(i, sourceIndex.column());
}

void IndexFilterModel::sourceDataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight)
{
    QModelIndex topLeftIndex = mapFromSource(topLeft);
    if (!topLeftIndex.isValid())
        topLeftIndex = index(0, topLeft.column());
    QModelIndex bottomRightIndex = mapFromSource(bottomRight);
    if (!bottomRightIndex.isValid())
        bottomRightIndex = index(0, bottomRight.column());
    emit dataChanged(topLeftIndex, bottomRightIndex);
}

void IndexFilterModel::sourceRowsRemoved(const QModelIndex &parent, int start, int end)
{
    Q_UNUSED(parent)
    Q_UNUSED(start)
    Q_UNUSED(end)
    filter(m_filter, m_wildcard);
}

void IndexFilterModel::sourceRowsInserted(const QModelIndex &parent, int start, int end)
{
    Q_UNUSED(parent)
    Q_UNUSED(start)
    Q_UNUSED(end)
    filter(m_filter, m_wildcard);
}
void IndexFilterModel::sourceModelReset()
{
    filter(m_filter, m_wildcard);
}
