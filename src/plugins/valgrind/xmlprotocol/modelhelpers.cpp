/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Author: Milian Wolff, KDAB (milian.wolff@kdab.com)
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "modelhelpers.h"
#include "frame.h"

#include <QString>
#include <QDir>
#include <QPair>
#include <QCoreApplication>

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
        lines << qMakePair(QCoreApplication::translate("Valgrind::XmlProtocol", "Function:"),
                           frame.functionName());
    if (!location.isEmpty())
        lines << qMakePair(QCoreApplication::translate("Valgrind::XmlProtocol", "Location:"),
                           location);
    if (frame.instructionPointer())
        lines << qMakePair(QCoreApplication::translate("Valgrind::XmlProtocol",
                                                       "Instruction pointer:"),
                           QString::fromLatin1("0x%1").arg(frame.instructionPointer(), 0, 16));
    if (!frame.object().isEmpty())
        lines << qMakePair(QCoreApplication::translate("Valgrind::XmlProtocol", "Object:"), frame.object());

    QString html = "<html><head>"
                   "<style>dt { font-weight:bold; } dd { font-family: monospace; }</style>\n"
                   "</head><body><dl>";

    foreach (const StringPair &pair, lines) {
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
