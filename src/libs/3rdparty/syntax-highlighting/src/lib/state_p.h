/*
    SPDX-FileCopyrightText: 2016 Volker Krause <vkrause@kde.org>
    SPDX-FileCopyrightText: 2018 Christoph Cullmann <cullmann@kde.org>

    SPDX-License-Identifier: MIT
*/

#ifndef KSYNTAXHIGHLIGHTING_STATE_P_H
#define KSYNTAXHIGHLIGHTING_STATE_P_H

#include <vector>

#include <QSharedData>
#include <QStringList>

namespace KSyntaxHighlighting
{
class Context;

class StateData : public QSharedData
{
    friend class State;
    friend class AbstractHighlighter;
    friend std::size_t qHash(const StateData &, std::size_t);

public:
    StateData() = default;

    static StateData *reset(State &state);
    static StateData *detach(State &state);

    static StateData *get(const State &state)
    {
        return state.d.data();
    }

    std::size_t size() const
    {
        return m_contextStack.size();
    }

    void push(Context *context, QStringList &&captures);

    /**
     * Pop the number of elements given from the top of the current stack.
     * Will not pop the initial element.
     * @param popCount number of elements to pop
     * @return false if one has tried to pop the initial context, else true
     */
    bool pop(int popCount);

    Context *topContext() const
    {
        return m_contextStack.back().context;
    }

    const QStringList &topCaptures() const
    {
        return m_contextStack.back().captures;
    }

    struct StackValue {
        Context *context;
        QStringList captures;

        bool operator==(const StackValue &other) const
        {
            return context == other.context && captures == other.captures;
        }
    };

private:
    /**
     * definition id to filter out invalid states
     */
    uint64_t m_defId = 0;

    /**
     * the context stack combines the active context + valid captures
     */
    std::vector<StackValue> m_contextStack;
};

inline std::size_t qHash(const StateData::StackValue &stackValue, std::size_t seed = 0)
{
    return qHashMulti(seed, stackValue.context, stackValue.captures);
}

inline std::size_t qHash(const StateData &k, std::size_t seed = 0)
{
    return qHashMulti(seed, k.m_defId, qHashRange(k.m_contextStack.begin(), k.m_contextStack.end(), seed));
}
}

#endif
