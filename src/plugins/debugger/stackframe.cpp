// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "stackframe.h"

#include "debuggerengine.h"
#include "debuggerprotocol.h"
#include "debuggertr.h"
#include "watchutils.h"

#include <QDebug>
#include <QDir>
#include <QFileInfo>

#include <utils/hostosinfo.h>

using namespace Utils;

namespace Debugger::Internal {

////////////////////////////////////////////////////////////////////////
//
// StackFrame
//
////////////////////////////////////////////////////////////////////////

StackFrame::StackFrame() = default;

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
    str << Tr::tr("Address:") << ' ';
    str.setIntegerBase(16);
    str << address;
    str.setIntegerBase(10);
    str << ' '
        << Tr::tr("Function:") << ' ' << function << ' '
        << Tr::tr("File:") << ' ' << file << ' '
        << Tr::tr("Line:") << ' ' << line << ' '
        << Tr::tr("From:") << ' ' << module << ' '
        << Tr::tr("To:") << ' ' << receiver;
    return res;
}

QList<StackFrame> StackFrame::parseFrames(const GdbMi &data, const DebuggerRunParameters &rp)
{
    StackFrames frames;
    frames.reserve(data.childCount());
    for (const GdbMi &item : data)
        frames.append(parseFrame(item, rp));
    return frames;
}

StackFrame StackFrame::parseFrame(const GdbMi &frameMi, const DebuggerRunParameters &rp)
{
    StackFrame frame;
    frame.level = frameMi["level"].data();
    frame.function = frameMi["function"].data();
    frame.module = frameMi["module"].data();
    const FilePath debugger = rp.debugger.command.executable();
    const FilePath onDevicePath = debugger.withNewPath(frameMi["file"].data()).cleanPath();
    frame.file = onDevicePath.localSource().value_or(onDevicePath);
    frame.line = frameMi["line"].toInt();
    frame.address = frameMi["address"].toAddress();
    frame.context = frameMi["context"].data();
    if (frameMi["language"].data() == "js"
            || frame.file.endsWith(".js")
            || frame.file.endsWith(".qml")) {
        frame.language = QmlLanguage;
        frame.fixQrcFrame(rp);
    }
    GdbMi usable = frameMi["usable"];
    if (usable.isValid())
        frame.usable = usable.data().toInt();
    else
        frame.usable = frame.file.isReadableFile();
    return frame;
}

QString StackFrame::toToolTip() const
{
    const QString filePath = file.toUserOutput();
    QString res;
    QTextStream str(&res);
    str << "<html><body><table>";
    if (address)
        str << "<tr><td>" << Tr::tr("Address:") << "</td><td>"
            << formatToolTipAddress(address) << "</td></tr>";
    if (!function.isEmpty())
        str << "<tr><td>"
            << (language == CppLanguage ? Tr::tr("Function:") : Tr::tr("JS-Function:"))
            << "</td><td>" << function << "</td></tr>";
    if (!file.isEmpty())
        str << "<tr><td>" << Tr::tr("File:") << "</td><td>" << filePath << "</td></tr>";
    if (line != -1)
        str << "<tr><td>" << Tr::tr("Line:") << "</td><td>" << line << "</td></tr>";
    if (!module.isEmpty())
        str << "<tr><td>" << Tr::tr("Module:") << "</td><td>" << module << "</td></tr>";
    if (!receiver.isEmpty())
        str << "<tr><td>" << Tr::tr("Receiver:") << "</td><td>" << receiver << "</td></tr>";
    str << "</table>";

    str <<"<br> <br><i>" << Tr::tr("Note:") << " </i> ";
    bool showDistributionNote = false;
    if (isUsable()) {
        str << Tr::tr("Sources for this frame are available.<br>Double-click on "
            "the file name to open an editor.");
    } else if (line <= 0) {
        str << Tr::tr("Binary debug information is not accessible for this "
            "frame. This either means the core was not compiled "
            "with debug information, or the debug information is not "
            "accessible.");
        showDistributionNote = true;
    } else {
        str << Tr::tr("Binary debug information is accessible for this "
            "frame. However, matching sources have not been found.");
        showDistributionNote = true;
    }
    if (file.osType() != OsTypeWindows  && showDistributionNote) {
        str << ' ' << Tr::tr("Note that most distributions ship debug information "
                         "in separate packages.");
    }

    str << "</body></html>";
    return res;
}

static FilePath findFile(const FilePath &baseDir, const FilePath &relativeFile)
{
    for (FilePath dir(baseDir); !dir.isEmpty(); dir = dir.parentDir()) {
        const FilePath absolutePath = dir.resolvePath(relativeFile);
        if (absolutePath.isFile())
            return absolutePath;
    }
    return {};
}

// Try to resolve files coming from resource files.
void StackFrame::fixQrcFrame(const DebuggerRunParameters &rp)
{
    if (language != QmlLanguage)
        return;
    if (file.isAbsolutePath()) {
        usable = file.isFile();
        return;
    }
    if (!file.startsWith("qrc:/"))
        return;

    FilePath relativeFile = file;
    QString relativePath = file.path();
    relativePath = relativePath.right(relativePath.size() - 5);
    while (relativePath.startsWith('/'))
        relativePath = relativePath.mid(1);
    relativeFile = relativeFile.withNewPath(relativePath);

    FilePath absFile = findFile(rp.projectSourceDirectory, relativeFile);
    if (absFile.isEmpty())
        absFile = findFile(FilePath::fromString(QDir::currentPath()), relativeFile);

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

} // Debugger::Internal
