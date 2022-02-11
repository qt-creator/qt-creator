/*
    SPDX-FileCopyrightText: 2016 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: MIT
*/

#ifndef KSYNTAXHIGHLIGHTING_CONTEXT_P_H
#define KSYNTAXHIGHLIGHTING_CONTEXT_P_H

#include "contextswitch_p.h"
#include "format.h"
#include "highlightingdata_p.hpp"
#include "rule_p.h"

#include <QString>

#include <vector>

QT_BEGIN_NAMESPACE
class QXmlStreamReader;
QT_END_NAMESPACE

namespace KSyntaxHighlighting
{
class DefinitionData;

class Context
{
public:
    Q_DISABLE_COPY(Context)

    Context(Context &&) = default;
    Context &operator=(Context &&) = default;

    Context(const DefinitionData &def, const HighlightingContextData &data);
    ~Context() = default;

    const QString &name() const
    {
        return m_name;
    }

    const ContextSwitch &lineEndContext() const
    {
        return m_lineEndContext;
    }

    const ContextSwitch &lineEmptyContext() const
    {
        return m_lineEmptyContext;
    }

    bool fallthrough() const
    {
        return m_fallthrough;
    }

    const ContextSwitch &fallthroughContext() const
    {
        return m_fallthroughContext;
    }

    const Format &attributeFormat() const
    {
        return m_attributeFormat;
    }

    const std::vector<Rule::Ptr> &rules() const
    {
        return m_rules;
    }

    /**
     * Returns @c true, when indentationBasedFolding is enabled for the
     * associated Definition and when "noIndentationBasedFolding" is NOT set.
     */
    bool indentationBasedFoldingEnabled() const;

    void resolveContexts(DefinitionData &def, const HighlightingContextData &data);
    void resolveIncludes(DefinitionData &def);

private:
    enum ResolveState : quint8 { Unresolved, Resolving, Resolved };

    std::vector<Rule::Ptr> m_rules;

    QString m_name;

    ContextSwitch m_lineEndContext;
    ContextSwitch m_lineEmptyContext;
    ContextSwitch m_fallthroughContext;

    /**
     * resolved format for our attribute, done in constructor and resolveIncludes
     */
    Format m_attributeFormat;

    ResolveState m_resolveState = Unresolved;
    bool m_fallthrough = false;
    bool m_indentationBasedFolding;
};
}

#endif // KSYNTAXHIGHLIGHTING_CONTEXT_P_H
