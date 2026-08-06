// Copyright (C) 2016 Lukas Holecek <hluk@email.cz>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

/*!
 * Tests for FakeVim plugin.
 * All test are based on Vim behaviour.
 */

#include "fakevim_test.h"

#include "fakevimhandler.h"
#include "fakevimactions.h"

#include <coreplugin/editormanager/editormanager.h>
#include <texteditor/syntaxhighlighter.h>
#include <texteditor/textdocument.h>
#include <texteditor/texteditor.h>

#include <utils/multitextcursor.h>
#include <utils/stringutils.h>

#include <QApplication>
#include <QFocusEvent>
#include <QDir>
#include <QFileInfo>
#include <QKeyEvent>
#include <QTemporaryFile>
#include <QTest>
#include <QTextEdit>
#include <QTextDocument>
#include <QTextBlock>

//TESTED_COMPONENT=src/plugins/fakevim

/*!
 * Tests after this macro will be skipped and warning printed.
 * Uncomment it to test a feature -- if tests succeeds it should be removed from the test.
 */
#define NOT_IMPLEMENTED QSKIP("Not fully implemented!");

// Text cursor representation in comparisons.
#define X "|"

// More distinct line separator in code.
#define N "\n"

// Document line start and end string in error text.
#define LINE_START "\t\t<"
#define LINE_END ">\n"

using namespace TextEditor;

namespace FakeVim::Internal {

static QString _(const char *c) { return QLatin1String(c); }
static QString _(const QByteArray &c) { return QLatin1String(c); }
static QString _(const QString &c) { return c; }

class FakeVimTester final : public QObject
{
    Q_OBJECT

private slots:
    void cleanup();

    void test_vim_movement();

    void test_vim_target_column_normal();
    void test_vim_target_column_visual_char();
    void test_vim_target_column_visual_block();
    void test_vim_target_column_visual_line();
    void test_vim_target_column_insert();
    void test_vim_target_column_replace();

    void test_vim_insert();
    void test_vim_fFtT();
    void test_vim_transform_numbers();
    void test_vim_delete();

    void test_vim_delete_inner_word();
    void test_vim_delete_a_word();
    void test_vim_change_a_word();

    void test_vim_change_replace();

    void test_vim_block_selection();
    void test_vim_block_selection_insert();

    void test_vim_delete_inner_paragraph();
    void test_vim_delete_a_paragraph();
    void test_vim_change_inner_paragraph();
    void test_vim_change_a_paragraph();
    void test_vim_select_inner_paragraph();
    void test_vim_select_a_paragraph();

    void test_vim_repeat();
    void test_vim_search();
    void test_vim_nohlsearch_core_search();
    void test_vim_indent();
    void test_vim_marks();
    void test_vim_jumps();
    void test_vim_current_column();
    void test_vim_copy_paste();
    void test_vim_undo_redo();
    void test_vim_letter_case();
    void test_vim_code_autoindent();
    void test_vim_code_folding();
    void test_vim_code_completion();
    void test_vim_substitute();
    void test_vim_ex_commandbuffer_paste();
    void test_vim_ex_yank();
    void test_vim_ex_delete();
    void test_vim_ex_change();
    void test_vim_ex_shift();
    void test_vim_ex_move();
    void test_vim_ex_join();
    void test_vim_ex_normal();
    void test_advanced_commands();

//public:
//    void changeStatusData(const QString &info) { m_statusData = info; }
//    void changeStatusMessage(const QString &info, int) { m_statusMessage = info; }
//    void changeExtraInformation(const QString &info) { m_infoMessage = info; }

//private slots:
//    // functional tests
    void test_vim_indentation();
    void test_vim_readonly();

    // command mode
    void test_vim_command_oO();
    void test_vim_command_put_at_eol();
    void test_vim_command_Cxx_down_dot();
    void test_vim_command_Gyyp();
    void test_vim_command_J();
    void test_vim_command_Yp();
    void test_vim_command_cc();
    void test_vim_command_cw();
    void test_vim_command_cj();
    void test_vim_command_ck();
    void test_vim_command_c_dollar();
    void test_vim_command_C();
    void test_vim_command_dd();
    void test_vim_command_dd_2();
    void test_vim_command_d_dollar();
    void test_vim_command_dgg();
    void test_vim_command_dG();
    void test_vim_command_dj();
    void test_vim_command_dk();
    void test_vim_command_D();
    void test_vim_command_dfx_down();
    void test_vim_command_dollar();
    void test_vim_command_down();
    void test_vim_command_dw();
    void test_vim_command_e();
    void test_vim_command_i();
    void test_vim_command_left();
    void test_vim_command_ma_yank();
    void test_vim_command_r();
    void test_vim_command_right();
    void test_vim_command_up();
    void test_vim_command_w();
    void test_vim_command_x();
    void test_vim_command_yyp();
    void test_vim_command_y_dollar();
    void test_vim_command_percent();
    void test_vim_percent_like_vim();

    void test_vim_visual_d();
    void test_vim_Visual_d();
    void test_vim_visual_block_D();

    // Plugin emulation
    void test_vim_commentary_emulation();
    void test_vim_commentary_file_names();
    void test_vim_replace_with_register_emulation();
    void test_vim_exchange_emulation();
    void test_vim_arg_text_obj_emulation();
    void test_vim_surround_emulation();
    void test_vim_unimpaired_emulation();
    void test_vim_reflow();
    void test_vim_visual_selection_focus_out();
    void test_vim_tagstack();
    void test_vim_source_utf8();
    void test_vim_fold_toggle_all();
    void test_vim_insert_indent();
    void test_vim_block_selection_to_eol();
    void test_vim_insert_map_with_quotes();
    void test_vim_search_smartcase();
    void test_vim_replace_char_newline();
    void test_vim_backspace_option();
    void test_vim_open_line_with_fold();
    void test_vim_scroll_center_on_scroll();
    void test_vim_tab_with_zero_tabstop();
    void test_vim_timeout_options();
    void test_vim_selection_for_shortcut();
    void test_vim_shortcut_override_text_key();
    void test_vim_jumplist_across_files();
    void test_vim_control_modifier();
    void test_vim_tabstop_distance();
    void test_vim_goto_definition();
    void test_vim_context_help();
    void test_vim_alternate_file();
    void test_vim_tag_text_object();
    void test_vim_script_echo_expression();
    void test_vim_script_variables();
    void test_vim_script_options_registers();
    void test_vim_script_builtins();
    void test_vim_script_expr_mapping();
    void test_vim_script_execute();
    void test_vim_script_if();
    void test_vim_script_while();
    void test_vim_script_lists();
    void test_vim_script_for();
    void test_vim_script_dicts();
    void test_vim_script_indexed_let();
    void test_vim_script_functions();
    void test_vim_script_string_builtins();
    void test_vim_script_collection_builtins();
    void test_vim_script_map_filter();
    void test_vim_script_try_catch();
    void test_vim_script_more_builtins();
    void test_vim_script_expr_register();
    void test_vim_script_operators();
    void test_vim_script_unpacking();
    void test_vim_script_slicing();
    void test_vim_script_ex_commands();
    void test_vim_script_funcref();
    void test_vim_script_autocmd();
    void test_vim_script_augroup();
    void test_vim_script_autoload();
    void test_vim_script_scriptlocal();
    void test_vim_script_if_chain();
    void test_vim_script_error_numbers();
    void test_vim_pattern_lookbehind_limit();
    void test_vim_script_block_abbreviations();
    void test_vim_command_line_ctrl_u();
    void test_vim_script_searchpair();
    void test_vim9_matchit();
    void test_vim_script_setline_place();
    void test_vim_ex_normal_unfinished();
    void test_vim_operator_pending_ex_mapping();
    void test_vim_script_list_compare();
    void test_vim_script_compare_ignorecase();
    void test_vim9_argtextobj();
    void test_vim_visual_mark_selection();
    void test_vim_script_v_register();
    void test_vim_set_trailing_comment();
    void test_vim_visual_paste_register_kind();
    void test_vim9_replace_with_register();
    void test_vim_visual_paste_linewise_register();
    void test_vim_script_script_local_funcref();
    void test_vim_motion_underscore();
    void test_vim_script_col_list();
    void test_vim_register_last_line();
    void test_vim_script_nonblank();
    void test_vim9_exchange();
    void test_vim_script_registers();
    void test_vim_script_mapping_queries();
    void test_vim_script_string_escapes();
    void test_vim_script_script_id();
    void test_vim9_commentary();
    void test_vim_script_range_function();
    void test_vim_ex_retab();
    void test_vim_script_width_and_getline();
    void test_vim9_justify();
    void test_vim_softtabstop();
    void test_vim_script_mode();
    void test_vim_ex_command_own_selection();
    void test_vim9_comment_text_object();
    void test_vim_script_skipped_subscript();
    void test_vim_search_wraps_to_cursor();
    void test_vim_pattern_buffer_position();
    void test_vim_set_showmatch_name();
    void test_vim_set_add_remove();
    void test_vim_set_escaped_value();
    void test_vim_script_throwpoint();
    void test_vim_pattern_lookaround();
    void test_vim_pattern_percent_atoms();
    void test_vim_pattern_very_magic();
    void test_vim_script_lockvar();
    void test_vim_script_messages();
    void test_vim_script_split();
    void test_vim_script_pattern_newline();
    void test_vim_script_known_options();
    void test_vim_script_trailing_comment();
    void test_vim_script_dict_dot();
    void test_vim_script_command();
    void test_vim_script_positions();
    void test_vim_script_operatorfunc();
    void test_vim9_basics();
    void test_vim9_def();
    void test_vim9_lambda();
    void test_vim9_continuation();
    void test_vim9_interpolation();
    void test_vim_heredoc();
    void test_vim9_import_export();
    void test_vim9_import_autoload();
    void test_vim9_nested_def();
    void test_vim_script_scope_dict();
    void test_vim_script_expand();
    void test_vim_script_regex_zs_ze();
    void test_vim_commentstring();
    void test_vim_filetype_detection();
    void test_vim_modeline();
    void test_vim_change_autocmds();
    void test_vim_script_readfile_writefile();
    void test_vim_script_search_cursor();
    void test_vim_map_cmd();
    void test_vim_ex_normal_modes();
    void test_vim_script_modifiers();
    void test_vim_script_operator_plugin();
    void test_vim_file_info();
    void test_vim_ex_plugin_command_moves_cursor();
    void test_vim_dot_after_visual_paste();
    void test_vim_use_editor_tab_settings();
    void test_vim_command_line_paste();
    void test_vim_tab_out();
    void test_vim_iso_level5_shift();

    void test_macros();

    void test_vim_qtcreator();

    // special tests
    void test_i_cw_i();

    // map test should be last one since it changes default behaviour
    void test_map();

//private:
//    QString m_statusMessage;
//    QString m_statusData;
//    QString m_infoMessage;

private:
    struct TestData;
    void setup(TestData *data);
};

static SetupTestCallback setupTest = nullptr;


QObject *createFakeVimTester(SetupTestCallback cb)
{
    setupTest = cb;
    return new FakeVimTester;
}


// Format of message after comparison fails (used by KEYS, COMMAND).
static const QString helpFormat = _(
    "\n\tBefore command [%1]:\n" \
    LINE_START "%2" LINE_END \
    "\n\tAfter the command:\n" \
    LINE_START "%3" LINE_END \
    "\n\tShould be:\n" \
    LINE_START "%4" LINE_END);

static QByteArray textWithCursor(const QByteArray &text, int position)
{
    return (position == -1) ? text : (text.left(position) + X + text.mid(position));
}

static QByteArray textWithCursor(const QByteArray &text, const QTextBlock &block, int column)
{
    const int pos = block.position() + qMin(column, qMax(0, block.length() - 2));
    return text.left(pos) + X + text.mid(pos);
}

// Compare document contents with a expectedText.
// Also check cursor position if the expectedText contains | chracter.

// Send keys and check if the expected result is same as document contents.
// Escape is always prepended to keys so that previous command is cancelled.
#define KEYS(keys, expectedText) \
    do { \
        QByteArray beforeText(data.text()); \
        int beforePosition = data.position(); \
        data.doKeys(keys); \
        QByteArray actual(data.text()); \
        QByteArray expected = expectedText; \
        QByteArray desc = data.fixup(_(keys), beforeText, actual, expected, beforePosition); \
        QVERIFY2(actual == expected, desc.constData()); \
    } while (false)

// Run Ex command and check if the expected result is same as document contents.
#define COMMAND(cmd, expectedText) \
    do { \
        QByteArray beforeText(data.text()); \
        int beforePosition = data.position(); \
        data.doCommand(cmd); \
        QByteArray actual(data.text()); \
        QByteArray expected = expectedText; \
        QByteArray desc = data.fixup(_(":" cmd), beforeText, actual, expected, beforePosition); \
        QVERIFY2(actual == expected, desc.constData()); \
    } while (false)

// Test undo, redo and repeat of last single command. This doesn't test cursor position.
// Set afterEnd to true if cursor position after undo and redo differs at the end of line
// (e.g. undoing 'A' operation moves cursor at the end of line and redo moves it one char right).
#define INTEGRITY(afterEnd) \
    do { \
        data.doKeys("<ESC>"); \
        const int newPosition = data.position(); \
        const int oldPosition = data.oldPosition; \
        const QByteArray redo = data.text(); \
        KEYS("u", data.oldText); \
        const QTextCursor tc = data.cursor(); \
        const int pos = tc.position(); \
        const int col = tc.positionInBlock() \
            + ((afterEnd && tc.positionInBlock() + 2 == tc.block().length()) ? 1 : 0); \
        const int line = tc.block().blockNumber(); \
        const QTextDocument *doc = data.editor()->document(); \
        KEYS("<c-r>", textWithCursor(redo, doc->findBlockByNumber(line), col)); \
        KEYS("u", textWithCursor(data.oldText, pos)); \
        data.setPosition(oldPosition); \
        KEYS(".", textWithCursor(redo, newPosition)); \
    } while (false)


const QByteArray testLines =
  /* 0         1         2         3        4 */
  /* 0123456789012345678901234567890123457890 */
    "\n"
    "#include <QtCore>\n"
    "#include <QtGui>\n"
    "\n"
    "int main(int argc, char *argv[])\n"
    "{\n"
    "    QApplication app(argc, argv);\n"
    "\n"
    "    return app.exec();\n"
    "}\n";

const QList<QByteArray> l = testLines.split('\n');

static QByteArray bajoin(const QList<QByteArray> &balist)
{
    QByteArray res;
    for (int i = 0; i < balist.size(); ++i) {
        if (i)
            res += '\n';
        res += balist.at(i);
    }
    return res;
}

// Insert cursor char at pos, negative counts from back.
static QByteArray cursor(int line, int column)
{
    const int col = column >= 0 ? column : l[line].size() + column;
    QList<QByteArray> res = l.mid(0, line) << textWithCursor(l[line], col);
    res.append(l.mid(line + 1));
    return bajoin(res);
}

static QByteArray lmid(int i, int n = -1) { return bajoin(l.mid(i, n)); }

// Data for tests containing BaseTextEditorWidget and FakeVimHAndler.
struct FakeVimTester::TestData
{
    FakeVimHandler *handler;
    QWidget *edit;
    QString title;

    int oldPosition;
    QByteArray oldText;

    TextEditorWidget *editor() const { return qobject_cast<TextEditorWidget *>(edit); }

    QTextCursor cursor() const { return editor()->textCursor(); }

    QByteArray fixup(const QString &cmd, QByteArray &before,
                     QByteArray &actual, QByteArray &expected,
                     int beforePosition)
    {
        oldPosition = beforePosition;
        oldText = before;
        if (expected.contains(X)) {
            before = textWithCursor(before, beforePosition);
            actual = textWithCursor(actual, position());
        }
        return helpFormat
                .arg(cmd)
                .arg(QString::fromLatin1(before.replace('\n', LINE_END LINE_START)))
                .arg(QString::fromLatin1(actual.replace('\n', LINE_END LINE_START)))
                .arg(QString::fromLatin1(expected.replace('\n', LINE_END LINE_START))).toLatin1();
    }

    int position() const
    {
        return cursor().position();
    }

    void setPosition(int position)
    {
        handler->setTextCursorPosition(position);
    }

    QByteArray text() const { return editor()->toPlainText().toUtf8(); }

    void doCommand(const QString &cmd) { handler->handleCommand(cmd); }
    void doCommand(const char *cmd) { doCommand(_(cmd)); }
    void doKeys(const QString &keys) {
        handler->handleInput(keys);
        QTRY_VERIFY(editor()->textDocument()->syntaxHighlighter()->syntaxHighlighterUpToDate());
    }
    void doKeys(const char *keys) { doKeys(_(keys)); }

    void setText(const char *text)
    {
        doKeys("<ESC>");
        QByteArray str = text;
        int i = str.indexOf(X);
        if (i != -1)
            str.remove(i, 1);
        else
            i = 0;
        editor()->document()->setPlainText(_(str));
        QTRY_VERIFY(editor()->textDocument()->syntaxHighlighter()->syntaxHighlighterUpToDate());
        setPosition(i);
        QCOMPARE(position(), i);
    }

    // Simulate text completion by inserting text directly to editor widget (bypassing FakeVim).
    void completeText(const char *text)
    {
        QTextCursor tc = editor()->textCursor();
        tc.insertText(_(text));
        editor()->setTextCursor(tc);
        QTRY_VERIFY(editor()->textDocument()->syntaxHighlighter()->syntaxHighlighterUpToDate());
    }

    // Simulate external position change.
    void jump(const char *textWithCursorPosition)
    {
        int pos = QString(_(textWithCursorPosition)).indexOf(_(X));
        QTextCursor tc = editor()->textCursor();
        tc.setPosition(pos);
        editor()->setTextCursor(tc);
        QCOMPARE(QByteArray(textWithCursorPosition), textWithCursor(text(), position()));
    }

    int lines() const
    {
        QTextDocument *doc = editor()->document();
        Q_ASSERT(doc != nullptr);
        return doc->lineCount();
    }

    // Enter command mode and go to start.
    void reset()
    {
        handler->handleInput(_("<ESC><ESC>gg0"));
    }
};

void FakeVimTester::setup(TestData *data)
{
    setupTest(&data->title, &data->handler, &data->edit);
    data->reset();
    data->doCommand("| set nopasskeys"
                    "| set nopasscontrolkey"
                    "| set smartindent"
                    "| set autoindent");
}


void FakeVimTester::cleanup()
{
    Core::EditorManager::closeAllEditors(false);
}


void FakeVimTester::test_vim_indentation()
{
    TestData data;
    setup(&data);

    data.doCommand("set expandtab");
    data.doCommand("set tabstop=4");
    data.doCommand("set shiftwidth=4");
    QCOMPARE(data.handler->physicalIndentation(_("      \t\t\tx")), 6 + 3);
    QCOMPARE(data.handler->logicalIndentation (_("      \t\t\tx")), 4 + 3 * 4);
    QCOMPARE(data.handler->physicalIndentation(_("     \t\t\tx")), 5 + 3);
    QCOMPARE(data.handler->logicalIndentation (_("     \t\t\tx")), 4 + 3 * 4);

    QCOMPARE(data.handler->tabExpand(3), _("   "));
    QCOMPARE(data.handler->tabExpand(4), _("    "));
    QCOMPARE(data.handler->tabExpand(5), _("     "));
    QCOMPARE(data.handler->tabExpand(6), _("      "));
    QCOMPARE(data.handler->tabExpand(7), _("       "));
    QCOMPARE(data.handler->tabExpand(8), _("        "));
    QCOMPARE(data.handler->tabExpand(9), _("         "));

    data.doCommand("set expandtab");
    data.doCommand("set tabstop=8");
    data.doCommand("set shiftwidth=4");
    QCOMPARE(data.handler->physicalIndentation(_("      \t\t\tx")), 6 + 3);
    QCOMPARE(data.handler->logicalIndentation (_("      \t\t\tx")), 0 + 3 * 8);
    QCOMPARE(data.handler->physicalIndentation(_("     \t\t\tx")), 5 + 3);
    QCOMPARE(data.handler->logicalIndentation (_("     \t\t\tx")), 0 + 3 * 8);

    QCOMPARE(data.handler->tabExpand(3), _("   "));
    QCOMPARE(data.handler->tabExpand(4), _("    "));
    QCOMPARE(data.handler->tabExpand(5), _("     "));
    QCOMPARE(data.handler->tabExpand(6), _("      "));
    QCOMPARE(data.handler->tabExpand(7), _("       "));
    QCOMPARE(data.handler->tabExpand(8), _("        "));
    QCOMPARE(data.handler->tabExpand(9), _("         "));

    data.doCommand("set noexpandtab");
    data.doCommand("set tabstop=4");
    data.doCommand("set shiftwidth=4");
    QCOMPARE(data.handler->physicalIndentation(_("      \t\t\tx")), 6 + 3);
    QCOMPARE(data.handler->logicalIndentation (_("      \t\t\tx")), 4 + 3 * 4);
    QCOMPARE(data.handler->physicalIndentation(_("     \t\t\tx")), 5 + 3);
    QCOMPARE(data.handler->logicalIndentation (_("     \t\t\tx")), 4 + 3 * 4);

    QCOMPARE(data.handler->tabExpand(3), _("   "));
    QCOMPARE(data.handler->tabExpand(4), _("\t"));
    QCOMPARE(data.handler->tabExpand(5), _("\t "));
    QCOMPARE(data.handler->tabExpand(6), _("\t  "));
    QCOMPARE(data.handler->tabExpand(7), _("\t   "));
    QCOMPARE(data.handler->tabExpand(8), _("\t\t"));
    QCOMPARE(data.handler->tabExpand(9), _("\t\t "));

    data.doCommand("set noexpandtab");
    data.doCommand("set tabstop=8");
    data.doCommand("set shiftwidth=4");
    QCOMPARE(data.handler->physicalIndentation(_("      \t\t\tx")), 6 + 3);
    QCOMPARE(data.handler->logicalIndentation (_("      \t\t\tx")), 0 + 3 * 8);
    QCOMPARE(data.handler->physicalIndentation(_("     \t\t\tx")), 5 + 3);
    QCOMPARE(data.handler->logicalIndentation (_("     \t\t\tx")), 0 + 3 * 8);

    QCOMPARE(data.handler->tabExpand(3), _("   "));
    QCOMPARE(data.handler->tabExpand(4), _("    "));
    QCOMPARE(data.handler->tabExpand(5), _("     "));
    QCOMPARE(data.handler->tabExpand(6), _("      "));
    QCOMPARE(data.handler->tabExpand(7), _("       "));
    QCOMPARE(data.handler->tabExpand(8), _("\t"));
    QCOMPARE(data.handler->tabExpand(9), _("\t "));
}

void FakeVimTester::test_vim_readonly()
{
    TestData data;
    setup(&data);

    // On a read-only editor, normal-mode commands must not modify the text.
    data.setText("abc def");
    data.editor()->setReadOnly(true);
    KEYS("x", "abc def");
    KEYS("dd", "abc def");

    // The read-only state is evaluated live, not cached at setup: once the
    // editor becomes writable again (e.g. after a document reload triggered by
    // switching the encoding), FakeVim handles keys as usual again
    // (QTCREATORBUG-24237).
    data.editor()->setReadOnly(false);
    KEYS("x", X "bc def");
}

void FakeVimTester::test_vim_movement()
{
    TestData data;
    setup(&data);

    // vertical movement
    data.setText("123" N   "456" N   "789" N   "abc");
    KEYS("",   X "123" N   "456" N   "789" N   "abc");
    KEYS("j",    "123" N X "456" N   "789" N   "abc");
    KEYS("G",    "123" N   "456" N   "789" N X "abc");
    KEYS("k",    "123" N   "456" N X "789" N   "abc");
    KEYS("2k", X "123" N   "456" N   "789" N   "abc");
    KEYS("k",  X "123" N   "456" N   "789" N   "abc");
    KEYS("jj",   "123" N   "456" N X "789" N   "abc");
    KEYS("gg", X "123" N   "456" N   "789" N   "abc");

    // horizontal movement
    data.setText(" " X "x"   "x"   "x"   "x");
    KEYS("",     " " X "x"   "x"   "x"   "x");
    KEYS("h",  X " "   "x"   "x"   "x"   "x");
    KEYS("l",    " " X "x"   "x"   "x"   "x");
    KEYS("3l",   " "   "x"   "x"   "x" X "x");
    KEYS("2h",   " "   "x" X "x"   "x"   "x");
    KEYS("$",    " "   "x"   "x"   "x" X "x");
    KEYS("^",    " " X "x"   "x"   "x"   "x");
    KEYS("0",  X " "   "x"   "x"   "x"   "x");

    // skip words
    data.setText("123 "   "456"   "."   "789 "   "abc");
    KEYS("b",  X "123 "   "456"   "."   "789 "   "abc");
    KEYS("w",    "123 " X "456"   "."   "789 "   "abc");
    KEYS("2w",   "123 "   "456"   "." X "789 "   "abc");
    KEYS("3w",   "123 "   "456"   "."   "789 "   "ab" X "c");
    KEYS("3b",   "123 "   "456" X "."   "789 "   "abc");

    data.setText("123 "   "456.789 "   "abc "   "def");
    KEYS("B",  X "123 "   "456.789 "   "abc "   "def");
    KEYS("W",    "123 " X "456.789 "   "abc "   "def");
    KEYS("2W",   "123 "   "456.789 "   "abc " X "def");
    KEYS("B",    "123 "   "456.789 " X "abc "   "def");
    KEYS("2B", X "123 "   "456.789 "   "abc "   "def");
    KEYS("4W",   "123 "   "456.789 "   "abc "   "de" X "f");

    data.setText("assert(abc);");
    KEYS("w",    "assert" X "(abc);");
    KEYS("w",    "assert(" X "abc);");
    KEYS("w",    "assert(abc" X ");");
    KEYS("w",    "assert(abc)" X ";");

    data.setText("123" N   "45."   "6" N   "" N " " N   "789");
    KEYS("3w",   "123" N   "45." X "6" N   "" N " " N   "789");
    // From Vim help (motion.txt): An empty line is also considered to be a word.
    KEYS("w",    "123" N   "45."   "6" N X "" N " " N   "789");
    KEYS("w",    "123" N   "45."   "6" N   "" N " " N X "789");

    KEYS("b",    "123" N   "45."   "6" N X "" N " " N   "789");
    KEYS("4b", X "123" N   "45."   "6" N   "" N " " N   "789");

    KEYS("3e",    "123" N "45" X "."   "6" N "" N " " N "789");
    KEYS("e",     "123" N "45"   "." X "6" N "" N " " N "789");
    // Command "e" does not stop on empty lines ("ge" does).
    KEYS("e",     "123" N "45"   "."   "6" N "" N " " N "78" X "9");
    KEYS("ge",    "123" N "45"   "."   "6" N X "" N " " N "789");
    KEYS("2ge",   "123" N "45" X "."   "6" N   "" N " " N "789");

    // do not move behind end of line in normal mode
    data.setText("abc def" N "ghi");
    KEYS("$h", "abc d" X "ef" N "ghi");
    data.setText("abc def" N "ghi");
    KEYS("4e", "abc def" N "gh" X "i");
    data.setText("abc def" N "ghi");
    KEYS("$i", "abc de" X "f" N "ghi");

    // move behind end of line in insert mode
    data.setText("abc def" N "ghi");
    KEYS("i<end>", "abc def" X N "ghi");
    data.setText("abc def" N "ghi");
    KEYS("A", "abc def" X N "ghi");
    data.setText("abc def" N "ghi");
    KEYS("$a", "abc def" X N "ghi");

    data.setText("abc" N "def ghi");
    KEYS("i<end><down>", "abc" N "def ghi" X);
    data.setText("abc" N "def ghi");
    KEYS("<end>i<down>", "abc" N "de" X "f ghi");
    data.setText("abc" N "def ghi");
    KEYS("<end>a<down>", "abc" N "def" X " ghi");

    // paragraph movement
    data.setText("abc"  N   N "def");
    KEYS("}",     "abc" N X N "def");
    KEYS("}",     "abc" N   N "de" X "f");
    KEYS("{",     "abc" N X N "def");
    KEYS("{",   X "abc" N   N "def");

    data.setText("abc" N   N N   N "def");
    KEYS("}",    "abc" N X N N   N "def");
    KEYS("}",    "abc" N   N N   N "de" X "f");
    KEYS("3{",   "abc" N   N N   N "de" X "f");
    KEYS("{",    "abc" N   N N X N "def");
    KEYS("{",  X "abc" N   N N   N "def");
    KEYS("3}", X "abc" N   N N   N "def");

    data.setText("abc def");
    KEYS("}", "abc de" X "f");
    KEYS("{", X "abc def");

    // bracket movement commands
    data.setText(
         "void a()" N
         "{" N
         "}" N "" N "int b()" N
         "{ return 0; }" N "" N "int c()" N
         "{ return 0;" N
         "}" N
         "");

    KEYS("]]",
         "void a()" N
         X "{" N
         "}" N "" N "int b()" N
         "{ return 0; }" N "" N "int c()" N
         "{ return 0;" N
         "}" N
         "");

    KEYS("]]",
         "void a()" N
         "{" N
         "}" N "" N "int b()" N
         X "{ return 0; }" N "" N "int c()" N
         "{ return 0;" N
         "}" N
         "");

    KEYS("2[[",
         X "void a()" N
         "{" N
         "}" N "" N "int b()" N
         "{ return 0; }" N "" N "int c()" N
         "{ return 0;" N
         "}" N
         "");

    KEYS("4]]",
         "void a()" N
         "{" N
         "}" N "" N "int b()" N
         "{ return 0; }" N "" N "int c()" N
         "{ return 0;" N
         "}" N
         X "");

    KEYS("2[]",
         "void a()" N
         "{" N
         X "}" N "" N "int b()" N
         "{ return 0; }" N "" N "int c()" N
         "{ return 0;" N
         "}" N
         "");

    KEYS("][",
         "void a()" N
         "{" N
         "}" N "" N "int b()" N
         "{ return 0; }" N "" N "int c()" N
         "{ return 0;" N
         X "}" N
         "");

    KEYS("][",
         "void a()" N
         "{" N
         "}" N "" N "int b()" N
         "{ return 0; }" N "" N "int c()" N
         "{ return 0;" N
         "}" N
         X "");
}

void FakeVimTester::test_vim_target_column_normal()
{
    TestData data;
    setup(&data);
    data.setText("a"   "b"   "c"   N   "d"   "e"   N   ""   N   "k"   "l"   "m"   "n");

    // normal mode movement
    KEYS("",  X  "a"   "b"   "c"   N   "d"   "e"   N   ""   N   "k"   "l"   "m"   "n");
    KEYS("j",    "a"   "b"   "c"   N X "d"   "e"   N   ""   N   "k"   "l"   "m"   "n");
    KEYS("$",    "a"   "b"   "c"   N   "d" X "e"   N   ""   N   "k"   "l"   "m"   "n");
    KEYS("k",    "a"   "b" X "c"   N   "d"   "e"   N   ""   N   "k"   "l"   "m"   "n");
    KEYS("3j",   "a"   "b"   "c"   N   "d"   "e"   N   ""   N   "k"   "l"   "m" X "n");
    KEYS("02k",  "a"   "b"   "c"   N X "d"   "e"   N   ""   N   "k"   "l"   "m"   "n");
    KEYS("j",    "a"   "b"   "c"   N   "d"   "e"   N X ""   N   "k"   "l"   "m"   "n");
    KEYS("$",    "a"   "b"   "c"   N   "d"   "e"   N   "" X N   "k"   "l"   "m"   "n");
    KEYS("2k",   "a"   "b" X "c"   N   "d"   "e"   N   ""   N   "k"   "l"   "m"   "n");
    KEYS("jj2|", "a"   "b"   "c"   N   "d"   "e"   N X ""   N   "k"   "l"   "m"   "n");
    KEYS("j",    "a"   "b"   "c"   N   "d"   "e"   N   ""   N   "k" X "l"   "m"   "n");
    KEYS("gg", X "a"   "b"   "c"   N   "d"   "e"   N   ""   N   "k"   "l"   "m"   "n");
    KEYS("j",    "a"   "b"   "c"   N X "d"   "e"   N   ""   N   "k"   "l"   "m"   "n");
    KEYS("^k", X "a"   "b"   "c"   N   "d"   "e"   N   ""   N   "k"   "l"   "m"   "n");
}

void FakeVimTester::test_vim_target_column_visual_char()
{
    TestData data;
    setup(&data);
    data.setText("a"   "b"   "c"   N   "d"   "e"   N   ""   N   "k"   "l"   "m"   "n");

    KEYS("v", X  "a"   "b"   "c"   N   "d"   "e"   N   ""   N   "k"   "l"   "m"   "n");
    KEYS("j",    "a"   "b"   "c"   N X "d"   "e"   N   ""   N   "k"   "l"   "m"   "n");
    KEYS("$",    "a"   "b"   "c"   N   "d"   "e" X N   ""   N   "k"   "l"   "m"   "n");
    KEYS("k",    "a"   "b"   "c" X N   "d"   "e"   N   ""   N   "k"   "l"   "m"   "n");
    KEYS("3j",   "a"   "b"   "c"   N   "d"   "e"   N   ""   N   "k"   "l"   "m"   "n" X);
    KEYS("02k",  "a"   "b"   "c"   N X "d"   "e"   N   ""   N   "k"   "l"   "m"   "n");
    KEYS("j",    "a"   "b"   "c"   N   "d"   "e"   N X ""   N   "k"   "l"   "m"   "n");
    KEYS("$",    "a"   "b"   "c"   N   "d"   "e"   N   "" X N   "k"   "l"   "m"   "n");
    KEYS("2k",   "a"   "b"   "c" X N   "d"   "e"   N   ""   N   "k"   "l"   "m"   "n");
    KEYS("jj2|", "a"   "b"   "c"   N   "d"   "e"   N   "" X N   "k"   "l"   "m"   "n");
    KEYS("j",    "a"   "b"   "c"   N   "d"   "e"   N   ""   N   "k" X "l"   "m"   "n");
    KEYS("gg", X "a"   "b"   "c"   N   "d"   "e"   N   ""   N   "k"   "l"   "m"   "n");
    KEYS("j",    "a"   "b"   "c"   N X "d"   "e"   N   ""   N   "k"   "l"   "m"   "n");
    KEYS("^k", X "a"   "b"   "c"   N   "d"   "e"   N   ""   N   "k"   "l"   "m"   "n");
    KEYS("lO", X "a"   "b"   "c"   N   "d"   "e"   N   ""   N   "k"   "l"   "m"   "n");
    KEYS("<ESC>j",
                 "a"   "b"   "c"   N X "d"   "e"   N   ""   N   "k"   "l"   "m"   "n");
}

void FakeVimTester::test_vim_target_column_visual_block()
{
    TestData data;
    setup(&data);
    data.setText("a"   "b"   "c"   N   "d"   "e"   N   ""   N   "k"   "l"   "m"   "n");

    KEYS("<C-V>",
                 "a" X "b"   "c"   N   "d"   "e"   N   ""   N   "k"   "l"   "m"   "n");
    KEYS("j",    "a"   "b"   "c"   N   "d" X "e"   N   ""   N   "k"   "l"   "m"   "n");
    KEYS("$",    "a"   "b"   "c"   N   "d"   "e" X N   ""   N   "k"   "l"   "m"   "n");
    KEYS("k",    "a"   "b"   "c" X N   "d"   "e"   N   ""   N   "k"   "l"   "m"   "n");
    KEYS("3j",   "a"   "b"   "c"   N   "d"   "e"   N   ""   N   "k"   "l"   "m"   "n" X);
    KEYS("02k",  "a"   "b"   "c"   N   "d" X "e"   N   ""   N   "k"   "l"   "m"   "n");
    KEYS("j",    "a"   "b"   "c"   N   "d"   "e"   N   "" X N   "k"   "l"   "m"   "n");
    KEYS("$",    "a"   "b"   "c"   N   "d"   "e"   N   "" X N   "k"   "l"   "m"   "n");
    KEYS("2k",   "a"   "b"   "c" X N   "d"   "e"   N   ""   N   "k"   "l"   "m"   "n");
    KEYS("jj2|", "a"   "b"   "c"   N   "d"   "e"   N   "" X N   "k"   "l"   "m"   "n");
    KEYS("j",    "a"   "b"   "c"   N   "d"   "e"   N   ""   N   "k"   "l" X "m"   "n");
    KEYS("gg",   "a" X "b"   "c"   N   "d"   "e"   N   ""   N   "k"   "l"   "m"   "n");
    KEYS("j",    "a"   "b"   "c"   N   "d" X "e"   N   ""   N   "k"   "l"   "m"   "n");
    KEYS("^k",   "a" X "b"   "c"   N   "d"   "e"   N   ""   N   "k"   "l"   "m"   "n");
    KEYS("lO", X "a"   "b"   "c"   N   "d"   "e"   N   ""   N   "k"   "l"   "m"   "n");
    KEYS("<ESC>j",
                 "a"   "b"   "c"   N X "d"   "e"   N   ""   N   "k"   "l"   "m"   "n");
}

void FakeVimTester::test_vim_target_column_visual_line()
{
    TestData data;
    setup(&data);
    data.setText("a"   "b"   "c"   N   "d"   "e"   N   ""   N   "k"   "l"   "m"   "n");

    KEYS("lV<ESC>",    "a" X "b"   "c"   N   "d"   "e"   N   ""   N   "k"   "l"   "m"   "n");
    KEYS("Vj<ESC>",    "a"   "b"   "c"   N   "d" X "e"   N   ""   N   "k"   "l"   "m"   "n");
    KEYS("Vj<ESC>",    "a"   "b"   "c"   N   "d"   "e"   N X ""   N   "k"   "l"   "m"   "n");
    KEYS("Vj<ESC>",    "a"   "b"   "c"   N   "d"   "e"   N   ""   N   "k" X "l"   "m"   "n");
    KEYS("Vgg<ESC>", X "a"   "b"   "c"   N   "d"   "e"   N   ""   N   "k"   "l"   "m"   "n");

    NOT_IMPLEMENTED
    // Movement inside selection is not supported.
}

void FakeVimTester::test_vim_target_column_insert()
{
    TestData data;
    setup(&data);
    data.setText("a"   "b"   "c"   N   "d"   "e"   N   ""   N   "k"   "l"   "m"   "n");

    KEYS("i", X  "a"   "b"   "c"   N   "d"   "e"   N   ""   N   "k"   "l"   "m"   "n");
    KEYS("<C-O>j",    "a"   "b"   "c"   N X "d"   "e"   N   ""   N   "k"   "l"   "m"   "n");
    KEYS("<C-O>$",    "a"   "b"   "c"   N   "d"   "e" X N   ""   N   "k"   "l"   "m"   "n");
    KEYS("<C-O>k",    "a"   "b"   "c" X N   "d"   "e"   N   ""   N   "k"   "l"   "m"   "n");
    KEYS("<C-O>3j",   "a"   "b"   "c"   N   "d"   "e"   N   ""   N   "k"   "l"   "m"   "n" X);
    KEYS("<C-O>0<C-O>2k",
                      "a"   "b"   "c"   N X "d"   "e"   N   ""   N   "k"   "l"   "m"   "n");
    KEYS("<C-O>j",    "a"   "b"   "c"   N   "d"   "e"   N X ""   N   "k"   "l"   "m"   "n");
    KEYS("<C-O>$",    "a"   "b"   "c"   N   "d"   "e"   N   "" X N   "k"   "l"   "m"   "n");
    KEYS("<C-O>2k",   "a"   "b"   "c" X N   "d"   "e"   N   ""   N   "k"   "l"   "m"   "n");
    KEYS("<down><down><c-o>2|",
                      "a"   "b"   "c"   N   "d"   "e"   N   "" X N   "k"   "l"   "m"   "n");
    KEYS("<C-O>j",    "a"   "b"   "c"   N   "d"   "e"   N   ""   N   "k" X "l"   "m"   "n");
    KEYS("<C-O>gg", X "a"   "b"   "c"   N   "d"   "e"   N   ""   N   "k"   "l"   "m"   "n");
    KEYS("<C-O>j",    "a"   "b"   "c"   N X "d"   "e"   N   ""   N   "k"   "l"   "m"   "n");
    KEYS("<C-O>^<up>",
                    X "a"   "b"   "c"   N   "d"   "e"   N   ""   N   "k"   "l"   "m"   "n");
}

void FakeVimTester::test_vim_target_column_replace()
{
    TestData data;
    setup(&data);
    data.setText("a"   "b"   "c"   N   "d"   "e"   N   ""   N   "k"   "l"   "m"   "n");

    KEYS("i<insert>",
                   X  "a"   "b"   "c"   N   "d"   "e"   N   ""   N   "k"   "l"   "m"   "n");
    KEYS("<C-O>j",    "a"   "b"   "c"   N X "d"   "e"   N   ""   N   "k"   "l"   "m"   "n");
    KEYS("<C-O>$",    "a"   "b"   "c"   N   "d"   "e" X N   ""   N   "k"   "l"   "m"   "n");
    KEYS("<C-O>k",    "a"   "b"   "c" X N   "d"   "e"   N   ""   N   "k"   "l"   "m"   "n");
    KEYS("<C-O>3j",   "a"   "b"   "c"   N   "d"   "e"   N   ""   N   "k"   "l"   "m"   "n" X);
    KEYS("<C-O>0<C-O>2k",
                      "a"   "b"   "c"   N X "d"   "e"   N   ""   N   "k"   "l"   "m"   "n");
    KEYS("<C-O>j",    "a"   "b"   "c"   N   "d"   "e"   N X ""   N   "k"   "l"   "m"   "n");
    KEYS("<C-O>$",    "a"   "b"   "c"   N   "d"   "e"   N   "" X N   "k"   "l"   "m"   "n");
    KEYS("<C-O>2k",   "a"   "b"   "c" X N   "d"   "e"   N   ""   N   "k"   "l"   "m"   "n");
    KEYS("<down><down><c-o>2|",
                      "a"   "b"   "c"   N   "d"   "e"   N   "" X N   "k"   "l"   "m"   "n");
    KEYS("<C-O>j",    "a"   "b"   "c"   N   "d"   "e"   N   ""   N   "k" X "l"   "m"   "n");
    KEYS("<C-O>gg", X "a"   "b"   "c"   N   "d"   "e"   N   ""   N   "k"   "l"   "m"   "n");
    KEYS("<C-O>j",    "a"   "b"   "c"   N X "d"   "e"   N   ""   N   "k"   "l"   "m"   "n");
    KEYS("<C-O>^<up>",
                    X "a"   "b"   "c"   N   "d"   "e"   N   ""   N   "k"   "l"   "m"   "n");
}

void FakeVimTester::test_vim_insert()
{
    TestData data;
    setup(&data);

    // basic insert text
    data.setText("ab" X "c" N "def");
    KEYS("i 123", "ab 123" X "c" N "def");
    INTEGRITY(false);

    data.setText("ab" X "c" N "def");
    KEYS("a 123", "abc 123" X N "def");
    INTEGRITY(true);

    data.setText("ab" X "c" N "def");
    KEYS("I 123", " 123" X "abc" N "def");
    INTEGRITY(false);

    data.setText("abc" N "def");
    KEYS("A 123", "abc 123" X N "def");
    INTEGRITY(true);

    data.setText("abc" N "def");
    KEYS("o 123", "abc" N " 123" X N "def");
    INTEGRITY(false);

    data.setText("abc" N "def");
    KEYS("O 123", " 123" X N "abc" N "def");
    INTEGRITY(false);

    // insert text [count] times
    data.setText("ab" X "c" N "def");
    KEYS("3i 123<esc>", "ab 123 123 12" X "3c" N "def");
    INTEGRITY(false);

    data.setText("ab" X "c" N "def");
    KEYS("3a 123<esc>", "abc 123 123 12" X "3" N "def");
    INTEGRITY(true);

    data.setText("ab" X "c" N "def");
    KEYS("3I 123<esc>", " 123 123 12" X "3abc" N "def");
    INTEGRITY(false);

    data.setText("abc" N "def");
    KEYS("3A 123<esc>", "abc 123 123 12" X "3" N "def");
    INTEGRITY(true);

    data.setText("abc" N "def");
    KEYS("3o 123<esc>", "abc" N " 123" N " 123" N " 12" X "3" N "def");
    INTEGRITY(false);

    data.setText("abc" N "def");
    KEYS("3O 123<esc>", " 123" N " 123" N " 12" X "3" N "abc" N "def");
    INTEGRITY(false);

    // <C-O>
    data.setText("abc" N "d" X "ef");
    KEYS("i<c-o>xX", "abc" N "dX" X "f");
    data.doKeys("<ESC>");
    KEYS("i<c-o><end>", "abc" N "dXf" X);
    data.setText("ab" X "c" N "def");
    KEYS("i<c-o>rX", "ab" X "X" N "def");
    data.setText("abc" N "def");
    KEYS("A<c-o>x", "ab" X N "def");
    data.setText("abc" N "de" X "f");
    KEYS("i<c-o>0x", "abc" N "x" X "def");
    data.setText("abc" N "de" X "f");
    KEYS("i<c-o>ggx", "x" X "abc" N "def");
    data.setText("abc" N "def" N "ghi");
    KEYS("i<c-o>vjlolx", "a" X "f" N "ghi");

    // <INSERT> to toggle between insert and replace mode
    data.setText("abc" N "def");
    KEYS("<insert>XYZ<insert>xyz<esc>", "XYZxy" X "z" N "def");
    KEYS("<insert><insert>" "<c-o>0<c-o>j" "XY<insert>Z", "XYZxyz" N "XYZ" X "f");

    // dot command for insert
    data.setText("abc" N "def");
    KEYS("ix<insert>X<insert>y<esc>", "xX" X "ybc" N "def");
    KEYS("0j.", "xXybc" N "xX" X "yef");

    data.setText("abc" N "def");
    KEYS("<insert>x<insert>X<right>Y<esc>", "xXb" X "Y" N "def");
    KEYS("0j.", "xXbY" N X "Yef");

    data.setText("abc" N "def");
    KEYS("<insert>x<insert>X<left><left><down><esc>", "xXbc" N X "def");
    KEYS(".", "xXbc" N "x" X "Xef");

    data.setText("abc" N "def");
    KEYS("2oXYZ<esc>.", "abc" N "XYZ" N "XYZ" N "XYZ" N "XY" X "Z" N "def");

    // delete in insert mode is part of dot command
    data.setText("abc" N "def");
    KEYS("iX<delete>Y", "XY" X "bc" N "def");
    data.doKeys("<ESC>");
    KEYS("0j.", "XYbc" N "X" X "Yef");

    data.setText("abc" N "def");
    KEYS("2iX<delete>Y<esc>", "XYX" X "Yc" N "def");
    KEYS("0j.", "XYXYc" N "XYX" X "Yf");

    data.setText("abc" N "def");
    KEYS("i<delete>XY", "XY" X "bc" N "def");
    data.doKeys("<ESC>");
    KEYS("0j.", "XYbc" N "X" X "Yef");

    data.setText("ab" X "c" N "def");
    KEYS("i<bs>XY", "aXY" X "c" N "def");
    data.doKeys("<ESC>");
    KEYS("j.", "aXYc" N "dX" X "Yf");

    // insert in visual mode
    data.setText("  a" X "bcde" N "  fghij" N "  klmno");
    KEYS("v2l" "2Ixyz<esc>", "xyzxy" X "z  abcde" N "  fghij" N "  klmno");
    KEYS("u", X "  abcde" N "  fghij" N "  klmno");
    KEYS("<c-r>", X "xyzxyz  abcde" N "  fghij" N "  klmno");
    KEYS("$.", "xyzxyz  abcdxyzxy" X "ze" N "  fghij" N "  klmno");

    // repeat only last insertion
    data.setText("  abc" N "  def" N "  ghi");
    KEYS("2l" "2i" "XYZ" "<C-O>j" "123<esc>", "  XYZabc" N "  def12" X "3" N "  ghi");
    KEYS("0l.", "  XYZabc" N " 12" X "3 def123" N "  ghi");
    // insert nothing
    KEYS("i<esc>", "  XYZabc" N " 1" X "23 def123" N "  ghi");
    KEYS(".", "  XYZabc" N " " X "123 def123" N "  ghi");

    // repeat insert with special characters
    data.setText("ab" X "c" N "def");
    KEYS("2i<lt>down><esc>", "ab<down><down" X ">c" N "def");
    INTEGRITY(false);

    data.setText("  ab" X "c" N "  def");
    KEYS("2I<lt>end><esc>", "  <end><end" X ">abc" N "  def");
    KEYS("u", "  " X "abc" N "  def");
    KEYS(".", "  <end><end" X ">abc" N "  def");
}

void FakeVimTester::test_vim_fFtT()
{
    TestData data;
    setup(&data);

    data.setText("123()456" N "a(b(c)d)e");
    KEYS("t(", "12" X "3()456" N "a(b(c)d)e");
    KEYS("lt(", "123" X "()456" N "a(b(c)d)e");
    KEYS("0j2t(", "123()456" N "a(" X "b(c)d)e");
    KEYS("l2T(", "123()456" N "a(b" X "(c)d)e");
    KEYS("l2T(", "123()456" N "a(" X "b(c)d)e");
    KEYS("T(", "123()456" N "a(" X "b(c)d)e");

    KEYS("ggf(", "123" X "()456" N "a(b(c)d)e");
    KEYS("lf(", "123(" X ")456" N "a(b(c)d)e");
    KEYS("0j2f(", "123()456" N "a(b" X "(c)d)e");
    KEYS("2F(", "123()456" N "a(b" X "(c)d)e");
    KEYS("l2F(", "123()456" N "a" X "(b(c)d)e");
    KEYS("F(", "123()456" N "a" X "(b(c)d)e");

    data.setText("abc def" N "ghi " X "jkl");
    KEYS("vFgx", "abc def" N X "kl");
    KEYS("u", "abc def" N X "ghi jkl");
    KEYS("tk", "abc def" N "ghi " X "jkl");
    KEYS("dTg", "abc def" N "g" X "jkl");
    INTEGRITY(false);
    KEYS("u", "abc def" N "g" X "hi jkl");
    KEYS("f .", "abc def" N "g" X " jkl");
    KEYS("u", "abc def" N "g" X "hi jkl");
    KEYS("rg$;", "abc def" N "gg" X "i jkl");

    // repeat with ;
    data.setText("int main() { return (x > 0) ? 0 : (x - 1); }");
    KEYS("f(", "int main" X "() { return (x > 0) ? 0 : (x - 1); }");
    KEYS(";", "int main() { return " X "(x > 0) ? 0 : (x - 1); }");
    KEYS(";", "int main() { return (x > 0) ? 0 : " X "(x - 1); }");
    KEYS(";", "int main() { return (x > 0) ? 0 : " X "(x - 1); }");
    KEYS("02;", "int main() { return " X "(x > 0) ? 0 : (x - 1); }");
    KEYS("2;", "int main() { return " X "(x > 0) ? 0 : (x - 1); }");
    KEYS("0t(", "int mai" X "n() { return (x > 0) ? 0 : (x - 1); }");
    KEYS(";", "int main() { return" X " (x > 0) ? 0 : (x - 1); }");
    KEYS("3;", "int main() { return" X " (x > 0) ? 0 : (x - 1); }");
    KEYS("2;", "int main() { return (x > 0) ? 0 :" X " (x - 1); }");

    // "," repeats the last f/F/t/T in the opposite direction
    // (QTCREATORBUG-12115).
    data.setText(X "a.b.c.d");
    KEYS("f.", "a" X ".b.c.d");
    KEYS(";",  "a.b" X ".c.d");
    KEYS(";",  "a.b.c" X ".d");
    KEYS(",",  "a.b" X ".c.d");
    KEYS(",",  "a" X ".b.c.d");
}

void FakeVimTester::test_vim_transform_numbers()
{
    TestData data;
    setup(&data);

    data.setText("8");
    KEYS("<c-a>", X "9");
    INTEGRITY(false);
    KEYS("<c-x>", X "8");
    INTEGRITY(false);
    KEYS("<c-a>", X "9");
    KEYS("<c-a>", "1" X "0");
    KEYS("<c-a>", "1" X "1");
    KEYS("5<c-a>", "1" X "6");
    INTEGRITY(false);
    KEYS("10<c-a>", "2" X "6");
    KEYS("h100<c-a>", "12" X "6");
    KEYS("100<c-x>", "2" X "6");
    INTEGRITY(false);
    KEYS("10<c-x>", "1" X "6");
    KEYS("5<c-x>", "1" X "1");
    KEYS("5<c-x>", X "6");
    KEYS("6<c-x>", X "0");
    KEYS("<c-x>", "-" X "1");
    KEYS("h10<c-x>", "-1" X "1");
    KEYS("h100<c-x>", "-11" X "1");
    KEYS("h889<c-x>", "-100" X "0");

    // increase nearest number
    data.setText("x-x+x: 1 2 3 -4 5");
    KEYS("8<c-a>", "x-x+x: " X "9 2 3 -4 5");
    KEYS("l8<c-a>", "x-x+x: 9 1" X "0 3 -4 5");
    KEYS("l8<c-a>", "x-x+x: 9 10 1" X "1 -4 5");
    KEYS("l16<c-a>", "x-x+x: 9 10 11 1" X "2 5");
    KEYS("w18<c-x>", "x-x+x: 9 10 11 12 -1" X "3");
    KEYS("hh13<c-a>", "x-x+x: 9 10 11 12 " X "0");
    KEYS("B12<c-x>", "x-x+x: 9 10 11 " X "0 0");
    KEYS("B11<c-x>", "x-x+x: 9 10 " X "0 0 0");
    KEYS("B10<c-x>", "x-x+x: 9 " X "0 0 0 0");
    KEYS("B9<c-x>", "x-x+x: " X "0 0 0 0 0");
    KEYS("B9<c-x>", "x-x+x: -" X "9 0 0 0 0");

    data.setText("-" X "- 1 --");
    KEYS("<c-x>", "-- " X "0 --");
    KEYS("u", "-" X "- 1 --");
    KEYS("<c-r>", "-" X "- 0 --");
    KEYS("<c-x><c-x>", "-- -" X "2 --");
    KEYS("2<c-a><c-a>", "-- " X "1 --");
    KEYS("<c-a>2<c-a>", "-- " X "4 --");
    KEYS(".", "-- " X "6 --");
    KEYS("u", "-- " X "4 --");
    KEYS("<c-r>", "-- " X "6 --");

    // hexadecimal and octal numbers
    data.setText("0x0 0x1 -1 07 08");
    KEYS("3<c-a>", "0x" X "3 0x1 -1 07 08");
    KEYS("7<c-a>", "0x" X "a 0x1 -1 07 08");
    KEYS("9<c-a>", "0x1" X "3 0x1 -1 07 08");
    // if last letter in hexadecimal number is capital then all letters are capital
    KEYS("ifA<esc>", "0x1f" X "A3 0x1 -1 07 08");
    KEYS("9<c-a>", "0x1FA" X "C 0x1 -1 07 08");
    KEYS("w1022<c-a>", "0x1FAC 0x3f" X "f -1 07 08");
    KEYS("w.", "0x1FAC 0x3ff 102" X "1 07 08");
    // octal number
    KEYS("w.", "0x1FAC 0x3ff 1021 0200" X "5 08");
    // non-octal number with leading zeroes
    KEYS("w.", "0x1FAC 0x3ff 1021 02005 103" X "0");

    // preserve width of hexadecimal and octal numbers
    data.setText("0x0001");
    KEYS("<c-a>", "0x000" X "2");
    KEYS("10<c-a>", "0x000" X "c");
    KEYS(".", "0x001" X "6");
    KEYS("999<c-a>", "0x03f" X "d");
    KEYS("99999<c-a>", "0x18a9" X "c");
    data.setText("0001");
    KEYS("<c-a>", "000" X "2");
    KEYS("10<c-a>", "001" X "4");
    KEYS("999<c-a>", "0176" X "3");
    data.setText("0x0100");
    KEYS("<c-x>", "0x00f" X "f");
    data.setText("0100");
    KEYS("<c-x>", "007" X "7");
}

void FakeVimTester::test_vim_delete()
{
    TestData data;
    setup(&data);

    data.setText("123" N "456");
    KEYS("x",  "23" N "456");
    INTEGRITY(false);
    KEYS("dd", "456");
    INTEGRITY(false);
    KEYS("2x", "6");
    INTEGRITY(false);
    KEYS("dd", "");
    INTEGRITY(false);

    // delete character / word / line in insert mode
    data.setText("123" N "456 789");
    KEYS("A<C-h>", "12" N "456 789");
    KEYS("<C-u>", "" N "456 789");
    KEYS("<Esc>jA<C-w>", "" N "456 ");

    data.setText("void main()");
    KEYS("dt(", "()");
    INTEGRITY(false);

    data.setText("void main()");
    KEYS("df(", ")");
    INTEGRITY(false);

    data.setText("void " X "main()");
    KEYS("D", "void ");
    INTEGRITY(false);
    KEYS("ggd$", "");

    data.setText("abc def ghi");
    KEYS("2dw", X "ghi");
    INTEGRITY(false);
    data.setText("abc def ghi");
    KEYS("d2w", X "ghi");
    INTEGRITY(false);

    data.setText("abc  " N "  def" N "  ghi" N "jkl");
    KEYS("3dw", X "jkl");
    data.setText("abc  " N "  def" N "  ghi" N "jkl");
    KEYS("d3w", X "jkl");

    // delete empty line
    data.setText("a" N X "" N "  b");
    KEYS("dd", "a" N "  " X "b");

    // delete on an empty line
    data.setText("a" N X "" N "  b");
    KEYS("d$", "a" N X "" N "  b");
    INTEGRITY(false);

    // delete in empty document
    data.setText("");
    KEYS("dd", X);

    // delete to start of line
    data.setText("  abc" N "  de" X "f" N "  ghi");
    KEYS("d0", "  abc" N X "f" N "  ghi");
    INTEGRITY(false);
    data.setText("  abc" N "  de" X "f" N "  ghi");
    KEYS("d^", "  abc" N "  " X "f" N "  ghi");
    INTEGRITY(false);

    // delete to mark
    data.setText("abc " X "def ghi");
    KEYS("ma" "3l" "d`a", "abc " X " ghi");
    KEYS("u" "gg" "d`a", X "def ghi");

    // delete lines
    data.setText("  abc" N "  de" X "f" N "  ghi" N "  jkl");
    KEYS("dj", "  abc" N "  " X "jkl");
    INTEGRITY(false);
    data.setText("  abc" N "  def" N "  gh" X "i" N "  jkl");
    KEYS("dk", "  abc" N "  " X "jkl");
    INTEGRITY(false);

    // delete with copy to a register
    data.setText("abc" N "def");
    KEYS("\"xd$", X "" N "def");
    KEYS("\"xp", "ab" X "c" N "def");
    KEYS("2\"xp", "abcabcab" X "c" N "def");

    /* QTCREATORBUG-9289 */
    data.setText("abc" N "def");
    KEYS("$" "dw", "a" X "b" N "def");
    KEYS("dw", X "a" N "def");
    KEYS("dw", X "" N "def");
    KEYS("dw", X "def");

    data.setText("abc" N "def ghi");
    KEYS("2dw", X "ghi");

    data.setText("abc" N X "" N "def");
    KEYS("dw", "abc" N X "def");
    KEYS("k$" "dw", "a" X "b" N "def");
    KEYS("j$h" "dw", "ab" N X "d");

    data.setText("abc" N "def");
    KEYS("2lvx", "a" X "b" N "def");
    KEYS("vlx", "a" X "def");

    data.setText("abc" N "def");
    KEYS("2lvox", "a" X "b" N "def");
    KEYS("vlox", "a" X "def");

    // bracket movement command
    data.setText(
         "void a()" N
         "{" N
         "}" N "" N "int b()" N
         "{ return 0; }" N "" N "int c()" N
         "{ return 0;" N
         "}" N
         "");

    KEYS("d]]",
         X "{" N
         "}" N "" N "int b()" N
         "{ return 0; }" N "" N "int c()" N
         "{ return 0;" N
         "}" N
         "");

    KEYS("u",
         X "void a()" N
         "{" N
         "}" N "" N "int b()" N
         "{ return 0; }" N "" N "int c()" N
         "{ return 0;" N
         "}" N
         "");

    // When ]] is used after an operator, then also stops below a '}' in the first column.
    KEYS("jd]]",
         "void a()" N
         X "" N "int b()" N
         "{ return 0; }" N "" N "int c()" N
         "{ return 0;" N
         "}" N
         "");

    KEYS("u",
         "void a()" N
         X "{" N
         "}" N "" N "int b()" N
         "{ return 0; }" N "" N "int c()" N
         "{ return 0;" N
         "}" N
         "");

    // do nothing on failed movement
    KEYS("Gd5[[",
         "void a()" N
         "{" N
         "}" N "" N "int b()" N
         "{ return 0; }" N "" N "int c()" N
         "{ return 0;" N
         "}" N
         X "");
}

void FakeVimTester::test_vim_delete_inner_word()
{
    TestData data;
    setup(&data);

    data.setText("abc def ghi");
    KEYS("wlldiw", "abc " X " ghi");

    data.setText("abc def ghi jkl");
    KEYS("3diw", X  " ghi jkl");
    INTEGRITY(false);

    data.setText("abc " X "  def");
    KEYS("diw", "abc" X "def");
    INTEGRITY(false);
    KEYS("diw", "");

    data.setText("abc  " N "  def");
    KEYS("3diw", X "def");

    data.setText("abc  " N "  def" N "  ghi");
    KEYS("4diw", "  " X "ghi");
    data.setText("ab" X "c  " N "  def" N "  ghi");
    KEYS("4diw", "  " X "ghi");
    data.setText("a b" X "c  " N "  def" N "  ghi");
    KEYS("4diw", "a" X " " N "  ghi");

    data.setText("abc def" N "ghi");
    KEYS("2diw", X "def" N "ghi");
    data.setText("abc def" N "ghi");
    KEYS("3diw", X "" N "ghi");

    data.setText("x" N X "" N "" N "  ");
    KEYS("diw", "x" N X "" N "" N "  ");
    data.setText("x" N X "" N "" N "  ");
    KEYS("2diw", "x" N " " X " ");
    data.setText("x" N X "" N "" N "" N "" N "  ");
    KEYS("3diw", "x" N " " X " ");
    data.setText("x" N X "" N "" N "" N "" N "" N "  ");
    KEYS("3diw", "x" N X "" N "  ");
    data.setText("x" N X "" N "" N "" N "" N "" N "" N "  ");
    KEYS("4diw", "x" N X "" N "  ");

    // delete single-character-word
    data.setText("a " X "b c");
    KEYS("diw", "a " X " c");
}

void FakeVimTester::test_vim_delete_a_word()
{
    TestData data;
    setup(&data);

    data.setText("abc def ghi");
    KEYS("wlldaw", "abc " X "ghi");

    data.setText("abc def ghi jkl");
    KEYS("wll2daw", "abc " X "jkl");

    data.setText("abc" X " def ghi");
    KEYS("daw", "abc" X " ghi");
    INTEGRITY(false);
    KEYS("daw", "ab" X "c");
    INTEGRITY(false);
    KEYS("daw", "");

    data.setText(X " ghi jkl");
    KEYS("daw", X " jkl");
    KEYS("ldaw", X " ");

    data.setText("abc def ghi jkl");
    KEYS("3daw", X "jkl");
    INTEGRITY(false);

    // remove trailing spaces
    data.setText("abc  " N "  def" N "  ghi" N "jkl");
    KEYS("3daw", X "jkl");

    data.setText("abc  " N "  def" N "  ghi" N "jkl");
    KEYS("3daw", X "jkl");

    data.setText("abc def" N "ghi");
    KEYS("2daw", X "" N "ghi");

    data.setText("x" N X "" N "" N "  ");
    KEYS("daw", "x" N " " X " ");
    data.setText("x" N X "" N "" N "" N "" N "  ");
    KEYS("2daw", "x" N " " X " ");
    data.setText("x" N X "" N "" N "" N "" N "" N "  ");
    KEYS("2daw", "x" N X "" N "  ");
    data.setText("x" N X "" N "" N "" N "" N "" N "" N "  ");
    KEYS("3daw", "x" N " " X " ");

    // delete single-character-word
    data.setText("a," X "b,c");
    KEYS("daw", "a," X ",c");

    // delete a word with visual selection
    data.setText(X "a" N "" N "b");
    KEYS("vawd", X "" N "" N "b");
    data.setText(X "a" N "" N "b");
    KEYS("Vawd", X "" N "" N "b");

    data.setText("abc def g" X "hi");
    KEYS("vawd", "abc de" X "f");
    KEYS("u", "abc def" X " ghi");

    // backward visual selection
    data.setText("abc def g" X "hi");
    KEYS("vhawd", "abc " X "i");

    data.setText("abc def gh" X "i");
    KEYS("vhawd", "abc de" X "f");

    data.setText("abc def gh" X "i");
    KEYS("vh2awd", "ab" X "c");
}

void FakeVimTester::test_vim_change_a_word()
{
    TestData data;
    setup(&data);

    data.setText("abc " X "def ghi");
    KEYS("caw#", "abc #" X "ghi");
    INTEGRITY(false);
    data.setText("abc d" X "ef ghi");
    KEYS("caw#", "abc #" X "ghi");
    data.setText("abc de" X "f ghi");
    KEYS("caw#", "abc #" X "ghi");

    data.setText("abc de" X "f ghi jkl");
    KEYS("2caw#", "abc #" X "jkl");
    INTEGRITY(false);

    data.setText("abc" X " def ghi jkl");
    KEYS("2caw#", "abc#" X " jkl");

    data.setText("abc " X "  def ghi jkl");
    KEYS("2caw#", "abc#" X " jkl");

    data.setText(" abc  " N "  def" N "  ghi" N " jkl");
    KEYS("3caw#", "#" X N " jkl");

    // change single-character-word
    data.setText("a " X "b c");
    KEYS("ciwX<esc>", "a " X "X c");
    KEYS("cawZ<esc>", "a " X "Zc");
}

void FakeVimTester::test_vim_change_replace()
{
    TestData data;
    setup(&data);

    // preserve lines in replace mode
    data.setText("abc" N "def");
    KEYS("llvjhrX", "ab" X "X" N "XXf");

    // change empty line
    data.setText("a" N X "" N "  b");
    KEYS("ccABC", "a" N "ABC" X N "  b");
    INTEGRITY(false);

    // change on empty line
    data.setText("a" N X "" N "  b");
    KEYS("c$ABC<esc>", "a" N "AB" X "C" N "  b");
    INTEGRITY(false);
    KEYS("u", "a" N X "" N "  b");
    KEYS("rA", "a" N X "" N "  b");

    // change in empty document
    data.setText("");
    KEYS("ccABC", "ABC" X);
    data.doKeys("<ESC>");
    KEYS("u", "");
    KEYS("SABC", "ABC" X);
    data.doKeys("<ESC>");
    KEYS("u", "");
    KEYS("sABC", "ABC" X);
    data.doKeys("<ESC>");
    KEYS("u", "");
    KEYS("rA", "" X);

    // indentation with change
    data.doCommand("set expandtab");
    data.doCommand("set shiftwidth=2");
    data.setText("int main()" N
         "{" N
         " " X "   return 0;" N
         "}" N
         "");

    KEYS("cc" "int i = 0;",
         "int main()" N
         "{" N
         "  int i = 0;" X N
         "}" N
         "");
    INTEGRITY(false);

    KEYS("uS" "int i = 0;" N "int j = 1;",
         "int main()" N
         "{" N
         "  int i = 0;" N
         "  int j = 1;" X N
         "}" N
         "");

    // change to start of line
    data.setText("  abc" N "  de" X "f" N "  ghi");
    KEYS("c0123<ESC>", "  abc" N "12" X "3f" N "  ghi");
    INTEGRITY(false);
    data.setText("  abc" N "  de" X "f" N "  ghi");
    KEYS("c^123<ESC>", "  abc" N "  12" X "3f" N "  ghi");
    INTEGRITY(false);

    // change to mark
    data.setText("abc " X "def ghi");
    KEYS("ma" "3l" "c`a123<ESC>", "abc 12" X "3 ghi");
    KEYS("u" "gg" "c`a123<ESC>", "12" X "3def ghi");

    // change lines
    data.setText("  abc" N "  de" X "f" N "  ghi" N "  jkl");
    KEYS("cj123<ESC>", "  abc" N "  12" X "3" N "  jkl");
    INTEGRITY(false);
    data.setText("  abc" N "  def" N "  gh" X "i" N "  jkl");
    KEYS("ck123<ESC>", "  abc" N "  12" X "3" N "  jkl");
    INTEGRITY(false);

    data.setText("abc" N X "def");
    KEYS("sXYZ", "abc" N "XYZ" X "ef");
    INTEGRITY(false);

    data.setText("abc" N X "def");
    KEYS("2sXYZ<ESC>", "abc" N "XY" X "Zf");
    INTEGRITY(false);

    data.setText("abc" N X "def");
    KEYS("6sXYZ<ESC>", "abc" N "XY" X "Z");
    INTEGRITY(false);

    // insert in visual block mode
    data.setText(
        "abc" N
        "d" X "ef" N
        "" N
        "jkl" N
        "mno" N
    );
    KEYS("<c-v>2j2sXYZ<esc>",
        "abc" N
        "dXY" X "Zf" N
        "" N
        "jXYZl" N
        "mno" N
    );
    INTEGRITY(false);

    data.setText(
        "abc" N
        "de" X "f" N
        "" N
        "jkl" N
        "mno" N
    );
    KEYS("<c-v>2jh2sXYZ<esc>",
        "abc" N
        "dXY" X "Z" N
        "" N
        "jXYZ" N
        "mno" N
    );
    INTEGRITY(false);

    // change with copy to a register
    data.setText("abc" N "def");
    KEYS("\"xCxyz<esc>", "xy" X "z" N "def");
    KEYS("\"xp", "xyzab" X "c" N "def");
    KEYS("2\"xp", "xyzabcabcab" X "c" N "def");

    // In Replace mode <BS> restores the overwritten characters and moves left,
    // rather than deleting them (QTCREATORBUG-12120).
    data.setText(X "abcdef");
    KEYS("RXY", "XY" X "cdef");   // overwrite 'a' and 'b'
    KEYS("<bs>", "X" X "bcdef");  // restore 'b'
    KEYS("<bs>", X "abcdef");     // restore 'a'
    KEYS("<bs>", X "abcdef");     // nothing left to restore, cursor stays put

    // Characters appended past the end of the line are removed again by <BS>,
    // there is nothing to restore for them.
    data.setText("ab" X "c");
    KEYS("RXYZ", "abXYZ" X);      // overwrite 'c', then append 'Y' and 'Z'
    KEYS("<bs>", "abXY" X);       // remove appended 'Z'
    KEYS("<bs>", "abX" X);        // remove appended 'Y'
    KEYS("<bs>", "ab" X "c");     // restore original 'c'

    // In Replace mode <Del> removes the character under the cursor.
    data.setText(X "abcdef");
    KEYS("R<delete>", X "bcdef");
    KEYS("<delete>", X "cdef");
}

void FakeVimTester::test_vim_block_selection()
{
    TestData data;
    setup(&data);

    data.setText("int main(int /* (unused) */, char *argv[]);");
    KEYS("f(", "int main" X "(int /* (unused) */, char *argv[]);");
    KEYS("da(", "int main" X ";");
    INTEGRITY(false);

    data.setText("int main(int /* (unused) */, char *argv[]);");
    KEYS("f(", "int main" X "(int /* (unused) */, char *argv[]);");
    KEYS("di(", "int main(" X ");");
    INTEGRITY(false);

    data.setText("int main(int /* (unused) */, char *argv[]);");
    KEYS("2f)", "int main(int /* (unused) */, char *argv[]" X ");");
    KEYS("da(", "int main" X ";");

    data.setText("int main(int /* (unused) */, char *argv[]);");
    KEYS("2f)", "int main(int /* (unused) */, char *argv[]" X ");");
    KEYS("di(", "int main(" X ");");

    data.setText("{ { { } } }");
    KEYS("2f{l", "{ { {" X " } } }");
    KEYS("da{", "{ { " X " } }");
    KEYS("da{", "{ " X " }");
    INTEGRITY(false);

    data.setText("{ { { } } }");
    KEYS("2f{l", "{ { {" X " } } }");
    KEYS("2da{", "{ " X " }");
    INTEGRITY(false);

    data.setText("{" N " { " N " } " N "}");
    KEYS("di{", "{" N "}");

    data.setText("(" X "())");
    KEYS("di(", "((" X "))");
    data.setText("\"\"");
    KEYS("di\"", "\"" X "\"");

    // visual selection
    data.setText("(abc()" X "(def))");
    KEYS("vi(d", "(abc()(" X "))");
    KEYS("u", "(abc()(" X "def))");
    KEYS("<c-r>", "(abc()(" X "))");
    KEYS("va(d", "(abc()" X ")");
    KEYS("u", "(abc()" X "())");
    KEYS("<c-r>", "(abc()" X ")");

    data.setText("\"abc" X "\"\"def\"");
    KEYS("vi\"d", "\"" X "\"\"def\"");

    /* QTCREATORBUG-9190 */
    data.setText(" abcd" N " efgh" N " ijkl" N " mnop" N "");
    data.doKeys("2lj" "<C-V>" "jl");
    data.doKeys("x");
    COMMAND("", " abcd" N " e" X "h" N " il" N " mnop" N "");
    COMMAND(":undo", " abcd" N " e" X "fgh" N " ijkl" N " mnop" N "");
    data.doKeys("<C-V>");
    data.doKeys("j");
    data.doKeys("l");
    data.doKeys("x");
    COMMAND("", " abcd" N " e" X "h" N " il" N " mnop" N "");
    COMMAND(":undo", " abcd" N " e" X "fgh" N " ijkl" N " mnop" N "");
    data.doKeys("gv");
    data.doKeys("j");
    data.doKeys("h");
    data.doKeys("x");
    COMMAND("", " abcd" N " e" X "gh" N " ikl" N " mop" N "");
    COMMAND(":undo", " abcd" N " e" X "fgh" N " ijkl" N " mnop" N "");
    data.doCommand("set passkeys");
    data.doKeys("gv");
    data.doKeys("k");
    data.doKeys("l");
    data.doKeys("r-");
    COMMAND("", " abcd" N " e" X "--h" N " i--l" N " mnop" N "");
    COMMAND(":undo", " abcd" N " e" X "fgh" N " ijkl" N " mnop" N "");
    data.doKeys("gv");
    data.doKeys("j");
    data.doKeys("o");
    data.doKeys("k");
    data.doKeys("h");
    data.doKeys("r9");
    COMMAND("", " " X "999d" N " 999h" N " 999l" N " 999p" N "");
    COMMAND(":undo", " " X "abcd" N " efgh" N " ijkl" N " mnop" N "");
    data.doCommand("set nopasskeys");

    // repeat change inner
    data.setText("(abc)" N "def" N "(ghi)");
    KEYS("ci(xyz<esc>", "(xy" X "z)" N "def" N "(ghi)");
    KEYS("j.", "(xyz)" N "de" X "f" N "(ghi)");
    KEYS("j.", "(xyz)" N "def" N "(xy" X "z)");

    // quoted string
    data.setText("\"abc" X "\"\"def\"");
    KEYS("di\"", "\"" X "\"\"def\"");
    KEYS("u", "\"" X "abc\"\"def\"");
    KEYS("<c-r>", "\"" X "\"\"def\"");

    /* QTCREATORBUG-12128 */
    data.setText("abc \"def\" ghi \"jkl\" mno");
    KEYS("di\"", "abc \"" X "\" ghi \"jkl\" mno");
    KEYS("u", "abc \"" X "def\" ghi \"jkl\" mno");
    KEYS("3l" "di\"", "abc \"" X "\" ghi \"jkl\" mno");
    KEYS("di\"", "abc \"" X "\" ghi \"jkl\" mno");
    KEYS("tj" "di\"", "abc \"\" ghi \"" X "\" mno");
    KEYS("l" "di\"", "abc \"\" ghi \"\"" X " mno");

    /* QTCREATORBUG-22484: quote text objects are line-based, so an (odd)
       quote on a previous line must not shift the pairing on this line. */
    data.setText("x\"" N "a \"" X "b\" c");
    KEYS("di\"", "x\"" N "a \"" X "\" c");
    KEYS("u", "x\"" N "a \"" X "b\" c");

    NOT_IMPLEMENTED
    // quoted string with escaped character
    data.setText("\"abc\"");
    KEYS("di\"", "\"abc\"\"" X "\"");
    KEYS("u", "\"abc\"\"" X "def\"");
}

void FakeVimTester::test_vim_block_selection_insert()
{
    TestData data;
    setup(&data);

    // insert in visual block mode
    data.setText("abc" N "d" X "ef" N "jkl" N "mno" N "pqr");
    KEYS("<c-v>2j" "2I" "XYZ<esc>", "abc" N "d" X "XYZXYZef" N "jXYZXYZkl" N "mXYZXYZno" N "pqr");
    INTEGRITY(false);

    data.setText("abc" N "d" X "ef" N "jkl" N "mno" N "pqr");
    KEYS("<c-v>2j" "2A" "XYZ<esc>", "abc" N "d" X "eXYZXYZf" N "jkXYZXYZl" N "mnXYZXYZo" N "pqr");
    INTEGRITY(false);

    data.setText("abc" N "de" X "f" N  "" N "jkl" N "mno");
    KEYS("<c-v>2jh" "2I" "XYZ<esc>", "abc" N "d" X "XYZXYZef" N "" N "jXYZXYZkl" N "mno");
    INTEGRITY(false);

    /* QTCREATORBUG-11378 */
    data.setText(
         " abcd" N
         " efgh" N
         " ijkl" N
         " mnop" N
         "");
    KEYS("<C-V>3j$AXYZ<ESC>",
         X " abcdXYZ" N
           " efghXYZ" N
           " ijklXYZ" N
           " mnopXYZ" N
           "");

    data.setText(
         " abcd" N
         " ef" N
         " ghijk" N
         " lm" N
         "");
    KEYS("<C-V>3j$AXYZ<ESC>",
         X " abcdXYZ" N
           " efXYZ" N
           " ghijkXYZ" N
           " lmXYZ" N
           "");

    data.setText(
        "a" N
        "" N
        "b" N
        "" N
    );
    KEYS("j<C-V>$jAXYZ<ESC>",
        "a" N
        "|XYZ" N
        "bXYZ" N
        "" N
    );

    data.setText(
        "abc" N
        "" N
        "def" N
    );
    KEYS("l<c-v>2jAXYZ<ESC>",
        "a" X "bXYZc" N
        "  XYZ" N
        "deXYZf" N
         );

    // Block-inserting whitespace to indent several lines must keep the
    // inserted indentation on every line, i.e. it must not be treated as
    // strippable auto-indentation (QTCREATORBUG-24094).
    data.setText("abc" N "def" N "ghi");
    KEYS("<c-v>2jI  <esc>", X "  abc" N "  def" N "  ghi");

    // also when the lines are already indented
    data.setText("  abc" N "  def" N "  ghi");
    KEYS("<c-v>2jI  <esc>", X "    abc" N "    def" N "    ghi");

    // The same must hold for a tab. With 'noexpandtab' a real tab is
    // inserted on every line ...
    data.doCommand("set noexpandtab");
    data.setText("abc" N "def" N "ghi");
    KEYS("<c-v>2jI<tab><esc>", X "\tabc" N "\tdef" N "\tghi");

    // ... a literal tab character in the input stream behaves the same ...
    data.setText("abc" N "def" N "ghi");
    KEYS("<c-v>2jI\t<esc>", X "\tabc" N "\tdef" N "\tghi");

    // ... and with 'expandtab' the equivalent spaces are used on every line.
    data.doCommand("set expandtab");
    data.doCommand("set tabstop=4");
    data.setText("abc" N "def" N "ghi");
    KEYS("<c-v>2jI<tab><esc>", X "    abc" N "    def" N "    ghi");
    data.doCommand("set noexpandtab");
    data.doCommand("set tabstop=8");
}

void FakeVimTester::test_vim_delete_inner_paragraph()
{
    TestData data;
    setup(&data);

    data.setText(
        "abc" N
        "def" N
        "" N
        "" N
        "ghi" N
        "" N
        "jkl" N
    );

    KEYS("dip",
        X "" N
        "" N
        "ghi" N
        "" N
        "jkl" N
    );
    KEYS("dip",
        X "ghi" N
        "" N
        "jkl" N
    );
    KEYS("2dip",
        X "jkl" N
    );
}

void FakeVimTester::test_vim_delete_a_paragraph()
{
    TestData data;
    setup(&data);

    data.setText(
        "abc" N
        "def" N
        "" N
        "" N
        "ghi" N
        "" N
        "jkl" N
    );

    KEYS("dap",
        X "ghi" N
        "" N
        "jkl" N
    );
    KEYS("dap",
        X "jkl" N
    );
    KEYS("u",
        X "ghi" N
        "" N
        "jkl" N
    );

    data.setText(
        "abc" N
        "" N
        "" N
        "def"
    );
    KEYS("Gdap",
        X "abc"
    );
}

void FakeVimTester::test_vim_change_inner_paragraph()
{
    TestData data;
    setup(&data);

    data.setText(
        "abc" N
        "def" N
        "" N
        "" N
        "ghi" N
        "" N
        "jkl" N
    );

    KEYS("cipXXX<ESC>",
        "XX" X "X" N
        "" N
        "" N
        "ghi" N
        "" N
        "jkl" N
    );
    KEYS("3j" "cipYYY<ESC>",
        "XXX" N
        "" N
        "" N
        "YY" X "Y" N
        "" N
        "jkl" N
    );
}

void FakeVimTester::test_vim_change_a_paragraph()
{
    TestData data;
    setup(&data);

    data.setText(
        "abc" N
        "def" N
        "" N
        "" N
        "ghi" N
        "" N
        "jkl" N
    );

    KEYS("4j" "capXXX<ESC>",
        "abc" N
        "def" N
        "" N
        "" N
        "XX" X "X" N
        "jkl" N
    );
    KEYS("gg" "capYYY<ESC>",
        "YY" X "Y" N
        "XXX" N
        "jkl" N
    );

    data.setText(
        "abc" N
        "" N
        "" N
        "def"
    );
    KEYS("GcapXXX<ESC>",
        "abc" N
        "XX" X "X"
         );
}

void FakeVimTester::test_vim_select_inner_paragraph()
{
    TestData data;
    setup(&data);

    data.setText(
        "" N
        X "abc" N
        "def" N
        "" N
        "ghi"
    );
    KEYS("vip" "r-",
        "" N
        X "---" N
        "---" N
        "" N
        "ghi"
    );

    data.setText(
        "" N
        X "abc" N
        "def" N
        "" N
        "ghi"
    );
    KEYS("vip" ":s/^/-<CR>",
        "" N
        "-abc" N
        X "-def" N
        "" N
        "ghi"
    );

    data.setText(
        "" N
        X "abc" N
        "def" N
        "" N
        "ghi"
    );
    KEYS("v2ip" ":s/^/-<CR>",
        "" N
        "-abc" N
        "-def" N
        X "-" N
        "ghi"
    );

    data.setText(
        "" N
        X "abc" N
        "def" N
        "" N
        "ghi"
    );
    KEYS("Vj" "ip" ":s/^/-<CR>",
        "" N
        "-abc" N
        "-def" N
        X "-" N
        "ghi"
    );

    data.setText(
        "" N
        X "abc" N
        "def" N
        "" N
        "ghi"
    );
    KEYS("vj" "ip" ":s/^/-<CR>",
        "" N
        "-abc" N
        "-def" N
        "-" N
        "ghi"
    );

    data.setText(
        "" N
        X "abc" N
        "def" N
        "ghi" N
        "" N
        "jkl"
    );
    KEYS("vj" "ip" ":s/^/-<CR>",
        "" N
        "-abc" N
        "-def" N
        "-ghi" N
        "" N
        "jkl"
    );

    data.setText(
        "" N
        X "abc" N
        "def" N
        "" N
        "ghi"
    );
    KEYS("vip" "r-",
        "" N
        X "---" N
        "---" N
        "" N
        "ghi"
    );

    data.setText(
        "abc" N
        "" N
        "def"
    );
    KEYS("G" "vip" "r-",
        "abc" N
        "" N
        "---"
    );

    data.setText(
        "" N
        "" N
        "ghi"
    );
    KEYS("vip" ":s/^/-<CR>",
        "-" N
        "-" N
        "ghi"
    );

    data.setText(
        "" N
        "ghi"
    );
    KEYS("vip" "ip" ":s/^/-<CR>",
        "-" N
        X "-ghi"
    );

    data.setText(
        "abc" N
        "" N
        ""
    );
    KEYS("j" "vip" ":s/^/-<CR>",
        "abc" N
        "-" N
        "-"
    );

    // Don't move anchor if it's on different line.
    data.setText(
        "" N
        "abc" N
        X "def" N
        "ghi" N
        "" N
        "jkl"
    );
    KEYS("vj" "ip" ":s/^/-<CR>",
        "" N
        "abc" N
        "-def" N
        "-ghi" N
        X "-" N
        "jkl"
    );

    // Don't change selection mode if anchor is on different line.
    data.setText(
        "" N
        "abc" N
        X "def" N
        "ghi" N
        "" N
        "jkl"
    );
    KEYS("vj" "2ip" "r-",
        "" N
        "abc" N
        X "---" N
        "---" N
        "" N
        "-kl"
    );
    KEYS("gv" ":s/^/X<CR>",
        "" N
        "abc" N
        "X---" N
        "X---" N
        "X" N
        X "X-kl"
    );

    data.setText(
        "" N
        "abc" N
        X "def" N
        "ghi" N
        "" N
        "jkl"
    );
    KEYS("<C-V>j" "2ip" "r-",
        "" N
        "abc" N
        X "-ef" N
        "-hi" N
        "" N
        "-kl"
    );
    KEYS("gv" "IX<ESC>",
        "" N
        "abc" N
        "X-ef" N
        "X-hi" N
        "X" N
        "X-kl"
    );
}

void FakeVimTester::test_vim_select_a_paragraph()
{
    TestData data;
    setup(&data);

    data.setText(
        "abc" N
        "def" N
        "" N
        "ghi"
    );
    KEYS("vap" ":s/^/-<CR>",
        "-abc" N
        "-def" N
        "-" N
        "ghi"
    );

    data.setText(
        "" N
        "abc" N
        "def" N
        "" N
        "ghi"
    );
    KEYS("vap" ":s/^/-<CR>",
        "-" N
        "-abc" N
        "-def" N
        "" N
        "ghi"
    );

    data.setText(
        "abc" N
        "def" N
        ""
    );
    KEYS("j" "vap" ":s/^/-<CR>",
        "-abc" N
        "-def" N
        "-"
    );

    data.setText(
        "" N
        "abc" N
        "def"
    );
    KEYS("j" "vap" ":s/^/-<CR>",
        "-" N
        "-abc" N
        "-def"
    );
}

void FakeVimTester::test_vim_repeat()
{
    TestData data;
    setup(&data);

    // delete line
    data.setText("abc" N "def" N "ghi");
    KEYS("dd", X "def" N "ghi");
    KEYS(".", X "ghi");
    INTEGRITY(false);

    // delete to next word
    data.setText("abc def ghi jkl");
    KEYS("dw", X "def ghi jkl");
    KEYS("w.", "def " X "jkl");
    KEYS("gg.", X "jkl");

    // change in word
    data.setText("WORD text");
    KEYS("ciwWORD<esc>", "WOR" X "D text");
    KEYS("w.", "WORD WOR" X "D");

    /* QTCREATORBUG-7248 */
    data.setText("test tex" X "t");
    KEYS("vbcWORD<esc>", "test " "WOR" X "D");
    KEYS("bb.", "WOR" X "D WORD");

    // delete selected range
    data.setText("abc def ghi jkl");
    KEYS("viwd", X " def ghi jkl");
    KEYS(".", X "f ghi jkl");
    KEYS(".", X "hi jkl");

    // delete two lines
    data.setText("abc" N "def" N "ghi" N "jkl" N "mno");
    KEYS("Vjx", X "ghi" N "jkl" N "mno");
    KEYS(".", X "mno");

    // delete three lines
    data.setText("abc" N "def" N "ghi" N "jkl" N "mno" N "pqr" N "stu");
    KEYS("d2j", X "jkl" N "mno" N "pqr" N "stu");
    KEYS(".", X "stu");

    // replace block selection
    data.setText("abcd" N "d" X "efg" N "ghij" N "jklm");
    KEYS("<c-v>jlrX", "abcd" N "d" X "XXg" N "gXXj" N "jklm");
    KEYS("gg.", "XXcd" N "XXXg" N "gXXj" N "jklm");
}

void FakeVimTester::test_vim_search()
{
    TestData data;
    setup(&data);

    data.setText("abc" N "def" N "ghi");
    KEYS("/ghi<CR>", "abc" N "def" N X "ghi");
    KEYS("gg/\\w\\{3}<CR>", "abc" N X "def" N "ghi");
    KEYS("n", "abc" N "def" N X "ghi");
    KEYS("N", "abc" N X "def" N "ghi");
    KEYS("N", X "abc" N "def" N "ghi");

    // Operator with a search motion, e.g. d/ (QTCREATORBUG-24172). This must
    // also work with the core search dialog enabled, which cannot carry out
    // the pending operator, so the operator uses the built-in search instead.
    data.setText("|abc def ghi");
    KEYS("d/ghi<CR>", "|ghi");
    {
        auto &useCoreSearch = FakeVim::Internal::settings().useCoreSearch;
        const bool saved = useCoreSearch.value();
        useCoreSearch.setValue(true);
        data.setText("|abc def ghi");
        KEYS("d/ghi<CR>", "|ghi");
        useCoreSearch.setValue(saved);
    }
    data.setText("abc" N "def" N "ghi");

    // return to search-start position on escape or not found
    KEYS("/def<ESC>", X "abc" N "def" N "ghi");
    KEYS("/x", X "abc" N "def" N "ghi");
    KEYS("/x<CR>", X "abc" N "def" N "ghi");
    KEYS("/x<ESC>", X "abc" N "def" N "ghi");
    KEYS("/ghX", X "abc" N "def" N "ghi");

    KEYS("?def<ESC>", X "abc" N "def" N "ghi");
    KEYS("?x", X "abc" N "def" N "ghi");
    KEYS("?x<CR>", X "abc" N "def" N "ghi");
    KEYS("?x<ESC>", X "abc" N "def" N "ghi");

    // set wrapscan (search wraps at end of file)
    data.doCommand("set ws");

    // search [count] times
    data.setText("abc" N "def" N "ghi");
    KEYS("/\\w\\{3}<CR>", "abc" N X "def" N "ghi");
    KEYS("2n", X "abc" N "def" N "ghi");
    KEYS("2N", "abc" N X "def" N "ghi");
    KEYS("2/\\w\\{3}<CR>", X "abc" N "def" N "ghi");

    data.setText("abc" N "def" N "abc" N "ghi abc jkl");
    KEYS("*", "abc" N "def" N X "abc" N "ghi abc jkl");
    KEYS("*", "abc" N "def" N "abc" N "ghi " X "abc jkl");
    KEYS("2*", "abc" N "def" N X "abc" N "ghi abc jkl");
    KEYS("#", X "abc" N "def" N "abc" N "ghi abc jkl");
    KEYS("#", "abc" N "def" N "abc" N "ghi " X "abc jkl");
    KEYS("#", "abc" N "def" N X "abc" N "ghi abc jkl");
    KEYS("2#", "abc" N "def" N "abc" N "ghi " X "abc jkl");

    data.doCommand("set nows");
    data.setText("abc" N "def" N "abc" N "ghi abc jkl");
    KEYS("*", "abc" N "def" N X "abc" N "ghi abc jkl");
    KEYS("*", "abc" N "def" N "abc" N "ghi " X "abc jkl");
    KEYS("*", "abc" N "def" N "abc" N "ghi " X "abc jkl");
    KEYS("#", "abc" N "def" N X "abc" N "ghi abc jkl");
    KEYS("#", X "abc" N "def" N "abc" N "ghi abc jkl");
    KEYS("#", X "abc" N "def" N "abc" N "ghi abc jkl");

    data.setText("abc" N "def" N "ab" X "c" N "ghi abc jkl");
    KEYS("#", X "abc" N "def" N "abc" N "ghi abc jkl");

    // search with g* and g#
    data.doCommand("set nows");
    data.setText("bc" N "abc" N "abcd" N "bc" N "b");
    KEYS("g*", "bc" N "a" X "bc" N "abcd" N "bc" N "b");
    KEYS("n", "bc" N "abc" N "a" X "bcd" N "bc" N "b");
    KEYS("n", "bc" N "abc" N "abcd" N X "bc" N "b");
    KEYS("n", "bc" N "abc" N "abcd" N X "bc" N "b");
    KEYS("g#", "bc" N "abc" N "a" X "bcd" N "bc" N "b");
    KEYS("n", "bc" N "a" X "bc" N "abcd" N "bc" N "b");
    KEYS("N", "bc" N "abc" N "a" X "bcd" N "bc" N "b");
    KEYS("3n", "bc" N "abc" N "a" X "bcd" N "bc" N "b");
    KEYS("2n", X "bc" N "abc" N "abcd" N "bc" N "b");

    /* QTCREATORBUG-7251 */
    data.setText("abc abc abc abc");
    KEYS("$?abc<CR>", "abc abc abc " X "abc");
    KEYS("2?abc<CR>", "abc " X "abc abc abc");
    KEYS("n", X "abc abc abc abc");
    KEYS("N", "abc " X "abc abc abc");

    // search is greedy
    data.doCommand("set ws");
    data.setText("abc" N "def" N "abc" N "ghi abc jkl");
    KEYS("/[a-z]*<CR>", "abc" N X "def" N "abc" N "ghi abc jkl");
    KEYS("2n", "abc" N "def" N "abc" N X "ghi abc jkl");
    KEYS("3n", "abc" N "def" N "abc" N "ghi abc" X " jkl");
    KEYS("3N", "abc" N "def" N "abc" N X "ghi abc jkl");
    KEYS("2N", "abc" N X "def" N "abc" N "ghi abc jkl");

    data.setText("a.b.c" N "" N "d.e.f");
    KEYS("/[a-z]*<CR>", "a" X ".b.c" N "" N "d.e.f");
    KEYS("n", "a." X "b.c" N "" N "d.e.f");
    KEYS("2n", "a.b." X "c" N "" N "d.e.f");
    KEYS("n", "a.b.c" N X "" N "d.e.f");
    KEYS("n", "a.b.c" N "" N X "d.e.f");
    KEYS("2N", "a.b." X "c" N "" N "d.e.f");
    KEYS("2n", "a.b.c" N "" N X "d.e.f");

    // find same stuff forward and backward,
    // i.e. '<ab>c' forward but not 'a<bc>' backward
    data.setText("abc" N "def" N "ghi");
    KEYS("/\\w\\{2}<CR>", "abc" N X "def" N "ghi");
    KEYS("n", "abc" N "def" N X "ghi");
    KEYS("N", "abc" N X "def" N "ghi");
    KEYS("N", X "abc" N "def" N "ghi");
    KEYS("2n2N", X "abc" N "def" N "ghi");

    // delete to match
    data.setText("abc" N "def" N "abc" N "ghi abc jkl" N "xyz");
    KEYS("2l" "d/ghi<CR>", "ab" X "ghi abc jkl" N "xyz");

    data.setText("abc" N "def" N "abc" N "ghi abc jkl" N "xyz");
    KEYS("l" "d2/abc<CR>", "a" X "abc jkl" N "xyz");

    data.setText("abc" N "def" N "abc" N "ghi abc jkl" N "xyz");
    KEYS("d/abc<CR>", X "abc" N "ghi abc jkl" N "xyz");
    KEYS(".", "abc jkl" N "xyz");

    data.setText("abc" N "def" N "abc" N "ghi abc jkl" N "xyz");
    KEYS("/abc<CR>" "l" "dn", "abc" N "def" N "a" X "abc jkl" N "xyz");

    data.setText("abc" N "def" N "abc" N "ghi abc jkl" N "xyz");
    KEYS("2/abc<CR>" "h" "dN", "abc" N "def" N X " abc jkl" N "xyz");
    KEYS("c/xxx<CR><ESC>" "h" "dN", "abc" N "def" N X " abc jkl" N "xyz");

    data.setText("abc" N "def" N "abc" N "ghi abc jkl" N "xyz");
    KEYS("l" "v2/abc<CR>" "x", "abc jkl" N "xyz");

    // don't leave visual mode after search failed or is cancelled
    data.setText("abc" N "def" N "abc" N "ghi abc jkl" N "xyz");
    KEYS("vj" "/abc<ESC>" "x", X "ef" N "abc" N "ghi abc jkl" N "xyz");
    KEYS("vj" "/xxx<CR>" "x", X "bc" N "ghi abc jkl" N "xyz");

    // insert word under cursor (C-R C-W)
    data.setText("abc def ghi def.");
    KEYS("fe/<C-R><C-W><CR>", "abc def ghi " X "def.");
    // insert register (C-R{register})
    data.setText("abc def ghi def.");
    KEYS("feyiw/<C-R>0<CR>", "abc def ghi " X "def.");
    // insert non-existing register
    data.setText("abc def ghi def.");
    KEYS("feyiw/<C-R>adef<CR>", "abc def ghi " X "def.");
    // abort C-R via Esc
    data.doCommand("set noincsearch");
    data.setText("abc def ghi def.");
    KEYS("fe/d<C-R><ESC>ef<CR>", "abc def ghi " X "def.");
}

void FakeVimTester::test_vim_nohlsearch_core_search()
{
    TestData data;
    setup(&data);

    // With "Use Qt Creator's find" enabled the matches are highlighted by the
    // find tool, not by FakeVim, so :nohlsearch clears them by hiding the find
    // tool bar (via the findHideRequested() callback), which the plugin routes
    // to Find::hideFindToolBar() (QTCREATORBUG-22298).
    data.doCommand("set ucs");

    int hideCount = 0;
    data.handler->findHideRequested.set([&] { ++hideCount; });

    data.setText("abc abc abc");
    data.doCommand("nohlsearch");

    QVERIFY(hideCount > 0);

    // Restore the global setting so later tests use the default search path.
    data.doCommand("set noucs");
}

void FakeVimTester::test_vim_indent()
{
    TestData data;
    setup(&data);

    data.doCommand("set expandtab");
    data.doCommand("set shiftwidth=4");

    data.setText(
        "abc" N
        "def" N
        "ghi" N
        "jkl" N
        "mno");
    KEYS("j3>>",
        "abc" N
        "    " X "def" N
        "    ghi" N
        "    jkl" N
        "mno");
    KEYS("j2>>",
        "abc" N
        "    def" N
        "        " X "ghi" N
        "        jkl" N
        "mno");

    KEYS("2<<",
        "abc" N
        "    def" N
        "    " X "ghi" N
        "    jkl" N
        "mno");
    INTEGRITY(false);
    KEYS("k3<<",
        "abc" N
        X "def" N
        "ghi" N
        "jkl" N
        "mno");

    data.setText(
        "abc" N
        "def" N
        "ghi" N
        "jkl" N
        "mno");
    KEYS("jj>j",
        "abc" N
        "def" N
        "    " X "ghi" N
        "    jkl" N
        "mno");

    data.setText("abc");
    KEYS(">>", "    " X "abc");
    INTEGRITY(false);

    data.setText("abc");
    data.doCommand("set shiftwidth=2");
    KEYS(">>", "  " X "abc");

    data.setText("abc");
    data.doCommand("set noexpandtab");
    data.doCommand("set tabstop=2");
    data.doCommand("set shiftwidth=7");
    // shiftwidth = TABS * tabstop + SPACES
    //          7 = 3    * 2       + 1
    KEYS(">>", "\t\t\t abc");

    data.doCommand("set tabstop=3");
    data.doCommand("set shiftwidth=7");
    data.setText("abc");
    KEYS(">>", "\t\t abc");
    INTEGRITY(false);

    // indent inner block
    data.doCommand("set expandtab");
    data.doCommand("set shiftwidth=2");
    data.setText("int main()" N
         "{" N
         "int i = 0;" N
         X "return i;" N
         "}" N
         "");
    KEYS(">i{",
         "int main()" N
         "{" N
         "  " X "int i = 0;" N
         "  return i;" N
         "}" N
         "");
    KEYS(">i}",
         "int main()" N
         "{" N
         "    " X "int i = 0;" N
         "    return i;" N
         "}" N
         "");
    KEYS("<i}",
         "int main()" N
         "{" N
         "  " X "int i = 0;" N
         "  return i;" N
         "}" N
         "");

    data.doCommand("set expandtab");
    data.doCommand("set shiftwidth=2");
    data.setText("int main() {" N
         "return i;" N
         X "}" N
         "");
    KEYS("l>i{",
         "int main() {" N
         "  " X "return i;" N
         "}" N
         "");
    KEYS("l>i}",
         "int main() {" N
         "    " X "return i;" N
         "}" N
         "");
    KEYS("l<i}",
         "int main() {" N
         "  " X "return i;" N
         "}" N
         "");
}

void FakeVimTester::test_vim_marks()
{
    TestData data;
    setup(&data);

    data.setText("  abc" N "  def" N "  ghi");
    data.doKeys("ma");
    data.doKeys("ma");
    data.doKeys("jmb");
    data.doKeys("j^mc");
    KEYS("'a",   "  " X "abc" N   "  "   "def" N   "  "   "ghi");
    KEYS("`a", X "  "   "abc" N   "  "   "def" N   "  "   "ghi");
    KEYS("`b",   "  "   "abc" N X "  "   "def" N   "  "   "ghi");
    KEYS("'b",   "  "   "abc" N   "  " X "def" N   "  "   "ghi");
    KEYS("`c",   "  "   "abc" N   "  "   "def" N   "  " X "ghi");
    KEYS("'c",   "  "   "abc" N   "  "   "def" N   "  " X "ghi");

    KEYS("`b",   "  "   "abc" N X "  "   "def" N   "  "   "ghi");
    KEYS("'c",   "  "   "abc" N   "  "   "def" N   "  " X "ghi");

    KEYS("`'",   "  "   "abc" N X "  "   "def" N   "  "   "ghi");
    KEYS("`a", X "  "   "abc" N   "  "   "def" N   "  "   "ghi");
    KEYS("''",   "  "   "abc" N   "  " X "def" N   "  "   "ghi");
    KEYS("`'", X "  "   "abc" N   "  "   "def" N   "  "   "ghi");
    KEYS("`'",   "  "   "abc" N   "  " X "def" N   "  "   "ghi");

    // new mark isn't lost on undo
    data.setText(       "abc" N "d" X "ef" N "ghi");
    KEYS("x" "mx" "gg", X "abc" N "df" N "ghi");
    KEYS("ugg" "`x",    "abc" N "d" X "ef" N "ghi");

    // previous value of mark is restored on undo/redo
    data.setText(        "abc" N "d" X "ef" N "ghi");
    KEYS("mx" "x" "ggl", "a" X "bc" N "df" N "ghi");
    KEYS("mx" "uG" "`x", "abc" N "d" X "ef" N "ghi");
    KEYS("<c-r>G" "`x",  "a" X "bc" N "df" N "ghi");
    KEYS("uG" "`x",      "abc" N "d" X "ef" N "ghi");
    KEYS("<c-r>G" "`x",  "a" X "bc" N "df" N "ghi");

    // :delmarks deletes marks, so afterwards jumping to them is a no-op
    // (QTCREATORBUG-21820).
    data.setText(X "  abc" N "  def" N "  ghi");
    data.doKeys("ma");          // mark 'a' at line 1
    data.doKeys("jjmb");        // mark 'b' at line 3, cursor now on line 3
    KEYS("`a", X "  abc" N "  def" N "  ghi");            // jump to 'a' works
    KEYS("`b", "  abc" N "  def" N X "  ghi");            // jump to 'b' works
    COMMAND("delmarks b", "  abc" N "  def" N X "  ghi"); // delete 'b'
    KEYS("`b", "  abc" N "  def" N X "  ghi");            // 'b' no longer moves
    KEYS("`a", X "  abc" N "  def" N "  ghi");            // 'a' still works
    COMMAND("delmarks!", X "  abc" N "  def" N "  ghi");  // delete all lowercase
    KEYS("`a", X "  abc" N "  def" N "  ghi");            // 'a' gone too
}

void FakeVimTester::test_vim_jumps()
{
    TestData data;
    setup(&data);

    // last position
    data.setText("  abc" N "  def" N "  ghi");
    KEYS("G", "  abc" N "  def" N "  " X "ghi");
    KEYS("`'", X "  abc" N "  def" N "  ghi");
    KEYS("`'", "  abc" N "  def" N "  " X "ghi");
    KEYS("''", "  " X "abc" N "  def" N "  ghi");
    KEYS("<C-O>", "  abc" N "  def" N "  " X "ghi");
    KEYS("<C-I>", "  " X "abc" N "  def" N "  ghi");

    KEYS("lgUlhj", "  aBc" N "  " X "def" N "  ghi");
    KEYS("`.", "  a" X "Bc" N "  def" N "  ghi");
    KEYS("`'", "  aBc" N "  " X "def" N "  ghi");
    KEYS("'.", "  " X "aBc" N "  def" N "  ghi");
    KEYS("G", "  aBc" N "  def" N "  " X "ghi");
    KEYS("u", "  a" X "bc" N "  def" N "  ghi");
    KEYS("`'", "  abc" N "  def" N "  " X "ghi");
    KEYS("<c-r>", "  a" X "Bc" N "  def" N "  ghi");
    KEYS("jd$", "  aBc" N "  " X "d" N "  ghi");
    KEYS("''", "  aBc" N "  d" N "  " X "ghi");
    KEYS("`'", "  aBc" N "  " X "d" N "  ghi");
    KEYS("u", "  aBc" N "  d" X "ef" N "  ghi");
    KEYS("''", "  aBc" N "  " X "def" N "  ghi");
    KEYS("`'", "  aBc" N "  d" X "ef" N "  ghi");

    // record external position changes
    data.setText("abc" N "def" N "g" X "hi");
    data.jump("abc" N "de" X "f" N "ghi");
    KEYS("<C-O>", "abc" N "def" N "g" X "hi");
    KEYS("<C-I>", "abc" N "de" X "f" N "ghi");
    data.jump("ab" X "c" N "def" N "ghi");
    KEYS("<C-O>", "abc" N "de" X "f" N "ghi");
    KEYS("<C-O>", "abc" N "def" N "g" X "hi");
}

void FakeVimTester::test_vim_current_column()
{
    // Check if column is correct after command and vertical cursor movement.
    TestData data;
    setup(&data);

    // always at end of line after <end>
    data.setText("  abc" N "  def 123" N "" N "  ghi");
    KEYS("<end><down>", "  abc" N "  def 12" X "3" N "" N "  ghi");
    KEYS("<down><down>", "  abc" N "  def 123" N "" N "  gh" X "i");
    KEYS("<up>", "  abc" N "  def 123" N X "" N "  ghi");
    KEYS("<up>", "  abc" N "  def 12" X "3" N "" N "  ghi");
    // ... in insert
    KEYS("i<end><up>", "  abc" X N "  def 123" N "" N "  ghi");
    data.doKeys("<ESC>");
    KEYS("<down>i<end><up><down>", "  abc" N "  def 123" X N "" N "  ghi");

    // vertical movement doesn't reset column
    data.setText("  abc" N "  def 1" X "23" N "" N "  ghi");
    KEYS("<up>", "  ab" X "c" N "  def 123" N "" N "  ghi");
    KEYS("<down>", "  abc" N "  def 1" X "23" N "" N "  ghi");
    KEYS("<down><down>", "  abc" N "  def 123" N "" N "  gh" X "i");
    KEYS("<up><up>", "  abc" N "  def 1" X "23" N "" N "  ghi");
    KEYS("^jj", "  abc" N "  def 123" N "" N "  " X "ghi");
    KEYS("kk", "  abc" N "  " X "def 123" N "" N "  ghi");

    // yiw, yaw
    data.setText("  abc" N "  def" N "  ghi");
    KEYS("e<down>", "  abc" N "  de" X "f" N "  ghi");
    KEYS("b<down>", "  abc" N "  def" N "  " X "ghi");
    KEYS("ll<up>", "  abc" N "  de" X "f" N "  ghi");
    KEYS("<down>yiw<up>", "  abc" N "  " X "def" N "  ghi");
    KEYS("llyaw<up>", "  " X "abc" N "  def" N "  ghi");

    // insert
    data.setText("  abc" N "  def" N "  ghi");
    KEYS("lljj", "  abc" N "  def" N "  " X "ghi");
    KEYS("i123<up>", "  abc" N "  def" X N "  123ghi");
    data.doKeys("<ESC>");
    KEYS("a456<up><down>", "  abc" N "  def456" X N "  123ghi");

    data.setText("  abc" N X "  def 123" N "" N "  ghi");
    KEYS("A<down><down>", "  abc" N "  def 123" N "" N "  ghi" X);
    data.doKeys("<ESC>");
    KEYS("A<up><up>", "  abc" N "  def" X " 123" N "" N "  ghi");
    data.doKeys("<ESC>");
    KEYS("A<down><down><up><up>", "  abc" N "  def 123" X N "" N "  ghi");

    data.setText("  abc" N X "  def 123" N "" N "  ghi");
    KEYS("I<down><down>", "  abc" N "  def 123" N "" N "  " X "ghi");

    // change
    data.setText("  abc" N "  d" X "ef" N "  ghi");
    KEYS("cc<up>", "  " X "abc" N "  " N "  ghi");
    data.setText("  abc" N "  d" X "ef" N "  ghi");
    KEYS("cc<up>x<down><down>", "  xabc" N "  " N "  g" X "hi");
}

void FakeVimTester::test_vim_copy_paste()
{
    TestData data;
    setup(&data);

    data.setText("123" N "456");
    KEYS("llyy2P", X "123" N "123" N "123" N "456");

    data.setText("123" N "456");
    KEYS("yyp", "123" N X "123" N "456");
    KEYS("2p", "123" N "123" N X "123" N "123" N "456");
    INTEGRITY(false);

    data.setText("123 456");
    KEYS("yw2P", "123 123" X " 123 456");
    KEYS("2p", "123 123 123 123" X " 123 456");

    data.setText("123" N "456");
    KEYS("2yyp", "123" N X "123" N "456" N "456");

    data.setText("123" N "456");
    KEYS("2yyP", X "123" N "456" N "123" N "456");

    data.setText("123" N "456" N "789");
    KEYS("ddp", "456" N X "123" N "789");

    // block-select middle column, copy and paste twice
    data.setText("123" N "456");
    KEYS("l<C-v>j\"xy2\"xp", "12" X "223" N "45556");

    data.setText("123" N "456" N "789");
    KEYS("wyiw" "wviwp", "123" N "456" N "45" X "6");

    // QTCREATORBUG-8148
    data.setText("abc");
    KEYS("yyp", "abc" N X "abc");
    KEYS("4p", "abc" N "abc" N X "abc" N "abc" N "abc" N "abc");

    // Yanking a whole line with "V" (visual line mode) is linewise, so "p"
    // pastes it on a new line below, just like "yy" (QTCREATORBUG-22865).
    data.setText("abc" N "def");
    KEYS("Vyp", "abc" N X "abc" N "def");

    // Visual line mode covers the whole logical line regardless of the cursor
    // column, so "Y" started mid-line still yanks the entire line and "d"
    // deletes all of it (QTCREATORBUG-16713).
    data.setText("abc def" N "ghi");
    KEYS("wVYp", "abc def" N X "abc def" N "ghi");
    data.setText("abc def" N "ghi");
    KEYS("wVd", X "ghi");

    // cursor position after yank
    data.setText("ab" X "c" N "def");
    KEYS("Vjy", X "abc" N "def");
    data.setText("ab" X "c" N "def");
    KEYS("<c-v>jhhy", X "abc" N "def");
    data.setText("ab" X "c" N "def");
    KEYS("yj", "ab" X "c" N "def");
    data.setText("abc" N "de" X "f");
    KEYS("yk", "ab" X "c" N "def");
    data.setText("ab" X "c" N "def");
    KEYS("yy", "ab" X "c" N "def");
    KEYS("2yy", "ab" X "c" N "def");

    // copy empty line
    data.setText(X "a" N "" N "b");
    KEYS("Vjy", X "a" N "" N "b");
    KEYS("p", "a" N X "a" N "" N "" N "b");

    // registers
    data.setText(X "abc" N "def" N "ghi");
    KEYS("\"xyy", X "abc" N "def" N "ghi");
    KEYS("\"xp", "abc" N X "abc" N "def" N "ghi");
    KEYS("j\"yyy", "abc" N "abc" N X "def" N "ghi");
    KEYS("gg\"yP", X "def" N "abc" N "abc" N "def" N "ghi");
    KEYS("\"xP", X "abc" N "def" N "abc" N "abc" N "def" N "ghi");

    // QTCREATORBUG-25281
    data.setText(X "abc" N "def" N "ghi");
    KEYS("\"xyy", X "abc" N "def" N "ghi");
    KEYS("\"xp", "abc" N X "abc" N "def" N "ghi");
    KEYS("j", "abc" N "abc" N X "def" N "ghi");
    KEYS("yy", "abc" N "abc" N X "def" N "ghi");
    KEYS("\"xp", "abc" N "abc" N "def" N X "abc" N "ghi");
    KEYS(".", "abc" N "abc" N "def" N "abc" N X "abc" N "ghi");
    KEYS("\"xP", "abc" N "abc" N "def" N "abc" N X "abc" N "abc" N "ghi");
    KEYS(".", "abc" N "abc" N "def" N "abc" N X "abc" N "abc" N "abc" N "ghi");

    // delete to black hole register
    data.setText("aaa bbb ccc");
    KEYS("yiww\"_diwP", "aaa aaa ccc");
    data.setText("aaa bbb ccc");
    KEYS("yiwwdiwP", "aaa bbb ccc");

    // yank register is only used for y{motion} commands
    data.setText("aaa bbb ccc");
    KEYS("yiwwdiw\"0P", "aaa aaa ccc");

    // paste register in insert mode
    data.setText("aaa bbb ccc ");
    KEYS("yiwA<C-r>0", "aaa bbb ccc aaa");
    KEYS("<C-r><Esc>x", "aaa bbb ccc aaax");
    KEYS("<Esc>dd", "");
    data.setText("aaa bbb");
    KEYS("\"ayawA<C-r>a", "aaa bbbaaa ");

    // Replacing a charwise visual selection with a yanked word via "p"/"P"
    // must replace the whole selection, not drop its last character
    // (QTCREATORBUG-26748). The cursor ends on the last pasted character.
    data.setText("abc def");
    KEYS("yiw" "w" "ve" "p", "abc ab" X "c");
    data.setText("abc def");
    KEYS("yiw" "w" "ve" "P", "abc ab" X "c");
    data.setText("abc def");
    KEYS("yw" "w" "ve" "P", "abc abc" X " ");
    // also for a word in the middle of the line
    data.setText("abc def ghi");
    KEYS("yiw" "w" "ve" "p", "abc ab" X "c ghi");
}

void FakeVimTester::test_vim_undo_redo()
{
    TestData data;
    setup(&data);

    data.setText("abc def" N "xyz" N "123");
    KEYS("ddu", X "abc def" N "xyz" N "123");
    COMMAND("redo", X "xyz" N "123");
    COMMAND("undo", X "abc def" N "xyz" N "123");
    COMMAND("redo", X "xyz" N "123");
    KEYS("dd", X "123");
    KEYS("3x", X "");
    KEYS("uuu", X "abc def" N "xyz" N "123");
    KEYS("<C-r>", X "xyz" N "123");
    KEYS("2<C-r>", X "");
    KEYS("3u", X "abc def" N "xyz" N "123");

    KEYS("wved", "abc" X " " N "xyz" N "123");
    KEYS("2w", "abc " N "xyz" N X "123");
    KEYS("u", "abc " X "def" N "xyz" N "123");
    KEYS("<C-r>", "abc" X " " N "xyz" N "123");
    KEYS("10ugg", X "abc def" N "xyz" N "123");

    KEYS("A xxx<ESC>", "abc def xx" X "x" N "xyz" N "123");
    KEYS("A yyy<ESC>", "abc def xxx yy" X "y" N "xyz" N "123");
    KEYS("u", "abc def xx" X "x" N "xyz" N "123");
    KEYS("u", "abc de" X "f" N "xyz" N "123");
    KEYS("<C-r>", "abc def" X " xxx" N "xyz" N "123");
    KEYS("<C-r>", "abc def xxx" X " yyy" N "xyz" N "123");

    KEYS("izzz<ESC>", "abc def xxxzz" X "z yyy" N "xyz" N "123");
    KEYS("<C-r>", "abc def xxxzz" X "z yyy" N "xyz" N "123");
    KEYS("u", "abc def xxx" X " yyy" N "xyz" N "123");

    data.setText("abc" N X "def");
    KEYS("oxyz<ESC>", "abc" N "def" N "xy" X "z");
    KEYS("u", "abc" N X "def");

    // undo paste lines
    data.setText("abc" N);
    KEYS("yy2p", "abc" N X "abc" N "abc" N);
    KEYS("yy3p", "abc" N "abc" N X "abc" N "abc" N "abc" N "abc" N);
    KEYS("u", "abc" N X "abc" N "abc" N);
    KEYS("u", X "abc" N);
    KEYS("<C-r>", X "abc" N "abc" N "abc" N);
    KEYS("<C-r>", "abc" N X "abc" N "abc" N "abc" N "abc" N "abc" N);
    KEYS("u", "abc" N X "abc" N "abc" N);
    KEYS("u", X "abc" N);

    // undo paste block
    data.setText("abc" N "def" N "ghi");
    KEYS("<C-v>jyp", "a" X "abc" N "ddef" N "ghi");
    KEYS("2p", "aa" X "aabc" N "ddddef" N "ghi");
    KEYS("3p", "aaa" X "aaaabc" N "dddddddef" N "ghi");
    KEYS("u", "aa" X "aabc" N "ddddef" N "ghi");
    KEYS("u", "a" X "abc" N "ddef" N "ghi");

    // undo indent
    data.doCommand("set expandtab");
    data.doCommand("set shiftwidth=4");
    data.setText("abc" N "def");
    KEYS(">>", "    " X "abc" N "def");
    KEYS(">>", "        " X "abc" N "def");
    KEYS("<<", "    " X "abc" N "def");
    KEYS("<<", X "abc" N "def");
    KEYS("u", "    " X "abc" N "def");
    KEYS("u", "        " X "abc" N "def");
    KEYS("u", "    " X "abc" N "def");
    KEYS("u", X "abc" N "def");
    KEYS("<C-r>", X "    abc" N "def");
    KEYS("<C-r>", "    " X "    abc" N "def");
    KEYS("<C-r>", "    ab" X "c" N "def");
    KEYS("<C-r>", "ab" X "c" N "def");
    KEYS("<C-r>", "ab" X "c" N "def");

    data.setText("abc" N "def");
    KEYS("2>>", "    " X "abc" N "    def");
    KEYS("u", X "abc" N "def");
    KEYS("<c-r>", X "    abc" N "    def");
    KEYS("u", X "abc" N "def");
    KEYS(">j", "    " X "abc" N "    def");
    KEYS("u", X "abc" N "def");
    KEYS("<c-r>", X "    abc" N "    def");

    // undo replace line
    data.setText("abc" N "  def" N "ghi");
    KEYS("jlllSxyz<ESC>", "abc" N "xyz" N "ghi");
    KEYS("u", "abc" N "  " X "def" N "ghi");
}

void FakeVimTester::test_vim_letter_case()
{
    TestData data;
    setup(&data);

    // set command ~ not to behave as g~
    data.doCommand("set notildeop");

    // upper- and lower-case
    data.setText("abc DEF");
    KEYS("~", "A" X "bc DEF");
    INTEGRITY(false);
    KEYS("4~", "ABC d" X "EF");
    INTEGRITY(false);

    data.setText("abc DEF" N "ghi");
    KEYS("l9~", "aBC de" X "f" N "ghi");
    KEYS(".", "aBC de" X "F" N "ghi");
    KEYS("h.", "aBC dE" X "f" N "ghi");

    // set command ~ to behave as g~
    data.doCommand("set tildeop");

    data.setText("abc DEF" N "ghi JKL");
    KEYS("ll~j", "ABC def" N "GHI jkl");

    data.setText("abc DEF");
    KEYS("lv3l~", "a" X "BC dEF");
    KEYS("v4lU", "a" X "BC DEF");
    KEYS("v4$u", "a" X "bc def");
    KEYS("v4$gU", "a" X "BC DEF");
    KEYS("gu$", "a" X "bc def");
    KEYS("lg~~", X "ABC DEF");
    KEYS(".", X "abc def");
    KEYS("gUiw", X "ABC def");

    data.setText("  ab" X "c" N "def");
    KEYS("2gUU", "  " X "ABC" N "DEF");
    KEYS("u", "  " X "abc" N "def");
    KEYS("<c-r>", "  " X "ABC" N "DEF");

    // undo, redo and dot command
    data.setText("  abcde" N "  fgh" N "  ijk");
    KEYS("3l" "<C-V>2l2j" "U", "  a" X "BCDe" N "  fGH" N "  iJK");
    KEYS("u", "  a" X "bcde" N "  fgh" N "  ijk");
    KEYS("<C-R>", "  a" X "BCDe" N "  fGH" N "  iJK");
    KEYS("u", "  a" X "bcde" N "  fgh" N "  ijk");
    KEYS("h.", "  " X "ABCde" N "  FGH" N "  IJK");
    KEYS("u", "  " X "abcde" N "  fgh" N "  ijk");
    KEYS("h.", " " X " ABcde" N "  FGh" N "  IJk");
    KEYS("u", " " X " abcde" N "  fgh" N "  ijk");
    KEYS("j.", "  abcde" N " " X " FGh" N "  IJk");
    KEYS("u", "  abcde" N " " X " fgh" N "  ijk");
}

void FakeVimTester::test_vim_code_autoindent()
{
    TestData data;
    setup(&data);

    data.doCommand("set nopassnewline");
    data.doCommand("set expandtab");
    data.doCommand("set shiftwidth=3");

    data.setText("int main()" N
         X "{" N
         "}" N
         "");
    KEYS("o" "return 0;",
         "int main()" N
         "{" N
         "   return 0;" X N
         "}" N
         "");
    INTEGRITY(false);
    KEYS("O" "int i = 0;",
         "int main()" N
         "{" N
         "   int i = 0;" X N
         "   return 0;" N
         "}" N
         "");
    INTEGRITY(false);
    KEYS("ddO" "int i = 0;" N "int j = 0;",
         "int main()" N
         "{" N
         "   int i = 0;" N
         "   int j = 0;" X N
         "   return 0;" N
         "}" N
         "");
    data.doKeys("<ESC>");
    KEYS("^i" "int x = 1;" N,
         "int main()" N
         "{" N
         "   int i = 0;" N
         "   int x = 1;" N
         "   " X "int j = 0;" N
         "   return 0;" N
         "}" N
         "");
    data.doKeys("<ESC>");
    KEYS("c2k" "if (true) {" N ";" N "}",
         "int main()" N
         "{" N
         "   if (true) {" N
         "      ;" N
         "   }" X N
         "   return 0;" N
         "}" N
         "");
    data.doKeys("<ESC>");
    KEYS("jci{" "return 1;",
         "int main()" N
         "{" N
         "   return 1;" X N
         "}" N
         "");
    data.doKeys("<ESC>");
    KEYS("di{",
         "int main()" N
         "{" N
         X "}" N
         "");
    INTEGRITY(false);

    // autoindent
    data.doCommand("set nosmartindent");
    data.setText("abc" N "def");
    KEYS("3o 123<esc>", "abc" N " 123" N "  123" N "   12" X "3" N "def");
    INTEGRITY(false);

    data.setText("abc" N "def");
    KEYS("3O 123<esc>", " 123" N "  123" N "   12" X "3" N "abc" N "def");
    INTEGRITY(false);
    data.doCommand("set smartindent");

    // Leaving an auto-indented line to which no text was added clears the
    // auto-indentation, matching Vim (QTCREATORBUG-15009).
    data.doCommand("set expandtab");
    data.doCommand("set shiftwidth=4");
    data.setText("    abc");
    KEYS("o<esc>", "    abc" N X "");
    KEYS(".", "    abc" N "" N X "");
    KEYS("u", "    abc" N X "");
    // Pressing <CR> also clears the line just left.
    data.setText("    abc");
    KEYS("o<cr>xyz<esc>", "    abc" N "" N "    xy" X "z");
    // But indentation is kept once something is typed on the line.
    data.setText("    abc");
    KEYS("oX<esc>", "    abc" N "    " X "X");
}

void FakeVimTester::test_vim_code_folding()
{
    TestData data;
    setup(&data);

    data.setText("int main()" N "{" N "    return 0;" N "}" N "");

    // fold/unfold function block
    data.doKeys("zc");
    QCOMPARE(data.lines(), 2);
    data.doKeys("zo");
    QCOMPARE(data.lines(), 5);
    data.doKeys("za");
    QCOMPARE(data.lines(), 2);

    // delete whole block
    KEYS("dd", "");

    // undo/redo
    KEYS("u", "int main()" N "{" N "    return 0;" N "}" N "");
    KEYS("<c-r>", "");

    // change block
    KEYS("uggzo", X "int main()" N "{" N "    return 0;" N "}" N "");
    KEYS("ccvoid f()<esc>", "void f(" X ")" N "{" N "    return 0;" N "}" N "");
    KEYS("uzc.", "void f(" X ")" N "");

    // open/close folds recursively
    data.setText("int main()" N
         "{" N
         "    if (true) {" N
         "        return 0;" N
         "    } else {" N
         "        // comment" N
         "        " X "return 2" N
         "    }" N
         "}" N
         "");
    int lines = data.lines();
    // close else block
    data.doKeys("zc");
    QCOMPARE(data.lines(), lines - 3);
    // close function block
    data.doKeys("zc");
    QCOMPARE(data.lines(), lines - 8);
    // jumping to a line opens all its parent folds
    data.doKeys("6gg");
    QCOMPARE(data.lines(), lines);

    // close recursively
    data.doKeys("zC");
    QCOMPARE(data.lines(), lines - 8);
    data.doKeys("za");
    QCOMPARE(data.lines(), lines - 3);
    data.doKeys("6gg");
    QCOMPARE(data.lines(), lines);
    data.doKeys("zA");
    QCOMPARE(data.lines(), lines - 8);
    data.doKeys("za");
    QCOMPARE(data.lines(), lines - 3);

    // close all folds
    data.doKeys("zM");
    QCOMPARE(data.lines(), lines - 8);
    data.doKeys("zo");
    QCOMPARE(data.lines(), lines - 4);
    data.doKeys("zM");
    QCOMPARE(data.lines(), lines - 8);

    // open all folds
    data.doKeys("zR");
    QCOMPARE(data.lines(), lines);

    // delete folded lined if deleting to the end of the first folding line
    data.doKeys("zMgg");
    //QTRY_COMPARE(data.lines(), lines - 8);
    QCOMPARE(data.lines(), lines - 8);
    KEYS("wwd$", "int main" N "");

    // undo
    KEYS("u", "int main" X "()" N
         "{" N
         "    if (true) {" N
         "        return 0;" N
         "    } else {" N
         "        // comment" N
         "        return 2" N
         "    }" N
         "}" N
         "");

    // Opening folds recursively isn't supported (previous position in fold isn't restored).
}

void FakeVimTester::test_vim_code_completion()
{
    // Test completion by simply bypassing FakeVim and inserting text directly in editor widget.
    TestData data;
    setup(&data);
    data.setText(
        "int test1Var;" N
        "int test2Var;" N
        "int main() {" N
        "    " X ";" N
        "}" N
        "");

    data.doKeys("i" "te");
    data.completeText("st");
    data.doKeys("1");
    data.completeText("Var");
    KEYS(" = 0<ESC>",
        "int test1Var;" N
        "int test2Var;" N
        "int main() {" N
        "    test1Var = " X "0;" N
        "}" N
        "");

    data.doKeys("o" "te");
    data.completeText("st");
    data.doKeys("2");
    data.completeText("Var");
    KEYS(" = 1;<ESC>",
        "int test1Var;" N
        "int test2Var;" N
        "int main() {" N
        "    test1Var = 0;" N
        "    test2Var = 1" X ";" N
        "}" N
        "");
    data.doKeys("<ESC>");

    // repeat text insertion with completion
    KEYS(".",
        "int test1Var;" N
        "int test2Var;" N
        "int main() {" N
        "    test1Var = 0;" N
        "    test2Var = 1;" N
        "    test2Var = 1" X ";" N
        "}" N
        "");
}

void FakeVimTester::test_vim_substitute()
{
    TestData data;
    setup(&data);

    data.setText("abcabc");
    COMMAND("s/abc/123/", X "123abc");
    COMMAND("u", X "abcabc");
    COMMAND("s/abc/123/g", X "123123");
    COMMAND("u", X "abcabc");

    data.setText("abc" N "def");
    COMMAND("%s/^/ -- /", " -- abc" N " " X "-- def");
    COMMAND("u", X "abc" N "def");

    data.setText("  abc" N "  def");
    COMMAND("%s/$/./", "  abc." N "  " X "def.");

    data.setText("abc" N "def");
    COMMAND("%s/.*/(&)", "(abc)" N X "(def)");
    COMMAND("u", X "abc" N "def");
    COMMAND("%s/.*/X/g", "X" N X "X");

    data.setText("abc" N "" N "def");
    COMMAND("%s/^\\|$/--", "--abc" N "--" N X "--def");
    COMMAND("u", X "abc" N "" N "def");
    COMMAND("%s/^\\|$/--/g", "--abc--" N "--" N X "--def--");

    // captures
    data.setText("abc def ghi");
    COMMAND("s/\\w\\+/'&'/g", X "'abc' 'def' 'ghi'");
    COMMAND("u", X "abc def ghi");
    COMMAND("s/\\w\\+/'\\&'/g", X "'&' '&' '&'");
    COMMAND("u", X "abc def ghi");
    COMMAND("s/\\(\\w\\{3}\\)/(\\1)/g", X "(abc) (def) (ghi)");
    COMMAND("u", X "abc def ghi");
    COMMAND("s/\\(\\w\\{3}\\) \\(\\w\\{3\\}\\)/\\2 \\1 \\\\1/g", X "def abc \\1 ghi");

    // case-insensitive
    data.setText("abc ABC abc");
    COMMAND("s/ABC/123/gi", X "123 123 123");

    // replace on a line
    data.setText("abc" N "def" N "ghi");
    COMMAND("2s/^/ + /", "abc" N " " X "+ def" N "ghi");
    COMMAND("1s/^/ * /", " " X "* abc" N " + def" N "ghi");
    COMMAND("$s/^/ - /", " * abc" N " + def" N " " X "- ghi");

    // replace on lines
    data.setText("abc" N "def" N "ghi");
    COMMAND("2,$s/^/ + /", "abc" N " + def" N " " X "+ ghi");
    COMMAND("1,2s/^/ * /", " * abc" N " " X "*  + def" N " + ghi");
    COMMAND("3,3s/^/ - /", " * abc" N " *  + def" N " " X "-  + ghi");
    COMMAND("%s/\\( \\S \\)*//g", "abc" N "def" N X "ghi");

    // last substitution
    data.setText("abc" N "def" N "ghi");
    COMMAND("%s/DEF/+&/i", "abc" N X "+def" N "ghi");
    COMMAND("&&", "abc" N X "++def" N "ghi");
    COMMAND("&", "abc" N X "++def" N "ghi");
    COMMAND("&&", "abc" N X "++def" N "ghi");
    COMMAND("&i", "abc" N X "+++def" N "ghi");
    COMMAND("s", "abc" N X "+++def" N "ghi");
    COMMAND("&&i", "abc" N X "++++def" N "ghi");

    // search for last substitute pattern
    data.setText("abc" N "def" N "ghi");
    COMMAND("%s/def/def", "abc" N X "def" N "ghi");
    KEYS("gg", X "abc" N "def" N "ghi");
    COMMAND("\\&", "abc" N X "def" N "ghi");

    // substitute last selection
    data.setText("abc" N "def" N "ghi" N "jkl");
    KEYS("jVj:s/^/*<CR>", "abc" N "*def" N X "*ghi" N "jkl");
    COMMAND("'<,'>s/^/*", "abc" N "**def" N X "**ghi" N "jkl");
    KEYS("u", "abc" N X "*def" N "*ghi" N "jkl");
    KEYS("gv:s/^/+<CR>", "abc" N "+*def" N X "+*ghi" N "jkl");

    // replace empty string
    data.setText("abc");
    COMMAND("s//--/g", "--a--b--c");

    // remove characters
    data.setText("abc def");
    COMMAND("s/[abde]//g", "c f");
    COMMAND("undo | s/[bcef]//g", "a d");
    COMMAND("undo | s/\\w//g", " ");
    COMMAND("undo | s/f\\|$/-/g", "abc de-");

    // modifiers
    data.setText("abC dEfGh");
    COMMAND("s/b...E/\\u&", "aBC dEfGh");
    COMMAND("undo | s/b...E/\\U&/g", "aBC DEfGh");
    COMMAND("undo | s/C..E/\\l&/g",  "abc dEfGh");
    COMMAND("undo | s/b...E/\\L&/g", "abc defGh");

    COMMAND("undo | s/\\(b...E\\)/\\u\\1/g", "aBC dEfGh");
    COMMAND("undo | s/\\(b...E\\)/\\U\\1/g", "aBC DEfGh");
    COMMAND("undo | s/\\(C..E\\)/\\l\\1/g",  "abc dEfGh");
    COMMAND("undo | s/\\(b...E\\)/\\L\\1/g", "abc defGh");

    // replace 1 backslash with 1 forward slash (separator: /)
    data.setText(R"(abc\def)");
    COMMAND(R"(s/\\/\/)", X "abc/def");

    // replace 1 backslash with X  normal on line (separator: /)
    data.setText(R"(abc\def\ghi)");
    COMMAND(R"(s/\\/X/g)", X "abcXdefXghi");

    // replace 1 backslash with 1 forward slash on line (separator: /)
    data.setText(R"(abc\def\ghi)");
    COMMAND(R"(s/\\/\//g)", X "abc/def/ghi");

    // replace 1 backslash with 1 forward slash
    data.setText(R"(abc\def)");
    COMMAND(R"(s#\\#/)", X "abc/def");

    // replace 1 backslash with 1 forward slash on line
    data.setText(R"(abc\def\ghi)");
    COMMAND(R"(s#\\#/#g)", X "abc/def/ghi");

    // replace 2 backslash with 2 forward slash
    data.setText(R"(abc\\def)");
    COMMAND(R"(s#\\\\#//)", X "abc//def");

    // replace 2 backslash with 2 forward slash on line
    data.setText(R"(abc\\def\\ghi)");
    COMMAND(R"(s#\\\\#//#g)", X "abc//def//ghi");

    // replace 1 backslash with 1 forward slash last char
    data.setText(R"(abc\)");
    COMMAND(R"(s#\\#/)", X "abc/");

    // replace 1 backslash with 1 forward slash first char
    data.setText(R"(\abc)");
    COMMAND(R"(s#\\#/)", X "/abc");

    // replace 1 # with 2 #  on line
    data.setText(R"(abc#def#ghi)");
    COMMAND(R"(s#\##\#\##g)", X "abc##def##ghi");

    // replace 2 # with 4 # on line
    data.setText(R"(abc##def##ghi)");
    COMMAND(R"(s#\#\##\#\#\#\##g)", X "abc####def####ghi");
}

void FakeVimTester::test_vim_ex_commandbuffer_paste()
{
    TestData data;
    setup(&data);

    data.setText("abc def abc def xyz");
    KEYS("fyyiw0:s/<C-R><C-W>/<C-R>0/g<CR>", "xyz def xyz def xyz");
}

void FakeVimTester::test_vim_ex_yank()
{
    TestData data;
    setup(&data);

    data.setText("abc" N "def");
    COMMAND("y x", X "abc" N "def");
    KEYS("\"xp", "abc" N X "abc" N "def");
    COMMAND("u", X "abc" N "def");
    COMMAND("redo", X "abc" N "abc" N "def");

    KEYS("uw", "abc" N X "def");
    COMMAND("1y y", "abc" N X "def");
    KEYS("\"yP", "abc" N X "abc" N "def");
    COMMAND("u", "abc" N X "def");

    COMMAND("-1,$y x", "abc" N X "def");
    KEYS("\"xP", "abc" N X "abc" N "def" N "def");
    COMMAND("u", "abc" N X "def");

    COMMAND("$-1y", "abc" N X "def");
    KEYS("P", "abc" N X "abc" N "def");
    COMMAND("u", "abc" N X "def");

    data.setText("abc" N "def");
    KEYS("\"xy$", X "abc" N "def");
    KEYS("\"xP", "ab" X "cabc" N "def");

    data.setText(
        "abc def" N
        "ghi jkl" N
    );
    KEYS("yiwp",
        "aab" X "cbc def" N
        "ghi jkl" N
    );
    KEYS("u",
        X "abc def" N
        "ghi jkl" N
    );
    KEYS("\"0p",
        "aab" X "cbc def" N
        "ghi jkl" N
    );
    KEYS("\"xyiw",
        X "aabcbc def" N
        "ghi jkl" N
    );
    KEYS("\"0p",
        "aab" X "cabcbc def" N
        "ghi jkl" N
    );
    KEYS("\"xp",
        "aabcaabcb" X "cabcbc def" N
        "ghi jkl" N
    );

    // register " is last yank
    data.setText(
        "abc def" N
        "ghi jkl" N
    );
    KEYS("yiwp\"xyiw\"\"p",
        "aaabcb" X "cabcbc def" N
        "ghi jkl" N
    );

    // uppercase register appends to lowercase
    data.setText(
        "abc" N
        "def" N
        "ghi" N
    );
    KEYS("\"zdd" "\"zp",
        "def" N
        X "abc" N
        "ghi" N
    );
    KEYS("k\"Zyy" "jj\"zp",
        "def" N
        "abc" N
        "ghi" N
        X "abc" N
        "def" N
    );
    KEYS("k\"Zdd" "j\"Zp",
        "def" N
        "abc" N
        "abc" N
        "def" N
        X "abc" N
        "def" N
        "ghi" N
    );
    KEYS("\"zdk" "gg\"zp",
        "def" N
        X "def" N
        "abc" N
        "abc" N
        "abc" N
        "def" N
        "ghi" N
    );
}

void FakeVimTester::test_vim_ex_delete()
{
    TestData data;
    setup(&data);

    data.setText("abc" N X "def" N "ghi" N "jkl");
    COMMAND("d", "abc" N X "ghi" N "jkl");
    COMMAND("1,2d", X "jkl");
    COMMAND("u", X "abc" N "ghi" N "jkl");
    COMMAND("u", "abc" N X "def" N "ghi" N "jkl");
    KEYS("p", "abc" N "def" N X "abc" N "ghi" N "ghi" N "jkl");
    COMMAND("set ws|" "/abc/,/ghi/d|" "set nows", X "ghi" N "jkl");
    COMMAND("u", X "abc" N "def" N "abc" N "ghi" N "ghi" N "jkl");
    COMMAND("2,/abc/d3", "abc" N "def" N X "jkl");
    COMMAND("u", "abc" N "def" N X "abc" N "ghi" N "ghi" N "jkl");
    COMMAND("5,.+1d", "abc" N "def" N "abc" N X "jkl");
}

void FakeVimTester::test_vim_ex_change()
{
    TestData data;
    setup(&data);

    data.setText("abc" N X "def" N "ghi" N "jkl");
    KEYS(":c<CR>xxx<ESC>0", "abc" N X "xxx" N "ghi" N "jkl");
    KEYS(":-1,+1c<CR>XXX<ESC>0", X "XXX" N "jkl");
}

void FakeVimTester::test_vim_ex_shift()
{
    TestData data;
    setup(&data);

    data.doCommand("set expandtab");
    data.doCommand("set shiftwidth=2");

    data.setText("abc" N X "def" N "ghi" N "jkl");
    COMMAND(">", "abc" N "  " X "def" N "ghi" N "jkl");
    COMMAND(">>", "abc" N "      " X "def" N "ghi" N "jkl");
    COMMAND("<", "abc" N "    " X "def" N "ghi" N "jkl");
    COMMAND("<<", "abc" N X "def" N "ghi" N "jkl");
}

void FakeVimTester::test_vim_ex_move()
{
    TestData data;
    setup(&data);

    data.setText("abc" N "def" N "ghi" N "jkl");
    COMMAND("m +1", "def" N X "abc" N "ghi" N "jkl");
    COMMAND("u", X "abc" N "def" N "ghi" N "jkl");
    COMMAND("redo", X "def" N "abc" N "ghi" N "jkl");
    COMMAND("m -2", X "def" N "abc" N "ghi" N "jkl");
    COMMAND("2m0", X "abc" N "def" N "ghi" N "jkl");

    COMMAND("m $-2", "def" N X "abc" N "ghi" N "jkl");
    KEYS("`'", X "def" N "abc" N "ghi" N "jkl");
    KEYS("Vj:m+2<cr>", "ghi" N "def" N X "abc" N "jkl");
    KEYS("u", X "def" N "abc" N "ghi" N "jkl");

    // Moving the last line (which has no trailing newline) must not merge it
    // with the target line.
    data.setText("abc" N "def" N "gh|i");
    COMMAND("m0", X "ghi" N "abc" N "def");
    data.setText("abc" N "def" N "gh|i");
    COMMAND("m1", "abc" N X "ghi" N "def");

    // move visual selection with indentation
    data.doCommand("set expandtab");
    data.doCommand("set shiftwidth=2");
    data.doCommand("vnoremap <C-S-J> :m'>+<CR>gv=");
    data.doCommand("vnoremap <C-S-K> :m-2<CR>gv=");
    data.setText(
         "int x;" N
         "int y;" N
         "int main() {" N
         "  if (true) {" N
         "  }" N
         "}" N
         "");
    KEYS("Vj<C-S-J>",
         "int main() {" N
         "  int x;" N
         "  int y;" N
         "  if (true) {" N
         "  }" N
         "}" N
         "");
    KEYS("gv<C-S-J>",
         "int main() {" N
         "  if (true) {" N
         "    int x;" N
         "    int y;" N
         "  }" N
         "}" N
         "");
    KEYS("gv<C-S-K>",
         "int main() {" N
         "  int x;" N
         "  int y;" N
         "  if (true) {" N
         "  }" N
         "}" N
         "");
    data.doCommand("vunmap <C-S-K>");
    data.doCommand("vunmap <C-S-J>");
}

void FakeVimTester::test_vim_ex_join()
{
    TestData data;
    setup(&data);

    data.setText("  abc" N X "  def" N "  ghi" N "  jkl");
    COMMAND("j", "  abc" N "  " X "def ghi" N "  jkl");
    COMMAND("u", "  abc" N X "  def" N "  ghi" N "  jkl");
    COMMAND("1j3", "  " X "abc def ghi" N "  jkl");
    COMMAND("u", X "  abc" N "  def" N "  ghi" N "  jkl");
}

void FakeVimTester::test_vim_ex_normal()
{
    TestData data;
    setup(&data);

    // :[range]normal runs the commands on every line in the range, not just the
    // current one (QTCREATORBUG-33296).
    data.setText("Line 1" N "Line 2" N "Line 3");
    COMMAND("%normal A;", "Line 1;" N "Line 2;" N "Line 3" X ";");

    // A sub-range only touches those lines.
    data.setText("Line 1" N "Line 2" N "Line 3");
    COMMAND("1,2normal A;", "Line 1;" N "Line 2" X ";" N "Line 3");

    // A single addressed line runs on that line, not the current one.
    data.setText("Line 1" N "Line 2" N "Line 3");
    COMMAND("3normal A;", "Line 1" N "Line 2" N "Line 3" X ";");

    // Without any range the commands run at the current cursor position.
    // Insert mode started by the commands is terminated at the end, so the
    // cursor ends up back in normal mode on the last inserted character
    // (QTCREATORBUG-25820).
    data.setText("Line 1" N "Line 2" N "Line 3");
    COMMAND("normal A;", "Line 1" X ";" N "Line 2" N "Line 3");

    // Same for insert commands other than "A", e.g. ":normal Iasdf".
    data.setText("Line 1" N "Line 2" N "Line 3");
    COMMAND("normal Iasdf", "asd" X "fLine 1" N "Line 2" N "Line 3");

    // Commands that delete lines work per line, too (as :%normal dd).
    data.setText("Line 1" N "Line 2" N "Line 3");
    COMMAND("%normal dd", X "");
}

void FakeVimTester::test_advanced_commands()
{
    TestData data;
    setup(&data);

    // subcommands
    data.setText("abc" N "  xxx" N "  xxx" N "def");
    COMMAND("%s/xxx/ZZZ/g|%s/ZZZ/OOO/g", "abc" N "  OOO" N "  " X "OOO" N "def");

    // undo/redo all subcommands
    COMMAND(":undo", "abc" N X "  xxx" N "  xxx" N "def");
    COMMAND(":redo", "abc" N X "  OOO" N "  OOO" N "def");

    // redundant characters
    COMMAND(" :::   %s/\\S\\S\\S/ZZZ/g   |"
        "  :: :  :   %s/ZZZ/XXX/g ", "XXX" N "  XXX" N "  XXX" N X "XXX");

    // bar character in regular expression is not command separator
    data.setText("abc");
    COMMAND("%s/a\\|b\\||/X/g|%s/[^X]/Y/g", "XXY");

    // :global command
    data.setText("abc" N "def" N "ghi");
    COMMAND("g/def/d", "abc" N X "ghi");

    data.setText("abc" N "def" N "ghi" N "def" N "jkl");
    COMMAND("g/def/d", "abc" N "ghi" N X "jkl");
}

void FakeVimTester::test_map()
{
    TestData data;
    setup(&data);

    data.setText("abc def");
    data.doCommand("map C i<space>x<space><esc>");
    data.doCommand("map c iXXX");
    data.doCommand("imap c YYY<space>");
    KEYS("C", " x" X " abc def");
    data.doCommand("map C <nop>");
    KEYS("C", " x" X " abc def");
    data.doCommand("map C i<bs><esc><right>");
    KEYS("C", " " X " abc def");
    KEYS("ccc<esc>", " XXXYYY YYY" X "  abc def");
    // unmap
    KEYS(":unmap c<cr>ccc<esc>", "YYY" X " ");
    KEYS(":iunmap c<cr>ccc<esc>", X "c");
    data.doCommand("unmap C");

    data.setText("abc def");
    data.doCommand("imap x (((<space><right><right>)))<esc>");
    KEYS("x", X "bc def");
    KEYS("ix", "((( bc))" X ") def");
    data.doCommand("iunmap x");

    data.setText("abc def");
    data.doCommand("map <c-right> 3l");
    KEYS("<C-Right>", "abc" X " def");
    KEYS("<C-Right>", "abc de" X "f");

    // map vs. noremap
    data.setText("abc def");
    data.doCommand("map x 3l");
    data.doCommand("map X x");
    KEYS("X", "abc" X " def");
    data.doCommand("noremap X x");
    KEYS("X", "abc" X "def");
    data.doCommand("unmap X");
    data.doCommand("unmap x");

    // limit number of recursions in mappings
    data.doCommand("map X Y");
    data.doCommand("map Y Z");
    data.doCommand("map Z X");
    KEYS("X", "abc" X "def");
    data.doCommand("map Z i<space><esc>");
    KEYS("X", "abc" X " def");
    data.doCommand("unmap X");
    data.doCommand("unmap Y");
    data.doCommand("unmap Z");

    // imcomplete mapping
    data.setText("abc");
    data.doCommand("map  Xa  ia<esc>");
    data.doCommand("map  Xb  ib<esc>");
    data.doCommand("map  X   ic<esc>");
    KEYS("Xa", X "aabc");
    KEYS("Xb", X "baabc");
    KEYS("Xic<esc>", X "ccbaabc");

    // unmap
    data.doCommand("unmap  Xa");
    KEYS("Xa<esc>", X "cccbaabc");
    data.doCommand("unmap  Xb");
    KEYS("Xb", X "ccccbaabc");
    data.doCommand("unmap  X");
    KEYS("Xb", X "ccccbaabc");
    KEYS("X<esc>", X "ccccbaabc");

    // recursive mapping
    data.setText("abc");
    data.doCommand("map  X    Y");
    data.doCommand("map  XXX  i1<esc>");
    data.doCommand("map  Y    i2<esc>");
    data.doCommand("map  YZ   i3<esc>");
    data.doCommand("map  _    i <esc>");
    KEYS("_XXX_", X " 1 abc");
    KEYS("XX_0", X " 22 1 abc");
    KEYS("XXXXZ_0", X " 31 22 1 abc");
    KEYS("XXXXX_0", X " 221 31 22 1 abc");
    KEYS("XXZ", X "32 221 31 22 1 abc");
    data.doCommand("unmap  X");
    data.doCommand("unmap  XXX");
    data.doCommand("unmap  Y");
    data.doCommand("unmap  YZ");
    data.doCommand("unmap  _");

    // shift modifier
    data.setText("abc");
    data.doCommand("map  x  i1<esc>");
    data.doCommand("map  X  i2<esc>");
    KEYS("x", X "1abc");
    KEYS("X", X "21abc");
    data.doCommand("map  <S-X>  i3<esc>");
    KEYS("X", X "321abc");
    data.doCommand("map  X  i4<esc>");
    KEYS("X", X "4321abc");
    KEYS("x", X "14321abc");
    data.doCommand("unmap  x");
    data.doCommand("unmap  X");

    // undo/redo mapped input
    data.setText("abc def ghi");
    data.doCommand("map X dwea xyz<esc>3l");
    KEYS("X", "def xyz g" X "hi");
    KEYS("u", X "abc def ghi");
    KEYS("<C-r>", X "def xyz ghi");
    data.doCommand("unmap  X");

    data.setText("abc" N "  def" N "  ghi");
    data.doCommand("map X jdd");
    KEYS("X", "abc" N "  " X "ghi");
    KEYS("u", "abc" N X "  def" N "  ghi");
    KEYS("<c-r>", "abc" N X "  ghi");
    data.doCommand("unmap  X");

    data.setText("abc" N "def" N "ghi");
    data.doCommand("map X jAxxx<cr>yyy<esc>");
    KEYS("X", "abc" N "defxxx" N "yy" X "y" N "ghi");
    KEYS("u", "abc" N "de" X "f" N "ghi");
    KEYS("<c-r>", "abc" N "def" X "xxx" N "yyy" N "ghi");
    data.doCommand("unmap  X");

    /* QTCREATORBUG-7913 */
    data.setText("");
    data.doCommand("noremap l k|noremap k j|noremap j h");
    KEYS("ikkk<esc>", "kk" X "k");
    KEYS("rj", "kk" X "j");
    data.doCommand("unmap l|unmap k|unmap j");

    // bad mapping
    data.setText(X "abc" N "def");
    data.doCommand("map X Zxx");
    KEYS("X", X "abc" N "def");
    // cancelled mapping
    data.doCommand("map X fxxx");
    KEYS("X", X "abc" N "def");
    data.doCommand("map X ciXxx");
    KEYS("X", X "abc" N "def");
    data.doCommand("map Y Xxx");
    KEYS("Y", X "abc" N "def");
    data.doCommand("unmap X|unmap Y");

    // correct mapping after bad one
    data.setText("abc" N "def");
    data.doCommand("map X Y");
    data.doCommand("map Y X");
    data.doCommand("map Xx xxx");
    KEYS("Xr2", X "2bc" N "def");
    data.doCommand("unmap X|unmap x|unmap Y");

    // <C-o>
    data.setText("abc def");
    data.doCommand("imap X <c-o>:%s/def/xxx/<cr>");
    KEYS("iX", "abc xxx");
    data.doCommand("iunmap X");

    // Test mappings in UTF-8 encoding.
    data.setText("abc" N "def");
    data.doKeys(QString::fromUtf8(":no \xc5\xaf xiX<cr>"));
    KEYS("oxyz<esc>", "abc" N "xy" X "z" N "def");
    KEYS(QString::fromUtf8("\xc5\xaf<esc>"), "abc" N "x" X "Xy" N "def");

    /* QTCREATORBUG-8774 */
    data.setText("abc" N "def");
    data.doCommand(QString::fromUtf8("no \xc3\xb8 l|no l k|no k j|no j h"));
    KEYS(QString::fromUtf8("\xc3\xb8"), "a" X "bc" N "def");
    data.doCommand(QString::fromUtf8("unmap \xc3\xb8|unmap l|unmap k|unmap j"));

    // Don't handle mapping in sub-modes that are not followed by movement command.
    data.setText("abc" N "def");
    data.doCommand("map <SPACE> A<cr>xy z<esc><left><left>");
    KEYS("<space>", "abc" N "x" X "y z" N "def");
    KEYS("r<space>", "abc" N "x" X "  z" N "def");
    KEYS("f<space>", "abc" N "x " X " z" N "def");
    KEYS("t<space>", "abc" N "x " X " z" N "def");
    data.doCommand("unmap <SPACE>");

    // operator-pending mappings
    data.setText("abc def" N "ghi jkl");
    data.doCommand("omap <SPACE> aw");
    KEYS("c<space>X<esc>", X "Xdef" N "ghi jkl");
    data.doCommand("onoremap <SPACE> iwX");
    KEYS("c<space>Y<esc>", "X" X "Y" N "ghi jkl");
    data.doCommand("ono <SPACE> l");
    KEYS("d<space>", X "X" N "ghi jkl");
    data.doCommand("unmap <SPACE>");

    data.setText("abc def" N "ghi jkl");
    data.doCommand("onoremap iwwX 3iwX Y");
    KEYS("ciwwX Z<esc>", "X Y " X "Z" N "ghi jkl");
    data.doCommand("unmap <SPACE>X");

    // use mapping for <ESC> in insert
    data.setText("ab" X "c def" N "ghi jkl");
    data.doCommand("inoremap jk <ESC>");
    KEYS("<C-V>jll" "I__jk", "ab" X "__c def" N "gh__i jkl");
    INTEGRITY(false);
    data.doCommand("unmap jk"); // shouldn't unmap for insert mode
    KEYS("ijk", "a" X "b__c def" N "gh__i jkl");
    data.doCommand("iunmap jk");
    KEYS("ijk<ESC>", "aj" X "kb__c def" N "gh__i jkl");

    // Remapping "'" and "`" (normally mark-jump prefixes) must be honored
    // (QTCREATORBUG-31932).
    data.setText("abc def");
    data.doCommand("noremap ' <Right>");
    KEYS("'", "a" X "bc def");
    data.doCommand("unmap '");
    data.setText("abc def");
    data.doCommand("noremap ` <Right>");
    KEYS("`", "a" X "bc def");
    data.doCommand("unmap `");
    // and the '"' register prefix (QTCREATORBUG-11979)
    data.setText("abc def");
    data.doCommand("noremap \" <Right>");
    KEYS("\"", "a" X "bc def");
    data.doCommand("unmap \"");

    // Faithful to the report: the remap is read from a sourced vimrc file.
    QTemporaryFile rc;
    QVERIFY(rc.open());
    rc.write("noremap ' <Right>\n");
    rc.flush();
    data.setText("abc def");
    data.doCommand("source " + rc.fileName());
    KEYS("'", "a" X "bc def");
    data.doCommand("unmap '");

    // A leading "~" in a :source path expands to the home directory
    // (QTCREATORBUG-11161).
    QTemporaryFile home(QDir::homePath() + "/qtc_fakevim_test_XXXXXX");
    if (home.open()) {
        home.write("noremap ' <Right>\n");
        home.flush();
        data.setText("abc def");
        data.doCommand("source ~/" + QFileInfo(home.fileName()).fileName());
        KEYS("'", "a" X "bc def");
        data.doCommand("unmap '");
    }
}

void FakeVimTester::test_vim_command_cc()
{
    TestData data;
    setup(&data);

    data.setText(       "|123" N "456"  N "789" N "abc");
    KEYS("cc456<ESC>",  "45|6" N "456"  N "789" N "abc");
    KEYS("ccabc<Esc>",  "ab|c" N "456"  N "789" N "abc");
    KEYS(".",           "ab|c" N "456"  N "789" N "abc");
    KEYS("j",           "abc"  N "45|6" N "789" N "abc");
    KEYS(".",           "abc"  N "ab|c" N "789" N "abc");
    KEYS("kkk",         "ab|c" N "abc"  N "789" N "abc");
    KEYS("3ccxyz<Esc>", "xy|z" N "abc");
}

void FakeVimTester::test_vim_command_cw()
{
    TestData data;
    setup(&data);
    data.setText(X "123 456");
    KEYS("cwx<Esc>", X "x 456");
}

void FakeVimTester::test_vim_command_cj()
{
    TestData data;
    setup(&data);

    data.setText(testLines);
    KEYS("j$",         cursor(1, -1));
    KEYS("cj<Esc>",    l[0]+"\n|" + '\n' + lmid(3));
    KEYS("P",          lmid(0,1)+'\n' + '|'+lmid(1,2)+'\n' + '\n' +  lmid(3));
    KEYS("u",          l[0]+"\n|" + '\n' + lmid(3));

    data.setText(testLines);
    KEYS("j$",          cursor(1, -1));
    KEYS("cjabc<Esc>",  l[0]+"\nab|c\n" + lmid(3));
    KEYS("u",           cursor(2, 0));
    KEYS("gg",          cursor(0, 0));
    KEYS(".",           "ab|c\n" + lmid(2));
}

void FakeVimTester::test_vim_command_ck()
{
    TestData data;
    setup(&data);

    data.setText(testLines);
    KEYS("j$",          cursor(1, -1));
    KEYS("ck<Esc>",     "|\n" + lmid(2));
    KEYS("P",           '|' + lmid(0,2)+'\n' + '\n' + lmid(2));
}

void FakeVimTester::test_vim_command_c_dollar()
{
    TestData data;
    setup(&data);

    data.setText(testLines);
    KEYS("j",           cursor(1, 0));
    KEYS("$",           cursor(1, -1));
    KEYS("c$<Esc>",     l[0]+'\n' + l[1].left(l[1].length()-2)+'|'+l[1][l[1].length()-2]+'\n' + lmid(2));
    KEYS("c$<Esc>",     l[0]+'\n' + l[1].left(l[1].length()-3)+'|'+l[1][l[1].length()-3]+'\n' + lmid(2));
    KEYS("0c$abc<Esc>", l[0]+'\n' + "ab|c\n" + lmid(2));
    KEYS("0c$abc<Esc>", l[0]+'\n' + "ab|c\n" + lmid(2));
}

void FakeVimTester::test_vim_command_C()
{
    TestData data;
    setup(&data);

    data.setText(testLines);
    KEYS("j",           cursor(1, 0));
    KEYS("Cabc<Esc>",   l[0] + "\nab|c\n" + lmid(2));
    KEYS("Cabc<Esc>",   l[0] + "\nabab|c\n" + lmid(2));
    KEYS("$Cabc<Esc>",  l[0] + "\nababab|c\n" + lmid(2));
    KEYS("0C<Esc>",     l[0] + "\n|\n" + lmid(2));
    KEYS("0Cabc<Esc>",  l[0] + "\nab|c\n" + lmid(2));
}

void FakeVimTester::test_vim_command_dw()
{
    TestData data;
    setup(&data);

    data.setText(testLines);
    KEYS("dw",  "|#include <QtCore>\n" + lmid(2));
    KEYS("dw",  "|include <QtCore>\n" + lmid(2));
    KEYS("dw",  "|<QtCore>\n" + lmid(2));
    KEYS("dw",  "|QtCore>\n" + lmid(2));
    KEYS("dw",  "|>\n" + lmid(2));
    KEYS("dw",  "|\n" + lmid(2)); // Real vim has this intermediate step, too
    KEYS("dw",  "|#include <QtGui>\n" + lmid(3));
    KEYS("dw",  "|include <QtGui>\n" + lmid(3));
    KEYS("dw",  "|<QtGui>\n" + lmid(3));
    KEYS("dw",  "|QtGui>\n" + lmid(3));
    KEYS("dw",  "|>\n" + lmid(3));
}

void FakeVimTester::test_vim_command_dd()
{
    TestData data;
    setup(&data);

    data.setText(testLines);
    KEYS("j",   cursor(1, 0));
    KEYS("dd",  l[0] + "\n|" + lmid(2));
    KEYS(".",   l[0] + "\n|" + lmid(3));
    KEYS("3dd", l[0] + "\n    |QApplication app(argc, argv);\n" + lmid(7));
    KEYS("4l",  l[0] + "\n    QApp|lication app(argc, argv);\n" + lmid(7));
    KEYS("dd",  l[0] + "\n|" + lmid(7));
    KEYS(".",   l[0] + "\n    |return app.exec();\n" + lmid(9));
    KEYS("dd",  l[0] + "\n|" + lmid(9));
}

void FakeVimTester::test_vim_command_dd_2()
{
    TestData data;
    setup(&data);

    data.setText(testLines);
    KEYS("j",   cursor(1, 0));
    KEYS("dd",  l[0] + "\n|" + lmid(2));
    KEYS("p",   l[0] + '\n' + l[2] + "\n|" + l[1] + '\n' + lmid(3));
    KEYS("u",   l[0] + "\n|" + lmid(2));
}

void FakeVimTester::test_vim_command_d_dollar()
{
    TestData data;
    setup(&data);

    data.setText(testLines);
    KEYS("j$",  cursor(1, -1));
    KEYS("$d$", l[0]+'\n' + l[1].left(l[1].length()-2)+'|'+l[1][l[1].length()-2]+'\n' + lmid(2));
    KEYS("0d$", l[0] + '\n'+"|\n" + lmid(2));
}

void FakeVimTester::test_vim_command_dj()
{
    TestData data;
    setup(&data);

    data.setText(testLines);
    KEYS("j$",   cursor(1, -1));
    KEYS("dj",  l[0]+"\n|" + lmid(3));
    KEYS("P",   lmid(0,1)+'\n' + '|'+lmid(1));
    KEYS("0",   lmid(0,1)+'\n' + '|'+lmid(1));
    KEYS("dj",  l[0]+"\n|" + lmid(3));
    KEYS("P",   lmid(0,1)+'\n' + '|'+lmid(1));
    KEYS("05l", l[0]+'\n' + l[1].left(5) + '|' + l[1].mid(5)+'\n' + lmid(2));
    KEYS("dj",  l[0]+"\n|" + lmid(3));
    KEYS("P",   lmid(0,1)+'\n' + '|'+lmid(1));
    KEYS("dj",  l[0]+"\n|" + lmid(3));
    KEYS("p",   lmid(0,1)+'\n' + lmid(3,1)+'\n' + '|'+lmid(1,2)+'\n' + lmid(4));
}

void FakeVimTester::test_vim_command_dk()
{
    TestData data;
    setup(&data);

    data.setText(testLines);
    KEYS("j$",    cursor(1, -1));
    KEYS("dk",   '|' + lmid(2));
    KEYS("P",    '|' + lmid(0));
    KEYS("j0",   l[0]+ "\n|" + lmid(1));
    KEYS("dk",   '|' + lmid(2));
    KEYS("P",    '|' + lmid(0));
    KEYS("j05l", l[0]+'\n' + l[1].left(5) + '|' + l[1].mid(5)+'\n' + lmid(2));
    KEYS("dk",   '|' + lmid(2));
    KEYS("P",    '|' + lmid(0));
    KEYS("j05l", l[0]+'\n' + l[1].left(5) + '|' + l[1].mid(5)+'\n' + lmid(2));
    KEYS("dk",   '|' + lmid(2));
    KEYS("p",    lmid(2,1)+'\n' + '|' + lmid(0,2)+'\n' + lmid(3));
}

void FakeVimTester::test_vim_command_dgg()
{
    TestData data;
    setup(&data);

    data.setText(testLines);
    KEYS("Gk",   lmid(0, l.size()-2)+'\n' +  '|'+lmid(l.size()-2));
    KEYS("dgg",  "|");
    KEYS("u",    '|' + lmid(0));
}

void FakeVimTester::test_vim_command_dG()
{
    TestData data;
    setup(&data);

    data.setText(testLines);
    KEYS("dG",   "|");
    KEYS("u",    '|' + lmid(0));
    KEYS("j",    cursor(1, 0));
    KEYS("dG",   "|");
    KEYS("u",    l[0]+'\n' + '|' + lmid(1));
    KEYS("Gk",   lmid(0, l.size()-2)+'\n' + '|'+lmid(l.size()-2));

    NOT_IMPLEMENTED
    // include movement to first column, as otherwise the result depends on the 'startofline' setting
    KEYS("dG0",  lmid(0, l.size()-2)+'\n' + '|'+lmid(l.size()-2,1));
    KEYS("dG0",  lmid(0, l.size()-3)+'\n' + '|'+lmid(l.size()-3,1));
}

void FakeVimTester::test_vim_command_D()
{
    TestData data;
    setup(&data);

    data.setText(testLines);
    KEYS("j",    cursor(1, 0));
    KEYS("$D",   l[0]+'\n' + l[1].left(l[1].length()-2)+'|'+l[1][l[1].length()-2]+'\n' + lmid(2));
    KEYS("0D",   l[0] + "\n|\n" + lmid(2));
}

void FakeVimTester::test_vim_command_dollar()
{
    TestData data;
    setup(&data);

    data.setText(testLines);
    KEYS("j$", cursor(1, -1));
    KEYS("j$", cursor(2, -1));
    KEYS("2j", cursor(4, -1));
}

void FakeVimTester::test_vim_command_down()
{
    TestData data;
    setup(&data);

    data.setText(testLines);
    KEYS("j",  l[0]+ "\n|" + lmid(1));
    KEYS("3j", lmid(0,4)+'\n' + "|int main(int argc, char *argv[])\n" + lmid(5));
    KEYS("4j", lmid(0,8)+'\n' + "|    return app.exec();\n" + lmid(9));
}

void FakeVimTester::test_vim_command_dfx_down()
{
    TestData data;
    setup(&data);

    data.setText(testLines);
    KEYS("j4l",  l[0] + "\n#inc|lude <QtCore>\n" + lmid(2));

    //NOT_IMPLEMENTED
    KEYS("df ",  l[0] + "\n#inc|<QtCore>\n" + lmid(2));
    KEYS("j",    l[0] + "\n#inc<QtCore>\n#inc|lude <QtGui>\n" + lmid(3));
    KEYS(".",    l[0] + "\n#inc<QtCore>\n#inc|<QtGui>\n" + lmid(3));
    KEYS("u",    l[0] + "\n#inc<QtCore>\n#inc|lude <QtGui>\n" + lmid(3));
    KEYS("u",    l[0] + "\n#inc|lude <QtCore>\n" + lmid(2));
}

void FakeVimTester::test_vim_command_Cxx_down_dot()
{
    TestData data;
    setup(&data);

    data.setText(testLines);
    KEYS("j4l",      l[0] + "\n#inc|lude <QtCore>\n" + lmid(2));
    KEYS("Cxx<Esc>", l[0] + "\n#incx|x\n" + lmid(2));
    KEYS("j",        l[0] + "\n#incxx\n#incl|ude <QtGui>\n" + lmid(3));
    KEYS(".",        l[0] + "\n#incxx\n#inclx|x\n" + lmid(3));
}

void FakeVimTester::test_vim_command_e()
{
    TestData data;
    setup(&data);

    data.setText(testLines);
    KEYS("e",  lmid(0,1)+'\n' + "|#include <QtCore>\n" + lmid(2));
    KEYS("e",  lmid(0,1)+'\n' + "#includ|e <QtCore>\n" + lmid(2));
    KEYS("e",  lmid(0,1)+'\n' + "#include |<QtCore>\n" + lmid(2));
    KEYS("3e", lmid(0,2)+'\n' + "|#include <QtGui>\n" + lmid(3));
    KEYS("e",  lmid(0,2)+'\n' + "#includ|e <QtGui>\n" + lmid(3));
    KEYS("e",  lmid(0,2)+'\n' + "#include |<QtGui>\n" + lmid(3));
    KEYS("e",  lmid(0,2)+'\n' + "#include <QtGu|i>\n" + lmid(3));
    KEYS("4e", lmid(0,4)+'\n' + "int main|(int argc, char *argv[])\n" + lmid(5));
    KEYS("e",  lmid(0,4)+'\n' + "int main(in|t argc, char *argv[])\n" + lmid(5));
    KEYS("e",  lmid(0,4)+'\n' + "int main(int arg|c, char *argv[])\n" + lmid(5));
    KEYS("e",  lmid(0,4)+'\n' + "int main(int argc|, char *argv[])\n" + lmid(5));
    KEYS("e",  lmid(0,4)+'\n' + "int main(int argc, cha|r *argv[])\n" + lmid(5));
    KEYS("e",  lmid(0,4)+'\n' + "int main(int argc, char |*argv[])\n" + lmid(5));
    KEYS("e",  lmid(0,4)+'\n' + "int main(int argc, char *arg|v[])\n" + lmid(5));
    KEYS("e",  lmid(0,4)+'\n' + "int main(int argc, char *argv[]|)\n" + lmid(5));
    KEYS("e",  lmid(0,5)+'\n' + "|{\n" + lmid(6));
    KEYS("10k","|\n" + lmid(1)); // home.
}

void FakeVimTester::test_vim_command_i()
{
    TestData data;
    setup(&data);

    data.setText(testLines);

    // empty insertion at start of document
    KEYS("i<Esc>", '|' + testLines);
    KEYS("u", '|' + testLines);

    // small insertion at start of document
    KEYS("ix<Esc>", "|x" + testLines);
    KEYS("u", '|' + testLines);
    COMMAND("redo", "|x" + testLines);
    KEYS("u", '|' + testLines);

    // small insertion at start of document
    KEYS("ixxx<Esc>", "xx|x" + testLines);
    KEYS("u", '|' + testLines);

    // combine insertions
    KEYS("i1<Esc>", "|1" + testLines);
    KEYS("i2<Esc>", "|21" + testLines);
    KEYS("i3<Esc>", "|321" + testLines);
    KEYS("u",       "|21" + testLines);
    KEYS("u",       "|1" + testLines);
    KEYS("u",       '|' + testLines);
    KEYS("ia<Esc>", "|a" + testLines);
    KEYS("ibx<Esc>", "b|xa" + testLines);
    KEYS("icyy<Esc>", "bcy|yxa" + testLines);
    KEYS("u", "b|xa" + testLines);
    KEYS("u", "|a" + testLines);
    COMMAND("redo", "|bxa" + testLines);
    KEYS("u", "|a" + testLines);
}

void FakeVimTester::test_vim_command_left()
{
    TestData data;
    setup(&data);

    data.setText(testLines);
    KEYS("4j",  lmid(0, 4) + "\n|int main(int argc, char *argv[])\n" + lmid(5));
    KEYS("h",   lmid(0, 4) + "\n|int main(int argc, char *argv[])\n" + lmid(5));
    KEYS("$",   lmid(0, 4) + "\nint main(int argc, char *argv[]|)\n" + lmid(5));
    KEYS("h",   lmid(0, 4) + "\nint main(int argc, char *argv[|])\n" + lmid(5));
    KEYS("3h",  lmid(0, 4) + "\nint main(int argc, char *ar|gv[])\n" + lmid(5));
    KEYS("50h", lmid(0, 4) + "\n|int main(int argc, char *argv[])\n" + lmid(5));
}

void FakeVimTester::test_vim_command_r()
{
    TestData data;
    setup(&data);

    data.setText(testLines);
    KEYS("4j",  lmid(0, 4) + "\n|int main(int argc, char *argv[])\n" + lmid(5));
    KEYS("$",   lmid(0, 4) + "\nint main(int argc, char *argv[]|)\n" + lmid(5));
    KEYS("rx",  lmid(0, 4) + "\nint main(int argc, char *argv[]|x\n" + lmid(5));
    KEYS("2h",  lmid(0, 4) + "\nint main(int argc, char *argv|[]x\n" + lmid(5));
    KEYS("4ra", lmid(0, 4) + "\nint main(int argc, char *argv|[]x\n" + lmid(5));
    KEYS("3rb", lmid(0, 4) + "\nint main(int argc, char *argvbb|b\n" + lmid(5));
    KEYS("2rc", lmid(0, 4) + "\nint main(int argc, char *argvbb|b\n" + lmid(5));
    KEYS("h2rc",lmid(0, 4) + "\nint main(int argc, char *argvbc|c\n" + lmid(5));
}

void FakeVimTester::test_vim_command_right()
{
    TestData data;
    setup(&data);

    data.setText(testLines);
    KEYS("4j",  lmid(0, 4) + "\n|int main(int argc, char *argv[])\n" + lmid(5));
    KEYS("l",   lmid(0, 4) + "\ni|nt main(int argc, char *argv[])\n" + lmid(5));
    KEYS("3l",  lmid(0, 4) + "\nint |main(int argc, char *argv[])\n" + lmid(5));
    KEYS("50l", lmid(0, 4) + "\nint main(int argc, char *argv[]|)\n" + lmid(5));
}

void FakeVimTester::test_vim_command_up()
{
    TestData data;
    setup(&data);

    data.setText(testLines);
    KEYS("9j", lmid(0, 9) + "\n|}\n");
    KEYS("k",  lmid(0, 8) + "\n|    return app.exec();\n" + lmid(9));
    KEYS("4k", lmid(0, 4) + "\n|int main(int argc, char *argv[])\n" + lmid(5));
    KEYS("3k", lmid(0, 1) + "\n|#include <QtCore>\n" + lmid(2));
    KEYS("k",  cursor(0, 0));
    KEYS("2k", cursor(0, 0));
}

void FakeVimTester::test_vim_command_w()
{
    TestData data;
    setup(&data);

    data.setText(testLines);
    KEYS("w",   lmid(0,1)+'\n' + "|#include <QtCore>\n" + lmid(2));
    KEYS("w",   lmid(0,1)+'\n' + "#|include <QtCore>\n" + lmid(2));
    KEYS("w",   lmid(0,1)+'\n' + "#include |<QtCore>\n" + lmid(2));
    KEYS("3w",  lmid(0,2)+'\n' + "|#include <QtGui>\n" + lmid(3));
    KEYS("w",   lmid(0,2)+'\n' + "#|include <QtGui>\n" + lmid(3));
    KEYS("w",   lmid(0,2)+'\n' + "#include |<QtGui>\n" + lmid(3));
    KEYS("w",   lmid(0,2)+'\n' + "#include <|QtGui>\n" + lmid(3));
    KEYS("4w",  lmid(0,4)+'\n' + "int |main(int argc, char *argv[])\n" + lmid(5));
    KEYS("w",   lmid(0,4)+'\n' + "int main|(int argc, char *argv[])\n" + lmid(5));
    KEYS("w",   lmid(0,4)+'\n' + "int main(|int argc, char *argv[])\n" + lmid(5));
    KEYS("w",   lmid(0,4)+'\n' + "int main(int |argc, char *argv[])\n" + lmid(5));
    KEYS("w",   lmid(0,4)+'\n' + "int main(int argc|, char *argv[])\n" + lmid(5));
    KEYS("w",   lmid(0,4)+'\n' + "int main(int argc, |char *argv[])\n" + lmid(5));
    KEYS("w",   lmid(0,4)+'\n' + "int main(int argc, char |*argv[])\n" + lmid(5));
    KEYS("w",   lmid(0,4)+'\n' + "int main(int argc, char *|argv[])\n" + lmid(5));
    KEYS("w",   lmid(0,4)+'\n' + "int main(int argc, char *argv|[])\n" + lmid(5));
    KEYS("w",   lmid(0,5)+'\n' + "|{\n" + lmid(6));

    // "w" on a non-BMP character (here the U+1F389 emoji, a surrogate pair)
    // must advance past it instead of stalling / spinning forever
    // (QTCREATORBUG-25873).
    const QString emoji = QString::fromUtf8("\xf0\x9f\x8e\x89");
    data.setText("");
    data.editor()->document()->setPlainText(emoji + QLatin1String(" ab"));
    data.setPosition(0);
    data.doKeys("w");
    QCOMPARE(data.position(), 3);
}

void FakeVimTester::test_vim_command_yyp()
{
    TestData data;
    setup(&data);

    data.setText(testLines);
    KEYS("4j",  lmid(0, 4) + "\n|int main(int argc, char *argv[])\n" + lmid(5));
    KEYS("yyp", lmid(0, 4) + '\n' + lmid(4, 1) + "\n|" + lmid(4));
}

void FakeVimTester::test_vim_command_y_dollar()
{
    TestData data;
    setup(&data);

    data.setText(testLines);
    KEYS("j",    l[0]+"\n|" + lmid(1));
    KEYS("$y$p", l[0]+'\n'+ l[1]+"|>\n" + lmid(2));
    KEYS("$y$p", l[0]+'\n'+ l[1]+">|>\n" + lmid(2));
    KEYS("$y$P", l[0]+'\n'+ l[1]+">|>>\n" + lmid(2));
    KEYS("$y$P", l[0]+'\n'+ l[1]+">>|>>\n" + lmid(2));
}

void FakeVimTester::test_vim_command_percent()
{
    TestData data;
    setup(&data);

    data.setText(
        "bool f(int arg1) {" N
        "    Q_ASSERT(arg1 >= 0);" N
        "    if (arg1 > 0) return true; else if (arg1 <= 0) return false;" N
        "}" N
    );

    KEYS("%",
        "bool f(int arg1" X ") {" N
        "    Q_ASSERT(arg1 >= 0);" N
        "    if (arg1 > 0) return true; else if (arg1 <= 0) return false;" N
        "}" N
    );

    KEYS("%",
        "bool f" X "(int arg1) {" N
        "    Q_ASSERT(arg1 >= 0);" N
        "    if (arg1 > 0) return true; else if (arg1 <= 0) return false;" N
        "}" N
    );

    KEYS("$h%",
        "bool f(int arg1) {" N
        "    Q_ASSERT(arg1 >= 0);" N
        "    if (arg1 > 0) return true; else if (arg1 <= 0) return false;" N
        X "}" N
     );

    KEYS("%",
        "bool f(int arg1) " X "{" N
        "    Q_ASSERT(arg1 >= 0);" N
        "    if (arg1 > 0) return true; else if (arg1 <= 0) return false;" N
        "}" N
     );

    KEYS("j%",
        "bool f(int arg1) {" N
        "    Q_ASSERT" X "(arg1 >= 0);" N
        "    if (arg1 > 0) return true; else if (arg1 <= 0) return false;" N
        "}" N
    );

    KEYS("%",
        "bool f(int arg1) {" N
        "    Q_ASSERT(arg1 >= 0" X ");" N
        "    if (arg1 > 0) return true; else if (arg1 <= 0) return false;" N
        "}" N
    );

    KEYS("j%",
        "bool f(int arg1) {" N
        "    Q_ASSERT(arg1 >= 0);" N
        "    if (arg1 > 0) return true; else if (arg1 <= 0" X ") return false;" N
        "}" N
    );

    KEYS("0%",
        "bool f(int arg1) {" N
        "    Q_ASSERT(arg1 >= 0);" N
        "    if (arg1 > 0" X ") return true; else if (arg1 <= 0) return false;" N
        "}" N
    );

    KEYS("%",
        "bool f(int arg1) {" N
        "    Q_ASSERT(arg1 >= 0);" N
        "    if " X "(arg1 > 0) return true; else if (arg1 <= 0) return false;" N
        "}" N
    );

    // jump to 50% of buffer
    KEYS("50%",
        "bool f(int arg1) {" N
        "    Q_ASSERT(arg1 >= 0);" N
        "    " X "if (arg1 > 0) return true; else if (arg1 <= 0) return false;" N
        "}" N
    );
}

void FakeVimTester::test_vim_percent_like_vim()
{
    // With matchBracketsLikeVim, % matches brackets purely textually, ignoring
    // syntax, as in real Vim.
    auto &opt = FakeVim::Internal::settings().matchBracketsLikeVim;
    const bool saved = opt.value();
    opt.setValue(true);

    TestData data;
    setup(&data);

    // Nested brackets: % on the first ( matches the outer ) and back.
    data.setText("|(a(b)c)");
    KEYS("%", "(a(b)c|)");
    KEYS("%", "|(a(b)c)");

    // A bracket inside a string is counted (Vim is not syntax-aware), so % on
    // the opening ( jumps to the ) between the quotes, not the one after them.
    data.setText("|( \")\" )");
    KEYS("%", "( \"|)\" )");

    opt.setValue(saved);
}

void FakeVimTester::test_vim_command_Yp()
{
    TestData data;
    setup(&data);

    data.setText(testLines);
    KEYS("4j",  lmid(0, 4) + "\n|int main(int argc, char *argv[])\n" + lmid(5));
    KEYS("Yp", lmid(0, 4) + '\n' + lmid(4, 1) + "\n|" + lmid(4));
}

void FakeVimTester::test_vim_command_ma_yank()
{
    TestData data;
    setup(&data);

    data.setText(testLines);
    KEYS("4j",  lmid(0, 4) + "\n|int main(int argc, char *argv[])\n" + lmid(5));
    KEYS("ygg", '|' + lmid(0));
    KEYS("4j",  lmid(0, 4) + "\n|int main(int argc, char *argv[])\n" + lmid(5));
    KEYS("p",   lmid(0,5) + "\n|" + lmid(0,4) +'\n' + lmid(4));

    data.setText(testLines);

    KEYS("gg",     '|' + lmid(0));
    KEYS("ma",     '|' + lmid(0));
    KEYS("4j",     lmid(0, 4) + "\n|int main(int argc, char *argv[])\n" + lmid(5));
    KEYS("mb",     lmid(0,4) + "\n|" + lmid(4));
    KEYS("\"ay'a", '|' + lmid(0));
    KEYS("'b",     lmid(0,4) + "\n|" + lmid(4));
    KEYS("\"ap",   lmid(0,5) + "\n|" + lmid(0,4) +'\n' + lmid(4));
}

void FakeVimTester::test_vim_command_Gyyp()
{
    TestData data;
    setup(&data);

    data.setText(testLines);
    KEYS("Gk",  lmid(0, l.size()-2) + "\n|" + lmid(l.size()-2));
    KEYS("yyp", lmid(0) + '|' + lmid(9, 1)+'\n');
}

void FakeVimTester::test_i_cw_i()
{
    TestData data;
    setup(&data);

    data.setText(testLines);
    KEYS("j",           l[0] + "\n|" + lmid(1));
    KEYS("ixx<Esc>",    l[0] + "\nx|x" + lmid(1));
    KEYS("cwyy<Esc>",   l[0] + "\nxy|y" + lmid(1));
    KEYS("iaa<Esc>",    l[0] + "\nxya|ay" + lmid(1));
}

void FakeVimTester::test_vim_command_J()
{
    TestData data;
    setup(&data);

    data.setText(testLines);
    KEYS("4j4l",  lmid(0, 4) + "\nint |main(int argc, char *argv[])\n" + lmid(5));

    KEYS("J", lmid(0, 5) + "| " + lmid(5));
    KEYS("u", lmid(0, 4) + "\nint |main(int argc, char *argv[])\n" + lmid(5));
    COMMAND("redo", lmid(0, 4) + "\nint |main(int argc, char *argv[]) " + lmid(5));

    KEYS("3J", lmid(0, 5) + " " + lmid(5, 1) + " " + lmid(6, 1).mid(4) + "| " + lmid(7));
    KEYS("uu", lmid(0, 4) + "\nint |main(int argc, char *argv[])\n" + lmid(5));
    COMMAND("redo", lmid(0, 4) + "\nint |main(int argc, char *argv[]) " + lmid(5));

    // Joining comments
    data.doCommand("set formatoptions=f");
    data.setText("// abc" N "// def");
    KEYS("J", "// abc def");

    data.setText("/*" N X "* abc" N "* def" N "*/");
    KEYS("J", "/*" N "* abc def" N "*/");

    data.setText("# abc" N "# def");
    KEYS("J", "# abc def");
}

void FakeVimTester::test_vim_command_put_at_eol()
{
    TestData data;
    setup(&data);

    data.setText(testLines);
    KEYS("j$", cursor(1, -1));
    KEYS("y$", cursor(1, -1));
    KEYS("p",  lmid(0,2)+"|>\n" + lmid(2));
    KEYS("p",  lmid(0,2)+">|>\n" + lmid(2));
    KEYS("$",  lmid(0,2)+">|>\n" + lmid(2));
    KEYS("P",  lmid(0,2)+">|>>\n" + lmid(2));
}

void FakeVimTester::test_vim_command_oO()
{
    TestData data;
    setup(&data);

    data.setText(testLines);
    KEYS("gg",              '|' + lmid(0));
    KEYS("Ol1<Esc>",    "l|1\n" + lmid(0));
    KEYS("gg",              "|l1\n" + lmid(0));
    KEYS("ol2<Esc>",    "l1\n" "l|2\n" + lmid(0));
    KEYS("Gk$",         "l1\n" "l2\n" + lmid(0,l.size()-2)+'\n' + '|'+lmid(l.size()-2));
    KEYS("ol-1<Esc>",   "l1\n" "l2\n" + lmid(0) + "l-|1\n");
    KEYS("Gk",          "l1\n" "l2\n" + lmid(0) + "|l-1\n");
    KEYS("Ol-2<Esc>",   "l1\n" "l2\n" + lmid(0) + "l-|2\n" + "l-1\n");
}

void FakeVimTester::test_vim_command_x()
{
    TestData data;
    setup(&data);

    data.setText(testLines);
    KEYS("x", '|' + lmid(0));
    KEYS("j$", cursor(1, -1));
    KEYS("x", lmid(0,1)+'\n' + l[1].left(l[1].length()-2)+'|'+l[1].mid(l[1].length()-2,1)+'\n' + lmid(2));
}

void FakeVimTester::test_vim_visual_d()
{
    TestData data;
    setup(&data);

    data.setText(testLines);
    KEYS("vd",  '|' + lmid(1));
    KEYS("u",   '|' + lmid(0));
    KEYS("vx",  '|' + lmid(1));
    KEYS("u",   '|' + lmid(0));
    KEYS("vjd", '|' + lmid(1).mid(1));
    KEYS("u",   '|' + lmid(0));
    KEYS("j",   lmid(0, 1)+'\n' + '|' + lmid(1));
    KEYS("vd",  lmid(0, 1)+'\n' + '|' + lmid(1).mid(1));
    KEYS("u",   lmid(0, 1)+'\n' + '|' + lmid(1));
    KEYS("vx",  lmid(0, 1)+'\n' + '|' + lmid(1).mid(1));
    KEYS("u",   lmid(0, 1)+'\n' + '|' + lmid(1));
    KEYS("vhx", lmid(0, 1)+'\n' + '|' + lmid(1).mid(1));
    KEYS("u",   lmid(0, 1)+'\n' + '|' + lmid(1));
    KEYS("vlx", lmid(0, 1)+'\n' + '|' + lmid(1).mid(2));
    KEYS("P",   lmid(0, 1)+'\n' + lmid(1).left(1)+'|'+lmid(1).mid(1));
    KEYS("vhd", lmid(0, 1)+'\n' + '|' + lmid(1).mid(2));
    KEYS("u",   lmid(0, 1)+'\n' + '|' + lmid(1));

    KEYS("v$d",     lmid(0, 1)+'\n' + '|' + lmid(2));
    KEYS("v$od",    lmid(0, 1)+'\n' + '|' + lmid(3));
    KEYS("$v$x",    lmid(0, 1)+'\n' + lmid(3,1) + '|' + lmid(4));
    KEYS("0v$d",    lmid(0, 1)+'\n' + '|' + lmid(5));
    KEYS("$v0d",    lmid(0, 1)+'\n' + "|\n" + lmid(6));
    KEYS("v$o0k$d", '|' + lmid(6));
}

void FakeVimTester::test_vim_Visual_d()
{
    TestData data;
    setup(&data);

    data.setText(testLines);
    KEYS("Vd",    '|' + lmid(1));
    KEYS("V2kd",  '|' + lmid(2));
    KEYS("u",     '|' + lmid(1));
    KEYS("u",     '|' + lmid(0));
    KEYS("j",     lmid(0,1)+'\n' + '|' + lmid(1));
    KEYS("V$d",   lmid(0,1)+'\n' + '|' + lmid(2));
    KEYS("$V$$d", lmid(0,1)+'\n' + '|' + lmid(3));
    KEYS("Vkx",   '|' + lmid(4));
    KEYS("P",     '|' + lmid(0,1)+'\n' + lmid(3));
}

void FakeVimTester::test_vim_visual_block_D()
{
    TestData data;
    setup(&data);

    data.setText("abc def" N "ghi" N "" N "jklm");
    KEYS("l<C-V>3j", "abc def" N "ghi" N "" N "jk" X "lm");
    KEYS("D", X "a" N "g" N "" N "j");

    KEYS("u", "a" X "bc def" N "ghi" N "" N "jklm");
    KEYS("<C-R>", X "a" N "g" N "" N "j");
    KEYS("u", "a" X "bc def" N "ghi" N "" N "jklm");
    KEYS(".", X "a" N "g" N "" N "j");
}

void FakeVimTester::test_vim_commentary_emulation()
{
    TestData data;
    setup(&data);
    data.doCommand("set commentary");

    // Commenting a single line
    data.setText("abc" N "def");
    KEYS("gcc", X "// abc" N "def");
    KEYS("gcc", X "abc" N "def");
    KEYS(".", X "// abc" N "def");

    // Multiple lines
    data.setText("abc" N "  def" N "ghi");
    KEYS("gcj", X "// abc" N "  // def" N "ghi");
    KEYS("gcj", X "abc" N "  def" N "ghi");
    KEYS("gc2j", X "// abc" N "  // def" N "// ghi");
    KEYS("gcj", X "abc" N "  def" N "// ghi");
    KEYS(".", X "// abc" N "  // def" N "// ghi");

    // Visual mode
    data.setText("abc" N "def");
    KEYS("Vjgc", X "// abc" N "// def");
    KEYS(".", X "abc" N "def");
}

void FakeVimTester::test_vim_commentary_file_names()
{
    TestData data;
    setup(&data);
    data.doCommand("set commentary");

    // Default is "//"
    data.setText("abc");
    KEYS("gcc", X "// abc");

    // pri and pro
    data.handler->setCurrentFileName("Test.pri");
    data.setText("abc");
    KEYS("gcc", X "# abc");
    data.handler->setCurrentFileName("Test.pro");
    KEYS("gcc", X "abc");

    // .h .hpp .cpp
    data.handler->setCurrentFileName("Test.h");
    data.setText("abc");
    KEYS("gcc", X "// abc");
    data.handler->setCurrentFileName("Test.hpp");
    KEYS("gcc", X "abc");
    data.handler->setCurrentFileName("Test.cpp");
    KEYS("gcc", X "// abc");
}

void FakeVimTester::test_vim_replace_with_register_emulation()
{
    TestData data;
    setup(&data);
    data.doCommand("set replacewithregister");

    // Simple replace
    data.setText("abc def ghi");
    KEYS("yw", "abc def ghi");
    KEYS("w", "abc " X "def ghi");
    KEYS("grw", "abc " X "abc ghi");
    KEYS("w", "abc abc " X "ghi");
    KEYS(".", "abc abc " X "abc ");

    // Registers
    data.setText("abc def ghi jkl mno");
    KEYS("\"xyiw", "abc def ghi jkl mno");
    KEYS("w", "abc " X "def ghi jkl mno");
    KEYS("yiw", "abc " X "def ghi jkl mno");
    KEYS("w", "abc def " X "ghi jkl mno");
    KEYS("griw", "abc def " X "def jkl mno");
    KEYS("w", "abc def def " X "jkl mno");
    KEYS("\"xgriw", "abc def def " X "abc mno");
    KEYS("w", "abc def def abc " X "mno");
    KEYS(".", "abc def def abc " X "abc");

    // Replace entire line
    data.setText("abc" N "def" N "ghi" N "jkhl");
    KEYS("yyj", "abc" N X "def" N "ghi" N "jkhl");
    KEYS("grr", "abc" N X "abc" N "ghi" N "jkhl");
    KEYS("j", "abc" N "abc" N X "ghi" N "jkhl");
    KEYS(".", "abc" N "abc" N X "abc" N "jkhl");

    // Visual line mode
    data.setText("abc" N "def" N "ghi" N "jkhl");
    KEYS("yyj", "abc" N X "def" N "ghi" N "jkhl");
    KEYS("Vgr", "abc" N X "abc" N "ghi" N "jkhl");
    KEYS("j", "abc" N "abc" N X "ghi" N "jkhl");
    KEYS(".", "abc" N "abc" N X "abc" N "jkhl");

    // Visual char mode
    data.setText("abc defghi");
    KEYS("yiw", "abc defghi");
    KEYS("w", "abc defghi");
    KEYS("v4lgr", "abc abci");
}

void FakeVimTester::test_vim_exchange_emulation()
{
    TestData data;
    setup(&data);
    data.doCommand("set exchange");

    // Simple exchange
    data.setText("abc def");
    KEYS("cxiw", "abc def");
    KEYS("W", "abc " X "def");
    KEYS(".", "def abc");

    // Clearing pending exchange
    data.setText("abc def ghi");
    KEYS("cxiw", "abc def ghi");
    KEYS("cxc", "abc def ghi");
    KEYS("W", "abc " X "def ghi");
    KEYS("cxiw", "abc def" X " ghi");
    KEYS("W", "abc def " X "ghi");
    KEYS(".", "abc ghi def");

    // Exchange line
    data.setText("abc" N "def");
    KEYS("cxx", "abc" N "def");
    KEYS("j", "abc" N "def");
    KEYS(".", "def" N "abc");
}

void FakeVimTester::test_vim_arg_text_obj_emulation()
{
    TestData data;
    setup(&data);
    data.doCommand("set argtextobj");

    data.setText("foo(int" X " i, double d, float f)");
    KEYS("dia", "foo(" X ", double d, float f)");
    KEYS("wdia", "foo(, " X ", float f)");
    KEYS("wdia", "foo(, , " X ")");

    data.setText("foo(int" X " i, double d, float f, long l)");
    KEYS("daa", "foo(" X "double d, float f, long l)");
    KEYS("WWdaa", "foo(double d" X ", long l)");
    KEYS("Wdaa", "foo(double d)");
    KEYS("daa", "foo()");

    data.setText("foo(std::map<int" X ", double> map)");
    KEYS("dia", "foo()");

    data.setText("foo(const C c" X " = C(bar, baz))");
    KEYS("dia", "foo()");
}

void FakeVimTester::test_vim_surround_emulation()
{
    TestData data;
    setup(&data);
    data.doCommand("set surround");

    // ys and ds
    data.setText("abc");
    KEYS(R"(ysawb)",      R"((abc))");
    KEYS(R"(ysabB)",     R"({(abc)})");
    KEYS(R"(ysaB])",    R"([{(abc)}])");
    KEYS(R"(ysa]>)",   R"(<[{(abc)}]>)");
    KEYS(R"(ysa>")",  R"("<[{(abc)}]>")");
    KEYS(R"(ysa"')", R"('"<[{(abc)}]>"')");
    KEYS(R"(ds')",    R"("<[{(abc)}]>")");
    KEYS(R"(ds")",     R"(<[{(abc)}]>)");
    KEYS(R"(ds>)",      R"([{(abc)}])");
    KEYS(R"(ds])",       R"({(abc)})");
    KEYS(R"(ds})",        R"((abc))");
    KEYS(R"(ds))",         R"(abc)");

    data.setText("abc d|ef ghi");
    KEYS("ysiWb", "abc (def) ghi");
    KEYS(".", "abc ((def)) ghi");
    KEYS("dsb", "abc (def) ghi");
    KEYS(".", "abc def ghi");
    KEYS("ysaWb", "abc (def) ghi");
    KEYS(".", "abc ((def)) ghi");
    KEYS("dsb", "abc (def) ghi");
    KEYS(".", "abc def ghi");

    // yss
    data.setText("\t" "abc");
    KEYS("yssb", "\t" "(abc)");
    KEYS(".", "\t" "((abc))");

    // Surround with function
    data.setText("abc");
    KEYS("ysiWftest<CR>", "test(abc)");
    KEYS(".", "test(test(abc))");

    // yS puts text on a new line
    data.setText("abc");
    KEYS("ySsB", "{" N
                 "abc" N
                 "}");

    // cs
    data.setText("(abc)");
    KEYS(R"(csbB)",   R"({abc})");
    KEYS(R"(csB])",   R"([abc])");
    KEYS(R"(cs]>)",   R"(<abc>)");
    KEYS(R"(cs>")",   R"("abc")");
    KEYS(R"(cs"')",   R"('abc')");

    // Visual line mode
    data.setText("abc" N);
    KEYS("VSB", "{" N
                 "abc" N
                 "}" N);

    // Visual char mode
    data.setText("abc");
    KEYS("vlSB", "{ab}c");

    // Visual block mode
    data.setText("abc" N "def");
    KEYS("<C-v>ljSB", "{ab}c" N "{de}f");
}

void FakeVimTester::test_vim_unimpaired_emulation()
{
    TestData data;
    setup(&data);
    data.doCommand("set unimpaired");

    // ]<Space> / [<Space>: add blank lines below/above, cursor stays put.
    data.setText("abc" N "d|ef" N "ghi");
    KEYS("]<Space>", "abc" N "d|ef" N "" N "ghi");
    KEYS("[<Space>", "abc" N "" N "d|ef" N "" N "ghi");

    // With a count.
    data.setText("a|bc" N "def");
    KEYS("2]<Space>", "a|bc" N "" N "" N "def");
    data.setText("abc" N "d|ef");
    KEYS("2[<Space>", "abc" N "" N "" N "d|ef");

    // ]e / [e: move the current line down/up.
    data.setText("a|bc" N "def" N "ghi");
    KEYS("]e", "def" N "a|bc" N "ghi");
    KEYS("[e", "a|bc" N "def" N "ghi");

    // Moving does not go past the first or last line.
    data.setText("a|bc" N "def");
    KEYS("[e", "a|bc" N "def");
    data.setText("abc" N "d|ef");
    KEYS("]e", "abc" N "d|ef");

    // With a count.
    data.setText("a|bc" N "def" N "ghi");
    KEYS("2]e", "def" N "ghi" N "a|bc");
    KEYS("2[e", "a|bc" N "def" N "ghi");
}

void FakeVimTester::test_vim_tagstack()
{
    TestData data;
    setup(&data);
    data.setText("one two three");

    // Verify how tag-stack input is routed to the handler callbacks. The stack
    // navigation itself needs a real file (openEditorAt) and is not exercised
    // here (QTCREATORBUG-11754).
    int jumps = 0;
    int distance = 0;
    data.handler->tagJumpRequested.set([&] { ++jumps; });
    data.handler->tagStackRequested.set([&](int d) { distance = d; });

    // CTRL-] and ":tag {name}" start a new jump.
    data.doKeys("<C-]>");
    QCOMPARE(jumps, 1);
    data.doCommand("tag foo");
    QCOMPARE(jumps, 2);

    // CTRL-T / ":pop" go back, and a count repeats; bare ":tag" goes forward.
    data.doKeys("<C-t>");
    QCOMPARE(distance, -1);
    data.doKeys("3<C-t>");
    QCOMPARE(distance, -3);
    data.doCommand("pop");
    QCOMPARE(distance, -1);
    data.doCommand("tag");
    QCOMPARE(distance, 1);
}

void FakeVimTester::test_vim_source_utf8()
{
    TestData data;
    setup(&data);

    // A sourced vimrc is decoded as UTF-8 (QTCREATORBUG-8776). Map Z to insert
    // U+00E9 (UTF-8 bytes 0xC3 0xA9); a wrong decoding would produce mojibake.
    QByteArray vimrc = "nnoremap Z a";
    vimrc += char(0xC3);
    vimrc += char(0xA9);
    vimrc += "<Esc>\n";
    QTemporaryFile rc;
    QVERIFY(rc.open());
    rc.write(vimrc);
    rc.flush();

    data.setText("|abc");
    data.doCommand("source " + rc.fileName());
    data.doKeys("Z");

    QString expected;
    expected += QLatin1Char('a');
    expected += QChar(0x00e9);
    expected += QLatin1String("bc");
    QCOMPARE(data.editor()->toPlainText(), expected);
}

void FakeVimTester::test_vim_insert_indent()
{
    TestData data;
    setup(&data);
    data.doCommand("set expandtab");
    data.doCommand("set shiftwidth=4");

    // In insert mode CTRL-T adds one shiftwidth of indent to the current line
    // and CTRL-D removes one (QTCREATORBUG-11912).
    data.setText("a|bc");
    KEYS("i<C-t>", "    |abc");
    KEYS("<C-t>", "        |abc");
    KEYS("<C-d>", "|    abc");
    KEYS("<C-d>", "|abc");
}

void FakeVimTester::test_vim_block_selection_to_eol()
{
    TestData data;
    setup(&data);

    // A block selection extended with '$' reaches the end of each (variable
    // length) line, not a fixed column (QTCREATORBUG-22192).
    data.setText("test line" N "  test line" N "abc" N "  test line" N "test line");
    data.doKeys("gg^");
    data.doKeys("<c-v>$4j");
    const Utils::MultiTextCursor mtc = data.editor()->multiTextCursor();
    QCOMPARE(mtc.cursors().size(), 5);
    for (const QTextCursor &c : mtc) {
        const QTextBlock b = c.document()->findBlock(c.selectionStart());
        QCOMPARE(c.selectionStart() - b.position(), 0);
        QCOMPARE(c.selectionEnd() - b.position(), int(b.length()) - 1);
    }
}

void FakeVimTester::test_vim_insert_map_with_quotes()
{
    TestData data;
    setup(&data);

    // A mapping whose expansion contains '"' must not be truncated as if the
    // quote started a comment (QTCREATORBUG-11617). Typing '"' inserts a pair
    // and leaves the cursor between them.
    data.doCommand("inoremap \" \"\"<esc>i");
    data.setText("|");
    KEYS("i\"", "\"" X "\"");
    data.doKeys("<esc>");
    data.doCommand("iunmap \"");

    // Same mapping, read from a sourced vimrc.
    QTemporaryFile rc;
    QVERIFY(rc.open());
    rc.write("inoremap \" \"\"<esc>i\n");
    rc.flush();
    data.setText("|");
    data.doCommand("source " + rc.fileName());
    KEYS("i\"", "\"" X "\"");
}

void FakeVimTester::test_vim_search_smartcase()
{
    TestData data;
    setup(&data);
    data.doCommand("set ignorecase");
    data.doCommand("set smartcase");

    // With 'smartcase', an uppercase letter in the pattern makes the search
    // case-sensitive (so a lowercase match is skipped), while an all-lowercase
    // pattern stays case-insensitive. I.e. smartcase refines ignorecase rather
    // than the reverse (QTCREATORBUG-11758).
    data.setText("|xxx foo xxx Foo xxx");
    KEYS("/Foo<cr>", "xxx foo xxx " X "Foo xxx");
    data.setText("|xxx foo xxx Foo xxx");
    KEYS("/foo<cr>", "xxx " X "foo xxx Foo xxx");

    // Without 'smartcase', 'ignorecase' alone is always case-insensitive.
    data.doCommand("set nosmartcase");
    data.setText("|xxx foo xxx Foo xxx");
    KEYS("/Foo<cr>", "xxx " X "foo xxx Foo xxx");
    // The options are shared with every other test.
    data.doCommand("set noignorecase");
}

void FakeVimTester::test_vim_replace_char_newline()
{
    TestData data;
    setup(&data);
    data.doCommand("set expandtab");
    data.doCommand("set shiftwidth=4");

    // r<CR> replaces the character with a line break; with 'autoindent' the
    // new line is indented like a normal insert-mode Enter would be, instead
    // of starting in the first column (QTCREATORBUG-21835).
    data.setText("    abc" X "def");
    KEYS("r<CR>", "    abc" N "    " X "ef");

    // Without auto-/smart-indent the new line stays in the first column
    // (plain Vim behavior).
    data.doCommand("set noautoindent");
    data.doCommand("set nosmartindent");
    data.setText("    abc" X "def");
    KEYS("r<CR>", "    abc" N X "ef");
}

void FakeVimTester::test_vim_backspace_option()
{
    TestData data;
    setup(&data);

    // The default 'backspace' (indent,eol,start) lets <BS> in insert mode
    // remove text that is already present.
    data.setText("ab" X "cd");
    KEYS("i<BS>", "a" X "cd");

    // An empty 'backspace' is Vi compatible: <BS> cannot remove text that was
    // not entered during the current insert.
    data.doKeys("<ESC>");
    data.doCommand("set bs=");
    data.setText("ab" X "cd");
    KEYS("i<BS>", "ab" X "cd");

    // The numeric form bs=2 is equivalent to indent,eol,start and must not
    // leave backspace unable to delete existing text (QTCREATORBUG-6640).
    data.doKeys("<ESC>");
    data.doCommand("set bs=2");
    data.setText("ab" X "cd");
    KEYS("i<BS>", "a" X "cd");
}

void FakeVimTester::test_vim_fold_toggle_all()
{
    TestData data;
    setup(&data);
    data.setText("abc");

    // "zi" toggles folding for the whole document (QTCREATORBUG-11753). The
    // fold effect needs a real folded document; here we only check the routing.
    int toggles = 0;
    data.handler->foldToggleAll.set([&] { ++toggles; });
    data.doKeys("zi");
    QCOMPARE(toggles, 1);
    data.doKeys("zi");
    QCOMPARE(toggles, 2);
}

void FakeVimTester::test_vim_visual_selection_focus_out()
{
    // Visual-char selection is inclusive of the character under the cursor.
    // While the editor is focused FakeVim paints that character with a block
    // cursor and keeps the text-cursor selection exclusive, but when focus
    // leaves (e.g. when opening Advanced Find) the selection must be extended
    // so external consumers get the whole text (QTCREATORBUG-22207).
    FvBoolAspect &useFakeVim = FakeVim::Internal::settings().useFakeVim;
    const bool savedUseFakeVim = useFakeVim.value();
    useFakeVim.setValue(true);

    TestData data;
    setup(&data);
    data.editor()->show();
    data.setText("test");
    data.doKeys("ve"); // visual-select the whole word

    QFocusEvent focusOut(QEvent::FocusOut);
    QApplication::sendEvent(data.editor(), &focusOut);
    const QString selected = data.editor()->textCursor().selectedText();

    useFakeVim.setValue(savedUseFakeVim);

    QCOMPARE(selected, QString("test"));
}

void FakeVimTester::test_vim_reflow()
{
    TestData data;
    setup(&data);
    data.doCommand("set textwidth=10");

    // gqq reflows the current line to the text width.
    data.setText("one two three four five six");
    KEYS("gqq", X "one two" N "three four" N "five six");

    // The indentation of the first line is kept for every wrapped line.
    data.setText("  alpha beta gamma delta");
    KEYS("gqq", "  " X "alpha" N "  beta" N "  gamma" N "  delta");

    // gq with a motion reflows the spanned lines as one paragraph.
    data.setText("aaa bbb ccc" N "ddd");
    KEYS("gqj", X "aaa bbb" N "ccc ddd");

    // Blank lines separate paragraphs and are preserved.
    data.setText("aaaa bbbb cccc" N "" N "dddd eeee ffff");
    KEYS("VGgq", X "aaaa bbbb" N "cccc" N "" N "dddd eeee" N "ffff");

    // A word longer than the text width still gets its own line.
    data.setText("hi supercalifragilistic bye");
    KEYS("gqq", X "hi" N "supercalifragilistic" N "bye");

    // gw reflows like gq but keeps the cursor where it was (gq moves it to
    // the start of the reflowed text).
    data.setText("one |two three four five six");
    KEYS("gww", "one |two" N "three four" N "five six");

    data.setText("aaa |bbb ccc" N "ddd");
    KEYS("gwj", "aaa |bbb" N "ccc ddd");
}

void FakeVimTester::test_vim_open_line_with_fold()
{
    TestData data;
    setup(&data);
    data.doCommand("set noautoindent");

    // Realize the editor so the document layout runs against real geometry;
    // otherwise folding does not affect the visual line numbering and the bug
    // below cannot be reproduced.
    data.editor()->resize(600, 400);
    data.editor()->show();

    // Fold the inner "if" block through FakeVim (keeps the handler in sync),
    // then move down onto "ccc;", which is now right below the folded region.
    data.setText("{" N "if (x) {" N "|aaa;" N "bbb;" N "}" N "ccc;" N "}");
    data.doKeys("zc");
    data.doKeys("j");

    // Wait until the layout has caught up with the fold: the hidden blocks
    // pull the visual line number of "ccc;" (block 5) below its block number.
    const QTextDocument *doc = data.editor()->document();
    QTRY_VERIFY(doc->findBlockByNumber(5).firstLineNumber() < 5);
    QVERIFY(!doc->findBlockByNumber(2).isVisible());

    // "o" must insert the new line after "ccc;", not inside the folded region
    // (QTCREATORBUG-24005).
    KEYS("oz",
         "{" N "if (x) {" N "aaa;" N "bbb;" N "}" N "ccc;" N "z" X N "}");
}

void FakeVimTester::test_vim_scroll_center_on_scroll()
{
    TestData data;
    setup(&data);

    // Realize the editor so the viewport has a real size and actually scrolls.
    data.editor()->resize(600, 400);
    data.editor()->show();
    // The bug only shows with the editor Center-cursor-on-scroll option enabled:
    // FakeVim scrolling relied on ensureCursorVisible aligning the line to the
    // top, but centering overrode that (QTCREATORBUG-15407, QTCREATORBUG-9516).
    data.editor()->setCenterOnScroll(true);

    QByteArray text;
    for (int i = 1; i <= 200; ++i)
        text += QByteArray("line ") + QByteArray::number(i) + '\n';
    data.setText(text.constData());

    const int visibleLines = data.editor()->viewport()->height()
                             / data.editor()->fontMetrics().lineSpacing();
    QVERIFY(visibleLines > 8);

    // Returns how far below the top of the viewport the cursor line (100) sits
    // after jumping to it and issuing the given z scroll command.
    const auto cursorRow = [&](const QByteArray &keys) {
        const QByteArray cmd = QByteArray("100G") + keys;
        data.doKeys("gg");
        data.doKeys(cmd.constData());
        const int first = data.editor()->cursorForPosition(QPoint(0, 0)).blockNumber();
        return 99 - first; // line 100 is block 99 (0-based)
    };

    // zt puts the line at the top, zz centers it, zb puts it at the bottom.
    // Without the fix, centering forced all three to the middle.
    QVERIFY(cursorRow("zt") <= visibleLines / 4);
    const int zzRow = cursorRow("zz");
    QVERIFY(zzRow > visibleLines / 4 && zzRow < visibleLines * 3 / 4);
    QVERIFY(cursorRow("zb") >= visibleLines * 3 / 4);
}

void FakeVimTester::test_vim_tab_with_zero_tabstop()
{
    // A stray "tabstop=0" (e.g. from a hand-edited settings file) must not
    // crash with a division by zero when Tab is pressed in insert mode
    // (QTCREATORBUG-29376).
    auto &ts = FakeVim::Internal::settings().tabStop;
    auto &et = FakeVim::Internal::settings().expandTab;
    const qint64 savedTs = ts.value();
    const bool savedEt = et.value();
    ts.setValue(0);
    et.setValue(true);

    TestData data;
    setup(&data);
    data.setText("|abc");
    data.doKeys("i<tab>"); // must not crash
    const QString text = data.editor()->toPlainText();

    ts.setValue(savedTs);
    et.setValue(savedEt);

    // With tabstop treated as 1, Tab expands to a single space.
    QCOMPARE(text, QString(" abc"));
}

void FakeVimTester::test_vim_timeout_options()
{
    // The mapping timeout is configurable via the Vim "timeout"/"timeoutlen"
    // options, so an ambiguous prefix (e.g. a mapping starting with "g") need
    // not stall for a full second (QTCREATORBUG-29162).
    auto &timeout = FakeVim::Internal::settings().timeout;
    auto &timeoutlen = FakeVim::Internal::settings().timeoutlen;
    const bool savedTimeout = timeout.value();
    const qint64 savedLen = timeoutlen.value();

    TestData data;
    setup(&data);

    data.doCommand("set timeoutlen=250");
    QCOMPARE(timeoutlen.value(), qint64(250));
    data.doCommand("set tm=400"); // short name
    QCOMPARE(timeoutlen.value(), qint64(400));

    data.doCommand("set notimeout");
    QCOMPARE(timeout.value(), false);
    data.doCommand("set timeout");
    QCOMPARE(timeout.value(), true);

    timeout.setValue(savedTimeout);
    timeoutlen.setValue(savedLen);
}

void FakeVimTester::test_vim_selection_for_shortcut()
{
    // The bug only shows while the editor is focused, so route the event
    // through the filter as the application does.
    FvBoolAspect &useFakeVim = FakeVim::Internal::settings().useFakeVim;
    const bool savedUseFakeVim = useFakeVim.value();
    useFakeVim.setValue(true);

    TestData data;
    setup(&data);
    data.editor()->setFocus();
    if (!data.editor()->hasFocus()) {
        useFakeVim.setValue(savedUseFakeVim);
        QSKIP("Editor did not get keyboard focus");
    }

    data.setText("|abc def");
    data.doKeys("viw");
    // While focused the selection is one character short of the inclusive
    // Vim selection (the last char is under the block cursor).
    QCOMPARE(data.editor()->textCursor().selectedText(), QString("ab"));

    // A pass-through shortcut (Ctrl+Shift+F is Advanced Find) makes it
    // inclusive so the shortcut action sees the whole word
    // (QTCREATORBUG-27442).
    QKeyEvent ev(QEvent::ShortcutOverride, Qt::Key_F,
                 Qt::ControlModifier | Qt::ShiftModifier, QString());
    data.handler->eventFilter(data.editor(), &ev);
    QCOMPARE(data.editor()->textCursor().selectedText(), QString("abc"));

    useFakeVim.setValue(savedUseFakeVim);
}

void FakeVimTester::test_vim_shortcut_override_text_key()
{
    // Route the ShortcutOverride through the filter as the application does.
    FvBoolAspect &useFakeVim = FakeVim::Internal::settings().useFakeVim;
    const bool savedUseFakeVim = useFakeVim.value();
    useFakeVim.setValue(true);

    TestData data;
    setup(&data);
    data.editor()->setFocus();
    if (!data.editor()->hasFocus()) {
        useFakeVim.setValue(savedUseFakeVim);
        QSKIP("Editor did not get keyboard focus");
    }
    data.setText("|abc");

    // A plain text key must be claimed as input (accepted) so it is delivered
    // as a key press rather than eaten by the shortcut machinery; this is what
    // lets layout-modifier characters through (QTCREATORBUG-24904).
    QKeyEvent textKey(QEvent::ShortcutOverride, Qt::Key_X, Qt::NoModifier, QString("x"));
    textKey.setAccepted(false);
    data.handler->eventFilter(data.editor(), &textKey);
    QVERIFY(textKey.isAccepted());

    // A function key is a genuine shortcut and must be left unaccepted.
    QKeyEvent functionKey(QEvent::ShortcutOverride, Qt::Key_F5, Qt::NoModifier, QString());
    functionKey.setAccepted(false);
    data.handler->eventFilter(data.editor(), &functionKey);
    QVERIFY(!functionKey.isAccepted());

    useFakeVim.setValue(savedUseFakeVim);
}

void FakeVimTester::test_vim_jumplist_across_files()
{
    TestData data;
    setup(&data);
    data.setText("abc" N "def" N "ghi");

    int distance = 0;
    data.handler->navigateHistoryRequested.set([&](int d) { distance += d; });

    // Once the buffer-local jump list is exhausted, CTRL-O falls back to the
    // global navigation history so it can cross files (QTCREATORBUG-12114).
    // A large count outruns any local jumps.
    data.doKeys("100<c-o>");
    QVERIFY(distance < 0);

    // CTRL-I likewise continues forward through the global history.
    distance = 0;
    data.doKeys("100<c-i>");
    QVERIFY(distance > 0);
}

void FakeVimTester::test_vim_control_modifier()
{
    // A key with the Control modifier (and no Alt) must not be taken as the
    // plain letter command; such keys are left for a Qt Creator shortcut
    // (QTCREATORBUG-14369). Ctrl-Shift-X must not act like "X" (delete the
    // character before the cursor).
    TestData data;
    setup(&data);
    data.setText("ab|c");
    KEYS("<C-S-X>", "ab|c"); // no-op
    KEYS("<C-X>", "ab|c");   // no-op
    KEYS("X", "a|c");        // plain X deletes the character before the cursor
}

void FakeVimTester::test_vim_tabstop_distance()
{
    // FakeVim renders a tab as tabstop space-widths, so that tab-, space- and
    // autoindent-indented lines line up (QTCREATORBUG-10367).
    auto &tabStop = FakeVim::Internal::settings().tabStop;
    const qint64 savedTabStop = tabStop.value();

    TestData data;
    setup(&data);
    const int spaceWidth = data.editor()->fontMetrics().horizontalAdvance(' ');

    tabStop.setValue(4);
    data.handler->setupWidget(); // re-applies the visual tab width
    QCOMPARE(int(data.editor()->tabStopDistance()), spaceWidth * 4);

    tabStop.setValue(8);
    data.handler->setupWidget();
    QCOMPARE(int(data.editor()->tabStopDistance()), spaceWidth * 8);

    tabStop.setValue(savedTabStop);
}

void FakeVimTester::test_vim_goto_definition()
{
    // "gd" requests Follow Symbol natively, without an ex-command mapping
    // (QTCREATORBUG-27191).
    TestData data;
    setup(&data);
    data.setText("abc" N "d|ef");
    bool requested = false;
    data.handler->tagJumpRequested.set([&] { requested = true; });
    data.doKeys("gd");
    QVERIFY(requested);
}

void FakeVimTester::test_vim_context_help()
{
    // K requests context help for the symbol under the cursor.
    TestData data;
    setup(&data);
    data.setText("ab|c");
    bool requested = false;
    data.handler->contextHelpRequested.set([&] { requested = true; });
    data.doKeys("K");
    QVERIFY(requested);
}

void FakeVimTester::test_vim_alternate_file()
{
    // CTRL-^ edits the alternate file.
    TestData data;
    setup(&data);
    data.setText("ab|c");
    bool requested = false;
    data.handler->alternateFileRequested.set([&] { requested = true; });
    data.doKeys("<c-^>");
    QVERIFY(requested);
}

void FakeVimTester::test_vim_tag_text_object()
{
    TestData data;
    setup(&data);

    // "it"/"at" on inline tags.
    data.setText("<a>fo" X "o</a>");
    KEYS("dit", "<a>" X "</a>");

    data.setText("x<a>fo" X "o</a>y");
    KEYS("dat", "x" X "y");

    // The cursor may sit on the opening tag.
    data.setText("<" X "a>foo</a>");
    KEYS("dit", "<a>" X "</a>");

    // Nested tags: innermost by default, count selects the outer level.
    data.setText("<a><b>" X "x</b></a>");
    KEYS("dit", "<a><b>" X "</b></a>");

    data.setText("<a><b>" X "x</b></a>");
    KEYS("2dit", "<a>" X "</a>");

    data.setText("<a><b>" X "x</b></a>");
    KEYS("dat", "<a>" X "</a>");

    // Attributes (with ">"-containing values) and self-closing siblings.
    data.setText("<a title=\"1>2\">y" X "o</a>");
    KEYS("dit", "<a title=\"1>2\">" X "</a>");

    data.setText("<a>f" X "<br/>g</a>");
    KEYS("dit", "<a>" X "</a>");
}

void FakeVimTester::test_vim_script_echo_expression()
{
    // ":echo" evaluates a Vimscript expression.
    TestData data;
    setup(&data);
    QString message;
    data.handler->commandBufferChanged.set(
        [&](const QString &msg, int, int, int) { message = msg; });

    auto echo = [&](const char *expr) -> QString {
        message.clear();
        data.doCommand(QLatin1String("echo ") + QLatin1String(expr));
        return message;
    };

    QCOMPARE(echo("1 + 2"), QLatin1String("3"));
    QCOMPARE(echo("2 * 3 + 1"), QLatin1String("7"));
    QCOMPARE(echo("(1 + 2) * 3"), QLatin1String("9"));
    QCOMPARE(echo("10 % 3"), QLatin1String("1"));
    QCOMPARE(echo("7 / 2"), QLatin1String("3"));               // integer division
    QCOMPARE(echo("-5 + 2"), QLatin1String("-3"));
    QCOMPARE(echo("\"ab\" . \"cd\""), QLatin1String("abcd"));  // concatenation
    QCOMPARE(echo("\"3\" + 4"), QLatin1String("7"));           // string to number
    QCOMPARE(echo("3 > 2"), QLatin1String("1"));               // comparison -> 1/0
    QCOMPARE(echo("3 < 2"), QLatin1String("0"));
    QCOMPARE(echo("\"a\" == \"a\""), QLatin1String("1"));
    QCOMPARE(echo("1 && 0"), QLatin1String("0"));
    QCOMPARE(echo("1 || 0"), QLatin1String("1"));
    QCOMPARE(echo("!0"), QLatin1String("1"));
    QCOMPARE(echo("1 ? \"y\" : \"n\""), QLatin1String("y"));
    QCOMPARE(echo("0 ? \"y\" : \"n\""), QLatin1String("n"));
    QCOMPARE(echo("\"a\" \"b\""), QLatin1String("a b"));       // multiple arguments
    QCOMPARE(echo("'it''s'"), QLatin1String("it's"));          // single-quote escape
}

void FakeVimTester::test_vim_script_variables()
{
    // ":let"/":unlet" and variable references.
    TestData data;
    setup(&data);
    QString message;
    data.handler->commandBufferChanged.set(
        [&](const QString &msg, int, int, int) { message = msg; });
    auto echo = [&](const char *expr) -> QString {
        message.clear();
        data.doCommand(QLatin1String("echo ") + QLatin1String(expr));
        return message;
    };

    data.doCommand("let g:x = 1 + 2");
    QCOMPARE(echo("g:x"), QLatin1String("3"));
    QCOMPARE(echo("x"), QLatin1String("3"));          // g: is the same as unscoped

    data.doCommand("let x = 10");                     // unscoped writes global
    QCOMPARE(echo("g:x"), QLatin1String("10"));

    data.doCommand("let g:x += 5");
    QCOMPARE(echo("g:x"), QLatin1String("15"));

    data.doCommand("let s = \"foo\"");
    data.doCommand("let s .= \"bar\"");
    QCOMPARE(echo("s"), QLatin1String("foobar"));

    data.doCommand("let b:local = 7");
    QCOMPARE(echo("b:local * 2"), QLatin1String("14"));

    QCOMPARE(echo("v:true"), QLatin1String("1"));
    QCOMPARE(echo("v:false"), QLatin1String("0"));

    // Undefined variable is reported as an error.
    QCOMPARE(echo("nosuchvar"), QLatin1String("E121: Undefined variable: nosuchvar"));

    data.doCommand("unlet g:x");
    QCOMPARE(echo("x"), QLatin1String("E121: Undefined variable: x"));
}

void FakeVimTester::test_vim_script_options_registers()
{
    // "&option", "@register" and "$env" in expressions and :let
    TestData data;
    setup(&data);
    QString message;
    data.handler->commandBufferChanged.set(
        [&](const QString &msg, int, int, int) { message = msg; });
    auto echo = [&](const char *expr) -> QString {
        message.clear();
        data.doCommand(QLatin1String("echo ") + QLatin1String(expr));
        return message;
    };

    // Options: write via long name, read via short name and vice versa.
    data.doCommand("let &shiftwidth = 4");
    QCOMPARE(echo("&sw"), QLatin1String("4"));
    data.doCommand("let &sw += 3");
    QCOMPARE(echo("&shiftwidth"), QLatin1String("7"));
    data.doCommand("let &expandtab = 1");
    QCOMPARE(echo("&et"), QLatin1String("1"));

    // Registers.
    data.doCommand("let @a = \"hello\"");
    QCOMPARE(echo("@a"), QLatin1String("hello"));
    data.doCommand("let @a .= \" world\"");
    QCOMPARE(echo("@a"), QLatin1String("hello world"));

    // Environment variables.
    data.doCommand("let $FVTEST = \"xyz\"");
    QCOMPARE(echo("$FVTEST"), QLatin1String("xyz"));
    QCOMPARE(echo("\"v=\" . $FVTEST"), QLatin1String("v=xyz"));
}

void FakeVimTester::test_vim_script_builtins()
{
    // Builtin function calls in expressions.
    TestData data;
    setup(&data);
    QString message;
    data.handler->commandBufferChanged.set(
        [&](const QString &msg, int, int, int) { message = msg; });
    auto echo = [&](const char *expr) -> QString {
        message.clear();
        data.doCommand(QLatin1String("echo ") + QLatin1String(expr));
        return message;
    };

    QCOMPARE(echo("strlen(\"hello\")"), QLatin1String("5"));
    QCOMPARE(echo("len(\"abc\")"), QLatin1String("3"));
    QCOMPARE(echo("toupper(\"aB\")"), QLatin1String("AB"));
    QCOMPARE(echo("tolower(\"aB\")"), QLatin1String("ab"));
    QCOMPARE(echo("str2nr(\"42\")"), QLatin1String("42"));
    QCOMPARE(echo("str2nr(\"ff\", 16)"), QLatin1String("255"));
    QCOMPARE(echo("abs(-7)"), QLatin1String("7"));
    QCOMPARE(echo("empty(\"\")"), QLatin1String("1"));
    QCOMPARE(echo("empty(\"x\")"), QLatin1String("0"));
    QCOMPARE(echo("string(\"foo\")"), QLatin1String("'foo'"));
    QCOMPARE(echo("string(12)"), QLatin1String("12"));
    QCOMPARE(echo("has(\"gui_running\")"), QLatin1String("0"));

    // Nested calls and expressions as arguments.
    QCOMPARE(echo("strlen(toupper(\"ab\")) + 1"), QLatin1String("3"));

    // exists() for variables, options and environment.
    data.doCommand("let g:defined = 1");
    QCOMPARE(echo("exists(\"g:defined\")"), QLatin1String("1"));
    QCOMPARE(echo("exists(\"g:undefined\")"), QLatin1String("0"));
    QCOMPARE(echo("exists(\"&sw\")"), QLatin1String("1"));
    QCOMPARE(echo("exists(\"&nosuchopt\")"), QLatin1String("0"));

    // exists("*name") asks whether a function can be called. Expected values
    // taken from Vim 9.1.
    data.doCommand("function MyFunc() | endfunction");
    data.doCommand("let g:Ref = function('MyFunc')");
    QCOMPARE(echo("exists('*matchstr')"), QLatin1String("1")); // builtin
    QCOMPARE(echo("exists('*NoSuchFuncHere')"), QLatin1String("0"));
    QCOMPARE(echo("exists('*MyFunc')"), QLatin1String("1"));
    QCOMPARE(echo("exists('*g:MyFunc')"), QLatin1String("1"));
    QCOMPARE(echo("exists('*Ref')"), QLatin1String("1")); // a variable holding one
    QCOMPARE(echo("exists('?matchstr')"), QLatin1String("1"));
    QCOMPARE(echo("exists('*strftime')"), QLatin1String("1"));

    // "&&" and "||" stop once the answer cannot change, so the other side is
    // neither worked out nor called. This is what lets the guard every plugin
    // writes ask about something that may not be there. Values from Vim 9.1.
    data.doCommand("function Bump() | let g:calls += 1 | return 1 | endfunction");
    QCOMPARE(echo("exists('g:nope') && g:nope == 1"), QLatin1String("0"));
    data.doCommand("let g:calls = 0");
    QCOMPARE(echo("0 && Bump()"), QLatin1String("0"));
    QCOMPARE(echo("g:calls"), QLatin1String("0"));
    data.doCommand("let g:calls = 0");
    QCOMPARE(echo("1 || Bump()"), QLatin1String("1"));
    QCOMPARE(echo("g:calls"), QLatin1String("0"));
    // ... and is called when it does matter.
    data.doCommand("let g:calls = 0");
    QCOMPARE(echo("1 && Bump()"), QLatin1String("1"));
    QCOMPARE(echo("g:calls"), QLatin1String("1"));
    data.doCommand("unlet g:calls");

    // 'cpoptions' and putting an option back with "&", the dance nearly every
    // legacy plugin does around its own body. Values taken from Vim 9.1.
    QCOMPARE(echo("&cpo"), QLatin1String("aABceFsz"));
    data.doCommand("let g:savedCpo = &cpo");
    data.doCommand("set cpo=abc");
    QCOMPARE(echo("&cpo"), QLatin1String("abc"));
    data.doCommand("set cpo&vim");
    QCOMPARE(echo("&cpo"), QLatin1String("aABceFsz"));
    data.doCommand("set cpo=xyz");
    data.doCommand("set cpo&");
    QCOMPARE(echo("&cpo"), QLatin1String("aABceFsz"));
    // ... and restoring what was saved, which is how the dance ends.
    data.doCommand("set cpo=qqq");
    data.doCommand("let &cpo = g:savedCpo");
    QCOMPARE(echo("&cpo"), QLatin1String("aABceFsz"));
    data.doCommand("unlet g:savedCpo");

    // "+" on lists puts one after the other, which is how a script grows a list
    // an item at a time. Values taken from Vim 9.1.
    data.doCommand("let g:lc = [1]");
    data.doCommand("let g:lc += [2, 3]");
    QCOMPARE(echo("g:lc"), QLatin1String("[1, 2, 3]"));
    QCOMPARE(echo("[1, 2] + [3]"), QLatin1String("[1, 2, 3]"));
    // ... including from nothing, where a numeric reading would give zero.
    data.doCommand("let g:lc = []");
    data.doCommand("let g:lc += [1]");
    QCOMPARE(echo("g:lc"), QLatin1String("[1]"));
    data.doCommand("unlet g:lc");

    // ":echohl" only picks a colour, so it is passed over rather than failing
    // and taking the rest of a script with it.
    data.doCommand("echohl WarningMsg");
    data.doCommand("echohl None");
    QCOMPARE(echo("'still here'"), QLatin1String("still here"));

    // deepcopy() shares nothing with the original, where copy() shares what is
    // nested. Values taken from Vim 9.1.
    data.doCommand("let g:dcA = [[1, 2], {'k': [3]}]");
    data.doCommand("let g:dcB = deepcopy(g:dcA)");
    data.doCommand("let g:dcB[0][0] = 99");
    QCOMPARE(echo("g:dcA[0][0] . ',' . g:dcB[0][0]"), QLatin1String("1,99"));
    data.doCommand("let g:dcC = copy(g:dcA)");
    data.doCommand("let g:dcC[0][1] = 77");
    QCOMPARE(echo("g:dcA[0][1]"), QLatin1String("77"));
    data.doCommand("unlet g:dcA | unlet g:dcB | unlet g:dcC");

    QCOMPARE(echo("executable('sh')"), QLatin1String("1"));
    QCOMPARE(echo("executable('definitely_no_such_cmd_xyz')"), QLatin1String("0"));
    QCOMPARE(echo("substitute(system('echo hi'), '\\n', '', 'g')"), QLatin1String("hi"));
    QCOMPARE(echo("iconv('abc', 'utf-8', 'utf-8')"), QLatin1String("abc"));
    // An encoding that cannot be had leaves the string as it was.
    QCOMPARE(echo("iconv('abc', 'no-such-enc', 'utf-8')"), QLatin1String("abc"));

    // The view can be noted and put back.
    data.setText("l1" N "l2" N "l3" N "l4");
    data.doCommand("call cursor(3, 2)");
    data.doCommand("let g:view = winsaveview()");
    QCOMPARE(echo("has_key(g:view, 'lnum') && has_key(g:view, 'col')"
                  " && has_key(g:view, 'topline')"), QLatin1String("1"));
    data.doCommand("call cursor(1, 1)");
    data.doCommand("call winrestview(g:view)");
    QCOMPARE(echo("[line('.'), col('.')]"), QLatin1String("[3, 2]"));
    data.doCommand("unlet g:view");

    // matchlist() gives the whole match and the nine groups, padded out, and
    // nothing at all when the pattern does not match. A "\=" replacement is an
    // expression worked out per match, where submatch() reaches the pieces.
    // Values taken from Vim 9.1.
    QCOMPARE(echo("matchlist('foo123bar', '\\v(\\a+)(\\d+)(\\a+)')"),
             QLatin1String("['foo123bar', 'foo', '123', 'bar', '', '', '', '', '', '']"));
    QCOMPARE(echo("matchlist('xyz', '\\d\\+')"), QLatin1String("[]"));
    QCOMPARE(echo("matchlist('abc', 'b')"),
             QLatin1String("['b', '', '', '', '', '', '', '', '', '']"));
    QCOMPARE(echo("substitute('foo42', '\\v(\\a+)(\\d+)', "
                  "'\\=submatch(2) . submatch(1)', '')"), QLatin1String("42foo"));
    QCOMPARE(echo("substitute('ab', '\\a', '\\=submatch(0) . \"-\"', 'g')"),
             QLatin1String("a-b-"));

    // Path functions. Values taken from Vim 9.1.
    QCOMPARE(echo("fnamemodify('a/b/file.txt', ':h')"), QLatin1String("a/b"));
    QCOMPARE(echo("fnamemodify('a/b/file.txt', ':t')"), QLatin1String("file.txt"));
    QCOMPARE(echo("fnamemodify('a/b/file.txt', ':r')"), QLatin1String("a/b/file"));
    QCOMPARE(echo("fnamemodify('a/b/file.txt', ':e')"), QLatin1String("txt"));
    // The modifiers apply one after another.
    QCOMPARE(echo("fnamemodify('a/b/file.tar.gz', ':t:r')"), QLatin1String("file.tar"));
    QCOMPARE(echo("fnameescape('a b')"), QLatin1String("a\\ b"));
    QCOMPARE(echo("shellescape('a b')"), QLatin1String("'a b'"));
    QCOMPARE(echo("strlen(getcwd()) > 0"), QLatin1String("1"));
    {
        // A directory and a file that are really there.
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        QVERIFY(QDir(dir.path()).mkpath("sub"));
        QFile f(dir.path() + "/file.txt");
        QVERIFY(f.open(QIODevice::WriteOnly));
        f.write("x\n");
        f.close();
        // Globals are shared between the handlers, so use a name of our own
        // and take it away again.
        data.doCommand("let g:pathProbe = '" + dir.path() + "'");
        QCOMPARE(echo("filereadable(g:pathProbe . '/file.txt')"), QLatin1String("1"));
        QCOMPARE(echo("filereadable(g:pathProbe . '/nope.txt')"), QLatin1String("0"));
        QCOMPARE(echo("isdirectory(g:pathProbe . '/sub')"), QLatin1String("1"));
        // A file is not a directory, and the other way round.
        QCOMPARE(echo("isdirectory(g:pathProbe . '/file.txt')"), QLatin1String("0"));
        QCOMPARE(echo("filereadable(g:pathProbe . '/sub')"), QLatin1String("0"));
        QCOMPARE(echo("fnamemodify(g:pathProbe . '/file.txt', ':t')"),
                 QLatin1String("file.txt"));
        data.doCommand("unlet g:pathProbe");
    }

    // Buffer variables. Only the buffer in hand is reachable here, which is
    // what a script asking about its own needs. Values taken from Vim 9.1.
    data.doCommand("let b:mine = 'hello'");
    QCOMPARE(echo("bufnr('%') > 0"), QLatin1String("1"));
    QCOMPARE(echo("getbufvar('%', 'mine')"), QLatin1String("hello"));
    QCOMPARE(echo("getbufvar('%', 'nosuch', 'fallback')"), QLatin1String("fallback"));
    // The name is taken as it stands, so "b:mine" is a different name.
    QCOMPARE(echo("'[' . getbufvar('%', 'b:mine') . ']'"), QLatin1String("[]"));
    // Nothing there and nothing offered instead reads as empty, not as zero.
    QCOMPARE(echo("'[' . getbufvar('%', 'nosuch') . ']'"), QLatin1String("[]"));
    data.doCommand("call setbufvar('%', 'other', 'world')");
    QCOMPARE(echo("b:other"), QLatin1String("world"));
    // A "&name" reads the option; compare against it rather than a number,
    // since the other tests leave it wherever they left it.
    QCOMPARE(echo("getbufvar('%', '&shiftwidth') == &shiftwidth"), QLatin1String("1"));
    // A buffer that cannot be reached reads as the default.
    QCOMPARE(echo("getbufvar('zzz_no_such', 'mine', 'def')"), QLatin1String("def"));

    // strftime(). What a conversion turns into depends on the time zone, so
    // check the shape rather than a fixed moment.
    QCOMPARE(echo("strftime('100%%')"), QLatin1String("100%"));
    QCOMPARE(echo("strftime('literal')"), QLatin1String("literal"));
    QCOMPARE(echo("strlen(strftime('%Y-%m-%d'))"), QLatin1String("10"));
    QCOMPARE(echo("strftime('%Y-%m-%d %H:%M:%S', 0) "
                  "=~ '^\\d\\d\\d\\d-\\d\\d-\\d\\d \\d\\d:\\d\\d:\\d\\d$'"),
             QLatin1String("1"));
    // The turn of the millennium, whichever side of it the zone falls on. The
    // alternation needs "\v", since a bare "|" is literal in a magic pattern.
    QCOMPARE(echo("strftime('%Y-%m-%d', 946684800) "
                  "=~ '\\v^(1999-12-31|2000-01-01)$'"), QLatin1String("1"));
    QCOMPARE(echo("strlen(strftime('%a', 946684800))"), QLatin1String("3"));

    // Buffer-backed builtins.
    data.setText("one" N "two" N "three");
    QCOMPARE(echo("line(\"$\")"), QLatin1String("3"));
    QCOMPARE(echo("line(\".\")"), QLatin1String("1"));
    QCOMPARE(echo("col(\".\")"), QLatin1String("1"));
    QCOMPARE(echo("getline(2)"), QLatin1String("two"));
    QCOMPARE(echo("strlen(getline(3))"), QLatin1String("5"));
    data.doCommand("call setline(1, \"ONE\")");
    QCOMPARE(echo("getline(1)"), QLatin1String("ONE"));
}

void FakeVimTester::test_vim_script_positions()
{
    // line()/col() accept marks, plus getpos()/getcurpos()/setpos().
    TestData data;
    setup(&data);
    QString message;
    data.handler->commandBufferChanged.set(
        [&](const QString &msg, int, int, int) { message = msg; });
    auto echo = [&](const char *expr) -> QString {
        message.clear();
        data.doCommand(QLatin1String("echo ") + QLatin1String(expr));
        return message;
    };

    data.setText("one" N "two" N "three");
    // Set mark a on line 2, column 2, then come back to the start.
    data.doKeys("jlma");
    data.doKeys("gg0");

    QCOMPARE(echo("line(\"'a\")"), QLatin1String("2"));
    QCOMPARE(echo("col(\"'a\")"), QLatin1String("2"));
    QCOMPARE(echo("line(\"'z\")"), QLatin1String("0")); // unset mark
    QCOMPARE(echo("getpos(\"'a\")"), QLatin1String("[0, 2, 2, 0]"));
    QCOMPARE(echo("getcurpos()"), QLatin1String("[0, 1, 1, 0]"));

    // setpos() moves the cursor and defines marks.
    data.doCommand("call setpos(\".\", [0, 3, 2, 0])");
    QCOMPARE(echo("getpos(\".\")"), QLatin1String("[0, 3, 2, 0]"));
    data.doCommand("call setpos(\"'b\", [0, 1, 3, 0])");
    QCOMPARE(echo("line(\"'b\")"), QLatin1String("1"));
    QCOMPARE(echo("col(\"'b\")"), QLatin1String("3"));
}

void FakeVimTester::test_vim_script_operatorfunc()
{
    // "g@{motion}" calls &operatorfunc with "char"/"line"/"block" after
    // setting the '[ and '] marks to the region.
    TestData data;
    setup(&data);
    QString message;
    data.handler->commandBufferChanged.set(
        [&](const QString &msg, int, int, int) { message = msg; });
    auto echo = [&](const char *expr) -> QString {
        message.clear();
        data.doCommand(QLatin1String("echo ") + QLatin1String(expr));
        return message;
    };
    auto source = [&](const char *text) {
        QTemporaryFile file;
        QVERIFY(file.open());
        file.write(text);
        file.flush();
        data.doCommand(QLatin1String("source ") + file.fileName());
    };

    // Record how the function was called, without touching the buffer.
    source("function Record(kind)\n"
           "  let g:kind = a:kind\n"
           "  let g:from = [line(\"'[\"), col(\"'[\")]\n"
           "  let g:to = [line(\"']\"), col(\"']\")]\n"
           "endfunction\n");
    data.doCommand("set operatorfunc=Record");

    data.setText("one two three" N "second line");

    // Charwise: "g@w" covers "one " exclusively, so '] is on the space.
    data.doKeys("gg0g@w");
    QCOMPARE(echo("g:kind"), QLatin1String("char"));
    QCOMPARE(echo("g:from"), QLatin1String("[1, 1]"));
    QCOMPARE(echo("g:to"), QLatin1String("[1, 4]"));

    // Linewise: "g@j" spans both lines whole.
    data.doKeys("gg0g@j");
    QCOMPARE(echo("g:kind"), QLatin1String("line"));
    QCOMPARE(echo("g:from"), QLatin1String("[1, 1]"));
    QCOMPARE(echo("g:to"), QLatin1String("[2, 11]"));

    // A text object as the motion.
    data.doKeys("gg0wg@iw");
    QCOMPARE(echo("g:kind"), QLatin1String("char"));
    QCOMPARE(echo("g:from"), QLatin1String("[1, 5]"));
    QCOMPARE(echo("g:to"), QLatin1String("[1, 7]"));

    // Visual mode passes "char"/"line" for the selection.
    data.doKeys("gg0vll<Esc>");
    data.doKeys("gg0vllg@");
    QCOMPARE(echo("g:kind"), QLatin1String("char"));
    QCOMPARE(echo("g:from"), QLatin1String("[1, 1]"));
    QCOMPARE(echo("g:to"), QLatin1String("[1, 3]"));

    data.doKeys("ggVjg@");
    QCOMPARE(echo("g:kind"), QLatin1String("line"));
    QCOMPARE(echo("g:from"), QLatin1String("[1, 1]"));
    QCOMPARE(echo("g:to"), QLatin1String("[2, 11]"));

    // An operatorfunc that edits the buffer, the usual real-world case.
    source("function Upper(kind)\n"
           "  let l:n = line(\"'[\")\n"
           "  call setline(l:n, toupper(getline(l:n)))\n"
           "endfunction\n");
    data.doCommand("set operatorfunc=Upper");
    data.setText("hello" N "world");
    data.doKeys("gg0g@j");
    QCOMPARE(data.text(), QByteArray("HELLO" N "world"));

    // "." repeats the operator together with its motion.
    data.doCommand("function Bump(kind) | let g:calls += 1 | endfunction");
    data.doCommand("set operatorfunc=Bump");
    data.doCommand("let g:calls = 0");
    data.setText("aa bb cc");
    data.doKeys("gg0g@w");
    QCOMPARE(echo("g:calls"), QLatin1String("1"));
    data.doKeys(".");
    QCOMPARE(echo("g:calls"), QLatin1String("2"));

    // An empty 'operatorfunc' reports an error instead of crashing.
    data.doCommand("set operatorfunc=");
    data.doKeys("gg0g@w");
    QVERIFY(message.contains("operatorfunc"));
}

void FakeVimTester::test_vim_script_expr_mapping()
{
    // ":inoremap <expr>" evaluates its right-hand side on each use
    TestData data;
    setup(&data);

    data.doCommand("inoremap <expr> zz \"ab\" . \"cd\"");
    data.setText("");
    KEYS("izz<ESC>", "abc" X "d");

    // The expression is evaluated at trigger time, so it can change.
    data.doCommand("inoremap <expr> qq g:x");
    data.doCommand("let g:x = \"A\"");
    data.setText("");
    KEYS("iqq<ESC>", X "A");
    data.doCommand("let g:x = \"BB\"");
    data.setText("");
    KEYS("iqq<ESC>", "B" X "B");
}

void FakeVimTester::test_vim_script_execute()
{
    // ":execute" builds a command from expressions and runs it
    TestData data;
    setup(&data);
    QString message;
    data.handler->commandBufferChanged.set(
        [&](const QString &msg, int, int, int) { message = msg; });
    auto echo = [&](const char *expr) -> QString {
        message.clear();
        data.doCommand(QLatin1String("echo ") + QLatin1String(expr));
        return message;
    };

    data.doCommand("execute \"let g:x = 40 + 2\"");
    QCOMPARE(echo("g:x"), QLatin1String("42"));

    // Several expressions are joined with a space before running.
    data.doCommand("let g:cmd = \"let\"");
    data.doCommand("execute g:cmd \"g:y = 7\"");
    QCOMPARE(echo("g:y"), QLatin1String("7"));

    // :execute can run a command that produces output.
    message.clear();
    data.doCommand("execute \"echo \" . string(21 * 2)");
    QCOMPARE(message, QLatin1String("42"));
}

void FakeVimTester::test_vim_script_if()
{
    // ":if/:elseif/:else/:endif" control flow.
    TestData data;
    setup(&data);
    QString message;
    data.handler->commandBufferChanged.set(
        [&](const QString &msg, int, int, int) { message = msg; });
    auto echo = [&](const char *expr) -> QString {
        message.clear();
        data.doCommand(QLatin1String("echo ") + QLatin1String(expr));
        return message;
    };

    // Interactive bar-separated form.
    data.doCommand("if 1 | let g:a = 10 | else | let g:a = 20 | endif");
    QCOMPARE(echo("g:a"), QLatin1String("10"));
    data.doCommand("if 0 | let g:a = 10 | else | let g:a = 20 | endif");
    QCOMPARE(echo("g:a"), QLatin1String("20"));

    // elseif chain.
    data.doCommand("if 0 | let g:b = 1 | elseif 1 | let g:b = 2 | else | let g:b = 3 | endif");
    QCOMPARE(echo("g:b"), QLatin1String("2"));

    // Nesting.
    data.doCommand("if 1 | if 0 | let g:c = 1 | else | let g:c = 2 | endif | endif");
    QCOMPARE(echo("g:c"), QLatin1String("2"));

    // A skipped branch has no side effects.
    data.doCommand("if 0 | let g:d = 99 | endif");
    QCOMPARE(echo("g:d"), QLatin1String("E121: Undefined variable: g:d"));

    // Multi-line block from a sourced file.
    QTemporaryFile file;
    QVERIFY(file.open());
    file.write("let g:n = 3\n"
               "if g:n > 5\n"
               "  let g:size = \"big\"\n"
               "elseif g:n > 1\n"
               "  let g:size = \"medium\"\n"
               "else\n"
               "  let g:size = \"small\"\n"
               "endif\n");
    file.flush();
    data.doCommand(QLatin1String("source ") + file.fileName());
    QCOMPARE(echo("g:size"), QLatin1String("medium"));
}

void FakeVimTester::test_vim_script_while()
{
    // ":while/:endwhile" with :break and :continue.
    TestData data;
    setup(&data);
    QString message;
    data.handler->commandBufferChanged.set(
        [&](const QString &msg, int, int, int) { message = msg; });
    auto echo = [&](const char *expr) -> QString {
        message.clear();
        data.doCommand(QLatin1String("echo ") + QLatin1String(expr));
        return message;
    };

    data.doCommand("let g:i = 0 | let g:sum = 0 | while g:i < 5 | "
                   "let g:sum += g:i | let g:i += 1 | endwhile");
    QCOMPARE(echo("g:sum"), QLatin1String("10"));

    // A false condition runs the body zero times.
    data.doCommand("let g:z = 5 | while 0 | let g:z = 99 | endwhile");
    QCOMPARE(echo("g:z"), QLatin1String("5"));

    // :break leaves the loop.
    data.doCommand("let g:i = 0 | while 1 | let g:i += 1 | "
                   "if g:i >= 3 | break | endif | endwhile");
    QCOMPARE(echo("g:i"), QLatin1String("3"));

    // :continue skips the rest of the body (sum of even values in 1..6).
    data.doCommand("let g:i = 0 | let g:s = 0 | while g:i < 6 | let g:i += 1 | "
                   "if g:i % 2 | continue | endif | let g:s += g:i | endwhile");
    QCOMPARE(echo("g:s"), QLatin1String("12"));

    // Multi-line loop from a sourced file: 4! = 24.
    QTemporaryFile file;
    QVERIFY(file.open());
    file.write("let g:p = 1\n"
               "let g:k = 1\n"
               "while g:k <= 4\n"
               "  let g:p = g:p * g:k\n"
               "  let g:k += 1\n"
               "endwhile\n");
    file.flush();
    data.doCommand(QLatin1String("source ") + file.fileName());
    QCOMPARE(echo("g:p"), QLatin1String("24"));
}

void FakeVimTester::test_vim_script_lists()
{
    // List values: literals, indexing and list builtins.
    TestData data;
    setup(&data);
    QString message;
    data.handler->commandBufferChanged.set(
        [&](const QString &msg, int, int, int) { message = msg; });
    auto echo = [&](const char *expr) -> QString {
        message.clear();
        data.doCommand(QLatin1String("echo ") + QLatin1String(expr));
        return message;
    };

    QCOMPARE(echo("[1, 2, 3]"), QLatin1String("[1, 2, 3]"));
    QCOMPARE(echo("[1, \"a\", [2, 3]]"), QLatin1String("[1, 'a', [2, 3]]"));
    QCOMPARE(echo("[10, 20, 30][1]"), QLatin1String("20"));
    QCOMPARE(echo("[10, 20, 30][-1]"), QLatin1String("30"));    // negative index
    QCOMPARE(echo("len([1, 2, 3, 4])"), QLatin1String("4"));
    QCOMPARE(echo("empty([])"), QLatin1String("1"));
    QCOMPARE(echo("empty([0])"), QLatin1String("0"));
    QCOMPARE(echo("get([1, 2], 5, -1)"), QLatin1String("-1"));  // default
    QCOMPARE(echo("range(3)"), QLatin1String("[0, 1, 2]"));
    QCOMPARE(echo("range(2, 5)"), QLatin1String("[2, 3, 4, 5]"));
    QCOMPARE(echo("range(0, 6, 2)"), QLatin1String("[0, 2, 4, 6]"));
    QCOMPARE(echo("string([1, \"x\"])"), QLatin1String("[1, 'x']"));

    // String indexing.
    QCOMPARE(echo("\"hello\"[1]"), QLatin1String("e"));

    // add() mutates the shared list and can be observed through a variable.
    data.doCommand("let g:l = [1, 2]");
    data.doCommand("call add(g:l, 3)");
    QCOMPARE(echo("g:l"), QLatin1String("[1, 2, 3]"));
    QCOMPARE(echo("g:l[2]"), QLatin1String("3"));
}

void FakeVimTester::test_vim_script_for()
{
    // ":for x in list ... :endfor".
    TestData data;
    setup(&data);
    QString message;
    data.handler->commandBufferChanged.set(
        [&](const QString &msg, int, int, int) { message = msg; });
    auto echo = [&](const char *expr) -> QString {
        message.clear();
        data.doCommand(QLatin1String("echo ") + QLatin1String(expr));
        return message;
    };

    // Sum a literal list.
    data.doCommand("let g:sum = 0 | for x in [1, 2, 3, 4] | let g:sum += x | endfor");
    QCOMPARE(echo("g:sum"), QLatin1String("10"));

    // Iterate range() with :break.
    data.doCommand("let g:sum = 0 | for i in range(1, 100) | "
                   "if i > 5 | break | endif | let g:sum += i | endfor");
    QCOMPARE(echo("g:sum"), QLatin1String("15"));

    // :continue skips elements.
    data.doCommand("let g:s = 0 | for i in range(1, 6) | "
                   "if i % 2 | continue | endif | let g:s += i | endfor");
    QCOMPARE(echo("g:s"), QLatin1String("12"));

    // Build a list, then multi-line :for from a sourced file.
    QTemporaryFile file;
    QVERIFY(file.open());
    file.write("let g:out = []\n"
               "for w in [\"a\", \"b\", \"c\"]\n"
               "  call add(g:out, w)\n"
               "endfor\n");
    file.flush();
    data.doCommand(QLatin1String("source ") + file.fileName());
    QCOMPARE(echo("g:out"), QLatin1String("['a', 'b', 'c']"));
}

void FakeVimTester::test_vim_script_dicts()
{
    // Dictionary values: literals, indexing and dict builtins
    TestData data;
    setup(&data);
    QString message;
    data.handler->commandBufferChanged.set(
        [&](const QString &msg, int, int, int) { message = msg; });
    auto echo = [&](const char *expr) -> QString {
        message.clear();
        data.doCommand(QLatin1String("echo ") + QLatin1String(expr));
        return message;
    };

    // Keys are sorted in the repr, matching Vim's string().
    QCOMPARE(echo("{\"b\": 1, \"a\": 2}"), QLatin1String("{'a': 2, 'b': 1}"));
    QCOMPARE(echo("{\"x\": 10}[\"x\"]"), QLatin1String("10"));
    QCOMPARE(echo("len({\"a\": 1, \"b\": 2})"), QLatin1String("2"));
    QCOMPARE(echo("empty({})"), QLatin1String("1"));
    QCOMPARE(echo("has_key({\"a\": 1}, \"a\")"), QLatin1String("1"));
    QCOMPARE(echo("has_key({\"a\": 1}, \"z\")"), QLatin1String("0"));
    QCOMPARE(echo("keys({\"b\": 1, \"a\": 2})"), QLatin1String("['a', 'b']"));
    QCOMPARE(echo("values({\"b\": 1, \"a\": 2})"), QLatin1String("[2, 1]"));
    QCOMPARE(echo("get({\"a\": 1}, \"z\", -1)"), QLatin1String("-1"));
    QCOMPARE(echo("items({\"a\": 1})"), QLatin1String("[['a', 1]]"));

    // Nested containers.
    QCOMPARE(echo("{\"list\": [1, 2]}[\"list\"][1]"), QLatin1String("2"));

    // A dictionary is shared: remove() through a variable is observable.
    data.doCommand("let g:d = {\"a\": 1, \"b\": 2}");
    data.doCommand("call remove(g:d, \"a\")");
    QCOMPARE(echo("g:d"), QLatin1String("{'b': 2}"));
    QCOMPARE(echo("has_key(g:d, \"a\")"), QLatin1String("0"));
}

void FakeVimTester::test_vim_script_indexed_let()
{
    // ":let container[index] = value" for lists and dictionaries
    TestData data;
    setup(&data);
    QString message;
    data.handler->commandBufferChanged.set(
        [&](const QString &msg, int, int, int) { message = msg; });
    auto echo = [&](const char *expr) -> QString {
        message.clear();
        data.doCommand(QLatin1String("echo ") + QLatin1String(expr));
        return message;
    };

    // List element assignment, negative index and compound form.
    data.doCommand("let g:l = [1, 2, 3]");
    data.doCommand("let g:l[1] = 99");
    QCOMPARE(echo("g:l"), QLatin1String("[1, 99, 3]"));
    data.doCommand("let g:l[-1] = 0");
    QCOMPARE(echo("g:l"), QLatin1String("[1, 99, 0]"));
    data.doCommand("let g:l[0] += 10");
    QCOMPARE(echo("g:l"), QLatin1String("[11, 99, 0]"));

    // Dictionary: create keys, then a compound update.
    data.doCommand("let g:d = {}");
    data.doCommand("let g:d[\"a\"] = 1");
    data.doCommand("let g:d[\"b\"] = 2");
    QCOMPARE(echo("g:d"), QLatin1String("{'a': 1, 'b': 2}"));
    data.doCommand("let g:d[\"a\"] += 5");
    QCOMPARE(echo("g:d[\"a\"]"), QLatin1String("6"));

    // Nested container assignment.
    data.doCommand("let g:n = {\"xs\": [0, 0]}");
    data.doCommand("let g:n[\"xs\"][1] = 7");
    QCOMPARE(echo("g:n"), QLatin1String("{'xs': [0, 7]}"));

    // Build a dictionary in a loop.
    data.doCommand("let g:m = {} | for i in range(1, 3) | "
                   "let g:m[string(i)] = i * i | endfor");
    QCOMPARE(echo("g:m"), QLatin1String("{'1': 1, '2': 4, '3': 9}"));
}

void FakeVimTester::test_vim_script_functions()
{
    // User functions: :function/:endfunction, :call, :return, a:/l: scopes
    TestData data;
    setup(&data);
    QString message;
    data.handler->commandBufferChanged.set(
        [&](const QString &msg, int, int, int) { message = msg; });
    auto echo = [&](const char *expr) -> QString {
        message.clear();
        data.doCommand(QLatin1String("echo ") + QLatin1String(expr));
        return message;
    };
    auto source = [&](const char *text) {
        QTemporaryFile file;
        QVERIFY(file.open());
        file.write(text);
        file.flush();
        data.doCommand(QLatin1String("source ") + file.fileName());
    };

    // One-line definition (bar form), arguments and :return.
    data.doCommand("function Add(a, b) | return a:a + a:b | endfunction");
    QCOMPARE(echo("Add(2, 3)"), QLatin1String("5"));

    // l: locals; they do not leak to the global scope.
    data.doCommand("function Inc(n) | let l:r = a:n + 1 | return l:r | endfunction");
    QCOMPARE(echo("Inc(9)"), QLatin1String("10"));
    QCOMPARE(echo("exists('r')"), QLatin1String("0"));

    // Recursion.
    source("function Fact(n)\n"
           "  if a:n <= 1\n"
           "    return 1\n"
           "  endif\n"
           "  return a:n * Fact(a:n - 1)\n"
           "endfunction\n");
    QCOMPARE(echo("Fact(5)"), QLatin1String("120"));

    // A loop and :return inside a function.
    source("function Sum(list)\n"
           "  let l:s = 0\n"
           "  for x in a:list\n"
           "    let l:s += x\n"
           "  endfor\n"
           "  return l:s\n"
           "endfunction\n");
    QCOMPARE(echo("Sum([1, 2, 3, 4])"), QLatin1String("10"));

    // :call a user function for its side effects, reaching a global.
    data.doCommand("function Push(x) | call add(g:acc, a:x) | endfunction");
    data.doCommand("let g:acc = []");
    data.doCommand("call Push(7)");
    data.doCommand("call Push(8)");
    QCOMPARE(echo("g:acc"), QLatin1String("[7, 8]"));

    // Variadic functions: a:0 counts extras, a:1.. name them, a:000 is the list.
    data.doCommand("function Count(...) | return a:0 | endfunction");
    QCOMPARE(echo("Count(1, 2, 3)"), QLatin1String("3"));
    data.doCommand("function First(...) | return a:1 | endfunction");
    QCOMPARE(echo("First(9, 8)"), QLatin1String("9"));
    source("function Total(...)\n"
           "  let l:s = 0\n"
           "  for x in a:000\n"
           "    let l:s += x\n"
           "  endfor\n"
           "  return l:s\n"
           "endfunction\n");
    QCOMPARE(echo("Total(1, 2, 3, 4)"), QLatin1String("10"));
    // Named parameters followed by "...".
    data.doCommand("function Tag(name, ...) | return a:name . \":\" . a:0 | endfunction");
    QCOMPARE(echo("Tag(\"x\", 1, 2)"), QLatin1String("x:2"));
}

void FakeVimTester::test_vim_script_string_builtins()
{
    // String builtins: split, join, stridx, strpart, substitute, printf
    TestData data;
    setup(&data);
    QString message;
    data.handler->commandBufferChanged.set(
        [&](const QString &msg, int, int, int) { message = msg; });
    auto echo = [&](const char *expr) -> QString {
        message.clear();
        data.doCommand(QLatin1String("echo ") + QLatin1String(expr));
        return message;
    };

    QCOMPARE(echo("split(\"a b  c\")"), QLatin1String("['a', 'b', 'c']"));
    QCOMPARE(echo("split(\"a,b,c\", \",\")"), QLatin1String("['a', 'b', 'c']"));
    QCOMPARE(echo("join([\"a\", \"b\", \"c\"], \"-\")"), QLatin1String("a-b-c"));
    QCOMPARE(echo("join([1, 2, 3])"), QLatin1String("1 2 3"));
    QCOMPARE(echo("stridx(\"hello\", \"ll\")"), QLatin1String("2"));
    QCOMPARE(echo("stridx(\"hello\", \"z\")"), QLatin1String("-1"));
    QCOMPARE(echo("strpart(\"hello\", 1, 3)"), QLatin1String("ell"));

    // substitute: first match and global, with a backreference.
    QCOMPARE(echo("substitute(\"foo\", \"o\", \"0\", \"\")"), QLatin1String("f0o"));
    QCOMPARE(echo("substitute(\"foo\", \"o\", \"0\", \"g\")"), QLatin1String("f00"));
    QCOMPARE(echo("substitute(\"ab\", \"\\\\(.\\\\)\\\\(.\\\\)\", \"\\\\2\\\\1\", \"\")"),
             QLatin1String("ba"));

    // printf: width, zero-fill, left align, hex and strings.
    QCOMPARE(echo("printf(\"%d-%d\", 1, 2)"), QLatin1String("1-2"));
    QCOMPARE(echo("printf(\"%05d\", 42)"), QLatin1String("00042"));
    QCOMPARE(echo("printf(\"%-4d|\", 7)"), QLatin1String("7   |"));
    QCOMPARE(echo("printf(\"%x\", 255)"), QLatin1String("ff"));
    QCOMPARE(echo("printf(\"[%s]\", \"hi\")"), QLatin1String("[hi]"));
    QCOMPARE(echo("printf(\"%.2f\", 3.14159)"), QLatin1String("3.14"));
}

void FakeVimTester::test_vim_script_collection_builtins()
{
    // sort, reverse, copy, type, max, min.
    TestData data;
    setup(&data);
    QString message;
    data.handler->commandBufferChanged.set(
        [&](const QString &msg, int, int, int) { message = msg; });
    auto echo = [&](const char *expr) -> QString {
        message.clear();
        data.doCommand(QLatin1String("echo ") + QLatin1String(expr));
        return message;
    };

    QCOMPARE(echo("sort([3, 1, 2], \"n\")"), QLatin1String("[1, 2, 3]"));
    QCOMPARE(echo("sort([\"b\", \"a\", \"c\"])"), QLatin1String("['a', 'b', 'c']"));
    QCOMPARE(echo("reverse([1, 2, 3])"), QLatin1String("[3, 2, 1]"));
    QCOMPARE(echo("max([3, 7, 2])"), QLatin1String("7"));
    QCOMPARE(echo("min([3, 7, 2])"), QLatin1String("2"));
    QCOMPARE(echo("type(1)"), QLatin1String("0"));
    QCOMPARE(echo("type(\"x\")"), QLatin1String("1"));
    QCOMPARE(echo("type([])"), QLatin1String("3"));
    QCOMPARE(echo("type({})"), QLatin1String("4"));

    // copy() makes an independent list: mutating the copy leaves the original.
    data.doCommand("let g:a = [1, 2]");
    data.doCommand("let g:b = copy(g:a)");
    data.doCommand("call add(g:b, 3)");
    QCOMPARE(echo("g:a"), QLatin1String("[1, 2]"));
    QCOMPARE(echo("g:b"), QLatin1String("[1, 2, 3]"));
}

void FakeVimTester::test_vim_script_map_filter()
{
    // map() and filter() evaluate an expression per element with v:val/v:key
    TestData data;
    setup(&data);
    QString message;
    data.handler->commandBufferChanged.set(
        [&](const QString &msg, int, int, int) { message = msg; });
    auto echo = [&](const char *expr) -> QString {
        message.clear();
        data.doCommand(QLatin1String("echo ") + QLatin1String(expr));
        return message;
    };

    QCOMPARE(echo("map([1, 2, 3], \"v:val * 2\")"), QLatin1String("[2, 4, 6]"));
    QCOMPARE(echo("filter([1, 2, 3, 4], \"v:val % 2 == 0\")"), QLatin1String("[2, 4]"));
    QCOMPARE(echo("map([1, 2, 3], \"v:key\")"), QLatin1String("[0, 1, 2]"));

    // On a dictionary, v:key is the key.
    QCOMPARE(echo("map({\"a\": 1, \"b\": 2}, \"v:val + 10\")"),
             QLatin1String("{'a': 11, 'b': 12}"));
    QCOMPARE(echo("filter({\"a\": 1, \"b\": 2, \"c\": 3}, \"v:val >= 2\")"),
             QLatin1String("{'b': 2, 'c': 3}"));

    // The list is mutated in place and v:val does not leak afterwards.
    data.doCommand("let g:l = [1, 2, 3]");
    data.doCommand("call map(g:l, \"v:val + 1\")");
    QCOMPARE(echo("g:l"), QLatin1String("[2, 3, 4]"));
    QCOMPARE(echo("exists('v:val')"), QLatin1String("0"));
}

void FakeVimTester::test_vim_script_try_catch()
{
    // :try/:catch/:finally/:endtry and :throw.
    TestData data;
    setup(&data);
    QString message;
    data.handler->commandBufferChanged.set(
        [&](const QString &msg, int, int, int) { message = msg; });
    auto echo = [&](const char *expr) -> QString {
        message.clear();
        data.doCommand(QLatin1String("echo ") + QLatin1String(expr));
        return message;
    };

    // A thrown value is caught and exposed as v:exception.
    data.doCommand("let g:c = \"none\" | try | throw \"boom\" | "
                   "catch | let g:c = v:exception | endtry");
    QCOMPARE(echo("g:c"), QLatin1String("boom"));

    // No exception: the catch body does not run.
    data.doCommand("let g:c = \"ok\" | try | let g:x = 1 | "
                   "catch | let g:c = \"caught\" | endtry");
    QCOMPARE(echo("g:c"), QLatin1String("ok"));

    // :finally always runs, with and without an exception.
    data.doCommand("let g:f = 0 | try | throw \"e\" | catch | finally | "
                   "let g:f = 1 | endtry");
    QCOMPARE(echo("g:f"), QLatin1String("1"));
    data.doCommand("let g:f = 0 | try | let g:x = 1 | finally | "
                   "let g:f = 2 | endtry");
    QCOMPARE(echo("g:f"), QLatin1String("2"));

    // A pattern selects the matching :catch clause.
    data.doCommand("let g:c = \"?\" | try | throw \"E42: bad\" | "
                   "catch /E13/ | let g:c = \"wrong\" | "
                   "catch /E42/ | let g:c = \"right\" | endtry");
    QCOMPARE(echo("g:c"), QLatin1String("right"));

    // An exception thrown in a function propagates to the caller's :try.
    data.doCommand("function Boom() | throw \"frombody\" | endfunction");
    data.doCommand("let g:c = \"?\" | try | call Boom() | "
                   "catch | let g:c = v:exception | endtry");
    QCOMPARE(echo("g:c"), QLatin1String("frombody"));

    // :throw inside a loop unwinds it and is caught outside.
    data.doCommand("let g:n = 0 | try | for i in range(1, 10) | "
                   "let g:n += 1 | if i == 3 | throw \"stop\" | endif | endfor | "
                   "catch | endtry");
    QCOMPARE(echo("g:n"), QLatin1String("3"));
}

void FakeVimTester::test_vim_script_more_builtins()
{
    // extend, index, count, insert, repeat, trim, nr2char, char2nr, matchstr,
    // match.
    TestData data;
    setup(&data);
    QString message;
    data.handler->commandBufferChanged.set(
        [&](const QString &msg, int, int, int) { message = msg; });
    auto echo = [&](const char *expr) -> QString {
        message.clear();
        data.doCommand(QLatin1String("echo ") + QLatin1String(expr));
        return message;
    };

    QCOMPARE(echo("extend([1, 2], [3, 4])"), QLatin1String("[1, 2, 3, 4]"));
    QCOMPARE(echo("extend({\"a\": 1}, {\"b\": 2})"), QLatin1String("{'a': 1, 'b': 2}"));
    QCOMPARE(echo("index([10, 20, 30], 20)"), QLatin1String("1"));
    QCOMPARE(echo("index([10, 20], 99)"), QLatin1String("-1"));
    QCOMPARE(echo("count([1, 2, 1, 1], 1)"), QLatin1String("3"));
    QCOMPARE(echo("insert([2, 3], 1)"), QLatin1String("[1, 2, 3]"));
    QCOMPARE(echo("insert([1, 3], 2, 1)"), QLatin1String("[1, 2, 3]"));
    QCOMPARE(echo("repeat(\"ab\", 3)"), QLatin1String("ababab"));
    QCOMPARE(echo("repeat([0], 3)"), QLatin1String("[0, 0, 0]"));
    QCOMPARE(echo("trim(\"  hi  \")"), QLatin1String("hi"));
    QCOMPARE(echo("nr2char(65)"), QLatin1String("A"));
    QCOMPARE(echo("char2nr(\"A\")"), QLatin1String("65"));
    QCOMPARE(echo("matchstr(\"foobar\", \"o\\\\+\")"), QLatin1String("oo"));
    QCOMPARE(echo("match(\"foobar\", \"bar\")"), QLatin1String("3"));
    QCOMPARE(echo("match(\"foobar\", \"xyz\")"), QLatin1String("-1"));
}

void FakeVimTester::test_vim_script_expr_register()
{
    // The "=" expression register in insert mode: CTRL-R = {expr} <CR>
    TestData data;
    setup(&data);

    data.setText("");
    data.doKeys("i<c-r>=1 + 2<CR>");
    data.doKeys("<ESC>");
    QCOMPARE(data.text(), QByteArray("3"));

    // A builtin call.
    data.setText("");
    data.doKeys("i<c-r>=toupper(\"ab\")<CR>");
    data.doKeys("<ESC>");
    QCOMPARE(data.text(), QByteArray("AB"));

    // A variable, inserted amid existing text.
    data.doCommand("let g:who = \"Bob\"");
    data.setText("hi " X "!");
    data.doKeys("i<c-r>=g:who<CR>");
    data.doKeys("<ESC>");
    QCOMPARE(data.text(), QByteArray("hi Bob!"));

    // Escape cancels the prompt without inserting.
    data.setText("");
    data.doKeys("ix<c-r>=1 + 2<ESC><ESC>");
    QCOMPARE(data.text(), QByteArray("x"));
}

void FakeVimTester::test_vim_script_operators()
{
    // =~ / !~ regex match, is / isnot identity, more number literals and
    // v:version.
    TestData data;
    setup(&data);
    QString message;
    data.handler->commandBufferChanged.set(
        [&](const QString &msg, int, int, int) { message = msg; });
    auto echo = [&](const char *expr) -> QString {
        message.clear();
        data.doCommand(QLatin1String("echo ") + QLatin1String(expr));
        return message;
    };

    QCOMPARE(echo("\"foobar\" =~ \"o\\\\+\""), QLatin1String("1"));
    QCOMPARE(echo("\"abc\" =~ \"x\""), QLatin1String("0"));
    QCOMPARE(echo("\"abc\" !~ \"x\""), QLatin1String("1"));
    QCOMPARE(echo("1 is 1"), QLatin1String("1"));
    QCOMPARE(echo("1 isnot 2"), QLatin1String("1"));

    // Identity of containers is by reference.
    data.doCommand("let g:a = [1, 2] | let g:b = g:a | let g:c = copy(g:a)");
    QCOMPARE(echo("g:a is g:b"), QLatin1String("1"));
    QCOMPARE(echo("g:a is g:c"), QLatin1String("0"));
    QCOMPARE(echo("g:a isnot g:c"), QLatin1String("1"));

    // Number literals and v:version.
    QCOMPARE(echo("0b101"), QLatin1String("5"));
    QCOMPARE(echo("0o17"), QLatin1String("15"));
    QCOMPARE(echo("0xff"), QLatin1String("255"));
    QCOMPARE(echo("v:version >= 800"), QLatin1String("1"));
}

void FakeVimTester::test_vim_script_unpacking()
{
    // :let [a, b] = list and :for [k, v] in list.
    TestData data;
    setup(&data);
    QString message;
    data.handler->commandBufferChanged.set(
        [&](const QString &msg, int, int, int) { message = msg; });
    auto echo = [&](const char *expr) -> QString {
        message.clear();
        data.doCommand(QLatin1String("echo ") + QLatin1String(expr));
        return message;
    };

    data.doCommand("let [g:a, g:b] = [1, 2]");
    QCOMPARE(echo("g:a"), QLatin1String("1"));
    QCOMPARE(echo("g:b"), QLatin1String("2"));

    // Extra items are ignored; missing ones default to 0.
    data.doCommand("let [g:p, g:q] = [10, 20, 30]");
    QCOMPARE(echo("g:p"), QLatin1String("10"));
    data.doCommand("let [g:m, g:n] = [7]");
    QCOMPARE(echo("g:n"), QLatin1String("0"));

    // :for unpacking, e.g. over items().
    data.doCommand("let g:r = [] | for [k, v] in items({\"a\": 1, \"b\": 2}) | "
                   "call add(g:r, k . \"=\" . v) | endfor");
    QCOMPARE(echo("g:r"), QLatin1String("['a=1', 'b=2']"));
}

void FakeVimTester::test_vim_script_slicing()
{
    // Inclusive slices "x[a:b]" on lists and strings.
    TestData data;
    setup(&data);
    QString message;
    data.handler->commandBufferChanged.set(
        [&](const QString &msg, int, int, int) { message = msg; });
    auto echo = [&](const char *expr) -> QString {
        message.clear();
        data.doCommand(QLatin1String("echo ") + QLatin1String(expr));
        return message;
    };

    QCOMPARE(echo("[0, 1, 2, 3, 4][1:3]"), QLatin1String("[1, 2, 3]")); // inclusive
    QCOMPARE(echo("[0, 1, 2, 3][2:]"), QLatin1String("[2, 3]"));
    QCOMPARE(echo("[0, 1, 2, 3][:1]"), QLatin1String("[0, 1]"));
    QCOMPARE(echo("[0, 1, 2, 3, 4][-2:]"), QLatin1String("[3, 4]"));  // from end
    QCOMPARE(echo("\"hello\"[1:3]"), QLatin1String("ell"));
    QCOMPARE(echo("\"hello\"[2:]"), QLatin1String("llo"));
    QCOMPARE(echo("\"hello\"[:1]"), QLatin1String("he"));
    QCOMPARE(echo("\"hello\"[-2:]"), QLatin1String("lo"));
    QCOMPARE(echo("[1, 2, 3][3:5]"), QLatin1String("[]"));            // out of range
    // A ":" inside a subscript from a ternary is still an index, not a slice.
    QCOMPARE(echo("[10, 20, 30][1 > 0 ? 2 : 0]"), QLatin1String("30"));
}

void FakeVimTester::test_vim_script_ex_commands()
{
    // :finish, :silent[!], :echoerr, :echon.
    TestData data;
    setup(&data);
    QString message;
    data.handler->commandBufferChanged.set(
        [&](const QString &msg, int, int, int) { message = msg; });
    auto echo = [&](const char *expr) -> QString {
        message.clear();
        data.doCommand(QLatin1String("echo ") + QLatin1String(expr));
        return message;
    };
    auto source = [&](const char *text) {
        QTemporaryFile file;
        QVERIFY(file.open());
        file.write(text);
        file.flush();
        data.doCommand(QLatin1String("source ") + file.fileName());
    };

    // :echoerr / :echon produce their evaluated argument.
    message.clear();
    data.doCommand("echoerr \"oops \" . 42");
    QCOMPARE(message, QLatin1String("oops 42"));
    message.clear();
    data.doCommand("echon 1 + 1");
    QCOMPARE(message, QLatin1String("2"));

    // :finish stops running the rest of a sourced file.
    source("let g:before = 1\nfinish\nlet g:after = 1\n");
    QCOMPARE(echo("g:before"), QLatin1String("1"));
    QCOMPARE(echo("exists('g:after')"), QLatin1String("0"));

    // :silent runs the command but suppresses its messages.
    data.doCommand("silent let g:s = 42");
    QCOMPARE(echo("g:s"), QLatin1String("42"));
    message.clear();
    data.doCommand("silent echomsg \"quiet\"");
    QVERIFY(message != QLatin1String("quiet"));

    // :silent! also swallows errors.
    message.clear();
    data.doCommand("silent! call NoSuchFunction()");
    QVERIFY(!message.contains(QLatin1String("NoSuchFunction")));
}

void FakeVimTester::test_vim_script_funcref()
{
    // Funcrefs, lambdas, function()/call(), the -> method syntax and closures.
    TestData data;
    setup(&data);
    QString message;
    data.handler->commandBufferChanged.set(
        [&](const QString &msg, int, int, int) { message = msg; });
    auto echo = [&](const char *expr) -> QString {
        message.clear();
        data.doCommand(QLatin1String("echo ") + QLatin1String(expr));
        return message;
    };
    auto source = [&](const char *text) {
        QTemporaryFile file;
        QVERIFY(file.open());
        file.write(text);
        file.flush();
        data.doCommand(QLatin1String("source ") + file.fileName());
    };

    // Lambdas stored in variables and called.
    data.doCommand("let g:Double = {x -> x * 2}");
    QCOMPARE(echo("g:Double(21)"), QLatin1String("42"));
    data.doCommand("let g:Add = {a, b -> a + b}");
    QCOMPARE(echo("g:Add(3, 4)"), QLatin1String("7"));

    // function() to a builtin, and call().
    data.doCommand("let g:L = function(\"strlen\")");
    QCOMPARE(echo("g:L(\"hello\")"), QLatin1String("5"));
    QCOMPARE(echo("call(function(\"strlen\"), [\"abcd\"])"), QLatin1String("4"));
    QCOMPARE(echo("call(g:Add, [10, 20])"), QLatin1String("30"));

    // Method syntax: v->f(args) is f(v, args).
    QCOMPARE(echo("\"abc\"->strlen()"), QLatin1String("3"));
    QCOMPARE(echo("[3, 1, 2]->sort()"), QLatin1String("[1, 2, 3]"));
    QCOMPARE(echo("21->g:Double()"), QLatin1String("42"));

    // string() of a named funcref.
    QCOMPARE(echo("string(function(\"Foo\"))"), QLatin1String("function('Foo')"));

    // Closures capture the defining scope.
    source("function MkAdder(n)\n  return {x -> x + a:n}\nendfunction\n");
    data.doCommand("let g:Add5 = MkAdder(5)");
    QCOMPARE(echo("g:Add5(10)"), QLatin1String("15"));

    // map()/filter()/sort() accept a lambda: f(key, val) for map/filter and
    // f(a, b) for the sort comparator.
    QCOMPARE(echo("map([1, 2, 3], {i, v -> v * 10})"), QLatin1String("[10, 20, 30]"));
    QCOMPARE(echo("filter([1, 2, 3, 4], {i, v -> v % 2 == 0})"), QLatin1String("[2, 4]"));
    QCOMPARE(echo("sort([3, 1, 2], {a, b -> a - b})"), QLatin1String("[1, 2, 3]"));
    QCOMPARE(echo("sort([1, 2, 3], {a, b -> b - a})"), QLatin1String("[3, 2, 1]"));
}





void FakeVimTester::test_vim_script_error_numbers()
{
    // An error carries the number Vim gives it, and inside a ":try" it arrives
    // as an exception a script can catch. Values taken from Vim 9.1.
    TestData data;
    setup(&data);
    QString message;
    data.handler->commandBufferChanged.set(
        [&](const QString &msg, int, int, int) { message = msg; });
    auto echo = [&](const char *expr) -> QString {
        message.clear();
        data.doCommand(QLatin1String("echo ") + QLatin1String(expr));
        return message;
    };

    // Outside a ":try" the error is reported as before, now with its number.
    QCOMPARE(echo("g:nosuchvar"), QLatin1String("E121: Undefined variable: g:nosuchvar"));
    QCOMPARE(echo("NoSuchFunc()"), QLatin1String("E117: Unknown function: NoSuchFunc"));
    QCOMPARE(echo("&nosuchoption"), QLatin1String("E113: Unknown option: nosuchoption"));
    QCOMPARE(echo("[1, 2][9]"), QLatin1String("E684: List index out of range: 9"));
    message.clear();
    data.doCommand("call NoSuchFunc()");
    QCOMPARE(message, QLatin1String("E117: Unknown function: NoSuchFunc"));

    // Inside a ":try" it arrives as an exception. A block spans several lines,
    // so this runs as a script rather than one line at a time.
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString path = dir.path() + "/t.vim";
    QFile f(path);
    QVERIFY(f.open(QIODevice::WriteOnly));
    f.write("let g:hit = 'no'\n"
            "try\n"
            "  echo g:nosuchvar\n"
            "catch /^Vim\\%((\\a\\+)\\)\\=:E121/\n"
            "  let g:hit = 'caught'\n"
            "  let g:ex = v:exception\n"
            "endtry\n"
            // This is what lets a plugin tell whether the work it wrapped went
            // through: the line after the error is not reached.
            "let g:ok = 1\n"
            "try\n"
            "  echo g:nosuchvar\n"
            "  let g:ok = 2\n"
            "catch\n"
            "  let g:ok = 0\n"
            "endtry\n");
    f.close();
    data.doCommand("source " + path);
    QCOMPARE(echo("g:hit"), QLatin1String("caught"));
    QCOMPARE(echo("g:ok"), QLatin1String("0"));
    // "v:exception" holds it in the shape a script reports or matches on.
    QCOMPARE(echo("g:ex"), QLatin1String("Vim:E121: Undefined variable: g:nosuchvar"));
    data.doCommand("unlet g:hit | unlet g:ok | unlet g:ex");
}

void FakeVimTester::test_vim_pattern_lookbehind_limit()
{
    // "\@123<=" says how far back Vim is to look, which every plugin that skips
    // an escaped character writes: matchit has "\\\@1<!\%(\\\\\)*". Driven from a
    // script, where the pattern can be written as Vim writes it. Values taken
    // from Vim 9.1.
    TestData data;
    setup(&data);
    QString message;
    data.handler->commandBufferChanged.set(
        [&](const QString &msg, int, int, int) {
            if (!msg.startsWith("--"))
                message = msg;
        });
    auto echo = [&](const char *expr) -> QString {
        message.clear();
        data.doCommand(QLatin1String("echo ") + QLatin1String(expr));
        return message;
    };

    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    QFile f(dir.path() + "/p.vim");
    QVERIFY(f.open(QIODevice::WriteOnly));
    f.write("let s:notslash = '\\\\\\@1<!\\%(\\\\\\\\\\)*'\n"
            "let g:a = substitute('\\<if\\>:\\<else\\>', s:notslash . '\\zs:', 'X', 'g')\n"
            "let g:b = match('a:b', s:notslash . ':')\n"
            "let g:c = match('a:b', '\\\\\\@<!:')\n"
            "let g:d = match('a\\:b', s:notslash . ':')\n");
    f.close();
    data.doCommand("source " + dir.path() + "/p.vim");

    // The colon that is not escaped is the one it finds.
    QCOMPARE(echo("g:a"), QLatin1String("\\<if\\>X\\<else\\>"));
    QCOMPARE(echo("g:b"), QLatin1String("1"));
    // Without a number it means the same thing here.
    QCOMPARE(echo("g:c"), QLatin1String("1"));
    // An escaped colon is passed over, which is the point of the pattern.
    QCOMPARE(echo("g:d"), QLatin1String("-1"));
    data.doCommand("unlet g:a | unlet g:b | unlet g:c | unlet g:d");
}

void FakeVimTester::test_vim_script_block_abbreviations()
{
    // Vim lets the block commands be shortened, and scripts do: matchit closes
    // an "if" with "end". Values taken from Vim 9.1, which accepts every one of
    // these.
    TestData data;
    setup(&data);
    QString message;
    data.handler->commandBufferChanged.set(
        [&](const QString &msg, int, int, int) {
            if (!msg.startsWith("--"))
                message = msg;
        });
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const auto ran = [&](const QByteArray &script) {
        const QString path = dir.path() + "/b.vim";
        QFile f(path);
        f.open(QIODevice::WriteOnly | QIODevice::Truncate);
        f.write("let g:hit = 0\n" + script);
        f.close();
        data.doCommand("source " + path);
        message.clear();
        data.doCommand("echo g:hit");
        return message;
    };

    QCOMPARE(ran("if 1\n let g:hit = 1\nen\n"), QLatin1String("1"));
    QCOMPARE(ran("if 1\n let g:hit = 1\nend\n"), QLatin1String("1"));
    QCOMPARE(ran("if 1\n let g:hit = 1\nendi\n"), QLatin1String("1"));
    QCOMPARE(ran("if 0\nel\n let g:hit = 1\nendif\n"), QLatin1String("1"));
    QCOMPARE(ran("if 0\nelsei 1\n let g:hit = 1\nendif\n"), QLatin1String("1"));
    QCOMPARE(ran("let i = 0\nwh i < 1\n let g:hit = 1\n let i = 1\nendw\n"), QLatin1String("1"));
    QCOMPARE(ran("for x in [1]\n let g:hit = 1\nendfo\n"), QLatin1String("1"));
    QCOMPARE(ran("try\n let g:hit = 1\nendt\n"), QLatin1String("1"));
    QCOMPARE(ran("try\n throw 'x'\ncat\n let g:hit = 1\nendtry\n"), QLatin1String("1"));
    QCOMPARE(ran("try\n let g:x = 1\nfina\n let g:hit = 1\nendtry\n"), QLatin1String("1"));
    QCOMPARE(ran("fu! F()\n let g:hit = 1\nendfunction\ncall F()\n"), QLatin1String("1"));
    QCOMPARE(ran("function! G()\n let g:hit = 1\nendf\ncall G()\n"), QLatin1String("1"));
    // A block that does not hold is still skipped, shortened or not.
    QCOMPARE(ran("if 0\n let g:hit = 1\nend\n"), QLatin1String("0"));
    data.doCommand("unlet g:hit | unlet! g:x | delfunction! F | delfunction! G");
}

void FakeVimTester::test_vim_command_line_ctrl_u()
{
    // ":<C-U>" takes away what is on the command line already, which is how a
    // plugin drops the range visual mode puts there. Values taken from Vim 9.1.
    TestData data;
    setup(&data);
    QString message;
    data.handler->commandBufferChanged.set(
        [&](const QString &msg, int, int, int) {
            if (!msg.startsWith("--"))
                message = msg;
        });
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    QFile f(dir.path() + "/m.vim");
    QVERIFY(f.open(QIODevice::WriteOnly));
    f.write("function! Mark(t)\n"
            "  let g:tag = a:t\n"
            "endfunction\n"
            "nnoremap Z1 :call Mark('plain')<CR>\n"
            "nnoremap Z2 :<C-U>call Mark('cleared')<CR>\n"
            "xnoremap Z3 :<C-U>call Mark('from visual')<CR>\n");
    f.close();
    data.doCommand("source " + dir.path() + "/m.vim");
    const auto after = [&](const char *keys) {
        data.setText("a" X "bc" N "def");
        data.doCommand("let g:tag = 'none'");
        data.doKeys(keys);
        message.clear();
        data.doCommand("echo g:tag");
        return message;
    };

    const QString plain = after("Z1");
    const QString cleared = after("Z2");
    const QString visual = after("VZ3");
    data.doCommand("nunmap Z1 | nunmap Z2 | xunmap Z3");
    data.doCommand("delfunction Mark | unlet g:tag");

    QCOMPARE(plain, QLatin1String("plain"));
    QCOMPARE(cleared, QLatin1String("cleared"));
    // The "'<,'>" the ":" put there is gone, so the command is reached.
    QCOMPARE(visual, QLatin1String("from visual"));
}

void FakeVimTester::test_vim_script_searchpair()
{
    // searchpair() answers where the other end of a nested pair is, counting
    // the nesting on the way. Values taken from Vim 9.1.
    TestData data;
    setup(&data);
    QString message;
    data.handler->commandBufferChanged.set(
        [&](const QString &msg, int, int, int) {
            if (!msg.startsWith("--"))
                message = msg;
        });
    auto echo = [&](const char *expr) -> QString {
        message.clear();
        data.doCommand(QLatin1String("echo ") + QLatin1String(expr));
        return message;
    };
    const char *const chain = "i" X "f 1" N "else" N "endif" N "after";

    // A {middle} met where nothing is open is an answer of its own.
    data.setText(chain);
    QCOMPARE(echo("searchpair('\\<if\\>', '\\<else\\>', '\\<endif\\>', 'W')"
                  " . ' at ' . line('.')"),
             QLatin1String("2 at 2"));
    // With no {middle} it walks on to the end of the pair.
    data.setText(chain);
    QCOMPARE(echo("string(searchpairpos('\\<if\\>', '', '\\<endif\\>', 'W'))"),
             QLatin1String("[3, 1]"));
    // "n" answers without going there.
    data.setText(chain);
    QCOMPARE(echo("searchpair('\\<if\\>', '', '\\<endif\\>', 'Wn') . ' at ' . line('.')"),
             QLatin1String("3 at 1"));
    // Backwards from the end finds the start.
    data.setText("if 1" N "else" N X "endif" N "after");
    QCOMPARE(echo("searchpair('\\<if\\>', '', '\\<endif\\>', 'bW') . ' at ' . line('.')"),
             QLatin1String("1 at 1"));
    // From inside the "endif" that one counts as the way in, so the "if" that
    // closes it is one level too far out and nothing is answered.
    data.setText("if 1" N "else" N "en" X "dif" N "after");
    QCOMPARE(echo("searchpair('\\<if\\>', '', '\\<endif\\>', 'bW') . ' at ' . line('.')"),
             QLatin1String("0 at 3"));
    // One nested inside another is passed over.
    data.setText("i" X "f 1" N "if 2" N "endif" N "endif");
    QCOMPARE(echo("searchpair('\\<if\\>', '', '\\<endif\\>', 'W')"), QLatin1String("4"));
    // Nothing to find.
    data.setText("i" X "f 1" N "after");
    QCOMPARE(echo("searchpair('\\<if\\>', '', '\\<endif\\>', 'W')"), QLatin1String("0"));
}

void FakeVimTester::test_vim9_matchit()
{
    // Vim 9.1's matchit plugin, which makes "%" jump between the words of a
    // pair rather than only between brackets. Values taken from Vim 9.1 running
    // the same plugin over the same lines.
    const QString D = "/usr/share/vim/vim91/pack/dist/opt/matchit";
    if (!QFileInfo::exists(D + "/plugin/matchit.vim"))
        QSKIP("Vim 9.1's matchit plugin is not installed");
    TestData data;
    setup(&data);
    data.doCommand("set runtimepath+=" + D);
    data.doCommand("source " + D + "/plugin/matchit.vim");
    const QLatin1String words("let b:match_words = '\\<if\\>:\\<else\\>:\\<endif\\>'");

    // "%" walks the chain round, and "g%" walks it the other way.
    const auto walk = [&](const char *keys) {
        data.setText("i" X "f 1" N "else" N "endif" N "after");
        data.doCommand(words);
        QStringList lines;
        for (int i = 0; i < 3; ++i) {
            data.doKeys(keys);
            lines << QString::number(data.handler->textCursor().blockNumber() + 1);
        }
        return lines.join(',');
    };
    QCOMPARE(walk("%"), QLatin1String("2,3,1"));
    QCOMPARE(walk("g%"), QLatin1String("3,2,1"));

    // Brackets still work, which the plugin takes from 'matchpairs'.
    data.setText("x = (a" X " + b) * c");
    data.doCommand(words);
    data.doKeys("%");
    QCOMPARE(data.handler->textCursor().positionInBlock(), 4);

    data.doCommand("nunmap % | nunmap g% | xunmap % | xunmap g%");
    data.doCommand("ounmap % | ounmap g%");
}

void FakeVimTester::test_vim_script_setline_place()
{
    // setline() takes "." for the line the cursor is on and leaves the cursor
    // where it was; argtextobj puts a line back that way. Values taken from
    // Vim 9.1.
    TestData data;
    setup(&data);
    QString message;
    data.handler->commandBufferChanged.set(
        [&](const QString &msg, int, int, int) {
            if (!msg.startsWith("--"))
                message = msg;
        });
    auto echo = [&](const char *expr) -> QString {
        message.clear();
        data.doCommand(QLatin1String("echo ") + QLatin1String(expr));
        return message;
    };

    data.setText("one" N "t" X "wo" N "three");
    // "." names the line the cursor is on.
    data.doCommand("call setline('.', 'TWO')");
    QCOMPARE(data.text(), QByteArray("one" N "TWO" N "three"));
    // And the cursor stays where it was, which a plugin counts on.
    QCOMPARE(echo("[line('.'), col('.')]"), QLatin1String("[2, 2]"));
    // "$" is the last line.
    data.doCommand("call setline('$', 'THREE')");
    QCOMPARE(data.text(), QByteArray("one" N "TWO" N "THREE"));
    QCOMPARE(echo("[line('.'), col('.')]"), QLatin1String("[2, 2]"));
    // A line that is not there is left alone.
    data.doCommand("call setline(99, 'nowhere')");
    QCOMPARE(data.text(), QByteArray("one" N "TWO" N "THREE"));
}

void FakeVimTester::test_vim_ex_normal_unfinished()
{
    // ":normal" gives up on what the keys did not finish, as if Escape had been
    // typed - insert mode and a command line alike. A plugin leaves visual mode
    // with ":normal :<junk>" that way. Values taken from Vim 9.1.
    TestData data;
    setup(&data);
    QString message;
    data.handler->commandBufferChanged.set(
        [&](const QString &msg, int, int, int) {
            if (!msg.startsWith("--"))
                message = msg;
        });
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    QFile f(dir.path() + "/m.vim");
    QVERIFY(f.open(QIODevice::WriteOnly));
    f.write("function! Mode()\n"
            "  return mode()\n"
            "endfunction\n"
            "nnoremap <expr> Q Mode()\n"
            "xnoremap <expr> Q Mode()\n");
    f.close();
    data.doCommand("source " + dir.path() + "/m.vim");

    // An unfinished command line is given up on, and what it was typed in is
    // not returned to: Vim leaves visual mode as soon as ":" is pressed.
    data.setText("on" X "e" N "two");
    data.doCommand("normal v:junk");
    // Nothing of the unfinished line was typed into the buffer.
    QCOMPARE(data.text(), QByteArray("one" N "two"));
    // A ":" typed in visual mode and given up on lands in normal mode.
    data.setText("on" X "e" N "two");
    data.doKeys("Vj:" "<Esc>");
    data.doKeys("d");
    QCOMPARE(data.text(), QByteArray("one" N "two"));
    data.doCommand("delfunction Mode | nunmap Q | xunmap Q");
}

void FakeVimTester::test_vim_operator_pending_ex_mapping()
{
    // A plugin writes its text objects as an operator-pending mapping to a ":"
    // command, and the selection the command leaves behind is the range the
    // operator works on. argtextobj and every plugin like it do this.
    TestData data;
    setup(&data);
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    QFile f(dir.path() + "/o.vim");
    QVERIFY(f.open(QIODevice::WriteOnly));
    // Selects the three characters the cursor stands on and after.
    f.write("function! Three()\n"
            "  normal! v2l\n"
            "endfunction\n"
            "onoremap <silent> iq :call Three()<CR>\n"
            "xnoremap <silent> iq <Esc>:call Three()<CR>\n");
    f.close();
    data.doCommand("source " + dir.path() + "/o.vim");

    data.setText("abc" X "defghi");
    data.doKeys("diq");
    QCOMPARE(data.text(), QByteArray("abcghi"));
    // The same as a change, which leaves insert mode behind.
    data.setText("abc" X "defghi");
    data.doKeys("ciqXY" "<Esc>");
    QCOMPARE(data.text(), QByteArray("abcXYghi"));
    // And as a yank, which leaves the text where it was.
    data.setText("abc" X "defghi");
    data.doKeys("yiq");
    QCOMPARE(data.text(), QByteArray("abcdefghi"));
    QString message;
    data.handler->commandBufferChanged.set(
        [&](const QString &msg, int, int, int) {
            if (!msg.startsWith("--"))
                message = msg;
        });
    message.clear();
    data.doCommand("echo getreg('')");
    QCOMPARE(message, QLatin1String("def"));
    data.doCommand("ounmap iq | xunmap iq | delfunction Three");
}

void FakeVimTester::test_vim_script_list_compare()
{
    // A List or a Dictionary compares with its own kind only, item by item, and
    // what it holds is compared by type as well: a Number in one is never equal
    // to a String. Values taken from Vim 9.1.
    TestData data;
    setup(&data);
    QString message;
    data.handler->commandBufferChanged.set(
        [&](const QString &msg, int, int, int) {
            if (!msg.startsWith("--"))
                message = msg;
        });
    const auto value = [&](const QString &expr) {
        message.clear();
        data.doCommand("echo " + expr);
        return message;
    };

    QCOMPARE(value("[1,2] == [1,2]"), QLatin1String("1"));
    QCOMPARE(value("[1,2] == [1,3]"), QLatin1String("0"));
    QCOMPARE(value("[1,2] == [1,2,3]"), QLatin1String("0"));
    QCOMPARE(value("[] == []"), QLatin1String("1"));
    QCOMPARE(value("[1,2] != [1,3]"), QLatin1String("1"));
    QCOMPARE(value("[1,[2,3]] == [1,[2,3]]"), QLatin1String("1"));
    QCOMPARE(value("[1,[2,3]] == [1,[2,4]]"), QLatin1String("0"));
    // A Number and a String are equal on their own, but not inside a List.
    QCOMPARE(value("1 == '1'"), QLatin1String("1"));
    QCOMPARE(value("[1] == ['1']"), QLatin1String("0"));
    QCOMPARE(value("1 == 1.0"), QLatin1String("1"));
    QCOMPARE(value("[1] == [1.0]"), QLatin1String("0"));
    QCOMPARE(value("[[1]] == [{}]"), QLatin1String("0"));
    // The keys of a Dictionary are a set, so their order says nothing.
    QCOMPARE(value("{'a':1} == {'a':1}"), QLatin1String("1"));
    QCOMPARE(value("{'a':1} == {'a':2}"), QLatin1String("0"));
    QCOMPARE(value("{'a':1} == {'a':1,'b':2}"), QLatin1String("0"));
    QCOMPARE(value("{'a':1,'b':2} == {'b':2,'a':1}"), QLatin1String("1"));
    QCOMPARE(value("{'a':[1]} == {'a':[1]}"), QLatin1String("1"));
    // A case suffix reaches the strings a container holds.
    QCOMPARE(value("['A'] ==? ['a']"), QLatin1String("1"));
    QCOMPARE(value("['A'] ==# ['a']"), QLatin1String("0"));
    QCOMPARE(value("['A'] !=? ['a']"), QLatin1String("0"));
    QCOMPARE(value("[['A']] ==? [['a']]"), QLatin1String("1"));
    // "is" asks after the container itself, not what it holds.
    QCOMPARE(value("[1] is [1]"), QLatin1String("0"));
    QCOMPARE(value("[1] isnot 1"), QLatin1String("1"));

    const auto failure = [&](const QString &expr) {
        data.doCommand("let g:e = ''");
        data.doCommand("try | echo " + expr + " | catch | let g:e = v:exception | endtry");
        return value("g:e");
    };
    QVERIFY(failure("[1] == 1").contains(QLatin1String("E691")));
    QVERIFY(failure("1 == [1]").contains(QLatin1String("E691")));
    QVERIFY(failure("[] == {}").contains(QLatin1String("E691")));
    QVERIFY(failure("[1] < 1").contains(QLatin1String("E691")));
    QVERIFY(failure("{'a':1} == 1").contains(QLatin1String("E735")));
    QVERIFY(failure("[1] < [2]").contains(QLatin1String("E692")));
    QVERIFY(failure("[1] >= [2]").contains(QLatin1String("E692")));
    QVERIFY(failure("{'a':1} < {'a':2}").contains(QLatin1String("E736")));
    data.doCommand("unlet g:e");
}

void FakeVimTester::test_vim_script_compare_ignorecase()
{
    // A bare comparison follows 'ignorecase', which is why a plugin that means
    // it writes "==#". Values taken from Vim 9.1.
    TestData data;
    setup(&data);
    QString message;
    data.handler->commandBufferChanged.set(
        [&](const QString &msg, int, int, int) {
            if (!msg.startsWith("--"))
                message = msg;
        });
    const auto value = [&](const QString &expr) {
        message.clear();
        data.doCommand("echo " + expr);
        return message;
    };
    const QStringList exprs = {"'ABC' == 'abc'", "'ABC' ==# 'abc'", "'ABC' ==? 'abc'",
                               "'ABC' =~ 'abc'", "'ABC' =~# 'abc'", "'ABC' =~? 'abc'",
                               "'ABC' != 'abc'", "'abc' < 'ABD'", "['ABC'] == ['abc']"};

    QStringList sensitive;
    for (const QString &expr : exprs)
        sensitive.append(value(expr));
    data.doCommand("set ignorecase");
    QStringList insensitive;
    for (const QString &expr : exprs)
        insensitive.append(value(expr));
    data.doCommand("set noignorecase");

    QCOMPARE(sensitive, QStringList({"0", "0", "1", "0", "0", "1", "1", "0", "0"}));
    QCOMPARE(insensitive, QStringList({"1", "0", "1", "1", "0", "1", "0", "1", "1"}));
}

void FakeVimTester::test_vim9_argtextobj()
{
    // argtextobj.vim, which gives an argument of a function call as a text
    // object. Values taken from Vim 9.1 running the real plugin over the same
    // lines. Not installed with Vim, so the test is skipped without it.
    const QString D = qEnvironmentVariable("FAKEVIM_TEST_PLUGINS") + "/argtextobj.vim";
    if (!QFileInfo::exists(D + "/plugin/argtextobj.vim"))
        QSKIP("argtextobj.vim is not there; set FAKEVIM_TEST_PLUGINS to a checkout");
    TestData data;
    setup(&data);
    data.doCommand("set runtimepath+=" + D);
    data.doCommand("source " + D + "/plugin/argtextobj.vim");

    // The cursor goes where the X stands, as in the Vim runs at column+1.
    const auto run = [&](const char *line, const char *keys) {
        data.setText(line);
        data.doKeys(keys);
        return QString::fromUtf8(data.text());
    };
    const QStringList got = {
        run("foo(alpha, " X "beta, gamma)", "dia"),
        run("foo(alpha, " X "beta, gamma)", "daa"),
        run("foo(" X "alpha, beta, gamma)", "dia"),
        run("foo(" X "alpha, beta, gamma)", "daa"),
        run("foo(alpha, beta, " X "gamma)", "dia"),
        run("foo(alpha, beta, " X "gamma)", "daa"),
        run("foo(" X "alpha)", "dia"),
        run("foo(" X "alpha)", "daa"),
        run("foo(alpha, bar(" X "x, y), gamma)", "dia"),
        run("foo(alpha, bar(" X "x, y), gamma)", "daa"),
        run("foo(alpha, " X "beta, gamma)", "ciaZZ" "<Esc>"),
        run("foo(alpha, " X "beta, gamma)", "viad"),
        run("foo(alpha, " X "beta, gamma)", "vaad"),
        run("foo(alpha,   " X "beta , gamma)", "dia"),
    };
    // The mappings live in a table shared with every other test.
    data.doCommand("ounmap ia | ounmap aa | vunmap ia | vunmap aa");

    const QStringList wanted = {
        "foo(alpha, , gamma)",
        "foo(alpha, gamma)",
        "foo(, beta, gamma)",
        "foo(beta, gamma)",
        "foo(alpha, beta, )",
        "foo(alpha, beta)",
        "foo()",
        "foo()",
        "foo(alpha, bar(, y), gamma)",
        "foo(alpha, bar(y), gamma)",
        "foo(alpha, ZZ, gamma)",
        "foo(alpha, , gamma)",
        "foo(alpha, gamma)",
        "foo(alpha,   , gamma)",
    };
    QCOMPARE(got, wanted);
}

void FakeVimTester::test_vim_visual_mark_selection()
{
    // A motion to a mark grows the selection in Visual mode rather than
    // starting a new one. Values taken from Vim 9.1.
    TestData data;
    setup(&data);

    // Mark at column 1, cursor on "three", so the selection reaches back.
    data.setText(X "one two three");
    data.doKeys("mawwv`ad");
    QCOMPARE(data.text(), QByteArray("hree"));
    data.setText(X "one two three");
    data.doKeys("mawwvg`ad");
    QCOMPARE(data.text(), QByteArray("hree"));
    // Outside Visual mode the motion is a plain jump, selecting nothing.
    data.setText(X "one two three");
    data.doKeys("maww`ax");
    QCOMPARE(data.text(), QByteArray("ne two three"));
    // A mark motion an operator waits for still takes the text between.
    data.setText(X "one two three");
    data.doKeys("mawwd`a");
    QCOMPARE(data.text(), QByteArray("three"));
}

void FakeVimTester::test_vim_script_v_register()
{
    // The register a command was given, which a plugin reads to work on what
    // the user asked for. Values taken from Vim 9.1.
    TestData data;
    setup(&data);
    QString message;
    data.handler->commandBufferChanged.set(
        [&](const QString &msg, int, int, int) {
            if (!msg.startsWith("--"))
                message = msg;
        });
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    QFile f(dir.path() + "/r.vim");
    QVERIFY(f.open(QIODevice::WriteOnly));
    f.write("function! Report()\n"
            "  let g:seen = v:register\n"
            "  return ''\n"
            "endfunction\n"
            "nnoremap <expr> QQ Report()\n");
    f.close();
    data.doCommand("source " + dir.path() + "/r.vim");
    const auto seenBy = [&](const char *keys) {
        data.setText(X "one two three");
        data.doKeys(keys);
        message.clear();
        data.doCommand("echo g:seen");
        return message;
    };

    QCOMPARE(seenBy("QQ"), QLatin1String("\""));
    QCOMPARE(seenBy("\"aQQ"), QLatin1String("a"));
    QCOMPARE(seenBy("\"AQQ"), QLatin1String("A"));
    message.clear();
    data.doCommand("echo v:register");
    QCOMPARE(message, QLatin1String("\""));
    message.clear();
    data.doCommand("echo exists('v:register')");
    QCOMPARE(message, QLatin1String("1"));
    data.doCommand("nunmap QQ | delfunction Report | unlet g:seen");
}

void FakeVimTester::test_vim_set_trailing_comment()
{
    // A '"' ends the options of a ":set", which is how a script explains the
    // line it is setting something on. Values taken from Vim 9.1.
    TestData data;
    setup(&data);
    QString message;
    data.handler->commandBufferChanged.set(
        [&](const QString &msg, int, int, int) {
            if (!msg.startsWith("--"))
                message = msg;
        });
    const auto value = [&](const QString &command, const QString &option) {
        data.doCommand(command);
        message.clear();
        data.doCommand("echo &" + option);
        return message;
    };

    QCOMPARE(value("set ts=4 \" comment", "ts"), QLatin1String("4"));
    QCOMPARE(value("set clipboard= \" Avoid clobbering", "clipboard"), QString());
    // Even glued to the value, as Vim reads it.
    QCOMPARE(value("set ts=6\" comment", "ts"), QLatin1String("6"));
    QCOMPARE(value("set ts=2 sw=2 \" comment", "sw"), QLatin1String("2"));
    QCOMPARE(value("set commentstring=//\\ %s \" c", "commentstring"),
             QLatin1String("// %s"));
    // The options are shared with every other test.
    data.doCommand("set ts=8 sw=8 commentstring=//\\ %s");
}

void FakeVimTester::test_vim_visual_paste_register_kind()
{
    // A linewise selection is replaced by whole lines whatever the register
    // holds, and a region reaching the last line keeps the ones before it.
    // Values taken from Vim 9.1.
    TestData data;
    setup(&data);
    QString message;
    data.handler->commandBufferChanged.set(
        [&](const QString &msg, int, int, int) {
            if (!msg.startsWith("--"))
                message = msg;
        });
    const auto run = [&](const char *contents, const char *type,
                         const char *start, const char *keys) -> QString {
        data.setText(X "alpha" N "beta" N "gamma");
        data.doCommand(QLatin1String("call setreg('a', ") + contents + ", '" + type + "')");
        data.doKeys(start);
        data.doKeys(keys);
        message.clear();
        data.doCommand("echo line('.') . ',' . col('.')");
        return QString::fromUtf8(data.text()).replace(QLatin1Char('\n'), QLatin1String(" / "))
               + "  at " + message;
    };

    // Charwise text over a linewise selection becomes a line of its own.
    QCOMPARE(run("'XX'", "v", "j0", "V\"ap"),
             QLatin1String("alpha / XX / gamma  at 2,1"));
    QCOMPARE(run("\"XX\\nYY\"", "v", "j0", "V\"ap"),
             QLatin1String("alpha / XX / YY / gamma  at 2,1"));
    // The last line is a line like any other.
    QCOMPARE(run("'XX'", "v", "jj0", "V\"ap"),
             QLatin1String("alpha / beta / XX  at 3,1"));
    QCOMPARE(run("\"XX\\n\"", "V", "jj0", "V\"ap"),
             QLatin1String("alpha / beta / XX  at 3,1"));
    QCOMPARE(run("'XX'", "v", "j0", "Vj\"ap"),
             QLatin1String("alpha / XX  at 2,1"));
    // Nothing is left to keep when the selection was the whole buffer.
    QCOMPARE(run("'XX'", "v", "0", "VG\"ap"), QLatin1String("XX  at 1,1"));
    // A charwise selection is still replaced in place.
    QCOMPARE(run("'XX'", "v", "j0", "viw\"ap"),
             QLatin1String("alpha / XX / gamma  at 2,2"));
}

void FakeVimTester::test_vim9_replace_with_register()
{
    // Ingo Karkat's ReplaceWithRegister, one of the plugins this editor carries
    // an imitation of ("gr"). Values taken from Vim 9.1 running the real one
    // over the same lines. Not installed with Vim, so skipped without it.
    const QString D = qEnvironmentVariable("FAKEVIM_TEST_PLUGINS") + "/vim-ReplaceWithRegister";
    if (!QFileInfo::exists(D + "/plugin/ReplaceWithRegister.vim"))
        QSKIP("ReplaceWithRegister is not there; set FAKEVIM_TEST_PLUGINS to a checkout");
    TestData data;
    setup(&data);
    data.doCommand("set runtimepath+=" + D);
    data.doCommand("source " + D + "/plugin/ReplaceWithRegister.vim");

    const auto run = [&](const char *lines, const char *keys) {
        data.setText(lines);
        data.doKeys(keys);
        return QString::fromUtf8(data.text()).replace(QLatin1Char('\n'), QLatin1String(" / "));
    };
    const QStringList got = {
        run(X "one two three", "yiwwwgriw"),
        run(X "alpha" N "beta", "yyjgrr"),
        run(X "one two three", "yiwwwviwgr"),
        run(X "one two three", "\"ayiwww\"agriw"),
        run(X "one two three four", "yiwwgr2w"),
        run(X "alpha" N "beta" N "gamma", "yyjgrj"),
        run(X "alpha bit" N "beta", "yiwjgrr"),
        run(X "alpha" N "beta" N "gamma", "yyjVgr"),
    };
    // The mappings live in a table shared with every other test.
    data.doCommand("nunmap gr | nunmap grr | vunmap gr | set opfunc=");

    const QStringList wanted = {
        "one two one",
        "alpha / alpha",
        "one two one",
        "one two one",
        "one onefour",
        "alpha / alpha",
        "alpha bit / alpha",
        "alpha / alpha / gamma",
    };
    QCOMPARE(got, wanted);
}

void FakeVimTester::test_vim_visual_paste_linewise_register()
{
    // A linewise register replacing a charwise selection breaks the line open
    // and goes in as whole lines. Values taken from Vim 9.1.
    TestData data;
    setup(&data);
    QString message;
    data.handler->commandBufferChanged.set(
        [&](const QString &msg, int, int, int) {
            if (!msg.startsWith("--"))
                message = msg;
        });
    const auto run = [&](const char *lines, const char *contents,
                         const char *start, const char *keys) -> QString {
        data.setText(lines);
        data.doCommand(QLatin1String("call setreg('a', ") + contents + ", 'V')");
        data.doKeys(start);
        data.doKeys(keys);
        message.clear();
        data.doCommand("echo line('.') . ',' . col('.')");
        const QString text = QString::fromUtf8(data.text())
                                 .replace(QLatin1Char('\n'), QLatin1String(" / "));
        return text + "  at " + message;
    };
    const char *const L = X "alpha" N "beta" N "gamma";
    const char *const W = X "one two three";

    // A whole line taken charwise leaves an empty line at either end.
    QCOMPARE(run(L, "\"XX\\n\"", "j0", "viw\"ap"),
             QLatin1String("alpha /  / XX /  / gamma  at 3,1"));
    // What stood before and after the selection each keep a line.
    QCOMPARE(run(W, "\"XX\\n\"", "0w", "viw\"ap"),
             QLatin1String("one  / XX /  three  at 2,1"));
    QCOMPARE(run(W, "\"XX\\n\"", "0", "viw\"ap"),
             QLatin1String(" / XX /  two three  at 2,1"));
    QCOMPARE(run(W, "\"XX\\n\"", "0ww", "viw\"ap"),
             QLatin1String("one two  / XX /   at 2,1"));
    QCOMPARE(run(W, "\"XX\\nYY\\n\"", "0w", "viw\"ap"),
             QLatin1String("one  / XX / YY /  three  at 2,1"));
    QCOMPARE(run(W, "\"XX\\n\"", "0w", "v\"ap"),
             QLatin1String("one  / XX / wo three  at 2,1"));
    // A selection over two lines keeps the head of the first and the tail of
    // the last.
    QCOMPARE(run(L, "\"XX\\n\"", "0ll", "vj\"ap"),
             QLatin1String("al / XX / a / gamma  at 2,1"));
    QCOMPARE(run(L, "\"XX\\n\"", "jj0", "viw\"ap"),
             QLatin1String("alpha / beta /  / XX /   at 4,1"));
}

void FakeVimTester::test_vim_script_script_local_funcref()
{
    // "<SID>name" names what belongs to the script a mapping came from, which is
    // how a plugin points 'operatorfunc' at something of its own. Vim writes the
    // "<SNR>42_" form into the option; this one keeps what the mapping said and
    // resolves it where it calls. Values taken from Vim 9.1.
    TestData data;
    setup(&data);
    QString message;
    data.handler->commandBufferChanged.set(
        [&](const QString &msg, int, int, int) {
            if (!msg.startsWith("--"))
                message = msg;
        });
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    QFile f(dir.path() + "/s.vim");
    QVERIFY(f.open(QIODevice::WriteOnly));
    f.write("let g:log = []\n"
            "function! s:priv(type)\n"
            "  call add(g:log, 'priv ' . a:type)\n"
            "endfunction\n"
            "function! s:plain()\n"
            "  return 'plain'\n"
            "endfunction\n"
            "let g:F = function('<SID>plain')\n"
            "call add(g:log, 'funcref ' . g:F())\n"
            "call <SID>priv('called')\n"
            "nnoremap <silent> <expr> <Plug>(Probe) ':<C-u>set opfunc=<SID>priv<CR>g@'\n"
            "nmap QF <Plug>(Probe)\n");
    f.close();
    data.doCommand("source " + dir.path() + "/s.vim");
    data.setText(X "one two three");
    data.doKeys("QFiw");
    message.clear();
    data.doCommand("echo join(g:log, ' | ')");
    const QString log = message;
    // The mappings and the option live in a table shared with every other test.
    data.doCommand("nunmap QF | nmapclear <Plug>(Probe) | set opfunc=");
    data.doCommand("unlet g:F | unlet g:log");

    QCOMPARE(log, QLatin1String("funcref plain | priv called | priv char"));
}

void FakeVimTester::test_vim_motion_underscore()
{
    // "_" is linewise over count-1 lines downwards, so "d_" takes this line and
    // "g@_" hands an operator a line. Values taken from Vim 9.1.
    TestData data;
    setup(&data);
    QString message;
    data.handler->commandBufferChanged.set(
        [&](const QString &msg, int, int, int) {
            if (!msg.startsWith("--"))
                message = msg;
        });
    const auto run = [&](const char *start, const char *keys) -> QString {
        data.setText(X "alpha" N "beta" N "gamma" N "delta");
        data.doKeys(start);
        data.doKeys(keys);
        message.clear();
        data.doCommand("echo '[' . substitute(getreg('\"'), \"\\n\", '\\\\n', 'g')"
                       " . '] ' . getregtype('\"')");
        return QString::fromUtf8(data.text()).replace(QLatin1Char('\n'), QLatin1String("/"))
               + "  reg=" + message;
    };

    QCOMPARE(run("0ll", "d_"), QLatin1String("beta/gamma/delta  reg=[alpha\\n] V"));
    QCOMPARE(run("0ll", "2d_"), QLatin1String("gamma/delta  reg=[alpha\\nbeta\\n] V"));
    QCOMPARE(run("0ll", "d2_"), QLatin1String("gamma/delta  reg=[alpha\\nbeta\\n] V"));
    QCOMPARE(run("j0ll", "y_"), QLatin1String("alpha/beta/gamma/delta  reg=[beta\\n] V"));
    QCOMPARE(run("j0ll", "c_XX" "<Esc>"),
             QLatin1String("alpha/XX/gamma/delta  reg=[beta\\n] V"));

    // What an operator is handed: a line, however the count is spelled.
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    QFile f(dir.path() + "/o.vim");
    QVERIFY(f.open(QIODevice::WriteOnly));
    f.write("let g:seen = ''\n"
            "function! Kind(type)\n"
            "  let g:seen = a:type . ' ' . (line(\"']\") - line(\"'[\") + 1)\n"
            "endfunction\n");
    f.close();
    data.doCommand("source " + dir.path() + "/o.vim");
    data.doCommand("set opfunc=Kind");
    const auto kind = [&](const char *start, const char *keys) -> QString {
        data.setText(X "alpha" N "beta" N "gamma" N "delta");
        data.doKeys(start);
        data.doKeys(keys);
        message.clear();
        data.doCommand("echo g:seen");
        return message;
    };
    QCOMPARE(kind("j0ll", "g@_"), QLatin1String("line 1"));
    QCOMPARE(kind("0ll", "g@2_"), QLatin1String("line 2"));
    QCOMPARE(kind("0ll", "2g@_"), QLatin1String("line 2"));
    data.doCommand("set opfunc= | delfunction Kind | unlet g:seen");
}

void FakeVimTester::test_vim_script_col_list()
{
    // line(), col() and virtcol() take a "[lnum, col]" list to ask after a place
    // the cursor is not at, where "$" is the column after the last character.
    // A line that is not there, or a column past its end, gives 0. Values taken
    // from Vim 9.1.
    TestData data;
    setup(&data);
    QString message;
    data.handler->commandBufferChanged.set(
        [&](const QString &msg, int, int, int) {
            if (!msg.startsWith("--"))
                message = msg;
        });
    const auto value = [&](const QString &expr) {
        message.clear();
        data.doCommand("echo " + expr);
        return message;
    };
    data.setText(X "alpha" N "beta" N "gamma");
    data.doKeys("j0ll");

    QCOMPARE(value("col([1, '$'])"), QLatin1String("6"));
    QCOMPARE(value("col([2, '$'])"), QLatin1String("5"));
    QCOMPARE(value("col([2, 4])"), QLatin1String("4"));
    // One past the last character is still a place; two past is not.
    QCOMPARE(value("col([2, 5])"), QLatin1String("5"));
    QCOMPARE(value("col([2, 6])"), QLatin1String("0"));
    QCOMPARE(value("col([2, 0])"), QLatin1String("0"));
    QCOMPARE(value("col([0, '$'])"), QLatin1String("0"));
    QCOMPARE(value("col([9, '$'])"), QLatin1String("0"));
    // Only a number or "$" is a column, and the line has to be one too.
    QCOMPARE(value("col([2, '.'])"), QLatin1String("0"));
    QCOMPARE(value("col(['$', '$'])"), QLatin1String("0"));
    QCOMPARE(value("col([2])"), QLatin1String("0"));
    QCOMPARE(value("line([2, 3])"), QLatin1String("2"));
    QCOMPARE(value("line([9, 1])"), QLatin1String("0"));
    // The plain forms still answer for where the cursor is.
    QCOMPARE(value("col('$')"), QLatin1String("5"));
    QCOMPARE(value("col('.')"), QLatin1String("3"));
    QCOMPARE(value("line('.')"), QLatin1String("2"));

    // A tab counts to its stop in virtcol(), and not in col().
    data.setText(X "alpha" N "" N "\tbeta");
    data.doCommand("set tabstop=8");
    QCOMPARE(value("col([2, '$'])"), QLatin1String("1"));
    QCOMPARE(value("col([2, 1])"), QLatin1String("1"));
    QCOMPARE(value("col([3, '$'])"), QLatin1String("6"));
    QCOMPARE(value("virtcol([3, 2])"), QLatin1String("9"));
    QCOMPARE(value("virtcol([3, '$'])"), QLatin1String("13"));
    QCOMPARE(value("virtcol([2, '$'])"), QLatin1String("1"));
    QCOMPARE(value("virtcol([1, 3])"), QLatin1String("3"));
    QCOMPARE(value("virtcol([1, 7])"), QLatin1String("0"));
}

void FakeVimTester::test_vim_register_last_line()
{
    // A linewise yank or delete holds the lines with a break after each, also
    // where the last line of the buffer is one of them - the deletion itself
    // reaches back over the break before it, the register must not. Values
    // taken from Vim 9.1.
    TestData data;
    setup(&data);
    QString message;
    data.handler->commandBufferChanged.set(
        [&](const QString &msg, int, int, int) {
            if (!msg.startsWith("--"))
                message = msg;
        });
    const auto run = [&](const char *start, const char *keys) -> QString {
        data.setText(X "alpha" N "beta" N "gamma");
        data.doKeys(start);
        data.doKeys(keys);
        message.clear();
        data.doCommand("echo '[' . substitute(getreg('\"'), \"\\n\", '\\\\n', 'g')"
                       " . '] ' . getregtype('\"')");
        return message + "  text=["
               + QString::fromUtf8(data.text()).replace(QLatin1Char('\n'), QLatin1String("/"))
               + "]";
    };

    QCOMPARE(run("jj0", "yy"), QLatin1String("[gamma\\n] V  text=[alpha/beta/gamma]"));
    QCOMPARE(run("j0", "yy"), QLatin1String("[beta\\n] V  text=[alpha/beta/gamma]"));
    QCOMPARE(run("jj0", "Vy"), QLatin1String("[gamma\\n] V  text=[alpha/beta/gamma]"));
    QCOMPARE(run("jj0", "dd"), QLatin1String("[gamma\\n] V  text=[alpha/beta]"));
    QCOMPARE(run("jj0", "Vd"), QLatin1String("[gamma\\n] V  text=[alpha/beta]"));
    QCOMPARE(run("j0", "yj"), QLatin1String("[beta\\ngamma\\n] V  text=[alpha/beta/gamma]"));
    // And pasting it back leaves no line behind.
    data.setText(X "alpha" N "beta" N "gamma");
    data.doKeys("jj0yyggp");
    QCOMPARE(data.text(), QByteArray("alpha\ngamma\nbeta\ngamma"));
}

void FakeVimTester::test_vim_script_nonblank()
{
    // The first line from there on holding more than blanks, downwards or
    // upwards; 0 where there is none. Values taken from Vim 9.1.
    TestData data;
    setup(&data);
    QString message;
    data.handler->commandBufferChanged.set(
        [&](const QString &msg, int, int, int) {
            if (!msg.startsWith("--"))
                message = msg;
        });
    const auto value = [&](const QString &expr) {
        message.clear();
        data.doCommand("echo " + expr);
        return message;
    };
    data.setText(X "one" N "" N "   " N "four" N "" N "\t");
    data.doKeys("j0");

    QCOMPARE(value("nextnonblank(1)"), QLatin1String("1"));
    QCOMPARE(value("nextnonblank(2)"), QLatin1String("4"));
    QCOMPARE(value("nextnonblank(3)"), QLatin1String("4"));
    QCOMPARE(value("nextnonblank(5)"), QLatin1String("0"));
    QCOMPARE(value("nextnonblank(7)"), QLatin1String("0"));
    QCOMPARE(value("nextnonblank(0)"), QLatin1String("0"));
    QCOMPARE(value("nextnonblank(-1)"), QLatin1String("0"));
    QCOMPARE(value("nextnonblank('.')"), QLatin1String("4"));
    QCOMPARE(value("nextnonblank('$')"), QLatin1String("0"));
    QCOMPARE(value("prevnonblank(3)"), QLatin1String("1"));
    QCOMPARE(value("prevnonblank(1)"), QLatin1String("1"));
    QCOMPARE(value("prevnonblank(6)"), QLatin1String("4"));
    QCOMPARE(value("prevnonblank(0)"), QLatin1String("0"));
    QCOMPARE(value("prevnonblank('.')"), QLatin1String("1"));
}

void FakeVimTester::test_vim9_exchange()
{
    // Tom McDonald's vim-exchange, the real plugin behind the "cx" emulation.
    // Values taken from Vim 9.1 running it over the same lines. Not installed
    // with Vim, so the test is skipped without it.
    const QString D = qEnvironmentVariable("FAKEVIM_TEST_PLUGINS") + "/vim-exchange";
    if (!QFileInfo::exists(D + "/plugin/exchange.vim"))
        QSKIP("vim-exchange is not there; set FAKEVIM_TEST_PLUGINS to a checkout");
    TestData data;
    setup(&data);
    data.doCommand("set runtimepath+=" + D);
    data.doCommand("source " + D + "/plugin/exchange.vim");
    // The plugin keeps the region it is waiting for in b:exchange, so every
    // case starts by clearing it.
    const auto run = [&](const char *lines, const char *start, const char *keys) -> QString {
        data.setText(lines);
        data.doKeys("cxc");
        data.doKeys(start);
        data.doKeys(keys);
        return QString::fromUtf8(data.text()).replace(QLatin1Char('\n'), QLatin1String(" / "));
    };
    const char *const W = X "one two three";
    const char *const L = X "alpha" N "beta" N "gamma";
    const QStringList got = {
        run(W, "0", "cxiwwwcxiw"),
        run(W, "0", "cxiwwcxiw"),
        run(L, "0", "cxxjjcxx"),
        run(W, "0", "cxiwcxcwwcxiw"),
        run(W, "0", "viwXwwviwX"),
        run(W, "0ww", "cxiw0cxiw"),
        run(X "alpha beta" N "gamma delta", "0", "cxiwjwcxiw"),
        run(X "a" N "b" N "c" N "d", "j0", "cxxjcxx"),
        run(X "a" N "b" N "c" N "d", "0", "cxjjjcxj"),
        run(X "    foo" N "bar", "0llll", "cxxjcxx"),
    };
    // The mappings live in a table shared with every other test.
    data.doCommand("nunmap cx | nunmap cxx | nunmap cxc | vunmap X | vunmap cx");
    data.doCommand("set opfunc=");

    const QStringList wanted = {
        "three two one",       // two words apart
        "two one three",       // two words next to each other
        "gamma / beta / alpha",
        "one two three",       // the first region was cleared again
        "three two one",       // the visual mapping
        "three two one",       // the second region before the first
        "delta beta / gamma alpha",
        "a / c / b / d",
        "c / d / a / b",       // two lines against two lines
        "    bar / foo",       // the indent stays where it was
    };
    QCOMPARE(got, wanted);
}

void FakeVimTester::test_vim_script_registers()
{
    // What a register holds and of what kind, which is how a plugin puts
    // something aside and gives it back. Values taken from Vim 9.1.
    TestData data;
    setup(&data);
    QString message;
    data.handler->commandBufferChanged.set(
        [&](const QString &msg, int, int, int) {
            if (!msg.startsWith("--"))
                message = msg;
        });
    auto echo = [&](const char *expr) -> QString {
        message.clear();
        data.doCommand(QLatin1String("echo ") + QLatin1String(expr));
        return message;
    };

    data.setText("al" X "pha beta" N "gamma delta");
    data.doCommand("call setreg('a', 'hello')");
    QCOMPARE(echo("getreg('a')"), QLatin1String("hello"));
    QCOMPARE(echo("getregtype('a')"), QLatin1String("v"));
    // A trailing newline makes it lines, and so does saying so.
    data.doCommand("call setreg('b', \"linewise\\n\", 'V')");
    QCOMPARE(echo("getregtype('b')"), QLatin1String("V"));
    data.doCommand("call setreg('c', 'block', 'b')");
    QCOMPARE(echo("getregtype('c')[0] == nr2char(22)"), QLatin1String("1"));
    QCOMPARE(echo("getregtype('c')[1:]"), QLatin1String("5"));
    // What a yank left behind, under the name a script reads it by.
    data.doKeys("gg" "yy");
    QCOMPARE(echo("getreg('')"), QLatin1String("alpha beta\n"));
    QCOMPARE(echo("getregtype('')"), QLatin1String("V"));
    // The third argument asks for the lines as a list.
    QCOMPARE(echo("string(getreg('a', 1, 1))"), QLatin1String("['hello']"));
    // A list put there becomes lines.
    data.doCommand("call setreg('d', ['one', 'two'])");
    QCOMPARE(echo("getreg('d')"), QLatin1String("one\ntwo\n"));
    // "@@" is the unnamed register, which is how a plugin puts back what it
    // borrowed: "let reg = @@" ... "let @@ = reg".
    data.doCommand("let @@ = 'borrowed'");
    QCOMPARE(echo("@@"), QLatin1String("borrowed"));
    QCOMPARE(echo("getreg('')"), QLatin1String("borrowed"));
    data.doCommand("let @a = 'named'");
    QCOMPARE(echo("@a"), QLatin1String("named"));
}

void FakeVimTester::test_vim_script_mapping_queries()
{
    // maparg() and hasmapto(), which every plugin asks before putting its own
    // mappings in. Values taken from Vim 9.1.
    TestData data;
    setup(&data);
    QString message;
    data.handler->commandBufferChanged.set(
        [&](const QString &msg, int, int, int) {
            if (!msg.startsWith("--"))
                message = msg;
        });
    auto echo = [&](const char *expr) -> QString {
        message.clear();
        data.doCommand(QLatin1String("echo ") + QLatin1String(expr));
        return message;
    };

    data.doCommand("nnoremap gX :echo 1<CR>");
    const QString mapped = echo("maparg('gX', 'n')");
    const QString missing = echo("'[' . maparg('gZ', 'n') . ']'");
    const QString to = echo("hasmapto(':echo 1<CR>', 'n')");
    const QString notTo = echo("hasmapto('nosuchrhs', 'n')");
    data.doCommand("nunmap gX");

    QCOMPARE(mapped, QLatin1String(":echo 1<CR>"));
    QCOMPARE(missing, QLatin1String("[]"));
    QCOMPARE(to, QLatin1String("1"));
    QCOMPARE(notTo, QLatin1String("0"));
}

void FakeVimTester::test_vim_script_string_escapes()
{
    // A double-quoted string names a character by its number or by the key that
    // sends it, which is how a script spells a key it means to pass on. Values
    // taken from Vim 9.1.
    TestData data;
    setup(&data);
    QString message;
    data.handler->commandBufferChanged.set(
        [&](const QString &msg, int, int, int) {
            if (!msg.startsWith("--"))
                message = msg;
        });
    auto echo = [&](const char *expr) -> QString {
        message.clear();
        data.doCommand(QLatin1String("echo ") + QLatin1String(expr));
        return message;
    };

    QCOMPARE(echo("\"\\x41\""), QLatin1String("A"));
    QCOMPARE(echo("\"\\x7a\""), QLatin1String("z"));
    QCOMPARE(echo("\"\\101\""), QLatin1String("A"));      // octal
    QCOMPARE(echo("char2nr(\"\\u00e9\")"), QLatin1String("233"));
    QCOMPARE(echo("\"\\<Esc>\" == nr2char(27)"), QLatin1String("1"));
    QCOMPARE(echo("\"\\<CR>\" == nr2char(13)"), QLatin1String("1"));
    QCOMPARE(echo("\"\\<Tab>\" == nr2char(9)"), QLatin1String("1"));
    QCOMPARE(echo("\"\\<C-R>\" == nr2char(18)"), QLatin1String("1"));
    QCOMPARE(echo("\"\\x16\" == nr2char(22)"), QLatin1String("1"));
    // The plain ones still stand.
    QCOMPARE(echo("strlen(\"a\\tb\")"), QLatin1String("3"));
    // A single-quoted string takes them all as they are written.
    QCOMPARE(echo("'\\x41'"), QLatin1String("\\x41"));
}

void FakeVimTester::test_vim_script_script_id()
{
    // "<SID>name" is the "s:name" of the script that wrote it, which is how a
    // mapping reaches a function of its own. There is one namespace here, so the
    // prefix stands for the scope it names.
    TestData data;
    setup(&data);
    QString message;
    data.handler->commandBufferChanged.set(
        [&](const QString &msg, int, int, int) {
            if (!msg.startsWith("--"))
                message = msg;
        });
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    QFile f(dir.path() + "/s.vim");
    QVERIFY(f.open(QIODevice::WriteOnly));
    f.write("function! s:answer()\n"
            "  return 'from the script'\n"
            "endfunction\n"
            "function! s:name()\n"
            // Inside a function "<sfile>" ends with the name of the function,
            // which is what a plugin sets 'operatorfunc' from.
            "  return matchstr(expand('<sfile>'), '[^. ]*$')\n"
            "endfunction\n"
            "let g:viaSid = <SID>answer()\n"
            "let g:ownName = s:name()\n");
    f.close();
    data.doCommand("source " + dir.path() + "/s.vim");

    message.clear();
    data.doCommand("echo g:viaSid");
    QCOMPARE(message, QLatin1String("from the script"));
    // The name it found is one that can be called again.
    message.clear();
    data.doCommand("echo exists('*' . g:ownName)");
    QCOMPARE(message, QLatin1String("1"));
    data.doCommand("unlet g:viaSid | unlet g:ownName");
}

void FakeVimTester::test_vim9_commentary()
{
    // tpope's vim-commentary, one of the plugins this editor carries an
    // imitation of. Values taken from Vim 9.1 running the real one over the same
    // lines. Not installed with Vim, so the test is skipped without it.
    const QString D = qEnvironmentVariable("FAKEVIM_TEST_PLUGINS") + "/vim-commentary";
    if (!QFileInfo::exists(D + "/plugin/commentary.vim"))
        QSKIP("vim-commentary is not there; set FAKEVIM_TEST_PLUGINS to a checkout");
    TestData data;
    setup(&data);
    data.doCommand("set runtimepath+=" + D);
    data.doCommand("source " + D + "/plugin/commentary.vim");
    data.doCommand("set commentstring=//\\ %s");
    const auto shown = [&] {
        return QString::fromUtf8(data.text()).replace(QLatin1Char('\n'), QLatin1String(" / "));
    };
    const char *const lines = "i" X "nt a = 1;" N "int b = 2;" N "int c = 3;";

    data.setText(lines);
    data.doKeys("gcc");
    const QString commented = shown();
    data.doKeys("gcc");
    const QString back = shown();
    data.setText(lines);
    data.doKeys("gcj");
    const QString twoLines = shown();
    data.setText(lines);
    data.doKeys("Vjgc");
    const QString visual = shown();
    // The plugin's mappings live in a table shared with every other test.
    data.doCommand("nunmap gc | nunmap gcc | nunmap gcu | xunmap gc | ounmap gc");
    data.doCommand("nunmap gcA | nunmap gco | nunmap gcO");
    data.doCommand("set opfunc=");

    QCOMPARE(commented, QLatin1String("// int a = 1; / int b = 2; / int c = 3;"));
    QCOMPARE(back, QLatin1String("int a = 1; / int b = 2; / int c = 3;"));
    QCOMPARE(twoLines, QLatin1String("// int a = 1; / // int b = 2; / int c = 3;"));
    QCOMPARE(visual, QLatin1String("// int a = 1; / // int b = 2; / int c = 3;"));
}

void FakeVimTester::test_vim_script_range_function()
{
    // A function declared "range" is handed the whole range and called once;
    // one that is not is called for each line of it, with the cursor there.
    // Either way it can read the range as a:firstline and a:lastline. Values
    // taken from Vim 9.1.
    TestData data;
    setup(&data);
    QString message;
    data.handler->commandBufferChanged.set(
        [&](const QString &msg, int, int, int) {
            if (!msg.startsWith("--"))
                message = msg;
        });
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    QFile f(dir.path() + "/r.vim");
    QVERIFY(f.open(QIODevice::WriteOnly));
    f.write("function! WithRange() range\n"
            "  call add(g:seen, 'first=' . a:firstline . ' last=' . a:lastline\n"
            "        \\ . ' cursor=' . line('.'))\n"
            "endfunction\n"
            "function! NoRange()\n"
            "  call add(g:seen, 'cursor=' . line('.') . ' first=' . a:firstline)\n"
            "endfunction\n");
    f.close();
    data.doCommand("source " + dir.path() + "/r.vim");
    const auto seen = [&](const char *command) {
        data.setText("on" X "e" N "two" N "three" N "four" N "five");
        data.doCommand("let g:seen = []");
        data.doCommand(QLatin1String(command));
        message.clear();
        data.doCommand("echo join(g:seen, ' | ')");
        return message;
    };

    // Once, with the range and the cursor on its first line.
    QCOMPARE(seen("2,4call WithRange()"), QLatin1String("first=2 last=4 cursor=2"));
    // Without a range both ends are the line the cursor is on.
    QCOMPARE(seen("call WithRange()"), QLatin1String("first=1 last=1 cursor=1"));
    // Once for each line, and the range is still there to be read.
    QCOMPARE(seen("2,4call NoRange()"),
             QLatin1String("cursor=2 first=2 | cursor=3 first=2 | cursor=4 first=2"));
    // A call with no range leaves the cursor where it was.
    data.setText("one" N "t" X "wo" N "three");
    data.doCommand("let g:seen = []");
    data.doCommand("call NoRange()");
    QCOMPARE(data.handler->textCursor().positionInBlock(), 1);

    data.doCommand("delfunction WithRange | delfunction NoRange | unlet g:seen");
}

void FakeVimTester::test_vim_ex_retab()
{
    // ":retab" writes the white space of each line out again for the tab stop.
    // Without a "!" only a run that holds a tab is touched. Values taken from
    // Vim 9.1. Everything is read before anything is compared, so that a
    // comparison giving up in between does not leave the options behind for
    // other tests to trip over.
    TestData data;
    setup(&data);
    QString message;
    data.handler->commandBufferChanged.set(
        [&](const QString &msg, int, int, int) {
            if (!msg.startsWith("--"))
                message = msg;
        });
    const auto retabbed = [&](const char *options, const char *text, const char *command) {
        data.doCommand(QLatin1String("set ") + QLatin1String(options));
        data.setText(text);
        data.doCommand(QLatin1String(command));
        return QString::fromUtf8(data.text())
            .replace(QLatin1Char('\t'), QLatin1String("<T>"))
            .replace(QLatin1Char('\n'), QLatin1String("/"));
    };

    // Spaces are left alone unless "!" says otherwise.
    const QString spaces = retabbed("noet ts=8", "" X "a               b", "retab");
    const QString bang = retabbed("noet ts=8", "" X "a               b", "retab!");
    // With 'expandtab' the tabs become spaces.
    const QString expanded = retabbed("et ts=8", "" X "\ta", "retab");
    // The whole file where no range says otherwise, and only the lines a range
    // does name.
    const QString whole = retabbed("et ts=8", "" X "\tone" N "\ttwo" N "\tthree", "retab");
    const QString ranged = retabbed("et ts=8", "" X "\tone" N "\ttwo", "2retab");
    // An argument is the new tab stop, and stays as one.
    const QString newStop = retabbed("noet ts=8", "" X "\ta", "retab 4");
    message.clear();
    data.doCommand("echo &ts");
    const QString tabStopAfter = message;
    data.doCommand("set ts=8 | set noet");

    QCOMPARE(spaces, QLatin1String("a               b"));
    QCOMPARE(bang, QLatin1String("a<T><T>b"));
    QCOMPARE(expanded, QLatin1String("        a"));
    QCOMPARE(whole, QLatin1String("        one/        two/        three"));
    QCOMPARE(ranged, QLatin1String("<T>one/        two"));
    QCOMPARE(newStop, QLatin1String("<T><T>a"));
    QCOMPARE(tabStopAfter, QLatin1String("4"));
}

void FakeVimTester::test_vim_script_width_and_getline()
{
    // strdisplaywidth() counts a tab to the next tab stop, from the column it is
    // given; strwidth() counts it as one. getline() with two arguments answers
    // with the lines between them. Values taken from Vim 9.1.
    TestData data;
    setup(&data);
    QString message;
    data.handler->commandBufferChanged.set(
        [&](const QString &msg, int, int, int) {
            if (!msg.startsWith("--"))
                message = msg;
        });
    auto echo = [&](const char *expr) -> QString {
        message.clear();
        data.doCommand(QLatin1String("echo ") + QLatin1String(expr));
        return message;
    };

    data.doCommand("set ts=8");
    QCOMPARE(echo("strdisplaywidth('abc')"), QLatin1String("3"));
    QCOMPARE(echo("strdisplaywidth(\"a\\tb\")"), QLatin1String("9"));
    QCOMPARE(echo("strdisplaywidth(\"\\t\")"), QLatin1String("8"));
    QCOMPARE(echo("strdisplaywidth(\"\\t\", 4)"), QLatin1String("4"));
    QCOMPARE(echo("strdisplaywidth(\"\\t\", 8)"), QLatin1String("8"));
    QCOMPARE(echo("strdisplaywidth('abc', 5)"), QLatin1String("3"));
    QCOMPARE(echo("strwidth(\"a\\tb\")"), QLatin1String("3"));
    data.doCommand("set ts=4");
    QCOMPARE(echo("strdisplaywidth(\"\\t\")"), QLatin1String("4"));
    data.doCommand("set ts=8");

    data.setText("on" X "e" N "two" N "three");
    QCOMPARE(echo("string(getline(1, '$'))"), QLatin1String("['one', 'two', 'three']"));
    QCOMPARE(echo("string(getline(2, 3))"), QLatin1String("['two', 'three']"));
    QCOMPARE(echo("getline('.')"), QLatin1String("one"));
    QCOMPARE(echo("string(getline(1, 1))"), QLatin1String("['one']"));
}

void FakeVimTester::test_vim9_justify()
{
    // Vim's justify plugin, which pads a line out to 'textwidth' through a
    // ":Justify" command over a range. Values taken from Vim 9.1 running the
    // same plugin over the same lines.
    const QString D = "/usr/share/vim/vim91/pack/dist/opt/justify";
    if (!QFileInfo::exists(D + "/plugin/justify.vim"))
        QSKIP("Vim's justify plugin is not installed");
    TestData data;
    setup(&data);
    data.doCommand("set runtimepath+=" + D);
    data.doCommand("source " + D + "/plugin/justify.vim");
    data.doCommand("set tw=40 | set noet | set ts=8 | set sw=8");
    data.setText("" X "The quick brown fox jumps over the lazy dog and keeps on running far away" N
                 "Short line here" N
                 "Another line of words to justify nicely across the page width");
    data.doCommand("1,3Justify");
    const QString after = QString::fromUtf8(data.text()).replace(QLatin1Char('\n'),
                                                                 QLatin1Char('/'));
    // The plugin leaves mappings and a command behind, and the tables they live
    // in are shared with every other test, so put them back before comparing.
    data.doCommand("nunmap _j | vunmap _j | nunmap ,gq | vunmap ,gq");
    data.doCommand("delcommand Justify");
    data.doCommand("set tw=0 | set ts=8 | set sw=8 | set noet");

    // Only the short line has room to be padded; the others are longer than the
    // text width and are left as they are.
    QCOMPARE(after,
             QLatin1String("The quick brown fox jumps over the lazy dog and keeps on running far away"
                           "/Short\t\t  line\t\t    here"
                           "/Another line of words to justify nicely across the page width"));
}

void FakeVimTester::test_vim_softtabstop()
{
    // 'softtabstop' sets how far a tab reaches in insert mode where that is not
    // the same as 'tabstop', and a backspace takes back just as much. Where
    // 'expandtab' is off the whitespace is written out as tabs as far as a real
    // tab stop takes it. Values taken from Vim 9.1.
    TestData data;
    setup(&data);
    const auto typed = [&](const char *options, const char *keys) {
        data.doCommand(QLatin1String("set ") + QLatin1String(options));
        data.setText("" X "x");
        data.doKeys(keys);
        data.doKeys("<Esc>");
        return QString::fromUtf8(data.text()).replace(QLatin1Char('\t'),
                                                      QLatin1String("<TAB>"));
    };

    QCOMPARE(typed("sts=4 et ts=8 sw=8", "i<Tab>"), QLatin1String("    x"));
    QCOMPARE(typed("sts=4 et ts=8 sw=8", "i<Tab><Tab>"), QLatin1String("        x"));
    QCOMPARE(typed("sts=4 noet ts=8 sw=8", "i<Tab>"), QLatin1String("    x"));
    // Two of them reach a real tab stop, so a tab is what is left there.
    QCOMPARE(typed("sts=4 noet ts=8 sw=8", "i<Tab><Tab>"), QLatin1String("<TAB>x"));
    // A backspace takes back one soft tab stop, not one character.
    QCOMPARE(typed("sts=4 noet ts=8 sw=8", "i<Tab><Tab><BS>"), QLatin1String("    x"));
    QCOMPARE(typed("sts=4 et ts=8 sw=8", "i<Tab><Tab><BS>"), QLatin1String("    x"));
    // Without it 'tabstop' is in charge, as before.
    QCOMPARE(typed("sts=0 et ts=8 sw=8", "i<Tab>"), QLatin1String("        x"));
    // A tab reaches the next stop, counting from where the cursor stands.
    QCOMPARE(typed("sts=3 et ts=8 sw=8", "iab<Tab>"), QLatin1String("ab x"));
    // What ":set" reports is what was asked for.
    QString message;
    data.handler->commandBufferChanged.set(
        [&](const QString &msg, int, int, int) {
            if (!msg.startsWith("--"))
                message = msg;
        });
    data.doCommand("set sts=7");
    message.clear();
    data.doCommand("echo &sts");
    QCOMPARE(message, QLatin1String("7"));

    data.doCommand("set sts=0 | set ts=8 | set sw=8 | set noet");
}

void FakeVimTester::test_vim_script_mode()
{
    // mode() says which mode is current, and with something passed has more to
    // say about operator-pending. Values taken from Vim 9.1, read there through
    // mappings of the same shape, since asking on the command line would only
    // ever answer "c". Vim answers "c" for that too, which is what happens here
    // when the question is put in an ex command.
    TestData data;
    setup(&data);
    QString message;
    data.handler->commandBufferChanged.set(
        [&](const QString &msg, int, int, int) {
            if (!msg.startsWith("--"))
                message = msg;
        });
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    QFile f(dir.path() + "/m.vim");
    QVERIFY(f.open(QIODevice::WriteOnly));
    f.write("function! Mark()\n"
            "  let g:tag = mode() . '/' . mode(1)\n"
            "  return ''\n"
            "endfunction\n"
            "nnoremap <expr> Q Mark()\n"
            "xnoremap <expr> Q Mark()\n"
            "inoremap <expr> Q Mark()\n"
            "onoremap <expr> Q Mark()\n");
    f.close();
    data.doCommand("source " + dir.path() + "/m.vim");

    const auto after = [&](const char *keys) -> QString {
        data.setText("aa" X "a" N "bbb");
        data.doCommand("let g:tag = 'none'");
        data.doKeys(keys);
        message.clear();
        data.doCommand("echo g:tag");
        return message;
    };

    // Read them all before comparing: an assertion that gives up in between
    // would leave the mappings behind for other tests to trip over, the map
    // table being shared.
    const QString normal = after("Q");
    const QString charwise = after("vQ");
    const QString linewise = after("VQ");
    const QString blockwise = after("<C-v>Q");
    const QString insert = after("iQ");
    const QString replace = after("RQ");
    const QString pending = after("dQ");
    message.clear();
    data.doCommand("echo mode() . '/' . mode(1)");
    const QString fromExCommand = message;
    data.doCommand("nunmap Q | xunmap Q | iunmap Q | ounmap Q");
    data.doCommand("delfunction Mark | unlet g:tag");

    QCOMPARE(normal, QLatin1String("n/n"));
    QCOMPARE(charwise, QLatin1String("v/v"));
    QCOMPARE(linewise, QLatin1String("V/V"));
    QCOMPARE(blockwise, QLatin1String("\x16/\x16"));
    QCOMPARE(insert, QLatin1String("i/i"));
    QCOMPARE(replace, QLatin1String("R/R"));
    // Only with something passed does operator-pending give itself away.
    QCOMPARE(pending, QLatin1String("n/no"));
    // A command that has been given already runs in normal mode; "c" is what
    // mode() answers while the line is still being typed, which is a mapping
    // away from here.
    QCOMPARE(fromExCommand, QLatin1String("n/n"));
}

void FakeVimTester::test_vim_ex_command_own_selection()
{
    // A selection an ex command puts there itself stays for the next command to
    // work on, which is how a script offers a text object of its own. A ":"
    // typed in visual mode still goes back to normal mode. Values taken from
    // Vim 9.1.
    TestData data;
    setup(&data);
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    QFile f(dir.path() + "/s.vim");
    QVERIFY(f.open(QIODevice::WriteOnly));
    f.write("function! Sel()\n"
            "  normal! V\n"
            "endfunction\n");
    f.close();
    data.doCommand("source " + dir.path() + "/s.vim");

    data.setText("on" X "e" N "two" N "three");
    data.doKeys(":call Sel()<CR>");
    data.doKeys("d");
    QCOMPARE(data.text(), QByteArray("two" N "three"));

    // The other way round: what the ":" was typed in is left behind.
    data.setText("on" X "e" N "two" N "three");
    data.doKeys("V");
    data.doCommand("echo 1");
    data.doKeys("d");
    QCOMPARE(data.text(), QByteArray("one" N "two" N "three"));

    data.doCommand("delfunction Sel");
}

void FakeVimTester::test_vim9_comment_text_object()
{
    // The comment plugin's "ic" and "ac" ask what the syntax is at a place,
    // which Qt Creator answers from the document's language. Standing in for it
    // here is a reply of "Comment" on the lines that are one, which is enough to
    // drive the plugin's own reckoning of where the comment block ends.
    // Values taken from Vim 9.1 with the same buffer and keys.
    const QString D = "/usr/share/vim/vim91/pack/dist/opt/comment";
    if (!QFileInfo::exists(D + "/plugin/comment.vim"))
        QSKIP("Vim 9.1's comment plugin is not installed");

    const auto check = [&](const char *keys) {
        TestData data;
        setup(&data);
        data.handler->syntaxNamesRequested.set([&](int line, int, QStringList *names) {
            const QTextBlock block = data.handler->textCursor().document()
                                         ->findBlockByNumber(line - 1);
            if (block.isValid() && block.text().startsWith("//"))
                names->append("Comment");
        });
        data.doCommand("set runtimepath+=" + D);
        data.doCommand("source " + D + "/plugin/comment.vim");
        data.doCommand("set commentstring=//\\ %s");
        data.doCommand("let g:syntax_on = 1");
        data.setText("/" X "/ one" N "// two" N "int a;" N "// three");
        data.doKeys(keys);
        const QString text = QString::fromUtf8(data.text())
                                 .replace(QLatin1Char(0x0a), QLatin1String(" / "));
        data.doCommand("nunmap gc | nunmap gcc | nunmap gC | xunmap gc");
        data.doCommand("ounmap ic | ounmap ac | xunmap ic | xunmap ac");
        data.doCommand("unlet g:syntax_on | set opfunc=");
        return text;
    };

    // The whole comment block goes, from either mode and by either name.
    QCOMPARE(check("dic"), QLatin1String("int a; / // three"));
    QCOMPARE(check("dac"), QLatin1String("int a; / // three"));
    QCOMPARE(check("vicd"), QLatin1String("int a; / // three"));
    QCOMPARE(check("vacd"), QLatin1String("int a; / // three"));
}

void FakeVimTester::test_vim_script_skipped_subscript()
{
    // What "&&" and "||" pass over is not looked at at all, which is how a
    // script guards an index against a list that is too short. Values taken
    // from Vim 9.1.
    TestData data;
    setup(&data);
    QString message;
    data.handler->commandBufferChanged.set(
        [&](const QString &msg, int, int, int) {
            if (!msg.startsWith("--"))
                message = msg;
        });
    auto echo = [&](const char *expr) -> QString {
        message.clear();
        data.doCommand(QLatin1String("echo ") + QLatin1String(expr));
        return message;
    };

    data.doCommand("let g:l = ['a', 'b'] | let g:i = 4 | let g:d = {'k': 1}");
    // The guarded forms take the other branch and say nothing.
    message.clear();
    data.doCommand("if g:i < len(g:l) && g:l[g:i] ==# 'x' | let g:r = 'taken' "
                   "| else | let g:r = 'not taken' | endif");
    QCOMPARE(message, QLatin1String(""));
    QCOMPARE(echo("g:r"), QLatin1String("not taken"));
    data.doCommand("if 0 && g:l[g:i] ==# 'x' | let g:r = 'taken' "
                   "| else | let g:r = 'not taken' | endif");
    QCOMPARE(echo("g:r"), QLatin1String("not taken"));
    // A key that is not there, guarded the same way.
    data.doCommand("if has_key(g:d, 'zz') && g:d['zz'] == 1 | let g:r = 'taken' "
                   "| else | let g:r = 'not taken' | endif");
    QCOMPARE(echo("g:r"), QLatin1String("not taken"));
    data.doCommand("if has_key(g:d, 'zz') && g:d.zz == 1 | let g:r = 'taken' "
                   "| else | let g:r = 'not taken' | endif");
    QCOMPARE(echo("g:r"), QLatin1String("not taken"));
    // "||" does look at what follows a false left side, so this one does fail.
    QCOMPARE(echo("0 || g:l[g:i] ==# 'x'"),
             QLatin1String("E684: List index out of range: 4"));

    data.doCommand("unlet g:l | unlet g:i | unlet g:d | unlet g:r");
}

void FakeVimTester::test_vim_search_wraps_to_cursor()
{
    // Coming all the way round, the place the cursor sits is the last one left
    // to look at, so a match there is found. Values taken from Vim 9.1.
    TestData data;
    setup(&data);
    QString message;
    data.handler->commandBufferChanged.set(
        [&](const QString &msg, int, int, int) {
            if (!msg.startsWith("--"))
                message = msg;
        });
    auto echo = [&](const char *expr) -> QString {
        message.clear();
        data.doCommand(QLatin1String("echo ") + QLatin1String(expr));
        return message;
    };

    data.setText("x" X "ax" N "yyy");
    QCOMPARE(echo("search('a', 'w')"), QLatin1String("1"));
    // Without wrapping there is nothing after the cursor to find.
    data.setText("x" X "ax" N "yyy");
    QCOMPARE(echo("search('a', 'W')"), QLatin1String("0"));
    data.setText("x" X "ax" N "yyy");
    QCOMPARE(echo("search('a', 'bw')"), QLatin1String("1"));
}

void FakeVimTester::test_vim_pattern_buffer_position()
{
    // "\%23l" and its kin say where in the buffer a match may sit rather than
    // anything about the text. Values taken from Vim 9.1.
    TestData data;
    setup(&data);
    QString message;
    data.handler->commandBufferChanged.set(
        [&](const QString &msg, int, int, int) {
            if (!msg.startsWith("--"))
                message = msg;
        });
    auto echo = [&](const char *expr) -> QString {
        message.clear();
        data.doCommand(QLatin1String("echo ") + QLatin1String(expr));
        return message;
    };
    const char *const start = "aa" X "a" N "bbb" N "ccc" N "ddd";
    data.doCommand("set wrapscan");

    // A search stops only where the position is allowed. "<" is left out of the
    // keys here because the notation would take it for the start of a key name.
    data.setText(start);
    data.doKeys("/\\%3lc<CR>");
    QCOMPARE(data.handler->textCursor().blockNumber() + 1, 3);
    data.setText(start);
    data.doKeys("/\\%>2l\\a<CR>");
    QCOMPARE(data.handler->textCursor().blockNumber() + 1, 3);

    // The "<" form, which finds nothing ahead of the cursor and so walks to the
    // end of the buffer, where the search used to answer with the place it
    // started from over and over.
    data.setText(start);
    data.doKeys("/\\%<2l\\a<CR>");
    QCOMPARE(data.handler->textCursor().blockNumber() + 1, 1);
    data.setText(start);
    data.doCommand("nnoremap Q /\\%<2l\\a<CR>");
    data.doKeys("Q");
    QCOMPARE(data.handler->textCursor().blockNumber() + 1, 1);
    data.doCommand("nunmap Q");

    // search() answers the same way, and takes the forms with a "<" as well.
    data.setText(start);
    QCOMPARE(echo("search('\\%3lc', 'w')"), QLatin1String("3"));
    data.setText(start);
    QCOMPARE(echo("search('\\%<2l\\a', 'w')"), QLatin1String("1"));
    data.setText(start);
    QCOMPARE(echo("search('\\%2cb', 'w')"), QLatin1String("2"));
    // "\%#" is where the cursor is, which is asked before it is moved. A
    // wrapped search reaches it by coming round.
    data.setText("aaa" N "bbb" N "c" X "cc" N "ddd");
    QCOMPARE(echo("search('\\%#', 'wn')"), QLatin1String("3"));

    // A substitution leaves alone what sits elsewhere.
    data.setText(start);
    data.doCommand("%s/\\%2l./X/");
    QCOMPARE(data.text(), QByteArray("aaa" N "Xbb" N "ccc" N "ddd"));
    data.setText(start);
    data.doCommand("%s/\\%<2l./X/");
    QCOMPARE(data.text(), QByteArray("Xaa" N "bbb" N "ccc" N "ddd"));

    // "\%V" is the area the last selection covered, which for a linewise one is
    // whole lines and for an ordinary one runs from the one end to the other.
    data.setText(start);
    data.doKeys("Vj<Esc>");
    data.doCommand("%s/\\%V./Y/g");
    QCOMPARE(data.text(), QByteArray("YYY" N "YYY" N "ccc" N "ddd"));
    data.setText("a" X "aa" N "bbb" N "ccc" N "ddd");
    data.doKeys("vj<Esc>");
    data.doCommand("%s/\\%V./Z/g");
    QCOMPARE(data.text(), QByteArray("aZZ" N "ZZb" N "ccc" N "ddd"));
}

void FakeVimTester::test_vim_set_showmatch_name()
{
    // "sm" is Vim's abbreviation for 'showmatch', which is not implemented
    // here, so it reads as 0 and setting it does nothing. It used to reach
    // 'showmarks', an option of this editor's own that Vim has no name for.
    TestData data;
    setup(&data);
    QString message;
    data.handler->commandBufferChanged.set(
        [&](const QString &msg, int, int, int) {
            if (!msg.startsWith("--"))
                message = msg;
        });
    auto echo = [&](const char *expr) -> QString {
        message.clear();
        data.doCommand(QLatin1String("echo ") + QLatin1String(expr));
        return message;
    };

    data.doCommand("set noshowmarks");
    // Setting "sm" is taken and passed over, and leaves 'showmarks' alone.
    message.clear();
    data.doCommand("set sm");
    QCOMPARE(message, QLatin1String(""));
    QCOMPARE(echo("&sm"), QLatin1String("0"));
    QCOMPARE(echo("&showmarks"), QLatin1String("0"));
    // The option itself still answers to its own name.
    data.doCommand("set showmarks");
    QCOMPARE(echo("&showmarks"), QLatin1String("1"));
    QCOMPARE(echo("&sm"), QLatin1String("0"));
    QCOMPARE(echo("exists('&showmatch')"), QLatin1String("1"));
    data.doCommand("set noshowmarks");
}

void FakeVimTester::test_vim_set_add_remove()
{
    // ":set {option}+=" puts a comma between the parts of a list and nothing
    // between the letters of a flag-style option. Values taken from Vim 9.1.
    TestData data;
    setup(&data);
    QString message;
    data.handler->commandBufferChanged.set(
        [&](const QString &msg, int, int, int) { message = msg; });
    auto echo = [&](const char *expr) -> QString {
        message.clear();
        data.doCommand(QLatin1String("echo ") + QLatin1String(expr));
        return message;
    };

    // Flag-style: the letters stand side by side.
    data.doCommand("set formatoptions=tc | set formatoptions+=j");
    QCOMPARE(echo("&fo"), QLatin1String("tcj"));
    data.doCommand("set formatoptions=tcj | set formatoptions-=c");
    QCOMPARE(echo("&fo"), QLatin1String("tj"));
    data.doCommand("set cpoptions=aA | set cpoptions+=B");
    QCOMPARE(echo("&cpo"), QLatin1String("aAB"));
    data.doCommand("set cpoptions=aAB | set cpoptions^=x");
    QCOMPARE(echo("&cpo"), QLatin1String("xaAB"));
    data.doCommand("set commentstring=#%s | set commentstring+=x");
    QCOMPARE(echo("&cms"), QLatin1String("#%sx"));

    // A list: the parts are separated by commas.
    data.doCommand("set backspace=indent | set backspace+=eol");
    QCOMPARE(echo("&bs"), QLatin1String("indent,eol"));
    data.doCommand("set backspace=indent,eol,start | set backspace-=eol");
    QCOMPARE(echo("&bs"), QLatin1String("indent,start"));
    data.doCommand("set backspace=eol,start | set backspace^=indent");
    QCOMPARE(echo("&bs"), QLatin1String("indent,eol,start"));
    data.doCommand("set iskeyword=a-z | set iskeyword+=A-Z | set iskeyword-=a-z");
    QCOMPARE(echo("&isk"), QLatin1String("A-Z"));

    data.doCommand("set formatoptions=tcq | set backspace=indent,eol,start");
    data.doCommand("set commentstring=/*%s*/");
    data.doCommand("set iskeyword=@,48-57,_,192-255,a-z,A-Z");
}

void FakeVimTester::test_vim_set_escaped_value()
{
    // In ":set {option}={value}" a backslash takes the character after it as it
    // stands, which is how a value holds a space of its own. Every ftplugin and
    // vimrc sets 'commentstring' this way. Values taken from Vim 9.1.
    TestData data;
    setup(&data);
    QString message;
    data.handler->commandBufferChanged.set(
        [&](const QString &msg, int, int, int) { message = msg; });
    auto echo = [&](const char *expr) -> QString {
        message.clear();
        data.doCommand(QLatin1String("echo ") + QLatin1String(expr));
        return message;
    };

    data.doCommand("set commentstring=//\\ %s");
    QCOMPARE(echo("&cms"), QLatin1String("// %s"));
    data.doCommand("set commentstring=/*\\ %s\\ */");
    QCOMPARE(echo("&cms"), QLatin1String("/* %s */"));
    // A backslash before anything else is dropped just the same.
    data.doCommand("set commentstring=x\\%sy");
    QCOMPARE(echo("&cms"), QLatin1String("x%sy"));
    // A value with no backslash is left alone.
    data.doCommand("set commentstring=plain%s");
    QCOMPARE(echo("&cms"), QLatin1String("plain%s"));
    data.doCommand("set commentstring=/*%s*/");
}

void FakeVimTester::test_vim_script_throwpoint()
{
    // "v:throwpoint" says where the exception a ":catch" took was thrown, as
    // the chain of frames that were running, and is put back when the ":try" is
    // left. Values taken from Vim 9.1 running this very script.
    TestData data;
    setup(&data);
    QString message;
    // The mode line arrives after the echo, so it has to be left out or an
    // empty value cannot be told apart from it.
    data.handler->commandBufferChanged.set(
        [&](const QString &msg, int, int, int) {
            if (!msg.startsWith("--"))
                message = msg;
        });
    auto echo = [&](const char *expr) -> QString {
        message.clear();
        data.doCommand(QLatin1String("echo ") + QLatin1String(expr));
        return message;
    };

    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString path = dir.path() + "/t.vim";
    QFile f(path);
    QVERIFY(f.open(QIODevice::WriteOnly));
    f.write("try\n"                              // line 1
            "  throw 'a'\n"                      // line 2
            "catch\n"
            "  let g:atScript = v:throwpoint\n"
            "endtry\n"
            "let g:afterScript = v:throwpoint\n"  // put back on leaving
            "function! Foo()\n"                   // line 7
            "  throw 'b'\n"                       // line 1 of Foo
            "endfunction\n"
            "try\n"
            "  call Foo()\n"                      // line 11
            "catch\n"
            "  let g:atFunc = v:throwpoint\n"
            "endtry\n"
            "try\n"
            "  echo g:nosuchvar\n"                // line 16, an error not a throw
            "catch\n"
            "  let g:atError = v:throwpoint\n"
            "endtry\n"
            "function! Outer()\n"                 // line 20
            "  call Foo()\n"                      // line 1 of Outer
            "endfunction\n"
            "try\n"
            "  call Outer()\n"                    // line 24
            "catch\n"
            "  let g:atNested = v:throwpoint\n"
            "endtry\n");
    f.close();
    data.doCommand("source " + path);
    const QString script = "command line..script " + path;

    // The frame that threw carries the line, the ones that called it the
    // statement they stopped at.
    QCOMPARE(echo("g:atScript"), script + ", line 2");
    QCOMPARE(echo("g:atFunc"), script + "[11]..function Foo, line 1");
    // An error is thrown from where it happened, like ":throw".
    QCOMPARE(echo("g:atError"), script + ", line 16");
    // The word "function" stands before the first one only.
    QCOMPARE(echo("g:atNested"), script + "[24]..function Outer[1]..Foo, line 1");
    // Leaving the ":try" puts it back to what it was, which was nothing.
    QCOMPARE(echo("g:afterScript"), QLatin1String(""));
    QCOMPARE(echo("v:throwpoint"), QLatin1String(""));
    QCOMPARE(echo("v:exception"), QLatin1String(""));

    data.doCommand("unlet g:atScript | unlet g:afterScript | unlet g:atFunc");
    data.doCommand("unlet g:atError | unlet g:atNested");
    data.doCommand("delfunction Foo | delfunction Outer");
}

void FakeVimTester::test_vim_pattern_lookaround()
{
    // "\@=" and its kin make the atom before them something that has to stand
    // there without being part of the match. Values taken from Vim 9.1.
    TestData data;
    setup(&data);
    QString message;
    data.handler->commandBufferChanged.set(
        [&](const QString &msg, int, int, int) { message = msg; });
    auto echo = [&](const char *expr) -> QString {
        message.clear();
        data.doCommand(QLatin1String("echo string(") + QLatin1String(expr) + ')');
        return message;
    };

    // What has to follow, and what must not.
    QCOMPARE(echo("matchstr('foobar', 'foo\\%(bar\\)\\@=')"), QLatin1String("'foo'"));
    QCOMPARE(echo("matchstr('foobaz', 'foo\\%(bar\\)\\@=')"), QLatin1String("''"));
    QCOMPARE(echo("matchstr('foobaz', 'foo\\%(bar\\)\\@!')"), QLatin1String("'foo'"));
    QCOMPARE(echo("matchstr('foobar', 'foo\\%(bar\\)\\@!')"), QLatin1String("''"));
    // What has to stand before, and what must not.
    QCOMPARE(echo("matchstr('foobar', '\\%(foo\\)\\@<=bar')"), QLatin1String("'bar'"));
    QCOMPARE(echo("matchstr('xxbar', '\\%(foo\\)\\@<=bar')"), QLatin1String("''"));
    QCOMPARE(echo("matchstr('xxbar', '\\%(foo\\)\\@<!bar')"), QLatin1String("'bar'"));
    QCOMPARE(echo("matchstr('foobar', '\\%(foo\\)\\@<!bar')"), QLatin1String("''"));
    // A group that gives nothing back once it has matched.
    QCOMPARE(echo("match('aaa', '\\%(a*\\)\\@>a')"), QLatin1String("-1"));
    // The operator applies to a single character as well as to a group.
    QCOMPARE(echo("matchstr('foobar', 'a\\@!foo')"), QLatin1String("'foo'"));
    // What stands before may be of more than one length, as long as there is a
    // longest one.
    QCOMPARE(echo("matchstr('foobar', '\\%(fo\\{1,2}\\)\\@<=bar')"), QLatin1String("'bar'"));
    QCOMPARE(echo("matchstr('foobar', '\\%(foo\\|xx\\)\\@<=bar')"), QLatin1String("'bar'"));
    // With no longest one it cannot be looked for at all here, where Vim finds
    // it: QRegularExpression wants a bounded length behind the match.
    QCOMPARE(echo("matchstr('foobar', '\\%(fo*\\)\\@<=bar')"), QLatin1String("''"));

    // Very magic writes the operator without the backslash, and a backslash
    // makes the "@" itself literal, leaving the "=" to quantify it.
    QCOMPARE(echo("matchstr('foobar', '\\vfoo(bar)@=')"), QLatin1String("'foo'"));
    QCOMPARE(echo("matchstr('foobar', '\\v(foo)@<=bar')"), QLatin1String("'bar'"));
    QCOMPARE(echo("matchstr('foobaz', '\\vfoo(bar)@!')"), QLatin1String("'foo'"));
    QCOMPARE(echo("matchstr('foobar', '\\vfoo(bar)\\@=')"), QLatin1String("'foobar'"));

    // A group inside one is still counted, so what follows keeps its number.
    QCOMPARE(echo("matchlist('foobar', '\\(foo\\)\\@<=\\(bar\\)')[0:2]"),
             QLatin1String("['bar', 'foo', 'bar']"));
}

void FakeVimTester::test_vim_pattern_percent_atoms()
{
    // "\%" opens several atoms of its own. Values taken from Vim 9.1.
    TestData data;
    setup(&data);
    QString message;
    data.handler->commandBufferChanged.set(
        [&](const QString &msg, int, int, int) { message = msg; });
    auto echo = [&](const char *expr) -> QString {
        message.clear();
        data.doCommand(QLatin1String("echo ") + QLatin1String(expr));
        return message;
    };

    // A character by its number, in decimal, hex, octal and as a code point.
    QCOMPARE(echo("match('abc', '\\%d98')"), QLatin1String("1"));
    QCOMPARE(echo("match('abc', '\\%x62')"), QLatin1String("1"));
    QCOMPARE(echo("match('abc', '\\%o142')"), QLatin1String("1"));
    QCOMPARE(echo("match('abc', '\\%u0062')"), QLatin1String("1"));
    // A sequence in which each character may be the last.
    QCOMPARE(echo("match('abc', '\\%[abc]')"), QLatin1String("0"));
    QCOMPARE(echo("match('abc', 'a\\%[bc]')"), QLatin1String("0"));
    QCOMPARE(echo("match('xa', 'a\\%[bc]')"), QLatin1String("1"));
    QCOMPARE(echo("match('xab', 'a\\%[bc]')"), QLatin1String("1"));
    QCOMPARE(echo("matchstr('abcd', 'a\\%[bc]')"), QLatin1String("abc"));
    QCOMPARE(echo("match('xyz', 'a\\%[bc]')"), QLatin1String("-1"));

    // A group that does not capture.
    QCOMPARE(echo("match('abcabc', '\\%(abc\\)\\{2}')"), QLatin1String("0"));
}

void FakeVimTester::test_vim_pattern_very_magic()
{
    // Where very magic gives punctuation a meaning, a backslash takes it away
    // again. Values taken from Vim 9.1.
    TestData data;
    setup(&data);
    QString message;
    data.handler->commandBufferChanged.set(
        [&](const QString &msg, int, int, int) { message = msg; });
    auto echo = [&](const char *expr) -> QString {
        message.clear();
        data.doCommand(QLatin1String("echo ") + QLatin1String(expr));
        return message;
    };

    // "\=" is an "=" here, not "0 or 1 of the atom before it". Reading it the
    // other way put the wrong text in the groups around it.
    QCOMPARE(echo("string(matchlist('a=b', '\\v^(\\w+)\\s*(\\=)\\s*(.*)$'))"),
             QLatin1String("['a=b', 'a', '=', 'b', '', '', '', '', '', '']"));
    QCOMPARE(echo("match('a=b', '\\v\\=')"), QLatin1String("1"));
    QCOMPARE(echo("match('ab', '\\va\\=b')"), QLatin1String("-1"));
    QCOMPARE(echo("match('a=b', '\\va\\=b')"), QLatin1String("0"));
    // Magic is the other way round: there "\=" is the quantifier.
    QCOMPARE(echo("match('ab', 'a\\=b')"), QLatin1String("0"));
    // The same holds for "\<" and "\>", a word boundary only where "<" and ">"
    // do not already mean one.
    QCOMPARE(echo("match('a<b', '\\v\\<')"), QLatin1String("1"));
    QCOMPARE(echo("match('word here', '\\v\\<here\\>')"), QLatin1String("-1"));
    QCOMPARE(echo("match('word here', '\\<here\\>')"), QLatin1String("5"));
}

void FakeVimTester::test_vim_script_lockvar()
{
    // ":lockvar" holds a variable against being given another value, which is
    // how a script keeps a constant. Values taken from Vim 9.1.
    TestData data;
    setup(&data);
    QString message;
    data.handler->commandBufferChanged.set(
        [&](const QString &msg, int, int, int) { message = msg; });
    auto echo = [&](const char *expr) -> QString {
        message.clear();
        data.doCommand(QLatin1String("echo ") + QLatin1String(expr));
        return message;
    };

    data.doCommand("let g:y = 1");
    data.doCommand("lockvar g:y");
    data.doCommand("let g:y = 2");
    QCOMPARE(echo("g:y"), QLatin1String("1"));
    data.doCommand("unlockvar g:y");
    data.doCommand("let g:y = 3");
    QCOMPARE(echo("g:y"), QLatin1String("3"));
    // Abbreviated, and with the depth a script asks for.
    data.doCommand("lockv g:y");
    data.doCommand("unlo g:y");
    data.doCommand("let g:y = 4");
    QCOMPARE(echo("g:y"), QLatin1String("4"));
    data.doCommand("lockvar 3 g:y");
    data.doCommand("let g:y = 9");
    QCOMPARE(echo("g:y"), QLatin1String("4"));
    // A script can catch the attempt, as in Vim. A block spans several lines,
    // so this runs as a script: driven one line at a time every line runs,
    // block or no block, and the catch would prove nothing.
    QTemporaryDir lockDir;
    QVERIFY(lockDir.isValid());
    QFile lf(lockDir.path() + "/l.vim");
    QVERIFY(lf.open(QIODevice::WriteOnly));
    lf.write("let g:caught = 'no'\n"
             "try\n"
             "  let g:y = 9\n"
             "  let g:caught = 'assigned'\n"
             "catch /locked/\n"
             "  let g:caught = 'caught'\n"
             "endtry\n");
    lf.close();
    data.doCommand("source " + lockDir.path() + "/l.vim");
    QCOMPARE(echo("g:caught"), QLatin1String("caught"));
    QCOMPARE(echo("g:y"), QLatin1String("4"));
    data.doCommand("unlockvar! g:y");
    data.doCommand("let g:y = 5");
    QCOMPARE(echo("g:y"), QLatin1String("5"));
    // Locking does not stand in the way of ":unlet".
    data.doCommand("lockvar g:y");
    data.doCommand("unlet g:y");
    QCOMPARE(echo("exists('g:y')"), QLatin1String("0"));
    data.doCommand("unlockvar g:y | unlet g:caught");
}

void FakeVimTester::test_vim_script_messages()
{
    // Only one message fits in the mini buffer, so a script that has several
    // things to say was heard once only. ":messages" reports them all, and what
    // it keeps is what Vim keeps. Values taken from Vim 9.1.
    TestData data;
    setup(&data);
    QString info;
    data.handler->extraInformationChanged.set([&](const QString &text) { info = text; });
    auto messages = [&]() -> QString {
        info.clear();
        data.doCommand("messages");
        return info;
    };

    data.doCommand("messages clear");
    QCOMPARE(messages(), QLatin1String("\n"));
    data.doCommand("echomsg 'first'");
    data.doCommand("echomsg 'second'");
    QCOMPARE(messages(), QLatin1String("first\nsecond\n"));
    // What ":echo" prints is not kept, nor is a message ":silent" swallowed.
    data.doCommand("echo 'printed'");
    data.doCommand("silent echomsg 'hushed'");
    QCOMPARE(messages(), QLatin1String("first\nsecond\n"));
    // An error is kept.
    data.doCommand("echoerr 'went wrong'");
    QVERIFY(messages().contains("went wrong"));
    data.doCommand("messages clear");
    QCOMPARE(messages(), QLatin1String("\n"));
}

void FakeVimTester::test_vim_script_split()
{
    // Values taken from Vim 9.1.
    TestData data;
    setup(&data);
    QString message;
    data.handler->commandBufferChanged.set(
        [&](const QString &msg, int, int, int) { message = msg; });
    auto echo = [&](const char *expr) -> QString {
        message.clear();
        data.doCommand(QLatin1String("echo string(") + QLatin1String(expr) + ')');
        return message;
    };

    // A separator that matches nothing at all still separates: every place it
    // matches ends a piece, which is how a string is taken apart character by
    // character. Plugins do this to walk a pattern.
    QCOMPARE(echo("split('a*b', '\\zs')"), QLatin1String("['a', '*', 'b']"));
    // An empty separator is not one that matches everywhere; it is the default,
    // a run of whitespace.
    QCOMPARE(echo("split('ab', '')"), QLatin1String("['ab']"));
    QCOMPARE(echo("split('  a b  ')"), QLatin1String("['a', 'b']"));
    // Only the first and last piece are dropped when empty, not one in between.
    QCOMPARE(echo("split('a,,b', ',')"), QLatin1String("['a', '', 'b']"));
    QCOMPARE(echo("split('a,,b', ',', 1)"), QLatin1String("['a', '', 'b']"));
    // The separator is a pattern, not a string.
    QCOMPARE(echo("split('a1b22c', '\\d\\+')"), QLatin1String("['a', 'b', 'c']"));
}

void FakeVimTester::test_vim_script_pattern_newline()
{
    // "\_x" is the atom x with a line break allowed as well. Values taken from
    // Vim 9.1.
    TestData data;
    setup(&data);
    QString message;
    data.handler->commandBufferChanged.set(
        [&](const QString &msg, int, int, int) { message = msg; });
    auto echo = [&](const char *expr) -> QString {
        message.clear();
        data.doCommand(QLatin1String("echo ") + QLatin1String(expr));
        return message;
    };

    // "\_." is any character or a line break.
    QCOMPARE(echo("match(\"a\\nb\", 'a\\_.b')"), QLatin1String("0"));
    QCOMPARE(echo("match('abc', 'a\\_.c')"), QLatin1String("0"));
    // "\_$" is the end of a line, which for a string is its end only.
    QCOMPARE(echo("match('abc', '\\_$')"), QLatin1String("3"));
    QCOMPARE(echo("match('abc', 'a\\_$')"), QLatin1String("-1"));
    QCOMPARE(echo("match(\"ab\\ncd\", 'b\\_$')"), QLatin1String("-1"));
    QCOMPARE(echo("match(\"a\\nb\", '\\_^b')"), QLatin1String("-1"));
    // A class takes one in as well, negated or not.
    QCOMPARE(echo("match('abc', '\\_[xy]')"), QLatin1String("-1"));
    QCOMPARE(echo("match('abc', '\\_[^x]')"), QLatin1String("0"));
    QCOMPARE(echo("match('abc', '\\_s')"), QLatin1String("-1"));
    // Very magic reads them the same way. This is the shape the editorconfig
    // plugin matches a file name against a section of its configuration with.
    QCOMPARE(echo("match('abc', '\\v\\_.*c\\_$')"), QLatin1String("0"));
    QCOMPARE(echo("match('/tmp/x/t.cpp', '\\v\\/tmp\\/x\\_.*\\/[^/]*\\_$')"),
             QLatin1String("0"));
}

void FakeVimTester::test_vim_script_known_options()
{
    // Vim keeps the name of every option even where the feature behind it is
    // missing, as 'shellslash' is on a system with one kind of slash only:
    // reading it gives 0 or an empty string and setting it does nothing, so a
    // script that consults it runs anyway. Values taken from Vim 9.1 on Linux.
    TestData data;
    setup(&data);
    QString message;
    data.handler->commandBufferChanged.set(
        [&](const QString &msg, int, int, int) { message = msg; });
    auto echo = [&](const char *expr) -> QString {
        message.clear();
        data.doCommand(QLatin1String("echo ") + QLatin1String(expr));
        return message;
    };

    QCOMPARE(echo("&shellslash"), QLatin1String("0"));
    QCOMPARE(echo("&ssl"), QLatin1String("0"));            // by abbreviation too
    QCOMPARE(echo("string(&guifont)"), QLatin1String("''"));
    // Which is what makes the common guarded form work at all.
    QCOMPARE(echo("exists('+shellslash') && !&shellslash ? 'a' : 'b'"),
             QLatin1String("b"));
    // "&opt" asks whether Vim has the option, "+opt" whether it can be used.
    QCOMPARE(echo("exists('&shellslash')"), QLatin1String("1"));
    QCOMPARE(echo("exists('+shellslash')"), QLatin1String("0"));
    QCOMPARE(echo("exists('&shiftwidth')"), QLatin1String("1"));
    QCOMPARE(echo("exists('+shiftwidth')"), QLatin1String("1"));
    // A name Vim does not have is still an error.
    QCOMPARE(echo("exists('&nosuchoption')"), QLatin1String("0"));
    message.clear();
    data.doCommand("echo &nosuchoption");
    QVERIFY(message.contains("nosuchoption"));

    // Setting one is accepted and leaves it as it was.
    data.doCommand("set shellslash");
    QCOMPARE(echo("&shellslash"), QLatin1String("0"));
    data.doCommand("let &guifont = 'zz'");
    QCOMPARE(echo("string(&guifont)"), QLatin1String("''"));
    message.clear();
    data.doCommand("set nosuchoption");
    QVERIFY(message.contains("nosuchoption"));
}

void FakeVimTester::test_vim_script_trailing_comment()
{
    // A '"' where an expression has already ended begins a comment, which is
    // how a script explains the line it is setting something on. Values taken
    // from Vim 9.1.
    TestData data;
    setup(&data);
    QString message;
    data.handler->commandBufferChanged.set(
        [&](const QString &msg, int, int, int) { message = msg; });
    auto echo = [&](const char *expr) -> QString {
        message.clear();
        data.doCommand(QLatin1String("echo ") + QLatin1String(expr));
        return message;
    };

    data.doCommand("let g:x = 'v'   \" a trailing comment");
    QCOMPARE(echo("g:x"), QLatin1String("v"));
    // A '"' that opens the expression is still a string, not a comment.
    data.doCommand("let g:s = \"str\"");
    QCOMPARE(echo("g:s"), QLatin1String("str"));
    // ... and one inside a string is just a character.
    data.doCommand("let g:q = 'has \" inside'");
    QCOMPARE(echo("g:q"), QLatin1String("has \" inside"));
    // A condition may carry one too, which only a block shows: a false one has
    // to skip its body, so this runs as a script rather than line by line.
    QTemporaryDir ifDir;
    QVERIFY(ifDir.isValid());
    QFile cf(ifDir.path() + "/c.vim");
    QVERIFY(cf.open(QIODevice::WriteOnly));
    cf.write("let g:n = 0\n"
             "if 0   \" a comment on the condition\n"
             "  let g:n = 5\n"
             "endif\n"
             "if 1   \" and on one that holds\n"
             "  let g:n = 7\n"
             "endif\n");
    cf.close();
    data.doCommand("source " + ifDir.path() + "/c.vim");
    QCOMPARE(echo("g:n"), QLatin1String("7"));
    // ":echo" is the exception: it takes several expressions, so a '"' there
    // opens another string rather than a comment.
    QCOMPARE(echo("'a' 'b'"), QLatin1String("a b"));
    QCOMPARE(echo("\"a\" \"b\""), QLatin1String("a b"));
    data.doCommand("unlet g:x | unlet g:s | unlet g:q | unlet g:n");
}

void FakeVimTester::test_vim_script_if_chain()
{
    // The shape a plugin settles a choice with: a block that fills in a
    // default, another that is skipped, then a chain whose last arm reports
    // not knowing what to do. Reaching that arm anyway is the sign the chain
    // was read wrongly. Values taken from Vim 9.1.
    TestData data;
    setup(&data);
    QString message;
    data.handler->commandBufferChanged.set(
        [&](const QString &msg, int, int, int) { message = msg; });
    auto echo = [&](const char *expr) -> QString {
        message.clear();
        data.doCommand(QLatin1String("echo ") + QLatin1String(expr));
        return message;
    };
    auto source = [&](const char *text) {
        QTemporaryFile file;
        QVERIFY(file.open());
        file.write(text);
        file.flush();
        data.doCommand(QLatin1String("source ") + file.fileName());
    };

    source("let s:mode = ''\n"
           "function! s:Init()\n"
           "  if empty(s:mode)\n"
           "    let s:mode = 'vim_core'\n"
           "  endif\n"
           "\n"
           "  if s:mode ==? 'external'\n"
           "    if 0\n"
           "      let s:mode = 'never'\n"
           "    endif\n"
           "  endif\n"
           "\n"
           "  if s:mode ==? 'vim_core'\n"
           "    if 0\n"
           "      return 'vimcore-failed'\n"
           "    endif\n"
           "  elseif s:mode ==? 'external'\n"
           "    \" nothing to do here\n"
           "  else\n"
           "    return 'unknown:' . s:mode\n"
           "  endif\n"
           "\n"
           "  return 'ok:' . s:mode\n"
           "endfunction\n"
           "let g:ret = s:Init()\n"
           "let g:mode = s:mode\n");
    QCOMPARE(echo("g:ret"), QLatin1String("ok:vim_core"));
    QCOMPARE(echo("g:mode"), QLatin1String("vim_core"));
    data.doCommand("unlet g:ret | unlet g:mode");
}

void FakeVimTester::test_vim_script_scriptlocal()
{
    // A script-scope variable set inside a function is the same one the script
    // sees, which is how a plugin settles a choice once and reads it back
    // later. Values taken from Vim 9.1.
    TestData data;
    setup(&data);
    QString message;
    data.handler->commandBufferChanged.set(
        [&](const QString &msg, int, int, int) { message = msg; });
    auto echo = [&](const char *expr) -> QString {
        message.clear();
        data.doCommand(QLatin1String("echo ") + QLatin1String(expr));
        return message;
    };
    auto source = [&](const char *text) {
        QTemporaryFile file;
        QVERIFY(file.open());
        file.write(text);
        file.flush();
        data.doCommand(QLatin1String("source ") + file.fileName());
    };

    source("let s:mode = ''\n"
           "let s:flag = 0\n"
           "function! s:Pick()\n"
           "  if empty(s:mode)\n"
           "    let s:mode = 'chosen'\n"
           "  endif\n"
           "  let s:flag = 1\n"
           "  return s:mode\n"
           "endfunction\n"
           "let g:before = s:mode\n"
           "let g:ret = s:Pick()\n"
           "let g:after = s:mode\n"
           "let g:flag = s:flag\n");
    QCOMPARE(echo("'[' . g:before . ']'"), QLatin1String("[]"));
    QCOMPARE(echo("g:ret"), QLatin1String("chosen"));
    QCOMPARE(echo("g:after"), QLatin1String("chosen"));
    QCOMPARE(echo("g:flag"), QLatin1String("1"));
    data.doCommand("unlet g:before | unlet g:ret | unlet g:after | unlet g:flag");
}

void FakeVimTester::test_vim_script_autoload()
{
    // A "#" name is loaded from the script it lives in, along the runtimepath,
    // the first time it is needed. Values taken from Vim 9.1.
    TestData data;
    setup(&data);
    QString message;
    data.handler->commandBufferChanged.set(
        [&](const QString &msg, int, int, int) { message = msg; });
    auto echo = [&](const char *expr) -> QString {
        message.clear();
        data.doCommand(QLatin1String("echo ") + QLatin1String(expr));
        return message;
    };

    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    QVERIFY(QDir(dir.path()).mkpath("autoload/deep"));
    QFile lib(dir.path() + "/autoload/mylib.vim");
    QVERIFY(lib.open(QIODevice::WriteOnly));
    lib.write("let g:mylib_loaded = 1\n"
              "function! mylib#greet(who)\n"
              "  return 'hello ' . a:who\n"
              "endfunction\n");
    lib.close();
    QFile nested(dir.path() + "/autoload/deep/nested.vim");
    QVERIFY(nested.open(QIODevice::WriteOnly));
    nested.write("function! deep#nested#answer()\n"
                 "  return 42\n"
                 "endfunction\n");
    nested.close();

    data.doCommand("set runtimepath+=" + dir.path());
    QCOMPARE(echo("&rtp =~ 'autoload' ? 0 : 1"), QLatin1String("1")); // the dir, not its parts
    // Nothing is read until the name is wanted.
    QCOMPARE(echo("exists('g:mylib_loaded')"), QLatin1String("0"));
    QCOMPARE(echo("mylib#greet('world')"), QLatin1String("hello world"));
    QCOMPARE(echo("exists('g:mylib_loaded')"), QLatin1String("1"));
    // A deeper name lives a directory further down.
    QCOMPARE(echo("deep#nested#answer()"), QLatin1String("42"));
    // One that is nowhere to be found says so rather than searching forever.
    QCOMPARE(echo("nosuchlib#nope()"), QLatin1String("E117: Unknown function: nosuchlib#nope"));
    data.doCommand("set runtimepath=");
}

void FakeVimTester::test_vim_script_augroup()
{
    // ":augroup" puts the autocommands that follow in a group, which is how a
    // plugin clears its own without touching anyone else's. Expected values
    // taken from Vim 9.1.
    TestData data;
    setup(&data);
    QString message;
    data.handler->commandBufferChanged.set(
        [&](const QString &msg, int, int, int) { message = msg; });
    auto echo = [&](const char *expr) -> QString {
        message.clear();
        data.doCommand(QLatin1String("echo ") + QLatin1String(expr));
        return message;
    };
    auto source = [&](const char *text) {
        QTemporaryFile file;
        QVERIFY(file.open());
        file.write(text);
        file.flush();
        data.doCommand(QLatin1String("source ") + file.fileName());
    };

    data.doCommand("autocmd!");
    source("let g:a = 0\n"
           "let g:b = 0\n"
           "augroup grpA\n"
           "  autocmd!\n"
           "  autocmd FileType zz let g:a += 1\n"
           "augroup END\n"
           "augroup grpB\n"
           "  autocmd!\n"
           "  autocmd FileType zz let g:b += 1\n"
           "augroup END\n"
           "set ft=zz\n");
    QCOMPARE(echo("g:a . ',' . g:b"), QLatin1String("1,1"));

    // Clearing one group leaves the other in place.
    source("augroup grpA\n"
           "  autocmd!\n"
           "augroup END\n"
           "let g:a = 0\n"
           "let g:b = 0\n"
           "set ft=\n"
           "set ft=zz\n");
    QCOMPARE(echo("g:a . ',' . g:b"), QLatin1String("0,1"));

    // One command may serve several events, written with commas between them,
    // which is how nearly every plugin registers. Any event named has to be
    // recognized or the whole list is misread as a group name.
    source("let g:n = 0\n"
           "autocmd BufNewFile,BufReadPost,BufFilePost * let g:n += 1\n"
           "doautocmd BufReadPost\n");
    QCOMPARE(echo("g:n"), QLatin1String("1"));
    // The one that does not fire here is still accepted, not an error.
    source("let g:n = 0\n"
           "doautocmd BufNewFile\n");
    QCOMPARE(echo("g:n"), QLatin1String("1"));
    data.doCommand("autocmd!");

    // A group named on the command itself works the same way.
    source("let g:b = 0\n"
           "autocmd grpB FileType yy let g:b += 10\n"
           "set ft=\n"
           "set ft=yy\n");
    QCOMPARE(echo("g:b"), QLatin1String("10"));
    source("autocmd! grpB\n"
           "let g:b = 0\n"
           "set ft=\n"
           "set ft=yy\n");
    QCOMPARE(echo("g:b"), QLatin1String("0"));

    data.doCommand("autocmd!");
}

void FakeVimTester::test_vim_script_autocmd()
{
    // :autocmd registration, :doautocmd, pattern matching and firing on :w.
    TestData data;
    setup(&data);
    QString message;
    data.handler->commandBufferChanged.set(
        [&](const QString &msg, int, int, int) { message = msg; });
    auto echo = [&](const char *expr) -> QString {
        message.clear();
        data.doCommand(QLatin1String("echo ") + QLatin1String(expr));
        return message;
    };

    // :doautocmd fires a registered command.
    data.doCommand("let g:hit = 0");
    data.doCommand("autocmd BufWritePre * let g:hit = 1");
    data.doCommand("doautocmd BufWritePre");
    QCOMPARE(echo("g:hit"), QLatin1String("1"));

    // The file pattern selects which autocommands run.
    data.handler->setCurrentFileName("example.txt");
    data.doCommand("autocmd BufReadPost *.txt let g:txt = 1");
    data.doCommand("autocmd BufReadPost *.cpp let g:cpp = 1");
    data.doCommand("doautocmd BufReadPost");
    QCOMPARE(echo("g:txt"), QLatin1String("1"));
    QCOMPARE(echo("exists('g:cpp')"), QLatin1String("0"));

    // :w fires BufWritePre and BufWritePost.
    data.doCommand("let g:pre = 0 | let g:post = 0");
    data.doCommand("autocmd BufWritePre * let g:pre = 1");
    data.doCommand("autocmd BufWritePost * let g:post = 1");
    QTemporaryFile wf;
    QVERIFY(wf.open());
    const QString writePath = wf.fileName();
    data.doCommand(QLatin1String("w! ") + writePath);
    QCOMPARE(echo("g:pre"), QLatin1String("1"));
    QCOMPARE(echo("g:post"), QLatin1String("1"));

    // InsertEnter / InsertLeave fire on the mode transitions.
    data.doCommand("let g:ie = 0 | let g:il = 0");
    data.doCommand("autocmd InsertEnter * let g:ie = 1");
    data.doCommand("autocmd InsertLeave * let g:il = 1");
    data.setText("");
    data.doKeys("i");
    data.doKeys("<ESC>");
    QCOMPARE(echo("g:ie"), QLatin1String("1"));
    QCOMPARE(echo("g:il"), QLatin1String("1"));

    // FileType matches the filetype (set via :set ft= or :setf), not the name.
    data.doCommand("autocmd FileType python let g:py = 1");
    data.doCommand("autocmd FileType c let g:onlyc = 1");
    data.doCommand("set filetype=python");
    QCOMPARE(echo("g:py"), QLatin1String("1"));
    QCOMPARE(echo("exists('g:onlyc')"), QLatin1String("0"));
    data.doCommand("setf c");
    QCOMPARE(echo("g:onlyc"), QLatin1String("1"));

    // :autocmd! removes all autocommands.
    data.doCommand("autocmd!");
    data.doCommand("let g:hit = 0");
    data.doCommand("doautocmd BufWritePre");
    QCOMPARE(echo("g:hit"), QLatin1String("0"));
}

void FakeVimTester::test_vim_script_dict_dot()
{
    // "d.key" dictionary access, and "." still concatenating for non-dicts.
    TestData data;
    setup(&data);
    QString message;
    data.handler->commandBufferChanged.set(
        [&](const QString &msg, int, int, int) { message = msg; });
    auto echo = [&](const char *expr) -> QString {
        message.clear();
        data.doCommand(QLatin1String("echo ") + QLatin1String(expr));
        return message;
    };

    // Read access, including chained.
    QCOMPARE(echo("{\"a\": 1, \"b\": 2}.b"), QLatin1String("2"));
    data.doCommand("let g:d = {\"x\": {\"y\": 5}}");
    QCOMPARE(echo("g:d.x.y"), QLatin1String("5"));
    QCOMPARE(echo("g:d.x.y + 1"), QLatin1String("6"));

    // "." is still concatenation for strings, with or without surrounding space.
    QCOMPARE(echo("\"a\" . \"b\""), QLatin1String("ab"));
    QCOMPARE(echo("\"a\".\"b\""), QLatin1String("ab"));
    data.doCommand("let g:s = \"x\"");
    QCOMPARE(echo("g:s.\"y\""), QLatin1String("xy"));

    // Write via ":let d.key = v", with compound and nested forms.
    data.doCommand("let g:m = {}");
    data.doCommand("let g:m.a = 1");
    data.doCommand("let g:m.b = 2");
    QCOMPARE(echo("g:m"), QLatin1String("{'a': 1, 'b': 2}"));
    data.doCommand("let g:m.a += 5");
    QCOMPARE(echo("g:m.a"), QLatin1String("6"));
    data.doCommand("let g:n = {\"sub\": {}}");
    data.doCommand("let g:n.sub.x = 9");
    QCOMPARE(echo("g:n"), QLatin1String("{'sub': {'x': 9}}"));
}

void FakeVimTester::test_vim_script_command()
{
    // ":command" user-defined commands and their <...> argument tokens.
    TestData data;
    setup(&data);
    QString message;
    data.handler->commandBufferChanged.set(
        [&](const QString &msg, int, int, int) { message = msg; });
    auto echo = [&](const char *expr) -> QString {
        message.clear();
        data.doCommand(QLatin1String("echo ") + QLatin1String(expr));
        return message;
    };

    data.doCommand("command Hello let g:greeted = 1");
    data.doCommand("Hello");
    QCOMPARE(echo("g:greeted"), QLatin1String("1"));

    // <args> and <q-args>.
    data.doCommand("command SetX let g:x = <args>");
    data.doCommand("SetX 42");
    QCOMPARE(echo("g:x"), QLatin1String("42"));
    data.doCommand("command Say let g:said = <q-args>");
    data.doCommand("Say hi there");
    QCOMPARE(echo("g:said"), QLatin1String("hi there"));

    // <bang>.
    data.doCommand("command Bang let g:bang = \"<bang>\"");
    data.doCommand("Bang!");
    QCOMPARE(echo("\"[\" . g:bang . \"]\""), QLatin1String("[!]"));
    data.doCommand("Bang");
    QCOMPARE(echo("\"[\" . g:bang . \"]\""), QLatin1String("[]"));

    // Attributes are accepted (and ignored).
    data.doCommand("command -nargs=1 Double let g:dbl = <args> * 2");
    data.doCommand("Double 21");
    QCOMPARE(echo("g:dbl"), QLatin1String("42"));

    // <f-args> for passing to a function.
    data.doCommand("function Store(a, b) | let g:pair = a:a . \",\" . a:b | endfunction");
    data.doCommand("command Pair call Store(<f-args>)");
    data.doCommand("Pair x y");
    QCOMPARE(echo("g:pair"), QLatin1String("x,y"));

    // :delcommand removes it (so it no longer runs).
    data.doCommand("command Temp let g:temp = 1");
    data.doCommand("delcommand Temp");
    data.doCommand("let g:temp = 0");
    data.doCommand("Temp");
    QCOMPARE(echo("g:temp"), QLatin1String("0"));
}

void FakeVimTester::test_vim9_basics()
{
    // Vim9-script mode basics: "#" comments, ".." concat, true/false literals.
    TestData data;
    setup(&data);
    QString message;
    data.handler->commandBufferChanged.set(
        [&](const QString &msg, int, int, int) { message = msg; });
    auto echo = [&](const char *expr) -> QString {
        message.clear();
        data.doCommand(QLatin1String("echo ") + QLatin1String(expr));
        return message;
    };
    auto source = [&](const char *text) {
        QTemporaryFile file;
        QVERIFY(file.open());
        file.write(text);
        file.flush();
        data.doCommand(QLatin1String("source ") + file.fileName());
    };

    source("vim9script\n"
           "# a hash-comment line\n"
           "let g:cat = \"a\" .. \"b\"\n"
           "let g:t = true\n"
           "let g:f = false\n"
           "let g:keep = 5\n"
           "# let g:keep = 99\n");
    QCOMPARE(echo("g:cat"), QLatin1String("ab")); // ".." concatenates
    QCOMPARE(echo("g:t"), QLatin1String("1"));     // true literal
    QCOMPARE(echo("g:f"), QLatin1String("0"));     // false literal
    QCOMPARE(echo("g:keep"), QLatin1String("5"));  // "#" line was a comment

    // var/const declarations and plain assignment (no :let).
    source("vim9script\n"
           "var x = 10\n"
           "const y = 32\n"
           "g:sum = x + y\n"
           "var s = \"a\"\n"
           "s ..= \"bc\"\n"
           "g:s = s\n"
           "var n: number = 7\n"
           "g:typed = n\n"
           "g:sum += 100\n");
    QCOMPARE(echo("g:sum"), QLatin1String("142"));
    QCOMPARE(echo("g:s"), QLatin1String("abc"));
    QCOMPARE(echo("g:typed"), QLatin1String("7"));

    // A legacy (non-vim9) sourced file still uses "\"" comments and "." concat.
    source("let g:legacy = \"x\" . \"y\"\n"
           "\" let g:legacy = \"z\"\n");
    QCOMPARE(echo("g:legacy"), QLatin1String("xy"));
}

void FakeVimTester::test_vim9_def()
{
    // Vim9 ":def" functions: typed signature, bare-name args, default args,
    // local var/for, and calling them.
    TestData data;
    setup(&data);
    QString message;
    data.handler->commandBufferChanged.set(
        [&](const QString &msg, int, int, int) { message = msg; });
    auto echo = [&](const char *expr) -> QString {
        message.clear();
        data.doCommand(QLatin1String("echo ") + QLatin1String(expr));
        return message;
    };
    auto source = [&](const char *text) {
        QTemporaryFile file;
        QVERIFY(file.open());
        file.write(text);
        file.flush();
        data.doCommand(QLatin1String("source ") + file.fileName());
    };

    source("vim9script\n"
           "def Add(a: number, b: number): number\n"
           "  return a + b\n"
           "enddef\n"
           "def Greet(name: string): string\n"
           "  return \"Hello, \" .. name\n"
           "enddef\n"
           "def WithDefault(x = 5): number\n"
           "  return x * 2\n"
           "enddef\n"
           "def Total(l: list<number>): number\n"
           "  var t = 0\n"
           "  for n in l\n"
           "    t += n\n"
           "  endfor\n"
           "  return t\n"
           "enddef\n"
           "g:sum = Add(3, 4)\n"
           "g:greet = Greet(\"Bob\")\n"
           "g:d1 = WithDefault()\n"
           "g:d2 = WithDefault(10)\n"
           "g:total = Total([1, 2, 3, 4])\n");
    QCOMPARE(echo("g:sum"), QLatin1String("7"));
    QCOMPARE(echo("g:greet"), QLatin1String("Hello, Bob"));
    QCOMPARE(echo("g:d1"), QLatin1String("10"));
    QCOMPARE(echo("g:d2"), QLatin1String("20"));
    QCOMPARE(echo("g:total"), QLatin1String("10"));
}

void FakeVimTester::test_vim9_lambda()
{
    // Vim9 "(args) => expr" lambdas, including as arguments to map/sort.
    TestData data;
    setup(&data);
    QString message;
    data.handler->commandBufferChanged.set(
        [&](const QString &msg, int, int, int) { message = msg; });
    auto echo = [&](const char *expr) -> QString {
        message.clear();
        data.doCommand(QLatin1String("echo ") + QLatin1String(expr));
        return message;
    };
    auto source = [&](const char *text) {
        QTemporaryFile file;
        QVERIFY(file.open());
        file.write(text);
        file.flush();
        data.doCommand(QLatin1String("source ") + file.fileName());
    };

    source("vim9script\n"
           "var Double = (x) => x * 2\n"
           "g:a = Double(21)\n"
           "var Add = (a, b) => a + b\n"
           "g:b = Add(3, 4)\n"
           "var Inc = (x: number) => x + 1\n"
           "g:c = Inc(9)\n"
           "g:m = map([1, 2, 3], (i, v) => v * 10)\n"
           "g:s = sort([3, 1, 2], (a, b) => a - b)\n");
    QCOMPARE(echo("g:a"), QLatin1String("42"));
    QCOMPARE(echo("g:b"), QLatin1String("7"));
    QCOMPARE(echo("g:c"), QLatin1String("10"));
    QCOMPARE(echo("g:m"), QLatin1String("[10, 20, 30]"));
    QCOMPARE(echo("g:s"), QLatin1String("[1, 2, 3]"));
}

void FakeVimTester::test_vim9_continuation()
{
    // Vim9 implicit line continuation: unclosed brackets and leading/trailing
    // operators.
    TestData data;
    setup(&data);
    QString message;
    data.handler->commandBufferChanged.set(
        [&](const QString &msg, int, int, int) { message = msg; });
    auto echo = [&](const char *expr) -> QString {
        message.clear();
        data.doCommand(QLatin1String("echo ") + QLatin1String(expr));
        return message;
    };
    auto source = [&](const char *text) {
        QTemporaryFile file;
        QVERIFY(file.open());
        file.write(text);
        file.flush();
        data.doCommand(QLatin1String("source ") + file.fileName());
    };

    source("vim9script\n"
           "var l = [\n"
           "  1,\n"
           "  2,\n"
           "  3,\n"
           "]\n"
           "g:l = l\n"
           "var d = {\n"
           "  'a': 1,\n"
           "  'b': 2,\n"
           "}\n"
           "g:d = d\n"
           "var s = \"x\"\n"
           "      .. \"y\"\n"
           "      .. \"z\"\n"
           "g:s = s\n"
           "var n = 1 +\n"
           "        2 +\n"
           "        3\n"
           "g:n = n\n"
           "g:pf = printf(\"%d-%d\",\n"
           "              10,\n"
           "              20)\n");
    QCOMPARE(echo("g:l"), QLatin1String("[1, 2, 3]"));
    QCOMPARE(echo("g:d"), QLatin1String("{'a': 1, 'b': 2}"));
    QCOMPARE(echo("g:s"), QLatin1String("xyz"));
    QCOMPARE(echo("g:n"), QLatin1String("6"));
    QCOMPARE(echo("g:pf"), QLatin1String("10-20"));
}

void FakeVimTester::test_vim9_interpolation()
{
    // String interpolation $"...{expr}..." / $'...{expr}...'; "{{"/"}}" are
    // literal braces; double quotes also honor backslash escapes.
    TestData data;
    setup(&data);
    QString message;
    data.handler->commandBufferChanged.set(
        [&](const QString &msg, int, int, int) { message = msg; });
    auto echo = [&](const char *expr) -> QString {
        message.clear();
        data.doCommand(QLatin1String("echo ") + QLatin1String(expr));
        return message;
    };

    data.doCommand("let g:who = \"Bob\"");
    QCOMPARE(echo("$\"Hi {g:who}!\""), QLatin1String("Hi Bob!"));
    QCOMPARE(echo("$'sum={1 + 2}'"), QLatin1String("sum=3"));
    QCOMPARE(echo("$'{{x}}'"), QLatin1String("{x}"));
    QCOMPARE(echo("$\"a{{b}}c\""), QLatin1String("a{b}c"));
    QCOMPARE(echo("$\"{g:who} is {1 + 40 + 1}\""), QLatin1String("Bob is 42"));
    QCOMPARE(echo("$\"tab\\tend\""), QLatin1String("tab\tend"));
    QCOMPARE(echo("$'plain'"), QLatin1String("plain"));
}

void FakeVimTester::test_vim_heredoc()
{
    // "let VAR =<< [trim] MARKER" gathers lines into a list of strings.
    TestData data;
    setup(&data);
    QString message;
    data.handler->commandBufferChanged.set(
        [&](const QString &msg, int, int, int) { message = msg; });
    auto echo = [&](const char *expr) -> QString {
        message.clear();
        data.doCommand(QLatin1String("echo ") + QLatin1String(expr));
        return message;
    };
    auto source = [&](const char *text) {
        QTemporaryFile file;
        QVERIFY(file.open());
        file.write(text);
        file.flush();
        data.doCommand(QLatin1String("source ") + file.fileName());
    };

    source("let g:plain =<< END\n"
           "one\n"
           "two\n"
           "END\n"
           "let g:trimmed =<< trim END\n"
           "    alpha\n"
           "    beta\n"
           "    END\n");
    QCOMPARE(echo("g:plain"), QLatin1String("['one', 'two']"));
    QCOMPARE(echo("g:trimmed"), QLatin1String("['alpha', 'beta']"));

    source("vim9script\n"
           "var lines =<< trim END\n"
           "  first\n"
           "  second\n"
           "END\n"
           "g:v9 = lines\n");
    QCOMPARE(echo("g:v9"), QLatin1String("['first', 'second']"));
}

void FakeVimTester::test_vim9_import_autoload()
{
    // "import autoload 'x.vim'" is found in the autoload directory next to the
    // importing script, the layout a packaged plugin uses.
    TestData data;
    setup(&data);
    QString message;
    data.handler->commandBufferChanged.set(
        [&](const QString &msg, int, int, int) { message = msg; });
    auto echo = [&](const char *expr) -> QString {
        message.clear();
        data.doCommand(QLatin1String("echo ") + QLatin1String(expr));
        return message;
    };

    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    QVERIFY(QDir(dir.path()).mkpath("autoload"));
    QVERIFY(QDir(dir.path()).mkpath("plugin"));

    QFile module(dir.path() + "/autoload/mod.vim");
    QVERIFY(module.open(QIODevice::WriteOnly));
    module.write("vim9script\n"
                 "export def Twice(n: number): number\n"
                 "  return n * 2\n"
                 "enddef\n");
    module.close();

    QFile plugin(dir.path() + "/plugin/mod.vim");
    QVERIFY(plugin.open(QIODevice::WriteOnly));
    plugin.write("vim9script\n"
                 "import autoload 'mod.vim'\n"
                 "g:doubled = mod.Twice(21)\n");
    plugin.close();

    data.doCommand("source " + plugin.fileName());
    QCOMPARE(echo("g:doubled"), QLatin1String("42"));

    // A script importing itself must be refused rather than recurse until the
    // stack is gone.
    QFile loop(dir.path() + "/plugin/loop.vim");
    QVERIFY(loop.open(QIODevice::WriteOnly));
    loop.write("vim9script\n"
               "import autoload 'loop.vim'\n"
               "g:reached = 1\n");
    loop.close();
    message.clear();
    data.doCommand("source " + loop.fileName());
    QCOMPARE(echo("g:reached"), QLatin1String("1"));
}

void FakeVimTester::test_vim9_nested_def()
{
    // Vim9 ":def" nests: the inner function is defined when the outer one runs.
    TestData data;
    setup(&data);
    QString message;
    data.handler->commandBufferChanged.set(
        [&](const QString &msg, int, int, int) { message = msg; });
    auto echo = [&](const char *expr) -> QString {
        message.clear();
        data.doCommand(QLatin1String("echo ") + QLatin1String(expr));
        return message;
    };
    auto source = [&](const char *text) {
        QTemporaryFile file;
        QVERIFY(file.open());
        file.write(text);
        file.flush();
        data.doCommand(QLatin1String("source ") + file.fileName());
    };

    // The outer "enddef" must not be swallowed by the inner one.
    source("vim9script\n"
           "def Outer(): number\n"
           "  def Inner(): number\n"
           "    return 7\n"
           "  enddef\n"
           "  return Inner() + 1\n"
           "enddef\n"
           "g:r = Outer()\n"
           "g:after = 'reached'\n");
    QCOMPARE(echo("g:r"), QLatin1String("8"));
    QCOMPARE(echo("g:after"), QLatin1String("reached"));
}

void FakeVimTester::test_vim_script_scope_dict()
{
    // A bare scope name is a dictionary of that scope, as in get(g:, 'x', 1).
    TestData data;
    setup(&data);
    QString message;
    data.handler->commandBufferChanged.set(
        [&](const QString &msg, int, int, int) { message = msg; });
    auto echo = [&](const char *expr) -> QString {
        message.clear();
        data.doCommand(QLatin1String("echo ") + QLatin1String(expr));
        return message;
    };

    data.doCommand("let g:known = 5");
    data.doCommand("let b:buf = 6");
    QCOMPARE(echo("get(g:, 'known', 99)"), QLatin1String("5"));
    QCOMPARE(echo("get(g:, 'missing', 99)"), QLatin1String("99"));
    QCOMPARE(echo("has_key(g:, 'known')"), QLatin1String("1"));
    QCOMPARE(echo("get(b:, 'buf', 0)"), QLatin1String("6"));
    // A scoped name is not visible in the global scope dictionary.
    QCOMPARE(echo("has_key(g:, 'b:buf')"), QLatin1String("0"));
}

void FakeVimTester::test_vim_script_expand()
{
    // expand() keywords. "<stack>" has to name a function that can be called
    // again, which is how plugins install their own 'operatorfunc'.
    TestData data;
    setup(&data);
    QString message;
    data.handler->commandBufferChanged.set(
        [&](const QString &msg, int, int, int) { message = msg; });
    auto echo = [&](const char *expr) -> QString {
        message.clear();
        data.doCommand(QLatin1String("echo ") + QLatin1String(expr));
        return message;
    };

    data.doCommand("function Where() | let g:stack = expand('<stack>') "
                   "| let g:sfile = expand('<sfile>') | endfunction");
    data.doCommand("call Where()");
    // Outside a script the chain starts at the command line, and the innermost
    // entry is the function name followed by the statement it is at, in
    // brackets, as Vim reports it ("function probe#Toggle[1]").
    QCOMPARE(echo("g:stack"), QLatin1String("command line..function Where[1]"));
    QCOMPARE(echo("matchstr(g:stack, '[^. ]*\\ze[')"), QLatin1String("Where"));
    // Inside a function "<sfile>" names the frames, the innermost being the
    // function itself and without the statement "<stack>" would add, so that a
    // plugin can pick its own name out of the end of it.
    QCOMPARE(echo("g:sfile"), QLatin1String("command line..function Where"));

    data.setText("alpha be" X "ta gamma");
    QCOMPARE(echo("expand('<cword>')"), QLatin1String("beta"));
}

void FakeVimTester::test_vim_ex_normal_modes()
{
    // ":normal" aborts a command its keys did not finish, which is what ends a
    // pending insert mode, but keeps a mode the keys did finish.
    TestData data;
    setup(&data);

    // Insert mode is still ended (QTCREATORBUG-25820).
    data.setText("abc" N "def");
    data.doCommand("normal A;");
    QCOMPARE(data.text(), QByteArray("abc;" N "def"));
    data.doKeys("x");
    QCOMPARE(data.text(), QByteArray("abc" N "def"));

    // A "|" is one of the keys, not the start of another command, so ":normal"
    // cannot be followed by one. Expected values taken from Vim 9.1.
    data.setText("abc" N "def");
    data.doCommand("call cursor(1, 1)");
    data.doCommand("normal ix|y");
    QCOMPARE(data.text(), QByteArray("x|yabc" N "def"));
    data.doCommand("call cursor(2, 1)");
    data.doCommand("normal! ip|q");
    QCOMPARE(data.text(), QByteArray("x|yabc" N "p|qdef"));

    // A mode the keys did finish is no longer ended. Typing ":" ends visual
    // mode again on its own, so this is reached through a <Cmd> mapping.
    data.doCommand("nnoremap zv <Cmd>normal! gg0vjl<CR>");
    data.setText("abcde" N "fghij");
    data.doKeys("zv");
    // Still selecting, so an operator typed now takes the selection.
    data.doKeys("d");
    QCOMPARE(data.text(), QByteArray("hij"));
    data.doCommand("nunmap zv");

    // Per line in a range, a selection does not reach the next one.
    data.setText("abc" N "def" N "ghi");
    data.doCommand("%normal v");
    QCOMPARE(data.text(), QByteArray("abc" N "def" N "ghi"));
    data.doKeys("<ESC>");
}

void FakeVimTester::test_vim_map_cmd()
{
    // "<Cmd>{command}<CR>" runs an ex command without leaving the mode it was
    // used in. "<ScriptCmd>" is the same here.
    TestData data;
    setup(&data);

    // Normal mode.
    data.doCommand("nnoremap zc <Cmd>call setline(1, 'FROM_CMD')<CR>");
    data.setText("aaa" N "bbb");
    data.doKeys("zc");
    QCOMPARE(data.text(), QByteArray("FROM_CMD" N "bbb"));

    // The mode is kept, so the following keys still act on the buffer.
    data.doKeys("jx");
    QCOMPARE(data.text(), QByteArray("FROM_CMD" N "bb"));

    // <ScriptCmd> behaves the same.
    data.doCommand("nnoremap zs <ScriptCmd>call setline(2, 'SCRIPT')<CR>");
    data.doKeys("zs");
    QCOMPARE(data.text(), QByteArray("FROM_CMD" N "SCRIPT"));

    // Insert mode is not left either: the command changes the other line, and
    // what is typed afterwards still goes into the text.
    data.doCommand("inoremap zi <Cmd>call setline(2, 'TOUCHED')<CR>");
    data.setText("aaa" N "bbb");
    data.doKeys("A");
    data.doKeys("zi");
    data.doKeys("Q<ESC>");
    QCOMPARE(data.text(), QByteArray("aaaQ" N "TOUCHED"));

    // Keys behind the <CR> are keys again and follow the command.
    data.doCommand("nnoremap zt <Cmd>call cursor(2, 1)<CR>x");
    data.setText("aaa" N "bbb");
    data.doKeys("zt");
    QCOMPARE(data.text(), QByteArray("aaa" N "bb"));

    // Operator pending, command as a plain motion: the cursor it left is the
    // end of the range.
    data.doCommand("onoremap zo <Cmd>call cursor(2, 3)<CR>");
    data.setText("abcde" N "fghij");
    data.doKeys("gg0dzo");
    QCOMPARE(data.text(), QByteArray("hij"));

    // Operator pending, command selecting a range: that selection is what the
    // operator takes, which is how a script defines a text object. The keys run
    // as if no operator were pending, so "v" starts a selection here.
    data.doCommand("onoremap zv <Cmd>normal! gg0vjl<CR>");
    data.setText("abcde" N "fghij");
    data.doKeys("dzv");
    QCOMPARE(data.text(), QByteArray("hij"));

    // A linewise selection makes the operator linewise.
    data.doCommand("onoremap zl <Cmd>normal! ggVj<CR>");
    data.setText("aaa" N "bbb" N "ccc");
    data.doKeys("dzl");
    QCOMPARE(data.text(), QByteArray("ccc"));
    // The shape a plugin uses: put the cursor on one end, start selecting, then
    // move to the other end. The moves must not undo the selection.
    {
        QTemporaryFile file;
        QVERIFY(file.open());
        file.write("vim9script\n"
                   "def Sel()\n"
                   "  cursor(3, 1)\n"
                   "  normal! V\n"
                   "  cursor(2, 1)\n"
                   "enddef\n");
        file.flush();
        data.doCommand(QLatin1String("source ") + file.fileName());
    }
    data.doCommand("onoremap zs <Cmd>call Sel()<CR>");
    data.setText("aaa" N "bbb" N "ccc" N "ddd");
    data.doKeys("dzs");
    QCOMPARE(data.text(), QByteArray("aaa" N "ddd"));

    data.doCommand("ounmap zv");
    data.doCommand("ounmap zl");
    data.doCommand("ounmap zs");

    data.doCommand("nunmap zc");
    data.doCommand("nunmap zs");
    data.doCommand("nunmap zt");
    data.doCommand("iunmap zi");
    data.doCommand("ounmap zo");
}

void FakeVimTester::test_vim_script_search_cursor()
{
    TestData data;
    setup(&data);
    QString message;
    data.handler->commandBufferChanged.set(
        [&](const QString &msg, int, int, int) { message = msg; });
    auto echo = [&](const char *expr) -> QString {
        message.clear();
        data.doCommand(QLatin1String("echo ") + QLatin1String(expr));
        return message;
    };

    data.setText("alpha" N "beta" N "gamma" N "beta" N "delta");

    // cursor() moves and reports where it went.
    QCOMPARE(echo("cursor(3, 2)"), QLatin1String("0"));
    QCOMPARE(echo("[line('.'), col('.')]"), QLatin1String("[3, 2]"));
    QCOMPARE(echo("cursor(99, 1)"), QLatin1String("-1")); // no such line

    // search() returns the line of the match and moves there.
    data.doCommand("call cursor(1, 1)");
    QCOMPARE(echo("search('beta')"), QLatin1String("2"));
    QCOMPARE(echo("line('.')"), QLatin1String("2"));
    // Again from there finds the second one.
    QCOMPARE(echo("search('beta')"), QLatin1String("4"));

    // "b" searches backwards, "n" leaves the cursor alone.
    data.doCommand("call cursor(5, 1)");
    QCOMPARE(echo("search('beta', 'bn')"), QLatin1String("4"));
    QCOMPARE(echo("line('.')"), QLatin1String("5"));

    // "c" accepts a match at the cursor.
    data.doCommand("call cursor(2, 1)");
    QCOMPARE(echo("search('beta', 'c')"), QLatin1String("2"));
    data.doCommand("call cursor(2, 1)");
    QCOMPARE(echo("search('beta', '')"), QLatin1String("4"));

    // No match gives 0 and the cursor stays put.
    data.doCommand("call cursor(3, 1)");
    QCOMPARE(echo("search('nowhere')"), QLatin1String("0"));
    QCOMPARE(echo("line('.')"), QLatin1String("3"));

    // {stopline} limits how far to look.
    data.doCommand("call cursor(1, 1)");
    QCOMPARE(echo("search('beta', 'W', 3)"), QLatin1String("2"));
    data.doCommand("call cursor(2, 5)");
    QCOMPARE(echo("search('beta', 'W', 3)"), QLatin1String("0"));

    // {skip} rejects a candidate; it runs with the cursor on the match, so it
    // can decide from where it is.
    data.doCommand("call cursor(1, 1)");
    QCOMPARE(echo("search('beta', 'W', 0, 0, 'line(\".\") == 2')"), QLatin1String("4"));
    // A funcref works the same way.
    data.doCommand("function SkipSecond() | return line('.') == 2 | endfunction");
    data.doCommand("call cursor(1, 1)");
    QCOMPARE(echo("search('beta', 'W', 0, 0, function('SkipSecond'))"),
             QLatin1String("4"));

    // "^" and "$" mean the ends of a line, not of the buffer, so an empty line
    // is findable. Plugins lean on this to spot a paragraph break.
    data.setText("aaa" N "" N "bbb" N "" N "ccc");
    data.doCommand("call cursor(1, 1)");
    QCOMPARE(echo("search('^$', 'W')"), QLatin1String("2"));
    QCOMPARE(echo("search('^$', 'W')"), QLatin1String("4"));
    data.doCommand("call cursor(1, 1)");
    QCOMPARE(echo("search('\\v^\\s*$', 'W')"), QLatin1String("2"));
    // "$" at the end of a line matches there too.
    data.doCommand("call cursor(1, 1)");
    QCOMPARE(echo("search('a$', 'W')"), QLatin1String("1"));

    // The two-step walk a plugin uses to find the end of a block of lines that
    // share a property: forward past everything having it, then back to the
    // last thing that had it.
    data.setText("int a = 1;" N "// c1" N "// c2" N "int b = 2;");
    data.doCommand("function IsC() | return getline('.') =~ '^\\s*//' | endfunction");
    data.doCommand("function NotC() | return getline('.') !~ '^\\s*//' | endfunction");
    data.doCommand("call cursor(2, 4)");
    QCOMPARE(echo("search('\\v%(\\S+)|%(^\\s*$)', 'W', 0, 200, function('IsC'))"),
             QLatin1String("4"));
    QCOMPARE(echo("[line('.'), col('.')]"), QLatin1String("[4, 1]"));
    QCOMPARE(echo("search('\\S', 'beW', 0, 200, function('NotC'))"), QLatin1String("3"));
    QCOMPARE(echo("[line('.'), col('.')]"), QLatin1String("[3, 5]"));

    data.setText("alpha" N "beta" N "gamma" N "beta" N "delta");
    // "w" wraps and "W" does not, whatever 'wrapscan' says.
    data.doCommand("call cursor(5, 1)");
    QCOMPARE(echo("search('alpha', 'w')"), QLatin1String("1"));
    data.doCommand("call cursor(5, 1)");
    QCOMPARE(echo("search('alpha', 'W')"), QLatin1String("0"));

    // Without either flag 'wrapscan' decides. It is a global option, so put it
    // back to where the other tests expect it.
    data.doCommand("set wrapscan");
    data.doCommand("call cursor(5, 1)");
    QCOMPARE(echo("search('alpha')"), QLatin1String("1"));
    data.doCommand("set nowrapscan");
    data.doCommand("call cursor(5, 1)");
    QCOMPARE(echo("search('alpha')"), QLatin1String("0"));
    data.doCommand("set wrapscan");
}

void FakeVimTester::test_vim_script_readfile_writefile()
{
    TestData data;
    setup(&data);
    QString message;
    data.handler->commandBufferChanged.set(
        [&](const QString &msg, int, int, int) { message = msg; });
    auto echo = [&](const char *expr) -> QString {
        message.clear();
        data.doCommand(QLatin1String("echo ") + QLatin1String(expr));
        return message;
    };

    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    data.doCommand("let g:p = '" + dir.path() + "/a.txt'");

    // A list written out comes back unchanged.
    QCOMPARE(echo("writefile(['one', 'two'], g:p)"), QLatin1String("0"));
    QCOMPARE(echo("readfile(g:p)"), QLatin1String("['one', 'two']"));
    // Text mode ended the last line, which does not turn into another item.
    QCOMPARE(echo("len(readfile(g:p))"), QLatin1String("2"));

    // Appending adds to what is there.
    data.doCommand("call writefile(['three'], g:p, 'a')");
    QCOMPARE(echo("readfile(g:p)"), QLatin1String("['one', 'two', 'three']"));

    // {max} takes lines from the start, a negative one from the end.
    QCOMPARE(echo("readfile(g:p, '', 2)"), QLatin1String("['one', 'two']"));
    QCOMPARE(echo("readfile(g:p, '', -2)"), QLatin1String("['two', 'three']"));

    // Binary mode leaves the last line unterminated, so reading it back in
    // binary mode gives no trailing empty item.
    data.doCommand("call writefile(['x', 'y'], g:p, 'b')");
    QCOMPARE(echo("readfile(g:p, 'b')"), QLatin1String("['x', 'y']"));
    // An empty last item is what puts a newline at the end in binary mode.
    data.doCommand("call writefile(['x', ''], g:p, 'b')");
    QCOMPARE(echo("readfile(g:p, 'b')"), QLatin1String("['x', '']"));

    // Reading a file that is not there gives an empty list.
    data.doCommand("let g:missing = '" + dir.path() + "/nope.txt'");
    QCOMPARE(echo("readfile(g:missing)"), QLatin1String("[]"));

    // Writing where that cannot work reports the failure.
    data.doCommand("let g:bad = '" + dir.path() + "/nodir/x.txt'");
    QCOMPARE(echo("writefile(['a'], g:bad)"), QLatin1String("-1"));
}

void FakeVimTester::test_vim_change_autocmds()
{
    // TextChanged and CursorMoved run from a timer, so let it fire.
    TestData data;
    setup(&data);
    QString message;
    data.handler->commandBufferChanged.set(
        [&](const QString &msg, int, int, int) { message = msg; });
    auto echo = [&](const char *expr) -> QString {
        message.clear();
        data.doCommand(QLatin1String("echo ") + QLatin1String(expr));
        return message;
    };
    auto settle = [] { QTest::qWait(20); };

    data.doCommand("autocmd!");
    data.doCommand("let g:tc = 0");
    data.doCommand("let g:cm = 0");
    data.doCommand("autocmd TextChanged * let g:tc += 1");
    data.doCommand("autocmd CursorMoved * let g:cm += 1");

    data.setText("abc" N "def" N "ghi");
    settle();
    data.doCommand("let g:tc = 0");
    data.doCommand("let g:cm = 0");

    // A motion reports CursorMoved but leaves the text alone.
    data.doKeys("j");
    settle();
    QCOMPARE(echo("g:tc"), QLatin1String("0"));
    QVERIFY(echo("g:cm").toInt() > 0);

    // A change in normal mode reports TextChanged.
    data.doCommand("let g:tc = 0");
    data.doKeys("x");
    settle();
    QVERIFY(echo("g:tc").toInt() > 0);

    // Several changes in a row are reported once, since the timer coalesces.
    data.doCommand("let g:tc = 0");
    data.doKeys("xxx");
    settle();
    QCOMPARE(echo("g:tc"), QLatin1String("1"));

    // Insert mode reports through the "I" events instead.
    data.doCommand("let g:tci = 0");
    data.doCommand("let g:cmi = 0");
    data.doCommand("autocmd TextChangedI * let g:tci += 1");
    data.doCommand("autocmd CursorMovedI * let g:cmi += 1");
    data.doCommand("let g:tc = 0");
    data.doCommand("let g:cm = 0");
    data.doKeys("ihello");
    settle();
    QVERIFY(echo("g:tci").toInt() > 0);
    QVERIFY(echo("g:cmi").toInt() > 0);
    QCOMPARE(echo("g:tc"), QLatin1String("0"));
    QCOMPARE(echo("g:cm"), QLatin1String("0"));
    data.doKeys("<ESC>");

    // Leaving insert mode does not turn the change into a normal mode one: the
    // mode is remembered from when the change happened, not when it is reported.
    data.doCommand("let g:tc = 0");
    data.doCommand("let g:tci = 0");
    data.doKeys("ix<ESC>");
    settle();
    QVERIFY(echo("g:tci").toInt() > 0);
    QCOMPARE(echo("g:tc"), QLatin1String("0"));

    data.doCommand("autocmd!");
}

void FakeVimTester::test_vim_modeline()
{
    TestData data;
    setup(&data);
    QString message;
    data.handler->commandBufferChanged.set(
        [&](const QString &msg, int, int, int) { message = msg; });
    auto echo = [&](const char *expr) -> QString {
        message.clear();
        data.doCommand(QLatin1String("echo ") + QLatin1String(expr));
        return message;
    };
    auto modeline = [&](const QByteArray &text) {
        data.doCommand("set sw=8");
        data.doCommand("set ft=");
        data.setText(text);
        data.handler->processModelines();
    };

    // Both forms, and the "set" form ending its options at the ":".
    modeline("# vim: sw=3" N "x");
    QCOMPARE(echo("&sw"), QLatin1String("3"));
    modeline("/* vim: set sw=4: */" N "x");
    QCOMPARE(echo("&sw"), QLatin1String("4"));
    // ":" separates options in the first form.
    modeline("# vim: sw=5:ts=7" N "x");
    QCOMPARE(echo("&sw"), QLatin1String("5"));
    QCOMPARE(echo("&ts"), QLatin1String("7"));
    // "vi:" and "ex:" are accepted too.
    modeline("# vi: sw=6" N "x");
    QCOMPARE(echo("&sw"), QLatin1String("6"));
    modeline("# ex: sw=2" N "x");
    QCOMPARE(echo("&sw"), QLatin1String("2"));

    // A modeline may set the file type, which is the file having the last word.
    modeline("# vim: ft=python" N "x");
    QCOMPARE(echo("&ft"), QLatin1String("python"));

    // The marker has to start the line or follow a blank, so an ordinary word
    // ending in "vim:" is not one.
    modeline("nonsense_vim: sw=3" N "x");
    QCOMPARE(echo("&sw"), QLatin1String("8"));

    // A version requirement is honored; v:version is 900 here.
    modeline("# vim>700: sw=3" N "x");
    QCOMPARE(echo("&sw"), QLatin1String("3"));
    modeline("# vim>900: sw=3" N "x");
    QCOMPARE(echo("&sw"), QLatin1String("8"));
    modeline("# vim<900: sw=3" N "x");
    QCOMPARE(echo("&sw"), QLatin1String("8"));
    modeline("# vim900: sw=3" N "x");
    QCOMPARE(echo("&sw"), QLatin1String("3"));

    // The last lines are searched as well, which is where they usually sit.
    modeline("x" N "y" N "# vim: sw=3");
    QCOMPARE(echo("&sw"), QLatin1String("3"));
    // ... but only 'modelines' at each end, so one in the middle is ignored.
    data.doCommand("set mls=2");
    modeline("a" N "b" N "# vim: sw=3" N "c" N "d" N "e");
    QCOMPARE(echo("&sw"), QLatin1String("8"));
    data.doCommand("set mls=5");
    modeline("a" N "b" N "# vim: sw=3" N "c" N "d" N "e");
    QCOMPARE(echo("&sw"), QLatin1String("3"));

    // 'nomodeline' turns the whole thing off.
    data.doCommand("set noml");
    modeline("# vim: sw=3" N "x");
    QCOMPARE(echo("&sw"), QLatin1String("8"));
    data.doCommand("set ml");

    // These options are global, so put back what the other tests expect.
    data.doCommand("set sw=8");
    data.doCommand("set ts=8");
    data.doCommand("set ft=");
}

void FakeVimTester::test_vim_filetype_detection()
{
    // The pieces file type detection is built from: buffer read autocommands
    // fire, ":setf" keeps a type that is already known, and "BufRead" and
    // "BufReadPost" are the same event.
    TestData data;
    setup(&data);
    QString message;
    data.handler->commandBufferChanged.set(
        [&](const QString &msg, int, int, int) { message = msg; });
    auto echo = [&](const char *expr) -> QString {
        message.clear();
        data.doCommand(QLatin1String("echo ") + QLatin1String(expr));
        return message;
    };

    // A rule registered for "BufRead" runs when "BufReadPost" is fired.
    data.doCommand("autocmd BufRead *.zz setf ziggy");
    data.handler->setCurrentFileName("a.zz");
    data.handler->triggerAutocmd("BufReadPost");
    QCOMPARE(echo("&ft"), QLatin1String("ziggy"));

    // How the editor fills in a type only if no rule claimed the buffer, which
    // is what keeps a vimrc rule ahead of what Qt Creator detected.
    data.doCommand("if &ft == '' | setf fallback | endif");
    QCOMPARE(echo("&ft"), QLatin1String("ziggy"));
    data.doCommand("set ft=");
    data.doCommand("if &ft == '' | setf fallback | endif");
    QCOMPARE(echo("&ft"), QLatin1String("fallback"));

    // did_filetype() only reports on a sequence of autocommands, so an
    // interactive ":setf" always applies.
    QCOMPARE(echo("did_filetype()"), QLatin1String("0"));
    data.doCommand("setf typedbyhand");
    QCOMPARE(echo("&ft"), QLatin1String("typedbyhand"));

    // Within one read the first ":setf" wins and later ones are ignored.
    data.doCommand("autocmd!");
    data.doCommand("autocmd BufRead * setf first");
    data.doCommand("autocmd BufRead * setf second");
    data.handler->triggerAutocmd("BufReadPost");
    QCOMPARE(echo("&ft"), QLatin1String("first"));

    // Reading again starts over, so a rule can claim the buffer anew.
    data.doCommand("autocmd!");
    data.doCommand("autocmd BufRead * setf third");
    data.handler->triggerAutocmd("BufReadPost");
    QCOMPARE(echo("&ft"), QLatin1String("third"));

    // A FALLBACK type is only a guess, so a later ":setf" replaces it.
    data.doCommand("autocmd!");
    data.doCommand("autocmd BufRead * setf FALLBACK guessed");
    data.doCommand("autocmd BufRead * setf certain");
    data.handler->triggerAutocmd("BufReadPost");
    QCOMPARE(echo("&ft"), QLatin1String("certain"));

    // A ":setf" inside a read autocommand still reaches the FileType rules.
    data.doCommand("autocmd!");
    data.doCommand("let g:ft = ''");
    data.doCommand("autocmd FileType nested let g:ft = 'ran'");
    data.doCommand("autocmd BufRead * setf nested");
    data.handler->triggerAutocmd("BufReadPost");
    QCOMPARE(echo("g:ft"), QLatin1String("ran"));

    // FileType rules see the type, and the type drives 'commentstring'.
    data.doCommand("let g:seen = ''");
    data.doCommand("autocmd FileType python let g:seen = 'py'");
    data.doCommand("set ft=python");
    QCOMPARE(echo("g:seen"), QLatin1String("py"));
    QCOMPARE(echo("&cms"), QLatin1String("# %s"));

    // The window and buffer lifecycle events reach their rules too.
    data.doCommand("autocmd!");
    data.doCommand("let g:seq = ''");
    data.doCommand("autocmd BufEnter * let g:seq .= 'be,'");
    data.doCommand("autocmd BufLeave * let g:seq .= 'bl,'");
    data.doCommand("autocmd BufWinEnter * let g:seq .= 'bwe,'");
    data.doCommand("autocmd WinEnter * let g:seq .= 'we,'");
    data.doCommand("autocmd WinLeave * let g:seq .= 'wl,'");
    data.doCommand("autocmd VimEnter * let g:seq .= 've,'");
    for (const QString &event : QStringList{"BufEnter", "BufLeave", "BufWinEnter",
                                           "WinEnter", "WinLeave", "VimEnter"})
        data.handler->triggerAutocmd(event);
    QCOMPARE(echo("g:seq"), QLatin1String("be,bl,bwe,we,wl,ve,"));

    data.doCommand("autocmd!");
}

void FakeVimTester::test_vim_commentstring()
{
    // 'commentstring' follows the file type and is buffer-local.
    TestData data;
    setup(&data);
    QString message;
    data.handler->commandBufferChanged.set(
        [&](const QString &msg, int, int, int) { message = msg; });
    auto echo = [&](const char *expr) -> QString {
        message.clear();
        data.doCommand(QLatin1String("echo ") + QLatin1String(expr));
        return message;
    };

    // Derived from the file name extension.
    data.handler->setCurrentFileName("Test.cpp");
    QCOMPARE(echo("&cms"), QLatin1String("// %s"));
    data.handler->setCurrentFileName("Test.pro");
    QCOMPARE(echo("&cms"), QLatin1String("# %s"));
    data.handler->setCurrentFileName("Test.py");
    QCOMPARE(echo("&commentstring"), QLatin1String("# %s"));
    data.handler->setCurrentFileName("Test.vim");
    QCOMPARE(echo("&cms"), QLatin1String("\" %s"));
    data.handler->setCurrentFileName("Test.html");
    QCOMPARE(echo("&cms"), QLatin1String("<!-- %s -->"));
    // An unknown extension falls back to the configured default.
    data.handler->setCurrentFileName("Test.zzz");
    QCOMPARE(echo("&cms"), QLatin1String("// %s"));

    // ":setf" wins over the extension, as the file type is more specific.
    data.handler->setCurrentFileName("Test.zzz");
    data.doCommand("setf python");
    QCOMPARE(echo("&cms"), QLatin1String("# %s"));

    // An explicit ":set" wins over both, and stays with this buffer. The space
    // is escaped, as Vim wants it: unescaped it would end the value there.
    data.doCommand("set commentstring=;;\\ %s");
    QCOMPARE(echo("&cms"), QLatin1String(";; %s"));
    data.handler->setCurrentFileName("Other.cpp");
    QCOMPARE(echo("&cms"), QLatin1String(";; %s"));

    // "gc" uses the same value, including a trailing part.
    data.doCommand("set commentary");
    data.doCommand("set commentstring=<!--\\ %s\\ -->");
    data.setText("abc");
    KEYS("gcc", X "<!-- abc -->");
    KEYS("gcc", X "abc");
    // A file type with a line comment keeps the previous behavior.
    data.doCommand("set commentstring=#\\ %s");
    data.setText("abc");
    KEYS("gcc", X "# abc");
    KEYS("gcc", X "abc");
}

void FakeVimTester::test_vim_script_regex_zs_ze()
{
    // Vim's \zs and \ze move the reported match start and end.
    TestData data;
    setup(&data);
    QString message;
    data.handler->commandBufferChanged.set(
        [&](const QString &msg, int, int, int) { message = msg; });
    auto echo = [&](const char *expr) -> QString {
        message.clear();
        data.doCommand(QLatin1String("echo ") + QLatin1String(expr));
        return message;
    };

    QCOMPARE(echo("matchstr('foobar', 'foo\\zsbar')"), QLatin1String("bar"));
    QCOMPARE(echo("matchstr('foobar', 'foo\\zebar')"), QLatin1String("foo"));
    // A trailing "[" opens no character class; Vim matches it literally.
    QCOMPARE(echo("matchstr('a name[3]', '[^. ]*\\ze[')"), QLatin1String("name"));
    QCOMPARE(echo("substitute('foobar', 'foo\\zsbar', 'X', '')"), QLatin1String("fooX"));
    QCOMPARE(echo("substitute('foobar', 'foo\\zebar', 'X', '')"), QLatin1String("Xbar"));

    // Magic levels. The expected values were taken from Vim 9.1.
    // "\v": punctuation carries meaning without a backslash.
    QCOMPARE(echo("matchstr('foo  bar', '\\v\\s+')"), QLatin1String("  "));
    QCOMPARE(echo("matchstr('xabcy', '\\v%(abc)')"), QLatin1String("abc"));
    QCOMPARE(echo("matchstr('cat dog', '\\v(dog|cat)')"), QLatin1String("cat"));
    QCOMPARE(echo("matchstr('a foo b', '\\v<foo>')"), QLatin1String("foo"));
    QCOMPARE(echo("matchstr('color', '\\vcolou=r')"), QLatin1String("color"));
    QCOMPARE(echo("matchstr('aaaa', '\\va{2,3}')"), QLatin1String("aaa"));
    // ... so a backslash is what makes one literal again.
    QCOMPARE(echo("matchstr('a+b', '\\va\\+b')"), QLatin1String("a+b"));

    // The default is "magic", where it is the other way round.
    QCOMPARE(echo("matchstr('foo  bar', '\\s\\+')"), QLatin1String("  "));
    QCOMPARE(echo("matchstr('a+b', 'a+b')"), QLatin1String("a+b"));

    // "\V" takes everything literally.
    QCOMPARE(echo("matchstr('a.c', '\\Va.c')"), QLatin1String("a.c"));
    QCOMPARE(echo("'[' . matchstr('abc', '\\Va.c') . ']'"), QLatin1String("[]"));

    // The character classes Vim has beyond what a Qt pattern knows. For these
    // the uppercase form is the same class without the digits, not its
    // negation. Expected values taken from Vim 9.1.
    QCOMPARE(echo("matchstr('  ab_9!', '\\v\\k+')"), QLatin1String("ab_9"));
    QCOMPARE(echo("matchstr('9ab', '\\v\\K+')"), QLatin1String("ab"));
    QCOMPARE(echo("matchstr('  a1_', '\\i\\+')"), QLatin1String("a1_"));
    QCOMPARE(echo("matchstr('1ab', '\\I\\+')"), QLatin1String("ab"));
    QCOMPARE(echo("matchstr('a b', '\\p\\+')"), QLatin1String("a b"));
    QCOMPARE(echo("matchstr('1ab', '\\P\\+')"), QLatin1String("ab"));

    // The shape a plugin uses to find the next word or an empty line.
    QCOMPARE(echo("matchstr('  hello', '\\v%(\\S+)|%(^\\s*$)')"), QLatin1String("hello"));
}

void FakeVimTester::test_vim_script_modifiers()
{
    // Command modifiers prefix another command; what follows is a statement in
    // its own right, so in Vim9 it may be a bare function call.
    TestData data;
    setup(&data);
    auto source = [&](const char *text) {
        QTemporaryFile file;
        QVERIFY(file.open());
        file.write(text);
        file.flush();
        data.doCommand(QLatin1String("source ") + file.fileName());
    };

    data.setText("one" N "two");
    data.doCommand("noautocmd call setline(1, 'ONE')");
    QCOMPARE(data.text(), QByteArray("ONE" N "two"));
    // Abbreviated, and stacked with a second modifier.
    data.doCommand("noa keepj call setline(2, 'TWO')");
    QCOMPARE(data.text(), QByteArray("ONE" N "TWO"));

    // ":noautocmd" keeps the autocommands of what follows from firing.
    QString message;
    data.handler->commandBufferChanged.set(
        [&](const QString &msg, int, int, int) { message = msg; });
    auto echo = [&](const char *expr) -> QString {
        message.clear();
        data.doCommand(QLatin1String("echo ") + QLatin1String(expr));
        return message;
    };
    data.doCommand("autocmd!");
    data.doCommand("let g:fired = 0");
    data.doCommand("autocmd FileType suppressed let g:fired = 1");
    data.doCommand("noautocmd set ft=suppressed");
    QCOMPARE(echo("&ft"), QLatin1String("suppressed"));
    QCOMPARE(echo("g:fired"), QLatin1String("0"));
    // The same rule does fire without the modifier.
    data.doCommand("set ft=");
    data.doCommand("set ft=suppressed");
    QCOMPARE(echo("g:fired"), QLatin1String("1"));
    data.doCommand("autocmd!");

    // In Vim9 the command after a modifier can be a bare call.
    data.setText("a" N "b");
    source("vim9script\n"
           "def Fill()\n"
           "  noautocmd keepjumps setline(1, ['x', 'y'])\n"
           "enddef\n"
           "Fill()\n");
    QCOMPARE(data.text(), QByteArray("x" N "y"));
}

void FakeVimTester::test_vim_script_operator_plugin()
{
    // The shape a real Vim 9 plugin uses for a custom operator: an autoload
    // module whose function sets 'operatorfunc' from expand('<stack>') and
    // returns "g@", reached through a <Plug> mapping with <silent> <expr>.
    TestData data;
    setup(&data);

    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    QVERIFY(QDir(dir.path()).mkpath("autoload"));
    QVERIFY(QDir(dir.path()).mkpath("plugin"));

    QFile module(dir.path() + "/autoload/hash.vim");
    QVERIFY(module.open(QIODevice::WriteOnly));
    module.write("vim9script\n"
                 "export def Toggle(...args: list<string>): string\n"
                 "    if len(args) == 0\n"
                 "        &opfunc = matchstr(expand('<stack>'), '[^. ]*\\ze[')\n"
                 "        return 'g@'\n"
                 "    endif\n"
                 "    var [l1, l2] = [line(\"'[\"), line(\"']\")]\n"
                 "    var lines = []\n"
                 "    for n in range(l1, l2)\n"
                 "        add(lines, '#' .. getline(n))\n"
                 "    endfor\n"
                 "    noautocmd keepjumps setline(l1, lines)\n"
                 "    return ''\n"
                 "enddef\n");
    module.close();

    QFile plugin(dir.path() + "/plugin/hash.vim");
    QVERIFY(plugin.open(QIODevice::WriteOnly));
    plugin.write("vim9script\n"
                 "import autoload 'hash.vim'\n"
                 "nnoremap <silent> <expr> <Plug>(hash-toggle) hash.Toggle()\n"
                 "xnoremap <silent> <expr> <Plug>(hash-toggle) hash.Toggle()\n"
                 "nnoremap <silent> <expr> <Plug>(hash-line) hash.Toggle() .. '_'\n"
                 "nmap gz <Plug>(hash-toggle)\n"
                 "xmap gz <Plug>(hash-toggle)\n"
                 "nmap gzz <Plug>(hash-line)\n");
    plugin.close();

    data.doCommand("source " + plugin.fileName());

    // Operator plus motion.
    data.setText("aa" N "bb" N "cc");
    data.doKeys("gg0gzj");
    QCOMPARE(data.text(), QByteArray("#aa" N "#bb" N "cc"));

    // The same operator from a visual selection, through the xmap.
    data.setText("aa" N "bb" N "cc");
    data.doKeys("gg0Vjgz");
    QCOMPARE(data.text(), QByteArray("#aa" N "#bb" N "cc"));

    // "." repeats the operator.
    data.setText("aa" N "bb");
    data.doKeys("gg0gzj");
    data.doKeys("gg0.");
    QCOMPARE(data.text(), QByteArray("##aa" N "##bb"));

    // Mappings live in global state, so undo what this test installed.
    data.doCommand("nunmap gz");
    data.doCommand("nunmap gzz");
    data.doCommand("xunmap gz");
    data.doCommand("nunmap <Plug>(hash-toggle)");
    data.doCommand("nunmap <Plug>(hash-line)");
    data.doCommand("xunmap <Plug>(hash-toggle)");
    data.doCommand("set opfunc=");
}

void FakeVimTester::test_vim9_import_export()
{
    // Vim9 "export def/var" + "import 'file' [as Name]"; exported names are
    // bound as a dict on the alias, resolved relative to the importing
    // script's own directory.
    TestData data;
    setup(&data);
    QString message;
    data.handler->commandBufferChanged.set(
        [&](const QString &msg, int, int, int) { message = msg; });
    auto echo = [&](const char *expr) -> QString {
        message.clear();
        data.doCommand(QLatin1String("echo ") + QLatin1String(expr));
        return message;
    };

    QTemporaryFile module(QDir::tempPath() + "/fakevim_module_XXXXXX.vim");
    QVERIFY(module.open());
    module.write("vim9script\n"
                 "export var greeting = \"hi\"\n"
                 "export def Add(x: number, y: number): number\n"
                 "  return x + y\n"
                 "enddef\n");
    module.flush();
    const QString moduleBase = QFileInfo(module.fileName()).fileName();

    QTemporaryFile main(QDir::tempPath() + "/fakevim_main_XXXXXX.vim");
    QVERIFY(main.open());
    main.write(("vim9script\n"
                "import '" + moduleBase + "' as mod\n"
                "g:greeting = mod.greeting\n"
                "g:sum = mod.Add(3, 4)\n").toUtf8());
    main.flush();
    data.doCommand(QLatin1String("source ") + main.fileName());

    QCOMPARE(echo("g:greeting"), QLatin1String("hi"));
    QCOMPARE(echo("g:sum"), QLatin1String("7"));
}

void FakeVimTester::test_vim_file_info()
{
    // CTRL-G reports file position and status, like Vim.
    TestData data;
    setup(&data);
    QString message;
    data.handler->commandBufferChanged.set(
        [&](const QString &msg, int, int, int) { message = msg; });
    data.setText("one" N "two" N "three" N "four");
    data.doKeys("2G");
    data.doKeys("<c-g>");
    QVERIFY2(message.contains("line 2 of 4"), qPrintable(message));
    QVERIFY2(message.contains("--50%--"), qPrintable(message));
}

void FakeVimTester::test_vim_ex_plugin_command_moves_cursor()
{
    // An Ex command mapped to a Qt Creator action that moves the cursor
    // synchronously must keep that new position; FakeVim must not revert it
    // to where the cursor was before the command (QTCREATORBUG-27191).
    TestData data;
    setup(&data);
    data.setText("|abc" N "def" N "ghi");
    const int target = data.text().indexOf('g');
    data.handler->handleExCommandRequested.set(
        [&](bool *handled, const ExCommand &) {
            QTextCursor tc = data.editor()->textCursor();
            tc.setPosition(target);
            data.editor()->setTextCursor(tc);
            *handled = true;
        });
    data.doKeys(":foo<CR>");
    QCOMPARE(data.position(), target);
}

void FakeVimTester::test_vim_dot_after_visual_paste()
{
    TestData data;
    setup(&data);

    // QTCREATORBUG-18298: as in Vim, redo of a paste over a visual selection
    // only repeats the deletion of the (re-selected) region; the put text is
    // not recorded, so "." leaves nothing behind.
    data.setText("(bl|ubb)" N "(abc)" N "(xyz)");
    KEYS("yi(", "(blubb)" N "(abc)" N "(xyz)");
    KEYS("2G0lvi(p", "(blubb)" N "(blubb)" N "(xyz)");
    KEYS("3G0l.", "(blubb)" N "(blubb)" N "()");

    // Same for a line-wise selection.
    data.setText("|AAA" N "BBB" N "CCC" N "DDD");
    KEYS("yy", "AAA" N "BBB" N "CCC" N "DDD");
    KEYS("2GVp", "AAA" N "AAA" N "CCC" N "DDD");
    KEYS("3G.", "AAA" N "AAA" N "DDD");
}

void FakeVimTester::test_vim_use_editor_tab_settings()
{
    // With useEditorTabSettings on, indentation follows the editor tab
    // settings (delivered through tabSettingsRequested) instead of the FakeVim
    // ones (QTCREATORBUG-14273).
    auto &sw = FakeVim::Internal::settings().shiftWidth;
    auto &et = FakeVim::Internal::settings().expandTab;
    auto &useEditor = FakeVim::Internal::settings().useEditorTabSettings;
    const qint64 savedSw = sw.value();
    const bool savedEt = et.value();
    const bool savedUseEditor = useEditor.value();

    // FakeVim own settings: 8 wide, real tabs.
    sw.setValue(8);
    et.setValue(false);

    TestData data;
    setup(&data);
    // Editor settings served to the handler: 2 wide, spaces.
    data.handler->tabSettingsRequested.set(
        [](int *tabSize, int *indentSize, bool *spacesForTabs) {
            *tabSize = 2;
            *indentSize = 2;
            *spacesForTabs = true;
        });

    // Off: uses the FakeVim settings (one tab).
    useEditor.setValue(false);
    data.setText("a|bc");
    KEYS(">>", "\t|abc");

    // On: follows the editor (two spaces).
    useEditor.setValue(true);
    data.setText("a|bc");
    KEYS(">>", "  |abc");

    sw.setValue(savedSw);
    et.setValue(savedEt);
    useEditor.setValue(savedUseEditor);
}

void FakeVimTester::test_vim_command_line_paste()
{
    // Ctrl-V pastes the clipboard into the command line, OS style, in addition
    // to the Vim way with Ctrl-R (QTCREATORBUG-23785).
    TestData data;
    setup(&data);
    data.setText("|abc" N "def" N "ghi");
    Utils::setClipboardAndSelection("ghi");
    // Paste the search term into the / command line, then run the search.
    data.doKeys("/<c-v><CR>");
    QCOMPARE(data.cursor().blockNumber(), 2);
}

void FakeVimTester::test_vim_tab_out()
{
    // With tabOut set, TAB in insert mode jumps over the next listed closing
    // character instead of inserting a tab (QTCREATORBUG-27441).
    auto &tabOut = FakeVim::Internal::settings().tabOut;
    const QString saved = tabOut.value();
    tabOut.setValue(")]}");

    TestData data;
    setup(&data);
    data.setText("foo(|)");
    KEYS("i<tab>", "foo()|");
    // Repeated tabs jump over successive closers.
    data.setText("(|))");
    KEYS("i<tab><tab>", "())|");

    tabOut.setValue(saved);
}

void FakeVimTester::test_vim_iso_level5_shift()
{
    // Route key events through the event filter (as in the real application)
    // so the QKeyEvent code path under test is actually exercised, and let
    // FakeVim insert the text itself instead of forwarding it to the editor.
    FvBoolAspect &useFakeVim = FakeVim::Internal::settings().useFakeVim;
    FvBoolAspect &passKeys = FakeVim::Internal::settings().passKeys;
    const bool savedUseFakeVim = useFakeVim.value();
    const bool savedPassKeys = passKeys.value();
    useFakeVim.setValue(true);
    passKeys.setValue(false);

    TestData data;
    setup(&data);
    data.setText("a|bc");
    data.doKeys("i"); // insert mode; also installs the event filter

    // A layout modifier such as ISO_Level5_Shift has no Qt key code and reaches
    // FakeVim as a key event whose text is a single null character. It must not
    // be inserted into the document (QTCREATORBUG-26818).
    QKeyEvent e(QEvent::KeyPress, Qt::Key_unknown, Qt::NoModifier, QString(QChar(0)));
    QApplication::sendEvent(data.editor(), &e);
    const QString text = data.editor()->toPlainText();

    useFakeVim.setValue(savedUseFakeVim);
    passKeys.setValue(savedPassKeys);

    QCOMPARE(text, QString("abc"));
}

void FakeVimTester::test_macros()
{
    TestData data;
    setup(&data);

    // execute register content
    data.setText("r1" N "r2r3");
    KEYS("\"xy$", X "r1" N "r2r3");
    KEYS("@x", X "11" N "r2r3");
    INTEGRITY(false);

    data.doKeys("j\"xy$");
    KEYS("@x", "11" N X "32r3");
    INTEGRITY(false);

    data.setText("3<C-A>");
    KEYS("\"xy$", X "3<C-A>");
    KEYS("@x", X "6<C-A>");
    KEYS("@x", X "9<C-A>");
    KEYS("2@x", "1" X "5<C-A>");
    KEYS("2@@", "2" X "1<C-A>");
    KEYS("@@", "2" X "4<C-A>");

// Raw characters for macro recording.
#define ESC "\x1b"
#define ENTER "\n"

    // record
    data.setText("abc" N "def");
    KEYS("qx" "A" ENTER "- xyz" ESC "rZjI- opq" ENTER ESC "q" , "abc" N "- xyZ" N "- opq" N X "def");
    KEYS("@x" , "abc" N "- xyZ" N "- opq" N "def" N "- opq" N X "- xyZ");

    data.setText("  1 2 3" N "  4 5 6" N "  7 8 9");
    KEYS("qx" "wrXj" "q", "  X 2 3" N "  4 5 6" N "  7 8 9");
    KEYS("2@x", "  X 2 3" N "  4 X 6" N "  7 8 X");

    data.setText("abc" N "def");
    KEYS("qx<right>i<right> xyz <esc>q", "ab xyz" X " c" N "def");
    KEYS("j0@x", "ab xyz c" N "de xyz" X " f");

    data.setText("abc" N "def");
    data.doCommand("unmap <S-down>");
    KEYS("qx<S-down><esc>q", X "abc" N "def");
    data.doCommand("noremap <S-down> ddp");
    KEYS("@x", "def" N X "abc");
    KEYS("gg@x", "abc" N X "def");
    data.doCommand("unmap <S-down>");

    data.setText("   abc xyz>." N "   def xyz>." N "   ghi xyz>." N "   jkl xyz>.");
    KEYS("qq" "^wdf>j" "q", "   abc ." N "   def " X "xyz>." N "   ghi xyz>." N "   jkl xyz>.");
    KEYS("2@q", "   abc ." N "   def ." N "   ghi ." N "   jkl " X "xyz>.");

    // record command line
    data.setText("abc" N "def");
    KEYS("qq" ":s/./*/g<ESC>" "iX<ESC>" "q", X "Xabc" N "def");
    KEYS("@q", X "XXabc" N "def");

    KEYS("qq" ":s/./*/g<BS><BS><BS><BS><BS><BS><BS><BS>" "iY<ESC>" "q", X "YXXabc" N "def");
    KEYS("@q", X "YYXXabc" N "def");

    KEYS("qq" ":s/./*/g<CR>" "q", X "*******" N "def");
    KEYS("j@q", "*******" N X "***");

    // record repeating last command
    data.setText("abc" N "def");
    KEYS(":s/./-/g<CR>", X "---" N "def");
    KEYS("u", X "abc" N "def");
    KEYS("qq" ":<UP><CR>" "q", X "---" N "def");
    KEYS(":s/./!/g<CR>", X "!!!" N "def");
    KEYS("j@q", "!!!" N X "!!!");
}

void FakeVimTester::test_vim_qtcreator()
{
    TestData data;
    setup(&data);

    // Pass input keys in insert mode to underlying editor widget.
    data.doCommand("set passkeys");

    data.setText("" N "");
    KEYS("i" "void f(int arg1) {<cr>// TODO<cr>;",
         "void f(int arg1) {" N
         "    // TODO" N
         "    ;" X N
         "}" N
         "");
    data.doKeys("<ESC>");
    KEYS("cc" "assert(arg1 != 0",
         "void f(int arg1) {" N
         "    // TODO" N
         "    assert(arg1 != 0" X ")" N
         "}" N
         "");
    data.doKeys("<ESC>");
    KEYS("k" "." "A;",
         "void f(int arg1) {" N
         "    assert(arg1 != 0);" X N
         "    assert(arg1 != 0)" N
         "}" N
         "");
    data.doKeys("<ESC>");
    KEYS("j.",
         "void f(int arg1) {" N
         "    assert(arg1 != 0);" N
         "    assert(arg1 != 0)" X ";" N
         "}" N
         "");
    KEYS("4b2#",
         "void f(int " X "arg1) {" N
         "    assert(arg1 != 0);" N
         "    assert(arg1 != 0);" N
         "}" N
         "");
    KEYS("e" "a, int arg2 = 0<esc>" "n",
         "void f(int " X "arg1, int arg2 = 0) {" N
         "    assert(arg1 != 0);" N
         "    assert(arg1 != 0);" N
         "}" N
         "");

    // Record macro.
    KEYS("2j" "qa" "<C-A>" "f!" "2s>=<esc>" "q",
         "void f(int arg1, int arg2 = 0) {" N
         "    assert(arg1 != 0);" N
         "    assert(arg2 >" X "= 0);" N
         "}" N
         "");
    // Replay macro.
    KEYS("n" "@a",
         "void f(int arg1, int arg2 = 0) {" N
         "    assert(arg2 >" X "= 0);" N
         "    assert(arg2 >= 0);" N
         "}" N
         "");

    // Undo.
    KEYS("u",
         "void f(int arg1, int arg2 = 0) {" N
         "    assert(" X "arg1 != 0);" N
         "    assert(arg2 >= 0);" N
         "}" N
         "");
    KEYS("u",
         "void f(int arg1, int arg2 = 0) {" N
         "    assert(arg1 != 0);" N
         "    assert(arg2 " X "!= 0);" N
         "}" N
         "");
    KEYS("u",
         "void f(int arg1, int arg2 = 0) {" N
         "    assert(arg1 != 0);" N
         "    assert(" X "arg1 != 0);" N
         "}" N
         "");
    KEYS("u",
         "void f(int arg1" X ") {" N
         "    assert(arg1 != 0);" N
         "    assert(arg1 != 0);" N
         "}" N
         "");
    KEYS("u",
         "void f(int arg1) {" N
         "    assert(arg1 != 0);" N
         "    assert(arg1 != 0" X ")" N
         "}" N
         "");
    KEYS("u",
         "void f(int arg1) {" N
         "    assert(arg1 != 0" X ")" N
         "    assert(arg1 != 0)" N
         "}" N
         "");
    KEYS("u",
         "void f(int arg1) {" N
         "    " X "// TODO" N
         "    assert(arg1 != 0)" N
         "}" N
         "");
    KEYS("u",
         "void f(int arg1) {" N
         "    // TODO" N
         "    " X ";" N
         "}" N
         "");

    // Redo and occasional undo.
    KEYS("<C-R>",
         "void f(int arg1) {" N
         "    // TODO" N
         "    " X "assert(arg1 != 0)" N
         "}" N
         "");
    KEYS("<C-R>",
         "void f(int arg1) {" N
         "    " X "assert(arg1 != 0)" N
         "    assert(arg1 != 0)" N
         "}" N
         "");
    KEYS("<C-R>",
         "void f(int arg1) {" N
         "    assert(arg1 != 0)" X ";" N
         "    assert(arg1 != 0)" N
         "}" N
         "");
    KEYS("u",
         "void f(int arg1) {" N
         "    assert(arg1 != 0" X ")" N
         "    assert(arg1 != 0)" N
         "}" N
         "");
    KEYS("<C-R>",
         "void f(int arg1) {" N
         "    assert(arg1 != 0)" X ";" N
         "    assert(arg1 != 0)" N
         "}" N
         "");
    KEYS("<C-R>",
         "void f(int arg1) {" N
         "    assert(arg1 != 0);" N
         "    assert(arg1 != 0)" X ";" N
         "}" N
         "");
    KEYS("<C-R>",
         "void f(int arg1" X ", int arg2 = 0) {" N
         "    assert(arg1 != 0);" N
         "    assert(arg1 != 0);" N
         "}" N
         "");
    KEYS("<C-R>",
         "void f(int arg1, int arg2 = 0) {" N
         "    assert(arg1 != 0);" N
         "    assert(" X "arg2 != 0);" N
         "}" N
         "");
    KEYS("<C-R>",
         "void f(int arg1, int arg2 = 0) {" N
         "    assert(arg1 != 0);" N
         "    assert(arg2 " X ">= 0);" N
         "}" N
         "");
    KEYS("<C-R>",
         "void f(int arg1, int arg2 = 0) {" N
         "    assert(" X "arg2 >= 0);" N
         "    assert(arg2 >= 0);" N
         "}" N
         "");
    KEYS("3u",
         "void f(int arg1, int arg2 = 0) {" N
         "    assert(arg1 != 0);" N
         "    assert(" X "arg1 != 0);" N
         "}" N
         "");

    // Repeat last command.
    KEYS("w.",
         "void f(int arg1, int arg2 = 0) {" N
         "    assert(arg1 != 0);" N
         "    assert(arg1 >" X "= 0);" N
         "}" N
         "");

    KEYS("kdd",
         "void f(int arg1, int arg2 = 0) {" N
         "    " X "assert(arg1 >= 0);" N
         "}" N
         "");

    // Make mistakes.
    KEYS("4<esc>3<esc>" "2oif (arg3<bs>2<bs>1 > 0) return true;<esc>",
         "void f(int arg1, int arg2 = 0) {" N
         "    assert(arg1 >= 0);" N
         "    if (arg1 > 0) return true;" N
         "    if (arg1 > 0) return true" X ";" N
         "}" N
         "");

    // Jumps around and change stuff.
    KEYS("gg" "ciw" "bool",
         "bool" X " f(int arg1, int arg2 = 0) {" N
         "    assert(arg1 >= 0);" N
         "    if (arg1 > 0) return true;" N
         "    if (arg1 > 0) return true;" N
         "}" N
         "");
    data.doKeys("<ESC>");
    KEYS("`'",
         "bool f(int arg1, int arg2 = 0) {" N
         "    assert(arg1 >= 0);" N
         "    if (arg1 > 0) return true;" N
         "    if (arg1 > 0) return true" X ";" N
         "}" N
         "");
    KEYS("caW" " false;",
         "bool f(int arg1, int arg2 = 0) {" N
         "    assert(arg1 >= 0);" N
         "    if (arg1 > 0) return true;" N
         "    if (arg1 > 0) return false;" X N
         "}" N
         "");
    data.doKeys("<ESC>");
    KEYS("k.",
         "bool f(int arg1, int arg2 = 0) {" N
         "    assert(arg1 >= 0);" N
         "    if (arg1 > 0) return false" X ";" N
         "    if (arg1 > 0) return false;" N
         "}" N
         "");

    // Undo/redo again.
    KEYS("u",
         "bool f(int arg1, int arg2 = 0) {" N
         "    assert(arg1 >= 0);" N
         "    if (arg1 > 0) return" X " true;" N
         "    if (arg1 > 0) return false;" N
         "}" N
         "");
    KEYS("u",
         "bool f(int arg1, int arg2 = 0) {" N
         "    assert(arg1 >= 0);" N
         "    if (arg1 > 0) return true;" N
         "    if (arg1 > 0) return" X " true;" N
         "}" N
         "");
    KEYS("<C-R>",
         "bool f(int arg1, int arg2 = 0) {" N
         "    assert(arg1 >= 0);" N
         "    if (arg1 > 0) return true;" N
         "    if (arg1 > 0) return" X " false;" N
         "}" N
         "");
    KEYS("u",
         "bool f(int arg1, int arg2 = 0) {" N
         "    assert(arg1 >= 0);" N
         "    if (arg1 > 0) return true;" N
         "    if (arg1 > 0) return" X " true;" N
         "}" N
         "");
    KEYS("u",
         X "void f(int arg1, int arg2 = 0) {" N
         "    assert(arg1 >= 0);" N
         "    if (arg1 > 0) return true;" N
         "    if (arg1 > 0) return true;" N
         "}" N
         "");

    // Record long insert mode.
    KEYS("qb"
         "4s" "bool" // 1
         "<down>"
         "Q_<insert>ASSERT" // 2
         "<down><down>"
         "<insert><bs>2" // 3
         "<c-o>2w"
         "<delete>1" // 4
         "<c-o>:s/true/false<cr><esc>" // 5
         "q",
         "bool f(int arg1, int arg2 = 0) {" N
         "    Q_ASSERT(arg1 >= 0);" N
         "    if (arg1 > 0) return true;" N
         "   " X " if (arg2 > 1) return false;" N
         "}" N
         "");

    KEYS("u", // 5
         "bool f(int arg1, int arg2 = 0) {" N
         "    Q_ASSERT(arg1 >= 0);" N
         "    if (arg1 > 0) return true;" N
         X "    if (arg2 > 1) return true;" N
         "}" N
         "");
    KEYS("u", // 4
         "bool f(int arg1, int arg2 = 0) {" N
         "    Q_ASSERT(arg1 >= 0);" N
         "    if (arg1 > 0) return true;" N
         "    if (arg2 > " X "0) return true;" N
         "}" N
         "");
    KEYS("u", // 3
         "bool f(int arg1, int arg2 = 0) {" N
         "    Q_ASSERT(arg1 >= 0);" N
         "    if (arg1 > 0) return true;" N
         "    if (arg1" X " > 0) return true;" N
         "}" N
         "");
    KEYS("u", // 2
         "bool f(int arg1, int arg2 = 0) {" N
         "    " X "assert(arg1 >= 0);" N
         "    if (arg1 > 0) return true;" N
         "    if (arg1 > 0) return true;" N
         "}" N
         "");
    KEYS("u", // 1
         X "void f(int arg1, int arg2 = 0) {" N
         "    assert(arg1 >= 0);" N
         "    if (arg1 > 0) return true;" N
         "    if (arg1 > 0) return true;" N
         "}" N
         "");

    // Replay.
    KEYS("@b",
         "bool f(int arg1, int arg2 = 0) {" N
         "    Q_ASSERT(arg1 >= 0);" N
         "    if (arg1 > 0) return true;" N
         "   " X " if (arg2 > 1) return false;" N
         "}" N
         "");

    // Return to the first change.
    KEYS("99u" "<C-R>",
         X "void f(int arg1) {" N
         "    // TODO" N
         "    ;" N
         "}" N
         "");
    KEYS("<C-R>",
         "void f(int arg1) {" N
         "    // TODO" N
         "    " X "assert(arg1 != 0)" N
         "}" N
         "");
    KEYS("<C-R>",
         "void f(int arg1) {" N
         "    " X "assert(arg1 != 0)" N
         "    assert(arg1 != 0)" N
         "}" N
         "");

    // Return to the last change.
    KEYS("99<C-R>",
         "bool f(int arg1, int arg2 = 0) {" N
         "    Q_ASSERT(arg1 >= 0);" N
         "    if (arg1 > 0) return true;" N
         "    if (arg2 > 1) return false;" N
         "}" N
         "");

    // Macros
    data.setText(
         "void f(int arg1) {" N
         "}" N
         "");
    KEYS("2o" "#ifdef HAS_FEATURE<cr>doSomething();<cr>"
         "#else<cr>"
         "doSomethingElse<bs><bs><bs><bs>2();<cr>"
         "#endif"
         "<esc>",
         "void f(int arg1) {" N
         "#ifdef HAS_FEATURE" N
         "    doSomething();" N
         "#else" N
         "    doSomething2();" N
         "#endif" N
         "#ifdef HAS_FEATURE" N
         "    doSomething();" N
         "#else" N
         "    doSomething2();" N
         "#endi" X "f" N
         "}" N
         "");
}

} // FakeVim::Internal

#undef N
#undef X

#include "fakevim_test.moc"
