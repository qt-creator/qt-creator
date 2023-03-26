// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "snippetparser.h"

namespace TextEditor {

QString SnippetParseError::htmlMessage() const
{
    QString message = errorMessage;
    if (pos < 0 || pos > 50)
        return message;
    QString detail = text.left(50);
    if (detail != text)
        detail.append("...");
    detail.replace(QChar::Space, "&nbsp;");
    message.append("<br><code>" + detail + "<br>");
    for (int i = 0; i < pos; ++i)
        message.append("&nbsp;");
    message.append("^</code>");
    return message;
}

} // namespace TextEditor
