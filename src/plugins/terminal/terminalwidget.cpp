// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "terminalwidget.h"
#include "terminalconstants.h"
#include "terminalsettings.h"
#include "terminaltr.h"

#include <aggregation/aggregate.h>

#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/fileutils.h>
#include <coreplugin/find/textfindconstants.h>
#include <coreplugin/icore.h>
#include <coreplugin/messagemanager.h>

#include <utils/algorithm.h>
#include <utils/async.h>
#include <utils/environment.h>
#include <utils/fileutils.h>
#include <utils/hostosinfo.h>
#include <utils/processinterface.h>
#include <utils/proxyaction.h>
#include <utils/stringutils.h>

#include <QApplication>
#include <QCache>
#include <QClipboard>
#include <QDesktopServices>
#include <QElapsedTimer>
#include <QGlyphRun>
#include <QLoggingCategory>
#include <QMenu>
#include <QMimeData>
#include <QPaintEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPixmapCache>
#include <QRawFont>
#include <QRegularExpression>
#include <QScrollBar>
#include <QTextItem>
#include <QTextLayout>
#include <QToolTip>

using namespace Utils;
using namespace Utils::Terminal;
using namespace Core;

namespace Terminal {
TerminalWidget::TerminalWidget(QWidget *parent, const OpenTerminalParameters &openParameters)
    : Core::SearchableTerminal(parent)
    , m_context(Utils::Id("TerminalWidget_").withSuffix(QString::number((uintptr_t) this)))
    , m_openParameters(openParameters)
{
    auto contextObj = new IContext(this);
    contextObj->setWidget(this);
    contextObj->setContext(m_context);
    ICore::addContextObject(contextObj);

    setupFont();
    setupColors();
    setupActions();

    surfaceChanged();

    setAllowBlinkingCursor(settings().allowBlinkingCursor());

    connect(&settings(), &AspectContainer::applied, this, [this] {
        // Setup colors first, as setupFont will redraw the screen.
        setupColors();
        setupFont();
        configBlinkTimer();
        setAllowBlinkingCursor(settings().allowBlinkingCursor());
    });
}

void TerminalWidget::setupPty()
{
    m_process = std::make_unique<Process>();

    CommandLine shellCommand = m_openParameters.shellCommand.value_or(
        CommandLine{settings().shell(), settings().shellArguments(), CommandLine::Raw});

    if (shellCommand.executable().isRootPath()) {
        writeToTerminal((Tr::tr("Connecting...") + "\r\n").toUtf8(), true);
        // We still have to find the shell to start ...
        m_findShellWatcher.reset(new QFutureWatcher<expected_str<FilePath>>());
        connect(m_findShellWatcher.get(), &QFutureWatcher<FilePath>::finished, this, [this] {
            const expected_str<FilePath> result = m_findShellWatcher->result();
            if (result) {
                m_openParameters.shellCommand->setExecutable(*result);
                restart(m_openParameters);
                return;
            }

            writeToTerminal(("\r\n\033[31m"
                             + Tr::tr("Failed to start shell: %1").arg(result.error()) + "\r\n")
                                .toUtf8(),
                            true);
        });

        m_findShellWatcher->setFuture(Utils::asyncRun([shellCommand]() -> expected_str<FilePath> {
            const expected_str<FilePath> result = Utils::Terminal::defaultShellForDevice(
                shellCommand.executable());
            if (result && !result->isExecutableFile())
                return make_unexpected(
                    Tr::tr("\"%1\" is not executable.").arg(result->toUserOutput()));
            return result;
        }));

        return;
    }

    Environment env = m_openParameters.environment.value_or(Environment{})
                          .appliedToEnvironment(shellCommand.executable().deviceEnvironment());

    // Set some useful defaults
    env.setFallback("TERM", "xterm-256color");
    env.setFallback("TERM_PROGRAM", QCoreApplication::applicationName());
    env.setFallback("COLORTERM", "truecolor");
    env.setFallback("COMMAND_MODE", "unix2003");
    env.setFallback("INIT_CWD", QCoreApplication::applicationDirPath());

    // For git bash on Windows
    env.prependOrSetPath(shellCommand.executable().parentDir());
    if (env.hasKey("CLINK_NOAUTORUN"))
        env.unset("CLINK_NOAUTORUN");

    m_process->setProcessMode(ProcessMode::Writer);
    Utils::Pty::Data data;
    data.setPtyInputFlagsChangedHandler([this](Pty::PtyInputFlag flags) {
        const bool password = (flags & Pty::InputModeHidden);
        setPasswordMode(password);
    });
    m_process->setPtyData(data);
    m_process->setCommand(shellCommand);
    if (m_openParameters.workingDirectory.has_value())
        m_process->setWorkingDirectory(*m_openParameters.workingDirectory);
    m_process->setEnvironment(env);

    if (m_shellIntegration)
        m_shellIntegration->prepareProcess(*m_process.get());

    connect(m_process.get(), &Process::readyReadStandardOutput, this, [this]() {
        onReadyRead(false);
    });

    connect(m_process.get(), &Process::done, this, [this] {
        QString errorMessage;

        if (m_process) {
            if (m_process->exitCode() != 0) {
                errorMessage
                    = Tr::tr("Terminal process exited with code %1").arg(m_process->exitCode());

                if (!m_process->errorString().isEmpty())
                    errorMessage += QString(" (%1)").arg(m_process->errorString());
            }
        }

        if (m_openParameters.m_exitBehavior == ExitBehavior::Restart) {
            QMetaObject::invokeMethod(
                this,
                [this] {
                    m_process.reset();
                    setupSurface();
                    setupPty();
                },
                Qt::QueuedConnection);
        }

        if (m_openParameters.m_exitBehavior == ExitBehavior::Close)
            deleteLater();

        if (m_openParameters.m_exitBehavior == ExitBehavior::Keep) {
            if (!errorMessage.isEmpty()) {
                QByteArray msg = QString("\r\n\033[31m%1").arg(errorMessage).toUtf8();

                writeToTerminal(msg, true);
            } else {
                QString exitMsg = Tr::tr("Process exited with code: %1")
                                      .arg(m_process ? m_process->exitCode() : -1);
                QByteArray msg = QString("\r\n%1").arg(exitMsg).toUtf8();
                writeToTerminal(msg, true);
            }
        } else if (!errorMessage.isEmpty()) {
            Core::MessageManager::writeFlashing(errorMessage);
        }
    });

    connect(m_process.get(), &Process::started, this, [this] {
        if (m_shellName.isEmpty())
            m_shellName = m_process->commandLine().executable().fileName();
        if (HostOsInfo::isWindowsHost() && m_shellName.endsWith(QTC_WIN_EXE_SUFFIX))
            m_shellName.chop(QStringLiteral(QTC_WIN_EXE_SUFFIX).size());

        applySizeChange();
        emit started(m_process->processId());
    });

    m_process->start();
}

void TerminalWidget::setupFont()
{
    QFont f;
    f.setFixedPitch(true);
    f.setFamily(settings().font());
    f.setPointSize(settings().fontSize());

    setFont(f);
}

void TerminalWidget::setupColors()
{
    // Check if the colors have changed.
    std::array<QColor, 20> newColors;
    for (int i = 0; i < 16; ++i) {
        newColors[i] = settings().colors[i]();
    }
    newColors[(size_t) WidgetColorIdx::Background] = settings().backgroundColor.value();
    newColors[(size_t) WidgetColorIdx::Foreground] = settings().foregroundColor.value();
    newColors[(size_t) WidgetColorIdx::Selection] = settings().selectionColor.value();
    newColors[(size_t) WidgetColorIdx::FindMatch] = settings().findMatchColor.value();

    setColors(newColors);
}

static bool contextMatcher(QObject *, Qt::ShortcutContext)
{
    return true;
}

void TerminalWidget::registerShortcut(Command *cmd)
{
    QTC_ASSERT(cmd, return);
    auto addShortCut = [this, cmd] {
        for (const auto &keySequence : cmd->keySequences()) {
            if (!keySequence.isEmpty()) {
                m_shortcutMap.addShortcut(cmd->action(),
                                          keySequence,
                                          Qt::ShortcutContext::WindowShortcut,
                                          contextMatcher);
            }
        }
    };
    auto removeShortCut = [this, cmd] { m_shortcutMap.removeShortcut(0, cmd->action()); };
    addShortCut();

    connect(cmd, &Command::keySequenceChanged, this, [addShortCut, removeShortCut]() {
        removeShortCut();
        addShortCut();
    });
}

RegisteredAction TerminalWidget::registerAction(Id commandId, const Context &context)
{
    QAction *action = new QAction;
    Command *cmd = ActionManager::registerAction(action, commandId, context);

    registerShortcut(cmd);

    return RegisteredAction(action, [commandId](QAction *a) {
        ActionManager::unregisterAction(a, commandId);
        delete a;
    });
}

void TerminalWidget::setupActions()
{
    m_copy = registerAction(Constants::COPY, m_context);
    m_paste = registerAction(Constants::PASTE, m_context);
    m_close = registerAction(Core::Constants::CLOSE, m_context);
    m_clearTerminal = registerAction(Constants::CLEAR_TERMINAL, m_context);
    m_clearSelection = registerAction(Constants::CLEARSELECTION, m_context);
    m_moveCursorWordLeft = registerAction(Constants::MOVECURSORWORDLEFT, m_context);
    m_moveCursorWordRight = registerAction(Constants::MOVECURSORWORDRIGHT, m_context);

    connect(m_copy.get(), &QAction::triggered, this, &TerminalWidget::copyToClipboard);
    connect(m_paste.get(), &QAction::triggered, this, &TerminalWidget::pasteFromClipboard);
    connect(m_close.get(), &QAction::triggered, this, &TerminalWidget::closeTerminal);
    connect(m_clearTerminal.get(), &QAction::triggered, this, &TerminalWidget::clearContents);
    connect(m_clearSelection.get(), &QAction::triggered, this, &TerminalWidget::clearSelection);
    connect(m_moveCursorWordLeft.get(),
            &QAction::triggered,
            this,
            &TerminalWidget::moveCursorWordLeft);
    connect(m_moveCursorWordRight.get(),
            &QAction::triggered,
            this,
            &TerminalWidget::moveCursorWordRight);

    // Ctrl+Q, the default "Quit" shortcut, is a useful key combination in a shell.
    // It can be used in combination with Ctrl+S to pause a program, and resume it with Ctrl+Q.
    // So we unlock the EXIT command only for macOS where the default is Cmd+Q to quit.
    if (HostOsInfo::isMacHost())
        unlockGlobalAction(Core::Constants::EXIT);
    unlockGlobalAction(Core::Constants::OPTIONS);
    unlockGlobalAction("Preferences.Terminal.General");
    unlockGlobalAction(Core::Constants::FIND_IN_DOCUMENT);
}

void TerminalWidget::closeTerminal()
{
    deleteLater();
}

qint64 TerminalWidget::writeToPty(const QByteArray &data)
{
    if (m_process && m_process->isRunning())
        return m_process->writeRaw(data);

    return data.size();
}

void TerminalWidget::resizePty(QSize newSize)
{
    if (m_process && m_process->ptyData())
        m_process->ptyData()->resize(newSize);
}

void TerminalWidget::surfaceChanged()
{
    Core::SearchableTerminal::surfaceChanged();

    m_shellIntegration.reset(new ShellIntegration());
    setSurfaceIntegration(m_shellIntegration.get());

    connect(m_shellIntegration.get(),
            &ShellIntegration::titleChanged,
            this,
            [this](const QString &title) {
                const FilePath titleFile = FilePath::fromUserInput(title);
                if (!m_title.isEmpty()
                    || m_openParameters.shellCommand.value_or(CommandLine{}).executable()
                           != titleFile) {
                    m_title = titleFile.isFile() ? titleFile.baseName() : title;
                }
                emit titleChanged();
            });

    connect(m_shellIntegration.get(),
            &ShellIntegration::commandChanged,
            this,
            [this](const CommandLine &command) {
                m_currentCommand = command;
                emit commandChanged(m_currentCommand);
            });
    connect(m_shellIntegration.get(),
            &ShellIntegration::currentDirChanged,
            this,
            [this](const QString &currentDir) {
                m_cwd = FilePath::fromUserInput(currentDir);
                emit cwdChanged(m_cwd);
            });
}

QString TerminalWidget::title() const
{
    const FilePath dir = cwd();
    QString title = m_title;
    if (title.isEmpty())
        title = currentCommand().isEmpty() ? shellName() : currentCommand().executable().fileName();
    if (dir.isEmpty())
        return title;
    return title + " - " + dir.fileName();
}

void TerminalWidget::updateCopyState()
{
    if (!hasFocus())
        return;

    m_copy->setEnabled(selection().has_value());
}

void TerminalWidget::setClipboard(const QString &text)
{
    setClipboardAndSelection(text);
}

std::optional<TerminalSolution::TerminalView::Link> TerminalWidget::toLink(const QString &text)
{
    if (text.size() > 0) {
        QString result = chopIfEndsWith(text, ':');

        if (!result.isEmpty()) {
            if (result.startsWith("~/"))
                result = QDir::homePath() + result.mid(1);

            Utils::Link link = Utils::Link::fromString(result, true);

            if (!link.targetFilePath.isEmpty() && !link.targetFilePath.isAbsolutePath())
                link.targetFilePath = m_cwd.pathAppended(link.targetFilePath.path());

            if (link.hasValidTarget()
                && (link.targetFilePath.scheme().toString().startsWith("http")
                    || link.targetFilePath.exists())) {
                return Link{link.targetFilePath.toString(), link.targetLine, link.targetColumn};
            }
        }
    }

    return std::nullopt;
}

void TerminalWidget::onReadyRead(bool forceFlush)
{
    QByteArray data = m_process->readAllRawStandardOutput();

    writeToTerminal(data, forceFlush);
}

void TerminalWidget::setShellName(const QString &shellName)
{
    m_shellName = shellName;
}

QString TerminalWidget::shellName() const
{
    return m_shellName;
}

FilePath TerminalWidget::cwd() const
{
    return m_cwd;
}

CommandLine TerminalWidget::currentCommand() const
{
    return m_currentCommand;
}

std::optional<Id> TerminalWidget::identifier() const
{
    return m_openParameters.identifier;
}

QProcess::ProcessState TerminalWidget::processState() const
{
    if (m_process)
        return m_process->state();

    return QProcess::NotRunning;
}

void TerminalWidget::restart(const OpenTerminalParameters &openParameters)
{
    QTC_ASSERT(!m_process || !m_process->isRunning(), return);
    m_openParameters = openParameters;
    m_process.reset();
    TerminalView::restart();
    setupPty();
}

void TerminalWidget::selectionChanged(const std::optional<Selection> &newSelection)
{
    SearchableTerminal::selectionChanged(newSelection);

    updateCopyState();

    if (selection() && selection()->final) {
        QString text = textFromSelection();

        QClipboard *clipboard = QApplication::clipboard();
        if (clipboard->supportsSelection())
            clipboard->setText(text, QClipboard::Selection);
    }
}

void TerminalWidget::linkActivated(const Link &link)
{
    FilePath filePath = FilePath::fromUserInput(link.text);

    if (filePath.scheme().toString().startsWith("http")) {
        QDesktopServices::openUrl(QUrl::fromUserInput(link.text));
        return;
    }

    if (filePath.isDir())
        Core::FileUtils::showInFileSystemView(filePath);
    else
        EditorManager::openEditorAt(Utils::Link{filePath, link.targetLine, link.targetColumn});
}

void TerminalWidget::focusInEvent(QFocusEvent *event)
{
    TerminalView::focusInEvent(event);
    updateCopyState();
}

void TerminalWidget::contextMenuRequested(const QPoint &pos)
{
    QMenu *contextMenu = new QMenu(this);
    QAction *configureAction = new QAction(contextMenu);
    configureAction->setText(Tr::tr("Configure..."));
    connect(configureAction, &QAction::triggered, this, [] {
        ICore::showOptionsDialog("Terminal.General");
    });

    contextMenu->addAction(ActionManager::command(Constants::COPY)->action());
    contextMenu->addAction(ActionManager::command(Constants::PASTE)->action());
    contextMenu->addSeparator();
    contextMenu->addAction(ActionManager::command(Constants::CLEAR_TERMINAL)->action());
    contextMenu->addSeparator();
    contextMenu->addAction(configureAction);

    contextMenu->popup(mapToGlobal(pos));
}

void TerminalWidget::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasUrls()) {
        event->setDropAction(Qt::CopyAction);
        event->accept();
    }
}

void TerminalWidget::dropEvent(QDropEvent *event)
{
    QString urls = Utils::transform(event->mimeData()->urls(), [](const QUrl &url) {
                       return QString("\"%1\"").arg(url.toDisplayString(QUrl::PreferLocalFile));
                   }).join(" ");

    writeToPty(urls.toUtf8());
    event->setDropAction(Qt::CopyAction);
    event->accept();
}

void TerminalWidget::showEvent(QShowEvent *event)
{
    Q_UNUSED(event);

    if (!m_process)
        setupPty();

    TerminalView::showEvent(event);
}

void TerminalWidget::handleEscKey(QKeyEvent *event)
{
    bool sendToTerminal = settings().sendEscapeToTerminal();
    bool send = false;
    if (sendToTerminal && event->modifiers() == Qt::NoModifier)
        send = true;
    else if (!sendToTerminal && event->modifiers() == Qt::ShiftModifier)
        send = true;

    if (send) {
        event->setModifiers(Qt::NoModifier);
        TerminalView::keyPressEvent(event);
        return;
    }

    if (selection()) {
        clearSelection();
    } else {
        QAction *returnAction = ActionManager::command(Core::Constants::S_RETURNTOEDITOR)
                                    ->actionForContext(Core::Constants::C_GLOBAL);
        QTC_ASSERT(returnAction, return);
        returnAction->trigger();
    }
}

bool TerminalWidget::event(QEvent *event)
{
    if (event->type() == QEvent::ShortcutOverride) {
        auto keyEvent = static_cast<QKeyEvent *>(event);
        if (keyEvent->key() == Qt::Key_Escape && keyEvent->modifiers() == Qt::NoModifier
            && settings().sendEscapeToTerminal()) {
            event->accept();
            return true;
        }

        if (settings().lockKeyboard()
            && QKeySequence(keyEvent->keyCombination())
                   == ActionManager::command(Constants::TOGGLE_KEYBOARD_LOCK)->keySequence()) {
            return false;
        }

        if (settings().lockKeyboard()) {
            event->accept();
            return true;
        }
    }

    if (event->type() == QEvent::KeyPress) {
        auto k = static_cast<QKeyEvent *>(event);

        if (k->key() == Qt::Key_Escape) {
            handleEscKey(k);
            return true;
        }

        if (settings().lockKeyboard() && m_shortcutMap.tryShortcut(k))
            return true;

        keyPressEvent(k);
        return true;
    }
    return TerminalView::event(event);
}

void TerminalWidget::initActions()
{
    Core::Context context(Utils::Id("TerminalWidget"));

    static QAction copy;
    static QAction paste;
    static QAction clearSelection;
    static QAction clearTerminal;
    static QAction moveCursorWordLeft;
    static QAction moveCursorWordRight;
    static QAction close;

    copy.setText(Tr::tr("Copy"));
    paste.setText(Tr::tr("Paste"));
    clearSelection.setText(Tr::tr("Clear Selection"));
    clearTerminal.setText(Tr::tr("Clear Terminal"));
    moveCursorWordLeft.setText(Tr::tr("Move Cursor Word Left"));
    moveCursorWordRight.setText(Tr::tr("Move Cursor Word Right"));
    close.setText(Tr::tr("Close Terminal"));

    auto copyCmd = ActionManager::registerAction(&copy, Constants::COPY, context);
    auto pasteCmd = ActionManager::registerAction(&paste, Constants::PASTE, context);

    if (HostOsInfo::isMacHost()) {
        copyCmd->setDefaultKeySequence(QKeySequence(QLatin1String("Ctrl+C")));
        pasteCmd->setDefaultKeySequence(QKeySequence(QLatin1String("Ctrl+V")));
    } else if (HostOsInfo::isLinuxHost()) {
        copyCmd->setDefaultKeySequence(QKeySequence(QLatin1String("Ctrl+Shift+C")));
        pasteCmd->setDefaultKeySequence(QKeySequence(QLatin1String("Ctrl+Shift+V")));
    } else if (HostOsInfo::isWindowsHost()) {
        copyCmd->setDefaultKeySequences(
            {QKeySequence(QLatin1String("Ctrl+C")), QKeySequence(QLatin1String("Ctrl+Shift+C"))});
        pasteCmd->setDefaultKeySequences(
            {QKeySequence(QLatin1String("Ctrl+V")), QKeySequence(QLatin1String("Ctrl+Shift+V"))});
    }

    ActionManager::registerAction(&clearSelection, Constants::CLEARSELECTION, context);

    ActionManager::registerAction(&moveCursorWordLeft, Constants::MOVECURSORWORDLEFT, context)
        ->setDefaultKeySequences({QKeySequence("Alt+Left")});

    ActionManager::registerAction(&moveCursorWordRight, Constants::MOVECURSORWORDRIGHT, context)
        ->setDefaultKeySequences({QKeySequence("Alt+Right")});

    ActionManager::registerAction(&clearTerminal, Constants::CLEAR_TERMINAL, context);
}

void TerminalWidget::unlockGlobalAction(const Utils::Id &commandId)
{
    Command *cmd = ActionManager::command(commandId);
    QTC_ASSERT(cmd, return);
    registerShortcut(cmd);
}

} // namespace Terminal
