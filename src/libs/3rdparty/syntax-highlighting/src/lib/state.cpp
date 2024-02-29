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

StateData *StateData::reset(State &state)
{
    auto *p = new StateData();
    state.d.reset(p);
    return p;
}

StateData *StateData::detach(State &state)
{
    state.d.detach();
    return state.d.data();
}

void StateData::push(Context *context, QStringList &&captures)
{
    Q_ASSERT(context);
    m_contextStack.push_back(StackValue{context, std::move(captures)});
}

bool StateData::pop(int popCount)
{
    // nop if nothing to pop
    if (popCount <= 0) {
        return true;
    }

    // keep the initial context alive in any case
    Q_ASSERT(!m_contextStack.empty());
    const bool initialContextSurvived = int(m_contextStack.size()) > popCount;
    m_contextStack.resize(std::max(1, int(m_contextStack.size()) - popCount));
    return initialContextSurvived;
}

State::State() = default;

State::State(State &&other) noexcept = default;

State::State(const State &other) noexcept = default;

State::~State() = default;

State &State::operator=(State &&other) noexcept = default;

State &State::operator=(const State &other) noexcept = default;

bool State::operator==(const State &other) const
{
    // use pointer equal as shortcut for shared states
    return (d == other.d) || (d && other.d && d->m_contextStack == other.d->m_contextStack && d->m_defId == other.d->m_defId);
}

bool State::operator!=(const State &other) const
{
    return !(*this == other);
}

bool State::indentationBasedFoldingEnabled() const
{
    if (!d || d->m_contextStack.empty()) {
        return false;
    }
    return d->m_contextStack.back().context->indentationBasedFoldingEnabled();
}

std::size_t KSyntaxHighlighting::qHash(const State &state, std::size_t seed)
{
    return state.d ? qHashMulti(seed, *state.d) : 0;
}
