/**************************************************************************
**
** Copyright (c) nsf <no.smile.face@gmail.com>
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef EMACSKEYS_H
#define EMACSKEYS_H

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

private slots:
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

private:
    QAction *registerAction(Core::Id id, const char *slot,
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

#endif // EMACSKEYS_H
