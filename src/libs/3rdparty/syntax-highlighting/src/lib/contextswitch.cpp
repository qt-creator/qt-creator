/*
    SPDX-FileCopyrightText: 2016 Volker Krause <vkrause@kde.org>
    SPDX-FileCopyrightText: 2024 Jonathan Poelen <jonathan.poelen@gmail.com>

    SPDX-License-Identifier: MIT
*/

#include "contextswitch_p.h"
#include "definition_p.h"
#include "ksyntaxhighlighting_logging.h"
#include <QStringTokenizer>

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

    for (auto contextPart : QStringTokenizer{context, u'!'}) {
        const qsizetype defNameIndex = contextPart.indexOf(QStringLiteral("##"));
        auto defName = (defNameIndex <= -1) ? QStringView() : contextPart.sliced(defNameIndex + 2);
        auto contextName = (defNameIndex <= -1) ? contextPart : contextPart.sliced(0, defNameIndex);

        auto resolvedCtx = def.resolveIncludedContext(defName, contextName);
        if (resolvedCtx.context) {
            m_contexts.append(resolvedCtx.context);
        } else {
            auto part = (defName.isEmpty() || resolvedCtx.def) ? "context" : "definition in";
            qCWarning(Log) << "cannot find" << part << contextPart << "in" << def.name;
        }
    }

    m_isStay = m_contexts.isEmpty();
}
