/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Author: Milian Wolff, KDAB (milian.wolff@kdab.com)
**
** Contact: http://www.qt-project.org/
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**************************************************************************/

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
    if (!frame.file().isEmpty()) {
        location = frame.directory() + QDir::separator() + frame.file();
        if (frame.line() > 0)
            location += ':' + QString::number(frame.line());
    }

    typedef QPair<QString, QString> StringPair;
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
                           QString::fromAscii("0x%1").arg(frame.instructionPointer(), 0, 16));
    if (!frame.object().isEmpty())
        lines << qMakePair(QCoreApplication::translate("Valgrind::XmlProtocol", "Object:"), frame.object());

    QString html = "<html>"
                   "<head>"
                   "<style>dt { font-weight:bold; } dd { font-family: monospace; }</style>\n"
                   "<body><dl>";

    foreach (const StringPair &pair, lines) {
        html += QLatin1String("<dt>");
        html += pair.first;
        html += QLatin1String("</dt><dd>");
        html += pair.second;
        html += QLatin1String("</dd>\n");
    }
    html += "</dl></body></html>";
    return html;
}

} // namespace XmlProtocol
} // namespace Valgrind
