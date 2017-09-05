/****************************************************************************
**
** Copyright (C) 2016 Lorenz Haas
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

#include "command.h"

#include <extensionsystem/iplugin.h>

#include <QPlainTextEdit>
#include <QPointer>
#include <QSharedPointer>

namespace Core {
class IDocument;
class IEditor;
}
namespace TextEditor {class TextEditorWidget;}

namespace Beautifier {
namespace Internal {

class BeautifierAbstractTool;
class GeneralSettings;

struct FormatTask
{
    FormatTask(QPlainTextEdit *_editor, const QString &_filePath, const QString &_sourceData,
               const Command &_command, int _startPos = -1, int _endPos = 0) :
        editor(_editor),
        filePath(_filePath),
        sourceData(_sourceData),
        command(_command),
        startPos(_startPos),
        endPos(_endPos) {}

    QPointer<QPlainTextEdit> editor;
    QString filePath;
    QString sourceData;
    Command command;
    int startPos = -1;
    int endPos = 0;
    QString formattedData;
    QString error;
};

class BeautifierPlugin : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "Beautifier.json")

public:
    bool initialize(const QStringList &arguments, QString *errorString) override;
    void extensionsInitialized() override;

    void formatCurrentFile(const Command &command, int startPos = -1, int endPos = 0);

    static QString msgCannotGetConfigurationFile(const QString &command);
    static QString msgFormatCurrentFile();
    static QString msgFormatSelectedText();
    static QString msgFormatAtCursor();
    static QString msgDisableFormattingSelectedText();
    static QString msgCommandPromptDialogTitle(const QString &command);
    static void showError(const QString &error);

private:
    void updateActions(Core::IEditor *editor = nullptr);
    QList<BeautifierAbstractTool *> m_tools;
    QSharedPointer<GeneralSettings> m_generalSettings;
    QHash<QObject*, QMetaObject::Connection> m_autoFormatConnections;
    void formatEditor(TextEditor::TextEditorWidget *editor, const Command &command,
                      int startPos = -1, int endPos = 0);
    void formatEditorAsync(TextEditor::TextEditorWidget *editor, const Command &command,
                           int startPos = -1, int endPos = 0);
    void checkAndApplyTask(const FormatTask &task);
    void updateEditorText(QPlainTextEdit *editor, const QString &text);

    void autoFormatOnSave(Core::IDocument *document);
};

} // namespace Internal
} // namespace Beautifier
