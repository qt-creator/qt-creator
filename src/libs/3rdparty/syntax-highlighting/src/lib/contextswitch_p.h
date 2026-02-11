/*
    SPDX-FileCopyrightText: 2016 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: MIT
*/

#ifndef KSYNTAXHIGHLIGHTING_CONTEXTSWITCH_P_H
#define KSYNTAXHIGHLIGHTING_CONTEXTSWITCH_P_H

#include <QStringView>
#include <QVarLengthArray>

namespace KSyntaxHighlighting
{
class Context;
class DefinitionData;

class ContextSwitch
{
public:
    using ContextList = QVarLengthArray<Context *, 2>;

    ContextSwitch() = default;
    ~ContextSwitch() = default;

    bool isStay() const
    {
        return m_isStay;
    }

    int popCount() const
    {
        return m_popCount;
    }

    const ContextList &contexts() const
    {
        return m_contexts;
    }

    void resolve(DefinitionData &def, QStringView context);

private:
    bool m_isStay = true;
    int m_popCount = 0;
    ContextList m_contexts;
};
}

#endif // KSYNTAXHIGHLIGHTING_CONTEXTSWITCH_P_H
