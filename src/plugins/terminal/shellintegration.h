// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH
// Qt-GPL-exception-1.0

#pragma once

#include <utils/commandline.h>
#include <utils/process.h>

#include <solutions/terminal/surfaceintegration.h>

#include <QTemporaryDir>

namespace Terminal {

class ShellIntegration : public QObject, public TerminalSolution::SurfaceIntegration
{
    Q_OBJECT
public:
    static bool canIntegrate(const Utils::CommandLine &cmdLine);

    void onOsc(int cmd, std::string_view str, bool initial, bool final) override;
    void onBell() override;
    void onTitle(const QString &title) override;

    void onSetClipboard(const QByteArray &text) override;

    void prepareProcess(Utils::Process &process);

signals:
    void commandChanged(const Utils::CommandLine &command);
    void currentDirChanged(const QString &dir);
    void titleChanged(const QString &title);

private:
    QTemporaryDir m_tempDir;
    QByteArray m_oscBuffer;
};

} // namespace Terminal
