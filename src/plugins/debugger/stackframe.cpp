/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "stackframe.h"

#include "debuggerengine.h"
#include "debuggerprotocol.h"
#include "watchutils.h"

#include <QDebug>
#include <QDir>
#include <QFileInfo>

#include <utils/hostosinfo.h>

namespace Debugger {
namespace Internal {

////////////////////////////////////////////////////////////////////////
//
// StackFrame
//
////////////////////////////////////////////////////////////////////////

StackFrame::StackFrame()
  : language(CppLanguage), line(-1), address(0), usable(false)
{}

void StackFrame::clear()
{
    line = -1;
    function.clear();
    file.clear();
    module.clear();
    receiver.clear();
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
        << tr("From:") << ' ' << module << ' '
        << tr("To:") << ' ' << receiver;
    return res;
}

QList<StackFrame> StackFrame::parseFrames(const GdbMi &data, const DebuggerRunParameters &rp)
{
    StackFrames frames;
    frames.reserve(data.children().size());
    foreach (const GdbMi &item, data.children())
        frames.append(parseFrame(item, rp));
    return frames;
}

StackFrame StackFrame::parseFrame(const GdbMi &frameMi, const DebuggerRunParameters &rp)
{
    StackFrame frame;
    frame.level = frameMi["level"].data();
    frame.function = frameMi["function"].data();
    frame.module = frameMi["module"].data();
    frame.file = frameMi["file"].data();
    frame.line = frameMi["line"].toInt();
    frame.address = frameMi["address"].toAddress();
    frame.context = frameMi["context"].data();
    if (frameMi["language"].data() == "js"
            || frame.file.endsWith(QLatin1String(".js"))
            || frame.file.endsWith(QLatin1String(".qml"))) {
        frame.language = QmlLanguage;
        frame.fixQrcFrame(rp);
    }
    GdbMi usable = frameMi["usable"];
    if (usable.isValid())
        frame.usable = usable.data().toInt();
    else
        frame.usable = QFileInfo(frame.file).isReadable();
    return frame;
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
        str << "<tr><td>"
            << (language == CppLanguage ? tr("Function:") : tr("JS-Function:"))
            << "</td><td>" << function << "</td></tr>";
    if (!file.isEmpty())
        str << "<tr><td>" << tr("File:") << "</td><td>" << filePath << "</td></tr>";
    if (line != -1)
        str << "<tr><td>" << tr("Line:") << "</td><td>" << line << "</td></tr>";
    if (!module.isEmpty())
        str << "<tr><td>" << tr("Module:") << "</td><td>" << module << "</td></tr>";
    if (!receiver.isEmpty())
        str << "<tr><td>" << tr("Receiver:") << "</td><td>" << receiver << "</td></tr>";
    str << "</table>";

    str <<"<br> <br><i>" << tr("Note:") << " </i> ";
    bool showDistributionNote = false;
    if (isUsable()) {
        str << tr("Sources for this frame are available.<br>Double-click on "
            "the file name to open an editor.");
    } else if (line <= 0) {
        str << tr("Binary debug information is not accessible for this "
            "frame. This either means the core was not compiled "
            "with debug information, or the debug information is not "
            "accessible.");
        showDistributionNote = true;
    } else {
        str << tr("Binary debug information is accessible for this "
            "frame. However, matching sources have not been found.");
        showDistributionNote = true;
    }
    if (!Utils::HostOsInfo::isWindowsHost() && showDistributionNote) {
        str << QLatin1Char(' ') <<
               tr("Note that most distributions ship debug information "
                  "in separate packages.");
    }

    str << "</body></html>";
    return res;
}

static QString findFile(const QString &baseDir, const QString &relativeFile)
{
    QDir dir(baseDir);
    while (true) {
        const QString path = dir.absoluteFilePath(relativeFile);
        const QFileInfo fi(path);
        if (fi.isFile())
            return path;
        if (dir.isRoot())
            break;
        dir.cdUp();
    }
    return QString();
}

// Try to resolve files coming from resource files.
void StackFrame::fixQrcFrame(const DebuggerRunParameters &rp)
{
    if (language != QmlLanguage)
        return;
    QFileInfo aFi(file);
    if (aFi.isAbsolute()) {
        usable = aFi.isFile();
        return;
    }
    if (!file.startsWith(QLatin1String("qrc:/")))
        return;

    QString relativeFile = file.right(file.size() - 5);
    while (relativeFile.startsWith(QLatin1Char('/')))
        relativeFile = relativeFile.mid(1);

    QString absFile = findFile(rp.projectSourceDirectory, relativeFile);
    if (absFile.isEmpty())
        absFile = findFile(QDir::currentPath(), relativeFile);

    if (absFile.isEmpty())
        return;
    file = absFile;
    usable = true;
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
    if (!f.module.isEmpty())
        str << " from=" << f.module;
    if (!f.receiver.isEmpty())
        str << " to=" << f.receiver;
    d.nospace() << res;
    return d;
}

} // namespace Internal
} // namespace Debugger
