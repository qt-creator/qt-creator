// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/filepath.h>

#include <QByteArray>
#include <QString>

namespace Remote {

// Helpers for running a single PowerShell command on the remote machine over SSH.
// We always go through PowerShell with -EncodedCommand: the script is base64 of its
// UTF-16LE representation, which side-steps all quoting issues across the local
// shell, ssh and the remote default shell (cmd.exe or PowerShell) layers.
inline QString encodePowerShellCommand(const QString &script)
{
    QByteArray utf16le;
    utf16le.reserve(script.size() * 2);
    for (const QChar c : script) {
        const ushort u = c.unicode();
        utf16le.append(char(u & 0xff));
        utf16le.append(char((u >> 8) & 0xff));
    }
    return QString::fromLatin1(utf16le.toBase64());
}

// Quotes a string for use inside a single-quoted PowerShell literal.
inline QString psQuote(const QString &str)
{
    QString result = str;
    result.replace('\'', "''");
    return '\'' + result + '\'';
}

inline QString psPath(const Utils::FilePath &filePath)
{
    return psQuote(filePath.nativePath());
}

} // namespace Remote
