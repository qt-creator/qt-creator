// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QString>

namespace TerminalSolution {

class SurfaceIntegration
{
public:
    virtual void onOsc(int cmd, std::string_view str, bool initial, bool final) = 0;

    virtual void onBell() {}
    virtual void onTitle(const QString &title) { Q_UNUSED(title) }

    virtual void onSetClipboard(const QByteArray &text) { Q_UNUSED(text) }
    virtual void onGetClipboard() {}
};

} // namespace TerminalSolution
