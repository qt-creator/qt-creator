// Copyright (C) 2016 Lorenz Haas
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <extensionsystem/iplugin.h>
#include <texteditor/command.h>

namespace Beautifier {
namespace Internal {

class BeautifierPlugin : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "Beautifier.json")

public:
    static QString msgCannotGetConfigurationFile(const QString &command);
    static QString msgFormatCurrentFile();
    static QString msgFormatSelectedText();
    static QString msgFormatAtCursor();
    static QString msgFormatLines();
    static QString msgDisableFormattingSelectedText();
    static QString msgCommandPromptDialogTitle(const QString &command);
    static void showError(const QString &error);

private:
    bool initialize(const QStringList &arguments, QString *errorString) override;
    void extensionsInitialized() override;
    ShutdownFlag aboutToShutdown() override;
};

} // namespace Internal
} // namespace Beautifier
