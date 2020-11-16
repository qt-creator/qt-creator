/*
    SPDX-FileCopyrightText: 2016 Volker Krause <vkrause@kde.org>
    SPDX-FileCopyrightText: 2018 Christoph Cullmann <cullmann@kde.org>

    SPDX-License-Identifier: MIT
*/

#include "state.h"
#include "state_p.h"

#include "context_p.h"

#include <QStringList>

using namespace KSyntaxHighlighting;

StateData *StateData::get(State &state)
{
    // create state data on demand, to make default state construction cheap
    if (!state.d) {
        state.d = new StateData();
    } else {
        state.d.detach();
    }
    return state.d.data();
}

bool StateData::isEmpty() const
{
    return m_contextStack.isEmpty();
}

void StateData::clear()
{
    m_contextStack.clear();
}

int StateData::size() const
{
    return m_contextStack.size();
}

void StateData::push(Context *context, const QStringList &captures)
{
    Q_ASSERT(context);
    m_contextStack.push_back(qMakePair(context, captures));
}

bool StateData::pop(int popCount)
{
    // nop if nothing to pop
    if (popCount <= 0) {
        return true;
    }

    // keep the initial context alive in any case
    Q_ASSERT(!isEmpty());
    const bool initialContextSurvived = m_contextStack.size() > popCount;
    m_contextStack.resize(std::max(1, int(m_contextStack.size()) - popCount));
    return initialContextSurvived;
}

Context *StateData::topContext() const
{
    Q_ASSERT(!isEmpty());
    return m_contextStack.last().first;
}

const QStringList &StateData::topCaptures() const
{
    Q_ASSERT(!isEmpty());
    return m_contextStack.last().second;
}

State::State()
{
}

State::State(const State &other)
    : d(other.d)
{
}

State::~State()
{
}

State &State::operator=(const State &other)
{
    d = other.d;
    return *this;
}

bool State::operator==(const State &other) const
{
    // use pointer equal as shortcut for shared states
    return (d == other.d) || (d && other.d && d->m_contextStack == other.d->m_contextStack && d->m_defRef == other.d->m_defRef);
}

bool State::operator!=(const State &other) const
{
    return !(*this == other);
}

bool State::indentationBasedFoldingEnabled() const
{
    if (!d || d->m_contextStack.isEmpty())
        return false;
    return d->m_contextStack.last().first->indentationBasedFoldingEnabled();
}
