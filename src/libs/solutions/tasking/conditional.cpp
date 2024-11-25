// Copyright (C) 2024 Jarek Kobus
// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "conditional.h"

QT_BEGIN_NAMESPACE

namespace Tasking {

static Group conditionRecipe(const Storage<bool> &bodyExecutedStorage, const ConditionData &condition)
{
    const auto onSetup = [bodyExecutedStorage] {
        return *bodyExecutedStorage ? SetupResult::StopWithSuccess : SetupResult::Continue;
    };

    const auto onBodyDone = [bodyExecutedStorage] { *bodyExecutedStorage = true; };

    const Group bodyTask { condition.m_body, onGroupDone(onBodyDone) };

    return {
        onGroupSetup(onSetup),
        condition.m_condition ? Group{ !*condition.m_condition || bodyTask } : bodyTask
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

ThenItem::operator ExecutableItem() const
{
    return conditionsRecipe(m_conditions);
}

ThenItem::ThenItem(const If &ifItem, const Then &thenItem)
    : m_conditions{{ifItem.m_condition, thenItem.m_body}}
{}

ThenItem::ThenItem(const ElseIfItem &elseIfItem, const Then &thenItem)
    : m_conditions(elseIfItem.m_conditions)
{
    m_conditions.append({elseIfItem.m_nextCondition, thenItem.m_body});
}

ElseItem::operator ExecutableItem() const
{
    return conditionsRecipe(m_conditions);
}

ElseItem::ElseItem(const ThenItem &thenItem, const Else &elseItem)
    : m_conditions(thenItem.m_conditions)
{
    m_conditions.append({{}, elseItem.m_body});
}

ElseIfItem::ElseIfItem(const ThenItem &thenItem, const ElseIf &elseIfItem)
    : m_conditions(thenItem.m_conditions)
    , m_nextCondition(elseIfItem.m_condition)
{}

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

} // namespace Tasking

QT_END_NAMESPACE
