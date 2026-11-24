// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "terminaleditor_test.h"

#include "terminalconstants.h"
#include "terminalpane.h"
#include "terminalsettings.h"
#include "terminalwidget.h"

#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/editormanager/documentmodel.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/ieditor.h>
#include <coreplugin/icore.h>
#include <coreplugin/idocument.h>

#include <utils/hostosinfo.h>
#include <utils/qtcprocess.h>

#include <QApplication>
#include <QMainWindow>
#include <QPointer>
#include <QScopeGuard>
#include <QTest>
#include <QToolButton>

namespace Terminal::Internal {

static QList<Core::IEditor *> terminalEditors()
{
    QList<Core::IEditor *> editors = Core::DocumentModel::editorsForOpenedDocuments();
    editors.removeIf([](Core::IEditor *editor) {
        return editor->document()->id() != Constants::TERMINAL_EDITOR_ID;
    });
    return editors;
}

static bool activateMainWindow()
{
    Core::ICore::raiseWindow(Core::ICore::mainWindow());
    return QTest::qWaitFor(
        [] { return QApplication::activeWindow() == Core::ICore::mainWindow(); });
}

class TerminalEditorTest final : public QObject
{
    Q_OBJECT

public:
    explicit TerminalEditorTest(TerminalPane *pane)
        : m_pane(pane)
    {}

private slots:
    void init()
    {
        if (!Utils::HostOsInfo::isAnyUnixHost())
            QSKIP("The test runs /bin/cat as the terminal's command.");
        QVERIFY(m_pane);
        QVERIFY(terminalEditors().isEmpty());
    }

    void cleanup()
    {
        Core::EditorManager::closeEditors(terminalEditors(), false);
        QTRY_VERIFY(terminalEditors().isEmpty());
    }

    void testMoveToEditorAndBack()
    {
        TerminalWidget *terminal = startedTerminal();
        QVERIFY(terminal);
        const QPointer<TerminalWidget> guard(terminal);
        QWidget *paneWidget = m_pane->outputWidget(nullptr);

        QVERIFY(m_pane->moveTerminalToEditor(terminal));
        const QList<Core::IEditor *> editors = terminalEditors();
        QCOMPARE(editors.size(), 1);
        QVERIFY(editors.first()->widget()->isAncestorOf(terminal));
        QVERIFY(!paneWidget->isAncestorOf(terminal));
        QCOMPARE(terminal->processState(), Utils::ProcessState::Running);

        auto moveBack = editors.first()->toolBar()->findChild<QToolButton *>(
            "moveTerminalToPaneButton");
        QVERIFY(moveBack);
        moveBack->click();

        QTRY_VERIFY(terminalEditors().isEmpty());
        QVERIFY(guard);
        QVERIFY(paneWidget->isAncestorOf(terminal));
        QCOMPARE(terminal->processState(), Utils::ProcessState::Running);

        terminal->closeTerminal();
        QTRY_VERIFY(!guard);
    }

    void testEditorClosesWithTerminal()
    {
        TerminalWidget *terminal = startedTerminal();
        QVERIFY(terminal);
        const QPointer<TerminalWidget> guard(terminal);

        QVERIFY(m_pane->moveTerminalToEditor(terminal));
        QCOMPARE(terminalEditors().size(), 1);

        terminal->closeTerminal();
        QTRY_VERIFY(!guard);
        QTRY_VERIFY(terminalEditors().isEmpty());
    }

    void testClosingEditorEndsTerminal()
    {
        TerminalWidget *terminal = startedTerminal();
        QVERIFY(terminal);
        const QPointer<TerminalWidget> guard(terminal);

        QVERIFY(m_pane->moveTerminalToEditor(terminal));
        QCOMPARE(terminalEditors().size(), 1);

        Core::EditorManager::closeEditors(terminalEditors(), false);
        QTRY_VERIFY(!guard);
    }

    void testOpenInNewWindow()
    {
        TerminalWidget *terminal = startedTerminal();
        QVERIFY(terminal);
        const QPointer<TerminalWidget> guard(terminal);
        QVERIFY(m_pane->moveTerminalToEditor(terminal));
        Core::IEditor *editor = terminalEditors().value(0);
        QVERIFY(editor);

        // "Open in New Window" acts on the editor area, so it needs the focus.
        QWidget *mainWindow = Core::ICore::mainWindow();
        if (!activateMainWindow())
            QSKIP("The main window is not activated in this environment.");
        terminal->setFocus();
        QAction *newWindow
            = Core::ActionManager::command(Core::Constants::SPLIT_NEW_WINDOW)->action();
        QTRY_VERIFY(newWindow->isEnabled());

        newWindow->trigger();
        QTRY_VERIFY(terminal->window() != mainWindow);
        const QPointer<QWidget> window = terminal->window();
        QVERIFY(window->isVisible());
        QCOMPARE(terminal->processState(), Utils::ProcessState::Running);

        // Closing the window keeps the editor open, as for any other editor.
        window->close();
        QTRY_VERIFY(!window);
        QVERIFY(guard);
        QCOMPARE(terminalEditors(), {editor});
        QCOMPARE(terminal->processState(), Utils::ProcessState::Running);

        Core::EditorManager::activateEditor(editor);
        QCOMPARE(terminal->window(), mainWindow);
        QVERIFY(terminal->isVisible());
        QCOMPARE(terminal->processState(), Utils::ProcessState::Running);
    }

    void testZoomActions()
    {
        // The actions are enabled by the context of the focus widget.
        if (!activateMainWindow())
            QSKIP("The main window is not activated in this environment.");
        TerminalWidget *terminal = startedTerminal();
        QVERIFY(terminal);
        const int original = terminal->font().pointSize();

        m_pane->ensureVisible(terminal);
        QTRY_VERIFY(terminal->hasFocus());
        checkZoomActions(terminal, original);
        if (QTest::currentTestFailed())
            return;

        QVERIFY(m_pane->moveTerminalToEditor(terminal));
        QTRY_VERIFY(terminal->hasFocus());
        checkZoomActions(terminal, original);
    }

    void testZoomShortcuts_data()
    {
        QTest::addColumn<bool>("blockShortcuts");
        QTest::newRow("shortcuts blocked") << true;
        QTest::newRow("shortcuts not blocked") << false;
    }

    void testZoomShortcuts()
    {
        QFETCH(bool, blockShortcuts);
        if (!activateMainWindow())
            QSKIP("The main window is not activated in this environment.");

        const bool wasBlocked = settings().lockKeyboard();
        settings().lockKeyboard.setValue(blockShortcuts);
        const auto restore = qScopeGuard(
            [wasBlocked] { settings().lockKeyboard.setValue(wasBlocked); });

        TerminalWidget *terminal = startedTerminal();
        QVERIFY(terminal);
        m_pane->ensureVisible(terminal);
        QTRY_VERIFY(terminal->hasFocus());
        const int original = terminal->font().pointSize();

        pressShortcut(terminal, Core::Constants::ZOOM_IN);
        QCOMPARE(terminal->font().pointSize(), original + 1);
        pressShortcut(terminal, Core::Constants::ZOOM_OUT);
        pressShortcut(terminal, Core::Constants::ZOOM_OUT);
        QCOMPARE(terminal->font().pointSize(), original - 1);
        pressShortcut(terminal, Core::Constants::ZOOM_RESET);
        QCOMPARE(terminal->font().pointSize(), original);

        terminal->closeTerminal();
    }

private:
    static void pressShortcut(QWidget *widget, Utils::Id command)
    {
        const QKeySequence sequence = Core::ActionManager::command(command)->keySequence();
        QVERIFY2(!sequence.isEmpty(), qPrintable(command.toString()));
        const QKeyCombination keys = sequence[0];
        QTest::keyClick(widget, keys.key(), keys.keyboardModifiers());
    }

    static void checkZoomActions(TerminalWidget *terminal, int original)
    {
        QAction *zoomIn = Core::ActionManager::command(Core::Constants::ZOOM_IN)->action();
        QAction *zoomOut = Core::ActionManager::command(Core::Constants::ZOOM_OUT)->action();
        QAction *zoomReset = Core::ActionManager::command(Core::Constants::ZOOM_RESET)->action();
        QTRY_VERIFY(zoomIn->isEnabled());
        QVERIFY(zoomOut->isEnabled());
        QVERIFY(zoomReset->isEnabled());

        zoomIn->trigger();
        QCOMPARE(terminal->font().pointSize(), original + 1);
        zoomOut->trigger();
        zoomOut->trigger();
        QCOMPARE(terminal->font().pointSize(), original - 1);
        zoomReset->trigger();
        QCOMPARE(terminal->font().pointSize(), original);
    }

    TerminalWidget *startedTerminal()
    {
        auto terminal = new TerminalWidget(
            nullptr, Utils::CommandLine{Utils::FilePath::fromString("/bin/cat")});
        m_pane->addTerminal(terminal, "cat");
        if (!QTest::qWaitFor(
                [terminal] { return terminal->processState() == Utils::ProcessState::Running; })) {
            delete terminal;
            return nullptr;
        }
        return terminal;
    }

    QPointer<TerminalPane> m_pane;
};

QObject *createTerminalEditorTest(TerminalPane *pane)
{
    return new TerminalEditorTest(pane);
}

} // namespace Terminal::Internal

#include "terminaleditor_test.moc"
