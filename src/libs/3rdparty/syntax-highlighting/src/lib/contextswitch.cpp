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

#include "contextswitch_p.h"
#include "definition.h"
#include "definition_p.h"
#include "repository.h"
#include "ksyntaxhighlighting_logging.h"


using namespace KSyntaxHighlighting;

bool ContextSwitch::isStay() const
{
    return m_popCount == 0 && !m_context && m_contextName.isEmpty() && m_defName.isEmpty();
}

int ContextSwitch::popCount() const
{
    return m_popCount;
}

Context* ContextSwitch::context() const
{
    return m_context;
}

void ContextSwitch::parse(const QStringRef& contextInstr)
{
    if (contextInstr.isEmpty() || contextInstr == QLatin1String("#stay"))
        return;

    if (contextInstr.startsWith(QLatin1String("#pop!"))) {
        ++m_popCount;
        m_contextName = contextInstr.mid(5).toString();
        return;
    }

    if (contextInstr.startsWith(QLatin1String("#pop"))) {
        ++m_popCount;
        parse(contextInstr.mid(4));
        return;
    }

    const auto idx = contextInstr.indexOf(QLatin1String("##"));
    if (idx >= 0) {
        m_contextName = contextInstr.left(idx).toString();
        m_defName = contextInstr.mid(idx + 2).toString();
    } else {
        m_contextName = contextInstr.toString();
    }
}

void ContextSwitch::resolve(const Definition &def)
{
    auto d = def;
    if (!m_defName.isEmpty()) {
        d = DefinitionData::get(def)->repo->definitionForName(m_defName);
        auto data = DefinitionData::get(d);
        data->load();
        if (m_contextName.isEmpty())
            m_context = data->initialContext();
    }

    if (!m_contextName.isEmpty()) {
        m_context = DefinitionData::get(d)->contextByName(m_contextName);
        if (!m_context)
            qCWarning(Log) << "cannot find context" << m_contextName << "in" << def.name();
    }
}
