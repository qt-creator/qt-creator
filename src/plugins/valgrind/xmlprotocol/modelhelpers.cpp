// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "modelhelpers.h"
#include "frame.h"
#include "../valgrindtr.h"

#include <QString>
#include <QDir>
#include <QPair>

namespace Valgrind {
namespace XmlProtocol {

QString toolTipForFrame(const Frame &frame)
{
    QString location;
    if (!frame.fileName().isEmpty()) {
        location = frame.filePath();
        if (frame.line() > 0)
            location += ':' + QString::number(frame.line());
    }

    using StringPair = QPair<QString, QString>;
    QList<StringPair> lines;

    if (!frame.functionName().isEmpty())
        lines << qMakePair(Tr::tr("Function:"), frame.functionName());
    if (!location.isEmpty())
        lines << qMakePair(Tr::tr("Location:"), location);
    if (frame.instructionPointer())
        lines << qMakePair(Tr::tr("Instruction pointer:"),
                           QString("0x%1").arg(frame.instructionPointer(), 0, 16));
    if (!frame.object().isEmpty())
        lines << qMakePair(Tr::tr("Object:"), frame.object());

    QString html = "<html><head>"
                   "<style>dt { font-weight:bold; } dd { font-family: monospace; }</style>\n"
                   "</head><body><dl>";

    for (const StringPair &pair : std::as_const(lines)) {
        html += "<dt>";
        html += pair.first;
        html += "</dt><dd>";
        html += pair.second;
        html += "</dd>\n";
    }
    html += "</dl></body></html>";
    return html;
}

} // namespace XmlProtocol
} // namespace Valgrind
