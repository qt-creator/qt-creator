// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QObject>
#include <QTextEdit>

#include <functional>

namespace FakeVim::Internal {

enum RangeMode
{
    // Reordering first three enum items here will break
    // compatibility with clipboard format stored by Vim.
    RangeCharMode,         // v
    RangeLineMode,         // V
    RangeBlockMode,        // Ctrl-v
    RangeLineModeExclusive,
    RangeBlockAndTailMode // Ctrl-v for D and X
};

struct Range
{
    Range() = default;
    Range(int b, int e, RangeMode m = RangeCharMode);

    bool isValid() const;

    int beginPos = -1;
    int endPos = -1;
    RangeMode rangemode = RangeCharMode;
};

struct ExCommand
{
    ExCommand() = default;

    bool matches(const QString &min, const QString &full) const;

    QString cmd;
    bool hasBang = false;
    QString args;
    Range range;
    bool hasRange = false;
    // ":0" names the place before the first line, which a range of positions
    // cannot hold. ":put" asks for it by this.
    bool zeroAddress = false;
    // The line of the file this was read from, 1-based, or 0 where it was not
    // read from one. What a script's frame is reported to be at.
    int sourceLine = 0;
    // The text this was parsed from, which ":append" and its kin take
    // verbatim rather than as a command.
    QString original;
    int count = 1;
};

// message levels sorted by severity
enum MessageLevel
{
    MessageMode,    // show current mode (format "-- %1 --")
    MessageCommand, // show last Ex command or search
    MessageInfo,    // result of a command
    MessageWarning, // warning
    MessageError,   // error
    MessageShowCmd  // partial command
};

template<typename>
class Callback;

template<typename R, typename... Params>
class Callback<R(Params...)>
{
public:
    static constexpr auto IsVoidReturnType = std::is_same_v<R, void>;
    using Function = std::function<R(Params...)>;
    void set(const Function &callable) { m_callable = callable; }

    R operator()(Params... params)
    {
        if (!m_callable)
            return R();

        if constexpr (IsVoidReturnType)
            m_callable(std::forward<Params>(params)...);
        else
            return m_callable(std::forward<Params>(params)...);
    }

private:
    Function m_callable;
};

class FakeVimHandler : public QObject
{
    Q_OBJECT

public:
    explicit FakeVimHandler(QWidget *widget, QObject *parent = nullptr);
    ~FakeVimHandler() override;

    QWidget *widget();

    // call before widget is deleted
    void disconnectFromEditor();

    static void updateGlobalMarksFilenames(const QString &oldFileName, const QString &newFileName);

public:
    void setCurrentFileName(const QString &fileName);
    QString currentFileName() const;

    void showMessage(MessageLevel level, const QString &msg);

    // Fire the autocommands registered for an event (e.g. from the plugin's
    // save path so BufWritePre/BufWritePost run on a real save).
    // "target" is what the pattern is matched against and what "<afile>"
    // stands for, where the event is about something that is no file - a
    // window id for WinClosed, say. Empty means the current file name.
    // Returns how many autocommands ran, so that a caller firing a "*Cmd"
    // event can leave the built-in action undone where one of them did it.
    int triggerAutocmd(const QString &event, const QString &target = {});

    // A completion having been applied, with the word it put in - the one
    // moment the chosen word is known, the completion UI being Qt Creator's.
    void triggerCompleteDone(const QString &word);

    // A context menu about to be shown. The handler works out the mode the
    // pattern is matched against itself.
    void triggerMenuPopup();

    // Splits having changed size, named by the ids Qt Creator hands out for
    // them. Vim reports them together, so they arrive as a list.
    void triggerWinResized(const QList<int> &viewIds);

    // Obey a "vim:" line in the first or last lines of the buffer. Called when
    // a document is opened, after the file type has been established.
    // Move by that many entries in the argument list, opening what it lands
    // on. False where no list has been set, which leaves ":next" and
    // ":previous" to the plugin - they walk Qt Creator's open documents there,
    // and did so before there was a list to walk at all.
    bool walkArgList(int distance);

    void processModelines();

    // This executes an "ex" style command taking context
    // information from the current widget.
    void handleCommand(const QString &cmd);
    void handleReplay(const QString &keys);
    void handleInput(const QString &keys);
    void enterCommandMode();

    void installEventFilter();

    // Convenience
    void setupWidget();
    void restoreWidget(int tabSize);

    // Test only
    int physicalIndentation(const QString &line) const;
    int logicalIndentation(const QString &line) const;
    QString tabExpand(int n) const;

    void miniBufferTextEdited(const QString &text, int cursorPos, int anchorPos);

    // Set text cursor position. Keeps anchor if in visual mode.
    void setTextCursorPosition(int position);

    QTextCursor textCursor() const;
    void setTextCursor(const QTextCursor &cursor);

    bool jumpToLocalMark(QChar mark, bool backTickMode);

    bool inFakeVimMode();

    bool eventFilter(QObject *ob, QEvent *ev) override;

    Callback<void(const QString &msg, int cursorPos, int anchorPos, int messageLevel)>
        commandBufferChanged;
    Callback<void(const QString &msg)> statusDataChanged;
    Callback<void(const QString &msg)> extraInformationChanged;
    Callback<void(const QList<QTextEdit::ExtraSelection> &selection)> selectionChanged;
    Callback<void(const QString &needle)> highlightMatches;
    // What a highlight group of Vim's looks like here, for matchadd(). An unset
    // callback leaves the handler to paint from the palette.
    Callback<QTextCharFormat(const QString &group)> highlightFormatRequested;
    Callback<void(bool *moved, bool *forward, QTextCursor *cursor)> moveToMatchingParenthesis;
    Callback<void(bool *result, QChar c)> checkForElectricCharacter;
    Callback<void(int beginLine, int endLine, QChar typedChar)> indentRegion;
    // Fills the editor tab/indent settings; used when useEditorTabSettings is
    // on so FakeVim follows the (project) editor settings (QTCREATORBUG-14273).
    Callback<void(int *tabSize, int *indentSize, bool *spacesForTabs)> tabSettingsRequested;
    // The display options Vim keeps per window and Qt Creator keeps per
    // editor: "number", "wrap", "list", "cursorline" and "breakindent". Read
    // and written rather than stored, the editor being what really knows.
    Callback<void(const QString &option, bool *on)> displayOptionRequested;
    Callback<void(const QString &option, bool on)> displayOptionChanged;
    // The options whose value is a name or a number rather than a switch, and
    // which the editor or its document holds: "fileformat" and "bomb" say what
    // shape the file has, "colorcolumn" and "foldcolumn" how it is shown. The
    // value goes both ways as a string - "unix"/"dos", "0"/"1", a column or an
    // empty one for none. Setting one may be refused, a line ending Qt Creator
    // has no mode for being asked for.
    Callback<void(const QString &option, QString *value)> documentOptionRequested;
    Callback<void(const QString &option, const QString &value, bool *accepted)>
        documentOptionChanged;
    // What "colorcolumn" and "foldcolumn" come to here: the one column to draw
    // a margin at, or the width of the fold column - zero for neither.
    Callback<void(const QString &option, int column)> marginOptionChanged;
    Callback<void(const QString &needle, bool forward)> simpleCompletionRequested;
    Callback<void(const QString &key, int count)> windowCommandRequested;
    // What input(), inputsecret() and inputlist() ask when no answer is
    // waiting in the typeahead. Vim blocks until the user answers, and a
    // dialog is what does that blocking here.
    Callback<void(const QString &prompt, const QString &preset, bool secret,
                  QString *answer, bool *cancelled)> inputRequested;
    // The prompt is the first entry of the list; the rest are the choices, and
    // the answer is the NUMBER of one of them, zero for none.
    Callback<void(const QStringList &lines, int *chosen)> inputListRequested;
    // What confirm() asks: a message, the buttons with "&" marking their
    // accelerators, and which one is the default. The answer is the button
    // number, or zero for a dismissed dialog.
    Callback<void(const QString &message, const QStringList &choices, int preferred,
                  int *chosen)> confirmRequested;
    // Where the application window sits, and where to put it - what ":winpos"
    // asks and sets.
    Callback<void(int *x, int *y)> windowPositionRequested;
    Callback<void(int x, int y)> windowMoveRequested;
    Callback<void(bool reverse)> findRequested;
    Callback<void(bool reverse)> findNextRequested;
    Callback<void()> findHideRequested;
    Callback<void(bool *handled, const ExCommand &cmd)> handleExCommandRequested;
    Callback<void()> requestDisableBlockSelection;
    Callback<void(const QTextCursor &cursor, bool toEndOfLine)> requestSetBlockSelection;
    Callback<void(QTextCursor *cursor)> requestBlockSelection;
    Callback<void(bool *on)> requestHasBlockSelection;
    Callback<void(int depth)> foldToggle;
    Callback<void()> foldToggleAll; // zi: open all folds if any is closed, else close all
    Callback<void(bool fold)> foldAll;
    // Close every fold deeper than that level and open the rest, which is
    // what 'foldlevel' comes to. A level of zero closes them all.
    Callback<void(int level)> foldLevelRequested;
    // What foldclosed(), foldclosedend() and foldlevel() answer about a line:
    // where the closed fold holding it starts and ends, both -1 for a line in
    // none, and how deep the folds are there.
    Callback<void(int line, int *closedStart, int *closedEnd, int *level)>
        foldStateRequested;
    // Open or close the folds over a range of lines, which is what
    // ":foldopen" and ":foldclose" come to.
    Callback<void(int firstLine, int lastLine, bool close)> foldRangeRequested;
    Callback<void(int depth, bool dofold)> fold;
    Callback<void(int count, bool current)> foldGoTo;
    Callback<void(QChar mark, bool backTickMode, const QString &fileName)> requestJumpToLocalMark;
    Callback<void(QChar mark, bool backTickMode, const QString &fileName)> requestJumpToGlobalMark;
    Callback<void()> completionRequested;
    Callback<void()> tabPreviousRequested;
    Callback<void()> tabNextRequested;
    // Vim tag stack (QTCREATORBUG-11754). tagJumpRequested: CTRL-] / :tag with
    // an argument (push and follow). tagStackRequested: move by the signed
    // count - CTRL-T / :pop go back (negative), bare :tag goes forward.
    // The symbol to follow, which the handler knows and the plugin needs to
    // write down: ":tags" lists the tag of each level, and the tag stack lives
    // on the plugin side.
    Callback<void(const QString &tag)> tagJumpRequested;
    Callback<void(int distance)> tagStackRequested;
    // The files opened before this session, newest first, which is what
    // "v:oldfiles" holds and ":oldfiles" lists. Qt Creator keeps the list.
    Callback<void(QStringList *files)> recentFilesRequested;
    // One level of that stack, oldest first, for ":tags" to list.
    struct TagStackEntry
    {
        QString tag;       // the symbol that was followed
        int fromLine = 0;  // the line it was followed from, counted from one
        QString fromText;  // and that line, which the listing shows
    };
    Callback<void(QList<TagStackEntry> *entries, int *index)> tagStackContents;
    // K: look up the symbol under the cursor.
    Callback<void()> contextHelpRequested;
    // The syntax item names at a position, for synstack(). Qt Creator has no
    // general way to name what is under the cursor, so this reports only what
    // the language of the document can be asked about, which is "Comment" and
    // "String". Line and column are 1-based, as in Vimscript.
    Callback<void(int line, int column, QStringList *names)> syntaxNamesRequested;
    // CTRL-^: edit the alternate file, the previously active one.
    Callback<void()> alternateFileRequested;
    // gf, gF: open the file named under the cursor, gF at the line the number
    // behind the name says. Line 0 asks for no particular line.
    Callback<void(const QString &fileName, int line)> fileOpenRequested;
    // Move in Qt Creator's global navigation history when the buffer-local
    // jump list is exhausted, so CTRL-O / CTRL-I cross files. Negative distance
    // goes back, positive forward (QTCREATORBUG-12114).
    Callback<void(int distance)> navigateHistoryRequested;
    Callback<void(bool insertMode)> modeChanged;
    Callback<bool()> tabPressedInInsertMode;
    Callback<void(const QString &, const QString &, QString *)> processOutput;

public:
    class Private;

private:
    Private *d;
};

} // namespace FakeVim::Internal

Q_DECLARE_METATYPE(FakeVim::Internal::ExCommand)
