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
class Definition;

class ContextSwitch
{
public:
    ContextSwitch() = default;
    ~ContextSwitch() = default;

    bool isStay() const;

    int popCount() const;
    Context *context() const;

    void parse(QStringView contextInstr);
    void resolve(const Definition &def);

private:
    QString m_defName;
    QString m_contextName;
    Context *m_context = nullptr;
    int m_popCount = 0;
};
}

#endif // KSYNTAXHIGHLIGHTING_CONTEXTSWITCH_P_H
