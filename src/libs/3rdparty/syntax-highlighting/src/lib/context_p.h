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

#ifndef KSYNTAXHIGHLIGHTING_CONTEXT_P_H
#define KSYNTAXHIGHLIGHTING_CONTEXT_P_H

#include "contextswitch_p.h"
#include "definition.h"
#include "definitionref_p.h"
#include "format.h"
#include "rule_p.h"

#include <QString>

#include <vector>

QT_BEGIN_NAMESPACE
class QXmlStreamReader;
QT_END_NAMESPACE

namespace KSyntaxHighlighting
{
class Context
{
public:
    Context() = default;
    ~Context() = default;

    Definition definition() const;
    void setDefinition(const DefinitionRef &def);

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

    void load(QXmlStreamReader &reader);
    void resolveContexts();
    void resolveIncludes();
    void resolveAttributeFormat();

private:
    Q_DISABLE_COPY(Context)

    enum ResolveState { Unknown, Unresolved, Resolving, Resolved };
    ResolveState resolveState();

    DefinitionRef m_def;
    QString m_name;

    /**
     * attribute name, to lookup our format
     */
    QString m_attribute;

    /**
     * context to use for lookup, if != this context
     */
    const Context *m_attributeContext = nullptr;

    /**
     * resolved format for our attribute, done in resolveAttributeFormat
     */
    Format m_attributeFormat;

    ContextSwitch m_lineEndContext;
    ContextSwitch m_lineEmptyContext;
    ContextSwitch m_fallthroughContext;

    std::vector<Rule::Ptr> m_rules;

    ResolveState m_resolveState = Unknown;
    bool m_fallthrough = false;
    bool m_noIndentationBasedFolding = false;
};
}

#endif // KSYNTAXHIGHLIGHTING_CONTEXT_P_H
