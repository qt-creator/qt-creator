/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
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
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "treeviewfind.h"

#include <QTreeView>
#include <QTextCursor>

namespace Find {

struct ItemModelFindPrivate
{
    explicit ItemModelFindPrivate(QTreeView *view, int role, int column)
        : m_view(view)
        , m_incrementalWrappedState(false),
          m_role(role),
          m_column(column)
    {
    }

    QTreeView *m_view;
    QModelIndex m_incrementalFindStart;
    bool m_incrementalWrappedState;
    int m_role;
    int m_column;
};

TreeViewFind::TreeViewFind(QTreeView *view, int role, int column)
    : d(new ItemModelFindPrivate(view, role, column))
{
}

TreeViewFind::~TreeViewFind()
{
    delete d;
}

bool TreeViewFind::supportsReplace() const
{
    return false;
}

Find::FindFlags TreeViewFind::supportedFindFlags() const
{
    return Find::FindBackward | Find::FindCaseSensitively
            | Find::FindRegularExpression | Find::FindWholeWords;
}

void TreeViewFind::resetIncrementalSearch()
{
    d->m_incrementalFindStart = QModelIndex();
    d->m_incrementalWrappedState = false;
}

void TreeViewFind::clearResults()
{
}

QString TreeViewFind::currentFindString() const
{
    return QString();
}

QString TreeViewFind::completedFindString() const
{
    return QString();
}

void TreeViewFind::highlightAll(const QString &/*txt*/, FindFlags /*findFlags*/)
{
}

IFindSupport::Result TreeViewFind::findIncremental(const QString &txt,
                                                    Find::FindFlags findFlags)
{
    if (!d->m_incrementalFindStart.isValid()) {
        d->m_incrementalFindStart = d->m_view->currentIndex();
        d->m_incrementalWrappedState = false;
    }
    d->m_view->setCurrentIndex(d->m_incrementalFindStart);
    bool wrapped = false;
    IFindSupport::Result result = find(txt, findFlags, true/*startFromCurrent*/,
                                       &wrapped);
    if (wrapped != d->m_incrementalWrappedState) {
        d->m_incrementalWrappedState = wrapped;
        showWrapIndicator(d->m_view);
    }
    return result;
}

IFindSupport::Result TreeViewFind::findStep(const QString &txt,
                                             Find::FindFlags findFlags)
{
    bool wrapped = false;
    IFindSupport::Result result = find(txt, findFlags, false/*startFromNext*/,
                                       &wrapped);
    if (wrapped)
        showWrapIndicator(d->m_view);
    if (result == IFindSupport::Found) {
        d->m_incrementalFindStart = d->m_view->currentIndex();
        d->m_incrementalWrappedState = false;
    }
    return result;
}

void TreeViewFind::replace(const QString &/*before*/, const QString &/*after*/,
                           Find::FindFlags /*findFlags*/)
{
}

bool TreeViewFind::replaceStep(const QString &/*before*/,
                               const QString &/*after*/,
                               Find::FindFlags /*findFlags*/)
{
    return false;
}

int TreeViewFind::replaceAll(const QString &/*before*/,
                             const QString &/*after*/,
                             Find::FindFlags /*findFlags*/)
{
    return 0;
}

IFindSupport::Result TreeViewFind::find(const QString &searchTxt,
                                        Find::FindFlags findFlags,
                                        bool startFromCurrentIndex,
                                        bool *wrapped)
{
    if (wrapped)
        *wrapped = false;
    if (searchTxt.isEmpty())
        return IFindSupport::NotFound;

    QTextDocument::FindFlags flags =
            Find::textDocumentFlagsForFindFlags(findFlags);
    QModelIndex resultIndex;
    QModelIndex currentIndex = d->m_view->currentIndex();
    QModelIndex index = currentIndex;
    bool backward = (flags & QTextDocument::FindBackward);
    if (wrapped)
        *wrapped = false;
    bool anyWrapped = false;
    bool stepWrapped = false;

    if (!startFromCurrentIndex)
        index = followingIndex(index, backward, &stepWrapped);
    do {
        anyWrapped |= stepWrapped; // update wrapped state if we actually stepped to next/prev item
        if (index.isValid()) {
            const QString &text = d->m_view->model()->data(
                        index, d->m_role).toString();
            if (findFlags & Find::FindRegularExpression) {
                bool sensitive = (findFlags & Find::FindCaseSensitively);
                QRegExp searchExpr = QRegExp(searchTxt,
                                             (sensitive ? Qt::CaseSensitive :
                                                          Qt::CaseInsensitive));
                if (searchExpr.indexIn(text) != -1)
                    resultIndex = index;
            } else {
                QTextDocument doc(text);
                if (!doc.find(searchTxt, 0, flags).isNull())
                    resultIndex = index;
            }
        }
        index = followingIndex(index, backward, &stepWrapped);
    } while (!resultIndex.isValid() && index.isValid() && index != currentIndex);

    if (resultIndex.isValid()) {
        d->m_view->setCurrentIndex(resultIndex);
        d->m_view->scrollTo(resultIndex);
        if (resultIndex.parent().isValid())
            d->m_view->expand(resultIndex.parent());
        if (wrapped)
            *wrapped = anyWrapped;
        return IFindSupport::Found;
    }
    return IFindSupport::NotFound;
}

QModelIndex TreeViewFind::nextIndex(const QModelIndex &idx, bool *wrapped) const
{
    if (wrapped)
        *wrapped = false;
    QAbstractItemModel *model = d->m_view->model();
    // pathological
    if (!idx.isValid())
        return model->index(0, 0);


    if (model->rowCount(idx) > 0) {
        // node with children
        return idx.child(0, 0);
    }
    // leaf node
    QModelIndex nextIndex;
    QModelIndex current = idx;
    while (!nextIndex.isValid()) {
        int row = current.row();
        current = current.parent();
        if (row + 1 < model->rowCount(current)) {
            // Same parent has another child
            nextIndex = model->index(row + 1, 0, current);
        } else {
            // go up one parent
            if (!current.isValid()) {
                // we start from the beginning
                if (wrapped)
                    *wrapped = true;
                nextIndex = model->index(0, 0);
            }
        }
    }
    return nextIndex;
}

QModelIndex TreeViewFind::prevIndex(const QModelIndex &idx, bool *wrapped) const
{
    if (wrapped)
        *wrapped = false;
    QModelIndex current = idx;
    bool checkForChildren = true;
    QAbstractItemModel *model = d->m_view->model();
    if (current.isValid()) {
        int row = current.row();
        if (row > 0) {
            current = model->index(row - 1, 0, current.parent());
        } else {
            current = current.parent();
            checkForChildren = !current.isValid();
            if (checkForChildren && wrapped) {
                // we start from the end
                *wrapped = true;
            }
        }
    }
    if (checkForChildren) {
        // traverse down the hierarchy
        while (int rc = model->rowCount(current)) {
            current = model->index(rc - 1, d->m_column, current);
        }
    }
    return current;
}

QModelIndex TreeViewFind::followingIndex(const QModelIndex &idx, bool backward, bool *wrapped)
{
    if (backward)
        return prevIndex(idx, wrapped);
    return nextIndex(idx, wrapped);
}

} // namespace Find
