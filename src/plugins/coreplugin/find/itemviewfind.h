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

#pragma once

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

    enum ColorOption {
        DarkColored = 0,
        LightColored = 1
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

    static QFrame *createSearchableWrapper(QAbstractItemView *treeView, ColorOption colorOption = DarkColored,
                                           FetchOption option = DoNotFetchMoreWhileSearching);
    static QFrame *createSearchableWrapper(ItemViewFind *finder, ColorOption colorOption = DarkColored);

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
