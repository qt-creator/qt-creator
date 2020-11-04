/*
    SPDX-FileCopyrightText: 2016 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: MIT
*/

#ifndef KSYNTAXHIGHLIGHTING_ABSTRACTHIGHLIGHTER_P_H
#define KSYNTAXHIGHLIGHTING_ABSTRACTHIGHLIGHTER_P_H

#include "definition.h"
#include "theme.h"

namespace KSyntaxHighlighting
{
class ContextSwitch;
class StateData;

class AbstractHighlighterPrivate
{
public:
    AbstractHighlighterPrivate();
    virtual ~AbstractHighlighterPrivate();

    void ensureDefinitionLoaded();
    bool switchContext(StateData *data, const ContextSwitch &contextSwitch, const QStringList &captures);

    Definition m_definition;
    Theme m_theme;
};

}

#endif
