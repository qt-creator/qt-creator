// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "terminaleditor.h"

#include "terminalconstants.h"
#include "terminalsettings.h"
#include "terminaltr.h"
#include "terminalwidget.h"

#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/editormanager/documentmodel.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/ieditor.h>
#include <coreplugin/editormanager/ieditorfactory.h>
#include <coreplugin/idocument.h>

#include <utils/qtcassert.h>
#include <utils/utilsicons.h>

#include <QHBoxLayout>
#include <QPointer>
#include <QToolButton>
#include <QVBoxLayout>

#include <memory>
#include <utility>

using namespace Core;
using namespace Utils;

namespace Terminal::Internal {

static TerminalWidget *s_pendingTerminal = nullptr;
static std::function<void(TerminalWidget *)> s_moveToPane;

// Nothing to load, save or reload: the terminal lives in its widget, not in a file.
class TerminalDocument final : public IDocument
{
public:
    TerminalDocument()
    {
        setId(Constants::TERMINAL_EDITOR_ID);
        // The shell does not survive the session, so restoring the editor
        // would only bring back a different one.
        setTemporary(true);
    }

    Result<> setContents(const QByteArray &contents) final
    {
        QTC_CHECK(contents.isEmpty());
        return ResultOk;
    }

    ReloadBehavior reloadBehavior(ChangeTrigger, ChangeType) const final
    {
        return BehaviorSilent;
    }
};

// Gets the focus when the editor is activated and passes it on to the terminal.
class TerminalEditorWidget final : public QWidget
{
public:
    void setTerminal(TerminalWidget *terminal) { m_terminal = terminal; }

protected:
    void focusInEvent(QFocusEvent *event) final
    {
        if (m_terminal)
            m_terminal->setFocus();
        else
            QWidget::focusInEvent(event);
    }

private:
    QPointer<TerminalWidget> m_terminal;
};

static QToolButton *toolButton(QAction *action)
{
    auto button = new QToolButton;
    button->setDefaultAction(action);
    return button;
}

class TerminalEditor final : public IEditor
{
public:
    explicit TerminalEditor(TerminalWidget *terminal)
        : m_document(std::make_unique<TerminalDocument>())
        , m_container(new TerminalEditorWidget)
        , m_toolBar(new QWidget)
        , m_terminal(terminal)
    {
        auto containerLayout = new QVBoxLayout(m_container);
        containerLayout->setContentsMargins(0, 0, 0, 0);
        containerLayout->setSpacing(0);
        containerLayout->addWidget(terminal);
        terminal->show();
        m_container->setTerminal(terminal);

        setWidget(m_container);
        setContext(Context(Constants::TERMINAL_EDITOR_ID));
        setDuplicateSupported(false);

        auto toolBarLayout = new QHBoxLayout(m_toolBar);
        toolBarLayout->setContentsMargins(0, 0, 0, 0);
        toolBarLayout->setSpacing(0);
        if (Command *lockKeyboard = ActionManager::command(Constants::TOGGLE_KEYBOARD_LOCK))
            toolBarLayout->addWidget(toolButton(lockKeyboard->action()));
        toolBarLayout->addWidget(toolButton(settings().sendEscapeToTerminal.action()));
        toolBarLayout->addStretch();

        auto moveButton = new QToolButton;
        moveButton->setObjectName("moveTerminalToPaneButton");
        moveButton->setIcon(Icons::COLLAPSE_TOOLBAR.icon());
        moveButton->setToolTip(Tr::tr("Move Terminal to Terminal Pane"));
        // Queued: moving closes the editor, which deletes the button.
        connect(moveButton, &QToolButton::clicked, this, &TerminalEditor::moveToPane,
                Qt::QueuedConnection);
        toolBarLayout->addWidget(moveButton);

        connect(terminal, &TerminalWidget::started, this, &TerminalEditor::updateTitle);
        connect(terminal, &TerminalWidget::cwdChanged, this, &TerminalEditor::updateTitle);
        connect(terminal, &TerminalWidget::commandChanged, this, &TerminalEditor::updateTitle);
        connect(terminal, &TerminalWidget::titleChanged, this, &TerminalEditor::updateTitle);
        // The terminal deletes itself when its shell exits or it is closed.
        // Closing the editor from within that deletion would delete the
        // terminal's parent while the terminal is still being destroyed.
        connect(terminal, &QObject::destroyed, this, [this] {
            EditorManager::closeEditors({this}, false);
        }, Qt::QueuedConnection);
        updateTitle();
    }

    ~TerminalEditor() final
    {
        if (m_terminal)
            disconnect(m_terminal, nullptr, this, nullptr);
        delete m_container;
        delete m_toolBar;
    }

    IDocument *document() const final { return m_document.get(); }
    QWidget *toolBar() final { return m_toolBar; }

private:
    void updateTitle()
    {
        const QString title = m_terminal ? m_terminal->title() : QString();
        m_document->setPreferredDisplayName(title.isEmpty() ? Tr::tr("Terminal") : title);
    }

    void moveToPane()
    {
        QTC_ASSERT(m_terminal, return);
        TerminalWidget *terminal = m_terminal;
        m_terminal = nullptr;
        disconnect(terminal, nullptr, this, nullptr);

        m_container->layout()->removeWidget(terminal);
        terminal->setParent(nullptr);
        m_container->setTerminal(nullptr);

        if (QTC_GUARD(s_moveToPane))
            s_moveToPane(terminal);
        else
            delete terminal;
        EditorManager::closeEditors({this}, false);
    }

    std::unique_ptr<TerminalDocument> m_document;
    TerminalEditorWidget *m_container = nullptr;
    QWidget *m_toolBar = nullptr;
    QPointer<TerminalWidget> m_terminal;
};

class TerminalEditorFactory final : public IEditorFactory
{
public:
    TerminalEditorFactory()
    {
        setId(Constants::TERMINAL_EDITOR_ID);
        setDisplayName(Tr::tr("Terminal"));
        setEditorCreator([] {
            TerminalWidget *terminal = std::exchange(s_pendingTerminal, nullptr);
            if (!terminal)
                terminal = new TerminalWidget;
            return new TerminalEditor(terminal);
        });
    }
};

/*!
    \internal

    Registers the editor that shows a terminal in the editor area. \a moveToPane
    takes over a terminal whose editor is moved back to the terminal pane.
*/
void setupTerminalEditor(const std::function<void(TerminalWidget *)> &moveToPane)
{
    static TerminalEditorFactory theTerminalEditorFactory;
    s_moveToPane = moveToPane;
}

/*!
    \internal

    Opens an editor that takes over \a terminal. Returns \c false if no editor
    was created, in which case the caller still owns \a terminal.
*/
bool openTerminalInEditor(TerminalWidget *terminal)
{
    QTC_ASSERT(terminal, return false);
    s_pendingTerminal = terminal;
    IEditor *editor = EditorManager::openEditorWithContents(Constants::TERMINAL_EDITOR_ID);
    if (std::exchange(s_pendingTerminal, nullptr))
        return false;
    QTC_CHECK(editor);
    terminal->setFocus();
    return true;
}

/*!
    \internal

    Closes the terminal editors, which ends their shells.
*/
void closeAllTerminalEditors()
{
    QList<IDocument *> documents = DocumentModel::openedDocuments();
    documents.removeIf([](IDocument *document) {
        return document->id() != Constants::TERMINAL_EDITOR_ID;
    });
    if (!documents.isEmpty())
        EditorManager::closeDocuments(documents, false);
}

} // namespace Terminal::Internal
