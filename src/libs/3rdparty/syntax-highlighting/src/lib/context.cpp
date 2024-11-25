/*
    SPDX-FileCopyrightText: 2016 Volker Krause <vkrause@kde.org>
    SPDX-FileCopyrightText: 2020 Jonathan Poelen <jonathan.poelen@gmail.com>

    SPDX-License-Identifier: MIT
*/

#include "context_p.h"
#include "definition_p.h"
#include "format.h"
#include "ksyntaxhighlighting_logging.h"
#include "rule_p.h"
#include "xml_p.h"

#include <QString>
#include <QXmlStreamReader>

using namespace KSyntaxHighlighting;

Context::Context(const DefinitionData &def, const HighlightingContextData &data)
    : m_name(data.name)
    , m_attributeFormat(data.attribute.isEmpty() ? Format() : def.formatByName(data.attribute))
    , m_indentationBasedFolding(!data.noIndentationBasedFolding && def.indentationBasedFolding)
{
    if (!data.attribute.isEmpty() && !m_attributeFormat.isValid()) {
        qCWarning(Log) << "Context: Unknown format" << data.attribute << "in context" << m_name << "of definition" << def.name;
    }
}

bool Context::indentationBasedFoldingEnabled() const
{
    return m_indentationBasedFolding;
}

void Context::resolveContexts(DefinitionData &def, const HighlightingContextData &data)
{
    m_lineEndContext.resolve(def, data.lineEndContext);
    m_lineEmptyContext.resolve(def, data.lineEmptyContext);
    m_fallthroughContext.resolve(def, data.fallthroughContext);
    m_stopEmptyLineContextSwitchLoop = data.stopEmptyLineContextSwitchLoop;

    /**
     * line end context switches only when lineEmptyContext is #stay. This avoids
     * skipping empty lines after a line continuation character (see bug 405903)
     */
    if (m_lineEmptyContext.isStay()) {
        m_lineEmptyContext = m_lineEndContext;
    }

    m_rules.reserve(data.rules.size());
    for (const auto &ruleData : data.rules) {
        m_rules.push_back(Rule::create(def, ruleData, m_name));
        if (!m_rules.back()) {
            m_rules.pop_back();
        }
    }
}

void Context::resolveIncludes(DefinitionData &def)
{
    if (m_resolveState == Resolved) {
        return;
    }
    if (m_resolveState == Resolving) {
        qCWarning(Log) << "Cyclic dependency!";
        return;
    }

    Q_ASSERT(m_resolveState == Unresolved);
    m_resolveState = Resolving; // cycle guard

    for (auto it = m_rules.begin(); it != m_rules.end();) {
        const IncludeRules *includeRules = it->get()->castToIncludeRules();
        if (!includeRules) {
            m_hasDynamicRule = m_hasDynamicRule || it->get()->isDynamic();
            ++it;
            continue;
        }

        const QStringView includeContext = includeRules->contextName();
        const qsizetype idx = includeContext.indexOf(QLatin1String("##"));

        Context *context = nullptr;
        DefinitionData *defData = &def;

        if (idx <= -1) { // local include
            context = def.contextByName(includeContext);
        } else {
            const auto definitionName = includeContext.sliced(idx + 2);
            const auto contextName = includeContext.sliced(0, idx);
            auto resolvedContext = def.resolveIncludedContext(definitionName, contextName);
            defData = resolvedContext.def;
            context = resolvedContext.context;
            if (!defData) {
                qCWarning(Log) << "Unable to resolve external include rule for definition" << definitionName << "in" << def.name;
            }
        }

        if (!context) {
            qCWarning(Log) << "Unable to resolve include rule for definition" << includeContext << "in" << def.name;
            ++it;
            continue;
        }

        if (context == this) {
            qCWarning(Log) << "Unable to resolve self include rule for definition" << includeContext << "in" << def.name;
            ++it;
            continue;
        }

        if (context->m_resolveState != Resolved) {
            context->resolveIncludes(*defData);
        }

        m_hasDynamicRule = m_hasDynamicRule || context->m_hasDynamicRule;

        /**
         * handle included attribute
         * transitive closure: we might include attributes included from somewhere else
         */
        if (includeRules->includeAttribute()) {
            m_attributeFormat = context->m_attributeFormat;
        }

        it = m_rules.erase(it);
        it = m_rules.insert(it, context->rules().begin(), context->rules().end());
        it += context->rules().size();
    }

    m_resolveState = Resolved;
}
