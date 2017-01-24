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

#pragma once

#include <coreplugin/id.h>

#include <QVersionNumber>
#include <QtGlobal>

QT_BEGIN_NAMESPACE
class QString;
QT_END_NAMESPACE

namespace Debugger { class DiagnosticLocation; }

namespace ClangStaticAnalyzer {
namespace Internal {

bool isClangExecutableUsable(const QString &filePath, QString *errorMessage = 0);

QString clangExecutableFromSettings(Core::Id toolchainType, bool *isValid);

QString createFullLocationString(const Debugger::DiagnosticLocation &location);

class ClangExecutableVersion : public QVersionNumber {
public:
    ClangExecutableVersion() : QVersionNumber(-1, -1, -1) {}
    ClangExecutableVersion(int major, int minor, int micro)
        : QVersionNumber(major, minor, micro) {}

    bool isValid() const
    {
        return majorVersion() >= 0 && minorVersion() >= 0 && microVersion() >= 0;
    }

    bool isSupportedVersion() const
    {
        return majorVersion() == 3 && minorVersion() == 9;
    }

    static QString supportedVersionAsString()
    {
        return QLatin1String("3.9");
    }
};

ClangExecutableVersion clangExecutableVersion(const QString &absolutePath);

} // namespace Internal
} // namespace ClangStaticAnalyzer
