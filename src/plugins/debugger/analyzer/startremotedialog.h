// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <debugger/debugger_global.h>

#include <QDialog>

namespace Utils {
class CommandLine;
class FilePath;
} // Utils

namespace Debugger {

namespace Internal { class StartRemoteDialogPrivate; }

class DEBUGGER_EXPORT StartRemoteDialog : public QDialog
{
public:
    explicit StartRemoteDialog(QWidget *parent = nullptr);
    ~StartRemoteDialog() override;

    Utils::CommandLine commandLine() const;
    Utils::FilePath workingDirectory() const;

private:
    void validate();
    void accept() override;

    Internal::StartRemoteDialogPrivate *d;
};

} // Debugger
