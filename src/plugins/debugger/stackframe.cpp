/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "stackframe.h"
#include "stackhandler.h"

#include <QtCore/QFileInfo>
#include <QtCore/QDebug>
#include <QtCore/QDir>
#include <QtCore/QTextStream>

#include <QtGui/QToolTip>
#include <QtGui/QFontMetrics>

namespace Debugger {
namespace Internal {

////////////////////////////////////////////////////////////////////////
//
// StackFrame
//
////////////////////////////////////////////////////////////////////////

StackFrame::StackFrame()
  : level(0), line(0), address(0)
{}

void StackFrame::clear()
{
    line = level = 0;
    function.clear();
    file.clear();
    from.clear();
    to.clear();
    address = 0;
}

bool StackFrame::isUsable() const
{
    return !file.isEmpty() && QFileInfo(file).isReadable();
}

QString StackFrame::toString() const
{
    QString res;
    QTextStream str(&res);
    str << StackHandler::tr("Address:") << ' ';
    str.setIntegerBase(16);
    str << address;
    str.setIntegerBase(10);
    str << ' '
        << StackHandler::tr("Function:") << ' ' << function << ' '
        << StackHandler::tr("File:") << ' ' << file << ' '
        << StackHandler::tr("Line:") << ' ' << line << ' '
        << StackHandler::tr("From:") << ' ' << from << ' '
        << StackHandler::tr("To:") << ' ' << to;
    return res;
}

QString StackFrame::toToolTip() const
{
    const QString filePath = QDir::toNativeSeparators(file);
    QString res;
    QTextStream str(&res);
    str << "<html><body><table>"
        << "<tr><td>" << StackHandler::tr("Address:") << "</td><td>0x";
    str.setIntegerBase(16);
    str <<  address;
    str.setIntegerBase(10);
    str << "</td></tr>"
        << "<tr><td>" << StackHandler::tr("Function:") << "</td><td>" << function << "</td></tr>"
        << "<tr><td>" << StackHandler::tr("File:") << "</td><td width="
        << QFontMetrics(QToolTip::font()).width(filePath) << ">" << filePath << "</td></tr>"
        << "<tr><td>" << StackHandler::tr("Line:") << "</td><td>" << line << "</td></tr>"
        << "<tr><td>" << StackHandler::tr("From:") << "</td><td>" << from << "</td></tr>"
        << "<tr><td>" << StackHandler::tr("To:") << "</td><td>" << to << "</td></tr>"
        << "</table></body></html>";
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
