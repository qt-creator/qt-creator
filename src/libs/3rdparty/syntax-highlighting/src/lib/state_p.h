/*
    SPDX-FileCopyrightText: 2016 Volker Krause <vkrause@kde.org>
    SPDX-FileCopyrightText: 2018 Christoph Cullmann <cullmann@kde.org>

    SPDX-License-Identifier: MIT
*/

#ifndef KSYNTAXHIGHLIGHTING_STATE_P_H
#define KSYNTAXHIGHLIGHTING_STATE_P_H

#include <QSharedData>
#include <QVector>

#include "definitionref_p.h"

namespace KSyntaxHighlighting
{
class Context;

class StateData : public QSharedData
{
    friend class State;
    friend class AbstractHighlighter;

public:
    StateData() = default;
    static StateData *get(State &state);

    bool isEmpty() const;
    void clear();
    int size() const;
    void push(Context *context, const QStringList &captures);

    /**
     * Pop the number of elements given from the top of the current stack.
     * Will not pop the initial element.
     * @param popCount number of elements to pop
     * @return false if one has tried to pop the initial context, else true
     */
    bool pop(int popCount);

    Context *topContext() const;
    const QStringList &topCaptures() const;

private:
    /**
     * weak reference to the used definition to filter out invalid states
     */
    DefinitionRef m_defRef;

    /**
     * the context stack combines the active context + valid captures
     */
    QVector<QPair<Context *, QStringList>> m_contextStack;
};

}

#endif
