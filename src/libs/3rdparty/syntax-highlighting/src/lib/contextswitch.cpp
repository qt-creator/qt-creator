/*
    SPDX-FileCopyrightText: 2016 Volker Krause <vkrause@kde.org>
    SPDX-FileCopyrightText: 2024 Jonathan Poelen <jonathan.poelen@gmail.com>

    SPDX-License-Identifier: MIT
*/

#include "contextswitch_p.h"
#include "definition_p.h"
#include "ksyntaxhighlighting_logging.h"

using namespace KSyntaxHighlighting;

void ContextSwitch::resolve(DefinitionData &def, QStringView context)
{
    if (context.isEmpty() || context == QStringLiteral("#stay")) {
        return;
    }

    while (context.startsWith(QStringLiteral("#pop"))) {
        ++m_popCount;
        if (context.size() > 4 && context.at(4) == QLatin1Char('!')) {
            context = context.sliced(5);
            break;
        }
        context = context.sliced(4);
    }

    m_isStay = !m_popCount;

    if (context.isEmpty()) {
        return;
    }

    const qsizetype defNameIndex = context.indexOf(QStringLiteral("##"));
    auto defName = (defNameIndex <= -1) ? QStringView() : context.sliced(defNameIndex + 2);
    auto contextName = (defNameIndex <= -1) ? context : context.sliced(0, defNameIndex);

    m_context = def.resolveIncludedContext(defName, contextName).context;

    if (m_context) {
        m_isStay = false;
    } else {
        qCWarning(Log) << "cannot find context" << contextName << "in" << def.name;
    }
}
