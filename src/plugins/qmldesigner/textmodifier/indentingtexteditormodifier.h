// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <qmldesigner_global.h>

#include <plaintexteditmodifier.h>

namespace QmlDesigner {

class QMLDESIGNER_EXPORT IndentingTextEditModifier : public NotIndentingTextEditModifier
{
public:
    IndentingTextEditModifier(QTextDocument *document, const QTextCursor &textCursor);

    void indent(int offset, int length) override;
    void indentLines(int startLine, int endLine) override;
};

} // namespace QmlDesigner
