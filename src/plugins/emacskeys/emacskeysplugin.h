// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <extensionsystem/iplugin.h>

#include <utils/id.h>

#include <QTextCursor>

QT_BEGIN_NAMESPACE
class QAction;
class QPlainTextEdit;
QT_END_NAMESPACE

namespace Core {
class IEditor;
}
namespace TextEditor {
class TextEditorWidget;
}

namespace EmacsKeys {
namespace Internal {

class EmacsKeysState;

class EmacsKeysPlugin : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "EmacsKeys.json")

public:
    EmacsKeysPlugin();
    ~EmacsKeysPlugin() override;

    void initialize() override;
    void extensionsInitialized() override;

private:
    void editorAboutToClose(Core::IEditor *editor);
    void currentEditorChanged(Core::IEditor *editor);

    void deleteCharacter();       // C-d
    void killWord();              // M-d
    void killLine();              // C-k
    void insertLineAndIndent();   // C-j

    void gotoFileStart();         // M-<
    void gotoFileEnd();           // M->
    void gotoLineStart();         // C-a
    void gotoLineEnd();           // C-e
    void gotoNextLine();          // C-n
    void gotoPreviousLine();      // C-p
    void gotoNextCharacter();     // C-f
    void gotoPreviousCharacter(); // C-b
    void gotoNextWord();          // M-f
    void gotoPreviousWord();      // M-b

    void mark();                  // C-SPC
    void exchangeCursorAndMark(); // C-x C-x
    void copy();                  // M-w
    void cut();                   // C-w
    void yank();                  // C-y

    void scrollHalfDown();        // C-v
    void scrollHalfUp();          // M-v

    QAction *registerAction(Utils::Id id, void (EmacsKeysPlugin::*callback)(),
                            const QString &title);
    void genericGoto(QTextCursor::MoveOperation op, bool abortAssist = true);
    void genericVScroll(int direction);

    QHash<QPlainTextEdit*, EmacsKeysState*> m_stateMap;
    QPlainTextEdit *m_currentEditorWidget = nullptr;
    EmacsKeysState *m_currentState = nullptr;
    TextEditor::TextEditorWidget *m_currentBaseTextEditorWidget = nullptr;
};

} // namespace Internal
} // namespace EmacsKeys
