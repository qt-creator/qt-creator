// Copyright (C) 2025 Jarek Kobus
// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <QtTaskTree/qconditional.h>

QT_BEGIN_NAMESPACE

namespace QtTaskTree {

/*!
    \class QtTaskTree::If
    \inheaderfile qconditional.h
    \inmodule TaskTree
    \brief An "if" element used in conditional expressions.
    \reentrant

    An initial condition element of the conditional expressions.
    Must always be followed by \l Then element.

    The conditional expressions in the TaskTree module enable processing of
    branch conditions and their bodies in an asynchronous way. For example:

    \code
        const Group recipe {
            If (conditionTask1) >> Then {
                bodyTask1
            } >> ElseIf (conditionTask2) >> Then {
                bodyTask2
            } >> Else {
                bodyTask3
            }
        };
    \endcode

    When the above recipe is run with QTaskTree, the task tree starts the
    asynchronous \c conditionTask1 first. After it's finished,
    depending on the result, the task tree may either execute the
    \c bodyTask1 when the \c conditionTask1 finished with success,
    or dispatch another condition by executing \c conditionTask2 otherwise.
    In case the \c bodyTask1 is executed, no other tasks are executed,
    and the result of \c bodyTask1 is the final result of the whole
    conditional expression.

    \sa Then, Else, ElseIf
*/

class IfPrivate : public QSharedData
{
public:
    IfPrivate(const ExecutableItem &condition) : m_condition(condition) {}
    ExecutableItem m_condition;
};

If::~If() = default;
If::If(const If &other) = default;
If::If(If &&other) noexcept = default;
If &If::operator=(const If &other) = default;

/*!
    \fn template <typename Handler, std::enable_if_t<!std::is_base_of_v<ExecutableItem, std::decay_t<Handler>>, bool> = true> If::If(Handler &&handler)
    \overload

    A helper constructor accepting the synchronous \a handler to be executed
    when evaluating the initial condition by the running QTaskTree.

    It's a shortcut for:
    \code
        If (QSyncTask(handler))
    \endcode

    See \l QSyncTask for more information on what handler types are acceptable.

    \sa QSyncTask, ElseIf
*/

/*!
    Creates an initial condition element with \a condition task to be used in
    conditional expression. The running QTaskTree executes the passed
    \a condition first, and after it's finished, either the \l Then branch
    is executed (on success), or optionally provided \l Else branch
    or \l ElseIf condition otherwise.

    The passed \a condition may consist of multiple tasks enclosed
    in \l Group element, or be a conjunction or disjunction of them, like:

    \code
        const Group subRecipe {
            parallel,
            parallelConditionTask1,
            parallelConditionTask2
        };

        const Group recipe {
            If (conditionTask1 && !conditionTask2) >> Then {
                bodyTask1
            } >> ElseIf (subRecipe) >> Then {
                bodyTask2
            } >> Else {
                bodyTask3
            }
        };
    \endcode
*/
If::If(const ExecutableItem &condition) : d(new IfPrivate{condition}) {}

ExecutableItem If::condition() const { return d->m_condition; }

/*!
    \class QtTaskTree::ElseIf
    \inheaderfile qconditional.h
    \inmodule TaskTree
    \brief An "else if" element used in conditional expressions.
    \reentrant

    An alternative condition element of the conditional expressions.
    Must always be preceeded and followed by \l Then element.

    \sa If, Then, Else
*/

class ElseIfPrivate : public QSharedData
{
public:
    ElseIfPrivate(const ExecutableItem &condition) : m_condition(condition) {}
    ExecutableItem m_condition;
};

ElseIf::~ElseIf() = default;
ElseIf::ElseIf(const ElseIf &other) = default;
ElseIf::ElseIf(ElseIf &&other) noexcept = default;
ElseIf &ElseIf::operator=(const ElseIf &other) = default;

/*!
    \fn template <typename Handler, std::enable_if_t<!std::is_base_of_v<ExecutableItem, std::decay_t<Handler>>, bool> = true> ElseIf::ElseIf(Handler &&handler)
    \overload

    A helper constructor accepting the synchronous \a handler to be executed
    when evaluating the initial condition by the running QTaskTree.

    It's a shortcut for:
    \code
        ElseIf (QSyncTask(handler))
    \endcode

    See \l QSyncTask for more information on what handler types are acceptable.

    \sa QSyncTask, If
*/

/*!
    Creates an alternative condition element with \a condition task
    to be used in conditional expression.

    \sa If
*/
ElseIf::ElseIf(const ExecutableItem &condition) : d(new ElseIfPrivate{condition}) {}

ExecutableItem ElseIf::condition() const { return d->m_condition; }

/*!
    \class QtTaskTree::Else
    \inheaderfile qconditional.h
    \inmodule TaskTree
    \brief An "else" element used in conditional expressions.
    \reentrant

    A body of an else branch of the conditional expressions.
    Must always be preceeded by \l Then element.

    \sa If, Then, ElseIf
*/

class ElsePrivate : public QSharedData
{
public:
    ElsePrivate(const Group &body) : m_body(body) {}
    Group m_body;
};

Else::~Else() = default;
Else::Else(const Else &other) = default;
Else::Else(Else &&other) noexcept = default;
Else &Else::operator=(const Else &other) = default;

/*!
    Creates an else branch body element of the conditional expressions
    with \a children items. If the previous condition finishes with an error,
    the preceeding \l Then body is skipped, and \c this branch is selected.
    In this case the running QTaskTree executes the \a children,
    and the result of this execution is the final result of the whole
    conditional expression.
*/
Else::Else(const GroupItems &children) : d(new ElsePrivate{{children}}) {}

/*!
    \overload

    Constructs an else branch body element from \c std::initializer_list
    given by \a children.
*/
Else::Else(std::initializer_list<GroupItem> children) : d(new ElsePrivate{{children}}) {}

Group Else::body() const { return d->m_body; }

/*!
    \class QtTaskTree::Then
    \inheaderfile qconditional.h
    \inmodule TaskTree
    \brief A "then" element used in conditional expressions.
    \reentrant

    A branch body element of the conditional expressions.
    Must always be preceeded by \l If or \l ElseIf element.
    May be followed by \l Else or \l ElseIf element.

    \sa If, Else, ElseIf
*/

class ThenPrivate : public QSharedData
{
public:
    ThenPrivate(const Group &body) : m_body(body) {}
    Group m_body;
};

Then::~Then() = default;
Then::Then(const Then &other) = default;
Then::Then(Then &&other) noexcept = default;
Then &Then::operator=(const Then &other) = default;

/*!
    Creates a branch body element with \a children items, to be used in
    conditional expression. If the preceeding \l If or \l ElseIf element
    finishes with success, the running QTaskTree executes the \a children,
    and the result of this execution is the final result of the whole
    conditional expression.
*/
Then::Then(const GroupItems &children) : d(new ThenPrivate{{children}}) {}

/*!
    \overload

    Constructs a branch body element from \c std::initializer_list
    given by \a children.
*/
Then::Then(std::initializer_list<GroupItem> children) : d(new ThenPrivate{{children}}) {}

Group Then::body() const { return d->m_body; }

class ConditionData final
{
public:
    std::optional<ExecutableItem> m_condition;
    Group m_body;
};

static Group conditionRecipe(const Storage<bool> &bodyExecutedStorage, const ConditionData &condition)
{
    const auto onSetup = [bodyExecutedStorage] {
        return *bodyExecutedStorage ? SetupResult::StopWithSuccess : SetupResult::Continue;
    };

    const auto onBodyDone = [bodyExecutedStorage] { *bodyExecutedStorage = true; };

    const Group bodyTask { condition.m_body, onGroupDone(onBodyDone) };

    return {
        onGroupSetup(onSetup),
        condition.m_condition ? Group { !*condition.m_condition || bodyTask } : bodyTask
    };
}

static ExecutableItem conditionsRecipe(const QList<ConditionData> &conditions)
{
    Storage<bool> bodyExecutedStorage;

    GroupItems recipes;
    for (const ConditionData &condition : conditions)
        recipes << conditionRecipe(bodyExecutedStorage, condition);

    return Group { bodyExecutedStorage, recipes };
}

class ThenItemPrivate : public QSharedData
{
public:
    ThenItemPrivate(const QList<ConditionData> &conditions) : m_conditions(conditions) {}
    QList<ConditionData> m_conditions;
};

ThenItem::~ThenItem() = default;
ThenItem::ThenItem(const ThenItem &other) = default;
ThenItem::ThenItem(ThenItem &&other) noexcept = default;
ThenItem &ThenItem::operator=(const ThenItem &other) = default;

ThenItem::operator ExecutableItem() const
{
    return conditionsRecipe(d->m_conditions);
}

ThenItem::ThenItem(const If &ifItem, const Then &thenItem)
    : d(new ThenItemPrivate{{{ifItem.condition(), thenItem.body()}}}) {}

ThenItem::ThenItem(const ElseIfItem &elseIfItem, const Then &thenItem)
    : d(new ThenItemPrivate{elseIfItem.conditions()})
{
    d->m_conditions.append({elseIfItem.nextCondition(), thenItem.body()});
}

QList<ConditionData> ThenItem::conditions() const { return d->m_conditions; }

class ElseItemPrivate : public QSharedData
{
public:
    ElseItemPrivate(const QList<ConditionData> &conditions) : m_conditions(conditions) {}
    QList<ConditionData> m_conditions;
};

ElseItem::~ElseItem() = default;
ElseItem::ElseItem(const ElseItem &other) = default;
ElseItem::ElseItem(ElseItem &&other) noexcept = default;
ElseItem &ElseItem::operator=(const ElseItem &other) = default;

ElseItem::operator ExecutableItem() const
{
    return conditionsRecipe(d->m_conditions);
}

ElseItem::ElseItem(const ThenItem &thenItem, const Else &elseItem)
    : d(new ElseItemPrivate{thenItem.conditions()})
{
    d->m_conditions.append({{}, elseItem.body()});
}

class ElseIfItemPrivate : public QSharedData
{
public:
    ElseIfItemPrivate(const QList<ConditionData> &conditions, const ExecutableItem &nextCondition)
        : m_conditions(conditions)
        , m_nextCondition(nextCondition)
    {}
    QList<ConditionData> m_conditions;
    ExecutableItem m_nextCondition;
};

ElseIfItem::~ElseIfItem() = default;
ElseIfItem::ElseIfItem(const ElseIfItem &other) = default;
ElseIfItem::ElseIfItem(ElseIfItem &&other) noexcept = default;
ElseIfItem &ElseIfItem::operator=(const ElseIfItem &other) = default;

ElseIfItem::ElseIfItem(const ThenItem &thenItem, const ElseIf &elseIfItem)
    : d(new ElseIfItemPrivate{thenItem.conditions(), elseIfItem.condition()}) { }

QList<ConditionData> ElseIfItem::conditions() const { return d->m_conditions; }

ExecutableItem ElseIfItem::nextCondition() const { return d->m_nextCondition; }

ThenItem operator>>(const If &ifItem, const Then &thenItem)
{
    return {ifItem, thenItem};
}

ThenItem operator>>(const ElseIfItem &elseIfItem, const Then &thenItem)
{
    return {elseIfItem, thenItem};
}

ElseIfItem operator>>(const ThenItem &thenItem, const ElseIf &elseIfItem)
{
    return {thenItem, elseIfItem};
}

ElseItem operator>>(const ThenItem &thenItem, const Else &elseItem)
{
    return {thenItem, elseItem};
}

} // namespace QtTaskTree

QT_END_NAMESPACE
