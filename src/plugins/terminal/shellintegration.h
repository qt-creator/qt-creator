// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH
// Qt-GPL-exception-1.0

#pragma once

#include <utils/commandline.h>
#include <utils/qtcprocess.h>

#include <vterm.h>

#include <QTemporaryDir>

namespace Terminal {

class ShellIntegration : public QObject
{
    Q_OBJECT
public:
    static bool canIntegrate(const Utils::CommandLine &cmdLine);

    void onOsc(int cmd, const VTermStringFragment &fragment);

    void prepareProcess(Utils::QtcProcess &process);

signals:
    void commandChanged(const Utils::CommandLine &command);
    void currentDirChanged(const QString &dir);

private:
    QTemporaryDir m_tempDir;
};

} // namespace Terminal
