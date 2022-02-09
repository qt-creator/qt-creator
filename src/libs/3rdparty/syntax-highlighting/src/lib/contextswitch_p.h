/*
    SPDX-FileCopyrightText: 2016 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: MIT
*/

#ifndef KSYNTAXHIGHLIGHTING_CONTEXTSWITCH_P_H
#define KSYNTAXHIGHLIGHTING_CONTEXTSWITCH_P_H

#include <QString>

namespace KSyntaxHighlighting
{
class Context;
class DefinitionData;

class ContextSwitch
{
public:
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

    Context *context() const
    {
        return m_context;
    }

    void resolve(DefinitionData &def, QStringView contextInstr);

private:
    Context *m_context = nullptr;
    int m_popCount = 0;
    bool m_isStay = true;
};
}

#endif // KSYNTAXHIGHLIGHTING_CONTEXTSWITCH_P_H
