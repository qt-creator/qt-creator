/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
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

#ifndef ITEMVIEWFIND_H
#define ITEMVIEWFIND_H

#include "ifindsupport.h"

QT_BEGIN_NAMESPACE
class QAbstractItemView;
class QFrame;
class QModelIndex;
QT_END_NAMESPACE

namespace Core {
class ItemModelFindPrivate;

class CORE_EXPORT ItemViewFind : public IFindSupport
{
    Q_OBJECT
public:
    enum FetchOption {
        DoNotFetchMoreWhileSearching,
        FetchMoreWhileSearching
    };

    explicit ItemViewFind(QAbstractItemView *view, int role = Qt::DisplayRole,
            FetchOption option = DoNotFetchMoreWhileSearching);
    virtual ~ItemViewFind();

    bool supportsReplace() const;
    FindFlags supportedFindFlags() const;
    void resetIncrementalSearch();
    void clearHighlights();
    QString currentFindString() const;
    QString completedFindString() const;

    virtual void highlightAll(const QString &txt, FindFlags findFlags);
    Result findIncremental(const QString &txt, FindFlags findFlags);
    Result findStep(const QString &txt, FindFlags findFlags);

    static QFrame *createSearchableWrapper(QAbstractItemView *treeView,
            FetchOption option = DoNotFetchMoreWhileSearching);

private:
    Result find(const QString &txt, FindFlags findFlags,
                bool startFromCurrentIndex, bool *wrapped);
    QModelIndex nextIndex(const QModelIndex &idx, bool *wrapped) const;
    QModelIndex prevIndex(const QModelIndex &idx, bool *wrapped) const;
    QModelIndex followingIndex(const QModelIndex &idx, bool backward,
                               bool *wrapped);

private:
    ItemModelFindPrivate *d;
};

} // namespace Core

#endif // ITEMVIEWFIND_H
