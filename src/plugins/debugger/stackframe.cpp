/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "stackframe.h"
#include "watchutils.h"

#include <QFileInfo>
#include <QDebug>
#include <QDir>
#include <QTextStream>
#include <QCoreApplication>

namespace Debugger {
namespace Internal {

////////////////////////////////////////////////////////////////////////
//
// StackFrame
//
////////////////////////////////////////////////////////////////////////

StackFrame::StackFrame()
  : level(-1), line(-1), address(0), usable(false)
{}

void StackFrame::clear()
{
    line = level = -1;
    function.clear();
    file.clear();
    from.clear();
    to.clear();
    address = 0;
}

bool StackFrame::isUsable() const
{
    return usable;
}

QString StackFrame::toString() const
{
    QString res;
    QTextStream str(&res);
    str << tr("Address:") << ' ';
    str.setIntegerBase(16);
    str << address;
    str.setIntegerBase(10);
    str << ' '
        << tr("Function:") << ' ' << function << ' '
        << tr("File:") << ' ' << file << ' '
        << tr("Line:") << ' ' << line << ' '
        << tr("From:") << ' ' << from << ' '
        << tr("To:") << ' ' << to;
    return res;
}

QString StackFrame::toToolTip() const
{
    const QString filePath = QDir::toNativeSeparators(file);
    QString res;
    QTextStream str(&res);
    str << "<html><body><table>";
    if (address)
        str << "<tr><td>" << tr("Address:") << "</td><td>"
            << formatToolTipAddress(address) << "</td></tr>";
    if (!function.isEmpty())
        str << "<tr><td>" << tr("Function:") << "</td><td>" << function << "</td></tr>";
    if (!file.isEmpty())
        str << "<tr><td>" << tr("File:") << "</td><td>" << filePath << "</td></tr>";
    if (line != -1)
        str << "<tr><td>" << tr("Line:") << "</td><td>" << line << "</td></tr>";
    if (!from.isEmpty())
        str << "<tr><td>" << tr("From:") << "</td><td>" << from << "</td></tr>";
    if (!to.isEmpty())
        str << "<tr><td>" << tr("To:") << "</td><td>" << to << "</td></tr>";
    str << "</table>";

    str <<"<br> <br><i>" << tr("Note:") << " </i> ";
    if (isUsable()) {
        str << tr("Sources for this frame are available.<br>Double-click on "
            "the file name to open an editor.");
    } else if (line <= 0) {
        str << tr("Binary debug information is not accessible for this "
            "frame. This either means the core was not compiled "
            "with debug information, or the debug information is not "
            "accessible. Note that most distributions ship debug information "
            "in separate packages.");
    } else {
        str << tr("Binary debug information is accessible for this "
            "frame. However, matching sources have not been found. "
            "Note that some distributions ship debug sources "
            "in separate packages.");
    }

    str << "</body></html>";
    return res;
}

QDebug operator<<(QDebug d, const  StackFrame &f)
{
    QString res;
    QTextStream str(&res);
    str << "level=" << f.level << " address=" << f.address;
    if (!f.function.isEmpty())
        str << ' ' << f.function;
    if (!f.file.isEmpty())
        str << ' ' << f.file << ':' << f.line;
    if (!f.from.isEmpty())
        str << " from=" << f.from;
    if (!f.to.isEmpty())
        str << " to=" << f.to;
    d.nospace() << res;
    return d;
}

} // namespace Internal
} // namespace Debugger
