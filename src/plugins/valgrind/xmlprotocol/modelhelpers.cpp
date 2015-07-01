/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
** Author: Milian Wolff, KDAB (milian.wolff@kdab.com)
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
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
            location += QLatin1Char(':') + QString::number(frame.line());
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
                           QString::fromLatin1("0x%1").arg(frame.instructionPointer(), 0, 16));
    if (!frame.object().isEmpty())
        lines << qMakePair(QCoreApplication::translate("Valgrind::XmlProtocol", "Object:"), frame.object());

    QString html = QLatin1String("<html>"
                   "<head>"
                   "<style>dt { font-weight:bold; } dd { font-family: monospace; }</style>\n"
                   "<body><dl>");

    foreach (const StringPair &pair, lines) {
        html += QLatin1String("<dt>");
        html += pair.first;
        html += QLatin1String("</dt><dd>");
        html += pair.second;
        html += QLatin1String("</dd>\n");
    }
    html += QLatin1String("</dl></body></html>");
    return html;
}

} // namespace XmlProtocol
} // namespace Valgrind
