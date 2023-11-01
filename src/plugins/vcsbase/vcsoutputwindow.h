// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "vcsbase_global.h"

#include <coreplugin/ioutputpane.h>

namespace Utils {
class CommandLine;
class FilePath;
} // Utils

namespace VcsBase {

namespace Internal { class VcsPlugin; }

class VCSBASE_EXPORT VcsOutputWindow : public Core::IOutputPane
{
    Q_OBJECT

public:
    QWidget *outputWidget(QWidget *parent) override;

    void clearContents() override;

    void setFocus() override;
    bool hasFocus() const override;
    bool canFocus() const override;

    bool canNavigate() const override;
    bool canNext() const override;
    bool canPrevious() const override;
    void goToNext() override;
    void goToPrev() override;

    static VcsOutputWindow *instance();

    // Helper to consistently format log entries for commands as
    // 'Executing <dir>: <cmd> <args>'. Hides well-known password option
    // arguments.
    static QString msgExecutionLogEntry(const Utils::FilePath &workingDir,
                                        const Utils::CommandLine &command);

    enum MessageStyle {
        None,
        Error, // Red error text
        Warning, // Dark yellow warning text
        Command, // A bold command with timestamp "10:00 " + "Executing: vcs -diff"
        Message, // A blue message text (e.g. "command has finished successfully")
    };

public slots:
    static void setRepository(const Utils::FilePath &repository);
    static void clearRepository();

    // Set the whole text.
    static void setText(const QString &text);
    // Set text from QProcess' output data using the Locale's converter.
    static void setData(const QByteArray &data);

    // Append text with a certain style (none by default),
    // and maybe pop up (silent by default)
    static void append(const QString &text, MessageStyle style = None, bool silently = false);

    // Silently append text, do not pop up.
    static void appendSilently(const QString &text);

    // Append red error text and pop up.
    static void appendError(const QString &text);

    // Append dark-yellow warning text and pop up.
    static void appendWarning(const QString &text);

    // Append a command, prepended by a log time stamp. "Executing: vcs -diff"
    // will result in "10:00 Executing: vcs -diff" in bold
    // Filter passwords from URLs while doing this.
    static void appendShellCommandLine(const QString &text);

    // Append a standard-formatted entry for command execution
    // (see msgExecutionLogEntry).
    static void appendCommand(const Utils::FilePath &workingDirectory,
                              const Utils::CommandLine &command);

    // Append a blue message text and pop up.
    static void appendMessage(const QString &text);

private:
    friend class Internal::VcsPlugin;
    static void destroy();

    VcsOutputWindow();
    ~VcsOutputWindow() override;
};

} // namespace VcsBase
