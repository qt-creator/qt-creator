// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "textmodifier.h"
#include "rewritertracing.h"

#include <qmljs/qmljsmodelmanagerinterface.h>

#include <utils/textutils.h>

namespace QmlDesigner {

using NanotraceHR::keyValue;
using RewriterTracing::category;

TextModifier::~TextModifier()
{
    NanotraceHR::Tracer tracer{"text modifier destructor", category()};
};

TextModifier::TextModifier()
{
    NanotraceHR::Tracer tracer{"text modifier constructor", category()};
}

int TextModifier::getLineInDocument(QTextDocument *document, int offset)
{
    NanotraceHR::Tracer tracer{"text modifier get line in document",
                               category(),
                               keyValue("offset", offset)};

    int line = -1;
    int column = -1;
    Utils::Text::convertPosition(document, offset, &line, &column);
    return line;
}

QmlJS::Snapshot TextModifier::qmljsSnapshot()
{
    NanotraceHR::Tracer tracer{"text modifier qmljs snapshot", category()};

    QmlJS::ModelManagerInterface *modelManager = QmlJS::ModelManagerInterface::instance();
    if (modelManager)
        return modelManager->snapshot();
    else
        return QmlJS::Snapshot();
}
} // namespace QmlDesigner
