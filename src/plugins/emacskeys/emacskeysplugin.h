/****************************************************************************
**
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

#include <extensionsystem/iplugin.h>

#include <QTextCursor>

// forward declarations
QT_FORWARD_DECLARE_CLASS(QAction)
QT_FORWARD_DECLARE_CLASS(QPlainTextEdit)

namespace Core {
class Id;
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
    ~EmacsKeysPlugin();

    bool initialize(const QStringList &arguments, QString *errorString);
    void extensionsInitialized();
    ShutdownFlag aboutToShutdown();

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

    QAction *registerAction(Core::Id id, void (EmacsKeysPlugin::*callback)(),
                            const QString &title);
    void genericGoto(QTextCursor::MoveOperation op, bool abortAssist = true);
    void genericVScroll(int direction);

    QHash<QPlainTextEdit*, EmacsKeysState*> m_stateMap;
    QPlainTextEdit *m_currentEditorWidget;
    EmacsKeysState *m_currentState;
    TextEditor::TextEditorWidget *m_currentBaseTextEditorWidget;
};

} // namespace Internal
} // namespace EmacsKeys
