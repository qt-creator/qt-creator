// Copyright (C) 2025 Jarek Kobus
// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QCONDITIONAL_H
#define QCONDITIONAL_H

#include <QtTaskTree/qttasktreeglobal.h>

#include <QtTaskTree/qtasktree.h>

#include <QtCore/QSharedData>

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

class If final
{
public:
    Q_TASKTREE_EXPORT explicit If(const ExecutableItem &condition);
    template <typename Handler,
              std::enable_if_t<!std::is_base_of_v<ExecutableItem, std::decay_t<Handler>>, bool> = true>
    explicit If(Handler &&handler) : If(QSyncTask(std::forward<Handler>(handler))) {}

    Q_TASKTREE_EXPORT ~If();
    Q_TASKTREE_EXPORT If(const If &other);
    Q_TASKTREE_EXPORT If(If &&other) noexcept;
    Q_TASKTREE_EXPORT If &operator=(const If &other);
    QT_MOVE_ASSIGNMENT_OPERATOR_IMPL_VIA_MOVE_AND_SWAP(If)
    void swap(If &other) noexcept { d.swap(other.d); }

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

    Q_TASKTREE_EXPORT ~ElseIf();
    Q_TASKTREE_EXPORT ElseIf(const ElseIf &other);
    Q_TASKTREE_EXPORT ElseIf(ElseIf &&other) noexcept;
    Q_TASKTREE_EXPORT ElseIf &operator=(const ElseIf &other);
    QT_MOVE_ASSIGNMENT_OPERATOR_IMPL_VIA_MOVE_AND_SWAP(ElseIf)
    void swap(ElseIf &other) noexcept { d.swap(other.d); }

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

    Q_TASKTREE_EXPORT ~Else();
    Q_TASKTREE_EXPORT Else(const Else &other);
    Q_TASKTREE_EXPORT Else(Else &&other) noexcept;
    Q_TASKTREE_EXPORT Else &operator=(const Else &other);
    QT_MOVE_ASSIGNMENT_OPERATOR_IMPL_VIA_MOVE_AND_SWAP(Else)
    void swap(Else &other) noexcept { d.swap(other.d); }

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

    Q_TASKTREE_EXPORT ~Then();
    Q_TASKTREE_EXPORT Then(const Then &other);
    Q_TASKTREE_EXPORT Then(Then &&other) noexcept;
    Q_TASKTREE_EXPORT Then &operator=(const Then &other);
    QT_MOVE_ASSIGNMENT_OPERATOR_IMPL_VIA_MOVE_AND_SWAP(Then)
    void swap(Then &other) noexcept { d.swap(other.d); }

private:
    friend class ThenItem;
    Group body() const;
    QExplicitlySharedDataPointer<ThenPrivate> d;
};

class ThenItem final
{
public:
    Q_TASKTREE_EXPORT operator ExecutableItem() const;
    Q_TASKTREE_EXPORT ~ThenItem();
    Q_TASKTREE_EXPORT ThenItem(const ThenItem &other);
    Q_TASKTREE_EXPORT ThenItem(ThenItem &&other) noexcept;
    Q_TASKTREE_EXPORT ThenItem &operator=(const ThenItem &other);
    QT_MOVE_ASSIGNMENT_OPERATOR_IMPL_VIA_MOVE_AND_SWAP(ThenItem)
    void swap(ThenItem &other) noexcept { d.swap(other.d); }

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
    Q_TASKTREE_EXPORT ~ElseItem();
    Q_TASKTREE_EXPORT ElseItem(const ElseItem &other);
    Q_TASKTREE_EXPORT ElseItem(ElseItem &&other) noexcept;
    Q_TASKTREE_EXPORT ElseItem &operator=(const ElseItem &other);
    QT_MOVE_ASSIGNMENT_OPERATOR_IMPL_VIA_MOVE_AND_SWAP(ElseItem)
    void swap(ElseItem &other) noexcept { d.swap(other.d); }

private:
    ElseItem(const ThenItem &thenItem, const Else &elseItem);

    Q_TASKTREE_EXPORT friend ElseItem operator>>(const ThenItem &thenItem, const Else &elseItem);

    QList<ConditionData> conditions() const;
    QExplicitlySharedDataPointer<ElseItemPrivate> d;
};

class ElseIfItem final
{
public:
    Q_TASKTREE_EXPORT ~ElseIfItem();
    Q_TASKTREE_EXPORT ElseIfItem(const ElseIfItem &other);
    Q_TASKTREE_EXPORT ElseIfItem(ElseIfItem &&other) noexcept;
    Q_TASKTREE_EXPORT ElseIfItem &operator=(const ElseIfItem &other);
    QT_MOVE_ASSIGNMENT_OPERATOR_IMPL_VIA_MOVE_AND_SWAP(ElseIfItem)
    void swap(ElseIfItem &other) noexcept { d.swap(other.d); }

private:
    ElseIfItem(const ThenItem &thenItem, const ElseIf &elseIfItem);

    Q_TASKTREE_EXPORT friend ThenItem operator>>(const ElseIfItem &elseIfItem, const Then &thenItem);
    Q_TASKTREE_EXPORT friend ElseIfItem operator>>(const ThenItem &thenItem, const ElseIf &elseIfItem);

    friend class ThenItem;
    QList<ConditionData> conditions() const;
    ExecutableItem nextCondition() const;
    QExplicitlySharedDataPointer<ElseIfItemPrivate> d;
};

Q_TASKTREE_EXPORT ThenItem operator>>(const If &ifItem, const Then &thenItem);
Q_TASKTREE_EXPORT ElseIfItem operator>>(const ThenItem &thenItem, const ElseIf &elseIfItem);
Q_TASKTREE_EXPORT ElseItem operator>>(const ThenItem &thenItem, const Else &elseItem);
Q_TASKTREE_EXPORT ThenItem operator>>(const ElseIfItem &elseIfItem, const Then &thenItem);

} // namespace QtTaskTree

QT_END_NAMESPACE

#endif // QCONDITIONAL_H
