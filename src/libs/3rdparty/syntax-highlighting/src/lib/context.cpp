/*
    Copyright (C) 2016 Volker Krause <vkrause@kde.org>

    Permission is hereby granted, free of charge, to any person obtaining
    a copy of this software and associated documentation files (the
    "Software"), to deal in the Software without restriction, including
    without limitation the rights to use, copy, modify, merge, publish,
    distribute, sublicense, and/or sell copies of the Software, and to
    permit persons to whom the Software is furnished to do so, subject to
    the following conditions:

    The above copyright notice and this permission notice shall be included
    in all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
    EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
    MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
    IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
    CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
    TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
    SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include "context_p.h"
#include "definition_p.h"
#include "format.h"
#include "repository.h"
#include "rule_p.h"
#include "ksyntaxhighlighting_logging.h"
#include "xml_p.h"

#include <QString>
#include <QXmlStreamReader>

using namespace KSyntaxHighlighting;

Definition Context::definition() const
{
    return m_def.definition();
}

void Context::setDefinition(const DefinitionRef &def)
{
    m_def = def;
}

bool Context::indentationBasedFoldingEnabled() const
{
    if (m_noIndentationBasedFolding)
        return false;

    return m_def.definition().indentationBasedFoldingEnabled();
}

void Context::load(QXmlStreamReader& reader)
{
    Q_ASSERT(reader.name() == QLatin1String("context"));
    Q_ASSERT(reader.tokenType() == QXmlStreamReader::StartElement);

    m_name = reader.attributes().value(QStringLiteral("name")).toString();
    m_attribute = reader.attributes().value(QStringLiteral("attribute")).toString();
    m_lineEndContext.parse(reader.attributes().value(QStringLiteral("lineEndContext")));
    m_lineEmptyContext.parse(reader.attributes().value(QStringLiteral("lineEmptyContext")));
    m_fallthrough = Xml::attrToBool(reader.attributes().value(QStringLiteral("fallthrough")));
    m_fallthroughContext.parse(reader.attributes().value(QStringLiteral("fallthroughContext")));
    if (m_fallthroughContext.isStay())
        m_fallthrough = false;
    m_noIndentationBasedFolding = Xml::attrToBool(reader.attributes().value(QStringLiteral("noIndentationBasedFolding")));

    reader.readNext();
    while (!reader.atEnd()) {
        switch (reader.tokenType()) {
            case QXmlStreamReader::StartElement:
            {
                auto rule = Rule::create(reader.name());
                if (rule) {
                    rule->setDefinition(m_def.definition());
                    if (rule->load(reader))
                        m_rules.push_back(rule);
                } else {
                    reader.skipCurrentElement();
                }
                reader.readNext();
                break;
            }
            case QXmlStreamReader::EndElement:
                return;
            default:
                reader.readNext();
                break;
        }
    }
}

void Context::resolveContexts()
{
    const auto def = m_def.definition();
    m_lineEndContext.resolve(def);
    m_lineEmptyContext.resolve(def);
    m_fallthroughContext.resolve(def);
    for (const auto &rule : m_rules)
        rule->resolveContext();
}

Context::ResolveState Context::resolveState()
{
    if (m_resolveState == Unknown) {
        for (const auto &rule : m_rules) {
            auto inc = std::dynamic_pointer_cast<IncludeRules>(rule);
            if (inc) {
                m_resolveState = Unresolved;
                return m_resolveState;
            }
        }
        m_resolveState = Resolved;
    }
    return m_resolveState;
}

void Context::resolveIncludes()
{
    if (resolveState() == Resolved)
        return;
    if (resolveState() == Resolving) {
        qCWarning(Log) << "Cyclic dependency!";
        return;
    }

    Q_ASSERT(resolveState() == Unresolved);
    m_resolveState = Resolving; // cycle guard

    for (auto it = m_rules.begin(); it != m_rules.end();) {
        auto inc = std::dynamic_pointer_cast<IncludeRules>(*it);
        if (!inc) {
            ++it;
            continue;
        }
        Context* context = nullptr;
        auto myDefData = DefinitionData::get(m_def.definition());
        if (inc->definitionName().isEmpty()) { // local include
            context = myDefData->contextByName(inc->contextName());
        } else {
            auto def = myDefData->repo->definitionForName(inc->definitionName());
            if (!def.isValid()) {
                qCWarning(Log) << "Unable to resolve external include rule for definition" << inc->definitionName() << "in" << m_def.definition().name();
                ++it;
                continue;
            }
            auto defData = DefinitionData::get(def);
            defData->load();
            if (inc->contextName().isEmpty())
                context = defData->initialContext();
            else
                context = defData->contextByName(inc->contextName());
        }
        if (!context) {
            qCWarning(Log) << "Unable to resolve include rule for definition" << inc->contextName() << "##" << inc->definitionName() << "in" << m_def.definition().name();
            ++it;
            continue;
        }
        context->resolveIncludes();

        /**
         * handle included attribute
         * transitive closure: we might include attributes included from somewhere else
         */
        if (inc->includeAttribute()) {
            m_attribute = context->m_attribute;
            m_attributeContext = context->m_attributeContext ? context->m_attributeContext : context;
        }

        it = m_rules.erase(it);
        for (const auto &rule : context->rules()) {
            it = m_rules.insert(it, rule);
            ++it;
        }
    }

    m_resolveState = Resolved;
}

void Context::resolveAttributeFormat()
{
    /**
     * try to get our format from the definition we stem from
     * we need to handle included attributes via m_attributeContext
     */
    if (!m_attribute.isEmpty()) {
        const auto def = m_attributeContext ? m_attributeContext->m_def.definition() : m_def.definition();
        m_attributeFormat = DefinitionData::get(def)->formatByName(m_attribute);
        if (!m_attributeFormat.isValid()) {
            if (m_attributeContext) {
                qCWarning(Log) << "Context: Unknown format" << m_attribute << "in context" << m_name << "of definition" << m_def.definition().name() << "from included context" << m_attributeContext->m_name << "of definition" << def.name();
            } else {
                qCWarning(Log) << "Context: Unknown format" << m_attribute << "in context" << m_name << "of definition" << m_def.definition().name();
            }
        }
    }

    /**
     * lookup formats for our rules
     */
    for (const auto &rule : m_rules) {
        rule->resolveAttributeFormat(this);
    }
}
