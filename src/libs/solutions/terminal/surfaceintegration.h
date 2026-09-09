// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QFlags>
#include <QString>

namespace TerminalSolution {

enum class ClipboardTarget {
    Clipboard = 0x01,
    Selection = 0x02,
};
Q_DECLARE_FLAGS(ClipboardTargets, ClipboardTarget)

class SurfaceIntegration
{
public:
    virtual void onOsc(int cmd, std::string_view str, bool initial, bool final) = 0;

    virtual void onBell() {}
    virtual void onTitle(const QString &title) { Q_UNUSED(title) }

    virtual void onSetClipboard(const QByteArray &text, ClipboardTargets targets)
    {
        Q_UNUSED(text)
        Q_UNUSED(targets)
    }
};

} // namespace TerminalSolution

Q_DECLARE_OPERATORS_FOR_FLAGS(TerminalSolution::ClipboardTargets)
