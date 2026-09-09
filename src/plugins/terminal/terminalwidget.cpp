// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "terminalwidget.h"
#include "terminalconstants.h"
#include "terminalsettings.h"
#include "terminaltr.h"

#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/fileutils.h>
#include <coreplugin/find/textfindconstants.h>
#include <coreplugin/foldernavigationwidget.h>
#include <coreplugin/icore.h>
#include <coreplugin/iversioncontrol.h>
#include <coreplugin/messagemanager.h>
#include <coreplugin/vcsmanager.h>

#include <utils/algorithm.h>
#include <utils/async.h>
#include <utils/dropsupport.h>
#include <utils/environment.h>
#include <utils/hostosinfo.h>
#include <utils/processinterface.h>
#include <utils/stringutils.h>

#include <QApplication>
#include <QCache>
#include <QClipboard>
#include <QDesktopServices>
#include <QElapsedTimer>
#include <QGlyphRun>
#include <QLoggingCategory>
#include <QMenu>
#include <QPaintEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPixmapCache>
#include <QRawFont>
#include <QRegularExpression>
#include <QTextItem>
#include <QTextLayout>
#include <QToolTip>

#include <algorithm>

using namespace Core;
using namespace QtTaskTree;
using namespace Utils;
using namespace Utils::Terminal;

namespace Terminal {

// A title that names a file or a directory is shown as its last component
// alone; the default prompt on several distributions reports something like
// "user@host: /a/very/long/path". Which titles those are has to be read off
// the text - asking the filesystem is what this replaced, and for a shell in
// a container or over ssh that is a request over a connection, repeated as
// often as the program cares to change its title. Only a trailing token that
// is a path counts, so a title such as "make -j8 [2/5]" is left as it is.
static QString shortenedTitle(const QString &title)
{
    const QString token = title.mid(title.lastIndexOf(u' ') + 1);

    // There is a last component to take only once a token names more than its
    // own root. fromUserInput expands a leading ~ against this machine's home
    // directory, so a bare ~ would otherwise be shown as the name that
    // expansion produced rather than as anything the shell reported.
    if (!token.contains(u'/') && !token.contains(u'\\'))
        return title;

    // fromUserInput normalises the separator, so a Windows path arrives here
    // as one fileName() can split, and "make -j8 [2/5]" as one it will not
    // call absolute.
    const FilePath path = FilePath::fromUserInput(token);
    if (!path.isAbsolutePath())
        return title;

    const QString name = path.fileName();
    return name.isEmpty() ? title : name;
}

TerminalWidget::TerminalWidget(QWidget *parent, const OpenTerminalParameters &openParameters)
    : Core::SearchableTerminal(parent)
    , m_context(Utils::Id("TerminalWidget_").withSuffix(QString::number((uintptr_t) this)))
    , m_openParameters(openParameters)
{
    IContext::attach(this, m_context);

    setupFont();
    setupColors();
    setupActions();

    auto *dropSupport = new DropSupport(this, &DropSupport::enforceCopyAction);
    connect(dropSupport,
            &DropSupport::filesDropped,
            this,
            [this](const QList<DropSupport::FileSpec> &files, const QPoint &) {
                const QStringList quotedPaths = Utils::transform(
                    files, [](const DropSupport::FileSpec &file) {
                        return QString("\"%1\"").arg(file.filePath.toUserOutput());
                    });
                writeToPty(quotedPaths.join(" ").toUtf8());
            });

    auto baseUpdater = surfaceUpdater();
    setSurfaceUpdater([this, baseUpdater] {
        if (baseUpdater)
            baseUpdater();

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
                        m_title = shortenedTitle(title);
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
                [this](const FilePath &currentDir) {
                    m_cwd = currentDir;
                    emit cwdChanged(m_cwd);
                    // Keep an auto-synchronizing File System view in sync with
                    // the focused terminal's directory (QTCREATORBUG-28936).
                    // Focus sits on the view's viewport, so check the focus chain.
                    const QWidget *fw = QApplication::focusWidget();
                    if (fw && (fw == this || isAncestorOf(fw)))
                        Core::FolderNavigationWidgetFactory::requestSyncWithFilePath(m_cwd);
                });
    });

    setAllowBlinkingCursor(settings().allowBlinkingCursor());
    enableMouseTracking(settings().enableMouseTracking());

    connect(&settings(), &AspectContainer::applied, this, [this] {
        // Setup colors first, as setupFont will redraw the screen.
        setupColors();
        setupFont();
        configBlinkTimer();
        setAllowBlinkingCursor(settings().allowBlinkingCursor());
        enableMouseTracking(settings().enableMouseTracking());
    });
}

void TerminalWidget::setupPty()
{
    m_process = std::make_unique<Process>();

    const CommandLine shellCommand = m_openParameters.shellCommand.value_or(
        CommandLine{settings().shell(), settings().shellArguments(), CommandLine::Raw});

    if (shellCommand.executable().isRootPath()) {
        writeToTerminal((Tr::tr("Connecting...") + "\r\n").toUtf8(), true);
        // We still have to find the shell to start ...
        using ResultType = Result<FilePath>;
        const auto onSetup = [exec = shellCommand.executable()](Async<ResultType> &task) {
            task.setConcurrentCallData([exec]() -> Result<FilePath> {
                const Result<FilePath> result = Utils::Terminal::defaultShellForDevice(exec);
                if (result && !result->isExecutableFile())
                    return make_unexpected(
                        Tr::tr("\"%1\" is not executable.").arg(result->toUserOutput()));
                return result;
            });
        };
        const auto onDone = [this](const Async<ResultType> &task) {
            const Result<FilePath> result = task.result();
            if (result) {
                m_openParameters.shellCommand->setExecutable(*result);
                return DoneResult::Success;
            }
            writeToTerminal(("\r\n\033[31m"
                             + Tr::tr("Failed to start shell: %1").arg(result.error()) + "\r\n")
                                .toUtf8(), true);
            return DoneResult::Error;
        };
        const auto onTreeDone = [this] { restart(m_openParameters); };
        m_taskTreeRunner.start({AsyncTask<ResultType>(onSetup, onDone)}, {},
                               onTreeDone, CallDoneFlag::OnSuccess);
        return;
    }

    Environment env = m_openParameters.environment.value_or(Environment{})
                          .appliedToEnvironment(shellCommand.executable().deviceEnvironment());

    // Some OS/Distros set a default value for TERM such as "dumb", which then breaks
    // command line tools such as "clear" which try to figure out what terminal they are
    // running in. Therefore we have to force-set our own TERM value here.
    env.set("TERM", "xterm-256color");

    // Set some useful defaults
    env.setFallback("TERM_PROGRAM", QCoreApplication::applicationName());
    env.setFallback("COLORTERM", "truecolor");
    env.setFallback("COMMAND_MODE", "unix2003");
    env.setFallback("INIT_CWD", QCoreApplication::applicationDirPath());

    if (env.hasKey("CLINK_NOAUTORUN"))
        env.unset("CLINK_NOAUTORUN");

    // Every variable the injected shell integration reads, so that an
    // inherited value cannot steer it. The first four set, prepend to or
    // append to arbitrary variables inside the running shell and put a
    // directory at the front of its PATH. VSCODE_SHELL_INTEGRATION makes the
    // script return before it has done anything, silently switching the
    // integration off. VSCODE_SHELL_LOGIN makes it source /etc/profile and
    // ~/.bash_profile instead of ~/.bashrc, and VSCODE_ZDOTDIR decides which
    // rc files zsh reads. VSCODE_INJECTION gates the injected branches.
    // VSCODE_SUGGEST turns on pwsh's completion machinery. VSCODE_NONCE is
    // interpolated *unquoted* into the report printf, so a value holding a
    // space reuses the format string and forges a second OSC 633 command
    // report - a command in the tab title that nothing ran.
    //
    // Dropping them here is safe even for the two Qt Creator wants set:
    // prepareProcess runs after this and sets VSCODE_INJECTION always and
    // VSCODE_SHELL_LOGIN for a shell it started with -l.
    for (const QString &name : {QString("VSCODE_ENV_REPLACE"),
                                QString("VSCODE_ENV_PREPEND"),
                                QString("VSCODE_ENV_APPEND"),
                                QString("VSCODE_PATH_PREFIX"),
                                QString("VSCODE_SHELL_INTEGRATION"),
                                QString("VSCODE_SHELL_LOGIN"),
                                QString("VSCODE_ZDOTDIR"),
                                QString("VSCODE_INJECTION"),
                                QString("VSCODE_SUGGEST"),
                                QString("VSCODE_NONCE")}) {
        if (env.hasKey(name))
            env.unset(name);
    }

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

    connect(m_process.get(), &Process::readyReadStandardOutput, this, [this] {
        onReadyRead(false);
    });

    connect(m_process.get(), &Process::done, this, [this] {
        QString errorMessage;

        const int exitCode = QTC_GUARD(m_process) ? m_process->exitCode() : -1;
        if (m_process) {
            if (exitCode != 0) {
                errorMessage = Tr::tr("Terminal process exited with code %1.").arg(exitCode);

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
                QString exitMsg = Tr::tr("Process exited with code: %1.").arg(exitCode);
                QByteArray msg = QString("\r\n%1").arg(exitMsg).toUtf8();
                writeToTerminal(msg, true);
            }
        } else if (!errorMessage.isEmpty()) {
            Core::MessageManager::writeFlashing(errorMessage);
        }
        emit finished(exitCode);
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
    f.setFamily(settings().fontFamily());
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

void TerminalWidget::setupActions()
{
    auto make_registered = [this](ActionBuilder &actionBuilder, bool registerInShortcutMap = true) {
        if (registerInShortcutMap)
            registerShortcut(actionBuilder.command());

        const auto unregister = [cmdId = actionBuilder.command()->id()](QAction *a) {
            ActionManager::unregisterAction(a, cmdId);
            delete a;
        };

        return RegisteredAction(actionBuilder.contextAction(), unregister);
    };

    ActionBuilder copyAction(this, Constants::COPY);
    copyAction.setContext(m_context);
    copyAction.addOnTriggered(this, &TerminalWidget::copyToClipboard);
    m_copy = make_registered(copyAction);

    ActionBuilder pasteAction(this, Constants::PASTE);
    pasteAction.setContext(m_context);
    pasteAction.addOnTriggered(this, &TerminalWidget::pasteFromClipboard);
    m_paste = make_registered(pasteAction);

    ActionBuilder closeTerminalAction(this, Core::Constants::CLOSE);
    closeTerminalAction.setContext(m_context)
        .addOnTriggered(this, &TerminalWidget::closeTerminal)
        .setText(Tr::tr("Close Terminal"));
    m_closeTerminal = make_registered(closeTerminalAction, false);

    ActionBuilder clearTerminalAction(this, Constants::CLEAR_TERMINAL);
    clearTerminalAction.setContext(m_context);
    clearTerminalAction.addOnTriggered(this, &TerminalWidget::clearContents);
    m_clearTerminal = make_registered(clearTerminalAction);

    ActionBuilder clearSelectionAction(this, Constants::CLEARSELECTION);
    clearSelectionAction.setContext(m_context);
    clearSelectionAction.addOnTriggered(this, &TerminalWidget::clearSelection);
    m_clearSelection = make_registered(clearSelectionAction);

    ActionBuilder moveCursorWordLeftAction(this, Constants::MOVECURSORWORDLEFT);
    moveCursorWordLeftAction.setContext(m_context);
    moveCursorWordLeftAction.addOnTriggered(this, &TerminalWidget::moveCursorWordLeft);
    m_moveCursorWordLeft = make_registered(moveCursorWordLeftAction);

    ActionBuilder moveCursorWordRightAction(this, Constants::MOVECURSORWORDRIGHT);
    moveCursorWordRightAction.setContext(m_context);
    moveCursorWordRightAction.addOnTriggered(this, &TerminalWidget::moveCursorWordRight);
    m_moveCursorWordRight = make_registered(moveCursorWordRightAction);

    ActionBuilder selectAllAction(this, Constants::SELECTALL);
    selectAllAction.setContext(m_context);
    selectAllAction.addOnTriggered(this, &TerminalWidget::selectAll);
    m_selectAll = make_registered(selectAllAction);

    ActionBuilder deleteWordLeft(this, Constants::DELETE_WORD_LEFT);
    deleteWordLeft.setContext(m_context);
    deleteWordLeft.addOnTriggered(this, [this]() { writeToPty("\x17"); });
    m_deleteWordLeft = make_registered(deleteWordLeft);

    ActionBuilder deleteLineLeft(this, Constants::DELETE_LINE_LEFT);
    deleteLineLeft.setContext(m_context);
    deleteLineLeft.addOnTriggered(this, [this]() { writeToPty("\x15"); });
    m_deleteLineLeft = make_registered(deleteLineLeft);

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

bool TerminalWidget::resizePty(QSize newSize)
{
    if (!m_process || !m_process->ptyData() || !m_process->isRunning())
        return false;

    m_process->ptyData()->resize(newSize);
    return true;
}


QString TerminalWidget::title() const
{
    const FilePath dir = cwd();
    QString currentExecutable = currentCommand().isEmpty()
                                    ? shellName()
                                    : currentCommand().executable().fileName();
    return Utils::joinStrings({currentExecutable, cwd().fileName()}, " - ");
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

std::optional<TerminalSolution::TerminalView::Link> TerminalWidget::toPathOrWebLink(
    const QString &text)
{
    if (text.isEmpty())
        return std::nullopt;

    QString result = chopIfEndsWith(text, ':');
    if (result.isEmpty())
        return std::nullopt;

    if (result.startsWith("~/")) {
        // For a shell running elsewhere the text names a different filesystem.
        if (!shellFilePath().isLocal())
            return std::nullopt;
        result = QDir::homePath() + result.mid(1);
    }

    // A web address is handed to the desktop when activated and is never
    // resolved as a path, so it needs neither a directory nor a check that it
    // exists. Naming only a host leaves its path empty, so it is the host that
    // has to be there. It is also read before a line and column postfix is
    // taken off the end: that postfix is a trailing ":<digits>", which is what
    // a port is, so https://host:8443 would otherwise be reached for at 443.
    const FilePath web = FilePath::fromUserInput(result);
    const QString webScheme = web.scheme().toString();
    if (webScheme == u"http" || webScheme == u"https") {
        if (web.host().isEmpty())
            return std::nullopt;
        return Link{web.toUrlishString()};
    }

    Utils::Link link = Utils::Link::fromString(result, true);
    const QString scheme = link.targetFilePath.scheme().toString();

    if (link.targetFilePath.isEmpty())
        return std::nullopt;

    // Everything else is treated as a path, and only a plain one: a scheme or
    // a host would turn the existence check below into a request to whatever
    // they name.
    if (!scheme.isEmpty() || !link.targetFilePath.host().isEmpty())
        return std::nullopt;

    if (link.targetFilePath.isAbsolutePath()) {
        // An absolute path printed by a shell elsewhere names a file on that
        // machine, so look for it there rather than here.
        if (!shellFilePath().isLocal())
            link.targetFilePath = shellFilePath().withNewPath(link.targetFilePath.path());
    } else {
        // Resolve against the directory the shell reported, or failing that
        // the one Qt Creator started it in. With neither, there is nothing the
        // reader chose to resolve against.
        const FilePath base = !m_cwd.isEmpty()
                                  ? m_cwd
                                  : m_openParameters.workingDirectory.value_or(FilePath{});
        if (base.isEmpty())
            return std::nullopt;
        link.targetFilePath = base.pathAppended(link.targetFilePath.path());
    }

    if (link.hasValidTarget() && link.targetFilePath.exists())
        return Link{link.targetFilePath.toUrlishString(), link.target.line, link.target.column};

    return std::nullopt;
}

std::optional<TerminalSolution::TerminalView::Link> TerminalWidget::toLink(const QString &text)
{
    constexpr qsizetype maxCachedLinks = 32;

    // Detection runs on every pointer move with Control held, and the path
    // branch below asks the shell's device whether the file exists - for a
    // shell in a container or over ssh, a round trip on the GUI thread. The
    // pointer crosses the same handful of words over and over, so keep the
    // answer for each of them rather than only for the last one: a single slot
    // was defeated by moving back and forth between two words, which is most
    // of what the pointer does.
    const auto cached = std::find_if(m_linkCache.begin(),
                                     m_linkCache.end(),
                                     [&text, this](const LinkCache &entry) {
                                         return entry.text == text && entry.cwd == m_cwd;
                                     });
    if (cached != m_linkCache.end()) {
        std::rotate(m_linkCache.begin(), cached, std::next(cached));
        return m_linkCache.first().link;
    }

    const std::optional<Link> link = sniffLink(text);
    m_linkCache.prepend(LinkCache{text, m_cwd, link});
    if (m_linkCache.size() > maxCachedLinks)
        m_linkCache.removeLast();
    return link;
}

std::optional<TerminalSolution::TerminalView::Link> TerminalWidget::sniffLink(const QString &text)
{
    if (const std::optional<Link> link = toPathOrWebLink(text))
        return link;

    if (m_cwd.isEmpty())
        return std::nullopt;

    const bool looksLikeRevision = text.size() >= 7 && text.size() <= 40
                                   && Utils::allOf(text, [](QChar c) {
                                          c = c.toLower();
                                          return c.isDigit() || (c >= 'a' && c <= 'f');
                                      });
    if (!looksLikeRevision)
        return std::nullopt;

    return Link{QString("vcs:///%1").arg(text)};
}

void TerminalWidget::onReadyRead(bool forceFlush)
{
    QByteArray data = m_process->readAllRawStandardOutput();

    // Whether a path names a file that exists can only have changed because
    // something ran, so the cached answers stand until the shell writes. This
    // is what keeps a "no such file" answer from outliving the file's creation.
    m_linkCache.clear();

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

ProcessState TerminalWidget::processState() const
{
    if (m_process)
        return m_process->state();

    return ProcessState::NotRunning;
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

// The shell's executable sits on the device the shell runs on, which m_cwd only names
// once shell integration has reported a directory - and it never does for a shell that
// is not integrated.
FilePath TerminalWidget::shellFilePath() const
{
    if (m_process && !m_process->commandLine().executable().isEmpty())
        return m_process->commandLine().executable();

    return m_openParameters.shellCommand.value_or(CommandLine{}).executable();
}

void TerminalWidget::linkActivated(const Link &link)
{
    const auto open = [&link](const FilePath &filePath) {
        if (filePath.isDir())
            Core::FileUtils::showInFileSystemView(filePath);
        else
            EditorManager::openEditorAt(Utils::Link{filePath, link.targetLine, link.targetColumn});
    };

    if (link.isUri) {
        QUrl url(link.text);
        if (!url.isLocalFile()) {
            QDesktopServices::openUrl(url);
            return;
        }

        // A host in the uri names the machine the shell runs on, not a path on it, so
        // drop it and resolve the path on that machine.
        url.setHost({});
        open(shellFilePath().withNewPath(FilePath::fromUrl(url).path()));
        return;
    }

    if (link.text.startsWith("vcs:///")) {
        if (IVersionControl *vcs = VcsManager::findVersionControlForDirectory(m_cwd))
            vcs->vcsDescribe(m_cwd, link.text.mid(7));
        return;
    }

    const QUrl url = QUrl::fromUserInput(link.text);
    if (url.scheme() == u"http" || url.scheme() == u"https") {
        QDesktopServices::openUrl(url);
        return;
    }

    open(FilePath::fromUserInput(link.text));
}

void TerminalWidget::focusInEvent(QFocusEvent *event)
{
    TerminalView::focusInEvent(event);
    updateCopyState();
    if (!m_cwd.isEmpty())
        Core::FolderNavigationWidgetFactory::requestSyncWithFilePath(m_cwd);
}

void TerminalWidget::contextMenuRequested(const QPoint &pos)
{
    QMenu *contextMenu = new QMenu(this);
    QAction *configureAction = new QAction(contextMenu);
    configureAction->setText(Tr::tr("Configure..."));
    connect(configureAction, &QAction::triggered, this, [] {
        ICore::showSettings("Terminal.General");
    });

    contextMenu->addAction(ActionManager::command(Constants::COPY)->action());
    contextMenu->addAction(ActionManager::command(Constants::PASTE)->action());
    contextMenu->addAction(ActionManager::command(Constants::SELECTALL)->action());
    contextMenu->addSeparator();
    contextMenu->addAction(ActionManager::command(Constants::CLEAR_TERMINAL)->action());
    contextMenu->addSeparator();
    contextMenu->addAction(configureAction);

    contextMenu->setAttribute(Qt::WA_DeleteOnClose);
    contextMenu->popup(mapToGlobal(pos));
}

void TerminalWidget::showEvent(QShowEvent *event)
{
    Q_UNUSED(event)

    if (!m_process)
        setupPty();

    TerminalView::showEvent(event);
}

void TerminalWidget::handleEscKey(QKeyEvent *event)
{
    bool sendToTerminal = settings().sendEscapeToTerminal();
    const bool send = (sendToTerminal && event->modifiers() == Qt::NoModifier)
                      || (!sendToTerminal && event->modifiers() == Qt::ShiftModifier);

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

void TerminalWidget::initActions(QObject *parent)
{
    Core::Context context(Utils::Id("TerminalWidget"));

    auto keySequence = [](const QChar &key) -> QList<QKeySequence> {
        if (HostOsInfo::isMacHost()) {
            return {QKeySequence(QLatin1String("Ctrl+") + key)};
        } else if (HostOsInfo::isLinuxHost()) {
            return {QKeySequence(QLatin1String("Ctrl+Shift+") + key)};
        } else if (HostOsInfo::isWindowsHost()) {
            return {QKeySequence(QLatin1String("Ctrl+") + key),
                    QKeySequence(QLatin1String("Ctrl+Shift+") + key)};
        }
        return {};
    };

    ActionBuilder copyAction(parent, Constants::COPY);
    copyAction.setText(Tr::tr("Copy"));
    copyAction.setContext(context);
    copyAction.setDefaultKeySequences(keySequence('C'));

    ActionBuilder pasteAction(parent, Constants::PASTE);
    pasteAction.setText(Tr::tr("Paste"));
    pasteAction.setContext(context);
    pasteAction.setDefaultKeySequences(keySequence('V'));

    ActionBuilder clearTerminalAction(parent, Constants::CLEAR_TERMINAL);
    clearTerminalAction.setText(Tr::tr("Clear Terminal"));
    clearTerminalAction.setContext(context);

    ActionBuilder selectAllAction(parent, Constants::SELECTALL);
    selectAllAction.setText(Tr::tr("Select All"));
    selectAllAction.setContext(context);
    selectAllAction.setDefaultKeySequences(keySequence('A'));

    ActionBuilder clearSelectionAction(parent, Constants::CLEARSELECTION);
    clearSelectionAction.setText(Tr::tr("Clear Selection"));
    clearSelectionAction.setContext(context);

    ActionBuilder moveCursorWordLeftAction(parent, Constants::MOVECURSORWORDLEFT);
    moveCursorWordLeftAction.setText(Tr::tr("Move Cursor Word Left"));
    moveCursorWordLeftAction.setContext(context);
    moveCursorWordLeftAction.setDefaultKeySequence({QKeySequence("Alt+Left")});

    ActionBuilder moveCursorWordRightAction(parent, Constants::MOVECURSORWORDRIGHT);
    moveCursorWordRightAction.setText(Tr::tr("Move Cursor Word Right"));
    moveCursorWordRightAction.setContext(context);
    moveCursorWordRightAction.setDefaultKeySequence({QKeySequence("Alt+Right")});

    ActionBuilder deleteWordLeft(parent, Constants::DELETE_WORD_LEFT);
    deleteWordLeft.setText(Tr::tr("Delete Word Left"));
    deleteWordLeft.setContext(context);
    deleteWordLeft.setDefaultKeySequence({QKeySequence("Alt+Backspace")});

    ActionBuilder deleteLineLeft(parent, Constants::DELETE_LINE_LEFT);
    deleteLineLeft.setText(Tr::tr("Delete Line Left"));
    deleteLineLeft.setContext(context);
    deleteLineLeft.setDefaultKeySequence({QKeySequence("Ctrl+Backspace")});
}

void TerminalWidget::unlockGlobalAction(const Utils::Id &commandId)
{
    Command *cmd = ActionManager::command(commandId);
    QTC_ASSERT(cmd, return);
    registerShortcut(cmd);
}

} // namespace Terminal
