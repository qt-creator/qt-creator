/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
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

    static QFrame *createSearchableWrapper(QAbstractItemView *treeView, ColorOption lightColored = DarkColored,
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
