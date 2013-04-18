/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "treeviewfind.h"

#include <QTreeView>
#include <QTextCursor>
#include <QModelIndex>

namespace Find {

class ItemModelFindPrivate
{
public:
    explicit ItemModelFindPrivate(QTreeView *view, int role)
        : m_view(view)
        , m_incrementalWrappedState(false),
          m_role(role)
    {
    }

    QTreeView *m_view;
    QModelIndex m_incrementalFindStart;
    bool m_incrementalWrappedState;
    int m_role;
};

TreeViewFind::TreeViewFind(QTreeView *view, int role)
    : d(new ItemModelFindPrivate(view, role))
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
    int currentRow = currentIndex.row();

    bool backward = (flags & QTextDocument::FindBackward);
    if (wrapped)
        *wrapped = false;
    bool anyWrapped = false;
    bool stepWrapped = false;
    if (!startFromCurrentIndex)
        index = followingIndex(index, backward, &stepWrapped);
    else
        currentRow = -1;
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
                if (searchExpr.indexIn(text) != -1
                        && d->m_view->model()->flags(index) & Qt::ItemIsSelectable
                        && (index.row() != currentRow || index.parent() != currentIndex.parent()))
                    resultIndex = index;
            } else {
                QTextDocument doc(text);
                if (!doc.find(searchTxt, 0,
                              flags & (Find::FindCaseSensitively |
                                       Find::FindWholeWords)).isNull()
                        && d->m_view->model()->flags(index) & Qt::ItemIsSelectable
                        && (index.row() != currentRow || index.parent() != currentIndex.parent()))
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

    // same parent has more columns, go to next column
    if (idx.column() + 1 < model->columnCount(idx.parent()))
        return model->index(idx.row(), idx.column() + 1, idx.parent());

    // tree views have their children attached to first column
    // make sure we are at first column
    QModelIndex current = model->index(idx.row(), 0, idx.parent());

    // check for children
    if (model->rowCount(current) > 0) {
        return current.child(0, 0);
    }

    // no more children, go up and look for parent with more children
    QModelIndex nextIndex;
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
    QAbstractItemModel *model = d->m_view->model();
    // if same parent has earlier columns, just move there
    if (idx.column() > 0)
        return model->index(idx.row(), idx.column() - 1, idx.parent());

    QModelIndex current = idx;
    bool checkForChildren = true;
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
            current = model->index(rc - 1, 0, current);
        }
    }
    // set to last column
    current = model->index(current.row(), model->columnCount(current.parent()) - 1, current.parent());
    return current;
}

QModelIndex TreeViewFind::followingIndex(const QModelIndex &idx, bool backward, bool *wrapped)
{
    if (backward)
        return prevIndex(idx, wrapped);
    return nextIndex(idx, wrapped);
}

} // namespace Find
