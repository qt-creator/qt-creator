// Copyright (C) 2025 Jarek Kobus
// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default


#ifndef QTASKTREE_QCONDITIONAL_H
#define QTASKTREE_QCONDITIONAL_H

#include <QtTaskTree/qttasktreeglobal.h>

#include <QtTaskTree/qtasktree.h>

#include <QtCore/QSharedData>

#if 0
#pragma qt_class(QConditional)
#endif

QT_BEGIN_NAMESPACE

namespace QtTaskTree {

class ConditionData;
class ElseIfItem;
class ElseItem;
class Then;
class ThenItem;

class ElsePrivate;
class ElseIfPrivate;
class IfPrivate;
class ThenPrivate;

class ElseIfItemPrivate;
class ElseItemPrivate;
class ThenItemPrivate;

}

QT_DECLARE_QESDP_SPECIALIZATION_DTOR(QtTaskTree::ElsePrivate)
QT_DECLARE_QESDP_SPECIALIZATION_DTOR(QtTaskTree::ElseIfPrivate)
QT_DECLARE_QESDP_SPECIALIZATION_DTOR(QtTaskTree::ElseIfItemPrivate)
QT_DECLARE_QESDP_SPECIALIZATION_DTOR(QtTaskTree::ElseItemPrivate)
QT_DECLARE_QESDP_SPECIALIZATION_DTOR(QtTaskTree::IfPrivate)
QT_DECLARE_QESDP_SPECIALIZATION_DTOR(QtTaskTree::ThenPrivate)
QT_DECLARE_QESDP_SPECIALIZATION_DTOR(QtTaskTree::ThenItemPrivate)

namespace QtTaskTree {

class If final
{
public:
    Q_TASKTREE_EXPORT explicit If(const ExecutableItem &condition);
    template <typename Handler,
              std::enable_if_t<!std::is_base_of_v<ExecutableItem, std::decay_t<Handler>>, bool> = true>
    explicit If(Handler &&handler) : If(QSyncTask(std::forward<Handler>(handler))) {}

    QT_TASKTREE_DECLARE_SMFS(If, Q_TASKTREE_EXPORT)

private:
    Q_TASKTREE_EXPORT friend ThenItem operator>>(const If &ifItem, const Then &thenItem);

    friend class ThenItem;
    ExecutableItem condition() const;
    QExplicitlySharedDataPointer<IfPrivate> d;
};

class ElseIf final
{
public:
    Q_TASKTREE_EXPORT explicit ElseIf(const ExecutableItem &condition);
    template <typename Handler,
             std::enable_if_t<!std::is_base_of_v<ExecutableItem, std::decay_t<Handler>>, bool> = true>
    explicit ElseIf(Handler &&handler) : ElseIf(QSyncTask(std::forward<Handler>(handler))) {}

    QT_TASKTREE_DECLARE_SMFS(ElseIf, Q_TASKTREE_EXPORT)

private:
    friend class ElseIfItem;
    ExecutableItem condition() const;
    QExplicitlySharedDataPointer<ElseIfPrivate> d;
};

class Else final
{
public:
    Q_TASKTREE_EXPORT explicit Else(const GroupItems &children);
    Q_TASKTREE_EXPORT explicit Else(std::initializer_list<GroupItem> children);

    QT_TASKTREE_DECLARE_SMFS(Else, Q_TASKTREE_EXPORT)

private:
    friend class ElseItem;
    Group body() const;
    QExplicitlySharedDataPointer<ElsePrivate> d;
};

class Then final
{
public:
    Q_TASKTREE_EXPORT explicit Then(const GroupItems &children);
    Q_TASKTREE_EXPORT explicit Then(std::initializer_list<GroupItem> children);

    QT_TASKTREE_DECLARE_SMFS(Then, Q_TASKTREE_EXPORT)

private:
    friend class ThenItem;
    Group body() const;
    QExplicitlySharedDataPointer<ThenPrivate> d;
};

class ThenItem final
{
public:
    Q_TASKTREE_EXPORT operator ExecutableItem() const;

    QT_TASKTREE_DECLARE_SMFS(ThenItem, Q_TASKTREE_EXPORT)

private:
    ThenItem(const If &ifItem, const Then &thenItem);
    ThenItem(const ElseIfItem &elseIfItem, const Then &thenItem);

    Q_TASKTREE_EXPORT friend ElseItem operator>>(const ThenItem &thenItem, const Else &elseItem);
    Q_TASKTREE_EXPORT friend ElseIfItem operator>>(const ThenItem &thenItem, const ElseIf &elseIfItem);
    Q_TASKTREE_EXPORT friend ThenItem operator>>(const If &ifItem, const Then &thenItem);
    Q_TASKTREE_EXPORT friend ThenItem operator>>(const ElseIfItem &elseIfItem, const Then &thenItem);

    friend class ElseItem;
    friend class ElseIfItem;
    QList<ConditionData> conditions() const;
    QExplicitlySharedDataPointer<ThenItemPrivate> d;
};

class ElseItem final
{
public:
    Q_TASKTREE_EXPORT operator ExecutableItem() const;

    QT_TASKTREE_DECLARE_SMFS(ElseItem, Q_TASKTREE_EXPORT)

private:
    ElseItem(const ThenItem &thenItem, const Else &elseItem);

    Q_TASKTREE_EXPORT friend ElseItem operator>>(const ThenItem &thenItem, const Else &elseItem);

    QList<ConditionData> conditions() const;
    QExplicitlySharedDataPointer<ElseItemPrivate> d;
};

class ElseIfItem final
{
public:
    QT_TASKTREE_DECLARE_SMFS(ElseIfItem, Q_TASKTREE_EXPORT)

private:
    ElseIfItem(const ThenItem &thenItem, const ElseIf &elseIfItem);

    Q_TASKTREE_EXPORT friend ThenItem operator>>(const ElseIfItem &elseIfItem, const Then &thenItem);
    Q_TASKTREE_EXPORT friend ElseIfItem operator>>(const ThenItem &thenItem, const ElseIf &elseIfItem);

    friend class ThenItem;
    QList<ConditionData> conditions() const;
    ExecutableItem nextCondition() const;
    QExplicitlySharedDataPointer<ElseIfItemPrivate> d;
};

} // namespace QtTaskTree

QT_END_NAMESPACE

#endif // QTASKTREE_QCONDITIONAL_H
