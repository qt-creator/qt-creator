/*
    SPDX-FileCopyrightText: 2016 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: MIT
*/

#include "contextswitch_p.h"
#include "definition.h"
#include "definition_p.h"
#include "highlightingdata_p.hpp"
#include "ksyntaxhighlighting_logging.h"
#include "repository.h"

using namespace KSyntaxHighlighting;

void ContextSwitch::resolve(DefinitionData &def, QStringView contextInstr)
{
    HighlightingContextData::ContextSwitch ctx(contextInstr);

    m_popCount = ctx.popCount();
    m_isStay = !m_popCount;

    auto contextName = ctx.contextName();
    auto defName = ctx.defName();

    if (contextName.isEmpty() && defName.isEmpty()) {
        return;
    }

    if (defName.isEmpty()) {
        m_context = def.contextByName(contextName);
    } else {
        auto d = def.repo->definitionForName(defName.toString());
        if (d.isValid()) {
            auto data = DefinitionData::get(d);
            def.addImmediateIncludedDefinition(d);
            data->load();
            if (contextName.isEmpty()) {
                m_context = data->initialContext();
            } else {
                m_context = data->contextByName(contextName);
            }
        }
    }

    if (!m_context) {
        qCWarning(Log) << "cannot find context" << contextName << "in" << def.name;
    } else {
        m_isStay = false;
    }
}
