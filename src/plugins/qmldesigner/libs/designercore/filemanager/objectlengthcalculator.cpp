// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "objectlengthcalculator.h"
#include "../rewriter/rewritertracing.h"

#include <qmljs/parser/qmljsast_p.h>

namespace QmlDesigner {

using NanotraceHR::keyValue;
using RewriterTracing::category;

ObjectLengthCalculator::ObjectLengthCalculator()
    : m_doc(QmlJS::Document::create(Utils::FilePath::fromString("<internal>"), QmlJS::Dialect::Qml))
{
    NanotraceHR::Tracer tracer{"object length calculator constructor", category()};
}

bool ObjectLengthCalculator::operator()(const QString &text, quint32 offset,
                                        quint32 &length)
{
    NanotraceHR::Tracer tracer{"object length calculator operator()",
                               category(),
                               keyValue("text", text),
                               keyValue("offset", offset)};

    m_offset = offset;
    m_length = 0;
    m_doc->setSource(text);

    if (!m_doc->parseQml())
        return false;

    QmlJS::AST::Node::accept(m_doc->qmlProgram(), this);
    if (m_length) {
        length = m_length;
        return true;
    } else {
        return false;
    }
}

bool ObjectLengthCalculator::visit(QmlJS::AST::UiObjectBinding *ast)
{
    NanotraceHR::Tracer tracer{"object length calculator visit ui object binding", category()};

    if (m_length > 0)
        return false;

    if (ast->qualifiedTypeNameId && ast->qualifiedTypeNameId->identifierToken.isValid()) {
        const quint32 start = ast->qualifiedTypeNameId->identifierToken.offset;
        const quint32 end = ast->lastSourceLocation().end();

        if (start == m_offset) {
            m_length = end - start;
            return false;
        }

        return m_offset < end;
    } else {
        return true;
    }
}

bool ObjectLengthCalculator::visit(QmlJS::AST::UiObjectDefinition *ast)
{
    NanotraceHR::Tracer tracer{"object length calculator visit ui object definition", category()};

    if (m_length > 0)
        return false;

    const quint32 start = ast->firstSourceLocation().offset;
    const quint32 end = ast->lastSourceLocation().end();

    if (start == m_offset) {
        m_length = end - start;
        return false;
    }

    return m_offset < end;
}

void ObjectLengthCalculator::throwRecursionDepthError()
{
    NanotraceHR::Tracer tracer{"object length calculator throw recursion depth error", category()};

    qWarning("Warning: Hit maximum recursion depth while visiting the AST in ObjectLengthCalculator");
}

} // namespace QmlDesigner
