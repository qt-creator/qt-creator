// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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
    ~ItemViewFind() override;

    bool supportsReplace() const override;
    Utils::FindFlags supportedFindFlags() const override;
    void resetIncrementalSearch() override;
    void clearHighlights() override;
    QString currentFindString() const override;
    QString completedFindString() const override;

    void highlightAll(const QString &txt, Utils::FindFlags findFlags) override;
    Result findIncremental(const QString &txt, Utils::FindFlags findFlags) override;
    Result findStep(const QString &txt, Utils::FindFlags findFlags) override;

    static QFrame *createSearchableWrapper(QAbstractItemView *treeView, ColorOption colorOption = DarkColored,
                                           FetchOption option = DoNotFetchMoreWhileSearching);
    static QFrame *createSearchableWrapper(ItemViewFind *finder, ColorOption colorOption = DarkColored);

private:
    Result find(const QString &txt, Utils::FindFlags findFlags,
                bool startFromCurrentIndex, bool *wrapped);
    QModelIndex nextIndex(const QModelIndex &idx, bool *wrapped) const;
    QModelIndex prevIndex(const QModelIndex &idx, bool *wrapped) const;
    QModelIndex followingIndex(const QModelIndex &idx, bool backward,
                               bool *wrapped);

private:
    ItemModelFindPrivate *d;
};

} // namespace Core
