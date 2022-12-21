// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "textmodifier.h"

#include <qmljs/qmljsmodelmanagerinterface.h>

#include <utils/textutils.h>

using namespace QmlDesigner;

TextModifier::~TextModifier() = default;

int TextModifier::getLineInDocument(QTextDocument *document, int offset)
{
    int line = -1;
    int column = -1;
    Utils::Text::convertPosition(document, offset, &line, &column);
    return line;
}

QmlJS::Snapshot TextModifier::qmljsSnapshot()
{
    QmlJS::ModelManagerInterface *modelManager = QmlJS::ModelManagerInterface::instance();
    if (modelManager)
        return modelManager->snapshot();
    else
        return QmlJS::Snapshot();
}
