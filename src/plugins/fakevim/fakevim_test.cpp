// Copyright (C) 2016 Lukas Holecek <hluk@email.cz>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

/*!
 * Tests for FakeVim plugin.
 * All test are based on Vim behaviour.
 */

#include "fakevim_test.h"

#include "fakevimhandler.h"
#include "fakevimactions.h"

#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/editormanager/editormanager.h>

#include <mcp/server/toolregistry.h>

#include <texteditor/snippets/snippet.h>
#include <texteditor/syntaxhighlighter.h>
#include <texteditor/textdocument.h>
#include <texteditor/texteditor.h>

#include <utils/filepath.h>
#include <utils/hostosinfo.h>
#include <utils/multitextcursor.h>
#include <utils/stringutils.h>

#include <QApplication>
#include <QFocusEvent>
#include <QJsonObject>
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

    void test_mcp_keys();
    void test_mcp_argument_validation();

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
    void test_vim_plugin_off_leaves_buffer_alone();
    void test_vim_plugin_modeline_of_the_current_buffer();
    void test_vim_plugin_buffer_lifecycle_events();
    void test_vim_plugin_window_events();
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
    void test_vim9_statements();
    void test_vim9_script_level_scope();
    void test_vim_script_script_scope();
    void test_vim_motion_underscore();
    void test_vim_script_col_list();
    void test_vim_register_last_line();
    void test_vim_script_nonblank();
    void test_vim9_exchange();
    void test_vim_script_unpack_rest();
    void test_vim_ex_put();
    void test_vim_script_named_key_string();
    void test_vim_register_carriage_return();
    void test_vim_script_getchar();
    void test_vim_script_eval();
    void test_vim_set_invert();
    void test_vim_command_line_expression();
    void test_vim9_unimpaired();
    void test_vim_script_count_in_mapping();
    void test_vim_script_curly_name();
    void test_vim_script_tr();
    void test_vim_script_string_as_number();
    void test_vim_script_maparg();
    void test_vim_script_flatten();
    void test_vim_script_fullcommand();
    void test_vim_script_indexof();
    void test_vim_script_error_inspection();
    void test_vim_script_maparg_dict();
    void test_vim_script_maplist();
    void test_vim_script_charsearch();
    void test_vim_script_winvar_tabvar();
    void test_vim_script_searchcount();
    void test_vim_script_environment();
    void test_vim_script_getbufinfo();
    void test_vim_script_file_info();
    void test_vim_script_islocked();
    void test_vim_script_autocmd_get();
    void test_vim_script_typename();
    void test_vim_autocmd_bang_clears();
    void test_vim_script_autocmd_add_delete();
    void test_vim_script_filecopy();
    void test_vim_autocmd_textyankpost();
    void test_vim_autocmd_cmdline();
    void test_vim_autocmd_insertcharpre();
    void test_vim_script_one_line_blocks();
    void test_vim_doautocmd_arguments();
    void test_vim_map_nowait();
    void test_vim_substitute_print_flags();
    void test_vim_substitute_count();
    void test_vim_normal_bang();
    void test_vim_autocmd_optionset();
    void test_vim_autocmd_encodingchanged();
    void test_vim_autocmd_insertchange();
    void test_vim_autocmd_funcundefined();
    void test_vim_autocmd_cmdundefined();
    void test_vim_autocmd_cmdlinechanged();
    void test_vim_autocmd_source();
    void test_vim_autocmd_cmdlineleavepre();
    void test_vim_autocmd_insertleavepre();
    void test_vim_autocmd_shell();
    void test_vim_autocmd_filter();
    void test_vim_autocmd_modechanged();
    void test_vim_read_from_command();
    void test_vim_ex_history();
    void test_vim_ex_join_count();
    void test_vim_command_nargs();
    void test_vim_autocmd_filewrite();
    void test_vim_command_write_whole_buffer();
    void test_vim_command_write_append();
    void test_vim_script_type_constants();
    void test_vim_script_searchforward();
    void test_vim_script_environment_vars();
    void test_vim_script_system_functions();
    void test_vim_script_arglist();
    void test_vim_script_fold_queries();
    void test_vim_script_setmatches_and_state();
    void test_vim_script_assert_functions();
    void test_vim_script_misc_builtins();
    void test_vim_script_directory_and_window_stubs();
    void test_vim_command_cd();
    void test_vim_script_more_stubs_and_region();
    void test_vim_line_address_zero_and_counts();
    void test_vim_operator_motion_at_the_edge();
    void test_vim_script_characters_and_bytes();
    void test_vim9_type_annotations();
    void test_vim9_dict_literal();
    void test_vim9_call_funcref_variable();
    void test_vim_script_feedkeys();
    void test_vim_plugin_repeat();
    void test_vim_autocmd_cursor_hold();
    void test_vim_plugin_nohlsearch();
    void test_vim_plugin_vimindent();
    void test_vim_indent_with_expression();
    void test_vim_script_method_call_blanks();
    void test_vim_pattern_white_space();
    void test_vim_pattern_column();
    void test_vim_script_wanted_column();
    void test_vim_script_literal_key_dict();
    void test_vim_script_lazy_ternary();
    void test_vim_plugin_pythonindent();
    void test_vim_pattern_lazy_multi();
    void test_vim_ex_bar_in_single_quotes();
    void test_vim_script_identity_case();
    void test_vim_plugin_indent_files();
    void test_vim_script_searchpos();
    void test_vim_langmap();
    void test_vim_replace_mode_register();
    void test_vim_iskeyword();
    void test_vim_cword();
    void test_vim_join_comment_leader();
    void test_vim_cfile();
    void test_vim_goto_file();
    void test_vim_script_findfile();
    void test_vim_script_line2byte();
    void test_vim_script_simplify();
    void test_vim_script_file_functions();
    void test_vim_script_append();
    void test_vim_script_json();
    void test_vim_script_glob();
    void test_vim_script_bufname();
    void test_vim_reflow_comment();
    void test_vim_script_winsaveview();
    void test_vim_command_gi();
    void test_vim_command_g_underscore();
    void test_vim_command_put_with_indent();
    void test_vim_command_go();
    void test_vim_command_g_ampersand();
    void test_vim_special_registers();
    void test_vim_script_histories();
    void test_vim_command_copy();
    void test_vim_command_align();
    void test_vim_command_print();
    void test_vim_command_z();
    void test_vim_script_execute_and_redir();
    void test_vim_command_sort();
    void test_vim_command_uniq();
    void test_vim_command_smagic();
    void test_vim_command_gn();
    void test_vim_command_changelist();
    void test_vim_script_list_functions();
    void test_vim_command_earlier_later();
    void test_vim_script_buffer_lines();
    void test_vim_substitute_flags();
    void test_vim_line_change_reports();
    void test_vim_search_offset();
    void test_vim_search_messages();
    void test_vim_nrformats();
    void test_vim_insert_ctrl_a_e_y();
    void test_vim_visual_numbers();
    void test_vim_joinspaces_gdefault();
    void test_vim_matchpairs();
    void test_vim_whichwrap();
    void test_vim_command_startinsert();
    void test_vim_command_file();
    void test_vim_command_undojoin();
    void test_vim_reflow_numbered_list();
    void test_vim_script_matchadd();
    void test_vim_rot13();
    void test_vim_g8();
    void test_vim_script_charclass();
    void test_vim_script_charcol_and_charpos();
    void test_vim_script_virtcol2col();
    void test_vim_script_slice();
    void test_vim_script_localtime_and_strptime();
    void test_vim_script_reltime();
    void test_vim_command_marks();
    void test_vim_command_mark();
    void test_vim_command_jumps();
    void test_vim_script_bufexists();
    void test_vim_method_motions();
    void test_vim_insert_whichwrap_brackets();
    void test_vim_script_float_format();
    void test_vim_script_math_functions();
    void test_vim_script_floor_round_fmod();
    void test_vim_script_isinf_float2nr();
    void test_vim_script_and_or_xor();
    void test_vim_script_reduce();
    void test_vim_script_strgetchar_matchstrpos();
    void test_vim_script_extendnew();
    void test_vim_script_srand_rand();
    void test_vim_script_cursorcharpos();
    void test_vim_script_marklist_jumplist_changelist();
    void test_vim_script_changenr_reg_recording_executing();
    void test_vim_script_strutf16len_utf16idx();
    void test_vim_script_glob2regpat_pathshorten_isabsolutepath();
    void test_vim_script_sha256_uri_encode_decode();
    void test_vim_script_matchstrlist_matchbufline();
    void test_vim_script_tabpage_functions();
    void test_vim_script_window_id_functions();
    void test_vim_script_command_line_functions();
    void test_vim_script_getbufoneline_wordcount();
    void test_vim_script_foreach();
    void test_vim_script_combining_and_non_bmp();
    void test_vim_script_window_functions();
    void test_vim_pattern_class_and_lookaround();
    void test_vim_script_changedtick();
    void test_vim_script_delfunction();
    void test_vim_ex_bar_after_file_name();
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
    void test_vim_script_const();
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

    void test_vim_motion_nowhere_to_go();
    void test_vim_marks_follow_the_text();
    void test_vim_visual_marks_when_left();
    void test_vim_script_hlsearch();
    void test_vim_script_heredoc_and_comments();
    void test_macros();

    void test_vim_qtcreator();

    // special tests
    void test_i_cw_i();

    // map test should be last one since it changes default behaviour
    void test_map();
    void test_vim_command_mapclear();
    void test_vim_command_iabbrev();
    void test_vim_no_overwrite_when_editor_takes_keys();
    void test_vim_command_map_bang();

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

void FakeVimTester::test_vim_script_reltime()
{
    // reltime() is the current instant, reltime({start}) how much of it has
    // gone by since, and reltime({start}, {end}) the span between the two - all
    // as a two-number List holding whole seconds and a NANOSECOND remainder (0
    // to 999999999, even where the seconds are negative - a floor split, not a
    // truncating one). reltimestr() writes seconds and the remainder cut down to
    // microseconds as "S.UUUUUU", the seconds part at least 3 characters wide;
    // reltimefloat() is the same span as a Float. Values taken from Vim 9.1.
    TestData data;
    setup(&data);
    QString message;
    data.handler->commandBufferChanged.set(
        [&](const QString &msg, int, int, int) {
            if (!msg.isEmpty() && !msg.startsWith("--"))
                message = msg;
        });
    const auto value = [&](const QString &expr) {
        message.clear();
        data.doCommand("echo " + expr);
        return message;
    };

    data.doCommand("let g:t = reltime()");
    QCOMPARE(value("type(g:t)"), QLatin1String("3"));
    QCOMPARE(value("len(g:t)"), QLatin1String("2"));
    QCOMPARE(value("reltimestr(g:t) =~ '^\\s*\\d\\+\\.\\d\\+$'"), QLatin1String("1"));
    QCOMPARE(value("type(reltimefloat(g:t))"), QLatin1String("5"));
    QCOMPARE(value("reltimefloat(g:t) >= 0.0"), QLatin1String("1"));

    // Elapsed since a moment, and the span between two moments.
    data.doCommand("let g:a = reltime(g:t)");
    QCOMPARE(value("type(g:a)"), QLatin1String("3"));
    QCOMPARE(value("len(g:a)"), QLatin1String("2"));
    data.doCommand("let g:b = reltime(g:t, reltime())");
    QCOMPARE(value("type(g:b)"), QLatin1String("3"));
    QCOMPARE(value("len(g:b)"), QLatin1String("2"));
    // A span of no time at all.
    QCOMPARE(value("reltimestr(reltime(g:t, g:t))"), QLatin1String("  0.000000"));
    QCOMPARE(value("reltimefloat(reltime(g:t, g:t))"), QLatin1String("0.0"));

    // The seconds part is at least 3 characters wide, padded with spaces; the
    // remainder is nanoseconds, cut down to microseconds by truncation, not
    // rounding - a value of its own can be built to check the exact width,
    // padding and the unit.
    QCOMPARE(value("reltimestr([0, 841000])"), QLatin1String("  0.000841"));
    QCOMPARE(value("reltimefloat([0, 841000])"), QLatin1String("8.41e-4"));
    QCOMPARE(value("reltimestr([3473822, 398842000])"), QLatin1String("3473822.398842"));
    QCOMPARE(value("reltimefloat([3473822, 398842000])"), QLatin1String("3473822.398842"));
    // A span running backwards writes its nanosecond remainder as a POSITIVE
    // number even though the seconds are negative - so "-1.999000" is the way
    // Vim writes -0.001 seconds, not -1.999 - while reltimefloat() combines them
    // into an ordinary signed number.
    QCOMPARE(value("reltimestr([-1, 999000000])"), QLatin1String(" -1.999000"));
    QCOMPARE(value("reltimefloat([-1, 999000000])"), QLatin1String("-0.001"));

    data.doCommand("unlet! g:t g:a g:b");
}

void FakeVimTester::test_vim_command_marks()
{
    // ":marks" lists the "'" mark first, then a-z/A-Z alphabetically, then any
    // other automatic mark by character code - the order Vim itself uses, not
    // the order the marks were set in. An argument filters the list down to
    // the marks named in it. Values taken from Vim 9.1.
    TestData data;
    setup(&data);
    QString info;
    data.handler->extraInformationChanged.set([&](const QString &text) { info = text; });
    const auto shown = [&](const QString &command) {
        info.clear();
        data.doCommand(command);
        return info;
    };

    data.setText(X "one" N "two" N "three" N "four" N "five");
    KEYS("ggmaj3Gmbk", "one" N X "two" N "three" N "four" N "five");
    QCOMPARE(shown("marks"),
             QLatin1String("mark line  col file/text\n"
                            " '      2    0 two\n"
                            " a      1    0 one\n"
                            " b      3    0 three\n"
                            " \"      1    0 one\n"
                            " [      1    0 one\n"
                            " ]      5    0 five\n"));
    QCOMPARE(shown("marks ab"),
             QLatin1String("mark line  col file/text\n"
                            " a      1    0 one\n"
                            " b      3    0 three\n"));
}

void FakeVimTester::test_vim_command_mark()
{
    // ":mark"/":k" SET a mark, unlike ":marks" which only lists them. Values
    // taken from Vim 9.1: the mark sits at column 1 (index 0) of the addressed
    // line regardless of the cursor's own column, an uppercase name works the
    // same way, a digit name is accepted with no error (not letters-only), no
    // name at all is E471, and two or more characters is E488.
    TestData data;
    setup(&data);
    QString message;
    data.handler->commandBufferChanged.set(
        [&](const QString &msg, int, int, int) {
            if (!msg.isEmpty() && !msg.startsWith("--"))
                message = msg;
        });

    data.setText(X "one" N "two" N "three" N "four");
    data.doCommand("2mark t");
    KEYS("gg" "'t", "one" N X "two" N "three" N "four");
    KEYS("gg" "`t", "one" N X "two" N "three" N "four");

    // ":k" glues the name directly onto itself with no space, so the general
    // command/argument splitter (which stops at the first non-letter) never
    // gets a chance to separate them; handleExMarkCommand() has to.
    data.setText(X "one" N "two" N "three" N "four");
    data.doCommand("3ku");
    KEYS("gg" "'u", "one" N "two" N X "three" N "four");

    // An uppercase name is the same command, and a digit name is accepted -
    // Vim does not restrict this to letters.
    data.setText(X "one" N "two" N "three" N "four");
    data.doCommand("2mark T");
    KEYS("gg" "'T", "one" N X "two" N "three" N "four");
    data.doCommand("3mark 1");
    KEYS("gg" "'1", "one" N "two" N X "three" N "four");

    message.clear();
    data.doCommand("mark");
    QCOMPARE(message, QLatin1String("E471: Argument required"));

    message.clear();
    data.doCommand("2mark ab");
    QCOMPARE(message, QLatin1String("E488: Trailing characters: ab"));
}

void FakeVimTester::test_vim_command_jumps()
{
    // ":jumps" lists the buffer's jump list, oldest (farthest via CTRL-O)
    // first, down to the newest (nearest), followed by a line marking the
    // live position. Values taken from Vim 9.1.
    TestData data;
    setup(&data);
    QString info;
    data.handler->extraInformationChanged.set([&](const QString &text) { info = text; });
    const auto shown = [&](const QString &command) {
        info.clear();
        data.doCommand(command);
        return info;
    };

    data.setText(X "one" N "two" N "three" N "four" N "five");
    KEYS("4G2G5G1G", X "one" N "two" N "three" N "four" N "five");
    QCOMPARE(shown("jumps"),
             QLatin1String(" jump line  col file/text\n"
                            "   4     1    0 one\n"
                            "   3     4    0 four\n"
                            "   2     2    0 two\n"
                            "   1     5    0 five\n"
                            ">\n"));
}

void FakeVimTester::test_vim_script_bufexists()
{
    // bufexists()/buflisted()/bufloaded() differ from bufnr()/bufname(): a
    // string argument is always a buffer NAME there, never the "%" or ""
    // that mean "the current buffer" for those two - "%" and "" are simply
    // names no buffer ever has, so they answer 0 here. The one buffer this
    // handler knows is always both listed and loaded, so all three agree.
    // Values taken from Vim 9.1.
    TestData data;
    setup(&data);
    QString message;
    data.handler->commandBufferChanged.set(
        [&](const QString &msg, int, int, int) {
            if (!msg.isEmpty() && !msg.startsWith("--"))
                message = msg;
        });
    const auto value = [&](const QString &expr) {
        message.clear();
        data.doCommand("echo " + expr);
        return message;
    };
    data.handler->setCurrentFileName("cl.txt");

    const QStringList functions = {"bufexists", "buflisted", "bufloaded"};
    for (const QString &fn : functions) {
        QCOMPARE(value(fn + "(bufnr(''))"), QLatin1String("1"));
        QCOMPARE(value(fn + "('cl.txt')"), QLatin1String("1"));
        QCOMPARE(value(fn + "(999)"), QLatin1String("0"));
        QCOMPARE(value(fn + "('nope.txt')"), QLatin1String("0"));
        QCOMPARE(value(fn + "(0)"), QLatin1String("0"));
        QCOMPARE(value(fn + "('%')"), QLatin1String("0"));
        QCOMPARE(value(fn + "('')"), QLatin1String("0"));
    }
}

void FakeVimTester::test_vim_method_motions()
{
    // "]m"/"[m" go to the next/previous "{", "]M"/"[M" to the next/previous
    // "}" - simply the nearest brace in that direction, never the one the
    // cursor already sits on (repeating the same command always advances).
    // Values taken from Vim 9.1.
    TestData data;
    setup(&data);

    data.setText(
        X "class C {" N
        "void f() {" N
        "  a;" N
        "}" N
        "void g() {" N
        "  b;" N
        "}" N
        "}");
    KEYS("]m",
         "class C " X "{" N
         "void f() {" N
         "  a;" N
         "}" N
         "void g() {" N
         "  b;" N
         "}" N
         "}");
    KEYS("]m",
         "class C {" N
         "void f() " X "{" N
         "  a;" N
         "}" N
         "void g() {" N
         "  b;" N
         "}" N
         "}");
    KEYS("]m",
         "class C {" N
         "void f() {" N
         "  a;" N
         "}" N
         "void g() " X "{" N
         "  b;" N
         "}" N
         "}");

    data.setText(
        X "class C {" N
        "void f() {" N
        "  a;" N
        "}" N
        "void g() {" N
        "  b;" N
        "}" N
        "}");
    KEYS("3]m",
         "class C {" N
         "void f() {" N
         "  a;" N
         "}" N
         "void g() " X "{" N
         "  b;" N
         "}" N
         "}");

    data.setText(
        "class C {" N
        "void f() {" N
        "  a;" N
        "}" N
        "void g() {" N
        "  b;" N
        "}" N
        X "}");
    KEYS("[m",
         "class C {" N
         "void f() {" N
         "  a;" N
         "}" N
         "void g() " X "{" N
         "  b;" N
         "}" N
         "}");
    KEYS("[m",
         "class C {" N
         "void f() " X "{" N
         "  a;" N
         "}" N
         "void g() {" N
         "  b;" N
         "}" N
         "}");

    data.setText(
        "class C {" N
        "void f() {" N
        X "  a;" N
        "}" N
        "void g() {" N
        "  b;" N
        "}" N
        "}");
    KEYS("]M",
         "class C {" N
         "void f() {" N
         "  a;" N
         X "}" N
         "void g() {" N
         "  b;" N
         "}" N
         "}");
    KEYS("]M",
         "class C {" N
         "void f() {" N
         "  a;" N
         "}" N
         "void g() {" N
         "  b;" N
         X "}" N
         "}");

    data.setText(
        "class C {" N
        "void f() {" N
        "  a;" N
        "}" N
        "void g() {" N
        "  b;" N
        "}" N
        X "}");
    KEYS("[M",
         "class C {" N
         "void f() {" N
         "  a;" N
         "}" N
         "void g() {" N
         "  b;" N
         X "}" N
         "}");

    data.setText(
        "class C {" N
        "void f() {" N
        "  a;" N
        "}" N
        "void g() {" N
        X "  b;" N
        "}" N
        "}");
    KEYS("[M",
         "class C {" N
         "void f() {" N
         "  a;" N
         X "}" N
         "void g() {" N
         "  b;" N
         "}" N
         "}");

    // The same rule when a brace sits alone on its own line.
    data.setText(
        X "class C" N
        "{" N
        "void f()" N
        "{");
    KEYS("]m",
         "class C" N
         X "{" N
         "void f()" N
         "{");
    KEYS("]m",
         "class C" N
         "{" N
         "void f()" N
         X "{");

    // A plain motion, usable standalone or as an operator's target.
    data.setText(X "class C {");
    KEYS("]mx", "class C ");
    data.setText(X "class C {");
    KEYS("d]m", "{");
}

void FakeVimTester::test_vim_insert_whichwrap_brackets()
{
    // 'whichwrap' "[" lets Insert mode's <Left> reach into the previous
    // line, "]" lets <Right> reach into the next one; by default neither
    // crosses. Measured directly against Vim 9.1 - the queue described the
    // two "stays put" positions as column 2, but the cursor there actually
    // sits one past the last real character (column 3 of a two-character
    // line), not on it.
    TestData data;
    setup(&data);

    data.setText(X "ab" N "cd");
    KEYS("A<Right>", "ab" X N "cd");

    data.setText("ab" N X "cd");
    KEYS("i<Left>", "ab" N X "cd");

    data.doCommand("set whichwrap+=]");
    data.setText(X "ab" N "cd");
    KEYS("A<Right>", "ab" N X "cd");

    data.doCommand("set whichwrap+=[");
    data.setText("ab" N X "cd");
    KEYS("i<Left>", "ab" X N "cd");
}

void FakeVimTester::test_vim_script_float_format()
{
    // Vim writes a Float with six decimals and always keeps a decimal point,
    // so a whole number reads "1.0" - it is not C's "%g", which would drop
    // the point and pad the exponent. Fixed notation holds for
    // 1e-3 <= |value| < 1e7, exponential outside it, and the exponent carries
    // neither a "+" nor padding zeros. Values taken from Vim 9.1.
    TestData data;
    setup(&data);
    QString message;
    data.handler->commandBufferChanged.set(
        [&](const QString &msg, int, int, int) {
            if (!msg.isEmpty() && !msg.startsWith("--"))
                message = msg;
        });
    const auto value = [&](const QString &expr) {
        message.clear();
        data.doCommand("echo " + expr);
        return message;
    };
    // Both the plain conversion an "echo" does and the quoted form string()
    // builds go through the same formatting.
    const auto both = [&](const QString &expr, const QString &expected) {
        QCOMPARE(value(expr), expected);
        QCOMPARE(value("string(" + expr + ')'), expected);
    };

    // A whole number keeps one decimal; a fraction keeps only what it needs.
    both("0.0", "0.0");
    both("1.0", "1.0");
    both("2.0", "2.0");
    both("100.0", "100.0");
    both("1.5", "1.5");
    both("0.1", "0.1");
    both("-2.5", "-2.5");
    both("1234.5678", "1234.5678");

    // Six decimals is all there is, so anything longer is cut to fit.
    both("3.14159265358979", "3.141593");
    both("1.0000005", "1.000001");
    both("0.0012345678", "0.001235");

    // The bounds of fixed notation: 1e-3 and 1e7 themselves fall on either
    // side of it.
    both("0.001", "0.001");
    both("-0.001", "-0.001");
    both("999999.0", "999999.0");
    both("1000000.0", "1000000.0");
    both("9999999.5", "9999999.5");
    both("1234567.891234", "1234567.891234");
    both("3473822.398842", "3473822.398842");

    // Outside them the exponent is written bare - no "+", no padding zeros -
    // and the mantissa keeps its own decimal point.
    both("10000000.0", "1.0e7");
    both("12345678.91234", "1.234568e7");
    both("123456789.0", "1.234568e8");
    both("0.0009999", "9.999e-4");
    both("0.000841", "8.41e-4");
    both("0.0001", "1.0e-4");
    both("0.00012345678", "1.234568e-4");
    both("0.0000005", "5.0e-7");
    both("1.0e10", "1.0e10");
    both("1.0e-10", "1.0e-10");
    both("1.0e-99", "1.0e-99");
    both("1.0e100", "1.0e100");
    both("1.0e-100", "1.0e-100");

    // Six decimals in exponential notation too, so a number just past 1e7
    // reads the same as 1e7 itself - as it does in Vim.
    both("10000001.0", "1.0e7");
    both("99999999.0", "1.0e8");
}


void FakeVimTester::test_vim_script_math_functions()
{
    // sqrt()/exp()/log()/log10()/pow() and the trig set all answer a Float,
    // even for a Number argument, and reject anything that is not a Number
    // or a Float outright. atan2() takes (y, x), not (x, y). Values taken
    // from Vim 9.1.
    TestData data;
    setup(&data);
    QString message;
    data.handler->commandBufferChanged.set(
        [&](const QString &msg, int, int, int) {
            if (!msg.isEmpty() && !msg.startsWith("--"))
                message = msg;
        });
    const auto value = [&](const QString &expr) {
        message.clear();
        data.doCommand("echo " + expr);
        return message;
    };

    QCOMPARE(value("sqrt(2)"), QLatin1String("1.414214"));
    QCOMPARE(value("sqrt(4.0)"), QLatin1String("2.0"));
    QCOMPARE(value("sqrt(0)"), QLatin1String("0.0"));
    QCOMPARE(value("sqrt(-1.0)"), QLatin1String("nan"));
    QCOMPARE(value("sqrt(-0.0)"), QLatin1String("-0.0"));
    QCOMPARE(value("type(sqrt(4))"), QLatin1String("5"));

    QCOMPARE(value("exp(1)"), QLatin1String("2.718282"));
    QCOMPARE(value("exp(0)"), QLatin1String("1.0"));

    QCOMPARE(value("log(1)"), QLatin1String("0.0"));
    QCOMPARE(value("log(10)"), QLatin1String("2.302585"));
    QCOMPARE(value("log(0)"), QLatin1String("-inf"));
    QCOMPARE(value("log(-1)"), QLatin1String("nan"));
    QCOMPARE(value("log10(1000)"), QLatin1String("3.0"));

    QCOMPARE(value("pow(2, 10)"), QLatin1String("1024.0"));
    QCOMPARE(value("pow(2, 0.5)"), QLatin1String("1.414214"));
    QCOMPARE(value("pow(-2, 3)"), QLatin1String("-8.0"));
    QCOMPARE(value("pow(0, 0)"), QLatin1String("1.0"));
    QCOMPARE(value("pow(-1, 0.5)"), QLatin1String("nan"));

    QCOMPARE(value("sin(0)"), QLatin1String("0.0"));
    QCOMPARE(value("cos(0)"), QLatin1String("1.0"));
    QCOMPARE(value("tan(0)"), QLatin1String("0.0"));
    QCOMPARE(value("sin(1)"), QLatin1String("0.841471"));
    QCOMPARE(value("cos(1)"), QLatin1String("0.540302"));
    QCOMPARE(value("tan(1)"), QLatin1String("1.557408"));

    QCOMPARE(value("asin(1)"), QLatin1String("1.570796"));
    QCOMPARE(value("acos(1)"), QLatin1String("0.0"));
    QCOMPARE(value("acos(0)"), QLatin1String("1.570796"));
    QCOMPARE(value("atan(1)"), QLatin1String("0.785398"));

    // atan2(y, x): the point (1,1) is at 45 degrees, (1,-1) at 135, and so on.
    QCOMPARE(value("atan2(1, 1)"), QLatin1String("0.785398"));
    QCOMPARE(value("atan2(-1, 1)"), QLatin1String("-0.785398"));
    QCOMPARE(value("atan2(0, -1)"), QLatin1String("3.141593"));
    QCOMPARE(value("atan2(1, 0)"), QLatin1String("1.570796"));

    QCOMPARE(value("sinh(1)"), QLatin1String("1.175201"));
    QCOMPARE(value("cosh(1)"), QLatin1String("1.543081"));
    QCOMPARE(value("tanh(1)"), QLatin1String("0.761594"));

    QCOMPARE(value("sqrt('abc')"), QLatin1String("E808: Number or Float required"));
    QCOMPARE(value("pow('abc', 1)"), QLatin1String("E808: Number or Float required"));

    for (const QString &fn : {QString("sqrt"), QString("exp"), QString("log"),
                              QString("log10"), QString("sin"), QString("cos"),
                              QString("tan"), QString("asin"), QString("acos"),
                              QString("atan"), QString("sinh"), QString("cosh"),
                              QString("tanh"), QString("pow"), QString("atan2")}) {
        QCOMPARE(value("exists('*" + fn + "')"), QLatin1String("1"));
    }
}

void FakeVimTester::test_vim_script_floor_round_fmod()
{
    // floor()/ceil()/trunc()/round() and fmod() all answer a Float, even for
    // a Number argument. round() rounds HALF AWAY FROM ZERO, not to even and
    // not toward +inf; fmod()'s sign follows the DIVIDEND, not the divisor.
    // The "-0.0" answers print with their sign, per the float formatting.
    // Values taken from Vim 9.1.
    TestData data;
    setup(&data);
    QString message;
    data.handler->commandBufferChanged.set(
        [&](const QString &msg, int, int, int) {
            if (!msg.isEmpty() && !msg.startsWith("--"))
                message = msg;
        });
    const auto value = [&](const QString &expr) {
        message.clear();
        data.doCommand("echo " + expr);
        return message;
    };

    QCOMPARE(value("floor(1.7)"), QLatin1String("1.0"));
    QCOMPARE(value("floor(-1.7)"), QLatin1String("-2.0"));
    QCOMPARE(value("floor(2.0)"), QLatin1String("2.0"));
    QCOMPARE(value("floor(-0.5)"), QLatin1String("-1.0"));

    QCOMPARE(value("ceil(1.2)"), QLatin1String("2.0"));
    QCOMPARE(value("ceil(-1.2)"), QLatin1String("-1.0"));
    QCOMPARE(value("ceil(2.0)"), QLatin1String("2.0"));
    QCOMPARE(value("ceil(-0.5)"), QLatin1String("-0.0"));

    QCOMPARE(value("round(2.5)"), QLatin1String("3.0"));
    QCOMPARE(value("round(-2.5)"), QLatin1String("-3.0"));
    QCOMPARE(value("round(0.5)"), QLatin1String("1.0"));
    QCOMPARE(value("round(-0.5)"), QLatin1String("-1.0"));
    QCOMPARE(value("round(2.4)"), QLatin1String("2.0"));
    QCOMPARE(value("round(-2.4)"), QLatin1String("-2.0"));
    QCOMPARE(value("round(3.0)"), QLatin1String("3.0"));

    QCOMPARE(value("trunc(2.7)"), QLatin1String("2.0"));
    QCOMPARE(value("trunc(-2.7)"), QLatin1String("-2.0"));
    QCOMPARE(value("trunc(-0.5)"), QLatin1String("-0.0"));

    QCOMPARE(value("fmod(7, 3)"), QLatin1String("1.0"));
    QCOMPARE(value("fmod(-7, 3)"), QLatin1String("-1.0"));
    QCOMPARE(value("fmod(7, -3)"), QLatin1String("1.0"));
    QCOMPARE(value("fmod(7.5, 2)"), QLatin1String("1.5"));
    QCOMPARE(value("fmod(1, 0)"), QLatin1String("nan"));

    // A bare Number is accepted too, converting to a Float just like the
    // rest of the math functions.
    QCOMPARE(value("floor(2)"), QLatin1String("2.0"));
    QCOMPARE(value("type(floor(2))"), QLatin1String("5"));
    QCOMPARE(value("floor('abc')"), QLatin1String("E808: Number or Float required"));

    for (const QString &fn : {QString("floor"), QString("ceil"),
                              QString("trunc"), QString("round"),
                              QString("fmod")}) {
        QCOMPARE(value("exists('*" + fn + "')"), QLatin1String("1"));
    }
}

void FakeVimTester::test_vim_script_isinf_float2nr()
{
    // isinf() is SIGNED, not a boolean; isnan() is the usual predicate.
    // float2nr() truncates toward zero and saturates rather than
    // overflowing, but ASYMMETRICALLY: only a NaN answers LLONG_MIN, while
    // -inf (and an overflowing negative finite value) answers -LLONG_MAX.
    // str2float() parses a leading prefix, taking hex and "inf" too, and
    // answers 0.0 for nothing usable. Values taken from Vim 9.1.
    TestData data;
    setup(&data);
    QString message;
    data.handler->commandBufferChanged.set(
        [&](const QString &msg, int, int, int) {
            if (!msg.isEmpty() && !msg.startsWith("--"))
                message = msg;
        });
    const auto value = [&](const QString &expr) {
        message.clear();
        data.doCommand("echo " + expr);
        return message;
    };

    QCOMPARE(value("isinf(1.0/0.0)"), QLatin1String("1"));
    QCOMPARE(value("isinf(-1.0/0.0)"), QLatin1String("-1"));
    QCOMPARE(value("isinf(1.0)"), QLatin1String("0"));
    QCOMPARE(value("isinf(0.0/0.0)"), QLatin1String("0"));
    QCOMPARE(value("isinf(5)"), QLatin1String("0"));

    QCOMPARE(value("isnan(0.0/0.0)"), QLatin1String("1"));
    QCOMPARE(value("isnan(1.0)"), QLatin1String("0"));
    QCOMPARE(value("isnan(1.0/0.0)"), QLatin1String("0"));

    QCOMPARE(value("float2nr(3.95)"), QLatin1String("3"));
    QCOMPARE(value("float2nr(-3.95)"), QLatin1String("-3"));
    QCOMPARE(value("type(float2nr(1.5))"), QLatin1String("0"));
    QCOMPARE(value("float2nr(1.0e20)"), QLatin1String("9223372036854775807"));
    QCOMPARE(value("float2nr(-1.0e20)"), QLatin1String("-9223372036854775807"));
    QCOMPARE(value("float2nr(1.0/0.0)"), QLatin1String("9223372036854775807"));
    QCOMPARE(value("float2nr(-1.0/0.0)"), QLatin1String("-9223372036854775807"));
    QCOMPARE(value("float2nr(0.0/0.0)"), QLatin1String("-9223372036854775808"));

    QCOMPARE(value("str2float('1.5')"), QLatin1String("1.5"));
    QCOMPARE(value("str2float('  2.5abc')"), QLatin1String("2.5"));
    QCOMPARE(value("str2float('abc')"), QLatin1String("0.0"));
    QCOMPARE(value("str2float('')"), QLatin1String("0.0"));
    QCOMPARE(value("str2float('-3.25')"), QLatin1String("-3.25"));
    QCOMPARE(value("str2float('1e3')"), QLatin1String("1000.0"));
    QCOMPARE(value("str2float('1.5e')"), QLatin1String("1.5"));
    QCOMPARE(value("str2float('0x10')"), QLatin1String("16.0"));
    QCOMPARE(value("str2float('inf')"), QLatin1String("inf"));
    QCOMPARE(value("type(str2float('1'))"), QLatin1String("5"));

    for (const QString &fn : {QString("isinf"), QString("isnan"),
                              QString("float2nr"), QString("str2float")}) {
        QCOMPARE(value("exists('*" + fn + "')"), QLatin1String("1"));
    }
}

void FakeVimTester::test_vim_script_and_or_xor()
{
    // Number in, Number out, two's complement on 64 bits. Unlike the math
    // functions, a Float argument is an ERROR, not a conversion. Values
    // taken from Vim 9.1.
    TestData data;
    setup(&data);
    QString message;
    data.handler->commandBufferChanged.set(
        [&](const QString &msg, int, int, int) {
            if (!msg.isEmpty() && !msg.startsWith("--"))
                message = msg;
        });
    const auto value = [&](const QString &expr) {
        message.clear();
        data.doCommand("echo " + expr);
        return message;
    };

    QCOMPARE(value("and(5, 3)"), QLatin1String("1"));
    QCOMPARE(value("or(5, 3)"), QLatin1String("7"));
    QCOMPARE(value("xor(5, 3)"), QLatin1String("6"));

    QCOMPARE(value("invert(5)"), QLatin1String("-6"));
    QCOMPARE(value("invert(0)"), QLatin1String("-1"));
    QCOMPARE(value("invert(-1)"), QLatin1String("0"));

    QCOMPARE(value("and(-1, 7)"), QLatin1String("7"));
    QCOMPARE(value("and(-2, -3)"), QLatin1String("-4"));
    QCOMPARE(value("or(-2, 3)"), QLatin1String("-1"));
    QCOMPARE(value("xor(-1, -1)"), QLatin1String("0"));

    QCOMPARE(value("and(1.5, 2)"), QLatin1String("E805: Using a Float as a Number"));
    QCOMPARE(value("invert(1.5)"), QLatin1String("E805: Using a Float as a Number"));

    for (const QString &fn : {QString("and"), QString("or"), QString("xor"),
                              QString("invert")}) {
        QCOMPARE(value("exists('*" + fn + "')"), QLatin1String("1"));
    }
}

void FakeVimTester::test_vim_script_reduce()
{
    // reduce({list}, {func} [, {initial}]) - with no initial value the
    // first element seeds the accumulator and the fold starts from the
    // second; an empty list then has nothing to seed it with. Values taken
    // from Vim 9.1.
    TestData data;
    setup(&data);
    QString message;
    data.handler->commandBufferChanged.set(
        [&](const QString &msg, int, int, int) {
            if (!msg.isEmpty() && !msg.startsWith("--"))
                message = msg;
        });
    const auto value = [&](const QString &expr) {
        message.clear();
        data.doCommand("echo " + expr);
        return message;
    };

    QCOMPARE(value("reduce([1, 2, 3], {a, b -> a + b})"), QLatin1String("6"));
    QCOMPARE(value("reduce([1, 2, 3], {a, b -> a + b}, 10)"), QLatin1String("16"));
    QCOMPARE(value("reduce([], {a, b -> a + b}, 5)"), QLatin1String("5"));
    QCOMPARE(value("reduce(['a', 'b'], {a, b -> a . b}, '')"), QLatin1String("ab"));
    QCOMPARE(value("reduce([], {a, b -> a + b})"),
             QLatin1String("E998: Reduce of an empty List with no initial value"));

    QCOMPARE(value("exists('*reduce')"), QLatin1String("1"));
}

void FakeVimTester::test_vim_script_strgetchar_matchstrpos()
{
    // strgetchar() answers the CODEPOINT, not a lone surrogate half for a
    // character outside the Basic Multilingual Plane, and -1 for anything
    // out of range. matchstrpos() gives three elements for a String
    // subject, four for a List one - an extra list index in front of
    // "start"; the optional third argument is a byte column for a String,
    // but a LIST INDEX for a List. Values taken from Vim 9.1.
    TestData data;
    setup(&data);
    QString message;
    data.handler->commandBufferChanged.set(
        [&](const QString &msg, int, int, int) {
            if (!msg.isEmpty() && !msg.startsWith("--"))
                message = msg;
        });
    const auto value = [&](const QString &expr) {
        message.clear();
        data.doCommand("echo " + expr);
        return message;
    };

    data.doCommand("let g:s = 'abc'");
    QCOMPARE(value("strgetchar(g:s, 0)"), QLatin1String("97"));
    QCOMPARE(value("strgetchar(g:s, 2)"), QLatin1String("99"));
    QCOMPARE(value("strgetchar(g:s, 5)"), QLatin1String("-1"));
    QCOMPARE(value("strgetchar(g:s, -1)"), QLatin1String("-1"));
    QCOMPARE(value("strgetchar('', 0)"), QLatin1String("-1"));

    // "a", U+00E4, "b", U+20AC - built from code points to keep the source
    // 7-bit ASCII.
    const QString mb = QString("a") + QChar(0x00E4) + QString("b") + QChar(0x20AC);
    data.doCommand("let g:mb = '" + mb + "'");
    QCOMPARE(value("strgetchar(g:mb, 0)"), QLatin1String("97"));
    QCOMPARE(value("strgetchar(g:mb, 1)"), QLatin1String("228"));
    QCOMPARE(value("strgetchar(g:mb, 2)"), QLatin1String("98"));
    QCOMPARE(value("strgetchar(g:mb, 3)"), QLatin1String("8364"));

    // "a", U+1F600 (a non-BMP character, a surrogate pair in a QString), "b".
    const char32_t emojiCodepoint = 0x1F600;
    const QString emoji = QString("a") + QString::fromUcs4(&emojiCodepoint, 1) + QString("b");
    data.doCommand("let g:e = '" + emoji + "'");
    QCOMPARE(value("strgetchar(g:e, 0)"), QLatin1String("97"));
    QCOMPARE(value("strgetchar(g:e, 1)"), QLatin1String("128512"));
    QCOMPARE(value("strgetchar(g:e, 2)"), QLatin1String("98"));

    QCOMPARE(value("matchstrpos('testing', 'ing')"), QLatin1String("['ing', 4, 7]"));
    QCOMPARE(value("matchstrpos('testing', 'xyz')"), QLatin1String("['', -1, -1]"));
    QCOMPARE(value("matchstrpos('aXbX', 'X', 2)"), QLatin1String("['X', 3, 4]"));
    QCOMPARE(value("matchstrpos(['a', 'b'], 'b')"), QLatin1String("['b', 1, 0, 1]"));
    QCOMPARE(value("matchstrpos(['a', 'b'], 'z')"), QLatin1String("['', -1, -1, -1]"));
    QCOMPARE(value("matchstrpos([], 'a')"), QLatin1String("['', -1, -1, -1]"));
    QCOMPARE(value("matchstrpos(['xa', 'xb', 'xa'], 'xa', 1)"),
             QLatin1String("['xa', 2, 0, 2]"));
    QCOMPARE(value("matchstrpos(['xa', 'xb', 'xa'], 'xa', 0)"),
             QLatin1String("['xa', 0, 0, 2]"));
    QCOMPARE(value("type(matchstrpos('abc', 'b'))"), QLatin1String("3"));

    for (const QString &fn : {QString("strgetchar"), QString("matchstrpos")}) {
        QCOMPARE(value("exists('*" + fn + "')"), QLatin1String("1"));
    }
}

void FakeVimTester::test_vim_script_extendnew()
{
    // Like extend(), but the first argument is left alone - a version that
    // just called extend() would pass the two success cases below without
    // actually doing that, so this checks the original is untouched too, not
    // only the answer it hands back. Values taken from Vim 9.1.
    TestData data;
    setup(&data);
    QString message;
    data.handler->commandBufferChanged.set(
        [&](const QString &msg, int, int, int) {
            if (!msg.isEmpty() && !msg.startsWith("--"))
                message = msg;
        });
    const auto value = [&](const QString &expr) {
        message.clear();
        data.doCommand("echo " + expr);
        return message;
    };

    data.doCommand("let g:a = [1, 2]");
    data.doCommand("let g:b = extendnew(g:a, [3])");
    QCOMPARE(value("string(g:a)"), QLatin1String("[1, 2]"));
    QCOMPARE(value("string(g:b)"), QLatin1String("[1, 2, 3]"));

    data.doCommand("let g:d1 = {'a': 1}");
    data.doCommand("let g:d2 = extendnew(g:d1, {'b': 2})");
    QCOMPARE(value("string(g:d1)"), QLatin1String("{'a': 1}"));
    QCOMPARE(value("string(g:d2)"), QLatin1String("{'a': 1, 'b': 2}"));

    QCOMPARE(value("exists('*extendnew')"), QLatin1String("1"));
}

void FakeVimTester::test_vim_script_srand_rand()
{
    // Vim's actual generator is xoshiro128** and is not cloned here (a
    // "carry the algorithm" decision, not a queue ticket) - so only the
    // STRUCTURE is asserted, deliberately not the numbers themselves:
    // srand() gives a List of 4 Numbers, rand() a Number in [0, 2^32), and
    // one seed replays one sequence.
    TestData data;
    setup(&data);
    QString message;
    data.handler->commandBufferChanged.set(
        [&](const QString &msg, int, int, int) {
            if (!msg.isEmpty() && !msg.startsWith("--"))
                message = msg;
        });
    const auto value = [&](const QString &expr) {
        message.clear();
        data.doCommand("echo " + expr);
        return message;
    };

    QCOMPARE(value("type(srand(4))"), QLatin1String("3"));
    QCOMPARE(value("len(srand(4))"), QLatin1String("4"));
    QCOMPARE(value("type(srand())"), QLatin1String("3"));
    QCOMPARE(value("len(srand())"), QLatin1String("4"));

    data.doCommand("let g:s = srand(4)");
    QCOMPARE(value("type(rand(g:s))"), QLatin1String("0"));
    QCOMPARE(value("rand(g:s) >= 0 && rand(g:s) < 4294967296"), QLatin1String("1"));
    QCOMPARE(value("type(rand())"), QLatin1String("0"));
    QCOMPARE(value("rand() >= 0 && rand() < 4294967296"), QLatin1String("1"));

    // One seed replays one sequence.
    data.doCommand("let g:s1 = srand(42)");
    data.doCommand("let g:a = [rand(g:s1), rand(g:s1), rand(g:s1)]");
    data.doCommand("let g:s2 = srand(42)");
    data.doCommand("let g:b = [rand(g:s2), rand(g:s2), rand(g:s2)]");
    QCOMPARE(value("g:a == g:b"), QLatin1String("1"));

    QCOMPARE(value("rand(5)"), QLatin1String("E475: Invalid argument: 5"));

    QCOMPARE(value("exists('*srand')"), QLatin1String("1"));
    QCOMPARE(value("exists('*rand')"), QLatin1String("1"));
}

void FakeVimTester::test_vim_script_cursorcharpos()
{
    // getcursorcharpos()/setcursorcharpos() are to the char-indexed family
    // what charcol() was to col(): this handler's getcurpos()/cursor() are
    // already character-indexed, so both alias that existing code. Values
    // taken from Vim 9.1.
    TestData data;
    setup(&data);
    QString message;
    data.handler->commandBufferChanged.set(
        [&](const QString &msg, int, int, int) {
            if (!msg.isEmpty() && !msg.startsWith("--"))
                message = msg;
        });
    const auto value = [&](const QString &expr) {
        message.clear();
        data.doCommand("echo " + expr);
        return message;
    };

    data.setText(X "one" N "two" N "three" N "four" N "five");
    data.doKeys("2Gl");
    QCOMPARE(value("string(getcursorcharpos())"), QLatin1String("[0, 2, 2, 0, 2]"));
    QCOMPARE(value("string(getcurpos())"), QLatin1String("[0, 2, 2, 0, 2]"));

    QCOMPARE(value("setcursorcharpos(3, 1)"), QLatin1String("0"));
    QCOMPARE(value("string(getcursorcharpos())"), QLatin1String("[0, 3, 1, 0, 1]"));

    QCOMPARE(value("setcursorcharpos([4, 2])"), QLatin1String("0"));
    QCOMPARE(value("string(getcursorcharpos())"), QLatin1String("[0, 4, 2, 0, 2]"));

    QCOMPARE(value("string(getcursorcharpos(win_getid()))"),
             QLatin1String("[0, 4, 2, 0, 2]"));

    QCOMPARE(value("setcursorcharpos(0, 0)"), QLatin1String("0"));
    QCOMPARE(value("string(getcursorcharpos())"), QLatin1String("[0, 4, 1, 0, 1]"));

    QCOMPARE(value("exists('*getcursorcharpos')"), QLatin1String("1"));
    QCOMPARE(value("exists('*setcursorcharpos')"), QLatin1String("1"));
}

void FakeVimTester::test_vim_script_marklist_jumplist_changelist()
{
    // getmarklist([{buf}])/getjumplist()/getchangelist() read the same state
    // ":marks"/":jumps"/"g;" already maintain. getmarklist()'s own order is
    // NOT ":marks"'s order (measured directly: letters first, then "'",
    // """, "[", "]", "^", "." in that fixed sequence). Dict key order is an
    // engine detail (QMap sorts them), so only lists are compared as exact
    // strings; dicts are checked by key set and by individual lookup, the
    // same way the window-functions test already does. Values taken from
    // Vim 9.1, using a recipe that avoids the already-parked jumpListUndo/
    // CTRL-O mismatch (a duplicate push a second time round).
    TestData data;
    setup(&data);
    QString message;
    data.handler->commandBufferChanged.set(
        [&](const QString &msg, int, int, int) {
            if (!msg.isEmpty() && !msg.startsWith("--"))
                message = msg;
        });
    const auto value = [&](const QString &expr) {
        message.clear();
        data.doCommand("echo " + expr);
        return message;
    };

    data.setText(X "one" N "two" N "three" N "four" N "five");
    data.doKeys("ggma3Gmb4G2G5G1GjxAZ<Esc>");

    // -- getmarklist({buf}) --
    // bufferNumber() hands out numbers as they are asked for and is shared
    // across the whole test run, so it must be read live here, not assumed
    // to be 1 (the same trap the bufexists() test already learned from).
    const QString bufNr = value("bufnr('%')");
    QCOMPARE(value("getmarklist()"), QLatin1String("[]"));
    QCOMPARE(value("len(getmarklist(bufnr('')))"), QLatin1String("8"));
    const QString ml = "getmarklist(bufnr(''))";
    QCOMPARE(value("sort(keys(" + ml + "[0]))"), QLatin1String("['mark', 'pos']"));
    struct { int index; QString mark; QString pos; } marks[] = {
        {0, "'a", "1, 1, 0"}, {1, "'b", "3, 1, 0"},
        {2, "''", "5, 1, 0"}, {3, "'\"", "1, 1, 0"},
        // "[" and "]" only move via the g@ operatorfunc mechanism here
        // (callOperatorFunc()), never for a plain edit like "x" - so they
        // stay at the file's own defaults; real Vim moves them to
        // [bufnr, 2, 3, 0] and [bufnr, 2, 4, 0] here, a real, pre-existing
        // gap outside this ticket.
        {4, "'[", "1, 1, 0"}, {5, "']", "5, 1, 0"},
        {6, "'^", "2, 4, 0"}, {7, "'.", "2, 3, 0"},
    };
    for (const auto &m : marks) {
        QCOMPARE(value(QString("%1[%2]['mark']").arg(ml).arg(m.index)), m.mark);
        QCOMPARE(value(QString("string(%1[%2]['pos'])").arg(ml).arg(m.index)),
                 "[" + bufNr + ", " + m.pos + "]");
    }

    // -- getjumplist() --
    QCOMPARE(value("string(getjumplist()[1])"), QLatin1String("5"));
    QCOMPARE(value("len(getjumplist()[0])"), QLatin1String("5"));
    QCOMPARE(value("sort(keys(getjumplist()[0][0]))"),
             QLatin1String("['bufnr', 'col', 'coladd', 'lnum']"));
    const int jumpLines[] = {1, 3, 4, 2, 5};
    for (int i = 0; i < 5; ++i) {
        QCOMPARE(value(QString("getjumplist()[0][%1]['lnum']").arg(i)),
                 QString::number(jumpLines[i]));
        QCOMPARE(value(QString("getjumplist()[0][%1]['col']").arg(i)),
                 QLatin1String("0"));
    }

    // -- getchangelist() --
    QCOMPARE(value("string(getchangelist()[1])"), QLatin1String("1"));
    QCOMPARE(value("len(getchangelist()[0])"), QLatin1String("1"));
    QCOMPARE(value("sort(keys(getchangelist()[0][0]))"),
             QLatin1String("['col', 'coladd', 'lnum']"));
    QCOMPARE(value("getchangelist()[0][0]['lnum']"), QLatin1String("2"));
    QCOMPARE(value("getchangelist()[0][0]['col']"), QLatin1String("2"));

    for (const QString &fn : {QString("getmarklist"), QString("getjumplist"),
                              QString("getchangelist")}) {
        QCOMPARE(value("exists('*" + fn + "')"), QLatin1String("1"));
    }
}

void FakeVimTester::test_vim_script_changenr_reg_recording_executing()
{
    // changenr() is where undo/redo already puts the cursor in the undo
    // sequence (availableUndoSteps()): 0 on a fresh buffer, up by one per
    // change, down again on "u". reg_recording()/reg_executing() answer ''
    // until something is actually happening; what they answer DURING a "qa"
    // recording and an "@a" replay is the whole point of the two, and a
    // script cannot read a value mid-keystroke, so a mapping reports it back
    // through a variable instead - measured this way against Vim 9.1.
    TestData data;
    setup(&data);
    QString message;
    data.handler->commandBufferChanged.set(
        [&](const QString &msg, int, int, int) {
            if (!msg.isEmpty() && !msg.startsWith("--"))
                message = msg;
        });
    const auto value = [&](const QString &expr) {
        message.clear();
        data.doCommand("echo " + expr);
        return message;
    };

    // -- changenr() --
    data.setText(X "one" N "two");
    QCOMPARE(value("changenr()"), QLatin1String("0"));
    data.doKeys("x");
    QCOMPARE(value("changenr()"), QLatin1String("1"));
    data.doKeys("x");
    QCOMPARE(value("changenr()"), QLatin1String("2"));
    data.doKeys("u");
    QCOMPARE(value("changenr()"), QLatin1String("1"));
    data.doKeys("u");
    QCOMPARE(value("changenr()"), QLatin1String("0"));

    // -- reg_recording() --
    QCOMPARE(value("reg_recording()"), QString());
    data.doKeys("qa");
    QCOMPARE(value("reg_recording()"), QLatin1String("a"));
    data.doKeys("q");
    QCOMPARE(value("reg_recording()"), QString());

    // -- reg_executing() --
    // "X" ends up as the sole content of register a; its mapping reports
    // reg_executing() through g:probe, since the mapping cannot answer that
    // question about itself synchronously any other way. Defined via
    // ":source", not separate doCommand() calls: a mapping whose RHS is
    // ":call ...<CR>" does not fire on keypress when built up one doCommand()
    // at a time here - an unrelated, pre-existing harness quirk, sidestepped
    // rather than chased since ":source" is what the rest of the suite
    // already uses for this exact pattern.
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    QFile f(dir.path() + "/probe.vim");
    QVERIFY(f.open(QIODevice::WriteOnly));
    f.write("let g:probe = 'unset'\n"
            "function! Probe()\n"
            "  let g:probe = reg_executing()\n"
            "endfunction\n"
            "nnoremap X :call Probe()<CR>\n");
    f.close();
    data.doCommand("source " + dir.path() + "/probe.vim");

    QCOMPARE(value("reg_executing()"), QString());
    data.doKeys("X");
    QCOMPARE(value("g:probe"), QString()); // a plain keypress is not a replay
    data.doCommand("let @a = 'X'");
    data.doKeys("@a");
    QCOMPARE(value("g:probe"), QLatin1String("a"));
    QCOMPARE(value("reg_executing()"), QString());
}

void FakeVimTester::test_vim_script_strutf16len_utf16idx()
{
    // strutf16len({string} [, {countcc}]) is just size() here: the string is
    // already stored in UTF-16, and neither string below has a composing
    // character (countcc, like elsewhere in this engine, is never
    // distinguished - every codepoint is always counted on its own).
    // utf16idx({string}, {idx} [, {countcc} [, {charidx}]]) answers the
    // UTF-16 index of byte (or, with {charidx}, character) {idx}, rounding
    // an {idx} in the middle of a multi-byte/surrogate sequence down to
    // where it starts. Values taken from Vim 9.1.
    TestData data;
    setup(&data);
    QString message;
    data.handler->commandBufferChanged.set(
        [&](const QString &msg, int, int, int) {
            if (!msg.isEmpty() && !msg.startsWith("--"))
                message = msg;
        });
    const auto value = [&](const QString &expr) {
        message.clear();
        data.doCommand("echo " + expr);
        return message;
    };

    // "a", U+00E4, "b" - 4 bytes, 3 chars, 3 UTF-16 units.
    const QString mb = QString("a") + QChar(0x00E4) + QString("b");
    // "a", U+1F600 (a non-BMP character, a surrogate pair), "b" - 6 bytes,
    // 3 chars, 4 UTF-16 units (the emoji is two).
    const char32_t emojiCodepoint = 0x1F600;
    const QString nb = QString("a") + QString::fromUcs4(&emojiCodepoint, 1) + QString("b");
    data.doCommand("let g:mb = '" + mb + "'");
    data.doCommand("let g:nb = '" + nb + "'");

    QCOMPARE(value("strutf16len(g:mb)"), QLatin1String("3"));
    QCOMPARE(value("strutf16len(g:mb, 1)"), QLatin1String("3"));
    QCOMPARE(value("strutf16len(g:nb)"), QLatin1String("4"));
    QCOMPARE(value("strutf16len(g:nb, 1)"), QLatin1String("4"));

    // {idx} defaults to a BYTE index.
    const int mbByte[] = {0, 1, 1, 2, 3, -1};
    for (int i = 0; i < 6; ++i)
        QCOMPARE(value(QString("utf16idx(g:mb, %1)").arg(i)), QString::number(mbByte[i]));
    const int nbByte[] = {0, 1, 1, 1, 1, 3, 4, -1};
    for (int i = 0; i < 8; ++i)
        QCOMPARE(value(QString("utf16idx(g:nb, %1)").arg(i)), QString::number(nbByte[i]));

    // With {charidx} present and true, {idx} is a CHARACTER index instead.
    const int mbChar[] = {0, 1, 2, 3, -1};
    for (int i = 0; i < 5; ++i) {
        QCOMPARE(value(QString("utf16idx(g:mb, %1, 0, 1)").arg(i)),
                 QString::number(mbChar[i]));
    }
    const int nbChar[] = {0, 1, 3, 4, -1};
    for (int i = 0; i < 5; ++i) {
        QCOMPARE(value(QString("utf16idx(g:nb, %1, 0, 1)").arg(i)),
                 QString::number(nbChar[i]));
    }

    for (const QString &fn : {QString("strutf16len"), QString("utf16idx")}) {
        QCOMPARE(value("exists('*" + fn + "')"), QLatin1String("1"));
    }
}

void FakeVimTester::test_vim_script_glob2regpat_pathshorten_isabsolutepath()
{
    // glob2regpat() is anchored at both ends, except that a leading or
    // trailing run of "*" is dropped together with the anchor it would have
    // needed - and a pattern of "*" alone (nothing else) answers ".*", not
    // "". pathshorten() cuts every directory component down to its first
    // {len} characters (default 1) except the last, which stays whole; a
    // leading dot in a component counts as part of that first character,
    // not against {len}. isabsolutepath() treats a leading "/" or "~" as
    // absolute. Values taken from Vim 9.1.
    TestData data;
    setup(&data);
    QString message;
    data.handler->commandBufferChanged.set(
        [&](const QString &msg, int, int, int) {
            if (!msg.isEmpty() && !msg.startsWith("--"))
                message = msg;
        });
    const auto value = [&](const QString &expr) {
        message.clear();
        data.doCommand("echo " + expr);
        return message;
    };

    QCOMPARE(value("glob2regpat('*.c')"), QLatin1String("\\.c$"));
    QCOMPARE(value("glob2regpat('foo?bar')"), QLatin1String("^foo.bar$"));
    QCOMPARE(value("glob2regpat('a*b?c')"), QLatin1String("^a.*b.c$"));
    QCOMPARE(value("glob2regpat('')"), QLatin1String("^$"));
    QCOMPARE(value("glob2regpat('/usr/**/x.h')"), QLatin1String("^/usr/.*/x\\.h$"));
    QCOMPARE(value("glob2regpat('foo*')"), QLatin1String("^foo"));
    QCOMPARE(value("glob2regpat('*foo*')"), QLatin1String("foo"));
    QCOMPARE(value("glob2regpat('foo**')"), QLatin1String("^foo"));
    QCOMPARE(value("glob2regpat('[abc].txt')"), QLatin1String("^[abc]\\.txt$"));
    QCOMPARE(value("glob2regpat('a\\b')"), QLatin1String("^a\\b$"));
    QCOMPARE(value("glob2regpat('a$b')"), QLatin1String("^a$b$"));
    QCOMPARE(value("glob2regpat('a^b')"), QLatin1String("^a^b$"));
    QCOMPARE(value("glob2regpat('a~b')"), QLatin1String("^a\\~b$"));
    QCOMPARE(value("glob2regpat('*')"), QLatin1String(".*"));
    QCOMPARE(value("glob2regpat('**')"), QLatin1String(".*"));
    QCOMPARE(value("glob2regpat('**foo')"), QLatin1String("foo$"));
    QCOMPARE(value("glob2regpat('a**b')"), QLatin1String("^a.*b$"));
    QCOMPARE(value("glob2regpat('?ab?')"), QLatin1String("^.ab.$"));

    QCOMPARE(value("pathshorten('/usr/local/include/x.h')"),
             QLatin1String("/u/l/i/x.h"));
    QCOMPARE(value("pathshorten('~/.vim/plugin/x.vim')"), QLatin1String("~/.v/p/x.vim"));
    QCOMPARE(value("pathshorten('/a/bb/ccc/d.txt', 2)"), QLatin1String("/a/bb/cc/d.txt"));
    QCOMPARE(value("pathshorten('abc')"), QLatin1String("abc"));

    QCOMPARE(value("isabsolutepath('/tmp/x')"), QLatin1String("1"));
    QCOMPARE(value("isabsolutepath('x')"), QLatin1String("0"));
    QCOMPARE(value("isabsolutepath('')"), QLatin1String("0"));
    QCOMPARE(value("isabsolutepath('~/x')"), QLatin1String("1"));

    for (const QString &fn : {QString("glob2regpat"), QString("pathshorten"),
                              QString("isabsolutepath")}) {
        QCOMPARE(value("exists('*" + fn + "')"), QLatin1String("1"));
    }
}

void FakeVimTester::test_vim_script_sha256_uri_encode_decode()
{
    // sha256() is lower-case hex, exactly what QCryptographicHash::Sha256
    // gives. uri_encode() leaves alphanumerics and "-_.~" alone and
    // percent-encodes everything else in UPPERCASE hex - including an
    // existing "%", despite what the docs claim (measured directly:
    // "%20" encodes to "%2520", not staying "%20"). uri_decode() reverses
    // "%HH"; a "%" not followed by two hex digits is left as-is, and
    // decoded bytes are combined as UTF-8. Values taken from Vim 9.1.
    TestData data;
    setup(&data);
    QString message;
    data.handler->commandBufferChanged.set(
        [&](const QString &msg, int, int, int) {
            if (!msg.isEmpty() && !msg.startsWith("--"))
                message = msg;
        });
    const auto value = [&](const QString &expr) {
        message.clear();
        data.doCommand("echo " + expr);
        return message;
    };

    QCOMPARE(value("sha256('abc')"),
             QLatin1String("ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad"));
    QCOMPARE(value("sha256('')"),
             QLatin1String("e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"));

    QCOMPARE(value("uri_encode('a b/c?d')"), QLatin1String("a%20b%2Fc%3Fd"));
    QCOMPARE(value("uri_encode('%20')"), QLatin1String("%2520"));
    QCOMPARE(value("uri_encode('100%')"), QLatin1String("100%25"));
    QCOMPARE(value("uri_encode('')"), QString());

    QCOMPARE(value("uri_decode('a%20b')"), QLatin1String("a b"));
    QCOMPARE(value("uri_decode('%GZ')"), QLatin1String("%GZ"));
    QCOMPARE(value("uri_decode('%3')"), QLatin1String("%3"));
    QCOMPARE(value("uri_decode('ab%')"), QLatin1String("ab%"));
    QCOMPARE(value("uri_decode('%3d')"), QLatin1String("="));
    QCOMPARE(value("uri_decode('%C3%A4')"), QString(QChar(0x00E4)));
    QCOMPARE(value("uri_decode('')"), QString());

    for (const QString &fn : {QString("sha256"), QString("uri_encode"),
                              QString("uri_decode")}) {
        QCOMPARE(value("exists('*" + fn + "')"), QLatin1String("1"));
    }
}

void FakeVimTester::test_vim_script_matchstrlist_matchbufline()
{
    // matchstrlist({list}, {pat} [, {dict}]) is every match of {pat} in
    // every string of {list}, one dict per match - NOT one per string, which
    // is not obvious from the plain-vanilla recipe measured for this ticket
    // and had to be checked with a string holding two matches.
    // matchbufline({buf}, {pat}, {lnum}, {end} [, {dict}]) is the same walk
    // over this buffer's own lines, "lnum" standing in for "idx". Dict key
    // order is an engine detail (QMap sorts them), so dicts are checked by
    // key set and by individual lookup rather than a full string() compare.
    // Values taken from Vim 9.1.
    TestData data;
    setup(&data);
    QString message;
    data.handler->commandBufferChanged.set(
        [&](const QString &msg, int, int, int) {
            if (!msg.isEmpty() && !msg.startsWith("--"))
                message = msg;
        });
    const auto value = [&](const QString &expr) {
        message.clear();
        data.doCommand("echo " + expr);
        return message;
    };

    QCOMPARE(value("matchstrlist([], 'x')"), QLatin1String("[]"));
    QCOMPARE(value("matchstrlist(['a'], 'zz')"), QLatin1String("[]"));

    QCOMPARE(value("len(matchstrlist(['abc', 'xbz'], 'b.'))"), QLatin1String("2"));
    QCOMPARE(value("sort(keys(matchstrlist(['abc', 'xbz'], 'b.')[0]))"),
             QLatin1String("['byteidx', 'idx', 'text']"));
    struct { int index; int byteidx; int idx; QString text; } matches[] = {
        {0, 1, 0, "bc"}, {1, 1, 1, "bz"},
    };
    const QString ml = "matchstrlist(['abc', 'xbz'], 'b.')";
    for (const auto &m : matches) {
        QCOMPARE(value(QString("%1[%2]['byteidx']").arg(ml).arg(m.index)),
                 QString::number(m.byteidx));
        QCOMPARE(value(QString("%1[%2]['idx']").arg(ml).arg(m.index)),
                 QString::number(m.idx));
        QCOMPARE(value(QString("%1[%2]['text']").arg(ml).arg(m.index)), m.text);
    }

    // {'submatches': v:true} adds a NINE-slot list, padded with '' past what
    // the pattern actually captured.
    const QString sub = "matchstrlist(['ab12'], '\\(\\a\\+\\)\\(\\d\\+\\)', "
                         "{'submatches': v:true})";
    QCOMPARE(value("sort(keys(" + sub + "[0]))"),
             QLatin1String("['byteidx', 'idx', 'submatches', 'text']"));
    QCOMPARE(value("string(" + sub + "[0]['submatches'])"),
             QLatin1String("['ab', '12', '', '', '', '', '', '', '']"));

    // -- matchbufline() --
    data.setText(X "one" N "two" N "three" N "four" N "five");
    QCOMPARE(value("len(matchbufline('%', 'o', 1, '$'))"), QLatin1String("3"));
    struct { int index; int lnum; int byteidx; } bufMatches[] = {
        {0, 1, 0}, {1, 2, 2}, {2, 4, 1},
    };
    const QString bl = "matchbufline('%', 'o', 1, '$')";
    for (const auto &m : bufMatches) {
        QCOMPARE(value(QString("%1[%2]['lnum']").arg(bl).arg(m.index)),
                 QString::number(m.lnum));
        QCOMPARE(value(QString("%1[%2]['byteidx']").arg(bl).arg(m.index)),
                 QString::number(m.byteidx));
        QCOMPARE(value(QString("%1[%2]['text']").arg(bl).arg(m.index)), QLatin1String("o"));
    }

    // A line with TWO matches yields two entries, not one.
    data.setText(X "xoxox" N "nope");
    QCOMPARE(value("len(matchbufline('%', 'o', 1, '$'))"), QLatin1String("3"));
    struct { int index; int lnum; int byteidx; } twoPerLine[] = {
        {0, 1, 1}, {1, 1, 3}, {2, 2, 1},
    };
    for (const auto &m : twoPerLine) {
        QCOMPARE(value(QString("%1[%2]['lnum']").arg(bl).arg(m.index)),
                 QString::number(m.lnum));
        QCOMPARE(value(QString("%1[%2]['byteidx']").arg(bl).arg(m.index)),
                 QString::number(m.byteidx));
    }

    for (const QString &fn : {QString("matchstrlist"), QString("matchbufline")}) {
        QCOMPARE(value("exists('*" + fn + "')"), QLatin1String("1"));
    }
}

void FakeVimTester::test_vim_script_tabpage_functions()
{
    // One tab, one window, so these are all constants, the same way the
    // plain window functions already are. Values taken from Vim 9.1.
    TestData data;
    setup(&data);
    QString message;
    data.handler->commandBufferChanged.set(
        [&](const QString &msg, int, int, int) {
            if (!msg.isEmpty() && !msg.startsWith("--"))
                message = msg;
        });
    const auto value = [&](const QString &expr) {
        message.clear();
        data.doCommand("echo " + expr);
        return message;
    };

    QCOMPARE(value("tabpagenr()"), QLatin1String("1"));
    QCOMPARE(value("tabpagenr('$')"), QLatin1String("1"));
    QCOMPARE(value("tabpagewinnr(1)"), QLatin1String("1"));

    // bufnr('%') is queried live: the global buffer-number counter this
    // engine hands out is shared across the whole test run.
    const QString bufNr = value("bufnr('%')");
    QCOMPARE(value("string(tabpagebuflist())"), "[" + bufNr + "]");
    QCOMPARE(value("winbufnr(0)"), bufNr);
    QCOMPARE(value("winbufnr(1)"), bufNr);
    QCOMPARE(value("winbufnr(2)"), QLatin1String("-1"));
    QCOMPARE(value("winnr('$')"), QLatin1String("1"));
    QCOMPARE(value("bufwinid('%')"), QLatin1String("1000"));
    QCOMPARE(value("bufwinid('nope.txt')"), QLatin1String("-1"));

    for (const QString &fn : {QString("tabpagenr"), QString("tabpagewinnr"),
                              QString("tabpagebuflist"), QString("winbufnr"),
                              QString("bufwinid")}) {
        QCOMPARE(value("exists('*" + fn + "')"), QLatin1String("1"));
    }
}

void FakeVimTester::test_vim_script_window_id_functions()
{
    // The window-id family, for the one window this handler drives: it is
    // window 1 of tab page 1 with the id win_getid() already answers, and an
    // id or number naming any other window is reported as absent rather than
    // guessed at. Values taken from Vim 9.1.
    TestData data;
    setup(&data);
    QString message;
    data.handler->commandBufferChanged.set(
        [&](const QString &msg, int, int, int) {
            if (!msg.isEmpty() && !msg.startsWith("--"))
                message = msg;
        });
    const auto value = [&](const QString &expr) {
        message.clear();
        data.doCommand("echo " + expr);
        return message;
    };
    data.setText(X "one" N "two" N "three");

    QCOMPARE(value("win_gettype()"), QString());
    QCOMPARE(value("win_gettype(1)"), QString());

    // The id is win_getid()'s own, read live rather than written down.
    const QString id = value("win_getid()");
    QCOMPARE(value("win_id2win(" + id + ")"), QLatin1String("1"));
    QCOMPARE(value("win_id2win(999)"), QLatin1String("0"));
    QCOMPARE(value("win_gotoid(" + id + ")"), QLatin1String("1"));
    QCOMPARE(value("win_gotoid(999)"), QLatin1String("0"));
    QCOMPARE(value("string(win_id2tabwin(" + id + "))"), QLatin1String("[1, 1]"));
    QCOMPARE(value("string(win_id2tabwin(999))"), QLatin1String("[0, 0]"));
    QCOMPARE(value("string(winlayout())"), "['leaf', " + id + "]");
    QCOMPARE(value("string(winlayout(1))"), "['leaf', " + id + "]");
    QCOMPARE(value("string(winlayout(9))"), QLatin1String("[]"));
    QCOMPARE(value("string(win_screenpos(1))"), QLatin1String("[1, 1]"));
    QCOMPARE(value("string(win_screenpos(0))"), QLatin1String("[1, 1]"));
    QCOMPARE(value("string(win_screenpos(9))"), QLatin1String("[0, 0]"));

    // bufferNumber() hands numbers out as they are asked for and the counter is
    // shared across the whole test run, so this one is read live too.
    const QString bufNr = value("bufnr('%')");
    QCOMPARE(value("string(win_findbuf(" + bufNr + "))"), "[" + id + "]");
    QCOMPARE(value("string(win_findbuf(999))"), QLatin1String("[]"));

    // winrestcmd() writes the whole sequence twice over. The size is the
    // widget's, so the expectation is computed from the same two functions
    // rather than from a terminal's numbers.
    const QString once = ":1resize " + value("winheight(0)")
                         + "|vert :1resize " + value("winwidth(0)") + "|";
    QCOMPARE(value("winrestcmd()"), once + once);

    // win_execute() is execute() with the window to run in named first, and
    // answers nothing at all for a window that is not there.
    QCOMPARE(value("string(win_execute(" + id + ", 'echo \"a\"'))"),
             QLatin1String("'\na'"));
    QCOMPARE(value("string(win_execute(" + id + ", ['echo 1', 'echo 2']))"),
             QLatin1String("'\n1\n2'"));
    QCOMPARE(value("string(win_execute(999, 'echo \"a\"'))"), QLatin1String("''"));
    // It really does run them, not just collect what they would have said.
    data.doCommand("call win_execute(" + id + ", 'call setline(1, \"CHANGED\")')");
    QCOMPARE(value("getline(1)"), QLatin1String("CHANGED"));

    for (const QString &fn : {QString("win_gettype"), QString("win_id2win"),
                              QString("win_id2tabwin"), QString("win_findbuf"),
                              QString("win_gotoid"), QString("winlayout"),
                              QString("winrestcmd"), QString("win_screenpos"),
                              QString("win_execute")}) {
        QCOMPARE(value("exists('*" + fn + "')"), QLatin1String("1"));
    }
}

void FakeVimTester::test_vim_script_command_line_functions()
{
    // These say something only while the command line is being EDITED. By the
    // time a ":" command runs the line is finished with, so the plain calls
    // below all answer empty or zero - and the setters answer ONE, which is
    // how Vim spells "could not". Reaching the editing case needs a mapping's
    // expression, the same hook Vim documents; a "cnoremap <expr>" fires while
    // the line is still there and reports through a variable.
    // Values taken from Vim 9.1.
    TestData data;
    setup(&data);
    QString message;
    data.handler->commandBufferChanged.set(
        [&](const QString &msg, int, int, int) {
            if (!msg.isEmpty() && !msg.startsWith("--"))
                message = msg;
        });
    const auto value = [&](const QString &expr) {
        message.clear();
        data.doCommand("echo " + expr);
        return message;
    };
    data.setText(X "one" N "two");

    // -- nothing being edited --
    QCOMPARE(value("getcmdtype()"), QString());
    QCOMPARE(value("getcmdline()"), QString());
    QCOMPARE(value("getcmdpos()"), QLatin1String("0"));
    QCOMPARE(value("getcmdscreenpos()"), QLatin1String("0"));
    QCOMPARE(value("setcmdline('x')"), QLatin1String("1"));
    QCOMPARE(value("setcmdpos(1)"), QLatin1String("1"));
    // Neither of these is ever anything else here: there is no command-line
    // window, and no input() to put a prompt of its own up.
    QCOMPARE(value("getcmdwintype()"), QString());
    QCOMPARE(value("getcmdprompt()"), QString());

    // -- while it IS being edited --
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    QFile f(dir.path() + "/cl.vim");
    QVERIFY(f.open(QIODevice::WriteOnly));
    f.write("let g:probe = ''\n"
            "function! Probe()\n"
            "  let g:probe = getcmdtype() . '|' . getcmdline() . '|'\n"
            "        \\ . getcmdpos() . '|' . getcmdscreenpos()\n"
            "  return ''\n"
            "endfunction\n"
            "cnoremap <expr> <C-g> Probe()\n");
    f.close();
    data.doCommand("source " + dir.path() + "/cl.vim");

    // ":abc" with the cursor at the end: the text is the line without its
    // prompt, the position counts from one, and the screen position is one
    // further along because the prompt takes a column.
    data.doKeys(":abc<C-g><Esc>");
    QCOMPARE(value("g:probe"), QLatin1String(":|abc|4|5"));
    // One step left moves only the position.
    data.doKeys(":abc<Left><C-g><Esc>");
    QCOMPARE(value("g:probe"), QLatin1String(":|abc|3|4"));
    // A search line reports its own prompt character. It is a command line
    // being edited like any other, so the same "cnoremap" reaches it.
    data.doKeys("/xy<C-g><Esc>");
    QCOMPARE(value("g:probe"), QLatin1String("/|xy|3|4"));
    data.doKeys("?xy<C-g><Esc>");
    QCOMPARE(value("g:probe"), QLatin1String("?|xy|3|4"));
    // So does the "=" expression prompt, opened from insert mode, which
    // borrows the command buffer to type the expression into.
    data.doKeys("i<C-r>=1+1<C-g>");
    QCOMPARE(value("g:probe"), QLatin1String("=|1+1|4|5"));
    data.doKeys("<Esc><Esc>");

    // -- the setters, while it is being edited --
    QFile g(dir.path() + "/cl2.vim");
    QVERIFY(g.open(QIODevice::WriteOnly));
    g.write("function! SetIt()\n"
            "  let g:ret = setcmdline('replaced')\n"
            "  let g:probe = getcmdline() . '|' . getcmdpos()\n"
            "  return ''\n"
            "endfunction\n"
            "function! SetItPos()\n"
            "  let g:ret = setcmdline('abcdef', 3)\n"
            "  let g:probe = getcmdline() . '|' . getcmdpos()\n"
            "  return ''\n"
            "endfunction\n"
            "function! MovePos()\n"
            "  let g:ret = setcmdpos(2)\n"
            "  let g:probe = getcmdline() . '|' . getcmdpos()\n"
            "  return ''\n"
            "endfunction\n"
            "cnoremap <expr> <C-y> SetIt()\n"
            "cnoremap <expr> <C-o> SetItPos()\n"
            "cnoremap <expr> <C-b> MovePos()\n");
    g.close();
    data.doCommand("source " + dir.path() + "/cl2.vim");

    // setcmdline({str}) puts the cursor at the end; a zero says it worked.
    data.doKeys(":zz<C-y><Esc>");
    QCOMPARE(value("g:ret"), QLatin1String("0"));
    QCOMPARE(value("g:probe"), QLatin1String("replaced|9"));
    // With a position, the cursor goes there instead.
    data.doKeys(":zz<C-o><Esc>");
    QCOMPARE(value("g:probe"), QLatin1String("abcdef|3"));
    // setcmdpos() moves the cursor and leaves the text alone.
    data.doKeys(":abcdef<C-b><Esc>");
    QCOMPARE(value("g:probe"), QLatin1String("abcdef|2"));

    // The other sub-sub-modes are each waiting for ONE character to be taken
    // literally, and a mapping must not get in front of it: an "f" whose
    // target happens to be a mapped key still searches for that character.
    // Vim takes it literally too, so this guards the line-editing exception
    // above from being widened to every sub-sub-mode.
    data.doCommand("nnoremap z :call setline(1, 'MAPPED')<CR>");
    data.setText(X "az bz");
    data.doKeys("fz");
    QCOMPARE(data.text(), QByteArray("az bz")); // not "MAPPED"
    QCOMPARE(value("col('.')"), QLatin1String("2"));
    data.doCommand("nunmap z");

    for (const QString &fn : {QString("getcmdline"), QString("getcmdpos"),
                              QString("getcmdtype"), QString("getcmdscreenpos"),
                              QString("getcmdprompt"), QString("getcmdwintype"),
                              QString("setcmdline"), QString("setcmdpos")}) {
        QCOMPARE(value("exists('*" + fn + "')"), QLatin1String("1"));
    }
}

void FakeVimTester::test_vim_script_getbufoneline_wordcount()
{
    // getbufoneline({buf}, {lnum}) is always a single line as a plain
    // string, never a list - "buf" can only be this one, like getbufline().
    // wordcount()'s cursor_* entries count up to and INCLUDING the cursor
    // (not just before it, despite what :help says); "word" there just
    // means a maximal run of non-whitespace, not a Vim keyword-boundary
    // word - measured directly ("a,b c" and "foo-bar" are ONE word each).
    // Every line, including the last, counts as newline-terminated, the
    // same convention line2byte() already uses. Values taken from Vim 9.1.
    TestData data;
    setup(&data);
    QString message;
    data.handler->commandBufferChanged.set(
        [&](const QString &msg, int, int, int) {
            if (!msg.isEmpty() && !msg.startsWith("--"))
                message = msg;
        });
    const auto value = [&](const QString &expr) {
        message.clear();
        data.doCommand("echo " + expr);
        return message;
    };

    // Cursor on the "o" of "two" (0-based column 2).
    data.setText("one" N "tw" X "o" N "three" N "four" N "five");
    QCOMPARE(value("getbufoneline('%', 3)"), QLatin1String("three"));
    QCOMPARE(value("getbufoneline('%', 0)"), QString());
    QCOMPARE(value("getbufoneline('%', 6)"), QString());
    QCOMPARE(value("getbufoneline('nope.txt', 1)"), QString());
    QCOMPARE(value("getbufoneline(999, 1)"), QString());

    QCOMPARE(value("wordcount()['chars']"), QLatin1String("24"));
    QCOMPARE(value("wordcount()['bytes']"), QLatin1String("24"));
    QCOMPARE(value("wordcount()['words']"), QLatin1String("5"));
    QCOMPARE(value("wordcount()['cursor_chars']"), QLatin1String("7"));
    QCOMPARE(value("wordcount()['cursor_bytes']"), QLatin1String("7"));
    QCOMPARE(value("wordcount()['cursor_words']"), QLatin1String("2"));

    // A multibyte line, to tell "chars" and "bytes" apart: "a-umlaut" twice
    // on its own line, cursor on its first one. setText() takes a plain
    // 7-bit string, so the placeholder "x" is typed over in insert mode.
    data.setText("abc" N "x" N "xyz");
    data.doKeys("2Gcl" + QString::fromUtf8("\xc3\xa4\xc3\xa4") + "\x1b" + "0");
    QCOMPARE(value("wordcount()['chars']"), QLatin1String("11"));
    QCOMPARE(value("wordcount()['bytes']"), QLatin1String("13"));
    QCOMPARE(value("wordcount()['words']"), QLatin1String("3"));
    QCOMPARE(value("wordcount()['cursor_chars']"), QLatin1String("5"));
    QCOMPARE(value("wordcount()['cursor_bytes']"), QLatin1String("6"));
    QCOMPARE(value("wordcount()['cursor_words']"), QLatin1String("2"));

    // "Word" is whitespace-delimited, not Vim's keyword-boundary word:
    // punctuation glued to a letter does not split it.
    data.setText(X "a,b c" N "foo-bar" N "" N "  spaced  out  ");
    QCOMPARE(value("wordcount()['words']"), QLatin1String("5"));

    for (const QString &fn : {QString("getbufoneline"), QString("wordcount")}) {
        QCOMPARE(value("exists('*" + fn + "')"), QLatin1String("1"));
    }
}

void FakeVimTester::test_vim_script_foreach()
{
    // foreach({expr1}, {expr2}) runs {expr2} for every item but IGNORES its
    // return value and hands back {expr1} completely unchanged - the SAME
    // object, not a copy. A version that behaved like map() would still
    // pass a check of the answer alone, so both the answer and the
    // original's own untouched values are asserted, and identity via "is",
    // the same trap extendnew() already guards against. Values taken from
    // Vim 9.1.
    TestData data;
    setup(&data);
    QString message;
    data.handler->commandBufferChanged.set(
        [&](const QString &msg, int, int, int) {
            if (!msg.isEmpty() && !msg.startsWith("--"))
                message = msg;
        });
    const auto value = [&](const QString &expr) {
        message.clear();
        data.doCommand("echo " + expr);
        return message;
    };

    data.doCommand("let g:l = [1, 2, 3]");
    data.doCommand("let g:r = foreach(g:l, {i, v -> 99})");
    QCOMPARE(value("string(g:r)"), QLatin1String("[1, 2, 3]"));
    QCOMPARE(value("g:l is g:r"), QLatin1String("1"));

    // v:key/v:val work the same way they do for map(), for the string form.
    data.doCommand("let g:hit = []");
    data.doCommand("call foreach(['a', 'b'], 'add(g:hit, v:key .. v:val)')");
    QCOMPARE(value("string(g:hit)"), QLatin1String("['0a', '1b']"));

    data.doCommand("let g:d = {'a': 1, 'b': 2}");
    data.doCommand("let g:hit2 = []");
    data.doCommand("call foreach(g:d, {k, v -> add(g:hit2, k .. v)})");
    QCOMPARE(value("sort(g:hit2)"), QLatin1String("['a1', 'b2']"));
    QCOMPARE(value("string(g:d)"), QLatin1String("{'a': 1, 'b': 2}"));

    QCOMPARE(value("exists('*foreach')"), QLatin1String("1"));
}

void FakeVimTester::test_vim_script_combining_and_non_bmp()
{
    // What Vim calls a "character" is a codepoint - a surrogate pair is ONE -
    // and the family below additionally folds a combining mark into the
    // character it belongs to: strchars() counts marks on their own,
    // strcharlen() and strchars({skipcc}) do not; byteidx() folds,
    // byteidxcomp() does not; charidx() folds unless {countcc}. Counting
    // QString units instead, as this engine used to, gets a surrogate pair
    // wrong (two, not one) and cannot tell the two mark conventions apart at
    // all. Values taken from Vim 9.1.
    TestData data;
    setup(&data);
    QString message;
    data.handler->commandBufferChanged.set(
        [&](const QString &msg, int, int, int) {
            if (!msg.isEmpty() && !msg.startsWith("--"))
                message = msg;
        });
    const auto value = [&](const QString &expr) {
        message.clear();
        data.doCommand("echo " + expr);
        return message;
    };

    // Built from code points to keep the source 7-bit ASCII.
    const QChar acute(0x0301);    // COMBINING ACUTE ACCENT, two bytes
    const QChar diaeresis(0x0308);
    const char32_t emojiCodepoint = 0x1F600;
    const QString emoji = QString::fromUcs4(&emojiCodepoint, 1);
    // "e" + acute + "x": 4 bytes, 3 codepoints, 2 characters when folding.
    const QString cc = QString("e") + acute + QString("x");
    // "a" + emoji + "b": 6 bytes, 3 codepoints and 3 characters - the pair is
    // one character, where QString stores it in two units.
    const QString nb = QString("a") + emoji + QString("b");
    // Two marks on one base: 6 bytes, 4 codepoints, 2 characters folding.
    const QString two = QString("e") + acute + diaeresis + QString("x");
    // A mark with nothing in front of it stays a character of its own.
    const QString lead = QString(acute) + QString("a");
    data.doCommand("let g:cc = '" + cc + "'");
    data.doCommand("let g:nb = '" + nb + "'");
    data.doCommand("let g:two = '" + two + "'");
    data.doCommand("let g:lead = '" + lead + "'");

    // -- strchars()/strcharlen() --
    struct { QString var; int chars; int folded; } counts[] = {
        {"g:cc", 3, 2}, {"g:nb", 3, 3}, {"g:two", 4, 2}, {"g:lead", 2, 2},
    };
    for (const auto &c : counts) {
        QCOMPARE(value("strchars(" + c.var + ")"), QString::number(c.chars));
        QCOMPARE(value("strchars(" + c.var + ", 0)"), QString::number(c.chars));
        QCOMPARE(value("strchars(" + c.var + ", 1)"), QString::number(c.folded));
        QCOMPARE(value("strcharlen(" + c.var + ")"), QString::number(c.folded));
    }
    QCOMPARE(value("strchars('')"), QLatin1String("0"));
    QCOMPARE(value("strcharlen('')"), QLatin1String("0"));

    // -- byteidx()/byteidxcomp() --
    // An index of exactly the character count answers the whole byte length.
    const auto walk = [&](const QString &fn, const QString &var,
                          const QList<int> &expected) {
        for (int i = 0; i < expected.size(); ++i) {
            QCOMPARE(value(QString("%1(%2, %3)").arg(fn, var).arg(i)),
                     QString::number(expected.at(i)));
        }
    };
    walk("byteidx", "g:cc", {0, 3, 4, -1});
    walk("byteidxcomp", "g:cc", {0, 1, 3, 4, -1});
    walk("byteidx", "g:two", {0, 5, 6, -1});
    walk("byteidxcomp", "g:two", {0, 1, 3, 5, 6, -1});
    walk("byteidx", "g:lead", {0, 2, 3, -1});
    // No marks in this one, so the two agree - but the pair still counts once.
    walk("byteidx", "g:nb", {0, 1, 5, 6, -1});
    walk("byteidxcomp", "g:nb", {0, 1, 5, 6, -1});
    walk("byteidx", "''", {0, -1});

    // -- charidx() --
    // A byte inside a character answers that character.
    walk("charidx", "g:cc", {0, 0, 0, 1, 2, -1});
    walk("charidx", "g:two", {0, 0, 0, 0, 0, 1, 2, -1});
    walk("charidx", "g:lead", {0, 0, 1, 2, -1});
    walk("charidx", "g:nb", {0, 1, 1, 1, 1, 2, 3, -1});
    walk("charidx", "''", {0});
    const int ccCountcc[] = {0, 1, 1, 2, 3, -1};
    for (int i = 0; i < 6; ++i) {
        QCOMPARE(value(QString("charidx(g:cc, %1, 1)").arg(i)),
                 QString::number(ccCountcc[i]));
    }

    // -- strcharpart() --
    // Without {skipcc} a mark is a piece of its own; with it, it goes along
    // with the character it belongs to. A surrogate pair is never halved.
    QCOMPARE(value("strcharpart(g:cc, 0, 1)"), QLatin1String("e"));
    QCOMPARE(value("strcharpart(g:cc, 1, 1)"), QString(acute));
    QCOMPARE(value("strcharpart(g:cc, 0, 1, 1)"), QString("e") + acute);
    QCOMPARE(value("strcharpart(g:cc, 1, 1, 1)"), QLatin1String("x"));
    QCOMPARE(value("strcharpart(g:two, 0, 1, 1)"), QString("e") + acute + diaeresis);
    QCOMPARE(value("strcharpart(g:nb, 1, 1)"), emoji);
    QCOMPARE(value("strcharpart(g:nb, 2, 1)"), QLatin1String("b"));
    // A negative start is not moved to zero: it eats into the count.
    const QString word = QString("a") + QChar(0x00E9) + QString("bc");
    data.doCommand("let g:w = '" + word + "'");
    QCOMPARE(value("strcharpart(g:w, -1, 2)"), QLatin1String("a"));
    QCOMPARE(value("strcharpart(g:w, 2, 99)"), QLatin1String("bc"));
    QCOMPARE(value("strcharpart(g:w, 9, 1)"), QString());

    for (const QString &fn : {QString("strcharlen"), QString("byteidxcomp")})
        QCOMPARE(value("exists('*" + fn + "')"), QLatin1String("1"));
}

void FakeVimTester::test_vim_script_window_functions()
{
    // With exactly one window there is nothing to enumerate; these all
    // answer from that single window's own numbers. Values taken from Vim
    // 9.1 (a real 80x24 terminal, cursor on line 3 of five, no scrolling).
    TestData data;
    setup(&data);
    QString message;
    data.handler->commandBufferChanged.set(
        [&](const QString &msg, int, int, int) {
            if (!msg.isEmpty() && !msg.startsWith("--"))
                message = msg;
        });
    const auto value = [&](const QString &expr) {
        message.clear();
        data.doCommand("echo " + expr);
        return message;
    };

    QCOMPARE(value("winnr()"), QLatin1String("1"));
    QCOMPARE(value("bufwinnr('%')"), QLatin1String("1"));
    QCOMPARE(value("bufwinnr(bufnr(''))"), QLatin1String("1"));
    QCOMPARE(value("bufwinnr('nope.txt')"), QLatin1String("-1"));
    QCOMPARE(value("bufwinnr(999)"), QLatin1String("-1"));
    QCOMPARE(value("win_getid()"), QLatin1String("1000"));
    QCOMPARE(value("sort(keys(getwininfo()[0]))"),
             QLatin1String("['botline', 'bufnr', 'height', 'leftcol', 'loclist', "
                            "'quickfix', 'status_height', 'tabnr', 'terminal', "
                            "'textoff', 'topline', 'variables', 'width', 'winbar', "
                            "'wincol', 'winid', 'winnr', 'winrow']"));
    QCOMPARE(value("getwininfo()[0]['winnr']"), QLatin1String("1"));
    QCOMPARE(value("getwininfo()[0]['winid']"), QLatin1String("1000"));
    QCOMPARE(value("getwininfo()[0]['loclist']"), QLatin1String("0"));
    QCOMPARE(value("getwininfo()[0]['quickfix']"), QLatin1String("0"));
    QCOMPARE(value("getwininfo()[0]['terminal']"), QLatin1String("0"));
    QCOMPARE(value("getwininfo()[0]['tabnr']"), QLatin1String("1"));

    // Realize the editor so scrolling has a real viewport to scroll within;
    // otherwise "zt" has nothing to do and winline() just answers with the
    // cursor's block number instead of its screen row.
    data.editor()->resize(600, 400);
    data.editor()->show();

    // winline()/wincol() track the cursor's actual screen row/column, not
    // the editor's size: "zt" puts the cursor line at the top of the window.
    // The buffer must be taller than the window or there is nothing to
    // scroll and the cursor line stays wherever it already was.
    QByteArray longText;
    for (int i = 1; i <= 200; ++i)
        longText += QByteArray("line ") + QByteArray::number(i) + '\n';
    data.setText(longText.constData());
    data.doKeys("100Gzt");
    QCOMPARE(value("winline()"), QLatin1String("1"));
    QCOMPARE(value("wincol()"), QLatin1String("1"));
    data.doKeys("j");
    QCOMPARE(value("winline()"), QLatin1String("2"));

    // winheight()/winwidth() answer the window's own real geometry, whatever
    // that happens to be here - not the 80x24 the measurement above was
    // taken in, which this headless widget has no reason to match.
    const int expectedHeight = data.editor()->viewport()->height()
                                / data.editor()->cursorRect().height();
    const int expectedWidth = data.editor()->viewport()->width()
                               / data.editor()->fontMetrics().horizontalAdvance(' ');
    QCOMPARE(value("winheight(0)"), QString::number(expectedHeight));
    QCOMPARE(value("winwidth(0)"), QString::number(expectedWidth));
}

void FakeVimTester::test_vim_script_localtime_and_strptime()
{
    // localtime() is the current time as Unix seconds, and strptime() parses
    // the same C library format strftime() writes with - so a round trip
    // through both is stable even though the number itself depends on the time
    // zone this runs in. Values taken from Vim 9.1.
    TestData data;
    setup(&data);
    QString message;
    data.handler->commandBufferChanged.set(
        [&](const QString &msg, int, int, int) {
            if (!msg.isEmpty() && !msg.startsWith("--"))
                message = msg;
        });
    const auto value = [&](const QString &expr) {
        message.clear();
        data.doCommand("echo " + expr);
        return message;
    };

    QCOMPARE(value("localtime() > 1700000000"), QLatin1String("1"));
    QCOMPARE(value("type(localtime())"), QLatin1String("0"));

    // strptime() is POSIX and not every C library has one, which is why Vim
    // documents it as "Not available on all systems" and tells a script to ask
    // first. This engine says the same, so the rest of this asks the same
    // question rather than assuming an answer.
    if (value("exists('*strptime')") != "1")
        QSKIP("no strptime() on this platform, as Vim has it on such a one");

    QCOMPARE(value("type(strptime('%Y-%m-%d', '2026-08-26'))"), QLatin1String("0"));
    QCOMPARE(value("strftime('%Y-%m-%d', strptime('%Y-%m-%d', '2026-08-26'))"),
             QLatin1String("2026-08-26"));
    QCOMPARE(value("strftime('%Y-%m-%d %H:%M:%S', "
                   "strptime('%Y-%m-%d %H:%M:%S', '2026-08-26 14:30:00'))"),
             QLatin1String("2026-08-26 14:30:00"));
    // What the format leaves out defaults to midnight.
    QCOMPARE(value("strftime('%H:%M:%S', strptime('%Y-%m-%d', '2026-08-26'))"),
             QLatin1String("00:00:00"));
    // A string the format cannot make sense of answers with nothing found.
    QCOMPARE(value("strptime('%Y-%m-%d', 'not a date')"), QLatin1String("0"));
    // What the format leaves at zero - here the day of the month - can carry a
    // date back into the month or year before, as it does in Vim.
    QCOMPARE(value("strftime('%Y', strptime('%Y', '2026extra'))"), QLatin1String("2025"));
}

void FakeVimTester::test_vim_script_slice()
{
    // slice({expr}, {start} [, {end}]) is Python-style: the end is one past
    // what is taken, a missing end reaches to the end of the sequence, a
    // negative index counts back from there, and both ends clamp to the range
    // there is. Values taken from Vim 9.1.
    TestData data;
    setup(&data);
    QString message;
    data.handler->commandBufferChanged.set(
        [&](const QString &msg, int, int, int) {
            if (!msg.isEmpty() && !msg.startsWith("--"))
                message = msg;
        });
    const auto value = [&](const QString &expr) {
        message.clear();
        data.doCommand("echo " + expr);
        return message;
    };

    QCOMPARE(value("string(slice('abcde', 1, 3))"), QLatin1String("'bc'"));
    QCOMPARE(value("string(slice([1, 2, 3, 4], 1))"), QLatin1String("[2, 3, 4]"));
    QCOMPARE(value("string(slice('abcde', 1))"), QLatin1String("'bcde'"));
    QCOMPARE(value("string(slice('abcde', -2))"), QLatin1String("'de'"));
    QCOMPARE(value("string(slice('abcde', 1, -1))"), QLatin1String("'bcd'"));
    QCOMPARE(value("string(slice('abcde', 0, 0))"), QLatin1String("''"));
    QCOMPARE(value("string(slice('abcde', 10, 20))"), QLatin1String("''"));
    QCOMPARE(value("string(slice([1, 2, 3, 4], -2, -1))"), QLatin1String("[3]"));
    QCOMPARE(value("string(slice('abcde', -10, 100))"), QLatin1String("'abcde'"));
    QCOMPARE(value("string(slice('abcde', 3, 1))"), QLatin1String("''"));
    QCOMPARE(value("string(slice([], 0, 5))"), QLatin1String("[]"));
    QCOMPARE(value("string(slice('abcde', 0))"), QLatin1String("'abcde'"));
}

void FakeVimTester::test_vim_script_virtcol2col()
{
    // virtcol2col({winid}, {lnum}, {col}) says which character a screen column
    // belongs to: a tab answers for every column it reaches, a column before the
    // line clamps to its first character, one past its end to its last, and an
    // empty line or an unknown window or line answers with nothing there at all.
    // Values taken from Vim 9.1.
    TestData data;
    setup(&data);
    data.doCommand("set tabstop=8");
    QString message;
    data.handler->commandBufferChanged.set(
        [&](const QString &msg, int, int, int) {
            if (!msg.isEmpty() && !msg.startsWith("--"))
                message = msg;
        });
    const auto value = [&](const QString &expr) {
        message.clear();
        data.doCommand("echo " + expr);
        return message;
    };

    data.setText(X "alpha" N "\tone two" N "three");
    QCOMPARE(value("virtcol2col(0, 2, 1)"), QLatin1String("1"));
    QCOMPARE(value("virtcol2col(0, 2, 5)"), QLatin1String("1"));
    QCOMPARE(value("virtcol2col(0, 2, 8)"), QLatin1String("1"));
    QCOMPARE(value("virtcol2col(0, 2, 9)"), QLatin1String("2"));
    QCOMPARE(value("virtcol2col(0, 2, 10)"), QLatin1String("3"));
    // Before the start of the line, or past its end.
    QCOMPARE(value("virtcol2col(0, 2, 0)"), QLatin1String("1"));
    QCOMPARE(value("virtcol2col(0, 2, 99)"), QLatin1String("8"));
    // A window or a line this handler does not know.
    QCOMPARE(value("virtcol2col(999, 2, 9)"), QLatin1String("-1"));
    QCOMPARE(value("virtcol2col(0, 99, 9)"), QLatin1String("-1"));
    // A line with nothing on it has nowhere to point to.
    data.setText(X "one" N "" N "three");
    QCOMPARE(value("virtcol2col(0, 2, 1)"), QLatin1String("0"));
    QCOMPARE(value("virtcol2col(0, 2, 5)"), QLatin1String("0"));
}

void FakeVimTester::test_vim_script_charcol_and_charpos()
{
    // charcol(), getcharpos() and setcharpos() count characters where col(),
    // getpos() and setpos() count bytes. On an all-ASCII line the two counts
    // agree, so a multibyte line is what tells a correct implementation from one
    // that only copies col()/getpos(). Values taken from Vim 9.1.
    TestData data;
    setup(&data);
    QString message;
    data.handler->commandBufferChanged.set(
        [&](const QString &msg, int, int, int) {
            if (!msg.isEmpty() && !msg.startsWith("--"))
                message = msg;
        });
    const auto value = [&](const QString &expr) {
        message.clear();
        data.doCommand("echo " + expr);
        return message;
    };

    data.setText(X "alpha beta" N "\tone two" N "three");
    KEYS("6l", "alpha " X "beta" N "\tone two" N "three");
    QCOMPARE(value("charcol('.') .. ',' .. col('.')"), QLatin1String("7,7"));
    QCOMPARE(value("charcol('$')"), QLatin1String("11"));
    QCOMPARE(value("string(getcharpos('.'))"), QLatin1String("[0, 1, 7, 0]"));
    QCOMPARE(value("string(getpos('.'))"), QLatin1String("[0, 1, 7, 0]"));
    data.doCommand("call setcharpos('.', [0, 3, 2, 0])");
    QCOMPARE(value("line('.') .. ',' .. col('.')"), QLatin1String("3,2"));

    // A multibyte character is one character but more than one byte: in real
    // Vim, on a line holding one, charcol()/getcharpos() and col()/getpos() give
    // different numbers (measured: charcol/getcharpos 4/[0,1,4,0] against
    // col/getpos 5/[0,1,5,0] here). This engine's own col()/getpos() are not yet
    // byte-based - see QTCREATORBUG-34817 - so only what THIS ticket adds is
    // asserted against the real values; col()/getpos() are left alone. The
    // escape is FakeVim's own Vimscript string syntax, so the source stays 7-bit
    // ASCII.
    data.doCommand("call setline(1, \"a\\u00e4bc\")");
    data.doCommand("normal! gg0lll");
    QCOMPARE(value("charcol('.')"), QLatin1String("4"));
    QCOMPARE(value("charcol('$')"), QLatin1String("5"));
    QCOMPARE(value("string(getcharpos('.'))"), QLatin1String("[0, 1, 4, 0]"));
    // setcharpos() reaches the second CHARACTER, not the second byte.
    data.doCommand("call setcharpos('.', [0, 1, 2, 0])");
    QCOMPARE(value("charcol('.')"), QLatin1String("2"));
}

void FakeVimTester::test_vim_script_charclass()
{
    // charclass() says what kind of character the first one of a string is: 0
    // for whitespace, 1 for punctuation, 2 for a keyword character (as
    // 'iskeyword' says), and 0 where there is none. Values taken from Vim 9.1.
    TestData data;
    setup(&data);
    QString message;
    data.handler->commandBufferChanged.set(
        [&](const QString &msg, int, int, int) {
            if (!msg.isEmpty() && !msg.startsWith("--"))
                message = msg;
        });
    const auto value = [&](const QString &expr) {
        message.clear();
        data.doCommand("echo " + expr);
        return message;
    };

    QCOMPARE(value("charclass('a')"), QLatin1String("2"));
    QCOMPARE(value("charclass(' ')"), QLatin1String("0"));
    QCOMPARE(value("charclass('.')"), QLatin1String("1"));
    QCOMPARE(value("charclass('5')"), QLatin1String("2"));
    QCOMPARE(value("charclass('')"), QLatin1String("0"));
}

void FakeVimTester::test_vim_g8()
{
    // "g8" shows the bytes of the character under the cursor, each two lower
    // case hex digits (zero-padded), with a leading and trailing space; a
    // multibyte character shows every byte of it. Driven through ":normal",
    // which - like a real key press of "g8" - is not looked up in mappings, so
    // it does not run into whatever else in the buffer is waiting on a possible
    // longer one. Values taken from Vim 9.1.
    TestData data;
    setup(&data);
    QString message;
    data.handler->commandBufferChanged.set(
        [&](const QString &msg, int, int, int) {
            if (!msg.isEmpty() && !msg.startsWith("--"))
                message = msg;
        });

    data.setText(X "a");
    message.clear();
    data.doCommand("normal! g8");
    QCOMPARE(message, QLatin1String(" 61 "));

    // The escapes are FakeVim's own Vimscript string syntax, so the source stays
    // 7-bit ASCII: "\u00e4" is a two-byte character in UTF-8, "\u20ac" a
    // three-byte one, and "\t" a plain byte below 0x10, to make sure zero
    // padding is not lost.
    data.doCommand("call setline(1, \"a\\u00e4\\u20ac\\tZ\")");
    data.doCommand("normal! 0");
    message.clear();
    data.doCommand("normal! g8");
    QCOMPARE(message, QLatin1String(" 61 "));
    data.doCommand("normal! l");
    message.clear();
    data.doCommand("normal! g8");
    QCOMPARE(message, QLatin1String(" c3 a4 "));
    data.doCommand("normal! l");
    message.clear();
    data.doCommand("normal! g8");
    QCOMPARE(message, QLatin1String(" e2 82 ac "));
    data.doCommand("normal! l");
    message.clear();
    data.doCommand("normal! g8");
    QCOMPARE(message, QLatin1String(" 09 "));
}

void FakeVimTester::test_vim_rot13()
{
    // "g?" rot13s the region it is given, like "g~"/"gu"/"gU" but with no bare
    // form of its own - it always needs the "g", in normal mode and in visual
    // mode alike. Values taken from Vim 9.1.
    TestData data;
    setup(&data);

    // "g??" rot13s the current line; the cursor ends on its first character.
    data.setText(X "Hello, World!");
    KEYS("g??", X "Uryyb, Jbeyq!");

    // "g?" with a motion.
    data.setText(X "abc");
    KEYS("g?l", X "nbc");

    // And over a visual selection.
    data.setText(X "Hello, World!" N "second");
    KEYS("Vjg?", X "Uryyb, Jbeyq!" N "frpbaq");
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

    // A pattern of no length is the last search pattern, which the substitutes
    // above left as "^" - so this one replaces the start of the line (Vim 9.1).
    data.setText("abc");
    COMMAND("s//--/g", "--abc");
    // And a search says what it is from then on.
    data.setText(X "abc");
    KEYS("/b<CR>:s//--/g<CR>", X "a--c");

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
    // The "'" mark moved along with the line it stood on, so there is nowhere to
    // jump back to (measured in Vim 9.1, which stays at 2,1 as well).
    KEYS("`'", "def" N X "abc" N "ghi" N "jkl");
    KEYS("Vj:m+2<cr>", "def" N "jkl" N "abc" N X "ghi");
    KEYS("u", "def" N X "abc" N "ghi" N "jkl");

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

    // Every matching line is taken once, the first one included.
    data.setText("abc" N "def");
    COMMAND("g/a/d", X "def");

    // Bare ":global", with no separator at all, used to crash
    // (cmd.args.front() on an empty string); it is E148 in Vim 9.1, not a
    // pattern of zero length. Disabling the guard makes this abort rather
    // than fail, which is still the fix proving itself.
    QString message;
    data.handler->commandBufferChanged.set(
        [&](const QString &msg, int, int, int) {
            if (!msg.isEmpty() && !msg.startsWith("--"))
                message = msg;
        });
    data.setText("abc" N "def");
    message.clear();
    data.doCommand("global");
    QCOMPARE(message, QLatin1String("E148: Regular expression missing from :global"));
    QCOMPARE(data.text(), QByteArray("abc" N "def"));
    message.clear();
    data.doCommand("vglobal");
    QCOMPARE(message, QLatin1String("E148: Regular expression missing from :global"));

    // An EMPTY pattern between separators is different from no separator at
    // all: it reuses the last search pattern, same as ":substitute" does.
    data.setText("aa" N "bb" N "aa");
    data.doCommand("let @/ = 'aa'");
    COMMAND("g//d", "bb");

    // A real pattern given to ":global" becomes the new last search pattern.
    data.setText("aa" N "bb" N "cc");
    data.doCommand("let @/ = 'zz'");
    data.doCommand("g/aa/d");
    message.clear();
    data.doCommand("echo @/");
    QCOMPARE(message, QLatin1String("aa"));
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

void FakeVimTester::test_vim_command_mapclear()
{
    // ":mapclear" (and its per-mode spellings) removes every mapping for the
    // modes the command applies to - a walk-and-remove narrowed to one mode,
    // the same table ":map" already uses. Values taken from Vim 9.1.
    TestData data;
    setup(&data);
    QString message;
    data.handler->commandBufferChanged.set(
        [&](const QString &msg, int, int, int) {
            if (!msg.isEmpty() && !msg.startsWith("--"))
                message = msg;
        });
    const auto value = [&](const QString &expr) {
        message.clear();
        data.doCommand("echo " + expr);
        return message;
    };

    data.doCommand("nnoremap zq :echo 1<CR>");
    QCOMPARE(value("maparg('zq', 'n')"), QLatin1String(":echo 1<CR>"));
    data.doCommand("nmapclear");
    QCOMPARE(value("maparg('zq', 'n')"), QString());

    // Bare ":mapclear" reaches Normal, Visual and Operator-pending - the
    // same "nvo" scope bare ":map" writes to - but not Insert.
    data.doCommand("nnoremap zq :echo 1<CR>");
    data.doCommand("inoremap zq XXX");
    data.doCommand("mapclear");
    QCOMPARE(value("maparg('zq', 'n')"), QString());
    QCOMPARE(value("maparg('zq', 'i')"), QLatin1String("XXX"));
    data.doCommand("iunmap zq");

    // ":mapclear!" is the Insert+Command-line pair, matching ":map!"'s own
    // scope.
    data.doCommand("inoremap zq XXX");
    data.doCommand("cnoremap zq echo 1");
    data.doCommand("mapclear!");
    QCOMPARE(value("maparg('zq', 'i')"), QString());
    QCOMPARE(value("maparg('zq', 'c')"), QString());

    // Every other per-mode spelling.
    data.doCommand("vnoremap zq d");
    data.doCommand("vmapclear");
    QCOMPARE(value("maparg('zq', 'v')"), QString());
    data.doCommand("onoremap zq iw");
    data.doCommand("omapclear");
    QCOMPARE(value("maparg('zq', 'o')"), QString());
    data.doCommand("snoremap zq d");
    data.doCommand("smapclear");
    QCOMPARE(value("maparg('zq', 's')"), QString());
    data.doCommand("lnoremap zq foo");
    data.doCommand("lmapclear");
    QCOMPARE(value("maparg('zq', 'l')"), QString());
}

void FakeVimTester::test_vim_no_overwrite_when_editor_takes_keys()
{
    // An inline rename reaches the same branch but needs the C++ code
    // model, so snippet mode is what is driven here. The key has to be a
    // real event: handleInput() goes straight to handleKey() and never
    // through the event filter this branch lives in.
    TestData data;
    setup(&data);

    data.setText("one two");
    data.doKeys("<Esc>");
    QVERIFY2(data.editor()->overwriteMode(),
             "command mode should draw a block cursor, which is overwrite mode");

    data.editor()->insertCodeSnippet(0, "$name$ tail", &TextEditor::Snippet::parse);
    bool inSnippetMode = false;
    QMetaObject::invokeMethod(data.editor(), "inSnippetMode", Q_ARG(bool *, &inSnippetMode));
    QVERIFY2(inSnippetMode, "snippet mode did not start - the test cannot say anything");

    QTest::keyClick(data.editor(), Qt::Key_X, Qt::NoModifier);
    QVERIFY2(!data.editor()->overwriteMode(),
             "editor left in overwrite mode while it is the one taking keys");
    // The first key replaces the selected placeholder, which happens with or
    // without overwrite mode; the second is the one that shows the damage.
    QTest::keyClick(data.editor(), Qt::Key_Y, Qt::NoModifier);
    QCOMPARE(data.text(), QByteArray("xy tailone two"));
}

void FakeVimTester::test_vim_command_iabbrev()
{
    // ":iabbrev" and its kin - insert-mode abbreviations. Typing the word
    // and then any character that is not a keyword character puts the
    // expansion in its place, keeping the character that ended it; leaving
    // insert mode expands too. Values taken from Vim 9.1.
    TestData data;
    setup(&data);

    data.doCommand("iabclear");
    data.doCommand("iabbrev teh the");

    // The non-keyword character that ends the word triggers it, and stays.
    data.setText("");
    KEYS("ccteh x<Esc>", "the " X "x");

    // Leaving insert mode ends the word as well.
    data.setText("");
    KEYS("ccteh<Esc>", "th" X "e");

    // It has to be a whole word: no expansion inside a longer one.
    data.setText("");
    KEYS("cctehx y<Esc>", "tehx " X "y");

    // Nor when the word merely ENDS with the abbreviation.
    data.setText("");
    KEYS("ccxteh y<Esc>", "xteh " X "y");

    // ":inoreabbrev" writes one the same way - an expansion is inserted as
    // text and never looked up again, so there is nothing for "nore" to
    // change here.
    data.doCommand("inoreabbrev fo bar");
    data.setText("");
    KEYS("ccfo<Esc>", "ba" X "r");

    // ":iunabbrev" takes one back.
    data.doCommand("iunabbrev teh");
    data.setText("");
    KEYS("ccteh x<Esc>", "teh " X "x");

    // ":iabclear" takes them all.
    data.doCommand("iabclear");
    data.setText("");
    KEYS("ccfo x<Esc>", "fo " X "x");
}

void FakeVimTester::test_vim_command_map_bang()
{
    // ":map!"/":noremap!"/":unmap!" are the Insert+Command-line pair, not
    // Normal+Visual+Operator-pending - the "!" they take never reaches the
    // dispatcher as a literal suffix (the general parser strips a trailing
    // "!" into cmd.hasBang first), so a plain `cmd == "map!"` string match
    // could never fire; a bare ":map!" fell through to the unbanged "nvo"
    // scope instead. Values taken from Vim 9.1.
    TestData data;
    setup(&data);
    QString message;
    data.handler->commandBufferChanged.set(
        [&](const QString &msg, int, int, int) {
            if (!msg.isEmpty() && !msg.startsWith("--"))
                message = msg;
        });
    const auto value = [&](const QString &expr) {
        message.clear();
        data.doCommand("echo " + expr);
        return message;
    };

    data.doCommand("map! zq XXX");
    QCOMPARE(value("maparg('zq', 'i')"), QLatin1String("XXX"));
    QCOMPARE(value("maparg('zq', 'c')"), QLatin1String("XXX"));
    QCOMPARE(value("maparg('zq', 'n')"), QString());
    data.doCommand("unmap! zq");
    QCOMPARE(value("maparg('zq', 'i')"), QString());

    data.doCommand("noremap! zq YYY");
    QCOMPARE(value("maparg('zq', 'i')"), QLatin1String("YYY"));
    QCOMPARE(value("maparg('zq', 'c')"), QLatin1String("YYY"));
    data.doCommand("unmap! zq");

    // ":unmap!" removes only Insert+Command-line, leaving Normal untouched -
    // isolates the Unmap branch's own mode-scope choice from the others.
    data.doCommand("inoremap zq XXX");
    data.doCommand("nnoremap zq YYY");
    data.doCommand("unmap! zq");
    QCOMPARE(value("maparg('zq', 'i')"), QString());
    QCOMPARE(value("maparg('zq', 'n')"), QLatin1String("YYY"));
    data.doCommand("nunmap zq");
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

    // Joining comments, which "j" among 'formatoptions' asks for - Vim has no "f"
    // there, which is what this used to be written with.
    data.doCommand("set formatoptions=j");
    data.setText("// abc" N "// def");
    KEYS("J", "// abc def");

    data.setText("/*" N X "* abc" N "* def" N "*/");
    KEYS("J", "/*" N "* abc def" N "*/");

    data.setText("# abc" N "# def");
    KEYS("J", "# abc def");
    data.doCommand("set formatoptions=");
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

void FakeVimTester::test_vim_plugin_off_leaves_buffer_alone()
{
    // The plugin gives a buffer what FakeVim wants of it - its file type, what
    // its modelines say, the autocommands for reading and entering it - only
    // where FakeVim is in use, and gives it to the buffers already open when
    // FakeVim is switched on. A modeline says what happened: "sw=3" reaches
    // 'shiftwidth' when the buffer was taken over, and not before.
    FvBoolAspect &useFakeVim = FakeVim::Internal::settings().useFakeVim;
    FvIntegerAspect &shiftWidth = FakeVim::Internal::settings().shiftWidth;
    FvIntegerAspect &tabStop = FakeVim::Internal::settings().tabStop;
    const bool savedUseFakeVim = useFakeVim.value();
    const int savedShiftWidth = shiftWidth.value();
    const int savedTabStop = tabStop.value();
    useFakeVim.setValue(false);
    shiftWidth.setValue(8);
    tabStop.setValue(8);

    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const Utils::FilePath file = Utils::FilePath::fromString(dir.path() + "/modeline.txt");
    QVERIFY(file.writeFileContents("# vim: sw=3 ts=3\nalpha\nbeta\n"));

    // Opening it while FakeVim is off leaves the settings where they stood.
    Core::IEditor *editor = Core::EditorManager::openEditor(file);
    const int afterOpen = shiftWidth.value();
    const int afterOpenTab = tabStop.value();

    // Switching FakeVim on reaches the buffer that is already open.
    useFakeVim.setValue(true);
    const int afterEnabling = shiftWidth.value();
    const int afterEnablingTab = tabStop.value();

    if (editor)
        Core::EditorManager::closeEditors({editor}, false);
    useFakeVim.setValue(savedUseFakeVim);
    shiftWidth.setValue(savedShiftWidth);
    tabStop.setValue(savedTabStop);

    QVERIFY(editor);
    QCOMPARE(afterOpen, 8);
    QCOMPARE(afterOpenTab, 8);
    QCOMPARE(afterEnabling, 3);
    QCOMPARE(afterEnablingTab, 3);
}

void FakeVimTester::test_vim_plugin_modeline_of_the_current_buffer()
{
    // Options are held once, not per buffer, so the modelines that have a say are
    // the ones of the buffer in front - not of whichever came first out of a hash.
    // A buffer visited later has its say then.
    FvBoolAspect &useFakeVim = FakeVim::Internal::settings().useFakeVim;
    FvIntegerAspect &shiftWidth = FakeVim::Internal::settings().shiftWidth;
    const bool savedUseFakeVim = useFakeVim.value();
    const int savedShiftWidth = shiftWidth.value();
    useFakeVim.setValue(false);
    shiftWidth.setValue(8);

    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const Utils::FilePath three = Utils::FilePath::fromString(dir.path() + "/three.txt");
    const Utils::FilePath five = Utils::FilePath::fromString(dir.path() + "/five.txt");
    QVERIFY(three.writeFileContents("# vim: sw=3\nalpha\n"));
    QVERIFY(five.writeFileContents("# vim: sw=5\nbeta\n"));

    Core::IEditor *first = Core::EditorManager::openEditor(three);
    Core::IEditor *second = Core::EditorManager::openEditor(five);
    const int afterOpen = shiftWidth.value();
    useFakeVim.setValue(true);
    const int afterEnabling = shiftWidth.value();
    // Going back to the other buffer gives it its say.
    if (first)
        Core::EditorManager::activateEditor(first);
    const int afterActivating = shiftWidth.value();

    QList<Core::IEditor *> open;
    if (first)
        open << first;
    if (second)
        open << second;
    if (!open.isEmpty())
        Core::EditorManager::closeEditors(open, false);
    useFakeVim.setValue(savedUseFakeVim);
    shiftWidth.setValue(savedShiftWidth);

    QVERIFY(first);
    QVERIFY(second);
    QCOMPARE(afterOpen, 8);
    QCOMPARE(afterEnabling, 5);
    QCOMPARE(afterActivating, 3);
}

void FakeVimTester::test_vim_plugin_buffer_lifecycle_events()
{
    // The events around a buffer coming and going, which happen in the
    // PLUGIN rather than the handler - opening an editor gives BufNew,
    // BufAdd and (for a file that is there) BufReadPre before the
    // BufReadPost this engine already fired; closing one gives BufWinLeave,
    // BufUnload and BufDelete. Measured in Vim 9.1 on ":edit" and
    // ":bdelete".
    //
    // How this reaches the plugin at all: GlobalData is a STATIC member of
    // FakeVimHandler::Private, so the autocommand table and the g: variable
    // store are shared by every handler in the process. The autocommands go
    // in through a plain TestData handler; the real editor opened below gets
    // its own handler from the plugin and fires into that same shared table,
    // which the TestData handler then reads back.
    FvBoolAspect &useFakeVim = FakeVim::Internal::settings().useFakeVim;
    const bool savedUseFakeVim = useFakeVim.value();
    useFakeVim.setValue(true);

    TestData data;
    setup(&data);
    QString message;
    data.handler->commandBufferChanged.set(
        [&](const QString &msg, int, int, int) {
            if (!msg.isEmpty() && !msg.startsWith("--"))
                message = msg;
        });
    const auto value = [&](const QString &expr) {
        message.clear();
        data.doCommand("echo " + expr);
        return message;
    };

    // Each entry records the event AND which file it is about - the test
    // editor TestData itself opens (setupTest() uses EditorManager too, so
    // its handler is a plugin-managed one) is switched away from here, and
    // without the name it is not clear which buffer an event belongs to.
    data.doCommand("let g:bl = []");
    for (const QString &event : QStringList{"BufNew", "BufAdd", "BufReadPre", "BufReadPost",
                                 "BufNewFile", "BufWinLeave", "BufUnload", "BufDelete",
                                 "BufHidden"}) {
        data.doCommand("autocmd FvBl " + event + " * call add(g:bl, '" + event
                       + ":' . fnamemodify(expand('<afile>'), ':t'))");
    }

    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const Utils::FilePath existing = Utils::FilePath::fromString(dir.path() + "/there.txt");
    const Utils::FilePath other = Utils::FilePath::fromString(dir.path() + "/other.txt");
    QVERIFY(existing.writeFileContents("alpha\n"));
    QVERIFY(other.writeFileContents("beta\n"));

    // A file that is there: read, so BufReadPre/Post and no BufNewFile.
    Core::IEditor *editor = Core::EditorManager::openEditor(existing);
    const QString afterOpen = value("string(g:bl)");

    // Switching away leaves the first buffer loaded but out of its window,
    // which is what Vim calls hidden.
    data.doCommand("let g:bl = []");
    Core::IEditor *second = Core::EditorManager::openEditor(other);
    const QString afterSwitch = value("string(g:bl)");

    // Closing gives the unload sequence and NO BufHidden - Vim fires that
    // only for a buffer that stays.
    data.doCommand("let g:bl = []");
    if (second)
        Core::EditorManager::closeEditors({second}, false);
    const QString afterClose = value("string(g:bl)");

    data.doCommand("autocmd! FvBl");
    data.doCommand("unlet! g:bl");
    if (editor)
        Core::EditorManager::closeEditors({editor}, false);
    useFakeVim.setValue(savedUseFakeVim);

    QVERIFY(editor);
    QVERIFY(second);
    // The new buffer: BufNew and BufAdd before it is read, then BufReadPre
    // (it is there, so it is read rather than being a new file) and the
    // BufReadPost this engine already fired. The trailing BufHidden is the
    // TEST editor being switched away from - it has no name, because
    // setupTest() makes it with openEditorWithContents() and so it has no
    // file behind it.
    // NOTE the order Qt Creator imposes, which is NOT the Vim one: it emits
    // editorOpened before the current-editor change, so the arriving
    // buffer is read BEFORE the leaving one is hidden, where Vim
    // has the leave first. That is the Creator signal order, not a choice
    // made here.
    QCOMPARE(afterOpen, QLatin1String(
        "['BufNew:there.txt', 'BufAdd:there.txt', 'BufReadPre:there.txt',"
        " 'BufReadPost:there.txt', 'BufHidden:']"));
    // Switching again: the arriving buffer is read, the one left is hidden -
    // loaded still, just out of its window.
    QCOMPARE(afterSwitch, QLatin1String(
        "['BufNew:other.txt', 'BufAdd:other.txt', 'BufReadPre:other.txt',"
        " 'BufReadPost:other.txt', 'BufHidden:there.txt']"));
    // Closing gives the unload sequence and NO BufHidden for the buffer
    // going away - Vim fires that only for one that stays. Here that falls
    // out of editorAboutToClose running first and taking the handler out of
    // the map the plugin keeps, so the later current-editor change finds none.
    QCOMPARE(afterClose, QLatin1String(
        "['BufWinLeave:other.txt', 'BufUnload:other.txt', 'BufDelete:other.txt']"));
}

void FakeVimTester::test_vim_plugin_window_events()
{
    // WinNew and WinClosed, which are about the window LAYOUT rather than
    // about one buffer. A Vim window is a Qt Creator split, so these hang
    // off EditorManager reporting a view arriving or going.
    // WinNew carries nothing; WinClosed names the window that went, which
    // Vim reports as its id (measured) - the id Qt Creator hands out is
    // passed straight through, so the test asserts the SHAPE (a positive
    // number, the same one both times) rather than a literal, the way the
    // rest of this file treats ids and paths it does not own.
    FvBoolAspect &useFakeVim = FakeVim::Internal::settings().useFakeVim;
    const bool savedUseFakeVim = useFakeVim.value();
    useFakeVim.setValue(true);

    TestData data;
    setup(&data);
    QString message;
    data.handler->commandBufferChanged.set(
        [&](const QString &msg, int, int, int) {
            if (!msg.isEmpty() && !msg.startsWith("--"))
                message = msg;
        });
    const auto value = [&](const QString &expr) {
        message.clear();
        data.doCommand("echo " + expr);
        return message;
    };

    data.doCommand("let g:wl = []");
    data.doCommand("autocmd FvWin WinNew * call add(g:wl, 'new:' . expand('<afile>'))");
    data.doCommand("autocmd FvWin WinClosed * call add(g:wl, 'closed:' . expand('<afile>'))");

    Core::EditorManager::splitSideBySide();
    const QString afterSplit = value("string(g:wl)");

    data.doCommand("let g:wl = []");
    // Removing a split has no public EditorManager entry point, so the
    // registered action is triggered - which is how the plugin itself does
    // it (see triggerAction in fakevimplugin.cpp).
    // REMOVE_CURRENT_SPLIT, not REMOVE_ALL_SPLITS: splitting activates the
    // NEW view, so this closes exactly the one just made and leaves the
    // original - the one holding the editor this TestData handler works on
    // - alive. Removing ALL splits keeps the new view and discards the
    // original instead, which deletes that editor from under the handler
    // and makes the next doCommand() dereference a dead widget.
    if (Core::Command *cmd = Core::ActionManager::command(Core::Constants::REMOVE_CURRENT_SPLIT)) {
        if (QAction *action = cmd->action())
            action->trigger();
    }
    const QString afterUnsplit = value("string(g:wl)");
    const QString closedId = value("substitute(g:wl[0], 'closed:', '', '')");

    data.doCommand("autocmd! FvWin");
    data.doCommand("unlet! g:wl");
    useFakeVim.setValue(savedUseFakeVim);

    // Splitting makes exactly one new window.
    // Vim leaves WinNew <afile> EMPTY, the event being about no file at
    // all; here it falls back to the current file name, because that is
    // what the shared firing path does with a target it was not given -
    // the same fallback the filter events already document. Only the
    // COUNT is asserted, so this does not pin the divergence in place.
    QCOMPARE(afterSplit.count("new:"), 1);
    QCOMPARE(afterSplit.count("closed:"), 0);
    // Removing the split closes exactly that one window, and names it.
    QCOMPARE(afterUnsplit.count("closed:"), 1);
    QCOMPARE(afterUnsplit.count("new:"), 0);
    bool ok = false;
    const int id = closedId.toInt(&ok);
    QVERIFY2(ok, qPrintable("WinClosed did not name a number: " + closedId));
    QVERIFY(id > 0);
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
    data.doCommand("set textwidth=10 noautoindent");

    // gqq reflows the current line to the text width, leaving the cursor on the
    // first character of the last line written. Values taken from Vim 9.1.
    data.setText("one two three four five six");
    KEYS("gqq", "one two" N "three four" N X "five six");

    // The indentation of the first line is kept for every wrapped line only where
    // "autoindent" asks for it.
    data.setText("  alpha beta gamma delta");
    KEYS("gqq", "  alpha" N "beta gamma" N X "delta");
    data.doCommand("set autoindent");
    data.setText("  alpha beta gamma delta");
    KEYS("gqq", "  alpha" N "  beta" N "  gamma" N "  " X "delta");
    data.doCommand("set noautoindent");

    // gq with a motion reflows the spanned lines as one paragraph.
    data.setText("aaa bbb ccc" N "ddd");
    KEYS("gqj", "aaa bbb" N X "ccc ddd");

    // Blank lines separate paragraphs and are preserved.
    data.setText("aaaa bbbb cccc" N "" N "dddd eeee ffff");
    KEYS("VGgq", "aaaa bbbb" N "cccc" N "" N "dddd eeee" N X "ffff");

    // A word longer than the text width still gets its own line.
    data.setText("hi supercalifragilistic bye");
    KEYS("gqq", "hi" N "supercalifragilistic" N X "bye");

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

    // A Boolean is a type of its own, shown as Vim shows it, and one where a
    // number is wanted.
    QCOMPARE(echo("v:true"), QLatin1String("v:true"));
    QCOMPARE(echo("v:false"), QLatin1String("v:false"));
    QCOMPARE(echo("v:true + 0"), QLatin1String("1"));
    QCOMPARE(echo("v:true == 1"), QLatin1String("1"));
    QCOMPARE(echo("type(v:true)"), QLatin1String("6"));
    QCOMPARE(echo("string([v:true, v:false])"), QLatin1String("[v:true, v:false]"));
    // A comparison answers with a plain number, not with a Boolean.
    QCOMPARE(echo("string(1 == 1)"), QLatin1String("1"));
    // And what a list holds is compared by type as well.
    QCOMPARE(echo("[v:true] == [1]"), QLatin1String("0"));
    QCOMPARE(echo("v:null"), QLatin1String("v:null"));
    QCOMPARE(echo("v:none"), QLatin1String("v:none"));
    QCOMPARE(echo("type(v:null)"), QLatin1String("7"));

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

    // Neither "sh" nor a standalone "echo" is on a Windows machine's PATH, and
    // system() starts the command itself rather than handing it to a shell
    const bool isWindows = Utils::HostOsInfo::isWindowsHost();
    QCOMPARE(echo(isWindows ? "executable('cmd')" : "executable('sh')"),
             QLatin1String("1"));
    QCOMPARE(echo("executable('definitely_no_such_cmd_xyz')"), QLatin1String("0"));
    QCOMPARE(echo(isWindows ? "substitute(system('cmd /c echo hi'), '\\n', '', 'g')"
                            : "substitute(system('echo hi'), '\\n', '', 'g')"),
             QLatin1String("hi"));
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
    // "%a" is the locale's abbreviated day name, so its width follows LC_TIME:
    // 3 letters in C, 2 in de_DE, 1 in ja_JP; check only that conversion was expanded at all
    QVERIFY(echo("strftime('%a', 946684800)") != QLatin1String("%a"));
    QVERIFY(!echo("strftime('%a', 946684800)").isEmpty());

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
    QCOMPARE(echo("getcurpos()"), QLatin1String("[0, 1, 1, 0, 1]"));

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

    // Linewise: "g@j" is handed "line", and the marks hold where the motion
    // began and ended - Vim does not pull them out to the edges of the lines
    // (measured from Vim 9.1, which had been read the other way here).
    data.doKeys("gg0g@j");
    QCOMPARE(echo("g:kind"), QLatin1String("line"));
    QCOMPARE(echo("g:from"), QLatin1String("[1, 1]"));
    QCOMPARE(echo("g:to"), QLatin1String("[2, 1]"));

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
    QCOMPARE(echo("g:to"), QLatin1String("[2, 1]"));

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

    // Vim wants a name for every item: too many of either is an error, and
    // nothing is assigned then (measured from Vim 9.1).
    data.doCommand("let g:p = 'keep' | let g:q = 'keep'");
    data.doCommand("let g:e = '' | try | let [g:p, g:q] = [10, 20, 30]"
                   " | catch | let g:e = v:exception | endtry");
    QVERIFY(echo("g:e").contains(QLatin1String("E687")));
    QCOMPARE(echo("g:p"), QLatin1String("keep"));
    data.doCommand("let g:e = '' | try | let [g:m, g:n] = [7]"
                   " | catch | let g:e = v:exception | endtry");
    QVERIFY(echo("g:e").contains(QLatin1String("E688")));

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
        [[maybe_unused]] const bool success = f.open(QIODevice::WriteOnly | QIODevice::Truncate);
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
    // how a plugin points 'operatorfunc' at something of its own. Values taken
    // from Vim 9.1.
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
    // Vim writes the "<SNR>42_" form of the name into the option, where "<SID>"
    // was written in the mapping.
    message.clear();
    data.doCommand("echo &opfunc =~# '^<SNR>\\d\\+_priv$'");
    const QString option = message;
    // The mappings and the option live in a table shared with every other test.
    data.doCommand("nunmap QF | nmapclear <Plug>(Probe) | set opfunc=");
    data.doCommand("unlet g:F | unlet g:log");

    QCOMPARE(log, QLatin1String("funcref plain | priv called | priv char"));
    QCOMPARE(option, QLatin1String("1"));
}

void FakeVimTester::test_vim9_statements()
{
    // What a Vim9 script may write down as a statement: a block of its own, a
    // ":for" naming the type it walks over, a chain of method calls whose value
    // is thrown away, a bracket closed on a line of its own and an unpacking
    // declaration naming types. A function declares its own names, so the one it
    // holds is not the one a block outside kept. Values taken from Vim 9.1.
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
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    QFile f(dir.path() + "/s.vim");
    QVERIFY(f.open(QIODevice::WriteOnly));
    f.write("vim9script\n"
            "var out = []\n"
            "var d = {}\n"
            "{\n"
            "    d->extend({x: 1})\n"
            "    var scoped = 5\n"
            "    out->add('block=' .. scoped)\n"
            "}\n"
            "var sum = 0\n"
            "for i: number in [1, 2, 3]\n"
            "    sum += i\n"
            "endfor\n"
            "out->add('for=' .. sum)\n"
            "var l = ['a']\n"
            "l->add('b')\n"
            "out->add('method=' .. l->join(','))\n"
            "var m = {\n"
            "    k: [\n"
            "        1,\n"
            "        2,\n"
            "    ],\n"
            "}\n"
            "out->add('close=' .. string(m))\n"
            "var [first: string, second: number] = ['s', 7]\n"
            "out->add('unpack=' .. first .. second)\n"
            "out->add('dict=' .. string(d))\n"
            "def Shadow(): string\n"
            "    var kwd = 'local'\n"
            "    return kwd\n"
            "enddef\n"
            "for kwd: string in ['a', 'b']\n"
            "    out->add('shadow=' .. Shadow() .. '/' .. kwd)\n"
            "endfor\n"
            "g:out = out\n");
    f.close();
    data.doCommand("source " + dir.path() + "/s.vim");
    QCOMPARE(value("g:out[0]"), QLatin1String("block=5"));
    QCOMPARE(value("g:out[1]"), QLatin1String("for=6"));
    QCOMPARE(value("g:out[2]"), QLatin1String("method=a,b"));
    QCOMPARE(value("g:out[3]"), QLatin1String("close={'k': [1, 2]}"));
    QCOMPARE(value("g:out[4]"), QLatin1String("unpack=s7"));
    QCOMPARE(value("g:out[5]"), QLatin1String("dict={'x': 1}"));
    QCOMPARE(value("g:out[6]"), QLatin1String("shadow=local/a"));
    QCOMPARE(value("g:out[7]"), QLatin1String("shadow=local/b"));
}

void FakeVimTester::test_vim9_script_level_scope()
{
    // A Vim9 function reads and writes the variables its script keeps, which is
    // how a plugin holds its patterns in one place and uses them everywhere -
    // Vim's own indent file for Vim scripts is built that way. A legacy function
    // sees only what it holds itself. Values taken from Vim 9.1.
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
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    QFile f(dir.path() + "/s.vim");
    QVERIFY(f.open(QIODevice::WriteOnly));
    f.write("vim9script\n"
            "var s_level = 'S'\n"
            "const s_const = 'C'\n"
            "def Read(): string\n"
            "  return s_level .. s_const\n"
            "enddef\n"
            "def Write()\n"
            "  s_level = 'W'\n"
            "enddef\n"
            "g:read = Read()\n"
            "Write()\n"
            "g:after = s_level\n");
    f.close();
    data.doCommand("source " + dir.path() + "/s.vim");
    QCOMPARE(value("g:read"), QLatin1String("SC"));
    QCOMPARE(value("g:after"), QLatin1String("W"));
    // A legacy function holds its own names alone: a bare one is not the global.
    data.doCommand("let g:bare = 'G'");
    QFile l(dir.path() + "/l.vim");
    QVERIFY(l.open(QIODevice::WriteOnly));
    l.write("function! Legacy()\n"
            "  return exists('bare') ? 'sees-global' : 'local-only'\n"
            "endfunction\n");
    l.close();
    data.doCommand("source " + dir.path() + "/l.vim");
    QCOMPARE(value("Legacy()"), QLatin1String("local-only"));
    data.doCommand("unlet g:read | unlet g:after | unlet g:bare");
    data.doCommand("delfunction Legacy | delfunction Read | delfunction Write");
}

void FakeVimTester::test_vim_script_script_scope()
{
    // Each script has an "s:" of its own: two scripts using the same name keep
    // two different things, and neither reaches what the other keeps there. The
    // script a function was defined in is the one its "s:" reaches, however far
    // from that script it is called. Values taken from Vim 9.1.
    TestData data;
    setup(&data);
    QString message;
    data.handler->commandBufferChanged.set(
        [&](const QString &msg, int, int, int) {
            if (!msg.startsWith("--"))
                message = msg;
        });
    const auto echo = [&](const QString &expr) {
        message.clear();
        data.doCommand("echo " + expr);
        return message;
    };
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    QFile a(dir.path() + "/a.vim");
    QVERIFY(a.open(QIODevice::WriteOnly));
    a.write("let s:x = 'from-A'\n"
            "let s:kept = 'A'\n"
            "function! s:Same()\n"
            "  return 'A-Same'\n"
            "endfunction\n"
            "function! s:Only()\n"
            "  return 'A-Only ' . s:x\n"
            "endfunction\n"
            "function! GlobalFromA()\n"
            "  return 'global-in-A sees ' . s:x\n"
            "endfunction\n"
            "function! A_Report()\n"
            "  return [s:x, s:Same(), s:Only(), expand('<SID>')]\n"
            "endfunction\n"
            "let g:A_ref = function('s:Same')\n"
            "nnoremap <silent> QA :call add(g:log, <SID>Same())<CR>\n"
            "augroup ScopeProbe\n"
            "  autocmd User * call add(g:log, 'autocmd-A ' . s:x)\n"
            "augroup END\n"
            "command! ACmd call add(g:log, 'cmd-A ' . s:x)\n");
    a.close();
    QFile b(dir.path() + "/b.vim");
    QVERIFY(b.open(QIODevice::WriteOnly));
    b.write("let s:x = 'from-B'\n"
            "function! s:Same()\n"
            "  return 'B-Same'\n"
            "endfunction\n"
            "function! B_Report()\n"
            "  return [s:x, s:Same(), exists('s:kept'), exists('*s:Only'), expand('<SID>')]\n"
            "endfunction\n"
            "function! B_CallsA()\n"
            "  return g:A_ref()\n"
            "endfunction\n");
    b.close();
    data.doCommand("let g:log = []");
    data.doCommand("source " + dir.path() + "/a.vim");
    data.doCommand("source " + dir.path() + "/b.vim");

    // Same names, two scripts, two answers.
    QCOMPARE(echo("string(A_Report()[0:2])"),
             QLatin1String("['from-A', 'A-Same', 'A-Only from-A']"));
    // And what A keeps in its own scope is not there to be read or called.
    QCOMPARE(echo("string(B_Report()[0:3])"),
             QLatin1String("['from-B', 'B-Same', 0, 0]"));
    // The script a function belongs to decides, so a global one defined in A
    // reads A's, and a Funcref made there still calls A's where B calls it.
    QCOMPARE(echo("GlobalFromA()"), QLatin1String("global-in-A sees from-A"));
    QCOMPARE(echo("B_CallsA()"), QLatin1String("A-Same"));
    // Each script is numbered, and "<SID>" names its own number.
    QCOMPARE(echo("A_Report()[3] =~ '^<SNR>\\d\\+_$'"), QLatin1String("1"));
    QCOMPARE(echo("A_Report()[3] != B_Report()[4]"), QLatin1String("1"));
    // A mapping is written with the number of the script that wrote it, which is
    // how it still finds what belongs there when it runs.
    QCOMPARE(echo("maparg('QA', 'n') =~ '<SNR>\\d\\+_Same()'"), QLatin1String("1"));
    data.setText(X "one two three");
    data.doKeys("QA");
    // An autocommand and a user command reach the "s:" of the script that
    // registered them.
    data.doCommand("doautocmd User ScopeProbe");
    data.doCommand("ACmd");
    QCOMPARE(echo("join(g:log, ' | ')"),
             QLatin1String("A-Same | autocmd-A from-A | cmd-A from-A"));
    // A typed command line has an "s:" of its own, which no script shares.
    data.doCommand("let s:x = 'typed'");
    QCOMPARE(echo("s:x"), QLatin1String("typed"));
    QCOMPARE(echo("A_Report()[0]"), QLatin1String("from-A"));
    QCOMPARE(echo("exists('*s:Only')"), QLatin1String("0"));

    // The mappings, autocommands and commands live in tables shared with every
    // other test.
    data.doCommand("nunmap QA | delcommand ACmd");
    data.doCommand("augroup ScopeProbe | autocmd! | augroup END");
    data.doCommand("unlet g:log | unlet g:A_ref | unlet s:x");
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

void FakeVimTester::test_vim_script_unpack_rest()
{
    // ":let [a, b; rest] = list" gives the last name what is left of the list,
    // and without one the counts have to match. Values taken from Vim 9.1.
    TestData data;
    setup(&data);
    QString message;
    data.handler->commandBufferChanged.set(
        [&](const QString &msg, int, int, int) {
            if (!msg.startsWith("--"))
                message = msg;
        });
    const auto result = [&](const QString &command, const QString &read) {
        data.doCommand("let g:r = 'unset'");
        data.doCommand("try | " + command + " | let g:r = string(" + read
                       + ") | catch | let g:r = v:exception | endtry");
        message.clear();
        data.doCommand("echo g:r");
        return message;
    };

    QCOMPARE(result("let [a, b; rest] = [1,2,3,4]", "[a, b, rest]"),
             QLatin1String("[1, 2, [3, 4]]"));
    // Nothing left over still makes a list, an empty one.
    QCOMPARE(result("let [c; rest2] = [1]", "[c, rest2]"), QLatin1String("[1, []]"));
    QCOMPARE(result("let [g, h; rest3] = ['n', '[e', 'x', 'y']", "[g, h, rest3]"),
             QLatin1String("['n', '[e', ['x', 'y']]"));
    QCOMPARE(result("let [i, j] = [1, 2]", "[i, j]"), QLatin1String("[1, 2]"));
    // Too few or too many items are errors a script can catch.
    QVERIFY(result("let [e, f; rest4] = [1]", "[e]").contains(QLatin1String("E688")));
    QVERIFY(result("let [k, l] = [1, 2, 3]", "[k, l]").contains(QLatin1String("E687")));
    data.doCommand("unlet g:r");
}

void FakeVimTester::test_vim_ex_put()
{
    // ":put" puts what a register or an expression holds as whole lines after
    // the line the range names, or before it with a "!", and leaves the cursor
    // on the last of them. Values taken from Vim 9.1.
    TestData data;
    setup(&data);
    QString message;
    data.handler->commandBufferChanged.set(
        [&](const QString &msg, int, int, int) {
            if (!msg.startsWith("--"))
                message = msg;
        });
    const auto run = [&](const char *command, const char *start) -> QString {
        data.setText(X "alpha" N "beta" N "gamma");
        data.doCommand("call setreg('a', \"AAA\\n\", 'V')");
        data.doCommand("call setreg('b', 'BBB', 'v')");
        // The registers are shared with every other test.
        data.doCommand("call setreg('z', '')");
        data.doKeys(start);
        data.doCommand(QLatin1String("let g:e = '' | try | ") + command
                       + " | catch | let g:e = v:exception | endtry");
        message.clear();
        data.doCommand("echo line('.') . ',' . col('.') . g:e");
        return QString::fromUtf8(data.text()).replace(QLatin1Char('\n'), QLatin1String("/"))
               + "  at " + message;
    };

    QCOMPARE(run("put a", "j0l"), QLatin1String("alpha/beta/AAA/gamma  at 3,1"));
    QCOMPARE(run("put! a", "j0l"), QLatin1String("alpha/AAA/beta/gamma  at 2,1"));
    // A charwise register goes in as a line all the same.
    QCOMPARE(run("put b", "j0l"), QLatin1String("alpha/beta/BBB/gamma  at 3,1"));
    QCOMPARE(run("put! b", "j0l"), QLatin1String("alpha/BBB/beta/gamma  at 2,1"));
    QCOMPARE(run("1put a", "j0l"), QLatin1String("alpha/AAA/beta/gamma  at 2,1"));
    QCOMPARE(run("$put a", "0l"), QLatin1String("alpha/beta/gamma/AAA  at 4,1"));
    QCOMPARE(run("2put a", "0l"), QLatin1String("alpha/beta/AAA/gamma  at 3,1"));
    // A range puts after its last line.
    QCOMPARE(run("1,2put a", "jj0l"), QLatin1String("alpha/beta/AAA/gamma  at 3,1"));
    QCOMPARE(run("pu a", "j0l"), QLatin1String("alpha/beta/AAA/gamma  at 3,1"));
    // The expression form, which needs no register at all.
    QCOMPARE(run("put ='X'", "j0l"), QLatin1String("alpha/beta/X/gamma  at 3,1"));
    QCOMPARE(run("put! ='X'", "j0l"), QLatin1String("alpha/X/beta/gamma  at 2,1"));
    QCOMPARE(run("put ='a' . nr2char(10) . 'b'", "j0l"),
             QLatin1String("alpha/beta/a/b/gamma  at 4,1"));
    QCOMPARE(run("put =['p', 'q']", "j0l"), QLatin1String("alpha/beta/p/q/gamma  at 4,1"));
    // Line breaks alone are empty lines, which is how a plugin makes room.
    QCOMPARE(run("put! =repeat(nr2char(10), 2)", "j0l"),
             QLatin1String("alpha///beta/gamma  at 3,1"));
    QCOMPARE(run("put =''", "j0l"), QLatin1String("alpha/beta//gamma  at 3,1"));
    // The cursor goes to the first non-blank of the last line put.
    QCOMPARE(run("put ='   ind'", "j0l"), QLatin1String("alpha/beta/   ind/gamma  at 3,4"));
    // An empty register is an error, and nothing is put.
    QVERIFY(run("put z", "j0l").contains(QLatin1String("E353")));

    // The unnamed register, as a plugin duplicates a line.
    data.setText(X "alpha" N "beta" N "gamma");
    data.doKeys("yy");
    data.doCommand("put");
    QCOMPARE(data.text(), QByteArray("alpha\nalpha\nbeta\ngamma"));
    data.doCommand("unlet g:e");
}

void FakeVimTester::test_vim_script_named_key_string()
{
    // A key with a name written into a string, as "\<Left>", is that key when
    // the string is used as keys. Values taken from Vim 9.1.
    TestData data;
    setup(&data);
    QString message;
    data.handler->commandBufferChanged.set(
        [&](const QString &msg, int, int, int) {
            if (!msg.startsWith("--"))
                message = msg;
        });
    const auto run = [&](const char *command) -> QString {
        data.setText(X "abcdef");
        data.doCommand(command);
        message.clear();
        data.doCommand("echo line('.') . ',' . col('.')");
        return message + " [" + QString::fromUtf8(data.text()) + "]";
    };

    QCOMPARE(run("execute \"normal! $\\<Left>\""), QLatin1String("1,5 [abcdef]"));
    QCOMPARE(run("execute \"normal! 0\\<Right>\\<Right>\""), QLatin1String("1,3 [abcdef]"));
    QCOMPARE(run("execute \"normal! iXY\\<Left>Z\\<Esc>\""),
             QLatin1String("1,2 [XZYabcdef]"));
    // Also when the keys come out of a register that is run.
    data.setText(X "abcdef");
    data.doCommand("let @z = \"0\\<Right>x\"");
    data.doKeys("@z");
    QCOMPARE(data.text(), QByteArray("acdef"));
    data.doCommand("let @z = ''");
}

void FakeVimTester::test_vim_register_carriage_return()
{
    // A register holds the characters themselves, so the carriage return in one
    // a script fills is the Return key. Values taken from Vim 9.1.
    TestData data;
    setup(&data);
    QString message;
    data.handler->commandBufferChanged.set(
        [&](const QString &msg, int, int, int) {
            if (!msg.startsWith("--"))
                message = msg;
        });
    const auto run = [&](const char *command, const char *keys) -> QString {
        data.setText(X "one two three");
        data.doCommand(command);
        data.doKeys(keys);
        message.clear();
        data.doCommand("echo line('.') . ',' . col('.')");
        return message + " [" + QString::fromUtf8(data.text()) + "]";
    };

    // The ex command has to be entered, not left standing on the command line.
    QCOMPARE(run("let @r = ':s/one/ONE/' . nr2char(13)", "@r"),
             QLatin1String("1,1 [ONE two three]"));
    QCOMPARE(run("let @s = '/two' . nr2char(13)", "@s"),
             QLatin1String("1,5 [one two three]"));
    QCOMPARE(run("let @t = 'iX' . nr2char(27)", "@t"),
             QLatin1String("1,1 [Xone two three]"));
    data.doCommand("let @r = '' | let @s = '' | let @t = ''");
}

void FakeVimTester::test_vim_script_getchar()
{
    // getchar() and getcharstr() hand out the keys already typed ahead: what is
    // left of a mapping being expanded, or of a register being run. Values taken
    // from Vim 9.1, which answers the same for both of those. Waiting for a key
    // that has not been typed is not possible here, so the form without an
    // argument answers like getchar(0).
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
    QFile f(dir.path() + "/g.vim");
    QVERIFY(f.open(QIODevice::WriteOnly));
    f.write("let g:log = []\n"
            "function! Grab()\n"
            "  call add(g:log, 'peek=' . string(getchar(1)))\n"
            "  call add(g:log, 'took=' . string(getchar(0)))\n"
            "endfunction\n"
            "function! GrabStr()\n"
            "  call add(g:log, 'str=' . string(getcharstr(0)))\n"
            "endfunction\n"
            "nnoremap QQ :call Grab()<CR>)\n"
            "nnoremap QS :call GrabStr()<CR>x\n"
            "nnoremap QA :call Grab()<CR><Left>\n");
    f.close();
    data.doCommand("source " + dir.path() + "/g.vim");
    const auto log = [&](const char *keys) {
        data.setText(X "one two three");
        data.doCommand("let g:log = []");
        data.doKeys(keys);
        message.clear();
        data.doCommand("echo join(g:log, ' ')");
        return message;
    };

    // Nothing typed ahead: no key to hand out.
    message.clear();
    data.doCommand("echo getchar(0) . ',' . getchar(1) . ',' . getchar()");
    QCOMPARE(message, QLatin1String("0,0,0"));
    message.clear();
    data.doCommand("echo '[' . getcharstr(0) . getcharstr() . ']'");
    QCOMPARE(message, QLatin1String("[]"));

    // What the mapping still has after the ":call" it is expanding.
    QCOMPARE(log("QQ"), QLatin1String("peek=41 took=41"));
    QCOMPARE(log("QS"), QLatin1String("str='x'"));
    // A key with a name comes back as the token this engine names it by, which
    // is what "\<Left>" writes as well, so comparing the two matches as it
    // does in Vim. Vim answers with its own encoding of the key there, and for
    // the peek with the first byte of it.
    QCOMPARE(log("QA"), QLatin1String("peek='<LEFT>' took='<LEFT>'"));
    message.clear();
    data.doCommand("echo \"\\<Left>\" . ',' . (\"\\<Left>\" ==# '<LEFT>')");
    QCOMPARE(message, QLatin1String("<LEFT>,1"));

    // And what a register being run still has.
    data.doCommand("let @q = \":call Grab()\" . nr2char(13) . '%'");
    QCOMPARE(log("@q"), QLatin1String("peek=37 took=37"));
    // The keys the plugin took are gone, so the "%" no longer moves the cursor.
    message.clear();
    data.doCommand("echo col('.')");
    QCOMPARE(message, QLatin1String("1"));

    data.doCommand("nunmap QQ | nunmap QS | nunmap QA");
    data.doCommand("delfunction Grab | delfunction GrabStr | unlet g:log");
    data.doCommand("let @q = ''");
}

void FakeVimTester::test_vim_script_eval()
{
    // eval() is the value of what a string says, which is how a script reads a
    // name it has built and the other half of string(). Values taken from Vim 9.1.
    TestData data;
    setup(&data);
    QString message;
    data.handler->commandBufferChanged.set(
        [&](const QString &msg, int, int, int) {
            if (!msg.startsWith("--"))
                message = msg;
        });
    const auto value = [&](const QString &expr) {
        data.doCommand("let g:e = ''");
        data.doCommand("try | let g:v = " + expr
                       + " | catch | let g:e = v:exception | let g:v = 'FAILED' | endtry");
        message.clear();
        data.doCommand("echo string(g:v)");
        return message;
    };
    const auto failure = [&](const QString &expr) {
        value(expr);
        message.clear();
        data.doCommand("echo g:e");
        return message;
    };
    data.doCommand("set noignorecase | let g:n = 7");

    QCOMPARE(value("eval('1+2')"), QLatin1String("3"));
    QCOMPARE(value("eval('&ignorecase')"), QLatin1String("0"));
    QCOMPARE(value("eval(\"'abc'\")"), QLatin1String("'abc'"));
    QCOMPARE(value("eval('[1,2]')"), QLatin1String("[1, 2]"));
    QCOMPARE(value("eval(\"{'a':1}\")"), QLatin1String("{'a': 1}"));
    QCOMPARE(value("eval('g:n')"), QLatin1String("7"));
    QCOMPARE(value("eval('2.5')"), QLatin1String("2.5"));
    // A number is taken as it stands.
    QCOMPARE(value("eval(42)"), QLatin1String("42"));
    // What string() wrote, eval() reads back.
    QCOMPARE(value("eval(string([1,'x']))"), QLatin1String("[1, 'x']"));
    // And what is no expression at all is an error a script can catch.
    QVERIFY(failure("eval('nosuchvar')").contains(QLatin1String("E121")));
    QVERIFY(!failure("eval('1+')").isEmpty());
    QVERIFY(!failure("eval('')").isEmpty());

    data.doCommand("unlet g:n | unlet g:v | unlet g:e");
}

void FakeVimTester::test_vim_set_invert()
{
    // ":set inv{option}" turns a boolean around, as "{option}!" does. Values
    // taken from Vim 9.1.
    TestData data;
    setup(&data);
    QString message;
    data.handler->commandBufferChanged.set(
        [&](const QString &msg, int, int, int) {
            if (!msg.startsWith("--"))
                message = msg;
        });
    const auto run = [&](const QString &command, const char *read) {
        data.doCommand("let g:e = ''");
        data.doCommand("try | " + command + " | catch | let g:e = v:exception | endtry");
        message.clear();
        data.doCommand(QLatin1String("echo &") + read + " . ' ' . g:e");
        return message;
    };
    data.doCommand("set noignorecase noexpandtab");

    QCOMPARE(run("set invignorecase", "ignorecase"), QLatin1String("1 "));
    QCOMPARE(run("set invignorecase", "ignorecase"), QLatin1String("0 "));
    QCOMPARE(run("set ignorecase!", "ignorecase"), QLatin1String("1 "));
    // Also by the short name.
    QCOMPARE(run("set invic", "ignorecase"), QLatin1String("0 "));
    QCOMPARE(run("set ic!", "ignorecase"), QLatin1String("1 "));
    QCOMPARE(run("set invexpandtab", "expandtab"), QLatin1String("1 "));
    // Only a boolean can be turned around, and "no" is not part of the name.
    QVERIFY(run("set invtabstop", "tabstop").contains(QLatin1String("E474")));
    QVERIFY(run("set tabstop!", "tabstop").contains(QLatin1String("E488")));
    QVERIFY(run("set invnoignorecase", "ignorecase").contains(QLatin1String("E518")));

    data.doCommand("set noignorecase noexpandtab | unlet g:e");
}

void FakeVimTester::test_vim_command_line_expression()
{
    // CTRL-R = in a command line asks for an expression and puts its value into
    // the line, which is how a plugin builds a command out of one. Values taken
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
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    QFile f(dir.path() + "/t.vim");
    QVERIFY(f.open(QIODevice::WriteOnly));
    f.write("function! Name(opt)\n"
            "  return 'inv' . a:opt\n"
            "endfunction\n");
    f.close();
    data.doCommand("source " + dir.path() + "/t.vim");
    data.doCommand("set noignorecase");

    data.setText(X "one two three");
    data.doKeys(":let g:a = <C-r>=1+2<CR><CR>");
    QCOMPARE(value("g:a"), QLatin1String("3"));
    data.doKeys(":let g:b = \"<C-r>=nr2char(65)<CR>\"<CR>");
    QCOMPARE(value("g:b"), QLatin1String("A"));
    // Escape gives up on the expression and leaves the line as it was.
    data.doKeys(":let g:c = 9<C-r>=2+2<Esc>9<CR>");
    QCOMPARE(value("g:c"), QLatin1String("99"));
    // Every item of a list is a line, and a command line holds one line.
    data.doKeys(":let g:e = \"<C-r>=[1,2]<CR>\"<CR>");
    QCOMPARE(value("g:e"), QLatin1String("1 2 "));
    // The shape a plugin turns an option around with.
    data.doKeys(":set <C-r>=Name('ignorecase')<CR><CR>");
    QCOMPARE(value("&ignorecase"), QLatin1String("1"));
    data.doKeys(":set <C-r>=Name('ignorecase')<CR><CR>");
    QCOMPARE(value("&ignorecase"), QLatin1String("0"));
    // And in a search line.
    data.setText(X "one two three");
    data.doKeys("/<C-r>=\"two\"<CR><CR>");
    QCOMPARE(value("col('.')"), QLatin1String("5"));

    // The same register from INSERT mode, which borrows the command buffer to
    // type the expression into rather than opening one of its own. The "="
    // shown in front is the mode's, so the command buffer's own ":" has to
    // survive the borrowing - g is shared between every handler, and nothing
    // put a ":" back, so one "i<C-r>=" used to leave "=" in front of every
    // command line for the rest of the session.
    data.setText(X "one");
    message.clear();
    data.doKeys("i<C-r>=");
    QCOMPARE(message, QLatin1String("="));
    data.doKeys("1+1");
    QCOMPARE(message, QLatin1String("=1+1"));
    data.doKeys("<CR>");
    QCOMPARE(data.text(), QByteArray("2one"));
    data.doKeys("<Esc>");
    // Back to an ordinary command line: the prompt is a colon again.
    message.clear();
    data.doKeys(":");
    QCOMPARE(message, QLatin1String(":"));
    data.doKeys("<Esc>");

    data.doCommand("unlet g:a | unlet g:b | unlet g:c | unlet g:e");
    data.doCommand("delfunction Name");
}

void FakeVimTester::test_vim9_unimpaired()
{
    // tpope's vim-unimpaired, the real plugin behind the "[q"/"]q" emulation.
    // Values taken from Vim 9.1 running it over the same lines. Only the
    // families that do not need a window, a buffer list or a quickfix list are
    // covered. Not installed with Vim, so the test is skipped without it.
    const QString D = qEnvironmentVariable("FAKEVIM_TEST_PLUGINS") + "/vim-unimpaired";
    if (!QFileInfo::exists(D + "/plugin/unimpaired.vim"))
        QSKIP("vim-unimpaired is not there; set FAKEVIM_TEST_PLUGINS to a checkout");
    TestData data;
    setup(&data);
    QString message;
    data.handler->commandBufferChanged.set(
        [&](const QString &msg, int, int, int) {
            if (!msg.startsWith("--"))
                message = msg;
        });
    data.doCommand("set runtimepath+=" + D);
    data.doCommand("source " + D + "/plugin/unimpaired.vim");

    const auto run = [&](const char *start, const char *keys) -> QString {
        data.setText(X "alpha" N "beta" N "gamma");
        data.doKeys(start);
        data.doKeys(keys);
        return QString::fromUtf8(data.text()).replace(QLatin1Char('\n'), QLatin1String(" / "));
    };
    // A blank line above and below, which needs ":put!" and ":put".
    QCOMPARE(run("j0", "[<Space>"), QLatin1String("alpha /  / beta / gamma"));
    QCOMPARE(run("j0", "]<Space>"), QLatin1String("alpha / beta /  / gamma"));
    // Exchanging a line with the one above or below it.
    QCOMPARE(run("j0", "[e"), QLatin1String("beta / alpha / gamma"));
    QCOMPARE(run("j0", "]e"), QLatin1String("alpha / gamma / beta"));
    // And with a count, which the mapping reads from v:count1.
    QCOMPARE(run("j0", "2[<Space>"), QLatin1String("alpha /  /  / beta / gamma"));
    QCOMPARE(run("0", "2]e"), QLatin1String("beta / gamma / alpha"));

    // The transform operators, which need a function named by a variable.
    const auto transform = [&](const char *lines, const char *start,
                               const char *keys) -> QString {
        data.setText(lines);
        data.doKeys(start);
        data.doKeys(keys);
        return QString::fromUtf8(data.text());
    };
    QCOMPARE(transform(X "a <b> c", "0ll", "[x$"), QLatin1String("a &lt;b&gt; c"));
    QCOMPARE(transform(X "a &lt;b&gt; c", "0ll", "]x$"), QLatin1String("a <b> c"));
    QCOMPARE(transform(X "a b&c", "0ll", "[u$"), QLatin1String("a b%26c"));
    QCOMPARE(transform(X "a b%26c", "0ll", "]u$"), QLatin1String("a b&c"));
    QCOMPARE(transform(X "a \"b\" c", "0", "[y$"), QLatin1String("a \\\"b\\\" c"));
    QCOMPARE(transform(X "a \\\"b\\\" c", "0", "]y$"), QLatin1String("a \"b\" c"));
    // The line variant of one of them.
    QCOMPARE(transform(X "a <b> c", "0", "[xx"), QLatin1String("a &lt;b&gt; c"));

    // Turning an option on, off and around.
    data.doCommand("set noignorecase");
    const auto option = [&](const char *keys) {
        data.setText(X "x");
        data.doKeys(keys);
        message.clear();
        data.doCommand("echo &ignorecase");
        return message;
    };
    QCOMPARE(option("yoi"), QLatin1String("1"));
    QCOMPARE(option("yoi"), QLatin1String("0"));
    QCOMPARE(option("[oi"), QLatin1String("1"));
    QCOMPARE(option("]oi"), QLatin1String("0"));

    // The mappings and the option live in a table shared with every other test.
    data.doCommand("nunmap [<Space> | nunmap ]<Space> | nunmap [e | nunmap ]e");
    data.doCommand("nunmap yoi | nunmap [oi | nunmap ]oi");
    data.doCommand("nunmap [x | nunmap ]x | nunmap [u | nunmap ]u"
                   " | nunmap [y | nunmap ]y | nunmap [xx | nunmap ]xx");
    data.doCommand("set opfunc=");
    data.doCommand("set noignorecase");
}

void FakeVimTester::test_vim_script_count_in_mapping()
{
    // The count typed in front of a mapping is what v:count answers inside it,
    // also when the mapping is a ":" command - which is how a plugin repeats
    // what it does. Values taken from Vim 9.1.
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
    QFile f(dir.path() + "/c.vim");
    QVERIFY(f.open(QIODevice::WriteOnly));
    f.write("let g:seen = ''\n"
            "function! Report(what)\n"
            "  let g:seen = a:what . ' ' . v:count . ' ' . v:count1\n"
            "endfunction\n"
            "function! Expr()\n"
            "  let g:seen = 'expr ' . v:count . ' ' . v:count1\n"
            "  return ''\n"
            "endfunction\n"
            "nnoremap QQ :<C-U>call Report('colon')<CR>\n"
            "nnoremap QP :<C-U>call Report('noCU')<CR>\n"
            "nnoremap <expr> QE Expr()\n");
    f.close();
    data.doCommand("source " + dir.path() + "/c.vim");
    const auto seen = [&](const char *keys) {
        data.setText(X "alpha" N "beta" N "gamma");
        data.doKeys(keys);
        message.clear();
        data.doCommand("echo g:seen");
        return message;
    };

    QCOMPARE(seen("QQ"), QLatin1String("colon 0 1"));
    QCOMPARE(seen("2QQ"), QLatin1String("colon 2 2"));
    QCOMPARE(seen("12QQ"), QLatin1String("colon 12 12"));
    // The count belongs to the command that was given it, not to the next one.
    QCOMPARE(seen("QQ"), QLatin1String("colon 0 1"));
    QCOMPARE(seen("4QP"), QLatin1String("noCU 4 4"));
    // An expression mapping is evaluated before the count is taken, and saw it
    // even before.
    QCOMPARE(seen("QE"), QLatin1String("expr 0 1"));
    QCOMPARE(seen("3QE"), QLatin1String("expr 3 3"));

    data.doCommand("nunmap QQ | nunmap QP | nunmap QE");
    data.doCommand("delfunction Report | delfunction Expr | unlet g:seen");
}

void FakeVimTester::test_vim_script_curly_name()
{
    // "{expr}" inside a name stands for what the expression says, so
    // "s:{algorithm}(x)" calls the one a variable names. Values taken from
    // Vim 9.1.
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
    QFile f(dir.path() + "/cb.vim");
    QVERIFY(f.open(QIODevice::WriteOnly));
    f.write("let g:out = []\n"
            "function! s:upper(s) abort\n"
            "  return toupper(a:s)\n"
            "endfunction\n"
            "let s:val = 5\n"
            "let s:dyn_x = 9\n"
            "let g:algo = 'upper'\n"
            "let g:part = 'zz'\n"
            "let g:k = 'x'\n"
            "function! Try(e)\n"
            "  try\n"
            "    return string(eval(a:e))\n"
            "  catch\n"
            "    return 'ERR ' . v:exception\n"
            "  endtry\n"
            "endfunction\n"
            "for e in [\"s:{g:algo}('ab')\", \"s:{'v'}{'al'}\", 's:dyn_{g:k}',"
            "          \"s:{g:part . 'q'}\", \"s:{'nosuchcurly'}('x')\"]\n"
            "  call add(g:out, Try(e))\n"
            "endfor\n");
    f.close();
    data.doCommand("source " + dir.path() + "/cb.vim");
    message.clear();
    data.doCommand("echo g:out[0] . ' | ' . g:out[1] . ' | ' . g:out[2]");
    QCOMPARE(message, QLatin1String("'AB' | 5 | 9"));
    // The name it spliced together is the one the error names. A name nothing
    // else uses, since the "s:" of every test is the same one.
    message.clear();
    data.doCommand("echo g:out[3]");
    QVERIFY(message.contains(QLatin1String("E121")));
    QVERIFY(message.contains(QLatin1String("s:zzq")));
    message.clear();
    data.doCommand("echo g:out[4]");
    QVERIFY(message.contains(QLatin1String("E117")));
    QVERIFY(message.contains(QLatin1String("s:nosuchcurly")));

    // The same without a scope in front of it, where a "{" would otherwise open
    // a dictionary. What tells them apart is the "(" behind the braces.
    const auto value = [&](const QString &expr) {
        message.clear();
        data.doCommand("echo " + expr);
        return message;
    };
    data.doCommand("let g:fn = 'toupper'");
    QCOMPARE(value("{g:fn}('ab')"), QLatin1String("AB"));
    QCOMPARE(value("{'toupper'}('ab')"), QLatin1String("AB"));
    // A dictionary is still a dictionary, and a lambda called on the spot still
    // a lambda.
    QCOMPARE(value("{'a': 1}"), QLatin1String("{'a': 1}"));
    QCOMPARE(value("{}"), QLatin1String("{}"));
    QCOMPARE(value("{x -> x * 2}(3)"), QLatin1String("6"));
    data.doCommand("unlet g:fn");
    data.doCommand("delfunction Try | unlet g:out | unlet g:algo | unlet g:part | unlet g:k");
}

void FakeVimTester::test_vim_script_tr()
{
    // tr() puts the character at the same place of the third argument in place
    // of the one from the second. Values taken from Vim 9.1.
    TestData data;
    setup(&data);
    QString message;
    data.handler->commandBufferChanged.set(
        [&](const QString &msg, int, int, int) {
            if (!msg.startsWith("--"))
                message = msg;
        });
    const auto value = [&](const QString &expr) {
        data.doCommand("let g:e = ''");
        data.doCommand("try | let g:v = " + expr
                       + " | catch | let g:e = v:exception | let g:v = 'FAILED' | endtry");
        message.clear();
        data.doCommand("echo g:v . '  ' . g:e");
        return message;
    };

    QCOMPARE(value("tr('foo_bar', '_', '-')"), QLatin1String("foo-bar  "));
    QCOMPARE(value("tr('abc', 'abc', 'xyz')"), QLatin1String("xyz  "));
    QCOMPARE(value("tr('hello', 'l', 'L')"), QLatin1String("heLLo  "));
    QCOMPARE(value("tr('aabb', 'ab', 'ba')"), QLatin1String("bbaa  "));
    // A character with no counterpart is only an error where it stands.
    QCOMPARE(value("tr('a', 'ab', 'x')"), QLatin1String("x  "));
    QCOMPARE(value("tr('', 'a', 'b')"), QLatin1String("  "));
    QCOMPARE(value("tr('x', '', '')"), QLatin1String("x  "));
    QCOMPARE(value("tr(123, '2', '9')"), QLatin1String("193  "));
    QVERIFY(value("tr('abc', 'a', '')").contains(QLatin1String("E475")));
    data.doCommand("unlet g:v | unlet g:e");
}

void FakeVimTester::test_vim_script_string_as_number()
{
    // Where a number is wanted, a string is read as str2nr() takes it: "0x" for
    // hex, "0b" for binary, "0o" or a leading zero for octal. A blank in front
    // means there is no number at all, and a "+" is not a sign. Values taken
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

    QCOMPARE(value("'0x26' + 0"), QLatin1String("38"));
    QCOMPARE(value("'0X1f' + 0"), QLatin1String("31"));
    QCOMPARE(value("'0b101' + 0"), QLatin1String("5"));
    QCOMPARE(value("'0o17' + 0"), QLatin1String("15"));
    QCOMPARE(value("'017' + 0"), QLatin1String("15"));
    QCOMPARE(value("'-017' + 0"), QLatin1String("-15"));
    QCOMPARE(value("'10' + 0"), QLatin1String("10"));
    QCOMPARE(value("'-0x10' + 0"), QLatin1String("-16"));
    QCOMPARE(value("'12abc' + 0"), QLatin1String("12"));
    // A leading zero is octal only where every digit could be one.
    QCOMPARE(value("'08' + 0"), QLatin1String("8"));
    QCOMPARE(value("'0779' + 0"), QLatin1String("779"));
    // Nothing to read gives 0.
    QCOMPARE(value("'0xzz' + 0"), QLatin1String("0"));
    QCOMPARE(value("'0x' + 0"), QLatin1String("0"));
    QCOMPARE(value("'0o8' + 0"), QLatin1String("0"));
    QCOMPARE(value("'x' + 0"), QLatin1String("0"));
    QCOMPARE(value("'' + 0"), QLatin1String("0"));
    // A blank in front, and a "+", stop it before it starts.
    QCOMPARE(value("' 10' + 0"), QLatin1String("0"));
    QCOMPARE(value("' 0x10' + 0"), QLatin1String("0"));
    QCOMPARE(value("'+10' + 0"), QLatin1String("0"));
    // Which is how a character is named by its number in hex.
    QCOMPARE(value("nr2char('0x26')"), QLatin1String("&"));
    QCOMPARE(value("repeat('a', '0x3')"), QLatin1String("aaa"));
    // str2nr() takes a base of its own, and 10 by default.
    QCOMPARE(value("str2nr('0x26')"), QLatin1String("0"));
    QCOMPARE(value("str2nr('0x26', 16)"), QLatin1String("38"));
}

void FakeVimTester::test_vim_script_maparg()
{
    // maparg() takes the keys written in "<>" notation as well as typed out, and
    // gives back the right-hand side the way it was written. A plugin asks that
    // way before putting a mapping of its own in. Values taken from Vim 9.1.
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
    data.doCommand("nnoremap [<Space> iX<Esc>");
    data.doCommand("nnoremap <Space>z iZ<Esc>");
    data.doCommand("nnoremap QQ :call Foo()<CR>");
    data.doCommand("nnoremap QT <Plug>Something");
    data.doCommand("nnoremap <C-x>y iC<Esc>");
    data.doCommand("nnoremap <Tab>t iT<Esc>");
    data.doCommand("nnoremap QL <lt>lt");
    data.doCommand("inoremap <C-l> done");

    // Both spellings of the keys find the same mapping.
    QCOMPARE(value("maparg('[<Space>', 'n')"), QLatin1String("iX<Esc>"));
    QCOMPARE(value("maparg('[ ', 'n')"), QLatin1String("iX<Esc>"));
    QCOMPARE(value("maparg('<Space>z', 'n')"), QLatin1String("iZ<Esc>"));
    QCOMPARE(value("maparg(' z', 'n')"), QLatin1String("iZ<Esc>"));
    QCOMPARE(value("maparg('<C-x>y', 'n')"), QLatin1String("iC<Esc>"));
    QCOMPARE(value("maparg('<Tab>t', 'n')"), QLatin1String("iT<Esc>"));
    // An answer without such a key comes back exactly as written, and a "<"
    // that stands for itself stays one.
    QCOMPARE(value("maparg('QQ', 'n')"), QLatin1String(":call Foo()<CR>"));
    QCOMPARE(value("maparg('QT', 'n')"), QLatin1String("<Plug>Something"));
    QCOMPARE(value("maparg('QL', 'n')"), QLatin1String("<lt"));
    QCOMPARE(value("maparg('<C-l>', 'i')"), QLatin1String("done"));
    QCOMPARE(value("maparg('nosuch', 'n')"), QString());
    // Which is what lets a plugin leave a mapping of the user's alone.
    QCOMPARE(value("empty(maparg('[<Space>', 'n'))"), QLatin1String("0"));
    QCOMPARE(value("empty(maparg('[x', 'n'))"), QLatin1String("1"));
    QCOMPARE(value("hasmapto('<Plug>Something', 'n')"), QLatin1String("1"));

    // A "<Plug>" a plugin writes out in its own right-hand side is text, not
    // the key, so hasmapto() does not count it: that is how a plugin names
    // itself there for repeat.vim and still asks whether the USER has a mapping
    // to it. Vim answers 0 and 1 the same way.
    data.doCommand("nnoremap <silent> <Plug>Foo :call X(\"<lt>Plug>Foo\")<CR>");
    QCOMPARE(value("hasmapto('<Plug>Foo', 'n')"), QLatin1String("0"));
    data.doCommand("nnoremap <silent> <Plug>Bar :call X('plain')<CR>");
    QCOMPARE(value("hasmapto('<Plug>Bar', 'n')"), QLatin1String("0"));
    data.doCommand("nmap gb <Plug>Bar");
    QCOMPARE(value("hasmapto('<Plug>Bar', 'n')"), QLatin1String("1"));
    // A plain mapping that mentions one does count.
    data.doCommand("nnoremap ZZ1 :echo \"<Plug>Zed\"<CR>");
    QCOMPARE(value("hasmapto('<Plug>Zed', 'n')"), QLatin1String("1"));

    // mapcheck() answers for the keys themselves, for a mapping they are the
    // START of, and for a mapping that is the start of THEM - which is how a
    // plugin asks whether adding a mapping would be ambiguous. maparg() only
    // ever answers for the keys exactly. Values taken from Vim 9.1, which
    // gives this table for a lone "ab": mapcheck("a") and mapcheck("abc")
    // find it, mapcheck("ax") does not.
    // The mapping table is shared with every other test, and a PREFIX match
    // reaches their mappings too - a single-key "Z" left behind by another
    // slot answers for "Zqb", since they share their first key. So these use
    // a corner nothing else maps (no test uses an F-key on the left), and
    // check it really is empty first: any answer after that is this
    // mapping's own rather than a stranger's.
    QCOMPARE(value("mapcheck('<F5>', 'n')"), QString());
    QCOMPARE(value("mapcheck('<F5>b', 'n')"), QString());
    QCOMPARE(value("mapcheck('<F5>bc', 'n')"), QString());
    QCOMPARE(value("mapcheck('<F6>', 'n')"), QString());
    data.doCommand("nnoremap <F5>b :echo 'rhs-F5b'<CR>");
    const QString f5b = ":echo 'rhs-F5b'<CR>";
    QCOMPARE(value("mapcheck('<F5>b', 'n')"), f5b);
    QCOMPARE(value("mapcheck('<F5>', 'n')"), f5b);
    QCOMPARE(value("mapcheck('<F5>bc', 'n')"), f5b);
    QCOMPARE(value("mapcheck('<F5>x', 'n')"), QString());
    QCOMPARE(value("mapcheck('<F6>', 'n')"), QString());
    // Where maparg() wants the whole of them.
    QCOMPARE(value("maparg('<F5>', 'n')"), QString());
    QCOMPARE(value("maparg('<F5>bc', 'n')"), QString());
    QCOMPARE(value("maparg('<F5>b', 'n')"), f5b);
    data.doCommand("nunmap <F5>b");
    // A mapping to <Nop> is still a mapping, so it has to be written out as
    // something rather than reading as absent.
    data.doCommand("nnoremap <F5>n <Nop>");
    QCOMPARE(value("mapcheck('<F5>n', 'n')"), QLatin1String("<Nop>"));
    QCOMPARE(value("maparg('<F5>n', 'n')"), QLatin1String("<Nop>"));
    data.doCommand("nunmap <F5>n");
    QCOMPARE(value("mapcheck('<F5>', 'n')"), QString());

    // The mappings live in a table shared with every other test.
    data.doCommand("nunmap gb | nunmap ZZ1 | nmapclear <Plug>Foo | nmapclear <Plug>Bar");
    data.doCommand("nunmap [<Space> | nunmap <Space>z | nunmap QQ | nunmap QT");
    data.doCommand("nunmap <C-x>y | nunmap <Tab>t | nunmap QL | iunmap <C-l>");
}

void FakeVimTester::test_vim_script_maplist()
{
    // maplist() is one dict per mapping, across every mode at once, and its
    // dict is the same one maparg(keys, mode, 0, 1) answers with - measured
    // equal in Vim 9.1, so they are built in one place here.
    // The mapping table is shared with every other test slot, so this asks
    // only about its own corner (no test uses an F-key on the left) rather
    // than counting what maplist() returns in total.
    TestData data;
    setup(&data);
    QString message;
    data.handler->commandBufferChanged.set(
        [&](const QString &msg, int, int, int) {
            if (!msg.isEmpty() && !msg.startsWith("--"))
                message = msg;
        });
    const auto value = [&](const QString &expr) {
        message.clear();
        data.doCommand("echo " + expr);
        return message;
    };
    const QString mine = "filter(maplist(), {_, m -> m.lhs =~ '^<F5>'})";

    // Nothing of its own to begin with.
    QCOMPARE(value("len(" + mine + ")"), QLatin1String("0"));

    data.doCommand("nnoremap <F5>a :echo 1<CR>");
    data.doCommand("cnoremap <F5>c cc");
    data.doCommand("inoremap <F5>i ii");
    QCOMPARE(value("len(" + mine + ")"), QLatin1String("3"));
    // Every mode it was mapped in comes back, each with its own mode letter.
    QCOMPARE(value("sort(map(" + mine + ", {_, m -> m.mode .. m.lhs .. '=' .. m.rhs}))"),
             QLatin1String("['c<F5>c=cc', 'i<F5>i=ii', 'n<F5>a=:echo 1<CR>']"));
    // The dict is maparg()'s dict.
    QCOMPARE(value("sort(keys(" + mine + "[0])) == sort(keys(maparg('<F5>a', 'n', 0, 1)))"),
             QLatin1String("1"));
    // The flags a mapping carries come through it.
    data.doCommand("nnoremap <silent> <F5>s :echo 2<CR>");
    data.doCommand("nmap <F5>r <F5>a");
    const QString one = "filter(maplist(), {_, m -> m.lhs == '<F5>s'})[0]";
    QCOMPARE(value(one + ".silent"), QLatin1String("1"));
    QCOMPARE(value(one + ".noremap"), QLatin1String("1"));
    const QString remapped = "filter(maplist(), {_, m -> m.lhs == '<F5>r'})[0]";
    QCOMPARE(value(remapped + ".noremap"), QLatin1String("0"));

    data.doCommand("nunmap <F5>a | cunmap <F5>c | iunmap <F5>i");
    data.doCommand("nunmap <F5>s | nunmap <F5>r");
    QCOMPARE(value("len(" + mine + ")"), QLatin1String("0"));

    // -- mapset() --
    // Round trip: what maparg() answered goes back in unchanged, flags and
    // all. The dict form takes the mode from the dict itself.
    data.doCommand("nnoremap <silent> <F5>a :echo 3<CR>");
    data.doCommand("let g:saved = maparg('<F5>a', 'n', 0, 1)");
    data.doCommand("nunmap <F5>a");
    QCOMPARE(value("maparg('<F5>a', 'n')"), QString());
    QCOMPARE(value("mapset(g:saved)"), QLatin1String("0"));
    QCOMPARE(value("maparg('<F5>a', 'n')"), QLatin1String(":echo 3<CR>"));
    QCOMPARE(value("maparg('<F5>a', 'n', 0, 1).silent"), QLatin1String("1"));
    QCOMPARE(value("maparg('<F5>a', 'n', 0, 1).noremap"), QLatin1String("1"));
    data.doCommand("nunmap <F5>a");

    // The three-argument form says which table to put it in, and that mode
    // WINS over the one the dict carries: this dict came from normal mode,
    // and it lands in insert mode. Measured against Vim 9.1.
    QCOMPARE(value("g:saved.mode"), QLatin1String("n"));
    QCOMPARE(value("mapset('i', 0, g:saved)"), QLatin1String("0"));
    QCOMPARE(value("maparg('<F5>a', 'n')"), QString());
    QCOMPARE(value("maparg('<F5>a', 'i')"), QLatin1String(":echo 3<CR>"));
    data.doCommand("iunmap <F5>a");

    // An <expr> mapping keeps being one across the round trip.
    data.doCommand("nnoremap <expr> <F5>e 'iX'");
    data.doCommand("let g:e = maparg('<F5>e', 'n', 0, 1)");
    data.doCommand("nunmap <F5>e");
    QCOMPARE(value("mapset(g:e)"), QLatin1String("0"));
    QCOMPARE(value("maparg('<F5>e', 'n', 0, 1).expr"), QLatin1String("1"));
    QCOMPARE(value("maparg('<F5>e', 'n')"), QLatin1String("'iX'"));
    data.doCommand("nunmap <F5>e");
    data.doCommand("unlet g:saved | unlet g:e");
    QCOMPARE(value("len(" + mine + ")"), QLatin1String("0"));
}

void FakeVimTester::test_vim_script_charsearch()
{
    // getcharsearch() reports the last "f"/"F"/"t"/"T" and what it looked
    // for - the state ";" and "," already repeat. "forward" tells f/t from
    // F/T, "until" tells t/T from f/F. setcharsearch() MERGES: only the
    // entries it is given change. Values taken from Vim 9.1.
    TestData data;
    setup(&data);
    QString message;
    data.handler->commandBufferChanged.set(
        [&](const QString &msg, int, int, int) {
            if (!msg.isEmpty() && !msg.startsWith("--"))
                message = msg;
        });
    const auto value = [&](const QString &expr) {
        message.clear();
        data.doCommand("echo " + expr);
        return message;
    };
    // Columns are 1-based: a1 x2 b3 x4 c5 x6 d7 x8.
    data.setText(X "axbxcxdx");

    // Only the key SET is compared as a whole: dict key order is an engine
    // detail here (QMap sorts them) where Vim's string() keeps the order they
    // went in, so the entries are looked up one at a time.
    data.doKeys("gg0fx");
    QCOMPARE(value("sort(keys(getcharsearch()))"),
             QLatin1String("['char', 'forward', 'until']"));
    struct { const char *keys; const char *ch; const char *forward; const char *until; }
    steps[] = {
        {"gg0fx", "x", "1", "0"},
        {"gg0$Fa", "a", "0", "0"},
        {"gg0tb", "b", "1", "1"},
        {"gg0$Tc", "c", "0", "1"},
    };
    for (const auto &step : steps) {
        data.doKeys(step.keys);
        QCOMPARE(value("getcharsearch()['char']"), QLatin1String(step.ch));
        QCOMPARE(value("getcharsearch()['forward']"), QLatin1String(step.forward));
        QCOMPARE(value("getcharsearch()['until']"), QLatin1String(step.until));
    }

    // setcharsearch() replaces what ";" will repeat, and answers zero.
    QCOMPARE(value("setcharsearch({'char': 'd', 'forward': 1, 'until': 0})"),
             QLatin1String("0"));
    QCOMPARE(value("getcharsearch()['char']"), QLatin1String("d"));
    QCOMPARE(value("getcharsearch()['forward']"), QLatin1String("1"));
    QCOMPARE(value("getcharsearch()['until']"), QLatin1String("0"));
    data.doKeys("gg0;");
    QCOMPARE(value("col('.')"), QLatin1String("7"));

    // Given only one entry, the others stay as they were.
    QCOMPARE(value("setcharsearch({'char': 'b'})"), QLatin1String("0"));
    QCOMPARE(value("getcharsearch()['char']"), QLatin1String("b"));
    QCOMPARE(value("getcharsearch()['forward']"), QLatin1String("1"));
    QCOMPARE(value("getcharsearch()['until']"), QLatin1String("0"));
    // And that is what ";" now looks for.
    data.doKeys("gg0;");
    QCOMPARE(value("col('.')"), QLatin1String("3"));
    // "until" on its own turns the same character search into a "t".
    QCOMPARE(value("setcharsearch({'until': 1})"), QLatin1String("0"));
    QCOMPARE(value("getcharsearch()['char']"), QLatin1String("b"));
    QCOMPARE(value("getcharsearch()['forward']"), QLatin1String("1"));
    QCOMPARE(value("getcharsearch()['until']"), QLatin1String("1"));
    data.doKeys("gg0;");
    QCOMPARE(value("col('.')"), QLatin1String("2"));

    QCOMPARE(value("exists('*getcharsearch') .. exists('*setcharsearch')"),
             QLatin1String("11"));
}

void FakeVimTester::test_vim_script_winvar_tabvar()
{
    // The w: and t: scoped variables through the accessors a plugin uses, the
    // same shape getbufvar()/setbufvar() already has for b:. One window and
    // one tab page, so any other number reads as though the variable were not
    // there - the default if one was offered, empty otherwise, which is what
    // Vim does rather than raising. Values taken from Vim 9.1.
    TestData data;
    setup(&data);
    QString message;
    data.handler->commandBufferChanged.set(
        [&](const QString &msg, int, int, int) {
            if (!msg.isEmpty() && !msg.startsWith("--"))
                message = msg;
        });
    const auto value = [&](const QString &expr) {
        message.clear();
        data.doCommand("echo " + expr);
        return message;
    };

    data.doCommand("let w:mine = 'W'");
    data.doCommand("let t:mine = 'T'");
    // A zero is the current window, and there is only window 1.
    QCOMPARE(value("getwinvar(0, 'mine')"), QLatin1String("W"));
    QCOMPARE(value("getwinvar(1, 'mine')"), QLatin1String("W"));
    QCOMPARE(value("gettabvar(1, 'mine')"), QLatin1String("T"));
    QCOMPARE(value("gettabwinvar(1, 0, 'mine')"), QLatin1String("W"));

    // Not there, and no window or tab page of that number, read the same way.
    QCOMPARE(value("getwinvar(0, 'nope')"), QString());
    QCOMPARE(value("getwinvar(0, 'nope', 'DEF')"), QLatin1String("DEF"));
    QCOMPARE(value("getwinvar(9, 'mine')"), QString());
    QCOMPARE(value("getwinvar(9, 'nope', 'DEF')"), QLatin1String("DEF"));
    QCOMPARE(value("gettabvar(9, 'mine')"), QString());
    QCOMPARE(value("gettabwinvar(1, 9, 'mine')"), QString());

    // The setters answer zero and put the variable in its own scope.
    QCOMPARE(value("setwinvar(0, 'fresh', 'NEW')"), QLatin1String("0"));
    QCOMPARE(value("w:fresh"), QLatin1String("NEW"));
    QCOMPARE(value("settabvar(1, 'freshT', 'NEWT')"), QLatin1String("0"));
    QCOMPARE(value("t:freshT"), QLatin1String("NEWT"));
    QCOMPARE(value("settabwinvar(1, 0, 'freshW', 'NEWW')"), QLatin1String("0"));
    QCOMPARE(value("w:freshW"), QLatin1String("NEWW"));
    // A window or tab page that is not there is not written to.
    QCOMPARE(value("setwinvar(9, 'ghost', 'X')"), QLatin1String("0"));
    QCOMPARE(value("exists('w:ghost')"), QLatin1String("0"));

    // An "&name" reaches the option rather than a variable. Vim takes a
    // global or buffer-local option here too, not only a window-local one -
    // measured, so "ignorecase" is a fair thing to ask about.
    data.doCommand("set ignorecase");
    QCOMPARE(value("getwinvar(0, '&ignorecase')"), QLatin1String("1"));
    QCOMPARE(value("gettabwinvar(1, 0, '&ignorecase')"), QLatin1String("1"));
    QCOMPARE(value("setwinvar(0, '&ignorecase', 0)"), QLatin1String("0"));
    QCOMPARE(value("&ignorecase"), QLatin1String("0"));
    data.doCommand("set shiftwidth=7");
    QCOMPARE(value("getwinvar(0, '&shiftwidth')"), QLatin1String("7"));
    // A window that is not there gives nothing, option or not.
    QCOMPARE(value("getwinvar(9, '&ignorecase')"), QString());
    data.doCommand("set noignorecase | set shiftwidth=8");

    data.doCommand("unlet w:mine | unlet t:mine | unlet w:fresh");
    data.doCommand("unlet t:freshT | unlet w:freshW");
}

void FakeVimTester::test_vim_script_searchcount()
{
    // Where the cursor stands among the matches of the last search - what a
    // status line shows as "[3/12]". Dict key order is an engine detail here
    // (QMap sorts them), so the entries are looked up one at a time.
    // Values taken from Vim 9.1 over the same five lines.
    TestData data;
    setup(&data);
    QString message;
    data.handler->commandBufferChanged.set(
        [&](const QString &msg, int, int, int) {
            if (!msg.isEmpty() && !msg.startsWith("--"))
                message = msg;
        });
    const auto value = [&](const QString &expr) {
        message.clear();
        data.doCommand("echo " + expr);
        return message;
    };
    // "foo" stands at (1,1), (3,1) and (4,5).
    data.setText(X "foo one" N "bar two" N "foo three" N "baz foo" N "last");
    data.doCommand("let @/ = 'foo'");

    QCOMPARE(value("sort(keys(searchcount()))"),
             QLatin1String("['current', 'exact_match', 'incomplete', "
                            "'maxcount', 'total']"));
    // "current" is the match at or before the cursor; "exact_match" says the
    // cursor is on one. Vim's 'maxsearchcount' default is 99.
    struct { const char *keys; const char *current; const char *exact; } spots[] = {
        {"gg0", "1", "1"},   // on the first match
        {"2G0", "1", "0"},   // between matches
        {"3G0", "2", "1"},   // on the second
        {"G$", "3", "0"},    // past the last
    };
    for (const auto &spot : spots) {
        data.doKeys(spot.keys);
        QCOMPARE(value("searchcount()['current']"), QLatin1String(spot.current));
        QCOMPARE(value("searchcount()['exact_match']"), QLatin1String(spot.exact));
        QCOMPARE(value("searchcount()['total']"), QLatin1String("3"));
        QCOMPARE(value("searchcount()['incomplete']"), QLatin1String("0"));
        QCOMPARE(value("searchcount()['maxcount']"), QLatin1String("99"));
    }

    // "maxcount" stops the count one PAST the limit, which is what says there
    // were more than that rather than exactly that many - so the total reads
    // maxcount + 1 and "incomplete" is 2. A limit the total fits inside is
    // not cut short.
    data.doKeys("G$");
    struct { const char *max; const char *total; const char *incomplete;
             const char *current; } limits[] = {
        {"1", "2", "2", "2"},
        {"2", "3", "2", "3"},
        {"3", "3", "0", "3"},
        {"4", "3", "0", "3"},
        {"0", "3", "0", "3"},   // 0 is no limit at all
    };
    for (const auto &limit : limits) {
        const QString call = QString("searchcount({'maxcount': %1})").arg(QLatin1String(limit.max));
        QCOMPARE(value(call + "['total']"), QLatin1String(limit.total));
        QCOMPARE(value(call + "['incomplete']"), QLatin1String(limit.incomplete));
        QCOMPARE(value(call + "['current']"), QLatin1String(limit.current));
        QCOMPARE(value(call + "['maxcount']"), QLatin1String(limit.max));
    }

    // "pattern" counts something else without becoming the last search.
    data.doKeys("gg0");
    QCOMPARE(value("searchcount({'pattern': 'bar'})['total']"), QLatin1String("1"));
    QCOMPARE(value("searchcount({'pattern': 'bar'})['current']"), QLatin1String("0"));
    QCOMPARE(value("@/"), QLatin1String("foo"));
    // "pos" counts as if the cursor were there instead.
    QCOMPARE(value("searchcount({'pos': [3, 1, 0]})['current']"), QLatin1String("2"));
    QCOMPARE(value("searchcount({'pos': [3, 1, 0]})['exact_match']"),
             QLatin1String("1"));

    // Nothing searched for yet answers with nothing at all.
    data.doCommand("let @/ = ''");
    QCOMPARE(value("searchcount()"), QLatin1String("{}"));
    QCOMPARE(value("empty(searchcount())"), QLatin1String("1"));
}

void FakeVimTester::test_vim_script_environment()
{
    // The process environment, which is what "$NAME" in an expression already
    // reads, plus where an executable stands along PATH. Nothing here asserts
    // a value the machine happens to have - only the shape of the answers -
    // except through a variable this test sets itself. Values taken from Vim
    // 9.1.
    TestData data;
    setup(&data);
    QString message;
    data.handler->commandBufferChanged.set(
        [&](const QString &msg, int, int, int) {
            if (!msg.isEmpty() && !msg.startsWith("--"))
                message = msg;
        });
    const auto value = [&](const QString &expr) {
        message.clear();
        data.doCommand("echo " + expr);
        return message;
    };

    // One that is not there is v:null, NOT an empty string - that is what
    // tells it apart from one set to nothing.
    QCOMPARE(value("getenv('FV_NOSUCH_XYZ')"), QLatin1String("v:null"));
    QCOMPARE(value("type(getenv('FV_NOSUCH_XYZ')) == type(v:null)"),
             QLatin1String("1"));

    // Setting one answers zero, and "$NAME" sees it too.
    QCOMPARE(value("setenv('FV_TEST_VAR', 'hello')"), QLatin1String("0"));
    QCOMPARE(value("getenv('FV_TEST_VAR')"), QLatin1String("hello"));
    QCOMPARE(value("$FV_TEST_VAR"), QLatin1String("hello"));
    QCOMPARE(value("environ()['FV_TEST_VAR']"), QLatin1String("hello"));
    QCOMPARE(value("has_key(environ(), 'FV_TEST_VAR')"), QLatin1String("1"));

    // An empty one is still a value, where a missing one is not.
    QCOMPARE(value("setenv('FV_TEST_VAR', '')"), QLatin1String("0"));
    QCOMPARE(value("getenv('FV_TEST_VAR')"), QString());
    QCOMPARE(value("type(getenv('FV_TEST_VAR')) == type('')"), QLatin1String("1"));

    // v:null takes it away again.
    QCOMPARE(value("setenv('FV_TEST_VAR', v:null)"), QLatin1String("0"));
    QCOMPARE(value("getenv('FV_TEST_VAR')"), QLatin1String("v:null"));
    QCOMPARE(value("has_key(environ(), 'FV_TEST_VAR')"), QLatin1String("0"));

    // environ() is a dictionary of them all.
    QCOMPARE(value("type(environ()) == type({})"), QLatin1String("1"));

    // exepath() gives the whole path where executable() gives a yes, and
    // nothing at all where it gives a no.
    QCOMPARE(value("empty(exepath('sh')) == !executable('sh')"),
             QLatin1String("1"));
    QCOMPARE(value("exepath('sh')[0]"), QLatin1String("/"));
    QCOMPARE(value("exepath('fv_nosuchbinary_xyz')"), QString());
    QCOMPARE(value("executable('fv_nosuchbinary_xyz')"), QLatin1String("0"));
}

void FakeVimTester::test_vim_script_getbufinfo()
{
    // What getwininfo() is for windows, for buffers. Values taken from Vim
    // 9.1. "lastused" is a timestamp and "changedtick" an absolute revision,
    // so neither is written down here - only that they are there, and that
    // the tick moves when the buffer changes. Dict key order is an engine
    // detail (QMap sorts them), so entries are looked up one at a time.
    TestData data;
    setup(&data);
    QString message;
    data.handler->commandBufferChanged.set(
        [&](const QString &msg, int, int, int) {
            if (!msg.isEmpty() && !msg.startsWith("--"))
                message = msg;
        });
    const auto value = [&](const QString &expr) {
        message.clear();
        data.doCommand("echo " + expr);
        return message;
    };
    data.setText(X "one" N "two" N "three" N "four" N "five");

    QCOMPARE(value("len(getbufinfo())"), QLatin1String("1"));
    QCOMPARE(value("sort(keys(getbufinfo()[0]))"),
             QLatin1String("['bufnr', 'changed', 'changedtick', 'command', "
                            "'hidden', 'lastused', 'linecount', 'listed', "
                            "'lnum', 'loaded', 'name', 'popups', 'variables', "
                            "'windows']"));
    // The one buffer is the one on show: listed, loaded, not hidden.
    const QString one = "getbufinfo()[0]";
    QCOMPARE(value(one + ".listed"), QLatin1String("1"));
    QCOMPARE(value(one + ".loaded"), QLatin1String("1"));
    QCOMPARE(value(one + ".hidden"), QLatin1String("0"));
    QCOMPARE(value(one + ".command"), QLatin1String("0"));
    QCOMPARE(value(one + ".linecount"), QLatin1String("5"));
    QCOMPARE(value(one + ".popups"), QLatin1String("[]"));
    // The numbers are read live: both counters are shared across the run.
    QCOMPARE(value(one + ".bufnr"), value("bufnr('%')"));
    QCOMPARE(value("string(" + one + ".windows)"), "[" + value("win_getid()") + "]");
    QCOMPARE(value(one + ".name"), value("bufname('%')"));

    // "lnum" follows the cursor.
    data.doKeys("3G");
    QCOMPARE(value(one + ".lnum"), QLatin1String("3"));

    // Editing the buffer moves the tick. "changed" is NOT checked against a
    // clean buffer: setText() marks the document modified itself, there is no
    // 'modified' option here to put it back, and no way for a test to reach
    // the flag - so the unmodified side is out of reach rather than asserted
    // from the harness's own doing. The tick moving is the real check.
    const QString before = value(one + ".changedtick");
    data.doKeys("x");
    QCOMPARE(value(one + ".changed"), QLatin1String("1"));
    QCOMPARE(value(one + ".changedtick > " + before), QLatin1String("1"));

    // The b: scope comes through, changedtick with it as in Vim.
    data.doCommand("let b:mine = 'B'");
    QCOMPARE(value(one + ".variables.mine"), QLatin1String("B"));
    QCOMPARE(value("has_key(" + one + ".variables, 'changedtick')"),
             QLatin1String("1"));

    // Named by number or name, and nothing for one that is not there.
    QCOMPARE(value("len(getbufinfo(bufnr('%')))"), QLatin1String("1"));
    QCOMPARE(value("len(getbufinfo('%'))"), QLatin1String("1"));
    QCOMPARE(value("len(getbufinfo(999))"), QLatin1String("0"));

    // A filter set to zero is OFF rather than inverted, so it still answers.
    QCOMPARE(value("len(getbufinfo({'buflisted': 1}))"), QLatin1String("1"));
    QCOMPARE(value("len(getbufinfo({'buflisted': 0}))"), QLatin1String("1"));
    QCOMPARE(value("len(getbufinfo({'bufloaded': 1}))"), QLatin1String("1"));
    // This buffer counts as modified, so it passes "bufmodified" as well. The
    // case where that filter leaves it OUT cannot be reached here, for the
    // same reason "changed" above is not checked against a clean buffer.
    QCOMPARE(value("len(getbufinfo({'bufmodified': 1}))"), QLatin1String("1"));

    data.doCommand("unlet b:mine");
}

void FakeVimTester::test_vim_script_file_info()
{
    // What the filesystem says about a path. The fixture is built here rather
    // than assumed, so nothing depends on what this machine happens to hold.
    // Values taken from Vim 9.1 over the same fixture.
    TestData data;
    setup(&data);
    QString message;
    data.handler->commandBufferChanged.set(
        [&](const QString &msg, int, int, int) {
            if (!msg.isEmpty() && !msg.startsWith("--"))
                message = msg;
        });
    const auto value = [&](const QString &expr) {
        message.clear();
        data.doCommand("echo " + expr);
        return message;
    };

    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString root = dir.path();
    QFile plain(root + "/plain.txt");
    QVERIFY(plain.open(QIODevice::WriteOnly));
    plain.write("twelve bytes");           // twelve of them
    plain.close();
    QVERIFY(QFile::setPermissions(root + "/plain.txt",
                                  QFile::ReadOwner | QFile::WriteOwner
                                  | QFile::ReadGroup | QFile::ReadOther));
    QVERIFY(QDir(root).mkdir("sub"));
    QVERIFY(QFile::link("plain.txt", root + "/link.txt"));
    data.doCommand("let g:d = '" + root + "'");

    // -- getftype() --
    // A link is reported as one, not as what it points at.
    QCOMPARE(value("getftype(g:d . '/plain.txt')"), QLatin1String("file"));
    QCOMPARE(value("getftype(g:d . '/sub')"), QLatin1String("dir"));
    QCOMPARE(value("getftype(g:d . '/link.txt')"), QLatin1String("link"));
    QCOMPARE(value("getftype(g:d . '/nope')"), QString());

    // -- getfperm() --
    // Nine characters, no leading type character.
    QCOMPARE(value("getfperm(g:d . '/plain.txt')"), QLatin1String("rw-r--r--"));
    QCOMPARE(value("len(getfperm(g:d . '/sub'))"), QLatin1String("9"));
    QCOMPARE(value("getfperm(g:d . '/nope')"), QString());

    // -- getfsize() --
    // A directory answers zero, and what is not there answers -1.
    QCOMPARE(value("getfsize(g:d . '/plain.txt')"), QLatin1String("12"));
    QCOMPARE(value("getfsize(g:d . '/sub')"), QLatin1String("0"));
    QCOMPARE(value("getfsize(g:d . '/nope')"), QLatin1String("-1"));

    // -- getftime() --
    // The time itself is the clock's, so only its shape is checked.
    QCOMPARE(value("getftime(g:d . '/nope')"), QLatin1String("-1"));
    QCOMPARE(value("getftime(g:d . '/plain.txt') > 0"), QLatin1String("1"));

    // -- resolve() --
    // The link leads to the file; a plain path and a missing one are
    // themselves.
    QCOMPARE(value("resolve(g:d . '/link.txt') == resolve(g:d . '/plain.txt')"),
             QLatin1String("1"));
    QCOMPARE(value("resolve(g:d . '/link.txt') =~ 'plain.txt$'"), QLatin1String("1"));
    QCOMPARE(value("resolve(g:d . '/nope') == g:d . '/nope'"), QLatin1String("1"));

    // -- readdir() --
    // Sorted, and without "." or "..".
    QCOMPARE(value("string(readdir(g:d))"),
             QLatin1String("['link.txt', 'plain.txt', 'sub']"));
    QCOMPARE(value("string(readdir(g:d . '/nope'))"), QLatin1String("[]"));
    // The second argument keeps a name when it answers true.
    QCOMPARE(value("string(readdir(g:d, {n -> n =~ 'txt$'}))"),
             QLatin1String("['link.txt', 'plain.txt']"));
    QCOMPARE(value("string(readdir(g:d, {n -> 0}))"), QLatin1String("[]"));
    // A -1 stops the walk. Vim stops over the filesystem's own order and
    // sorts afterwards, so WHICH names it keeps is not reproducible; here the
    // walk is the sorted one, so stopping at the second name leaves the first.
    QCOMPARE(value("string(readdir(g:d, {n -> n == 'plain.txt' ? -1 : 1}))"),
             QLatin1String("['link.txt']"));

    data.doCommand("unlet g:d");
}

void FakeVimTester::test_vim_script_islocked()
{
    // Whether ":lockvar" holds a name: 1 held, 0 there and not held, -1
    // nothing of that name. Values taken from Vim 9.1.
    TestData data;
    setup(&data);
    QString message;
    data.handler->commandBufferChanged.set(
        [&](const QString &msg, int, int, int) {
            if (!msg.isEmpty() && !msg.startsWith("--"))
                message = msg;
        });
    const auto value = [&](const QString &expr) {
        message.clear();
        data.doCommand("echo " + expr);
        return message;
    };

    data.doCommand("let g:plain = 1");
    QCOMPARE(value("islocked('g:plain')"), QLatin1String("0"));
    data.doCommand("lockvar g:plain");
    QCOMPARE(value("islocked('g:plain')"), QLatin1String("1"));
    // Writing to it is refused while it is held, which is what the lock is
    // for - so the answer is not just bookkeeping.
    message.clear();
    data.doCommand("let g:plain = 2");
    QVERIFY(message.contains("E741"));
    data.doCommand("unlockvar g:plain");
    QCOMPARE(value("islocked('g:plain')"), QLatin1String("0"));
    QCOMPARE(value("g:plain"), QLatin1String("1"));

    // Nothing of that name at all.
    QCOMPARE(value("islocked('g:nosuch_xyz')"), QLatin1String("-1"));
    // The name needs no "g:" in front of it.
    QCOMPARE(value("islocked('plain')"), QLatin1String("0"));

    // An entry answers for the container it is in, spelled either way.
    data.doCommand("let g:l = [1, 2]");
    data.doCommand("let g:d = {'field': 1}");
    data.doCommand("lockvar g:l | lockvar g:d");
    QCOMPARE(value("islocked('g:l')"), QLatin1String("1"));
    QCOMPARE(value("islocked('g:l[0]')"), QLatin1String("1"));
    QCOMPARE(value("islocked('g:d')"), QLatin1String("1"));
    QCOMPARE(value("islocked('g:d.field')"), QLatin1String("1"));
    QCOMPARE(value("islocked(\"g:d['field']\")"), QLatin1String("1"));
    data.doCommand("unlockvar g:l | unlockvar g:d");
    QCOMPARE(value("islocked('g:l[0]')"), QLatin1String("0"));
    QCOMPARE(value("islocked('g:d.field')"), QLatin1String("0"));
    // An entry of a container that is not there answers -1 through it.
    QCOMPARE(value("islocked('g:nodict_xyz.f')"), QLatin1String("-1"));

    data.doCommand("unlet g:plain | unlet g:l | unlet g:d");
}

void FakeVimTester::test_vim_script_autocmd_get()
{
    // The autocommands there are, one dict each. Values taken from Vim 9.1.
    // The autocommand list is shared with every other test slot, so this asks
    // only about a group of its own rather than counting the whole list. Dict
    // key order is an engine detail (QMap sorts them), so the entries are
    // looked up one at a time.
    TestData data;
    setup(&data);
    QString message;
    data.handler->commandBufferChanged.set(
        [&](const QString &msg, int, int, int) {
            if (!msg.isEmpty() && !msg.startsWith("--"))
                message = msg;
        });
    const auto value = [&](const QString &expr) {
        message.clear();
        data.doCommand("echo " + expr);
        return message;
    };
    const QString mine = "autocmd_get({'group': 'FvGetTest'})";

    // Defined by ":source", not by separate doCommand() calls: the suite's
    // other autocommand tests do it that way, and an "augroup" block built up
    // one command at a time does not register here - the same harness quirk a
    // mapping whose right-hand side is ":call ...<CR>" runs into.
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    QFile f(dir.path() + "/ag.vim");
    QVERIFY(f.open(QIODevice::WriteOnly));
    f.write("augroup FvGetTest\n"
            "  autocmd BufWritePost *.c echo 1\n"
            "  autocmd User FvEvent echo 2\n"
            "augroup END\n");
    f.close();
    data.doCommand("source " + dir.path() + "/ag.vim");

    QCOMPARE(value("len(" + mine + ")"), QLatin1String("2"));
    // A dict that narrows by nothing narrows nothing away. A key that is not
    // there must not be taken for a filter: an absent one reads back as "0"
    // through the engine's own default value, which once left this answering
    // with nothing at all.
    QCOMPARE(value("len(autocmd_get({})) >= 2"), QLatin1String("1"));
    QCOMPARE(value("sort(keys(" + mine + "[0]))"),
             QLatin1String("['cmd', 'event', 'group', 'nested', 'once', 'pattern']"));
    // Each carries its group, event, pattern and command.
    QCOMPARE(value("sort(map(copy(" + mine + "), {_, a -> a.event}))"),
             QLatin1String("['BufWritePost', 'User']"));
    QCOMPARE(value("sort(map(copy(" + mine + "), {_, a -> a.pattern}))"),
             QLatin1String("['*.c', 'FvEvent']"));
    QCOMPARE(value("sort(map(copy(" + mine + "), {_, a -> a.cmd}))"),
             QLatin1String("['echo 1', 'echo 2']"));
    QCOMPARE(value(mine + "[0].group"), QLatin1String("FvGetTest"));
    // Neither flag is kept here, and both answer FALSE rather than zero.
    QCOMPARE(value(mine + "[0].once"), QLatin1String("v:false"));
    QCOMPARE(value(mine + "[0].nested"), QLatin1String("v:false"));
    QCOMPARE(value("type(" + mine + "[0].once) == type(v:false)"),
             QLatin1String("1"));

    // Narrowing by event and by pattern, on top of the group.
    QCOMPARE(value("len(autocmd_get({'group': 'FvGetTest', 'event': 'BufWritePost'}))"),
             QLatin1String("1"));
    // An event is named without regard to case.
    QCOMPARE(value("len(autocmd_get({'group': 'FvGetTest', 'event': 'bufwritepost'}))"),
             QLatin1String("1"));
    QCOMPARE(value("len(autocmd_get({'group': 'FvGetTest', 'pattern': '*.c'}))"),
             QLatin1String("1"));
    QCOMPARE(value("len(autocmd_get({'group': 'FvGetTest', 'event': 'CursorMoved'}))"),
             QLatin1String("0"));

    // A group nothing belongs to is an error, not an empty answer.
    message.clear();
    data.doCommand("echo autocmd_get({'group': 'FvNoSuchGroupXyz'})");
    QVERIFY(message.contains("E367"));

    data.doCommand("autocmd! FvGetTest");
    QCOMPARE(value("exists('*autocmd_get')"), QLatin1String("1"));
}

void FakeVimTester::test_vim_script_typename()
{
    // The name of a type as Vim 9.1 writes it. All values measured there.
    TestData data;
    setup(&data);
    QString message;
    data.handler->commandBufferChanged.set(
        [&](const QString &msg, int, int, int) {
            if (!msg.isEmpty() && !msg.startsWith("--"))
                message = msg;
        });
    const auto name = [&](const QString &expr) {
        message.clear();
        data.doCommand("echo typename(" + expr + ")");
        return message;
    };

    QCOMPARE(name("0"), QLatin1String("number"));
    QCOMPARE(name("'x'"), QLatin1String("string"));
    QCOMPARE(name("1.5"), QLatin1String("float"));
    QCOMPARE(name("v:true"), QLatin1String("bool"));
    QCOMPARE(name("v:null"), QLatin1String("special"));

    // A container is named after the type its members agree on.
    QCOMPARE(name("[1, 2]"), QLatin1String("list<number>"));
    QCOMPARE(name("['a']"), QLatin1String("list<string>"));
    QCOMPARE(name("{'a': 1}"), QLatin1String("dict<number>"));
    // Members that disagree leave "any" behind, and a number and a float
    // disagree as much as a number and a string do.
    QCOMPARE(name("[1, 'a']"), QLatin1String("list<any>"));
    QCOMPARE(name("[1, 1.5]"), QLatin1String("list<any>"));
    QCOMPARE(name("{'a': 1, 'b': 's'}"), QLatin1String("dict<any>"));
    QCOMPARE(name("[v:true, v:false]"), QLatin1String("list<bool>"));
    QCOMPARE(name("[v:true, 1]"), QLatin1String("list<any>"));
    QCOMPARE(name("[v:null, v:null]"), QLatin1String("list<special>"));

    // The naming reaches all the way down, not one level.
    QCOMPARE(name("[[1], [2]]"), QLatin1String("list<list<number>>"));
    QCOMPARE(name("[[1], ['a']]"), QLatin1String("list<list<any>>"));
    QCOMPARE(name("[[[1]], [['a']]]"), QLatin1String("list<list<list<any>>>"));
    QCOMPARE(name("[{'a': 1}]"), QLatin1String("list<dict<number>>"));
    QCOMPARE(name("{'a': [1]}"), QLatin1String("dict<list<number>>"));
    QCOMPARE(name("[{'a': 1}, {'a': 's'}]"), QLatin1String("list<dict<any>>"));
    QCOMPARE(name("{'a': {'x': 1}, 'b': {'y': 's'}}"), QLatin1String("dict<dict<any>>"));
    QCOMPARE(name("[[1, 'a'], [2]]"), QLatin1String("list<list<any>>"));

    // An empty container has no member to ask, so it is undecided rather than
    // "any": next to a decided one it takes that type on, whichever comes
    // first and however deep it sits. Only where nothing ever decides it, or
    // where it meets a type it cannot be, does it end up as "any".
    QCOMPARE(name("[]"), QLatin1String("list<any>"));
    QCOMPARE(name("{}"), QLatin1String("dict<any>"));
    QCOMPARE(name("[[1], []]"), QLatin1String("list<list<number>>"));
    QCOMPARE(name("[[], [1]]"), QLatin1String("list<list<number>>"));
    QCOMPARE(name("[[], [], [1]]"), QLatin1String("list<list<number>>"));
    QCOMPARE(name("[[], []]"), QLatin1String("list<list<any>>"));
    QCOMPARE(name("[[[1]], [[]]]"), QLatin1String("list<list<list<number>>>"));
    QCOMPARE(name("[{}, {'a': 1}]"), QLatin1String("list<dict<number>>"));
    QCOMPARE(name("{'a': {}, 'b': {'x': 1}}"), QLatin1String("dict<dict<number>>"));
    QCOMPARE(name("[[1], [], ['a']]"), QLatin1String("list<list<any>>"));
    // A list and a dict are not each other, and neither is a scalar.
    QCOMPARE(name("[[], 1]"), QLatin1String("list<any>"));
    QCOMPARE(name("[[], {}]"), QLatin1String("list<any>"));
    QCOMPARE(name("[{}, [1]]"), QLatin1String("list<any>"));
    QCOMPARE(name("[[1], 'a']"), QLatin1String("list<any>"));

    // A lambda returns it is not known what. A named function has no declared
    // types either, so nothing is said beyond that it is one - sourced rather
    // than defined here one line at a time, and asked about first so that a
    // function that never arrived cannot be mistaken for one without types.
    QCOMPARE(name("{-> 1}"), QLatin1String("func(...): [unknown]"));

    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    QFile f(dir.path() + "/fn.vim");
    QVERIFY(f.open(QIODevice::WriteOnly));
    f.write("function! FvTypeNameFunc(a)\n  return 1\nendfunction\n");
    f.close();
    data.doCommand("source " + dir.path() + "/fn.vim");
    message.clear();
    data.doCommand("echo exists('*FvTypeNameFunc')");
    QCOMPARE(message, QLatin1String("1"));
    QCOMPARE(name("function('FvTypeNameFunc')"), QLatin1String("func(...): any"));

    message.clear();
    data.doCommand("echo exists('*typename')");
    QCOMPARE(message, QLatin1String("1"));
}

void FakeVimTester::test_vim_autocmd_bang_clears()
{
    // ":autocmd!" with an event named CLEARS before it registers. Measured in
    // Vim 9.1. The autocommand list is shared with every other test slot, so
    // everything here stays inside groups of its own, named per command rather
    // than through an "augroup" block - one built up a line at a time does not
    // register in this harness.
    TestData data;
    setup(&data);
    QString message;
    data.handler->commandBufferChanged.set(
        [&](const QString &msg, int, int, int) {
            if (!msg.isEmpty() && !msg.startsWith("--"))
                message = msg;
        });
    const auto value = [&](const QString &expr) {
        message.clear();
        data.doCommand("echo " + expr);
        return message;
    };
    const auto seed = [&] {
        data.doCommand("autocmd! FvBang");
        data.doCommand("autocmd! FvBang2");
        data.doCommand("autocmd FvBang BufRead *.a echo 1");
        data.doCommand("autocmd FvBang BufRead *.a echo 2");
        data.doCommand("autocmd FvBang BufWrite *.a echo 3");
        data.doCommand("autocmd FvBang BufRead *.b echo 4");
        data.doCommand("autocmd FvBang2 BufRead *.a echo other");
    };
    const QString mine = "autocmd_get({'group': 'FvBang'})";
    const auto listed = [&](const QString &what) {
        return value("sort(map(copy(" + mine + "), {_, a -> a." + what + "}))");
    };

    seed();
    QCOMPARE(value("len(" + mine + ")"), QLatin1String("4"));

    // An event and a pattern: those registrations go, the others stay.
    data.doCommand("autocmd! FvBang BufRead *.a");
    QCOMPARE(listed("cmd"), QLatin1String("['echo 3', 'echo 4']"));
    // A group of its own is left alone.
    QCOMPARE(value("len(autocmd_get({'group': 'FvBang2'}))"), QLatin1String("1"));

    // An event with no pattern takes every pattern with it.
    seed();
    data.doCommand("autocmd! FvBang BufRead");
    QCOMPARE(listed("cmd"), QLatin1String("['echo 3']"));

    // A command as well: the registrations for that event and pattern are
    // cleared and the new one put in their place, so ONE is left - the whole
    // point of the bang, and what makes it more than a way to register.
    seed();
    data.doCommand("autocmd! FvBang BufRead *.a echo new");
    QCOMPARE(listed("cmd"), QLatin1String("['echo 3', 'echo 4', 'echo new']"));
    QCOMPARE(value("len(autocmd_get({'group': 'FvBang', 'pattern': '*.a',"
                   " 'event': 'BufRead'}))"), QLatin1String("1"));

    // "*" stands for every event.
    seed();
    data.doCommand("autocmd! FvBang * *.a");
    QCOMPARE(listed("cmd"), QLatin1String("['echo 4']"));

    // Events written with commas between them.
    seed();
    data.doCommand("autocmd! FvBang BufRead,BufWrite *.a");
    QCOMPARE(listed("cmd"), QLatin1String("['echo 4']"));

    // Two names for one event: BufRead clears a BufReadPost registration. One
    // unrelated registration is kept so the group does not run empty - a group
    // with nothing in it still exists in Vim, but is indistinguishable from one
    // that was never there here, and that is not what this is about.
    data.doCommand("autocmd! FvBang");
    data.doCommand("autocmd FvBang BufReadPost *.s echo post");
    data.doCommand("autocmd FvBang BufWrite *.keep echo keep");
    QCOMPARE(listed("cmd"), QLatin1String("['echo keep', 'echo post']"));
    data.doCommand("autocmd! FvBang BufRead *.s");
    QCOMPARE(listed("cmd"), QLatin1String("['echo keep']"));

    data.doCommand("autocmd! FvBang");
    data.doCommand("autocmd! FvBang2");
}

void FakeVimTester::test_vim_script_autocmd_add_delete()
{
    // Registering and removing autocommands through a dict. All values
    // measured in Vim 9.1. Everything stays inside groups of its own: the
    // autocommand list is shared with every other test slot, and a delete
    // that narrows by nothing would take the whole anonymous group with it.
    TestData data;
    setup(&data);
    QString message;
    data.handler->commandBufferChanged.set(
        [&](const QString &msg, int, int, int) {
            if (!msg.isEmpty() && !msg.startsWith("--"))
                message = msg;
        });
    const auto value = [&](const QString &expr) {
        message.clear();
        data.doCommand("echo " + expr);
        return message;
    };
    const QString mine = "autocmd_get({'group': 'FvAdd'})";
    const auto cmds = [&] {
        return value("sort(map(copy(" + mine + "), {_, a -> a.cmd}))");
    };
    const auto add = [&](const QString &what) {
        return value("autocmd_add([{'group': 'FvAdd', " + what + "}])");
    };
    const auto del = [&](const QString &what) {
        return value("autocmd_delete([{'group': 'FvAdd', " + what + "}])");
    };
    const auto clear = [&] { data.doCommand("autocmd! FvAdd"); };

    // Both answer with v:true, not 1.
    clear();
    QCOMPARE(add("'event': 'BufRead', 'pattern': '*.a', 'cmd': 'echo 1'"),
             QLatin1String("v:true"));
    QCOMPARE(value("type(autocmd_add([])) == type(v:true)"), QLatin1String("1"));
    QCOMPARE(cmds(), QLatin1String("['echo 1']"));

    // The same one again registers a second time.
    add("'event': 'BufRead', 'pattern': '*.a', 'cmd': 'echo 1'");
    QCOMPARE(cmds(), QLatin1String("['echo 1', 'echo 1']"));
    // "replace" clears what is there for that event and pattern first, so a
    // DIFFERENT command still leaves one behind, not three.
    add("'event': 'BufRead', 'pattern': '*.a', 'cmd': 'echo 2', 'replace': v:true");
    QCOMPARE(cmds(), QLatin1String("['echo 2']"));
    // It reaches no further than that event and pattern.
    add("'event': 'BufWrite', 'pattern': '*.a', 'cmd': 'echo w'");
    add("'event': 'BufRead', 'pattern': '*.b', 'cmd': 'echo b'");
    add("'event': 'BufRead', 'pattern': '*.a', 'cmd': 'echo 3', 'replace': v:true");
    QCOMPARE(cmds(), QLatin1String("['echo 3', 'echo b', 'echo w']"));

    // An event and a pattern each naming several means every pairing.
    clear();
    add("'event': ['BufRead', 'BufWrite'], 'pattern': ['*.a', '*.b'],"
        " 'cmd': 'echo m'");
    QCOMPARE(value("len(" + mine + ")"), QLatin1String("4"));
    QCOMPARE(value("sort(map(copy(" + mine + "), {_, a -> a.event . a.pattern}))"),
             QLatin1String("['BufRead*.a', 'BufRead*.b', 'BufWrite*.a', 'BufWrite*.b']"));

    // An entry with nowhere or nothing to register leaves nothing behind, and
    // is not an error - it answers v:true like any other. One registration is
    // kept throughout so that "nothing was added" can be told apart from the
    // group having gone: an emptied group still exists in Vim, but here it is
    // indistinguishable from one that was never there.
    clear();
    const QString keeper = "'event': 'BufEnter', 'pattern': '*.keeper',"
                           " 'cmd': 'echo keeper'";
    add(keeper);
    QCOMPARE(add("'event': 'BufRead', 'pattern': '*.a'"), QLatin1String("v:true"));
    QCOMPARE(add("'event': 'BufRead', 'cmd': 'echo x'"), QLatin1String("v:true"));
    QCOMPARE(add("'pattern': '*.a', 'cmd': 'echo x'"), QLatin1String("v:true"));
    QCOMPARE(cmds(), QLatin1String("['echo keeper']"));

    // A buffer number stands in for a pattern, written the way Vim writes it.
    clear();
    add("'event': 'BufRead', 'bufnr': 7, 'cmd': 'echo buf'");
    QCOMPARE(value(mine + "[0].pattern"), QLatin1String("<buffer=7>"));

    // Deleting: an event and a pattern take just those.
    clear();
    add(keeper);
    add("'event': 'BufRead', 'pattern': '*.a', 'cmd': 'echo 1'");
    add("'event': 'BufRead', 'pattern': '*.b', 'cmd': 'echo 2'");
    add("'event': 'BufWrite', 'pattern': '*.a', 'cmd': 'echo 3'");
    QCOMPARE(del("'event': 'BufRead', 'pattern': '*.a'"), QLatin1String("v:true"));
    QCOMPARE(cmds(), QLatin1String("['echo 2', 'echo 3', 'echo keeper']"));
    // An event with no pattern takes every pattern, and a pattern with no
    // event takes every event.
    del("'event': 'BufRead'");
    QCOMPARE(cmds(), QLatin1String("['echo 3', 'echo keeper']"));
    add("'event': 'BufRead', 'pattern': '*.a', 'cmd': 'echo 4'");
    del("'pattern': '*.a'");
    QCOMPARE(cmds(), QLatin1String("['echo keeper']"));
    // "*" stands for every event here, though autocmd_add() has no use for it.
    add("'event': 'BufRead', 'pattern': '*.c', 'cmd': 'echo 5'");
    add("'event': 'BufWrite', 'pattern': '*.c', 'cmd': 'echo 6'");
    del("'event': '*', 'pattern': '*.c'");
    QCOMPARE(cmds(), QLatin1String("['echo keeper']"));

    // The surprise: a delete naming a "cmd" is ":au! {ev} {pat} {cmd}", which
    // CLEARS that event and pattern and then registers the command - so asking
    // for one that was never there leaves it behind rather than doing nothing.
    clear();
    add("'event': 'BufRead', 'pattern': '*.a', 'cmd': 'echo A'");
    add("'event': 'BufRead', 'pattern': '*.a', 'cmd': 'echo B'");
    del("'event': 'BufRead', 'pattern': '*.a', 'cmd': 'echo NEVER'");
    QCOMPARE(cmds(), QLatin1String("['echo NEVER']"));

    // A group named for a delete has to be there, whatever else is asked.
    message.clear();
    data.doCommand("echo autocmd_delete([{'group': 'FvNoSuchXyz'}])");
    QVERIFY(message.contains("E367"));
    message.clear();
    data.doCommand("echo autocmd_delete([{'group': 'FvNoSuchXyz',"
                   " 'event': 'BufRead'}])");
    QVERIFY(message.contains("E367"));

    // An event that is no event is reported...
    clear();
    add("'event': 'BufRead', 'pattern': '*.a', 'cmd': 'echo keep'");
    message.clear();
    data.doCommand("echo autocmd_add([{'group': 'FvAdd', 'event': 'FvNoSuchEvent',"
                   " 'pattern': '*.z', 'cmd': 'echo bad'}])");
    QVERIFY(message.contains("E216"));
    // ...without the entries around it being dropped, and nothing registers
    // for it.
    message.clear();
    data.doCommand("echo autocmd_add([{'group': 'FvAdd', 'event': 'FvNoSuchEvent',"
                   " 'pattern': '*.z', 'cmd': 'echo bad'},"
                   " {'group': 'FvAdd', 'event': 'BufRead', 'pattern': '*.good',"
                   " 'cmd': 'echo good'}])");
    QCOMPARE(cmds(), QLatin1String("['echo good', 'echo keep']"));
    // Adding has no use for "*" and says the same.
    message.clear();
    data.doCommand("echo autocmd_add([{'group': 'FvAdd', 'event': '*',"
                   " 'pattern': '*.z', 'cmd': 'echo star'}])");
    QVERIFY(message.contains("E216"));

    // A list is wanted, and something that is not a dict is passed over.
    message.clear();
    data.doCommand("echo autocmd_add({'event': 'BufRead'})");
    QVERIFY(message.contains("E1211"));
    QCOMPARE(value("autocmd_add(['x'])"), QLatin1String("v:true"));

    // Naming only the group empties it, which is what takes the group away.
    clear();
    add("'event': 'BufRead', 'pattern': '*.a', 'cmd': 'echo 1'");
    add("'event': 'BufWrite', 'pattern': '*.b', 'cmd': 'echo 2'");
    QCOMPARE(del(""), QLatin1String("v:true"));
    message.clear();
    data.doCommand("echo autocmd_get({'group': 'FvAdd'})");
    QVERIFY(message.contains("E367"));

    QCOMPARE(value("exists('*autocmd_add') && exists('*autocmd_delete')"),
             QLatin1String("1"));
    clear();
}

void FakeVimTester::test_vim_script_filecopy()
{
    // filecopy({from}, {to}). All values measured in Vim 9.1.
    TestData data;
    setup(&data);
    QString message;
    data.handler->commandBufferChanged.set(
        [&](const QString &msg, int, int, int) {
            if (!msg.isEmpty() && !msg.startsWith("--"))
                message = msg;
        });
    const auto value = [&](const QString &expr) {
        message.clear();
        data.doCommand("echo " + expr);
        return message;
    };
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString where = dir.path() + "/";
    const auto write = [&](const QString &name, const QByteArray &what) {
        QFile file(where + name);
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write(what);
    };
    const auto copy = [&](const QString &from, const QString &to) {
        return value("filecopy('" + where + from + "', '" + where + to + "')");
    };
    const auto contentOf = [&](const QString &name) {
        QFile file(where + name);
        return file.open(QIODevice::ReadOnly) ? file.readAll() : QByteArray("<none>");
    };

    write("src.txt", "the source\n");

    // One where the copy is there now, and a Number rather than a Boolean.
    QCOMPARE(copy("src.txt", "dst.txt"), QLatin1String("1"));
    QCOMPARE(contentOf("dst.txt"), QByteArray("the source\n"));
    QCOMPARE(value("type(filecopy('" + where + "src.txt', '" + where
                   + "dst_b.txt')) == type(0)"), QLatin1String("1"));

    // A destination that is already there is NOT written over: it keeps what it
    // had and the answer is zero, which is the part worth knowing - a copy that
    // silently replaced the file would look the same from the return value of a
    // first call.
    write("taken.txt", "already here\n");
    QCOMPARE(copy("src.txt", "taken.txt"), QLatin1String("0"));
    QCOMPARE(contentOf("taken.txt"), QByteArray("already here\n"));

    // Nothing to copy, nowhere to put it, or a directory at either end.
    QCOMPARE(copy("nosuch.txt", "out.txt"), QLatin1String("0"));
    QCOMPARE(copy("src.txt", "nodir/out.txt"), QLatin1String("0"));
    QVERIFY(QDir().mkpath(where + "adir"));
    QCOMPARE(copy("adir", "out.txt"), QLatin1String("0"));
    QCOMPARE(copy("src.txt", "adir"), QLatin1String("0"));
    // The same file at both ends is a destination that is already there.
    QCOMPARE(copy("src.txt", "src.txt"), QLatin1String("0"));
    QCOMPARE(contentOf("src.txt"), QByteArray("the source\n"));
    QCOMPARE(value("filecopy('', '" + where + "out.txt')"), QLatin1String("0"));
    QCOMPARE(value("filecopy('" + where + "src.txt', '')"), QLatin1String("0"));

    // What the file may be done with comes along with it.
    write("run.sh", "#!/bin/sh\n");
    QVERIFY(QFile::setPermissions(where + "run.sh",
                                  QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner
                                  | QFile::ReadGroup | QFile::ExeGroup
                                  | QFile::ReadOther | QFile::ExeOther));
    QCOMPARE(value("getfperm('" + where + "run.sh')"), QLatin1String("rwxr-xr-x"));
    QCOMPARE(copy("run.sh", "run2.sh"), QLatin1String("1"));
    QCOMPARE(value("getfperm('" + where + "run2.sh')"), QLatin1String("rwxr-xr-x"));
    // A source nothing may write to still copies.
    write("ro.txt", "read only\n");
    QVERIFY(QFile::setPermissions(where + "ro.txt",
                                  QFile::ReadOwner | QFile::ReadGroup | QFile::ReadOther));
    QCOMPARE(copy("ro.txt", "ro2.txt"), QLatin1String("1"));
    QCOMPARE(value("getfperm('" + where + "ro2.txt')"), QLatin1String("r--r--r--"));

    // A link is followed rather than copied as a link, which is where this
    // parts from Vim: there the copy is a link of its own again.
    QVERIFY(QFile::link("src.txt", where + "link.txt"));
    QCOMPARE(value("getftype('" + where + "link.txt')"), QLatin1String("link"));
    QCOMPARE(copy("link.txt", "fromlink.txt"), QLatin1String("1"));
    QCOMPARE(contentOf("fromlink.txt"), QByteArray("the source\n"));
    QCOMPARE(value("getftype('" + where + "fromlink.txt')"), QLatin1String("file"));

    QCOMPARE(value("exists('*filecopy')"), QLatin1String("1"));
}

void FakeVimTester::test_vim_autocmd_textyankpost()
{
    // TextYankPost, and the v:event it is handed. All values measured in Vim
    // 9.1. The autocommand list is shared with every other test slot, so this
    // keeps to a group of its own and clears it at the end.
    TestData data;
    setup(&data);
    QString message;
    data.handler->commandBufferChanged.set(
        [&](const QString &msg, int, int, int) {
            if (!msg.isEmpty() && !msg.startsWith("--"))
                message = msg;
        });
    const auto value = [&](const QString &expr) {
        message.clear();
        data.doCommand("echo " + expr);
        return message;
    };
    // Each firing appends one line, so what fired and what it saw are both
    // readable afterwards - and so is a yank that fires nothing.
    data.doCommand("let g:yanks = []");
    data.doCommand("autocmd FvYank TextYankPost * call add(g:yanks, v:event.operator"
                   " . ' ' . v:event.regtype . ' ' . string(v:event.regname)"
                   " . ' ' . string(v:event.visual) . ' ' . string(v:event.inclusive)"
                   " . ' ' . string(v:event.regcontents))");
    const auto fired = [&] { return value("get(g:yanks, -1, 'NOTHING')"); };
    const auto count = [&] { return value("len(g:yanks)"); };
    const auto reset = [&] { data.doCommand("let g:yanks = []"); };

    data.setText("alpha beta gamma" N "second line here" N "third line here");

    // v:event is there only while an autocommand runs.
    QCOMPARE(value("empty(v:event)"), QLatin1String("1"));

    // Whole lines, and the newline the register keeps is not a line of its own.
    data.doKeys("gg0yy");
    QCOMPARE(fired(), QLatin1String("y V '' v:false v:false ['alpha beta gamma']"));
    QCOMPARE(count(), QLatin1String("1"));

    // An exclusive motion says so, an inclusive one says so too.
    data.doKeys("gg0yw");
    QCOMPARE(fired(), QLatin1String("y v '' v:false v:false ['alpha ']"));
    data.doKeys("gg0ye");
    QCOMPARE(fired(), QLatin1String("y v '' v:false v:true ['alpha']"));

    // Deleting and changing announce themselves as what they are.
    data.doKeys("gg0dd");
    QCOMPARE(fired(), QLatin1String("d V '' v:false v:false ['alpha beta gamma']"));
    data.doKeys("gg0x");
    QCOMPARE(fired(), QLatin1String("d v '' v:false v:false ['s']"));

    // The register a command named, where the unnamed one names nothing.
    reset();
    data.doKeys("gg0\"ayy");
    QCOMPARE(fired(), QLatin1String("y V 'a' v:false v:false ['econd line here']"));

    // A selection takes in where it ends, whichever way it was made.
    data.doKeys("gg0vey");
    QCOMPARE(fired(), QLatin1String("y v '' v:true v:true ['econd']"));
    data.doKeys("ggVy");
    QCOMPARE(fired(), QLatin1String("y V '' v:true v:true ['econd line here']"));

    // Blockwise is a CTRL-V with the width of the block behind it, and takes
    // one piece out of each line it spans.
    reset();
    data.setText("abcd" N "efgh" N "ijkl");
    data.doKeys("gg0l<c-v>jly");
    QCOMPARE(value("v:true ? char2nr(g:yanks[0][2]) : 0"), QLatin1String("22"));
    QCOMPARE(value("g:yanks[0][3]"), QLatin1String("2"));
    QCOMPARE(value("g:yanks[0][0]"), QLatin1String("y"));
    QCOMPARE(value("g:yanks[0][5:]"), QLatin1String("'' v:true v:true ['bc', 'fg']"));

    // Putting text back is not yanking it, and neither is joining lines.
    reset();
    data.doKeys("ggp");
    data.doKeys("ggJ");
    QCOMPARE(count(), QLatin1String("0"));
    QCOMPARE(fired(), QLatin1String("NOTHING"));

    // The black hole takes nothing and so says nothing.
    data.doKeys("gg0\"_dd");
    QCOMPARE(count(), QLatin1String("0"));

    data.doCommand("autocmd! FvYank");
    data.doCommand("unlet! g:yanks");
}

void FakeVimTester::test_vim_autocmd_cmdline()
{
    // CmdlineEnter and CmdlineLeave. All values measured in Vim 9.1, where the
    // PATTERN is matched against the character naming the command line rather
    // than a file name, "<afile>" stands for that character, and v:char on
    // leaving is the key that left it. There is no v:event here at all.
    TestData data;
    setup(&data);
    QString message;
    data.handler->commandBufferChanged.set(
        [&](const QString &msg, int, int, int) {
            if (!msg.isEmpty() && !msg.startsWith("--"))
                message = msg;
        });
    const auto value = [&](const QString &expr) {
        message.clear();
        data.doCommand("echo " + expr);
        return message;
    };
    const auto log = [&] { return value("string(g:cl)"); };
    const auto reset = [&] { data.doCommand("let g:cl = []"); };

    data.setText("alpha beta" N "second line" N "third line");
    data.doCommand("let g:cl = []");
    data.doCommand("autocmd FvCl CmdlineEnter * call add(g:cl, 'E' . expand('<afile>'))");
    data.doCommand("autocmd FvCl CmdlineLeave * call add(g:cl, 'L' . expand('<afile>')"
                   " . char2nr(v:char))");

    // A ":" line, entered and left with Return (13).
    reset();
    data.doKeys(":echo 1<CR>");
    QCOMPARE(log(), QLatin1String("['E:', 'L:13']"));

    // Given up on with Escape (27), and emptied with backspace (8).
    reset();
    data.doKeys(":echo 2<Esc>");
    QCOMPARE(log(), QLatin1String("['E:', 'L:27']"));
    reset();
    data.doKeys(":ab<BS><BS><BS>");
    QCOMPARE(log(), QLatin1String("['E:', 'L:8']"));

    // A search line names itself by its direction.
    reset();
    data.doKeys("gg/second<CR>");
    QCOMPARE(log(), QLatin1String("['E/', 'L/13']"));
    reset();
    data.doKeys("G?alpha<CR>");
    QCOMPARE(log(), QLatin1String("['E?', 'L?13']"));
    reset();
    data.doKeys("gg/zzz<Esc>");
    QCOMPARE(log(), QLatin1String("['E/', 'L/27']"));

    // The "=" expression register is a command line of its own.
    reset();
    data.doKeys("ggA<c-r>=1+1<CR><Esc>");
    QCOMPARE(log(), QLatin1String("['E=', 'L=13']"));

    // The pattern picks the kind of command line, which is the part that is
    // not a file name at all.
    data.doCommand("autocmd! FvCl");
    data.doCommand("let g:cl = []");
    data.doCommand("autocmd FvCl CmdlineEnter : call add(g:cl, 'colon')");
    data.doCommand("autocmd FvCl CmdlineEnter / call add(g:cl, 'slash')");
    reset();
    data.doKeys(":echo 3<CR>");
    data.doKeys("gg/second<CR>");
    data.doKeys("G?alpha<CR>");
    QCOMPARE(log(), QLatin1String("['colon', 'slash']"));

    // Leaving fires BEFORE the line is carried out, so an autocommand still
    // sees what is about to run.
    data.doCommand("autocmd! FvCl");
    data.doCommand("let g:cl = []");
    data.doCommand("autocmd FvCl CmdlineLeave : call add(g:cl, 'leave')");
    reset();
    data.doKeys(":call add(g:cl, 'ran')<CR>");
    QCOMPARE(log(), QLatin1String("['leave', 'ran']"));

    // v:char is nothing outside an autocommand, and there is no v:event.
    QCOMPARE(value("char2nr(v:char)"), QLatin1String("0"));
    QCOMPARE(value("empty(v:event)"), QLatin1String("1"));

    data.doCommand("autocmd! FvCl");
    data.doCommand("unlet! g:cl");
}

void FakeVimTester::test_vim_autocmd_insertcharpre()
{
    // InsertCharPre, whose v:char is WRITABLE: what an autocommand leaves there
    // is what gets inserted. All values measured in Vim 9.1.
    TestData data;
    setup(&data);
    QString message;
    data.handler->commandBufferChanged.set(
        [&](const QString &msg, int, int, int) {
            if (!msg.isEmpty() && !msg.startsWith("--"))
                message = msg;
        });
    const auto value = [&](const QString &expr) {
        message.clear();
        data.doCommand("echo " + expr);
        return message;
    };
    const auto clear = [&] { data.doCommand("autocmd! FvIc"); };

    // What it is handed: the character, no v:event, and a pattern matched
    // against the file name as usual - not against the character.
    data.setText("|");
    data.doCommand("let g:seen = []");
    data.doCommand("autocmd FvIc InsertCharPre * call add(g:seen, v:char"
                   " . ':' . string(v:event))");
    data.doKeys("iab<Esc>");
    QCOMPARE(value("string(g:seen)"), QLatin1String("['a:{}', 'b:{}']"));
    data.doCommand("unlet! g:seen");
    clear();

    // Writing v:char changes what is inserted.
    data.setText("|");
    data.doCommand("autocmd FvIc InsertCharPre * let v:char = toupper(v:char)");
    data.doKeys("iabc<Esc>");
    QCOMPARE(data.text(), QString("ABC"));
    clear();

    // Emptying it drops the character altogether.
    data.setText("|");
    data.doCommand("autocmd FvIc InsertCharPre *"
                   " let v:char = v:char ==# 'x' ? '' : v:char");
    data.doKeys("iaxb<Esc>");
    QCOMPARE(data.text(), QString("ab"));
    clear();

    // More than one character there inserts them all.
    data.setText("|");
    data.doCommand("autocmd FvIc InsertCharPre *"
                   " let v:char = v:char ==# 'q' ? 'QQ' : v:char");
    data.doKeys("iaqb<Esc>");
    QCOMPARE(data.text(), QString("aQQb"));
    clear();

    // It fires in Replace mode too, and the replacement is what lands.
    data.setText("|12345");
    data.doCommand("autocmd FvIc InsertCharPre * let v:char = toupper(v:char)");
    data.doKeys("Rab<Esc>");
    QCOMPARE(data.text(), QString("AB345"));
    clear();

    // Nothing is announced for what was not typed: no autocommand means the
    // character still goes in untouched.
    data.setText("|");
    data.doKeys("iplain<Esc>");
    QCOMPARE(data.text(), QString("plain"));

    // v:char is nothing outside an autocommand.
    QCOMPARE(value("char2nr(v:char)"), QLatin1String("0"));
    QCOMPARE(value("exists('*maparg') && 1"), QLatin1String("1"));
    clear();
}

void FakeVimTester::test_vim_script_one_line_blocks()
{
    // "if ... | ... | endif" written on ONE line has to hold together wherever
    // the line comes from, and a line that arrives from an autocommand, an
    // ":execute", a modifier or ":silent" is a SEQUENCE, not a row of
    // unrelated commands. Measured in Vim 9.1. The assertion that matters in
    // each case is the "if 0" one: where the pieces are run one at a time the
    // guard never reaches what it guards, and the body runs regardless.
    TestData data;
    setup(&data);
    QString message;
    data.handler->commandBufferChanged.set(
        [&](const QString &msg, int, int, int) {
            if (!msg.isEmpty() && !msg.startsWith("--"))
                message = msg;
        });
    const auto value = [&](const QString &expr) {
        message.clear();
        data.doCommand("echo " + expr);
        return message;
    };
    const auto reset = [&] { data.doCommand("let g:r = []"); };
    const auto ran = [&] { return value("string(g:r)"); };

    data.setText("one" N "two");

    // Typed as an ex command line, which already held together.
    reset();
    data.doCommand("if 1 | call add(g:r, 'then') | endif");
    data.doCommand("if 0 | call add(g:r, 'NOT') | endif");
    QCOMPARE(ran(), QLatin1String("['then']"));

    // The else branch, on one line.
    reset();
    data.doCommand("if 0 | call add(g:r, 'then') | else | call add(g:r, 'else') | endif");
    QCOMPARE(ran(), QLatin1String("['else']"));

    // The loops too.
    data.doCommand("let g:n = 0");
    data.doCommand("let g:i = 0");
    data.doCommand("while g:i < 3 | let g:i += 1 | let g:n += 1 | endwhile");
    QCOMPARE(value("g:n"), QLatin1String("3"));
    data.doCommand("let g:f = 0");
    data.doCommand("for k in [1, 2, 3] | let g:f += k | endfor");
    QCOMPARE(value("g:f"), QLatin1String("6"));

    // From ":execute", where the bars sit inside a string.
    reset();
    data.doCommand("execute \"if 1 | call add(g:r, 'exe') | endif\"");
    data.doCommand("execute \"if 0 | call add(g:r, 'exe-NOT') | endif\"");
    QCOMPARE(ran(), QLatin1String("['exe']"));

    // After ":silent", which passes the rest of the line on.
    reset();
    data.doCommand("silent if 1 | call add(g:r, 'sil') | endif");
    data.doCommand("silent if 0 | call add(g:r, 'sil-NOT') | endif");
    QCOMPARE(ran(), QLatin1String("['sil']"));

    // After a modifier, likewise.
    reset();
    data.doCommand("noautocmd if 1 | call add(g:r, 'mod') | endif");
    data.doCommand("noautocmd if 0 | call add(g:r, 'mod-NOT') | endif");
    QCOMPARE(ran(), QLatin1String("['mod']"));

    // And from an autocommand body, which is where this was found. Fired by
    // really leaving insert mode rather than by ":doautocmd User", which drops
    // the name after the event and so could never match a pattern.
    reset();
    data.doCommand("autocmd FvOneLine InsertLeave *"
                   " if 1 | call add(g:r, 'ac') | endif");
    data.doCommand("autocmd FvOneLine InsertLeave *"
                   " if 0 | call add(g:r, 'ac-NOT') | endif");
    data.doKeys("ix<Esc>");
    QCOMPARE(ran(), QLatin1String("['ac']"));

    data.doCommand("autocmd! FvOneLine");
    data.doCommand("unlet! g:r");
    data.doCommand("unlet! g:n");
    data.doCommand("unlet! g:i");
    data.doCommand("unlet! g:f");
}

void FakeVimTester::test_vim_doautocmd_arguments()
{
    // ":doautocmd [<nomodeline>] [group] {event} [fname]". The name after the
    // event is what the patterns are matched against and what "<afile>" stands
    // for - dropping it meant ":doautocmd User Foo" could never match anything.
    // All values measured in Vim 9.1.
    TestData data;
    setup(&data);
    QString message;
    data.handler->commandBufferChanged.set(
        [&](const QString &msg, int, int, int) {
            if (!msg.isEmpty() && !msg.startsWith("--"))
                message = msg;
        });
    const auto value = [&](const QString &expr) {
        message.clear();
        data.doCommand("echo " + expr);
        return message;
    };
    const auto fired = [&] { return value("string(g:r)"); };
    const auto go = [&](const QString &args) {
        data.doCommand("let g:r = []");
        data.doCommand("doautocmd " + args);
    };

    data.setText("x");
    data.doCommand("let g:r = []");
    data.doCommand("autocmd FvDoA User Foo call add(g:r, 'Foo:' . expand('<afile>'))");
    data.doCommand("autocmd FvDoA User Bar call add(g:r, 'Bar')");
    data.doCommand("autocmd FvDoA User * call add(g:r, 'star')");
    data.doCommand("autocmd FvDoA BufRead *.c call add(g:r, 'c:' . expand('<afile>'))");
    data.doCommand("autocmd FvDoA BufRead *.h call add(g:r, 'h')");
    data.doCommand("autocmd FvDoB User Foo call add(g:r, 'B-Foo')");

    // The name picks which patterns match, and is what <afile> reports.
    go("User Foo");
    QCOMPARE(fired(), QLatin1String("['Foo:Foo', 'star', 'B-Foo']"));
    go("User Bar");
    QCOMPARE(fired(), QLatin1String("['Bar', 'star']"));
    // With no name only a pattern that matches anything comes up.
    go("User");
    QCOMPARE(fired(), QLatin1String("['star']"));

    // For an ordinary event the name is a file name, matched the same way.
    go("BufRead thing.c");
    QCOMPARE(fired(), QLatin1String("['c:thing.c']"));
    go("BufRead thing.h");
    QCOMPARE(fired(), QLatin1String("['h']"));

    // A group before the event narrows it to that group.
    go("FvDoA User Foo");
    QCOMPARE(fired(), QLatin1String("['Foo:Foo', 'star']"));
    go("FvDoB User Foo");
    QCOMPARE(fired(), QLatin1String("['B-Foo']"));

    // "<nomodeline>" is skipped rather than taken for a group.
    go("<nomodeline> User Foo");
    QCOMPARE(fired(), QLatin1String("['Foo:Foo', 'star', 'B-Foo']"));

    // ":doautoall" is ":doautocmd" over every loaded buffer; there is only
    // ever one here, so it takes the identical arguments the same way - NOT
    // separately measured against Vim, since the one-buffer case makes it
    // identical by construction.
    data.doCommand("let g:r = []");
    data.doCommand("doautoall User Foo");
    QCOMPARE(fired(), QLatin1String("['Foo:Foo', 'star', 'B-Foo']"));

    // A word that is neither an event nor a group is reported, and it names
    // the whole line it was given.
    data.doCommand("let g:r = []");
    message.clear();
    data.doCommand("doautocmd FvNoSuchGroup User Foo");
    QVERIFY(message.contains("E216"));
    QVERIFY(message.contains("FvNoSuchGroup User Foo"));
    QCOMPARE(fired(), QLatin1String("[]"));

    data.doCommand("autocmd! FvDoA");
    data.doCommand("autocmd! FvDoB");
    data.doCommand("unlet! g:r");
}

void FakeVimTester::test_vim_map_nowait()
{
    // ":map <nowait>". It was not recognised at all, so it was taken for the
    // left-hand side and the mapping went on the wrong key entirely. Values
    // measured in Vim 9.1.
    TestData data;
    setup(&data);
    QString message;
    data.handler->commandBufferChanged.set(
        [&](const QString &msg, int, int, int) {
            if (!msg.isEmpty() && !msg.startsWith("--"))
                message = msg;
        });
    const auto value = [&](const QString &expr) {
        message.clear();
        data.doCommand("echo " + expr);
        return message;
    };

    // The corner used here is empty to begin with, so nothing below can pass
    // by borrowing another test slot's mapping.
    QCOMPARE(value("empty(maparg('<F6>', 'n'))"), QLatin1String("1"));
    QCOMPARE(value("empty(maparg('<F7>', 'n'))"), QLatin1String("1"));

    // The left-hand side is the key, not the modifier.
    data.doCommand("nnoremap <nowait> <F6> :let g:hit = 'f6'<CR>");
    QCOMPARE(value("maparg('<F6>', 'n', 0, 1).lhs"), QLatin1String("<F6>"));
    QCOMPARE(value("maparg('<F6>', 'n', 0, 1).nowait"), QLatin1String("1"));
    QCOMPARE(value("empty(maparg('<nowait>', 'n'))"), QLatin1String("1"));

    // Without it the flag is off, and it survives being written with others in
    // either order.
    data.doCommand("nnoremap <F7> :let g:hit = 'f7'<CR>");
    QCOMPARE(value("maparg('<F7>', 'n', 0, 1).nowait"), QLatin1String("0"));
    data.doCommand("nnoremap <nowait><silent> <F8> :let g:hit = 'f8'<CR>");
    QCOMPARE(value("maparg('<F8>', 'n', 0, 1).nowait"), QLatin1String("1"));
    QCOMPARE(value("maparg('<F8>', 'n', 0, 1).silent"), QLatin1String("1"));
    data.doCommand("nnoremap <silent> <nowait> <F9> :let g:hit = 'f9'<CR>");
    QCOMPARE(value("maparg('<F9>', 'n', 0, 1).nowait"), QLatin1String("1"));
    QCOMPARE(value("maparg('<F9>', 'n', 0, 1).silent"), QLatin1String("1"));
    // maplist() answers with it too.
    QCOMPARE(value("len(filter(maplist(), {_, m -> m.lhs ==# '<F6>' && m.nowait}))"),
             QLatin1String("1"));

    // What it is FOR: a mapping that another one extends runs at once instead
    // of being held back to see whether the longer one is coming. Both keys
    // below start the same way, so "gq" alone is ambiguous.
    data.doCommand("let g:hit = ''");
    data.doCommand("nnoremap gqq :let g:hit = 'long'<CR>");
    data.doCommand("nnoremap <nowait> gq :let g:hit = 'short'<CR>");
    data.doKeys("gq");
    QCOMPARE(value("g:hit"), QLatin1String("short"));

    // Without it the short one waits, so nothing has happened yet.
    data.doCommand("let g:hit = ''");
    data.doCommand("nnoremap gw :let g:hit = 'short'<CR>");
    data.doCommand("nnoremap gww :let g:hit = 'long'<CR>");
    data.doKeys("gw");
    QCOMPARE(value("g:hit"), QLatin1String(""));
    // The next key settles it.
    data.doKeys("w");
    QCOMPARE(value("g:hit"), QLatin1String("long"));

    data.doCommand("nunmap <F6>");
    data.doCommand("nunmap <F7>");
    data.doCommand("nunmap <F8>");
    data.doCommand("nunmap <F9>");
    data.doCommand("nunmap gqq");
    data.doCommand("nunmap gq");
    data.doCommand("nunmap gw");
    data.doCommand("nunmap gww");
    data.doCommand("unlet! g:hit");
}

void FakeVimTester::test_vim_substitute_print_flags()
{
    // The ":substitute" flags that print, and the complaint about one that is
    // no flag at all. All values measured in Vim 9.1.
    TestData data;
    setup(&data);
    QString message;
    data.handler->commandBufferChanged.set(
        [&](const QString &msg, int, int, int) {
            if (!msg.isEmpty() && !msg.startsWith("--"))
                message = msg;
        });
    const auto run = [&](const QString &command) {
        message.clear();
        data.doCommand(command);
        return message;
    };

    // "p" prints the line as it now stands.
    data.setText("ab");
    QCOMPARE(run("%substitute/a/X/p"), QLatin1String("Xb"));

    // "#" puts the line number in front of it, right-aligned in three.
    data.setText("ab");
    QCOMPARE(run("%substitute/a/X/#"), QLatin1String("  1 Xb"));

    // "l" prints it the way ":list" shows one: a tab stands as "^I", and the
    // end of the line is marked.
    data.setText("a\tb");
    QCOMPARE(run("%substitute/a/X/l"), QLatin1String("X^Ib$"));
    data.setText("ab  ");
    QCOMPARE(run("%substitute/a/X/l"), QLatin1String("Xb  $"));

    // The LAST line a substitution was made in is the one printed.
    data.setText("a1" N "a2" N "a3");
    QCOMPARE(run("%substitute/a/X/p"), QLatin1String("X3"));

    // With "n" nothing is changed, so the line that matched is printed as it
    // stands - and the count is still reported.
    data.setText("ab");
    QCOMPARE(run("%substitute/a/X/np"), QLatin1String("ab"));
    QCOMPARE(data.text(), QString("ab"));

    // A flag that is no flag is reported rather than passed over.
    data.setText("ab");
    QVERIFY(run("%substitute/a/X/Q").contains("E488"));
    QCOMPARE(data.text(), QString("ab"));
    // ...and the message names what was trailing.
    data.setText("ab");
    QVERIFY(run("%substitute/a/X/gQ").contains("Q"));

    // The flags that are real are still taken, including the ones this engine
    // does not act on.
    data.setText("aa");
    data.doCommand("%substitute/a/X/g");
    QCOMPARE(data.text(), QString("XX"));
    data.setText("ab");
    QVERIFY(!run("%substitute/a/X/r").contains("E488"));
    data.setText("ab");
    QVERIFY(!run("%substitute/a/X/c").contains("E488"));

    // A trailing count is not mistaken for a flag, so it draws no complaint.
    // What it then reaches is asserted in test_vim_substitute_count().
    data.setText("ab" N "ab" N "ab");
    QVERIFY(!run("1substitute/a/X/ 2").contains("E488"));
}

void FakeVimTester::test_vim_script_more_stubs_and_region()
{
    // assert_fails(), a handful more "nothing here does that" answers, and
    // getregion()/getregionpos()/readdirex(). All values measured in Vim 9.1.
    TestData data;
    setup(&data);
    QString message;
    data.handler->commandBufferChanged.set(
        [&](const QString &msg, int, int, int) {
            if (!msg.isEmpty() && !msg.startsWith("--"))
                message = msg;
        });
    const auto value = [&](const QString &expr) {
        message.clear();
        data.doCommand("echo " + expr);
        return message;
    };

    data.setText("alpha beta" N "gamma delta" N "epsilon zeta");

    // assert_fails() PASSES when the command throws, and fails when it does
    // not - the opposite sense from most of the assert_*() family.
    data.doCommand("let v:errors = []");
    QCOMPARE(value("assert_fails('echo 1')"), QLatin1String("1"));
    data.doCommand("let v:errors = []");
    QCOMPARE(value("assert_fails('call NoSuchFuncXyz()')"), QLatin1String("0"));
    QCOMPARE(value("len(v:errors)"), QLatin1String("0"));
    // What is checked is that a PARTICULAR error came up.
    data.doCommand("let v:errors = []");
    QCOMPARE(value("assert_fails('call NoSuchFuncXyz()', 'E999')"),
             QLatin1String("1"));
    data.doCommand("let v:errors = []");
    QCOMPARE(value("assert_fails('call NoSuchFuncXyz()', 'E117')"),
             QLatin1String("0"));

    // No diff mode is ever on.
    QCOMPARE(value("diff_filler(1)"), QLatin1String("0"));
    QCOMPARE(value("diff_hlID(1, 1)"), QLatin1String("0"));
    QCOMPARE(value("getcellpixels()"), QLatin1String("[]"));
    QCOMPARE(value("getmouseshape()"), QLatin1String(""));

    // One tab, holding the one window there is.
    QCOMPARE(value("len(gettabinfo())"), QLatin1String("1"));
    QCOMPARE(value("gettabinfo()[0].tabnr"), QLatin1String("1"));
    QCOMPARE(value("len(gettabinfo()[0].windows)"), QLatin1String("1"));

    // No tag stack is ever kept.
    QCOMPARE(value("gettagstack()"),
             QLatin1String("{'curidx': 1, 'items': [], 'length': 0}"));
    QCOMPARE(value("settagstack(1, {})"), QLatin1String("0"));

    // No server is ever started, so nothing can ever reply, and none is ever
    // listed.
    QVERIFY(value("server2client('x', 'y')").contains("E1565"));
    QCOMPARE(value("serverlist()"), QLatin1String(""));

    // No conceal support, so nothing is ever concealed.
    QCOMPARE(value("synconcealed(1, 1)"), QLatin1String("[0, '', 0]"));

    QCOMPARE(value("terminalprops().mouse"), QLatin1String("u"));
    QCOMPARE(value("cmdcomplete_info()"), QLatin1String("{}"));

    // getregion(): a single-line charwise selection.
    data.doKeys("gg0vey");
    QCOMPARE(value("getregion(getpos(\"'<\"), getpos(\"'>\"))"),
             QLatin1String("['alpha']"));
    // A linewise one spans whole lines.
    data.doKeys("gg0Vjy");
    QCOMPARE(value("getregion(getpos(\"'<\"), getpos(\"'>\"))"),
             QLatin1String("['alpha beta', 'gamma delta']"));
    // A blockwise one takes the same column from each line.
    data.doKeys("gg0<c-v>jly");
    QCOMPARE(value("getregion(getpos(\"'<\"), getpos(\"'>\"))"),
             QLatin1String("['al', 'ga']"));
    // An explicit type overrides what the last visual mode was.
    data.doKeys("gg0vey");
    QCOMPARE(value("getregion(getpos(\"'<\"), getpos(\"'>\"), {'type': 'V'})"),
             QLatin1String("['alpha beta']"));

    // getregionpos(): the one shape actually measured, a single charwise line.
    data.doKeys("gg0vey");
    QCOMPARE(value("getregionpos(getpos(\"'<\"), getpos(\"'>\"))"),
             QLatin1String("[[[0, 1, 1, 0], [0, 1, 5, 0]]]"));

    // readdirex(): the same names readdir() gives, with details alongside.
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    QFile f(dir.path() + "/probe.txt");
    QVERIFY(f.open(QIODevice::WriteOnly));
    f.write("hello\n");
    f.close();
    QVERIFY(QDir().mkpath(dir.path() + "/subdir"));
    QCOMPARE(value("sort(map(readdirex('" + dir.path() + "'), {_, v -> v.name}))"),
             QLatin1String("['probe.txt', 'subdir']"));
    QCOMPARE(value("sort(keys(readdirex('" + dir.path() + "')[0]))"),
             QLatin1String("['group', 'name', 'perm', 'size', 'time', 'type', 'user']"));
    data.doCommand("let g:rd = readdirex('" + dir.path() + "')");
    data.doCommand("let g:probe = filter(copy(g:rd), {_, v -> v.name ==# 'probe.txt'})[0]");
    QCOMPARE(value("g:probe.type"), QLatin1String("file"));
    QCOMPARE(value("g:probe.size"), QLatin1String("6"));
    data.doCommand("let g:subdir = filter(copy(g:rd), {_, v -> v.name ==# 'subdir'})[0]");
    QCOMPARE(value("g:subdir.type"), QLatin1String("dir"));

    for (const QString &fn : QStringList{"assert_fails", "diff_filler",
                                         "diff_hlID", "getcellpixels",
                                         "getmouseshape", "gettabinfo",
                                         "gettagstack", "settagstack",
                                         "server2client", "serverlist",
                                         "synconcealed", "terminalprops",
                                         "cmdcomplete_info", "getregion",
                                         "getregionpos", "readdirex",
                                         "getscriptinfo"}) {
        QCOMPARE(value("exists('*" + fn + "')"), QLatin1String("1"));
    }

    // getscriptinfo(): the registry sourcing keeps, walked by id. Shared with
    // every other test slot the way the command history is, so only THIS
    // script's own entry is asserted, found by name rather than by number.
    QTemporaryDir scriptDir;
    QVERIFY(scriptDir.isValid());
    const QString scriptPath = scriptDir.path() + "/gsi_probe.vim";
    QFile sf(scriptPath);
    QVERIFY(sf.open(QIODevice::WriteOnly));
    sf.write("let g:gsi_touched = 1\n");
    sf.close();
    data.doCommand("source " + scriptPath);
    data.doCommand("let g:gsi = filter(getscriptinfo(), {_, v -> v.name ==# '"
                   + scriptPath + "'})");
    QCOMPARE(value("len(g:gsi)"), QLatin1String("1"));
    QCOMPARE(value("g:gsi[0].version"), QLatin1String("1"));
    QCOMPARE(value("g:gsi[0].sourced"), QLatin1String("0"));
    QCOMPARE(value("g:gsi[0].autoload"), QLatin1String("v:false"));
    QCOMPARE(value("g:gsi[0].sid > 0"), QLatin1String("1"));
    QCOMPARE(value("type(getscriptinfo()) == v:t_list"), QLatin1String("1"));
    // A script under an "autoload" directory is named as one.
    QVERIFY(QDir().mkpath(scriptDir.path() + "/autoload"));
    const QString autoPath = scriptDir.path() + "/autoload/gsi_auto.vim";
    QFile af(autoPath);
    QVERIFY(af.open(QIODevice::WriteOnly));
    af.write("let g:gsi_auto_touched = 1\n");
    af.close();
    data.doCommand("source " + autoPath);
    data.doCommand("let g:gsiauto = filter(getscriptinfo(), {_, v -> v.name ==# '"
                   + autoPath + "'})");
    QCOMPARE(value("g:gsiauto[0].autoload"), QLatin1String("v:true"));

    data.doCommand("unlet! g:rd g:probe g:subdir g:gsi g:gsi_touched g:gsiauto"
                   " g:gsi_auto_touched");
    data.doCommand("let v:errors = []");
}

void FakeVimTester::test_vim_script_directory_and_window_stubs()
{
    // chdir(), and the family of things this engine has no real answer for -
    // window position, swap files, cellwidths, cscope, tags, mouse position,
    // and the single-window layout functions. All values measured in Vim 9.1.
    TestData data;
    setup(&data);
    QString message;
    data.handler->commandBufferChanged.set(
        [&](const QString &msg, int, int, int) {
            if (!msg.isEmpty() && !msg.startsWith("--"))
                message = msg;
        });
    const auto value = [&](const QString &expr) {
        message.clear();
        data.doCommand("echo " + expr);
        return message;
    };

    data.setText("x");

    // haslocaldir() is always zero - nothing here narrows a directory to one
    // window on its own.
    QCOMPARE(value("haslocaldir()"), QLatin1String("0"));

    // chdir() changes the real working directory and hands back the one it
    // was before, so it is put back afterwards - this is shared with every
    // other test slot in the process.
    const QString before = value("getcwd()");
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    QCOMPARE(value("chdir('" + dir.path() + "') != ''"), QLatin1String("1"));
    QCOMPARE(value("getcwd()"), dir.path());
    QVERIFY(value("chdir('" + before + "')").contains(dir.path()));
    QCOMPARE(value("getcwd()"), before);
    // A directory that is not there is refused rather than followed.
    QVERIFY(value("chdir('/no/such/directory/xyz')").contains("E344"));
    QCOMPARE(value("getcwd()"), before);

    // undofile() names where an undo file WOULD go, in the file's own
    // directory - a dot in front, "un~" behind.
    QCOMPARE(value("undofile('" + dir.path() + "/probe.txt')"),
             QLatin1String(dir.path().toUtf8() + "/.probe.txt.un~"));

    // Nothing here knows where the window sits, or what its font is, so both
    // answer what Vim answers when it does not know either.
    QCOMPARE(value("getwinpos()"), QLatin1String("[-1, -1]"));
    QCOMPARE(value("getwinposx()"), QLatin1String("-1"));
    QCOMPARE(value("getwinposy()"), QLatin1String("-1"));
    QCOMPARE(value("getfontname()"), QLatin1String(""));

    // Nothing is ever mid command-line-completion here.
    QCOMPARE(value("getcmdcomplpat()"), QLatin1String(""));
    QCOMPARE(value("getcmdcompltype()"), QLatin1String(""));

    // Character widths are Qt's question, not this engine's.
    QCOMPARE(value("getcellwidths()"), QLatin1String("[]"));
    QCOMPARE(value("setcellwidths([])"), QLatin1String("0"));

    // No cscope connection, no tag files, no swap file, ever.
    QCOMPARE(value("cscope_connection()"), QLatin1String("0"));
    QCOMPARE(value("tagfiles()"), QLatin1String("[]"));
    QCOMPARE(value("swapname('%')"), QLatin1String(""));
    QCOMPARE(value("swapinfo('nosuchfile').error"), QLatin1String("Cannot open file"));
    QCOMPARE(value("swapfilelist()"), QLatin1String("[]"));

    // Nothing here is mid mouse-click, so every part of where one would be
    // is zero.
    QCOMPARE(value("sort(keys(getmousepos()))"),
             QLatin1String("['coladd', 'column', 'line', 'screencol', "
                           "'screenrow', 'wincol', 'winid', 'winrow']"));
    QCOMPARE(value("getmousepos().line"), QLatin1String("0"));

    // One window is all there ever is: moving a separator or a status line
    // succeeds trivially, since the window named is real, but there is never
    // a second window to move INTO.
    QCOMPARE(value("win_move_separator(0, 0)"), QLatin1String("1"));
    QCOMPARE(value("win_move_statusline(0, 0)"), QLatin1String("1"));
    QVERIFY(value("win_splitmove(0, 0)").contains("E957"));

    QCOMPARE(value("wildtrigger()"), QLatin1String("0"));
    QCOMPARE(value("foreground()"), QLatin1String("0"));

    // No real declaration search runs, so it is always "not found" and never
    // a false positive.
    QCOMPARE(value("searchdecl('nothing')"), QLatin1String("1"));

    // There are no classes, so nothing is ever an Object.
    QVERIFY(value("instanceof(1, 'Foo')").contains("E616"));

    // hlget()/hlset(): a group this engine knows of but has never coloured
    // answers the way Vim answers for one nothing has styled - known, but
    // "cleared". One that was never named at all is an empty list.
    QCOMPARE(value("hlget('Comment')[0].name"), QLatin1String("Comment"));
    QCOMPARE(value("hlget('Comment')[0].cleared"), QLatin1String("v:true"));
    QCOMPARE(value("hlget('Comment')[0].id > 0"), QLatin1String("1"));
    QCOMPARE(value("hlget('NoSuchGroupXyz')"), QLatin1String("[]"));
    QCOMPARE(value("len(hlget()) > 1"), QLatin1String("1"));
    QCOMPARE(value("hlset([])"), QLatin1String("0"));

    for (const QString &fn : QStringList{"haslocaldir", "chdir", "undofile",
                                         "getwinpos", "getwinposx", "getwinposy",
                                         "getfontname", "getcmdcomplpat",
                                         "getcmdcompltype", "getcellwidths",
                                         "setcellwidths", "cscope_connection",
                                         "tagfiles", "swapname", "swapinfo",
                                         "swapfilelist", "getmousepos",
                                         "win_move_separator",
                                         "win_move_statusline", "win_splitmove",
                                         "wildtrigger", "foreground",
                                         "searchdecl", "instanceof", "hlget",
                                         "hlset"}) {
        QCOMPARE(value("exists('*" + fn + "')"), QLatin1String("1"));
    }
}

void FakeVimTester::test_vim_command_cd()
{
    // ":cd"/":chdir" (the same command under its long name), ":lcd" and
    // ":tcd" (which alias :cd - there is no window/tab of this engine's own
    // to scope them to, so they are told apart only by which DirChanged
    // pattern they fire) change the same process-global directory chdir()
    // already does. "-" returns to whatever :cd/:lcd/:tcd last left, and no
    // argument at all goes home. Values taken from Vim 9.1. Shared with
    // every other test slot in the process, like chdir() itself - restored
    // at the end.
    TestData data;
    setup(&data);
    QString message;
    data.handler->commandBufferChanged.set(
        [&](const QString &msg, int, int, int) {
            if (!msg.isEmpty() && !msg.startsWith("--"))
                message = msg;
        });
    const auto value = [&](const QString &expr) {
        message.clear();
        data.doCommand("echo " + expr);
        return message;
    };

    const QString before = value("getcwd()");
    QTemporaryDir dirA;
    QVERIFY(dirA.isValid());
    QTemporaryDir dirB;
    QVERIFY(dirB.isValid());

    data.doCommand("cd " + dirA.path());
    QCOMPARE(value("getcwd()"), dirA.path());

    message.clear();
    data.doCommand("pwd");
    QCOMPARE(message, dirA.path());

    data.doCommand("chdir " + dirB.path());
    QCOMPARE(value("getcwd()"), dirB.path());

    // "-" returns to the previous one, and toggles.
    data.doCommand("cd -");
    QCOMPARE(value("getcwd()"), dirA.path());
    data.doCommand("cd -");
    QCOMPARE(value("getcwd()"), dirB.path());

    data.doCommand("lcd " + dirA.path());
    QCOMPARE(value("getcwd()"), dirA.path());
    data.doCommand("tcd " + dirB.path());
    QCOMPARE(value("getcwd()"), dirB.path());

    // No argument at all goes home, as real Vim does on Unix.
    data.doCommand("cd");
    QCOMPARE(value("getcwd()"), QDir::homePath());

    // A directory that is not there is refused.
    message.clear();
    data.doCommand("cd /no/such/directory/xyz");
    QVERIFY(message.contains("E344"));

    // DirChangedPre carries the new directory in v:event; both it and
    // DirChanged are matched against the SCOPE, not a file name.
    data.doCommand("let g:dc = []");
    data.doCommand("autocmd FvCd DirChangedPre * call add(g:dc,"
                    " 'pre:' . expand('<amatch>') . ':' . get(v:event, 'directory', '?'))");
    data.doCommand("autocmd FvCd DirChanged * call add(g:dc, 'post:' . expand('<amatch>'))");

    data.doCommand("cd " + dirA.path());
    QCOMPARE(value("string(g:dc)"),
             QString("['pre:global:" + dirA.path() + "', 'post:global']"));
    data.doCommand("let g:dc = []");
    data.doCommand("lcd " + dirB.path());
    QCOMPARE(value("string(g:dc)"),
             QString("['pre:window:" + dirB.path() + "', 'post:window']"));
    data.doCommand("let g:dc = []");
    data.doCommand("tcd " + dirA.path());
    QCOMPARE(value("string(g:dc)"),
             QString("['pre:tabpage:" + dirA.path() + "', 'post:tabpage']"));

    // Known to autocmd_add() now, where ":autocmd" would have taken either
    // name whether it existed or not.
    QCOMPARE(value("autocmd_add([{'group': 'FvCd', 'event': 'DirChangedPre',"
                   " 'pattern': '*', 'cmd': 'echo 1'}])"), QLatin1String("v:true"));
    QCOMPARE(value("autocmd_add([{'group': 'FvCd', 'event': 'DirChanged',"
                   " 'pattern': '*', 'cmd': 'echo 1'}])"), QLatin1String("v:true"));

    data.doCommand("autocmd! FvCd");
    data.doCommand("unlet! g:dc");
    data.doCommand("cd " + before);
}

void FakeVimTester::test_vim_script_misc_builtins()
{
    // hlexists/hlID and kin, gettext/ngettext/bindtextdomain, err_teapot, and
    // the "nothing is going on" family: pumvisible, wildmenumode, eventhandler,
    // garbagecollect, id(). All values measured in Vim 9.1.
    TestData data;
    setup(&data);
    QString message;
    data.handler->commandBufferChanged.set(
        [&](const QString &msg, int, int, int) {
            if (!msg.isEmpty() && !msg.startsWith("--"))
                message = msg;
        });
    const auto value = [&](const QString &expr) {
        message.clear();
        data.doCommand("echo " + expr);
        return message;
    };

    data.setText("x");

    // A group this engine knows (it treats syntax as always active) exists;
    // one that was never named does not.
    QCOMPARE(value("hlexists('Comment')"), QLatin1String("1"));
    QCOMPARE(value("highlight_exists('Comment')"), QLatin1String("1"));
    QCOMPARE(value("hlexists('NoSuchGroupXyz')"), QLatin1String("0"));
    // hlID() answers a POSITIVE id for one that exists, zero for one that does
    // not, and highlightID() is the older name for the same question.
    QCOMPARE(value("hlID('Comment') > 0"), QLatin1String("1"));
    QCOMPARE(value("hlID('NoSuchGroupXyz')"), QLatin1String("0"));
    QCOMPARE(value("highlightID('Comment') == hlID('Comment')"), QLatin1String("1"));
    // Two different groups get two different ids.
    QCOMPARE(value("hlID('Comment') == hlID('String')"), QLatin1String("0"));
    // Nothing links one group to another here, so synIDtrans() hands back
    // what it was given.
    QCOMPARE(value("synIDtrans(hlID('Comment')) == hlID('Comment')"),
             QLatin1String("1"));
    QCOMPARE(value("synIDtrans(0)"), QLatin1String("0"));

    // No message catalog is read, so the text comes back untranslated.
    QCOMPARE(value("gettext('hello')"), QLatin1String("hello"));
    QCOMPARE(value("ngettext('one', 'many', 1)"), QLatin1String("one"));
    QCOMPARE(value("ngettext('one', 'many', 5)"), QLatin1String("many"));
    QCOMPARE(value("bindtextdomain('x', '/tmp')"), QLatin1String("v:true"));

    // err_teapot() is a fixed, catchable error meant to be raised on purpose.
    QVERIFY(value("err_teapot()").contains("E418"));
    QVERIFY(value("err_teapot(1)").contains("E503"));
    data.doCommand("let g:caught = ''");
    data.doCommand("try | call err_teapot() | catch | let g:caught = v:exception | endtry");
    QCOMPARE(value("g:caught =~# 'E418'"), QLatin1String("1"));

    // Nothing here is ever the case, so a script asking may go on.
    QCOMPARE(value("pumvisible()"), QLatin1String("0"));
    QCOMPARE(value("wildmenumode()"), QLatin1String("0"));
    QCOMPARE(value("eventhandler()"), QLatin1String("0"));
    QCOMPARE(value("garbagecollect()"), QLatin1String("0"));

    // id(): a container has an identity, a scalar has none - measured as an
    // empty string rather than zero. The same list twice over shares its id,
    // as copies of one Vim List do.
    QCOMPARE(value("id(1)"), QLatin1String(""));
    QCOMPARE(value("id('x')"), QLatin1String(""));
    data.doCommand("let g:l = [1, 2]");
    QCOMPARE(value("id(g:l) != ''"), QLatin1String("1"));
    QCOMPARE(value("id(g:l) ==# id(g:l)"), QLatin1String("1"));
    data.doCommand("let g:l2 = g:l");
    QCOMPARE(value("id(g:l) ==# id(g:l2)"), QLatin1String("1"));
    data.doCommand("let g:l3 = [1, 2]");
    QCOMPARE(value("id(g:l) ==# id(g:l3)"), QLatin1String("0"));

    for (const QString &fn : QStringList{"hlexists", "highlight_exists", "hlID",
                                         "highlightID", "synIDtrans", "gettext",
                                         "ngettext", "bindtextdomain",
                                         "err_teapot", "pumvisible", "wildmenumode",
                                         "eventhandler", "garbagecollect", "id"}) {
        QCOMPARE(value("exists('*" + fn + "')"), QLatin1String("1"));
    }
    data.doCommand("unlet! g:l g:l2 g:l3 g:caught");
}

void FakeVimTester::test_vim_script_assert_functions()
{
    // The assert_*() family, and v:errors. All values measured in Vim 9.1: each
    // pushes onto v:errors the same "command line..." prefix an uncaught
    // exception gets, then ": " and the message, and returns 1 on failure.
    TestData data;
    setup(&data);
    QString message;
    data.handler->commandBufferChanged.set(
        [&](const QString &msg, int, int, int) {
            if (!msg.isEmpty() && !msg.startsWith("--"))
                message = msg;
        });
    const auto value = [&](const QString &expr) {
        message.clear();
        data.doCommand("echo " + expr);
        return message;
    };
    const auto lastError = [&] { return value("v:errors[-1]"); };
    const auto reset = [&] { data.doCommand("let v:errors = []"); };

    data.setText("x");

    // A pass returns 0 and adds nothing.
    reset();
    QCOMPARE(value("assert_equal(1, 1)"), QLatin1String("0"));
    QCOMPARE(value("len(v:errors)"), QLatin1String("0"));

    // A failure returns 1 and describes what it expected, quoting a string the
    // way string() does.
    reset();
    QCOMPARE(value("assert_equal(1, 2)"), QLatin1String("1"));
    QVERIFY(lastError().contains("Expected 1 but got 2"));
    reset();
    QCOMPARE(value("assert_equal('a', 'b')"), QLatin1String("1"));
    QVERIFY(lastError().contains("Expected 'a' but got 'b'"));

    // A message argument comes BEFORE the "Expected" text.
    reset();
    data.doCommand("call assert_equal(1, 2, 'mine')");
    QVERIFY(lastError().contains("mine: Expected 1 but got 2"));

    // Equality is deep, and TYPE-SENSITIVE: a Number and a Float holding the
    // same value are not equal, matching what "==" does for a List already.
    reset();
    QCOMPARE(value("assert_equal(1.0, 1)"), QLatin1String("1"));
    // But a List and a Dict compare by their own content.
    reset();
    QCOMPARE(value("assert_equal([1, 2], [1, 2])"), QLatin1String("0"));
    QCOMPARE(value("assert_equal({'a': 1}, {'a': 1})"), QLatin1String("0"));

    // assert_notequal is the mirror, and names only what was expected NOT to
    // equal, not what it actually was.
    reset();
    QCOMPARE(value("assert_notequal(1, 1)"), QLatin1String("1"));
    QVERIFY(lastError().contains("Expected not equal to 1"));
    reset();
    QCOMPARE(value("assert_notequal(1, 2)"), QLatin1String("0"));

    // assert_true/false take only a Number or a Bool - a STRING that reads as
    // the right number still fails, which is the part easy to get wrong.
    reset();
    QCOMPARE(value("assert_true(v:true)"), QLatin1String("0"));
    QCOMPARE(value("assert_true(2)"), QLatin1String("0")); // any nonzero Number
    QCOMPARE(value("assert_true(0)"), QLatin1String("1"));
    reset();
    QCOMPARE(value("assert_true('1')"), QLatin1String("1"));
    QVERIFY(lastError().contains("Expected True but got '1'"));
    reset();
    QCOMPARE(value("assert_false(v:false)"), QLatin1String("0"));
    QCOMPARE(value("assert_false(1)"), QLatin1String("1"));

    // assert_match/notmatch use a Vim pattern, not a literal string.
    reset();
    QCOMPARE(value("assert_match('al', 'alpha')"), QLatin1String("0"));
    QCOMPARE(value("assert_match('zz', 'alpha')"), QLatin1String("1"));
    QVERIFY(lastError().contains("Pattern 'zz' does not match 'alpha'"));
    reset();
    QCOMPARE(value("assert_notmatch('zz', 'alpha')"), QLatin1String("0"));
    QCOMPARE(value("assert_notmatch('al', 'alpha')"), QLatin1String("1"));
    QVERIFY(lastError().contains("Pattern 'al' does match 'alpha'"));

    // assert_inrange is inclusive at both ends.
    reset();
    QCOMPARE(value("assert_inrange(1, 3, 2)"), QLatin1String("0"));
    QCOMPARE(value("assert_inrange(1, 3, 1)"), QLatin1String("0"));
    QCOMPARE(value("assert_inrange(1, 3, 3)"), QLatin1String("0"));
    QCOMPARE(value("assert_inrange(1, 3, 9)"), QLatin1String("1"));
    QVERIFY(lastError().contains("Expected range 1 - 3, but got 9"));

    // assert_report ALWAYS fails, and its argument is the whole message.
    reset();
    QCOMPARE(value("assert_report('boom')"), QLatin1String("1"));
    QVERIFY(lastError().contains("boom"));

    // assert_exception looks inside v:exception, which is only there in a
    // :catch; outside one it is its own failure.
    reset();
    QCOMPARE(value("assert_exception('nope')"), QLatin1String("1"));
    QVERIFY(lastError().contains("v:exception is not set"));
    reset();
    data.doCommand("try | throw 'my custom error' | catch | call assert_exception('custom') | endtry");
    QCOMPARE(value("len(v:errors)"), QLatin1String("0"));
    reset();
    data.doCommand("try | throw 'my custom error' | catch | call assert_exception('nope') | endtry");
    QCOMPARE(value("len(v:errors)"), QLatin1String("1"));

    // v:errors accumulates across separate calls rather than being replaced.
    reset();
    data.doCommand("call assert_equal(1, 2)");
    data.doCommand("call assert_report('second')");
    QCOMPARE(value("len(v:errors)"), QLatin1String("2"));

    // assert_equalfile compares the content of two files.
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString pathA = dir.path() + "/a.txt";
    const QString pathB = dir.path() + "/b.txt";
    QFile fa(pathA);
    QVERIFY(fa.open(QIODevice::WriteOnly));
    fa.write("same\n");
    fa.close();
    QFile fb(pathB);
    QVERIFY(fb.open(QIODevice::WriteOnly));
    fb.write("same\n");
    fb.close();
    reset();
    QCOMPARE(value("assert_equalfile('" + pathA + "', '" + pathB + "')"),
             QLatin1String("0"));
    QFile fc(pathB);
    QVERIFY(fc.open(QIODevice::WriteOnly | QIODevice::Truncate));
    fc.write("different\n");
    fc.close();
    reset();
    QCOMPARE(value("assert_equalfile('" + pathA + "', '" + pathB + "')"),
             QLatin1String("1"));

    for (const QString &fn : QStringList{"assert_equal", "assert_notequal",
                                         "assert_true", "assert_false",
                                         "assert_match", "assert_notmatch",
                                         "assert_inrange", "assert_report",
                                         "assert_exception", "assert_equalfile"}) {
        QCOMPARE(value("exists('*" + fn + "')"), QLatin1String("1"));
    }
    reset();
}

void FakeVimTester::test_vim_script_setmatches_and_state()
{
    // setmatches() puts back what getmatches() handed out; matcharg() answers
    // only for the three ":match" commands; state() and getcharmod() answer
    // that nothing is going on. All measured in Vim 9.1.
    TestData data;
    setup(&data);
    QString message;
    data.handler->commandBufferChanged.set(
        [&](const QString &msg, int, int, int) {
            if (!msg.isEmpty() && !msg.startsWith("--"))
                message = msg;
        });
    const auto value = [&](const QString &expr) {
        message.clear();
        data.doCommand("echo " + expr);
        return message;
    };

    data.setText("alpha beta alpha");
    data.doCommand("call clearmatches()");

    // Round trip: what getmatches() hands out goes back in.
    data.doCommand("call matchadd('Search', 'alpha')");
    data.doCommand("call matchadd('Search', 'beta')");
    QCOMPARE(value("len(getmatches())"), QLatin1String("2"));
    data.doCommand("let g:m = getmatches()");
    data.doCommand("call clearmatches()");
    QCOMPARE(value("len(getmatches())"), QLatin1String("0"));
    QCOMPARE(value("setmatches(g:m)"), QLatin1String("0"));
    QCOMPARE(value("len(getmatches())"), QLatin1String("2"));
    // What came back is what went in.
    QCOMPARE(value("getmatches()[0].group"), QLatin1String("Search"));
    QCOMPARE(value("getmatches()[0].pattern"), QLatin1String("alpha"));
    QCOMPARE(value("string(getmatches()) ==# string(g:m)"), QLatin1String("1"));
    // An empty list leaves none behind, and that is not a failure.
    QCOMPARE(value("setmatches([])"), QLatin1String("0"));
    QCOMPARE(value("len(getmatches())"), QLatin1String("0"));
    // Something that is no list of dicts is refused with minus one.
    QCOMPARE(value("setmatches('nonsense')"), QLatin1String("-1"));
    QCOMPARE(value("setmatches(['nonsense'])"), QLatin1String("-1"));

    // matcharg() answers a PAIR for one, two and three, and an empty list for
    // anything else - which is how a script tells "no match" from "no such
    // number".
    QCOMPARE(value("matcharg(1)"), QLatin1String("['', '']"));
    QCOMPARE(value("matcharg(3)"), QLatin1String("['', '']"));
    QCOMPARE(value("matcharg(4)"), QLatin1String("[]"));
    QCOMPARE(value("matcharg(0)"), QLatin1String("[]"));
    QCOMPARE(value("len(matcharg(1))"), QLatin1String("2"));
    QCOMPARE(value("len(matcharg(9))"), QLatin1String("0"));

    // Nothing is going on, and both say so with a value of the right type.
    QCOMPARE(value("state()"), QLatin1String(""));
    QCOMPARE(value("type(state()) == v:t_string"), QLatin1String("1"));
    QCOMPARE(value("getcharmod()"), QLatin1String("0"));
    QCOMPARE(value("type(getcharmod()) == v:t_number"), QLatin1String("1"));

    for (const QString &fn : QStringList{"setmatches", "matcharg", "state",
                                         "getcharmod"}) {
        QCOMPARE(value("exists('*" + fn + "')"), QLatin1String("1"));
    }
    data.doCommand("call clearmatches()");
    data.doCommand("unlet! g:m");
}

void FakeVimTester::test_vim_script_fold_queries()
{
    // The fold questions. There is no folding here, so each answers what Vim
    // answers where the line asked about is in no fold - measured in Vim 9.1 on
    // a buffer with no folds. The point is that a script asking may go on
    // rather than stop with an unknown function.
    TestData data;
    setup(&data);
    QString message;
    data.handler->commandBufferChanged.set(
        [&](const QString &msg, int, int, int) {
            if (!msg.isEmpty() && !msg.startsWith("--"))
                message = msg;
        });
    const auto value = [&](const QString &expr) {
        message.clear();
        data.doCommand("echo " + expr);
        return message;
    };

    data.setText("a" N "b" N "c");

    // A line in no fold is minus one, not zero - which is what tells "no fold"
    // from "the first line".
    QCOMPARE(value("foldclosed(1)"), QLatin1String("-1"));
    QCOMPARE(value("foldclosed(99)"), QLatin1String("-1"));
    QCOMPARE(value("foldclosedend(1)"), QLatin1String("-1"));
    QCOMPARE(value("type(foldclosed(1)) == v:t_number"), QLatin1String("1"));

    // No depth, and no text.
    QCOMPARE(value("foldlevel(1)"), QLatin1String("0"));
    QCOMPARE(value("foldlevel(99)"), QLatin1String("0"));
    QCOMPARE(value("foldtext()"), QLatin1String(""));
    QCOMPARE(value("foldtextresult(1)"), QLatin1String(""));
    QCOMPARE(value("foldtextresult(99)"), QLatin1String(""));
    QCOMPARE(value("type(foldtext()) == v:t_string"), QLatin1String("1"));

    for (const QString &fn : QStringList{"foldclosed", "foldclosedend",
                                         "foldlevel", "foldtext",
                                         "foldtextresult"}) {
        QCOMPARE(value("exists('*" + fn + "')"), QLatin1String("1"));
    }
}

void FakeVimTester::test_vim_script_arglist()
{
    // The argument list. One file is open at a time here, so the list holds
    // that one. Shapes and edges measured in Vim 9.1.
    TestData data;
    setup(&data);
    QString message;
    data.handler->commandBufferChanged.set(
        [&](const QString &msg, int, int, int) {
            if (!msg.isEmpty() && !msg.startsWith("--"))
                message = msg;
        });
    const auto value = [&](const QString &expr) {
        message.clear();
        data.doCommand("echo " + expr);
        return message;
    };

    data.setText("x");
    data.doCommand("file thefile.txt");

    QCOMPARE(value("argc()"), QLatin1String("1"));
    QCOMPARE(value("type(argc()) == v:t_number"), QLatin1String("1"));
    QCOMPARE(value("argidx()"), QLatin1String("0"));
    QCOMPARE(value("arglistid()"), QLatin1String("0"));
    QCOMPARE(value("type(argv()) == v:t_list"), QLatin1String("1"));
    QCOMPARE(value("len(argv())"), QLatin1String("1"));
    QCOMPARE(value("argv()[0]"), QLatin1String("thefile.txt"));
    // argv({nr}) names one of them, and a number past the end is nothing
    // rather than an error.
    QCOMPARE(value("argv(0)"), QLatin1String("thefile.txt"));
    QCOMPARE(value("argv(9)"), QLatin1String(""));
    // The list and the numbered form agree.
    QCOMPARE(value("argv(0) ==# argv()[0]"), QLatin1String("1"));
    QCOMPARE(value("argc() == len(argv())"), QLatin1String("1"));

    // The older names for the buffer questions answer as the newer ones do.
    // Measured in Vim 9.1, where they are the same functions under two names.
    QCOMPARE(value("buffer_number('') == bufnr('')"), QLatin1String("1"));
    QCOMPARE(value("buffer_name('%') ==# bufname('%')"), QLatin1String("1"));
    QCOMPARE(value("buffer_exists(bufnr('')) == bufexists(bufnr(''))"),
             QLatin1String("1"));
    QCOMPARE(value("buffer_exists(bufnr(''))"), QLatin1String("1"));
    // A number nothing goes by is not there.
    QCOMPARE(value("buffer_exists(9999)"), QLatin1String("0"));
    // The highest number handed out is at least this buffer's own.
    QCOMPARE(value("last_buffer_nr() >= bufnr('')"), QLatin1String("1"));
    QCOMPARE(value("type(last_buffer_nr()) == v:t_number"), QLatin1String("1"));

    // Answering is one thing and being KNOWN is another: the two live in
    // different places here, so both are asked about.
    for (const QString &fn : QStringList{"argc", "argidx", "argv", "arglistid",
                                         "buffer_exists", "buffer_name",
                                         "buffer_number", "last_buffer_nr",
                                         "systemlist", "hostname", "getpid",
                                         "file_readable", "filewritable",
                                         "setfperm"}) {
        QCOMPARE(value("exists('*" + fn + "')"), QLatin1String("1"));
    }
}

void FakeVimTester::test_vim_script_system_functions()
{
    // hostname(), getpid() and systemlist(). Measured in Vim 9.1.
    TestData data;
    setup(&data);
    QString message;
    data.handler->commandBufferChanged.set(
        [&](const QString &msg, int, int, int) {
            if (!msg.isEmpty() && !msg.startsWith("--"))
                message = msg;
        });
    const auto value = [&](const QString &expr) {
        message.clear();
        data.doCommand("echo " + expr);
        return message;
    };
    // Nothing here reaches a shell, so the command is answered from a table.
    data.handler->processOutput.set(
        [](const QString &command, const QString &input, QString *output) {
            Q_UNUSED(input)
            if (command == "two")
                *output = "a\nb\n";
            else if (command == "one")
                *output = "only\n";
            else if (command == "nobreak")
                *output = "tail";
        });

    QCOMPARE(value("hostname() != ''"), QLatin1String("1"));
    QCOMPARE(value("type(hostname()) == v:t_string"), QLatin1String("1"));
    QCOMPARE(value("getpid() > 0"), QLatin1String("1"));
    QCOMPARE(value("type(getpid()) == v:t_number"), QLatin1String("1"));

    // systemlist() hands back the lines, and the break ending the last one is
    // not a line of its own.
    QCOMPARE(value("systemlist('two')"), QLatin1String("['a', 'b']"));
    QCOMPARE(value("systemlist('one')"), QLatin1String("['only']"));
    QCOMPARE(value("systemlist('nobreak')"), QLatin1String("['tail']"));
    // Nothing written means no lines at all, not one empty one.
    QCOMPARE(value("systemlist('says nothing')"), QLatin1String("[]"));
    // system() still hands back the text as it stands, break and all.
    QCOMPARE(value("system('two') ==# \"a\nb\n\""), QLatin1String("1"));

    // file_readable() is the older name for the same question, and
    // filewritable() answers ONE for a file, TWO for a directory and nothing
    // where neither holds. Measured in Vim 9.1.
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString path = dir.path() + "/perm.txt";
    QFile f(path);
    QVERIFY(f.open(QIODevice::WriteOnly));
    f.write("x\n");
    f.close();

    QCOMPARE(value("file_readable('" + path + "')"), QLatin1String("1"));
    QCOMPARE(value("file_readable('" + dir.path() + "/nope')"), QLatin1String("0"));
    QCOMPARE(value("filewritable('" + path + "')"), QLatin1String("1"));
    QCOMPARE(value("filewritable('" + dir.path() + "')"), QLatin1String("2"));
    QCOMPARE(value("filewritable('" + dir.path() + "/nope')"), QLatin1String("0"));

    // setfperm() reads back the nine characters getfperm() writes.
    QCOMPARE(value("setfperm('" + path + "', 'rwxr-xr-x')"), QLatin1String("1"));
    QCOMPARE(value("getfperm('" + path + "')"), QLatin1String("rwxr-xr-x"));
    QCOMPARE(value("setfperm('" + path + "', 'rw-r--r--')"), QLatin1String("1"));
    QCOMPARE(value("getfperm('" + path + "')"), QLatin1String("rw-r--r--"));
    // Anything but nine characters is refused.
    QCOMPARE(value("setfperm('" + path + "', 'rw-')"), QLatin1String("0"));
    QCOMPARE(value("getfperm('" + path + "')"), QLatin1String("rw-r--r--"));
    // A file that is not there cannot be given permissions.
    QCOMPARE(value("setfperm('" + dir.path() + "/nope', 'rw-r--r--')"),
             QLatin1String("0"));
}

void FakeVimTester::test_vim_script_environment_vars()
{
    // The v: variables a script reads to find out where it is running. Vim 9.1
    // answers each of these, and a script that reads one before anything has
    // happened must not be met with an undefined variable.
    TestData data;
    setup(&data);
    QString message;
    data.handler->commandBufferChanged.set(
        [&](const QString &msg, int, int, int) {
            if (!msg.isEmpty() && !msg.startsWith("--"))
                message = msg;
        });
    const auto value = [&](const QString &expr) {
        message.clear();
        data.doCommand("echo " + expr);
        return message;
    };

    QCOMPARE(value("v:progname"), QLatin1String("vim"));
    QCOMPARE(value("v:shell_error"), QLatin1String("0"));
    QCOMPARE(value("v:dying"), QLatin1String("0"));
    QCOMPARE(value("v:profiling"), QLatin1String("0"));
    QCOMPARE(value("v:testing"), QLatin1String("0"));
    // v:prevcount holds zero. Vim has it hold the count of the last but one
    // command, which is not reproduced here - see the note in the source.
    QCOMPARE(value("v:prevcount"), QLatin1String("0"));
    QCOMPARE(value("v:windowid"), QLatin1String("0"));
    QCOMPARE(value("v:vim_did_enter"), QLatin1String("1"));

    // Empty strings rather than nothing at all.
    QCOMPARE(value("v:warningmsg"), QLatin1String(""));
    QCOMPARE(value("v:this_session"), QLatin1String(""));
    QCOMPARE(value("v:servername"), QLatin1String(""));
    QCOMPARE(value("v:folddashes"), QLatin1String(""));

    // Lists, and empty ones.
    QCOMPARE(value("type(v:errors) == v:t_list"), QLatin1String("1"));
    QCOMPARE(value("len(v:errors)"), QLatin1String("0"));
    QCOMPARE(value("type(v:oldfiles) == v:t_list"), QLatin1String("1"));
    QCOMPARE(value("type(v:argv) == v:t_list"), QLatin1String("1"));

    // Nothing is exiting, which Vim answers with v:null and not with zero -
    // the two are told apart by their type.
    QCOMPARE(value("v:exiting"), QLatin1String("v:null"));
    QCOMPARE(value("type(v:exiting) == v:t_none"), QLatin1String("1"));
    QCOMPARE(value("v:exiting is v:null"), QLatin1String("1"));

    // The version, written as Vim writes it, and in step with v:version.
    QCOMPARE(value("v:versionlong"), QLatin1String("9010000"));
    QCOMPARE(value("v:versionlong / 1000000"), QLatin1String("9"));

    // The locale, which is a name with an encoding behind it.
    QCOMPARE(value("v:lang =~# '\\.UTF-8$'"), QLatin1String("1"));
    QCOMPARE(value("v:ctype ==# v:lang"), QLatin1String("1"));
    QCOMPARE(value("v:collate ==# v:lang"), QLatin1String("1"));

    // v:statusmsg holds what the engine itself last had to say about how many
    // lines a command touched - measured in Vim 9.1 with 'report' at zero, so
    // every count is reported.
    data.setText("a" N "b" N "c" N "d" N "e" N "f");
    // Options are shared with every other test slot, so this one is put back
    // afterwards: leaving 'report' at zero makes line counts appear in tests
    // that ran fine without them.
    const QString wasReport = value("&report");
    data.doCommand("set report=0");
    data.doKeys("gg03yy");
    QCOMPARE(value("v:statusmsg"), QLatin1String("3 lines yanked"));
    data.doKeys("gg02dd");
    QCOMPARE(value("v:statusmsg"), QLatin1String("2 fewer lines"));
    // An ":echo" does not go in there, so the last one stands.
    data.doCommand("echo 'plain'");
    QCOMPARE(value("v:statusmsg"), QLatin1String("2 fewer lines"));
    // A script may write it.
    data.doCommand("let v:statusmsg = 'mine'");
    QCOMPARE(value("v:statusmsg"), QLatin1String("mine"));
    data.doCommand("set report=" + wasReport);

    // The fold ones hold what Vim holds outside a 'foldtext'.
    QCOMPARE(value("v:foldlevel"), QLatin1String("0"));
    QCOMPARE(value("v:foldstart"), QLatin1String("0"));
    QCOMPARE(value("v:foldend"), QLatin1String("0"));
}

void FakeVimTester::test_vim_script_searchforward()
{
    // v:searchforward says which way the last search went and may be written to
    // turn "n" around; v:register is the register the command in hand was
    // given. Both measured in Vim 9.1.
    TestData data;
    setup(&data);
    QString message;
    data.handler->commandBufferChanged.set(
        [&](const QString &msg, int, int, int) {
            if (!msg.isEmpty() && !msg.startsWith("--"))
                message = msg;
        });
    const auto value = [&](const QString &expr) {
        message.clear();
        data.doCommand("echo " + expr);
        return message;
    };

    data.setText("alpha" N "beta" N "alpha" N "beta");

    // Searching forward and back says so.
    data.doKeys("gg0/beta<CR>");
    QCOMPARE(value("v:searchforward"), QLatin1String("1"));
    data.doKeys("G$?alpha<CR>");
    QCOMPARE(value("v:searchforward"), QLatin1String("0"));
    data.doKeys("gg0/beta<CR>");
    QCOMPARE(value("v:searchforward"), QLatin1String("1"));

    // Writing it turns "n" around without searching again: from line 1 a
    // forward "n" reaches the next "beta" below, and a backward one the one
    // above.
    data.setText("beta" N "x" N "beta" N "x" N "beta");
    data.doKeys("gg0/beta<CR>");     // on line 3 now
    QCOMPARE(value("line('.')"), QLatin1String("3"));
    data.doKeys("n");                // forward again, line 5
    QCOMPARE(value("line('.')"), QLatin1String("5"));
    data.doCommand("let v:searchforward = 0");
    QCOMPARE(value("v:searchforward"), QLatin1String("0"));
    data.doKeys("n");                // now backwards, line 3
    QCOMPARE(value("line('.')"), QLatin1String("3"));

    // The register the command in hand was given: the unnamed one by default,
    // which is written as a quote.
    QCOMPARE(value("v:register"), QLatin1String("\""));

    // v:operator is the last operator that ran, written the way it is typed,
    // and it keeps the last one. Values measured in Vim 9.1.
    data.setText("alpha beta" N "second line" N "third line");
    data.doKeys("gg0dd");
    QCOMPARE(value("v:operator"), QLatin1String("d"));
    data.doKeys("gg0yy");
    QCOMPARE(value("v:operator"), QLatin1String("y"));
    data.doKeys("gg0cwZ<Esc>");
    QCOMPARE(value("v:operator"), QLatin1String("c"));
    data.doKeys("gg0guu");
    QCOMPARE(value("v:operator"), QLatin1String("gu"));
    data.doKeys("gg0gUU");
    QCOMPARE(value("v:operator"), QLatin1String("gU"));
    data.doKeys("gg0>>");
    QCOMPARE(value("v:operator"), QLatin1String(">"));
    // That it KEEPS the last one is not asserted: nothing reachable from here
    // clears it either way, so an assertion would hold whatever the code did.
    // Writing the value unconditionally instead of only for a known operator
    // passes every check above.
}

void FakeVimTester::test_vim_script_type_constants()
{
    // The v:t_* names a script compares type() against. All values measured in
    // Vim 9.1, where the numbers are NOT in the order the names are: a Blob is
    // 10, after a Job and a Channel, and there is no 11.
    TestData data;
    setup(&data);
    QString message;
    data.handler->commandBufferChanged.set(
        [&](const QString &msg, int, int, int) {
            if (!msg.isEmpty() && !msg.startsWith("--"))
                message = msg;
        });
    const auto value = [&](const QString &expr) {
        message.clear();
        data.doCommand("echo " + expr);
        return message;
    };

    QCOMPARE(value("v:t_number"), QLatin1String("0"));
    QCOMPARE(value("v:t_string"), QLatin1String("1"));
    QCOMPARE(value("v:t_func"), QLatin1String("2"));
    QCOMPARE(value("v:t_list"), QLatin1String("3"));
    QCOMPARE(value("v:t_dict"), QLatin1String("4"));
    QCOMPARE(value("v:t_float"), QLatin1String("5"));
    QCOMPARE(value("v:t_bool"), QLatin1String("6"));
    QCOMPARE(value("v:t_none"), QLatin1String("7"));
    QCOMPARE(value("v:t_job"), QLatin1String("8"));
    QCOMPARE(value("v:t_channel"), QLatin1String("9"));
    QCOMPARE(value("v:t_blob"), QLatin1String("10"));
    QCOMPARE(value("v:t_class"), QLatin1String("12"));
    QCOMPARE(value("v:t_object"), QLatin1String("13"));
    QCOMPARE(value("v:t_typealias"), QLatin1String("14"));
    QCOMPARE(value("v:t_enum"), QLatin1String("15"));
    QCOMPARE(value("v:t_enumvalue"), QLatin1String("16"));
    QCOMPARE(value("v:t_tuple"), QLatin1String("17"));

    // What they are for: they line up with what type() answers.
    QCOMPARE(value("type(0) == v:t_number"), QLatin1String("1"));
    QCOMPARE(value("type('x') == v:t_string"), QLatin1String("1"));
    QCOMPARE(value("type([]) == v:t_list"), QLatin1String("1"));
    QCOMPARE(value("type({}) == v:t_dict"), QLatin1String("1"));
    QCOMPARE(value("type(1.5) == v:t_float"), QLatin1String("1"));
    QCOMPARE(value("type(v:true) == v:t_bool"), QLatin1String("1"));
    QCOMPARE(value("type(v:null) == v:t_none"), QLatin1String("1"));
    QCOMPARE(value("type(function('strlen')) == v:t_func"), QLatin1String("1"));
    // A type this engine has no values of compares false rather than failing.
    QCOMPARE(value("type('x') == v:t_blob"), QLatin1String("0"));

    // How far a number reaches, and how wide what holds it is. Measured in Vim
    // 9.1 on a 64-bit build. "v:maxcol" is a 32-bit maximum, not a 64-bit one.
    QCOMPARE(value("v:numbermax"), QLatin1String("9223372036854775807"));
    QCOMPARE(value("v:numbermin"), QLatin1String("-9223372036854775808"));
    QCOMPARE(value("v:numbersize"), QLatin1String("64"));
    QCOMPARE(value("v:maxcol"), QLatin1String("2147483647"));
    QCOMPARE(value("v:sizeofint"), QLatin1String("4"));
    QCOMPARE(value("v:sizeoflong"), QLatin1String("8"));
    QCOMPARE(value("v:sizeofpointer"), QLatin1String("8"));
    // They line up with what arithmetic here actually does.
    QCOMPARE(value("v:numbermax + 0 == v:numbermax"), QLatin1String("1"));
    QCOMPARE(value("v:numbermax > v:maxcol"), QLatin1String("1"));
}

void FakeVimTester::test_vim_autocmd_filewrite()
{
    // Writing PART of the buffer is a file write, not a buffer write, and the
    // two are announced by names of their own. Everything was announced as a
    // buffer write before. All values measured in Vim 9.1, where a RANGE is
    // what decides - writing the whole buffer to another file is still a buffer
    // write.
    TestData data;
    setup(&data);
    QString message;
    data.handler->commandBufferChanged.set(
        [&](const QString &msg, int, int, int) {
            if (!msg.isEmpty() && !msg.startsWith("--"))
                message = msg;
        });
    const auto value = [&](const QString &expr) {
        message.clear();
        data.doCommand("echo " + expr);
        return message;
    };
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const auto go = [&](const QString &command) {
        data.setText("one" N "two" N "three");
        data.doCommand("let g:w = []");
        data.doCommand(command);
        return value("string(g:w)");
    };

    data.doCommand("let g:w = []");
    data.doCommand("autocmd FvW BufWritePre * call add(g:w, 'bufpre')");
    data.doCommand("autocmd FvW BufWritePost * call add(g:w, 'bufpost')");
    data.doCommand("autocmd FvW FileWritePre * call add(g:w, 'filepre')");
    data.doCommand("autocmd FvW FileWritePost * call add(g:w, 'filepost')");

    // The whole buffer, to another file: still a buffer write.
    QCOMPARE(go("w! " + dir.path() + "/whole.txt"),
             QLatin1String("['bufpre', 'bufpost']"));

    // Part of it: a file write, whichever file it goes to.
    QCOMPARE(go("1,2w! " + dir.path() + "/part.txt"),
             QLatin1String("['filepre', 'filepost']"));
    QCOMPARE(go("1,2w! " + dir.path() + "/whole.txt"),
             QLatin1String("['filepre', 'filepost']"));

    // A file write names the file it goes to.
    data.doCommand("autocmd! FvW");
    data.doCommand("autocmd FvW FileWritePre * call add(g:w, expand('<afile>'))");
    go("1,2w! " + dir.path() + "/named.txt");
    QCOMPARE(value("g:w[0] =~# 'named.txt$'"), QLatin1String("1"));

    data.doCommand("autocmd! FvW");
    data.doCommand("unlet! g:w");
}

void FakeVimTester::test_vim_command_write_whole_buffer()
{
    // ":w[!] {file}" with no explicit line range writes the WHOLE buffer,
    // not just the current line - a pre-existing bug found while writing
    // the append form (196-197): cmd.range always names a real line (the
    // current one) even where none was typed, so the old
    // "beginLine == -1 means no range, use the whole buffer" check never
    // triggered, and only the cursor's own line was ever written whenever
    // no range was given. Only the events this function fires were tested
    // before, never the file's actual content.
    TestData data;
    setup(&data);
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString target = dir.path() + "/whole.txt";

    data.setText("one" N "two" N "three");
    data.doCommand("w! " + target);
    QFile f(target);
    QVERIFY(f.open(QIODevice::ReadOnly));
    QCOMPARE(f.readAll(), QByteArray("one\ntwo\nthree\n"));
    f.close();

    // An explicit range still writes only that part.
    QVERIFY(QFile::remove(target));
    data.doCommand("1,2w! " + target);
    QVERIFY(f.open(QIODevice::ReadOnly));
    QCOMPARE(f.readAll(), QByteArray("one\ntwo\n"));
    f.close();
}

void FakeVimTester::test_vim_command_write_append()
{
    // ":w[!] >> [file]" appends instead of overwriting - to the current
    // file if none is named. "!" allows creating a file that does not exist
    // yet; without it, appending to a missing one is refused. Fires its own
    // FileAppendPre/FileAppendPost pair, neither of which existed in the
    // event table at all before. Values taken from Vim 9.1.
    TestData data;
    setup(&data);
    QString message;
    data.handler->commandBufferChanged.set(
        [&](const QString &msg, int, int, int) {
            if (!msg.isEmpty() && !msg.startsWith("--"))
                message = msg;
        });
    const auto value = [&](const QString &expr) {
        message.clear();
        data.doCommand("echo " + expr);
        return message;
    };
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString target = dir.path() + "/append.txt";

    // Appending to an existing file adds after what was there.
    QFile f(target);
    QVERIFY(f.open(QIODevice::WriteOnly));
    f.write("EXISTING\n");
    f.close();
    data.setText("one" N "two");
    data.doCommand("w >> " + target);
    QCOMPARE(value("readfile('" + target + "')"),
             QLatin1String("['EXISTING', 'one', 'two']"));

    // Appending to a MISSING file, without "!", is refused.
    QVERIFY(QFile::remove(target));
    message.clear();
    data.doCommand("w >> " + target);
    QVERIFY(message.contains("E212"));
    QVERIFY(!QFile::exists(target));

    // "!" creates it.
    data.doCommand("w! >> " + target);
    QCOMPARE(value("readfile('" + target + "')"), QLatin1String("['one', 'two']"));

    // With no name at all, appends to the CURRENT file (its own content,
    // unsaved changes included).
    QVERIFY(QFile::remove(target));
    QFile f2(target);
    QVERIFY(f2.open(QIODevice::WriteOnly));
    f2.write("orig1\norig2\n");
    f2.close();
    data.handler->setCurrentFileName(target);
    data.setText("orig1" N "orig2" N "extra");
    data.doCommand("w >>");
    QCOMPARE(value("readfile('" + target + "')"),
             QLatin1String("['orig1', 'orig2', 'orig1', 'orig2', 'extra']"));

    // Fires its own pair, naming the file it appends to; the pair fires
    // regardless of range, unlike the ordinary Buf/FileWrite pair.
    QVERIFY(QFile::remove(target));
    data.setText("one" N "two" N "three");
    data.doCommand("let g:w = []");
    data.doCommand("autocmd FvWA FileAppendPre * call add(g:w, 'pre:' . expand('<afile>'))");
    data.doCommand("autocmd FvWA FileAppendPost * call add(g:w, 'post')");
    data.doCommand("w! >> " + target);
    QCOMPARE(value("g:w[0] =~# 'append.txt$'"), QLatin1String("1"));
    QCOMPARE(value("g:w[1]"), QLatin1String("post"));
    // The file now holds what the previous append put there ("one",
    // "two", "three"); a RANGE appends only that part on top, not the
    // whole buffer again.
    data.doCommand("let g:w = []");
    data.doCommand("1,2w >> " + target);
    QCOMPARE(value("string(g:w)[0:5]"), QLatin1String("['pre:"));
    QCOMPARE(value("g:w[1]"), QLatin1String("post"));
    QCOMPARE(value("readfile('" + target + "')"),
             QLatin1String("['one', 'two', 'three', 'one', 'two']"));

    // A failed append fires FileAppendPre but NOT FileAppendPost.
    QVERIFY(QFile::remove(target));
    data.doCommand("let g:w = []");
    data.doCommand("w >> " + target);
    QCOMPARE(value("string(g:w)"), QString("['pre:" + target + "']"));

    // Known to autocmd_add() now.
    QCOMPARE(value("autocmd_add([{'group': 'FvWA', 'event': 'FileAppendPre',"
                   " 'pattern': '*', 'cmd': 'echo 1'}])"), QLatin1String("v:true"));
    QCOMPARE(value("autocmd_add([{'group': 'FvWA', 'event': 'FileAppendPost',"
                   " 'pattern': '*', 'cmd': 'echo 1'}])"), QLatin1String("v:true"));

    data.doCommand("autocmd! FvWA");
    data.doCommand("unlet! g:w");
}

void FakeVimTester::test_vim_command_nargs()
{
    // ":command -nargs=" was skipped along with the other attributes, so a
    // wrong number of arguments drew no complaint. All values measured in Vim
    // 9.1.
    TestData data;
    setup(&data);
    QString message;
    data.handler->commandBufferChanged.set(
        [&](const QString &msg, int, int, int) {
            if (!msg.isEmpty() && !msg.startsWith("--"))
                message = msg;
        });
    const auto run = [&](const QString &command) {
        message.clear();
        data.doCommand(command);
        return message;
    };

    data.setText("x");
    data.doCommand("let g:n = []");
    data.doCommand("command! -nargs=0 FvZero call add(g:n, 'zero')");
    data.doCommand("command! -nargs=1 FvOne call add(g:n, 'one <args>')");
    data.doCommand("command! -nargs=? FvOpt call add(g:n, 'opt')");
    data.doCommand("command! -nargs=+ FvPlus call add(g:n, 'plus')");
    data.doCommand("command! -nargs=* FvStar call add(g:n, 'star')");
    data.doCommand("command! FvBare call add(g:n, 'bare')");

    // What is allowed goes through.
    QVERIFY(run("FvZero").isEmpty());
    QVERIFY(run("FvOne a").isEmpty());
    QVERIFY(run("FvOpt").isEmpty());
    QVERIFY(run("FvOpt a b").isEmpty());
    QVERIFY(run("FvPlus a").isEmpty());
    QVERIFY(run("FvStar").isEmpty());
    // "1" takes all that follows as the one argument, so several words are no
    // complaint.
    QVERIFY(run("FvOne a b").isEmpty());

    // One that wants an argument and got none.
    QVERIFY(run("FvOne").contains("E471"));
    QVERIFY(run("FvOne").contains("FvOne"));
    QVERIFY(run("FvPlus").contains("E471"));

    // One that allows none and got some, which names the trailing text and
    // then the whole line.
    const QString trailing = run("FvZero x");
    QVERIFY(trailing.contains("E488"));
    QVERIFY(trailing.contains("x: FvZero x"));
    // Saying nothing about it is the same as saying none.
    QVERIFY(run("FvBare x").contains("E488"));

    // Nothing ran where it was refused.
    data.doCommand("let g:n = []");
    data.doCommand("FvOne");
    data.doCommand("FvZero x");
    QCOMPARE(run("echo string(g:n)"), QLatin1String("[]"));

    data.doCommand("delcommand FvZero");
    data.doCommand("delcommand FvOne");
    data.doCommand("delcommand FvOpt");
    data.doCommand("delcommand FvPlus");
    data.doCommand("delcommand FvStar");
    data.doCommand("delcommand FvBare");
    data.doCommand("unlet! g:n");
}

void FakeVimTester::test_vim_ex_join_count()
{
    // ":join" and its count, in every spelling. All values measured in Vim 9.1
    // on the five lines a b c d e with the cursor on the first.
    TestData data;
    setup(&data);
    const auto five = [&] {
        data.setText("a" N "b" N "c" N "d" N "e");
        data.doKeys("gg0");
    };

    five(); data.doCommand("join");
    QCOMPARE(data.text(), QString("a b" N "c" N "d" N "e"));
    five(); data.doCommand("join 3");
    QCOMPARE(data.text(), QString("a b c" N "d" N "e"));
    // The count may follow the name with nothing between.
    five(); data.doCommand("j3");
    QCOMPARE(data.text(), QString("a b c" N "d" N "e"));
    // An address says where to start, the count how many from there.
    five(); data.doCommand("2join 3");
    QCOMPARE(data.text(), QString("a" N "b c d" N "e"));
    // A range says it instead.
    five(); data.doCommand("1,3join");
    QCOMPARE(data.text(), QString("a b c" N "d" N "e"));
    // The bang puts no blank between what it joins.
    five(); data.doCommand("join!");
    QCOMPARE(data.text(), QString("ab" N "c" N "d" N "e"));
    five(); data.doCommand("j!3");
    QCOMPARE(data.text(), QString("abc" N "d" N "e"));
}

void FakeVimTester::test_vim_ex_history()
{
    // ":history [{name}] [{first}[,{last}]]". Any argument at all used to be
    // answered with "not implemented". All values measured in Vim 9.1, where a
    // number is written in seven and the entry that would come up next carries
    // a ">".
    TestData data;
    setup(&data);
    QString info;
    data.handler->extraInformationChanged.set([&](const QString &text) { info = text; });
    const auto show = [&](const QString &args) {
        info.clear();
        data.doCommand("history" + (args.isEmpty() ? QString() : " " + args));
        return info;
    };

    data.setText("alpha beta");
    // Two command lines and a search, so both histories have something in them.
    data.doKeys(":echo 1<CR>");
    data.doKeys(":echo 2<CR>");
    data.doKeys("gg/alpha<CR>");

    // The command history by default, laid out as Vim lays it out. The history
    // is shared with every other test slot, so what NUMBER an entry has here
    // depends on what ran before: only the layout and the text are asserted.
    const QString cmds = show("");
    QVERIFY(cmds.startsWith("      #  cmd history\n"));
    QVERIFY(cmds.contains("  echo 1\n"));
    QVERIFY(cmds.contains("  echo 2\n"));
    // The last one is the one that would come up next.
    QVERIFY(cmds.contains(">"));

    // The search history is its own, and named so.
    const QString searches = show("search");
    QVERIFY(searches.startsWith("      #  search history\n"));
    QVERIFY(searches.contains("alpha"));
    QVERIFY(!searches.contains("echo 1"));

    // Each name may be shortened, and some are written as the character that
    // opens the line.
    QVERIFY(show("s").startsWith("      #  search history"));
    QVERIFY(show("/").startsWith("      #  search history"));
    QVERIFY(show("c").startsWith("      #  cmd history"));
    QVERIFY(show(":").startsWith("      #  cmd history"));

    // "all" names every one of them, in Vim's order, and one with nothing
    // under it is still named.
    const QString all = show("all");
    QVERIFY(all.contains("cmd history"));
    QVERIFY(all.contains("search history"));
    QVERIFY(all.contains("expr history"));
    QVERIFY(all.contains("input history"));
    QVERIFY(all.contains("debug history"));
    QVERIFY(all.indexOf("cmd history") < all.indexOf("search history"));
    QVERIFY(all.indexOf("search history") < all.indexOf("expr history"));

    // A range picks which of them to show, so fewer come back than without one.
    const QString one = show("cmd 1,1");
    QCOMPARE(one.count('\n'), 2); // the header and the one entry
    QVERIFY(one.count('\n') < cmds.count('\n'));

    // A name that is no name is reported rather than passed over.
    info.clear();
    QString message;
    data.handler->commandBufferChanged.set(
        [&](const QString &msg, int, int, int) {
            if (!msg.isEmpty() && !msg.startsWith("--"))
                message = msg;
        });
    data.doCommand("history nosuchhistory");
    QVERIFY(message.contains("E488"));
}

void FakeVimTester::test_vim_read_from_command()
{
    // ":r !{cmd}" put nothing anywhere: ":read" only ever opened a file, so the
    // "!" was taken for the start of a file name. All values measured in Vim
    // 9.1, on the three lines "one", "two", "three" with the cursor on line 2.
    TestData data;
    setup(&data);
    QString message;
    data.handler->commandBufferChanged.set(
        [&](const QString &msg, int, int, int) {
            if (!msg.isEmpty() && !msg.startsWith("--"))
                message = msg;
        });
    const auto value = [&](const QString &expr) {
        message.clear();
        data.doCommand("echo " + expr);
        return message;
    };
    const auto three = [&] {
        data.setText("one" N "two" N "three");
        data.doKeys("gg0j"); // on line two, as the measurement had it
    };
    // Nothing here reaches a shell - the plugin is what wires that up - so the
    // command is answered from a table instead, which also keeps the test off
    // the machine it runs on.
    data.handler->processOutput.set(
        [](const QString &command, const QString &input, QString *output) {
            Q_UNUSED(input)
            if (command == "echo X")
                *output = "X\n";
            else if (command == "two lines")
                *output = "a\nb\n";
            else if (command == "no newline")
                *output = "Z";
        });

    // What the command writes goes in after the line, and the cursor is left
    // on the last of what came in.
    three();
    data.doCommand("r !echo X");
    QCOMPARE(data.text(), QString("one" N "two" N "X" N "three"));
    QCOMPARE(value("line('.')"), QLatin1String("3"));

    // Several lines all go in.
    three();
    data.doCommand("r !two lines");
    QCOMPARE(data.text(), QString("one" N "two" N "a" N "b" N "three"));
    QCOMPARE(value("line('.')"), QLatin1String("4"));

    // An address says which line to put it after.
    three();
    data.doCommand("1r !echo X");
    QCOMPARE(data.text(), QString("one" N "X" N "two" N "three"));

    // Output with no line break of its own still goes in as a whole line.
    three();
    data.doCommand("r !no newline");
    QCOMPARE(data.text(), QString("one" N "two" N "Z" N "three"));

    // A command that writes nothing puts nothing anywhere.
    three();
    data.doCommand("r !says nothing");
    QCOMPARE(data.text(), QString("one" N "two" N "three"));

    // Vim counts reading from a command among the filters.
    data.doCommand("let g:f = []");
    data.doCommand("autocmd FvRd ShellFilterPost * call add(g:f, 'filter')");
    three();
    data.doCommand("r !echo X");
    QCOMPARE(value("string(g:f)"), QLatin1String("['filter']"));

    // Reading a FILE still works, and is no filter. It puts its lines in the
    // same place, honours an address the same way, and leaves the cursor on the
    // last line read - none of which it did before, since it went by the cursor
    // alone. Measured in Vim 9.1 alongside the command form.
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString plain = dir.path() + "/plain.txt";
    QFile f(plain);
    QVERIFY(f.open(QIODevice::WriteOnly));
    f.write("FROMFILE\n");
    f.close();
    data.doCommand("let g:f = []");
    three();
    data.doCommand("r " + plain);
    QCOMPARE(data.text(), QString("one" N "two" N "FROMFILE" N "three"));
    QCOMPARE(value("line('.')"), QLatin1String("3"));
    QCOMPARE(value("string(g:f)"), QLatin1String("[]"));

    three();
    data.doCommand("1r " + plain);
    QCOMPARE(data.text(), QString("one" N "FROMFILE" N "two" N "three"));
    three();
    data.doCommand("3r " + plain);
    QCOMPARE(data.text(), QString("one" N "two" N "three" N "FROMFILE"));

    // Reading a file does not rename the buffer, so "%" must not come back
    // naming the file that was read.
    three();
    data.doCommand("r " + plain);
    QCOMPARE(value("expand('%') =~# 'plain.txt$'"), QLatin1String("0"));

    // Reading a FILE announces FileReadPre and FileReadPost, naming the file
    // read; reading from a COMMAND announces neither, being a filter.
    data.doCommand("autocmd! FvRd");
    data.doCommand("let g:r2 = []");
    data.doCommand("autocmd FvRd FileReadPre * call add(g:r2, 'pre:' . expand('<afile>'))");
    data.doCommand("autocmd FvRd FileReadPost * call add(g:r2, 'post')");
    three();
    data.doCommand("r " + plain);
    QCOMPARE(value("string(g:r2)[0:5]"), QLatin1String("['pre:"));
    QCOMPARE(value("len(g:r2)"), QLatin1String("2"));
    QCOMPARE(value("g:r2[0] =~# 'plain.txt$'"), QLatin1String("1"));
    QCOMPARE(value("g:r2[1]"), QLatin1String("post"));

    data.doCommand("let g:r2 = []");
    three();
    data.doCommand("r !echo X");
    QCOMPARE(value("string(g:r2)"), QLatin1String("[]"));

    // A file that is not there is read by nobody, so neither is announced.
    data.doCommand("let g:r2 = []");
    three();
    data.doCommand("r " + dir.path() + "/no_such_file.txt");
    QCOMPARE(value("string(g:r2)"), QLatin1String("[]"));

    // A file that cannot be opened used to answer "E492: Not an editor
    // command: read" instead of a file error - handleExReadCommand() returned
    // false on QFile::open() failure, which tells the dispatcher "not my
    // command" rather than reporting the problem itself.
    const QString missing = dir.path() + "/no_such_file.txt";
    three();
    message.clear();
    data.doCommand("r " + missing);
    QCOMPARE(message, QString("E484: Can't open file " + missing));
    QCOMPARE(data.text(), QString("one" N "two" N "three"));

    // A directory cannot be opened either.
    three();
    message.clear();
    data.doCommand("r " + dir.path());
    QCOMPARE(message, QString("E484: Can't open file " + dir.path()));

    // No name at all is a different error - Vim does not try to read the
    // current file.
    three();
    message.clear();
    data.doCommand("r");
    QCOMPARE(message, QLatin1String("E32: No file name"));
    QCOMPARE(data.text(), QString("one" N "two" N "three"));

    data.doCommand("autocmd! FvRd");
    data.doCommand("unlet! g:f");
    data.doCommand("unlet! g:r2");
}

void FakeVimTester::test_vim_autocmd_modechanged()
{
    // ModeChanged, an event this engine did not know as one. Its pattern is the
    // two modes with a colon between them, which "<amatch>" also stands for,
    // and v:event carries them apart. All values measured in Vim 9.1.
    TestData data;
    setup(&data);
    QString message;
    data.handler->commandBufferChanged.set(
        [&](const QString &msg, int, int, int) {
            if (!msg.isEmpty() && !msg.startsWith("--"))
                message = msg;
        });
    const auto value = [&](const QString &expr) {
        message.clear();
        data.doCommand("echo " + expr);
        return message;
    };
    const auto go = [&](const QString &keys) {
        data.setText("alpha beta");
        data.doCommand("let g:d = []");
        data.doKeys(keys);
        return value("string(g:d)");
    };

    data.doCommand("let g:d = []");
    data.doCommand("autocmd FvMc ModeChanged * call add(g:d, expand('<amatch>'))");

    // Into insert mode and out again.
    QCOMPARE(go("gg0ix<Esc>"), QLatin1String("['n:i', 'i:n']"));
    // Visual, and visual line.
    QCOMPARE(go("gg0v<Esc>"), QLatin1String("['n:v', 'v:n']"));
    QCOMPARE(go("gg0V<Esc>"), QLatin1String("['n:V', 'V:n']"));
    // Replace mode is named apart from insert.
    QCOMPARE(go("gg0R<Esc>"), QLatin1String("['n:R', 'R:n']"));

    // Keys that leave the mode where it was announce nothing.
    QCOMPARE(go("gg0ll"), QLatin1String("[]"));

    // v:event carries the two modes on their own.
    data.doCommand("autocmd! FvMc");
    data.doCommand("autocmd FvMc ModeChanged * call add(g:d,"
                   " v:event.old_mode . '>' . v:event.new_mode)");
    QCOMPARE(go("gg0ix<Esc>"), QLatin1String("['n>i', 'i>n']"));

    // The pattern picks which change, which is the point of writing it so.
    data.doCommand("autocmd! FvMc");
    data.doCommand("autocmd FvMc ModeChanged n:i call add(g:d, 'entering-insert')");
    QCOMPARE(go("gg0ix<Esc>"), QLatin1String("['entering-insert']"));
    QCOMPARE(go("gg0v<Esc>"), QLatin1String("[]"));

    // It is an event this engine knows now, which autocmd_add() is the test of.
    QCOMPARE(value("autocmd_add([{'group': 'FvMc', 'event': 'ModeChanged',"
                   " 'pattern': '*', 'cmd': 'echo 1'}])"), QLatin1String("v:true"));

    data.doCommand("autocmd! FvMc");
    data.doCommand("unlet! g:d");
}

void FakeVimTester::test_vim_autocmd_shell()
{
    // ShellCmdPost and ShellFilterPost. Which of the two it is turns on whether
    // a range was given: a range makes it a filter, no range a command of its
    // own. Measured in Vim 9.1, where ":!true" gives the first and ":1,3!sort"
    // the second.
    TestData data;
    setup(&data);
    QString message;
    data.handler->commandBufferChanged.set(
        [&](const QString &msg, int, int, int) {
            if (!msg.isEmpty() && !msg.startsWith("--"))
                message = msg;
        });
    const auto value = [&](const QString &expr) {
        message.clear();
        data.doCommand("echo " + expr);
        return message;
    };
    const auto go = [&](const QString &command) {
        data.setText("bbb" N "aaa" N "ccc");
        data.doCommand("let g:h = []");
        data.doCommand(command);
        return value("string(g:h)");
    };

    data.doCommand("let g:h = []");
    data.doCommand("autocmd FvSh ShellCmdPost * call add(g:h, 'cmd')");
    data.doCommand("autocmd FvSh ShellFilterPost * call add(g:h, 'filter')");

    // No range: a command of its own.
    QCOMPARE(go("!true"), QLatin1String("['cmd']"));
    // A range: a filter.
    QCOMPARE(go("1,3!sort"), QLatin1String("['filter']"));
    QCOMPARE(go("%!sort"), QLatin1String("['filter']"));

    // Something that reaches no shell at all announces neither.
    QCOMPARE(go("echo 1"), QLatin1String("[]"));
    QCOMPARE(go("set ignorecase"), QLatin1String("[]"));
    data.doCommand("set noignorecase");

    data.doCommand("autocmd! FvSh");
    data.doCommand("unlet! g:h");
}

void FakeVimTester::test_vim_autocmd_filter()
{
    // FilterWritePre/Post and FilterReadPre/Post - none of the four was in
    // the event table before. Real Vim writes the range to a temp file, runs
    // the command, and reads its output back from another; this engine
    // filters in process, so there is no real file on either side. Values
    // taken from Vim 9.1: measured order is WritePre, WritePost, ReadPre,
    // ReadPost, around the existing ShellFilterPost firing point. Only a
    // RANGE reaches these - no range is a command of its own (ShellCmdPost),
    // which fires none of the four.
    TestData data;
    setup(&data);
    QString message;
    data.handler->commandBufferChanged.set(
        [&](const QString &msg, int, int, int) {
            if (!msg.isEmpty() && !msg.startsWith("--"))
                message = msg;
        });
    const auto value = [&](const QString &expr) {
        message.clear();
        data.doCommand("echo " + expr);
        return message;
    };

    data.setText("bbb" N "aaa" N "ccc");
    data.doCommand("let g:h = []");
    data.doCommand("autocmd FvFi FilterWritePre * call add(g:h, 'wpre')");
    data.doCommand("autocmd FvFi FilterWritePost * call add(g:h, 'wpost')");
    data.doCommand("autocmd FvFi FilterReadPre * call add(g:h, 'rpre')");
    data.doCommand("autocmd FvFi FilterReadPost * call add(g:h, 'rpost')");
    data.doCommand("autocmd FvFi ShellFilterPost * call add(g:h, 'shellfilterpost')");

    data.doCommand("1,3!sort");
    QCOMPARE(value("string(g:h)"),
             QLatin1String("['wpre', 'wpost', 'rpre', 'rpost', 'shellfilterpost']"));

    // No range: none of the four (it is ShellCmdPost's territory, not this
    // family's).
    data.doCommand("let g:h = []");
    data.doCommand("!true");
    QCOMPARE(value("string(g:h)"), QLatin1String("[]"));

    data.doCommand("autocmd! FvFi");
    data.doCommand("unlet! g:h");
}

void FakeVimTester::test_vim_autocmd_insertleavepre()
{
    // InsertLeavePre, an event this engine did not know as one. It comes
    // immediately before InsertLeave, and both are told which mode is being
    // left. Measured in Vim 9.1.
    TestData data;
    setup(&data);
    QString message;
    data.handler->commandBufferChanged.set(
        [&](const QString &msg, int, int, int) {
            if (!msg.isEmpty() && !msg.startsWith("--"))
                message = msg;
        });
    const auto value = [&](const QString &expr) {
        message.clear();
        data.doCommand("echo " + expr);
        return message;
    };

    data.setText("alpha beta");
    data.doCommand("let g:i = []");
    data.doCommand("autocmd FvIlp InsertLeavePre * call add(g:i, 'pre:' . v:insertmode)");
    data.doCommand("autocmd FvIlp InsertLeave * call add(g:i, 'leave:' . v:insertmode)");

    // The "Pre" comes first, both naming the mode being left.
    data.doKeys("gg0ix<Esc>");
    QCOMPARE(value("string(g:i)"), QLatin1String("['pre:i', 'leave:i']"));

    // Leaving replace mode says so through both.
    data.doCommand("let g:i = []");
    data.doKeys("gg0R<Esc>");
    QCOMPARE(value("string(g:i)"), QLatin1String("['pre:r', 'leave:r']"));

    // It is an event this engine knows now, which autocmd_add() is the test of.
    QCOMPARE(value("autocmd_add([{'group': 'FvIlp', 'event': 'InsertLeavePre',"
                   " 'pattern': '*', 'cmd': 'echo 1'}])"), QLatin1String("v:true"));

    data.doCommand("autocmd! FvIlp");
    data.doCommand("unlet! g:i");
}

void FakeVimTester::test_vim_autocmd_cmdlineleavepre()
{
    // CmdlineLeavePre, another event this engine did not know. It comes
    // immediately before CmdlineLeave, wherever a command line is left.
    // Measured in Vim 9.1.
    TestData data;
    setup(&data);
    QString message;
    data.handler->commandBufferChanged.set(
        [&](const QString &msg, int, int, int) {
            if (!msg.isEmpty() && !msg.startsWith("--"))
                message = msg;
        });
    const auto value = [&](const QString &expr) {
        message.clear();
        data.doCommand("echo " + expr);
        return message;
    };

    data.setText("alpha beta");
    data.doCommand("let g:p = []");
    data.doCommand("autocmd FvLp CmdlineLeavePre * call add(g:p,"
                   " 'pre' . expand('<afile>') . char2nr(v:char))");
    data.doCommand("autocmd FvLp CmdlineLeave * call add(g:p,"
                   " 'leave' . expand('<afile>') . char2nr(v:char))");

    // The "Pre" comes first, and both are handed the same line and key.
    data.doCommand("let g:p = []");
    data.doKeys(":echo 1<CR>");
    QCOMPARE(value("string(g:p)"), QLatin1String("['pre:13', 'leave:13']"));

    // Giving up on the line says so through both, with the key that left it.
    data.doCommand("let g:p = []");
    data.doKeys(":echo 2<Esc>");
    QCOMPARE(value("string(g:p)"), QLatin1String("['pre:27', 'leave:27']"));

    // A search line names itself by its direction through both.
    data.doCommand("let g:p = []");
    data.doKeys("gg/alpha<CR>");
    QCOMPARE(value("string(g:p)"), QLatin1String("['pre/13', 'leave/13']"));

    // It is an event this engine knows now, which autocmd_add() is the test of.
    QCOMPARE(value("autocmd_add([{'group': 'FvLp', 'event': 'CmdlineLeavePre',"
                   " 'pattern': '*', 'cmd': 'echo 1'}])"), QLatin1String("v:true"));

    data.doCommand("autocmd! FvLp");
    data.doCommand("unlet! g:p");
}

void FakeVimTester::test_vim_autocmd_source()
{
    // SourcePre and SourcePost, which bracket the reading of a sourced file and
    // name it rather than the file in the window. Measured in Vim 9.1.
    TestData data;
    setup(&data);
    QString message;
    data.handler->commandBufferChanged.set(
        [&](const QString &msg, int, int, int) {
            if (!msg.isEmpty() && !msg.startsWith("--"))
                message = msg;
        });
    const auto value = [&](const QString &expr) {
        message.clear();
        data.doCommand("echo " + expr);
        return message;
    };

    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString path = dir.path() + "/sourced.vim";
    QFile f(path);
    QVERIFY(f.open(QIODevice::WriteOnly));
    f.write("let g:sourced = 'yes'\n");
    f.close();

    data.setText("x");
    data.doCommand("let g:s = []");
    data.doCommand("autocmd FvSrc SourcePre * call add(g:s, 'pre')");
    data.doCommand("autocmd FvSrc SourcePost * call add(g:s, 'post')");

    // One of each, in that order, around the file being read.
    data.doCommand("source " + path);
    QCOMPARE(value("string(g:s)"), QLatin1String("['pre', 'post']"));
    QCOMPARE(value("g:sourced"), QLatin1String("yes"));

    // They name the file that was sourced, which is no file in any window.
    data.doCommand("autocmd! FvSrc");
    data.doCommand("let g:s = []");
    data.doCommand("autocmd FvSrc SourcePre * call add(g:s, expand('<afile>'))");
    data.doCommand("source " + path);
    QCOMPARE(value("g:s[0] =~# 'sourced.vim$'"), QLatin1String("1"));

    // A file that is not there is read by nobody, so nothing is announced.
    data.doCommand("let g:s = []");
    data.doCommand("source " + dir.path() + "/no_such_file.vim");
    QCOMPARE(value("string(g:s)"), QLatin1String("[]"));

    // The pattern picks which file.
    data.doCommand("autocmd! FvSrc");
    data.doCommand("let g:s = []");
    data.doCommand("autocmd FvSrc SourcePre *.vim call add(g:s, 'vim')");
    data.doCommand("autocmd FvSrc SourcePre *.zzz call add(g:s, 'zzz')");
    data.doCommand("source " + path);
    QCOMPARE(value("string(g:s)"), QLatin1String("['vim']"));

    data.doCommand("autocmd! FvSrc");
    data.doCommand("unlet! g:s");
    data.doCommand("unlet! g:sourced");
}

void FakeVimTester::test_vim_autocmd_cmdlinechanged()
{
    // CmdlineChanged, which this engine did not know as an event at all - so
    // ":autocmd" took it, since that validates nothing, and it then never
    // happened. It fires on EVERY change to the line, and its pattern is the
    // character naming the line, as for the rest of the family. All values
    // measured in Vim 9.1.
    TestData data;
    setup(&data);
    QString message;
    data.handler->commandBufferChanged.set(
        [&](const QString &msg, int, int, int) {
            if (!msg.isEmpty() && !msg.startsWith("--"))
                message = msg;
        });
    const auto value = [&](const QString &expr) {
        message.clear();
        data.doCommand("echo " + expr);
        return message;
    };

    data.setText("alpha beta");
    data.doCommand("let g:c = []");
    data.doCommand("autocmd FvCc CmdlineChanged * call add(g:c,"
                   " expand('<afile>') . getcmdline())");

    // One for each character typed, and one more for the backspace.
    data.doKeys(":ab<BS><Esc>");
    QCOMPARE(value("string(g:c)"), QLatin1String("[':a', ':ab', ':a']"));

    // A search line names itself by its direction, and the Return that runs it
    // is not a change to it.
    data.doCommand("let g:c = []");
    data.doKeys("gg/al<CR>");
    QCOMPARE(value("string(g:c)"), QLatin1String("['/a', '/al']"));

    // The pattern picks which line, as for CmdlineEnter and CmdlineLeave.
    data.doCommand("autocmd! FvCc");
    data.doCommand("let g:c = []");
    data.doCommand("autocmd FvCc CmdlineChanged / call add(g:c, 'search')");
    data.doKeys(":ab<Esc>");
    data.doKeys("gg/al<Esc>");
    QCOMPARE(value("string(g:c)"), QLatin1String("['search', 'search']"));

    // It is an event this engine knows now, which autocmd_add() is the test of:
    // that one checks the name, where ":autocmd" takes whatever it is given.
    QCOMPARE(value("autocmd_add([{'group': 'FvCc', 'event': 'CmdlineChanged',"
                   " 'pattern': '*', 'cmd': 'echo 1'}])"), QLatin1String("v:true"));

    data.doCommand("autocmd! FvCc");
    data.doCommand("unlet! g:c");
}

void FakeVimTester::test_vim_autocmd_insertchange()
{
    // InsertChange, fired by <Insert> turning inserting into replacing and
    // back, and v:insertmode, which says which of the two is in force. All
    // values measured in Vim 9.1.
    TestData data;
    setup(&data);
    QString message;
    data.handler->commandBufferChanged.set(
        [&](const QString &msg, int, int, int) {
            if (!msg.isEmpty() && !msg.startsWith("--"))
                message = msg;
        });
    const auto value = [&](const QString &expr) {
        message.clear();
        data.doCommand("echo " + expr);
        return message;
    };

    data.setText("abcdef");
    data.doCommand("let g:m = []");
    data.doCommand("autocmd FvIm InsertEnter * call add(g:m, 'enter:' . v:insertmode)");
    data.doCommand("autocmd FvIm InsertChange * call add(g:m, 'change:' . v:insertmode)");
    data.doCommand("autocmd FvIm InsertLeave * call add(g:m, 'leave:' . v:insertmode)");

    // Inserting, then <Insert> twice, then out again. Each change says the mode
    // that is now in force, and leaving says the one being left.
    data.doKeys("gg0i<Insert><Insert><Esc>");
    QCOMPARE(value("string(g:m)"),
             QLatin1String("['enter:i', 'change:r', 'change:i', 'leave:i']"));

    // Starting in replace mode, changing to insert, then out.
    data.doCommand("let g:m = []");
    data.doKeys("gg0R<Insert><Esc>");
    QCOMPARE(value("string(g:m)"),
             QLatin1String("['enter:r', 'change:i', 'leave:i']"));

    // Replacing and leaving without changing says so both times.
    data.doCommand("let g:m = []");
    data.doKeys("gg0R<Esc>");
    QCOMPARE(value("string(g:m)"), QLatin1String("['enter:r', 'leave:r']"));

    // v:insertmode KEEPS the last one after the mode is left.
    QCOMPARE(value("v:insertmode"), QLatin1String("r"));
    data.doKeys("gg0i<Esc>");
    QCOMPARE(value("v:insertmode"), QLatin1String("i"));

    data.doCommand("autocmd! FvIm");
    data.doCommand("unlet! g:m");
}

void FakeVimTester::test_vim_autocmd_funcundefined()
{
    // FuncUndefined - not in the event table at all before this. Vim's own
    // use for it is autoloading: an autocommand may DEFINE the function, in
    // which case the original call succeeds after all. <afile>/<amatch> are
    // both the function name. Measured in Vim 9.1.
    TestData data;
    setup(&data);
    QString message;
    data.handler->commandBufferChanged.set(
        [&](const QString &msg, int, int, int) {
            if (!msg.isEmpty() && !msg.startsWith("--"))
                message = msg;
        });
    const auto value = [&](const QString &expr) {
        message.clear();
        data.doCommand("echo " + expr);
        return message;
    };

    data.doCommand("let g:m = []");
    data.doCommand("autocmd FvFu FuncUndefined * call add(g:m,"
                    " expand('<afile>') . ':' . expand('<amatch>'))");
    message.clear();
    data.doCommand("echo NoSuchFunc99()");
    QCOMPARE(message, QLatin1String("E117: Unknown function: NoSuchFunc99"));
    QCOMPARE(value("string(g:m)"), QLatin1String("['NoSuchFunc99:NoSuchFunc99']"));

    // If the autocommand DEFINES the function, the call succeeds. Built as a
    // ":source"d file, not separate doCommand() calls - a known harness
    // quirk (see the ":call ...<CR>" mapping note elsewhere in this file)
    // that a definition assembled one call at a time can silently misbehave.
    data.doCommand("autocmd! FvFu");
    data.doCommand("let g:m = []");
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    QFile f(dir.path() + "/fallback.vim");
    QVERIFY(f.open(QIODevice::WriteOnly));
    f.write("function! FvFallback(name)\n"
            "  call add(g:m, a:name)\n"
            "  if a:name ==# 'NoSuchFunc99'\n"
            "    function! NoSuchFunc99()\n"
            "      return 42\n"
            "    endfunction\n"
            "  endif\n"
            "endfunction\n"
            "autocmd FvFu FuncUndefined * call FvFallback(expand('<afile>'))\n");
    f.close();
    data.doCommand("source " + dir.path() + "/fallback.vim");
    QCOMPARE(value("NoSuchFunc99()"), QLatin1String("42"));
    QCOMPARE(value("string(g:m)"), QLatin1String("['NoSuchFunc99']"));

    // Known to autocmd_add() now, where ":autocmd" would have taken any name.
    QCOMPARE(value("autocmd_add([{'group': 'FvFu', 'event': 'FuncUndefined',"
                   " 'pattern': '*', 'cmd': 'echo 1'}])"), QLatin1String("v:true"));

    data.doCommand("autocmd! FvFu");
    data.doCommand("delfunction! FvFallback");
    data.doCommand("delfunction! NoSuchFunc99");
    data.doCommand("unlet! g:m");
}

void FakeVimTester::test_vim_autocmd_cmdundefined()
{
    // CmdUndefined - not in the event table at all before this, same shape
    // as FuncUndefined. Whether a retry after the event succeeds here was
    // NOT measured, so the command still errors either way; only the firing
    // and <afile>/<amatch> naming the command are asserted. Measured in
    // Vim 9.1.
    TestData data;
    setup(&data);
    QString message;
    data.handler->commandBufferChanged.set(
        [&](const QString &msg, int, int, int) {
            if (!msg.isEmpty() && !msg.startsWith("--"))
                message = msg;
        });
    const auto value = [&](const QString &expr) {
        message.clear();
        data.doCommand("echo " + expr);
        return message;
    };

    data.doCommand("let g:m = []");
    data.doCommand("autocmd FvCu CmdUndefined * call add(g:m,"
                    " expand('<afile>') . ':' . expand('<amatch>'))");
    message.clear();
    // Letters only: the general command/argument splitter stops at the
    // first NON-letter character, so a name with a digit in it would be cut
    // short before ever reaching the dispatcher as one word.
    data.doCommand("NoSuchCmdXyz");
    QCOMPARE(message, QLatin1String("E492: Not an editor command: NoSuchCmdXyz"));
    QCOMPARE(value("string(g:m)"), QLatin1String("['NoSuchCmdXyz:NoSuchCmdXyz']"));

    QCOMPARE(value("autocmd_add([{'group': 'FvCu', 'event': 'CmdUndefined',"
                   " 'pattern': '*', 'cmd': 'echo 1'}])"), QLatin1String("v:true"));

    data.doCommand("autocmd! FvCu");
    data.doCommand("unlet! g:m");
}

void FakeVimTester::test_vim_autocmd_optionset()
{
    // OptionSet, whose pattern is matched against the option's NAME and which
    // brings v:option_old, v:option_new, v:option_type and v:option_command
    // with it. All values measured in Vim 9.1, where they are STRINGS even for
    // a boolean.
    TestData data;
    setup(&data);
    QString message;
    data.handler->commandBufferChanged.set(
        [&](const QString &msg, int, int, int) {
            if (!msg.isEmpty() && !msg.startsWith("--"))
                message = msg;
        });
    const auto value = [&](const QString &expr) {
        message.clear();
        data.doCommand("echo " + expr);
        return message;
    };
    const auto go = [&](const QString &command) {
        data.doCommand("let g:o = []");
        data.doCommand(command);
        return value("string(g:o)");
    };

    data.setText("x");
    data.doCommand("set noignorecase");
    data.doCommand("autocmd FvOs OptionSet * call add(g:o, expand('<amatch>')"
                   " . ' ' . v:option_old . '->' . v:option_new"
                   " . ' ' . v:option_type . ' ' . v:option_command)");

    // A boolean, whose values are the strings "0" and "1".
    QCOMPARE(go("set ignorecase"),
             QLatin1String("['ignorecase 0->1 global set']"));
    // Setting it to what it already is still announces it.
    QCOMPARE(go("set ignorecase"),
             QLatin1String("['ignorecase 1->1 global set']"));
    QCOMPARE(go("set noignorecase"),
             QLatin1String("['ignorecase 1->0 global set']"));

    // ":setlocal" and ":setglobal" say which they were.
    QCOMPARE(go("setlocal tabstop=7"),
             QLatin1String("['tabstop 8->7 local setlocal']"));
    QCOMPARE(go("setglobal tabstop=9"),
             QLatin1String("['tabstop 7->9 global setglobal']"));

    // Two options on one line are announced one at a time.
    data.doCommand("set tabstop=8");
    QCOMPARE(go("set shiftwidth=4 tabstop=4"),
             QLatin1String("['shiftwidth 8->4 global set', 'tabstop 8->4 global set']"));

    // A query announces nothing, and neither does a name that is no option.
    QCOMPARE(go("set ignorecase?"), QLatin1String("[]"));
    QCOMPARE(go("set nosuchoptionxyz"), QLatin1String("[]"));

    // The pattern picks the option, which is the part that is no file name.
    data.doCommand("autocmd! FvOs");
    data.doCommand("autocmd FvOs OptionSet ignorecase call add(g:o, 'only-ic')");
    QCOMPARE(go("set ignorecase"), QLatin1String("['only-ic']"));
    QCOMPARE(go("set tabstop=6"), QLatin1String("[]"));

    // The scope the command did not name says nothing.
    data.doCommand("autocmd! FvOs");
    data.doCommand("autocmd FvOs OptionSet tabstop call add(g:o,"
                   " 'l=' . v:option_oldlocal . ' g=' . v:option_oldglobal)");
    QCOMPARE(go("setlocal tabstop=3"), QLatin1String("['l=6 g=']"));
    QCOMPARE(go("setglobal tabstop=2"), QLatin1String("['l= g=3']"));

    // v:option_new is nothing outside an autocommand.
    QCOMPARE(value("v:option_new"), QLatin1String(""));

    data.doCommand("autocmd! FvOs");
    data.doCommand("set tabstop=8");
    data.doCommand("set noignorecase");
    data.doCommand("unlet! g:o");
}

void FakeVimTester::test_vim_autocmd_encodingchanged()
{
    // EncodingChanged, and 'encoding' itself, which was accepted but not
    // stored before - one of the unimplementedOption() fallbacks, so a
    // written value was silently dropped and "&encoding" read back empty.
    // Nothing here acts on the charset (Qt Creator owns what a document is
    // really read and written as), but the VALUE is recorded, which is what
    // a script saving and restoring an option needs. Values taken from
    // Vim 9.1. Shared with every other test slot, so put back at the end.
    TestData data;
    setup(&data);
    QString message;
    data.handler->commandBufferChanged.set(
        [&](const QString &msg, int, int, int) {
            if (!msg.isEmpty() && !msg.startsWith("--"))
                message = msg;
        });
    const auto value = [&](const QString &expr) {
        message.clear();
        data.doCommand("echo " + expr);
        return message;
    };
    data.setText("x");

    // The value round-trips, under either name - Vim treats "enc" and
    // "encoding" as one option.
    QCOMPARE(value("&encoding"), QLatin1String("utf-8"));
    data.doCommand("set encoding=latin1");
    QCOMPARE(value("&encoding"), QLatin1String("latin1"));
    QCOMPARE(value("&enc"), QLatin1String("latin1"));
    data.doCommand("set enc=utf-8");
    QCOMPARE(value("&encoding"), QLatin1String("utf-8"));

    // Changing it announces EncodingChanged, before the generic OptionSet.
    // One firing reaches FileEncoding's registrations too - Vim treats the
    // two event names as one - so no order between those two is asserted.
    data.doCommand("let g:e = []");
    data.doCommand("autocmd FvEnc EncodingChanged * call add(g:e, 'enc')");
    data.doCommand("autocmd FvEnc OptionSet * call add(g:e, 'opt:' . expand('<amatch>'))");
    data.doCommand("set encoding=latin1");
    QCOMPARE(value("string(g:e)"), QLatin1String("['enc', 'opt:encoding']"));

    // A registration for the other name of the same event comes up as well.
    data.doCommand("autocmd! FvEnc");
    data.doCommand("let g:e = []");
    data.doCommand("autocmd FvEnc FileEncoding * call add(g:e, 'fenc')");
    data.doCommand("set encoding=utf-8");
    QCOMPARE(value("string(g:e)"), QLatin1String("['fenc']"));

    // Setting 'fileencoding' - a DIFFERENT option, despite FileEncoding
    // being the event's other name - announces neither. This one holds
    // structurally rather than by the firing condition: 'fileencoding' has
    // no aspect, so the ":set" loop skips it before reaching any event at
    // all. Kept as a regression guard for that, not as a test of the
    // condition.
    data.doCommand("let g:e = []");
    data.doCommand("set fileencoding=latin1");
    QCOMPARE(value("string(g:e)"), QLatin1String("[]"));

    // An option that changes nothing still announces, as OptionSet does.
    data.doCommand("let g:e = []");
    data.doCommand("set encoding=utf-8");
    QCOMPARE(value("string(g:e)"), QLatin1String("['fenc']"));

    // Known to autocmd_add() now, under both names.
    QCOMPARE(value("autocmd_add([{'group': 'FvEnc', 'event': 'EncodingChanged',"
                   " 'pattern': '*', 'cmd': 'echo 1'}])"), QLatin1String("v:true"));
    QCOMPARE(value("autocmd_add([{'group': 'FvEnc', 'event': 'FileEncoding',"
                   " 'pattern': '*', 'cmd': 'echo 1'}])"), QLatin1String("v:true"));

    data.doCommand("autocmd! FvEnc");
    data.doCommand("unlet! g:e");
    data.doCommand("set encoding=utf-8");
}

void FakeVimTester::test_vim_normal_bang()
{
    // ":normal" runs its keys THROUGH the mappings; ":normal!" runs the keys
    // themselves. The bang was not looked at, so both ignored mappings. All
    // values measured in Vim 9.1.
    TestData data;
    setup(&data);
    QString message;
    data.handler->commandBufferChanged.set(
        [&](const QString &msg, int, int, int) {
            if (!msg.isEmpty() && !msg.startsWith("--"))
                message = msg;
        });
    const auto value = [&](const QString &expr) {
        message.clear();
        data.doCommand("echo " + expr);
        return message;
    };
    const auto go = [&](const QString &command) {
        data.setText("abc" N "def");
        data.doCommand("let g:r = []");
        data.doCommand(command);
    };

    data.doCommand("nnoremap x :call add(g:r, 'mapped-x')<CR>");
    data.doCommand("nmap y xx");

    // Without the bang the mapping is what runs, so the text is untouched.
    go("normal x");
    QCOMPARE(value("string(g:r)"), QLatin1String("['mapped-x']"));
    QCOMPARE(data.text(), QString("abc" N "def"));

    // With it the key itself runs, and "x" deletes a character.
    go("normal! x");
    QCOMPARE(value("string(g:r)"), QLatin1String("[]"));
    QCOMPARE(data.text(), QString("bc" N "def"));

    // What a mapping leaves behind may be mapped again: "y" stands for "xx",
    // and each of those is the mapped "x".
    go("normal y");
    QCOMPARE(value("string(g:r)"), QLatin1String("['mapped-x', 'mapped-x']"));
    // With the bang, "y" is an operator with no motion after it and nothing
    // comes of it.
    go("normal! y");
    QCOMPARE(value("string(g:r)"), QLatin1String("[]"));
    QCOMPARE(data.text(), QString("abc" N "def"));

    // A range runs the keys once per line, either way.
    go("1,2normal x");
    QCOMPARE(value("string(g:r)"), QLatin1String("['mapped-x', 'mapped-x']"));
    go("1,2normal! x");
    QCOMPARE(data.text(), QString("bc" N "ef"));

    data.doCommand("nunmap x");
    data.doCommand("nunmap y");
    data.doCommand("unlet! g:r");
}

void FakeVimTester::test_vim_substitute_count()
{
    // A count behind the flags of a ":substitute" means that many lines,
    // counted from the LAST line of the range - not the range over and over,
    // which is what this did and which changes nothing after the first pass.
    // All values measured in Vim 9.1, on four lines of "ab".
    TestData data;
    setup(&data);
    const auto four = [&] { data.setText("ab" N "ab" N "ab" N "ab"); };

    four();
    data.doCommand("1substitute/a/X/ 2");
    QCOMPARE(data.text(), QString("Xb" N "Xb" N "ab" N "ab"));
    four();
    data.doCommand("2substitute/a/X/ 2");
    QCOMPARE(data.text(), QString("ab" N "Xb" N "Xb" N "ab"));
    // Flags and a count together.
    four();
    data.doCommand("1substitute/a/X/g 2");
    QCOMPARE(data.text(), QString("Xb" N "Xb" N "ab" N "ab"));

    // With no range the current line is the one it counts from.
    four();
    data.doCommand("substitute/a/X/ 3");
    QCOMPARE(data.text(), QString("Xb" N "Xb" N "Xb" N "ab"));

    // No count reaches only the range itself.
    four();
    data.doCommand("1substitute/a/X/");
    QCOMPARE(data.text(), QString("Xb" N "ab" N "ab" N "ab"));

    // A count reaching past the last line stops there rather than failing.
    four();
    data.doCommand("3substitute/a/X/ 9");
    QCOMPARE(data.text(), QString("ab" N "ab" N "Xb" N "Xb"));

    // A range of several lines still counts from its last one, so what lies
    // before that line is left alone.
    four();
    data.doCommand("1,2substitute/a/X/ 2");
    QCOMPARE(data.text(), QString("ab" N "Xb" N "Xb" N "ab"));

    // A count of ONE narrows just as much, which is what tells a count that
    // was given from one that was not: the range is dropped either way.
    four();
    data.doCommand("1,3substitute/a/X/ 1");
    QCOMPARE(data.text(), QString("ab" N "ab" N "Xb" N "ab"));
}

void FakeVimTester::test_vim_script_flatten()
{
    // flatten() takes the lists inside a list apart, as deep as it is told to,
    // and flattennew() leaves the list it was given alone. Values taken from
    // Vim 9.1.
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
    QCOMPARE(value("string(flatten([1, [2, [3, [4]]]]))"), QLatin1String("[1, 2, 3, 4]"));
    QCOMPARE(value("string(flatten([1, [2, [3, [4]]]], 1))"), QLatin1String("[1, 2, [3, [4]]]"));
    // The one takes the list apart where it stands, the other hands back a new one.
    data.doCommand("let g:l = [1, [2]]");
    QCOMPARE(value("string(flattennew(g:l))"), QLatin1String("[1, 2]"));
    QCOMPARE(value("string(g:l)"), QLatin1String("[1, [2]]"));
    QCOMPARE(value("string(flatten(g:l))"), QLatin1String("[1, 2]"));
    QCOMPARE(value("string(g:l)"), QLatin1String("[1, 2]"));
    data.doCommand("unlet g:l");
}

void FakeVimTester::test_vim_script_fullcommand()
{
    // fullcommand() says what command a name stands for, spelled out, and nothing
    // where it stands for none. A range, a leading colon and a "!" are passed
    // over, a name of the user's own is looked for as well, and Vim lets a few
    // characters follow a name directly: anything after "k", the flags of ":s"
    // and the two of ":d". Every value here is what Vim 9.1 answers; the table
    // behind it was harvested from Vim and its lookup checked against Vim's
    // answer for every prefix of every command it knows.
    TestData data;
    setup(&data);
    QString message;
    data.handler->commandBufferChanged.set(
        [&](const QString &msg, int, int, int) {
            if (!msg.startsWith("--"))
                message = msg;
        });
    const auto full = [&](const QString &name) {
        message.clear();
        data.doCommand("echo fullcommand(" + name + ")");
        return message;
    };
    data.doCommand("command! -nargs=0 Foobar echo 'hi'");
    // An abbreviation, spelled out or as short as it may be.
    QCOMPARE(full("'s'"), QLatin1String("substitute"));
    QCOMPARE(full("'sub'"), QLatin1String("substitute"));
    QCOMPARE(full("'substitute'"), QLatin1String("substitute"));
    QCOMPARE(full("'g'"), QLatin1String("global"));
    QCOMPARE(full("'v'"), QLatin1String("vglobal"));
    QCOMPARE(full("'x'"), QLatin1String("xit"));
    QCOMPARE(full("'e'"), QLatin1String("edit"));
    QCOMPARE(full("'ene'"), QLatin1String("enew"));
    QCOMPARE(full("'sil'"), QLatin1String("silent"));
    QCOMPARE(full("'fu'"), QLatin1String("function"));
    QCOMPARE(full("'argd'"), QLatin1String("argdelete"));
    QCOMPARE(full("'argdo'"), QLatin1String("argdo"));
    QCOMPARE(full("'foldd'"), QLatin1String("folddoopen"));
    // Which of two commands a short name belongs to is Vim's to say: "di" is
    // display, where diffupdate wants three letters, and "en" is endif.
    QCOMPARE(full("'di'"), QLatin1String("display"));
    QCOMPARE(full("'en'"), QLatin1String("endif"));
    // A command whose name is not a word of letters, and a second character
    // that belongs to what follows rather than to the name.
    QCOMPARE(full("'&'"), QLatin1String("&"));
    QCOMPARE(full("'&&'"), QLatin1String("&"));
    QCOMPARE(full("'~'"), QLatin1String("~"));
    QCOMPARE(full("'@@'"), QLatin1String("@"));
    // What Vim lets follow a name directly.
    QCOMPARE(full("'ke'"), QLatin1String("k"));
    QCOMPARE(full("'dl'"), QLatin1String("delete"));
    QCOMPARE(full("'dp'"), QLatin1String("delete"));
    QCOMPARE(full("'sc'"), QLatin1String("substitute"));
    QCOMPARE(full("'si'"), QLatin1String("substitute"));
    QCOMPARE(full("'sig'"), QLatin1String("sign"));
    QCOMPARE(full("'sm'"), QLatin1String("smagic"));
    // A colon, a range and a "!" are none of the name.
    QCOMPARE(full("':s'"), QLatin1String("substitute"));
    QCOMPARE(full("'3s'"), QLatin1String("substitute"));
    QCOMPARE(full("'%s'"), QLatin1String("substitute"));
    QCOMPARE(full("'.,+2d'"), QLatin1String("delete"));
    QCOMPARE(full("'$put'"), QLatin1String("put"));
    QCOMPARE(full("\"'<,'>s\""), QLatin1String("substitute"));
    QCOMPARE(full("'w!'"), QLatin1String("write"));
    QCOMPARE(full("'norm!'"), QLatin1String("normal"));
    // A name of the user's own, whole or shortened.
    QCOMPARE(full("'Foobar'"), QLatin1String("Foobar"));
    QCOMPARE(full("'Foo'"), QLatin1String("Foobar"));
    // And nothing where there is no such command.
    QCOMPARE(full("'zzz'"), QString());
    QCOMPARE(full("'*'"), QString());
    QCOMPARE(full("''"), QString());
    data.doCommand("delcommand Foobar");
}

void FakeVimTester::test_vim_script_indexof()
{
    // indexof() says where the first item stands that an expression holds for,
    // whether the expression is a funcref or a string reading v:key and v:val.
    // Values taken from Vim 9.1.
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
    QCOMPARE(value("indexof([1, 2, 3], {i, v -> v == 2})"), QLatin1String("1"));
    QCOMPARE(value("indexof(['a', 'b', 'c'], 'v:val ==# \"b\"')"), QLatin1String("1"));
    QCOMPARE(value("indexof(['a', 'b'], 'v:key == 1')"), QLatin1String("1"));
    QCOMPARE(value("indexof([1, 2], {i, v -> v == 9})"), QLatin1String("-1"));
    // Where to start looking is one of the things the options may say.
    QCOMPARE(value("indexof([1, 2, 1], {i, v -> v == 1}, {'startidx': 1})"),
             QLatin1String("2"));
}

void FakeVimTester::test_vim_script_error_inspection()
{
    // What a script uses to see what happened: strtrans() to show a string with
    // the characters that do not print, getreginfo() for what a register holds,
    // and v:errmsg for the last error. Values taken from Vim 9.1.
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

    QCOMPARE(value("strtrans(\"a\\<Esc>b\")"), QLatin1String("a^[b"));
    QCOMPARE(value("strtrans('plain')"), QLatin1String("plain"));
    QCOMPARE(value("strtrans(\"a\\<C-a>b\\<Tab>c\")"), QLatin1String("a^Ab^Ic"));
    QCOMPARE(value("strtrans(nr2char(127))"), QLatin1String("^?"));
    // Vim holds a NUL as a line break and shows it as "^@".
    QCOMPARE(value("strtrans(\"a\\nb\")"), QLatin1String("a^@b"));
    // A character it cannot show at all it writes in hex, and it knows a few
    // ranges of those besides the ones below a blank.
    QCOMPARE(value("strtrans(nr2char(128))"), QLatin1String("<80>"));
    QCOMPARE(value("strtrans(nr2char(159))"), QLatin1String("<9f>"));
    QCOMPARE(value("strtrans(nr2char(0x180b))"), QLatin1String("<180b>"));
    QCOMPARE(value("strtrans(nr2char(0x200b))"), QLatin1String("<200b>"));
    QCOMPARE(value("strtrans(nr2char(0x202a))"), QLatin1String("<202a>"));
    QCOMPARE(value("strtrans(nr2char(0x2060))"), QLatin1String("<2060>"));
    QCOMPARE(value("strtrans(nr2char(0xfeff))"), QLatin1String("<feff>"));
    QCOMPARE(value("strtrans(nr2char(0xfff9))"), QLatin1String("<fff9>"));
    QCOMPARE(value("strtrans(nr2char(0xffff))"), QLatin1String("<ffff>"));
    // The character on either side of a range is shown as it stands.
    for (const char *n : {"0x00ad", "0x180a", "0x180f", "0x2010", "0x202f", "0x2070",
                          "0xfff8", "0xfffc", "0x2028"}) {
        const QString same = QLatin1String("strtrans(nr2char(") + n + ")) ==# nr2char(" + n + ")";
        QCOMPARE(value(same), QLatin1String("1"));
    }
    // What it can show stands as it is, whatever its number.
    QCOMPARE(value("strtrans(nr2char(233)) ==# nr2char(233)"), QLatin1String("1"));
    QCOMPARE(value("strtrans(nr2char(955)) ==# nr2char(955)"), QLatin1String("1"));
    QCOMPARE(value("strtrans(nr2char(160)) ==# nr2char(160)"), QLatin1String("1"));
    QCOMPARE(value("strtrans(nr2char(8232)) ==# nr2char(8232)"), QLatin1String("1"));
    // The ones Vim writes in angle brackets, and a letter that just prints.
    QCOMPARE(value("strtrans(nr2char(128) . nr2char(159))"), QLatin1String("<80><9f>"));
    QCOMPARE(value("strtrans('e' . nr2char(233))"), QString::fromUtf8("e\xc3\xa9"));

    data.doCommand("call setreg('a', \"line\\n\", 'V')");
    data.doCommand("call setreg('b', 'chars', 'v')");
    data.doCommand("call setreg('z', '')");
    // Point the unnamed register somewhere known: the registers are shared with
    // every other test, and getreginfo() tells where it points.
    data.doCommand("call setreg('\"', 'aside')");
    QCOMPARE(value("getreginfo('a')"),
             QLatin1String("{'isunnamed': v:false, 'regcontents': ['line'], 'regtype': 'V'}"));
    QCOMPARE(value("getreginfo('b')"),
             QLatin1String("{'isunnamed': v:false, 'regcontents': ['chars'], 'regtype': 'v'}"));
    // An empty register has nothing to tell.
    QCOMPARE(value("getreginfo('z')"), QLatin1String("{}"));

    // The error is held even where it is not shown, which is what the
    // "empty it, do the thing, look at it" idiom reads.
    data.doCommand("let v:errmsg = ''");
    data.doCommand("silent! call NoSuchFunction()");
    QCOMPARE(value("v:errmsg"), QLatin1String("E117: Unknown function: NoSuchFunction"));
    data.doCommand("let v:errmsg = ''");
    data.doCommand("silent! echo nosuchvariable");
    QCOMPARE(value("v:errmsg"), QLatin1String("E121: Undefined variable: nosuchvariable"));
    // A script may put its own text there.
    data.doCommand("let v:errmsg = 'set by hand'");
    QCOMPARE(value("v:errmsg"), QLatin1String("set by hand"));
    // An error a ":try" catches is an exception, and leaves v:errmsg alone.
    data.doCommand("let v:errmsg = ''");
    data.doCommand("try | call NoSuchFunction() | catch | endtry");
    QCOMPARE(value("v:errmsg"), QString());

    // And an expression that is none names itself the way Vim names it.
    QCOMPARE(value("eval('1+')"), QLatin1String("E15: Invalid expression: \"1+\""));
    QCOMPARE(value("eval('')"), QLatin1String("E15: Invalid expression: \"\""));
    data.doCommand("let v:errmsg = ''");
}

void FakeVimTester::test_vim_script_maparg_dict()
{
    // maparg() names a key the way Vim names it, and with a fourth argument
    // answers with all of the mapping rather than its right-hand side alone -
    // which is how a plugin reads the flags one carries. Values taken from
    // Vim 9.1.
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
    data.doCommand("nnoremap K1 a<Esc>b<CR>c<Tab>d<Space>e<BS>f<Del>");
    data.doCommand("nnoremap K2 a<Left>b<F1>c<Home>d<PageUp>e<S-Left>");
    data.doCommand("nnoremap K3 a<C-x>b<C-X>c<lt>d<Bar>");
    data.doCommand("nnoremap <silent> D1 :call Foo()<CR>");
    data.doCommand("nnoremap <expr> D2 Bar()");
    data.doCommand("inoremap D3 x");
    data.doCommand("nnoremap [<Space> iX<Esc>");

    // A key with a name is spelled as Vim spells it, and one that stands for a
    // character comes back as that character.
    QCOMPARE(value("maparg('K1', 'n')"), QLatin1String("a<Esc>b<CR>c<Tab>d e<BS>f<Del>"));
    QCOMPARE(value("maparg('K2', 'n')"),
             QLatin1String("a<Left>b<F1>c<Home>d<PageUp>e<S-Left>"));
    QCOMPARE(value("maparg('K3', 'n')"), QLatin1String("a<C-X>b<C-X>c<d|"));
    // An expression mapping answers with the expression.
    QCOMPARE(value("maparg('D2', 'n')"), QLatin1String("Bar()"));

    // The flags a mapping carries, including the script it came from and the
    // mode as a number.
    QCOMPARE(value("maparg('D1', 'n', 0, 1).lhs"), QLatin1String("D1"));
    QCOMPARE(value("maparg('D1', 'n', 0, 1).rhs"), QLatin1String(":call Foo()<CR>"));
    QCOMPARE(value("maparg('D1', 'n', 0, 1).silent"), QLatin1String("1"));
    // The mode as Vim numbers it: normal 1, visual 2, operator-pending 4,
    // command line 8, insert 16, and "v" is visual and select together.
    QCOMPARE(value("maparg('D1', 'n', 0, 1).mode_bits"), QLatin1String("1"));
    QCOMPARE(value("maparg('D3', 'i', 0, 1).mode_bits"), QLatin1String("16"));
    QCOMPARE(value("maparg('D1', 'n', 0, 1).scriptversion"), QLatin1String("1"));
    // A mapping written by hand comes from no script.
    QCOMPARE(value("maparg('D1', 'n', 0, 1).sid"), QLatin1String("0"));
    QCOMPARE(value("maparg('D1', 'n', 0, 1).lnum"), QLatin1String("0"));
    QCOMPARE(value("maparg('D1', 'n', 0, 1).expr"), QLatin1String("0"));
    QCOMPARE(value("maparg('D1', 'n', 0, 1).noremap"), QLatin1String("1"));
    QCOMPARE(value("maparg('D1', 'n', 0, 1).mode"), QLatin1String("n"));
    QCOMPARE(value("maparg('D2', 'n', 0, 1).expr"), QLatin1String("1"));
    QCOMPARE(value("maparg('D2', 'n', 0, 1).rhs"), QLatin1String("Bar()"));
    QCOMPARE(value("maparg('D2', 'n', 0, 1).silent"), QLatin1String("0"));
    QCOMPARE(value("maparg('D3', 'i', 0, 1).mode"), QLatin1String("i"));
    QCOMPARE(value("maparg('D3', 'i', 0, 1).buffer"), QLatin1String("0"));
    // The keys come back in the notation, where the right-hand side has the
    // character itself.
    QCOMPARE(value("maparg('[<Space>', 'n', 0, 1).lhs"), QLatin1String("[<Space>"));
    // Nothing mapped, nothing to tell.
    QCOMPARE(value("maparg('NOPE', 'n', 0, 1)"), QLatin1String("{}"));

    // A mapping written in a script remembers which one, and the line of the
    // file it stands on - the comment above it counts as a line. The number a
    // script is given depends on how many came before it, so it is only
    // compared against 0.
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    QFile f(dir.path() + "/m.vim");
    QVERIFY(f.open(QIODevice::WriteOnly));
    f.write("\" a mapping on the third line\n"
            "let g:unused = 1\n"
            "nnoremap D4 iZ<Esc>\n");
    f.close();
    data.doCommand("source " + dir.path() + "/m.vim");
    QCOMPARE(value("maparg('D4', 'n', 0, 1).lnum"), QLatin1String("3"));
    QCOMPARE(value("maparg('D4', 'n', 0, 1).sid > 0"), QLatin1String("1"));
    data.doCommand("nunmap D4 | unlet g:unused");

    // The mappings live in a table shared with every other test.
    data.doCommand("nunmap K1 | nunmap K2 | nunmap K3 | nunmap D1 | nunmap D2");
    data.doCommand("iunmap D3 | nunmap [<Space>");
}

void FakeVimTester::test_vim_line_address_zero_and_counts()
{
    // ":0" names the place before the first line; a count that wants a line
    // below the last one does nothing at all; and the marks a linewise operator
    // leaves hold the ends of the region, columns and all. Values taken from
    // Vim 9.1.
    TestData data;
    setup(&data);
    QString message;
    data.handler->commandBufferChanged.set(
        [&](const QString &msg, int, int, int) {
            if (!msg.startsWith("--"))
                message = msg;
        });
    const auto put = [&](const char *command) -> QString {
        data.setText(X "alpha" N "beta" N "gamma");
        data.doCommand("call setreg('a', \"AAA\\n\", 'V')");
        data.doKeys("j0l");
        data.doCommand(command);
        message.clear();
        data.doCommand("echo line('.') . ',' . col('.')");
        return QString::fromUtf8(data.text()).replace(QLatin1Char('\n'), QLatin1String("/"))
               + "  at " + message;
    };
    QCOMPARE(put("0put a"), QLatin1String("AAA/alpha/beta/gamma  at 1,1"));
    // Which is what ":1put!" has always meant.
    QCOMPARE(put("1put! a"), QLatin1String("AAA/alpha/beta/gamma  at 1,1"));
    // A line address of 0 changes nothing for the ordinary forms.
    QCOMPARE(put("put a"), QLatin1String("alpha/beta/AAA/gamma  at 3,1"));
    QCOMPARE(put("$put a"), QLatin1String("alpha/beta/gamma/AAA  at 4,1"));

    const auto run = [&](const char *start, const char *keys) -> QString {
        data.setText(X "alpha" N "beta" N "gamma");
        data.doCommand("call setreg('\"', 'UNTOUCHED')");
        data.doKeys(start);
        data.doKeys(keys);
        message.clear();
        data.doCommand("echo '[' . substitute(getreg('\"'), \"\\n\", '\\\\n', 'g') . ']'");
        return message + " text=["
               + QString::fromUtf8(data.text()).replace(QLatin1Char('\n'), QLatin1String("/"))
               + "]";
    };
    // One line is always there to take.
    QCOMPARE(run("jj0", "yy"), QLatin1String("[gamma\\n] text=[alpha/beta/gamma]"));
    // Two are not, on the last line, and then nothing happens at all - not even
    // to the register.
    QCOMPARE(run("jj0", "2yy"), QLatin1String("[UNTOUCHED] text=[alpha/beta/gamma]"));
    QCOMPARE(run("jj0", "y2y"), QLatin1String("[UNTOUCHED] text=[alpha/beta/gamma]"));
    QCOMPARE(run("jj0", "3yy"), QLatin1String("[UNTOUCHED] text=[alpha/beta/gamma]"));
    QCOMPARE(run("jj0", "2dd"), QLatin1String("[UNTOUCHED] text=[alpha/beta/gamma]"));
    QCOMPARE(run("jj0", "2Y"), QLatin1String("[UNTOUCHED] text=[alpha/beta/gamma]"));
    // Asking for more lines than there are takes what is there, anywhere else.
    QCOMPARE(run("j0", "2yy"), QLatin1String("[beta\\ngamma\\n] text=[alpha/beta/gamma]"));
    QCOMPARE(run("j0", "3yy"), QLatin1String("[beta\\ngamma\\n] text=[alpha/beta/gamma]"));
    QCOMPARE(run("0", "4yy"),
             QLatin1String("[alpha\\nbeta\\ngamma\\n] text=[alpha/beta/gamma]"));
    QCOMPARE(run("j0", "3dd"), QLatin1String("[beta\\ngamma\\n] text=[alpha]"));

    // The marks a linewise "g@" leaves.
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    QFile f(dir.path() + "/k.vim");
    QVERIFY(f.open(QIODevice::WriteOnly));
    f.write("let g:m = []\n"
            "function! Kind(type)\n"
            "  call add(g:m, a:type . ' [' . string(getpos(\"'[\")) . '] ]'"
            " . string(getpos(\"']\")))\n"
            "endfunction\n");
    f.close();
    data.doCommand("source " + dir.path() + "/k.vim");
    data.doCommand("set opfunc=Kind | let g:m = []");
    const auto atCursor = [&](int line, int col, const char *keys) {
        data.setText(X "alpha" N "beta" N "gamma");
        data.doCommand(QString("call cursor(%1, %2)").arg(line).arg(col));
        data.doKeys(keys);
    };
    atCursor(2, 3, "g@_");
    atCursor(1, 4, "g@j");
    atCursor(2, 2, "g@2_");
    message.clear();
    data.doCommand("echo join(g:m, ' | ')");
    QCOMPARE(message, QLatin1String(
        "line [[0, 2, 1, 0]] ][0, 2, 3, 0] | "
        "line [[0, 1, 4, 0]] ][0, 2, 4, 0] | "
        "line [[0, 2, 2, 0]] ][0, 3, 1, 0]"));
    data.doCommand("set opfunc= | delfunction Kind | unlet g:m");

    // 'report' says how many lines are worth a message, as in Vim: more than
    // 'report' lines are reported, that many or fewer are not.
    message.clear();
    data.doCommand("echo &report");
    QCOMPARE(message, QLatin1String("2"));
    const auto yankMessage = [&](const char *keys) {
        data.setText(X "a" N "b" N "c" N "d" N "e");
        message.clear();
        data.doKeys(keys);
        return message;
    };
    QCOMPARE(yankMessage("gg3yy"), QLatin1String("3 lines yanked"));
    QCOMPARE(yankMessage("gg2yy"), QString());
    data.doCommand("set report=4");
    QCOMPARE(yankMessage("gg3yy"), QString());
    QCOMPARE(yankMessage("gg5yy"), QLatin1String("5 lines yanked"));
    data.doCommand("set report=0");
    QCOMPARE(yankMessage("gg2yy"), QLatin1String("2 lines yanked"));
    // The option is shared with every other test.
    data.doCommand("set report=2");
}

void FakeVimTester::test_vim_operator_motion_at_the_edge()
{
    // A motion that cannot go anywhere gives up on the operator instead of
    // letting it have the line the cursor is on. Moving fewer lines than asked
    // for is fine. Values taken from Vim 9.1.
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
        data.doCommand("call setreg('\"', 'UNTOUCHED')");
        data.doKeys(start);
        data.doKeys(keys);
        message.clear();
        data.doCommand("echo '[' . substitute(getreg('\"'), \"\\n\", '\\\\n', 'g') . ']'");
        return message + " text=["
               + QString::fromUtf8(data.text()).replace(QLatin1Char('\n'), QLatin1String("/"))
               + "]";
    };

    // Downwards from the last line, and upwards from the first: nothing at all.
    QCOMPARE(run("jj0l", "yj"), QLatin1String("[UNTOUCHED] text=[alpha/beta/gamma]"));
    QCOMPARE(run("jj0l", "dj"), QLatin1String("[UNTOUCHED] text=[alpha/beta/gamma]"));
    QCOMPARE(run("0l", "yk"), QLatin1String("[UNTOUCHED] text=[alpha/beta/gamma]"));
    // One line down is there, so the operator has two lines.
    QCOMPARE(run("j0l", "yj"), QLatin1String("[beta\\ngamma\\n] text=[alpha/beta/gamma]"));
    QCOMPARE(run("jj0l", "yk"), QLatin1String("[beta\\ngamma\\n] text=[alpha/beta/gamma]"));
    // Asking for more lines than there are takes what is there.
    QCOMPARE(run("j0l", "y2j"), QLatin1String("[beta\\ngamma\\n] text=[alpha/beta/gamma]"));
    QCOMPARE(run("j0l", "y2k"), QLatin1String("[alpha\\nbeta\\n] text=[alpha/beta/gamma]"));
    QCOMPARE(run("j0l", "d2j"), QLatin1String("[beta\\ngamma\\n] text=[alpha]"));
    // On its own the key only fails: the cursor stays and nothing is cleared.
    data.setText(X "alpha" N "beta" N "gamma");
    data.doKeys("jj0lj");
    message.clear();
    data.doCommand("echo line('.') . ',' . col('.')");
    QCOMPARE(message, QLatin1String("3,2"));
    // And a count that cannot be met in full still moves as far as it can.
    data.doKeys("gg0l2j");
    message.clear();
    data.doCommand("echo line('.')");
    QCOMPARE(message, QLatin1String("3"));
}

void FakeVimTester::test_vim_script_characters_and_bytes()
{
    // Vim counts a string in bytes (strlen, strpart, stridx) and offers the
    // character-wise ones beside them; here everything is counted in characters,
    // so strlen() answers what strchars() answers. Values taken from Vim 9.1,
    // with the two that differ named.
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
    // The word is "a", an e with an accent, "b" and "c": four characters
    // and five bytes.
    // The escape has to end before "bc": in C++ "\xa9bc" is one hex escape.
    const QString word = QString::fromUtf8("'a\xc3\xa9" "bc'");

    QCOMPARE(value("strchars(" + word + ")"), QLatin1String("4"));
    QCOMPARE(value("strwidth(" + word + ")"), QLatin1String("4"));
    QCOMPARE(value("strchars('')"), QLatin1String("0"));
    // Vim says 5 here, counting the bytes; every column and offset in this
    // engine is a character, so this agrees with those instead.
    QCOMPARE(value("strlen(" + word + ")"), QLatin1String("4"));
    // A piece counted in characters, which Vim's strcharpart() gives as well.
    QCOMPARE(value("strcharpart(" + word + ", 1, 1)"), QString::fromUtf8("\xc3\xa9"));
    QCOMPARE(value("strcharpart(" + word + ", 0, 2)"), QString::fromUtf8("a\xc3\xa9"));
    QCOMPARE(value("strcharpart(" + word + ", 2)"), QLatin1String("bc"));
    // Between the two ways of counting.
    QCOMPARE(value("byteidx(" + word + ", 0)"), QLatin1String("0"));
    QCOMPARE(value("byteidx(" + word + ", 2)"), QLatin1String("3"));
    QCOMPARE(value("charidx(" + word + ", 3)"), QLatin1String("2"));
    QCOMPARE(value("charidx(" + word + ", 1)"), QLatin1String("1"));
    // Past the end there is nothing to point at.
    QCOMPARE(value("byteidx(" + word + ", 9)"), QLatin1String("-1"));
    QCOMPARE(value("charidx(" + word + ", 9)"), QLatin1String("-1"));
}

void FakeVimTester::test_vim9_type_annotations()
{
    // The types a Vim9 script writes down: on a lambda's return, and on a
    // parameter that carries a type with a comma of its own. All of it is read
    // and passed over. Values taken from Vim 9.1 running the same script.
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
    QFile f(dir.path() + "/t.vim");
    QVERIFY(f.open(QIODevice::WriteOnly));
    f.write("vim9script\n"
            "var Double = (x: number): number => x * 2\n"
            "def Count(d: dict<number>): number\n"
            "  var total = 0\n"
            "  for v in values(d)\n"
            "    total += v\n"
            "  endfor\n"
            "  return total\n"
            "enddef\n"
            "def Pair(a: dict<string>, b: list<number>): string\n"
            "  return a['k'] .. len(b)\n"
            "enddef\n"
            "var Len = (d: dict<number>): number => len(d)\n"
            "def Apply(F: func(number, string): string, n: number): string\n"
            "  return F(n, 'x')\n"
            "enddef\n"
            "def Join(n: number, s: string): string\n"
            "  return s .. n\n"
            "enddef\n"
            "g:answers = [Double(21), Count({a: 1, b: 2}), Pair({k: 'x'}, [1, 2, 3]),"
            " Len({a: 1}), Apply(Join, 7)]\n");
    f.close();
    data.doCommand("let g:e = ''");
    data.doCommand("try | source " + dir.path() + "/t.vim"
                   " | catch | let g:e = v:exception | endtry");
    message.clear();
    data.doCommand("echo g:e");
    QCOMPARE(message, QString());
    message.clear();
    data.doCommand("echo string(g:answers)");
    QCOMPARE(message, QLatin1String("[42, 3, 'x3', 1, 'x7']"));
    data.doCommand("unlet g:answers | unlet g:e");
}

void FakeVimTester::test_vim9_dict_literal()
{
    // In Vim9 the key of a dictionary item is the word itself, and "[expr]" is
    // how a key is worked out instead; legacy Vim reads a plain expression
    // there. Values taken from Vim 9.1 running the same script.
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
    QFile f(dir.path() + "/d.vim");
    QVERIFY(f.open(QIODevice::WriteOnly));
    f.write("vim9script\n"
            "var k = 'q'\n"
            "g:out = []\n"
            "add(g:out, string({a: 1}))\n"
            "add(g:out, string({a: 1, b_2: 2}))\n"
            "add(g:out, string({[k]: 1}))\n"
            "add(g:out, string({'lit': 2}))\n"
            "add(g:out, string({}))\n"
            "add(g:out, string({a: {b: 1}}))\n"
            "add(g:out, string(len({a: 1, b: 2})))\n"
            "add(g:out, string({a: 1}->keys()))\n"
            "add(g:out, string({true: 1}))\n"
            "add(g:out, string({a: k}))\n");
    f.close();
    // A variable of the same name as a key is not what the key says.
    data.doCommand("let a = 'VARIABLE'");
    data.doCommand("source " + dir.path() + "/d.vim");
    const QStringList expected = {"{'a': 1}", "{'a': 1, 'b_2': 2}", "{'q': 1}", "{'lit': 2}",
                                  "{}", "{'a': {'b': 1}}", "2", "['a']", "{'true': 1}",
                                  "{'a': 'q'}"};
    for (int i = 0; i < expected.size(); ++i) {
        message.clear();
        data.doCommand(QString("echo g:out[%1]").arg(i));
        QCOMPARE(message, expected.at(i));
    }
    data.doCommand("unlet g:out | unlet a");
}

void FakeVimTester::test_vim9_call_funcref_variable()
{
    // Which of the two a bare name calls where a variable and a function go by
    // the same name: in Vim9 the variable, in legacy Vim the function. Values
    // taken from Vim 9.1.
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
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const auto write = [&dir](const QString &name, const QByteArray &text) {
        QFile f(dir.path() + '/' + name);
        [[maybe_unused]] const bool ok = f.open(QIODevice::WriteOnly);
        f.write(text);
    };
    write("l.vim", "function! Sub(a, b)\n"
                   "  return 'GLOBAL'\n"
                   "endfunction\n"
                   "function! Legacy()\n"
                   "  let Sub = function('strlen')\n"
                   "  return Sub('abc', 'x')\n"
                   "endfunction\n");
    write("r.vim", "vim9script\n"
                   "def Join(n: number, s: string): string\n"
                   "  return s .. n\n"
                   "enddef\n"
                   "def Apply(Sub: func(number, string): string, n: number): string\n"
                   "  return Sub(n, 'x')\n"
                   "enddef\n"
                   "def Piped(Sub: func(number, string): string, n: number): string\n"
                   "  return n->Sub('x')\n"
                   "enddef\n"
                   "g:called = Apply(Join, 7)\n"
                   "g:piped = Piped(Join, 7)\n");
    data.doCommand("source " + dir.path() + "/l.vim");
    QCOMPARE(value("Sub(1, 2)"), QLatin1String("GLOBAL"));
    // The parameter holding a funcref is what is called, in both spellings.
    data.doCommand("source " + dir.path() + "/r.vim");
    QCOMPARE(value("g:called"), QLatin1String("x7"));
    QCOMPARE(value("g:piped"), QLatin1String("x7"));
    // A legacy function calls the function, not the variable of that name.
    QCOMPARE(value("Legacy()"), QLatin1String("GLOBAL"));
    data.doCommand("delfunction Sub | delfunction Legacy");
    data.doCommand("unlet g:called | unlet g:piped");
}

void FakeVimTester::test_vim_script_feedkeys()
{
    // feedkeys() puts keys in the queue rather than handling them there and
    // then: the function runs to its end first, and the keys follow. "i" puts
    // them in front of what is already waiting, so the last call is handled
    // first, and they are mapped unless "n" says not to. Values taken from
    // Vim 9.1 pressing the same keys.
    TestData data;
    setup(&data);
    data.setText("one" N "two");
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
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    QFile f(dir.path() + "/f.vim");
    QVERIFY(f.open(QIODevice::WriteOnly));
    f.write("nnoremap ,3 :call add(g:log, '3')<CR>\n"
            "nnoremap ,4 :call add(g:log, '4')<CR>\n"
            "function! TwoI()\n"
            "  call feedkeys(',3', 'i')\n"
            "  call feedkeys(',4', 'i')\n"
            "  call add(g:log, 'in-function')\n"
            "endfunction\n"
            "function! Tail()\n"
            "  call feedkeys(',3', 'i')\n"
            "  call feedkeys(',4', '')\n"
            "endfunction\n"
            "function! NoRemap()\n"
            "  call feedkeys(',3', 'ni')\n"
            "endfunction\n"
            "nnoremap ,t :call TwoI()<CR>\n"
            "nnoremap ,a :call Tail()<CR>\n"
            "nnoremap ,n :call NoRemap()<CR>\n");
    f.close();
    data.doCommand("source " + dir.path() + "/f.vim");
    // The function finishes before the keys are handled, and "i" puts the last
    // of them in front.
    data.doCommand("let g:log = []");
    data.doKeys(",t");
    QCOMPARE(value("string(g:log)"), QLatin1String("['in-function', '4', '3']"));
    // Without "i" they wait behind what is already there.
    data.doCommand("let g:log = []");
    data.doKeys(",a");
    QCOMPARE(value("string(g:log)"), QLatin1String("['3', '4']"));
    // "n" hands the keys over unmapped, so the mapping does not run.
    data.doCommand("let g:log = []");
    data.doKeys(",n");
    QCOMPARE(value("string(g:log)"), QLatin1String("[]"));
    // "<Cmd>{command}<CR>" among the keys asks for the command, which is how a
    // plugin (nohlsearch.vim) has an autocommand run one.
    data.doCommand("unlet! g:x");
    data.doCommand("call feedkeys(\"\\<Cmd>let g:x = 7\\<CR>\", 'n')");
    data.doKeys("0");
    QCOMPARE(value("get(g:, 'x', 'NONE')"), QLatin1String("7"));
    data.doCommand("unlet! g:x");
    // Keys that are not a mapping do what they say.
    data.doCommand("call feedkeys('x', 'n')");
    data.doKeys("gg");
    KEYS("", X "ne" N "two");
    QCOMPARE(value("feedkeys('', 'n')"), QLatin1String("0"));
    data.doCommand("unlet g:log");
    data.doCommand("nunmap ,3 | nunmap ,4 | nunmap ,t | nunmap ,a | nunmap ,n");
    data.doCommand("delfunction TwoI | delfunction Tail | delfunction NoRemap");
}

void FakeVimTester::test_vim_plugin_repeat()
{
    // Tim Pope's repeat.vim, which lets "." repeat a plugin's own mapping: the
    // plugin hands it the mapping and it remembers b:changedtick, so that "."
    // feeds the mapping back where nothing else has changed the text since and
    // repeats the change itself where something has. Driven here with
    // ReplaceWithRegister, whose "grr" asks to be remembered that way. Values
    // taken from Vim 9.1 pressing the same keys with both plugins. Neither is
    // installed with Vim, so this is skipped without them.
    const QString P = qEnvironmentVariable("FAKEVIM_TEST_PLUGINS");
    const QString REP = P + "/vim-repeat";
    const QString RWR = P + "/vim-ReplaceWithRegister";
    if (!QFileInfo::exists(REP + "/autoload/repeat.vim")
        || !QFileInfo::exists(RWR + "/plugin/ReplaceWithRegister.vim"))
        QSKIP("repeat.vim or ReplaceWithRegister is not there;"
              " set FAKEVIM_TEST_PLUGINS to checkouts of both");
    TestData data;
    setup(&data);
    // Both plugins only register once, and another test may have had them
    // before: the mappings it took back would not come again.
    data.doCommand("unlet! g:loaded_repeat | unlet! g:loaded_ReplaceWithRegister");
    data.doCommand("set runtimepath+=" + REP);
    data.doCommand("source " + REP + "/autoload/repeat.vim");
    data.doCommand("set runtimepath+=" + RWR);
    data.doCommand("source " + RWR + "/plugin/ReplaceWithRegister.vim");

    const auto run = [&](const char *keys) {
        data.setText(X "one" N "two" N "three");
        data.doKeys(keys);
        return QString::fromUtf8(data.text()).replace(QLatin1Char('\n'), QLatin1String("/"));
    };
    const QStringList got = {
        run("yyjgrr"),
        run("yyjgrrj."),      // "." repeats the mapping, not the change
        run("yyjgrrj.."),
        run("yyjgrrjx."),     // the text changed in between, so "." is a "."
        run("yy2jgrr"),
        run("yyjgrrj2."),     // two lines wanted and one left, so nothing happens
    };
    // Everything either plugin left behind is shared with every other test.
    data.doCommand("nunmap . | nunmap u | nunmap U | nunmap <C-R>");
    data.doCommand("nunmap <Plug>(RepeatDot) | nunmap <Plug>(RepeatUndo)");
    data.doCommand("nunmap <Plug>(RepeatUndoLine) | nunmap <Plug>(RepeatRedo)");
    data.doCommand("autocmd! repeatPlugin | autocmd! repeat_custom_motion");
    data.doCommand("nunmap gr | nunmap grr | vunmap gr | set opfunc=");

    const QStringList wanted = {
        "one/one/three",
        "one/one/one",
        "one/one/one",
        "one/one/ree",
        "one/two/one",
        "one/one/three",
    };
    QCOMPARE(got, wanted);
}

void FakeVimTester::test_vim_autocmd_cursor_hold()
{
    // "CursorHold" runs where nothing has been typed for 'updatetime', and
    // "CursorHoldI" is the one for insert mode; neither runs in the other's mode.
    // Which fires where was measured in Vim 9.1 by letting the autocommand end it.
    TestData data;
    setup(&data);
    data.setText("one" N "two");
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
    data.doCommand("set updatetime=10");
    data.doCommand("autocmd CursorHold * let g:held = 'n'");
    data.doCommand("autocmd CursorHoldI * let g:held = 'i'");
    data.doCommand("unlet! g:held");
    // Standing still in normal mode.
    data.doKeys("l");
    QTRY_COMPARE(value("get(g:, 'held', '-')"), QLatin1String("n"));
    // And in insert mode, where the other one is not the one that runs.
    data.doCommand("unlet! g:held");
    data.doKeys("ix");
    QTRY_COMPARE(value("get(g:, 'held', '-')"), QLatin1String("i"));
    data.doKeys("<ESC>");
    data.doCommand("autocmd! CursorHold | autocmd! CursorHoldI");
    data.doCommand("unlet! g:held | set updatetime=4000");
}

void FakeVimTester::test_vim_plugin_nohlsearch()
{
    // Vim's own nohlsearch.vim, which takes the search highlighting away once the
    // user has stood still: it asks v:hlsearch from a "CursorHold" autocommand and
    // feeds "<Cmd>nohlsearch<CR>" where there is highlighting to take away.
    const QString plugin = "/usr/share/vim/vim91/pack/dist/opt/nohlsearch/plugin/nohlsearch.vim";
    if (!QFileInfo::exists(plugin))
        QSKIP("Vim's nohlsearch.vim is not installed");
    TestData data;
    setup(&data);
    data.setText("one two one" N "three one");
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
    data.doCommand("set hlsearch | set updatetime=10");
    data.doCommand("unlet! g:loaded_nohlsearch");
    data.doCommand("source " + plugin);
    data.doKeys("/three<CR>");
    QCOMPARE(value("v:hlsearch"), QLatin1String("1"));
    QTRY_COMPARE(value("v:hlsearch"), QLatin1String("0"));
    data.doCommand("autocmd! nohlsearch");
    data.doCommand("unlet! g:loaded_nohlsearch | set updatetime=4000");
}

void FakeVimTester::test_vim_plugin_vimindent()
{
    // Vim's own indent file for Vim scripts, which is a Vim9 script asking a
    // Vim9 module what the indent of a line is. Values taken from Vim 9.1
    // running "gg=G" over the same lines with 'sw' 4, 'ts' 8 and 'expandtab'.
    const QString indentFile = "/usr/share/vim/vim91/indent/vim.vim";
    if (!QFileInfo::exists(indentFile))
        QSKIP("Vim's indent file for Vim scripts is not installed");
    TestData data;
    setup(&data);
    data.doCommand("set shiftwidth=4 | set tabstop=8 | set expandtab");
    data.doCommand("unlet! b:did_indent");
    data.doCommand("source " + indentFile);
    data.setText(X "vim9script" N
                 "def Foo(x: number): number" N
                 "if x > 0" N
                 "for i in range(3)" N
                 "echo i" N
                 "endfor" N
                 "else" N
                 "var d = {" N
                 "a: 1," N
                 "b: 2," N
                 "}" N
                 "echo d" N
                 "endif" N
                 "return x" N
                 "enddef");
    KEYS("gg=G",
         X "vim9script" N
         "def Foo(x: number): number" N
         "    if x > 0" N
         "        for i in range(3)" N
         "            echo i" N
         "        endfor" N
         "    else" N
         "        var d = {" N
         "            a: 1," N
         "            b: 2," N
         "        }" N
         "        echo d" N
         "    endif" N
         "    return x" N
         "enddef");
    // A legacy script, where a keyword opens a block and a backslash carries a
    // command on to the next line.
    data.setText(X
                 "function! Foo(a, b) abort" N
                 "let l:x = a:a" N
                 "while l:x > 0" N
                 "if l:x == 2" N
                 "echo \"two\"" N
                 "elseif l:x == 3" N
                 "echo \"three\"" N
                 "else" N
                 "echo \"other\"" N
                 "endif" N
                 "let l:x -= 1" N
                 "endwhile" N
                 "try" N
                 "call Bar()" N
                 "catch /^Vim\\%((\\a\\+)\\)\\=:E/" N
                 "echomsg \"caught\"" N
                 "finally" N
                 "echo \"done\"" N
                 "endtry" N
                 "let l:list = [" N
                 "\\ 1," N
                 "\\ 2," N
                 "\\ ]" N
                 "let l:cmd = 'echo'" N
                 "\\ . ' more'" N
                 "augroup Test" N
                 "autocmd!" N
                 "autocmd BufWritePost * echo \"written\"" N
                 "augroup END" N
                 "return l:x" N
                 "endfunction" N
                 "let s:heredoc =<< trim END" N
                 "line one" N
                 "  line two" N
                 "END");
    KEYS("gg=G",
         X
         "function! Foo(a, b) abort" N
         "    let l:x = a:a" N
         "    while l:x > 0" N
         "        if l:x == 2" N
         "            echo \"two\"" N
         "        elseif l:x == 3" N
         "            echo \"three\"" N
         "        else" N
         "            echo \"other\"" N
         "        endif" N
         "        let l:x -= 1" N
         "    endwhile" N
         "    try" N
         "        call Bar()" N
         "    catch /^Vim\\%((\\a\\+)\\)\\=:E/" N
         "        echomsg \"caught\"" N
         "    finally" N
         "        echo \"done\"" N
         "    endtry" N
         "    let l:list = [" N
         "                \\ 1," N
         "                \\ 2," N
         "                \\ ]" N
         "    let l:cmd = 'echo'" N
         "                \\ . ' more'" N
         "    augroup Test" N
         "        autocmd!" N
         "        autocmd BufWritePost * echo \"written\"" N
         "    augroup END" N
         "    return l:x" N
         "endfunction" N
         "let s:heredoc =<< trim END" N
         "    line one" N
         "      line two" N
         "END");

    // A bracket block inside another one, which the plugin keeps on a stack.
    data.setText(X
                 "vim9script" N
                 "var d = {" N
                 "one: 1," N
                 "two: [" N
                 "2," N
                 "3," N
                 "]," N
                 "}");
    KEYS("gg=G",
         X
         "vim9script" N
         "var d = {" N
         "    one: 1," N
         "    two: [" N
         "        2," N
         "        3," N
         "    ]," N
         "}");
    data.doCommand("unlet! b:vimindent | unlet! b:did_indent");
    data.doCommand("set indentexpr= | set noautoindent");
    data.doCommand("set shiftwidth=8 | set tabstop=8 | set noexpandtab");
}

void FakeVimTester::test_vim_indent_with_expression()
{
    // "=" asks 'indentexpr' what the indent of each line is, as Vim does: v:lnum
    // names the line and a value below zero leaves the line the indent it has.
    // ":setlocal" sets it, which is how an indent file does. Values taken from
    // Vim 9.1 running "gg=G" over the same lines with the same expression.
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
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    QFile f(dir.path() + "/i.vim");
    QVERIFY(f.open(QIODevice::WriteOnly));
    f.write("function! MyIndent()\n  return (v:lnum - 1) * 2\nendfunction\n"
            "function! KeepIndent()\n  return v:lnum == 1 ? 6 : -1\nendfunction\n");
    f.close();
    data.doCommand("source " + dir.path() + "/i.vim");
    data.doCommand("set expandtab | set shiftwidth=4");
    // An indent file writes ":setlocal"; an option belongs to the session here,
    // where Vim keeps a local and a global one, so this sets the one there is.
    data.doCommand("setlocal indentexpr=MyIndent()");
    QCOMPARE(value("&indentexpr"), QLatin1String("MyIndent()"));
    const auto run = [&](const char *lines) {
        data.setText(lines);
        data.doKeys("gg=G");
        return QString::fromUtf8(data.text()).replace(QLatin1Char('\n'), QLatin1String("/"));
    };
    QCOMPARE(run(X "a" N "b" N "c" N "d"), QLatin1String("a/  b/    c/      d"));
    // Below zero the line keeps what it has.
    data.doCommand("setlocal indentexpr=KeepIndent()");
    QCOMPARE(run(X "  a" N "  b"), QLatin1String("      a/  b"));
    // Without one the editor does the indenting, as before.
    data.doCommand("set indentexpr=");
    QCOMPARE(value("&indentexpr"), QString());
    // An indent script asks what 'shiftwidth' it has to work with, which is
    // 'tabstop' where 'shiftwidth' is zero. Values taken from Vim 9.1.
    data.doCommand("set tabstop=8 | set shiftwidth=4");
    QCOMPARE(value("exists('*shiftwidth')"), QLatin1String("1"));
    QCOMPARE(value("shiftwidth()"), QLatin1String("4"));
    data.doCommand("set shiftwidth=0");
    QCOMPARE(value("shiftwidth()"), QLatin1String("8"));
    data.doCommand("set tabstop=3");
    QCOMPARE(value("shiftwidth()"), QLatin1String("3"));
    data.doCommand("set noexpandtab | set shiftwidth=8 | set tabstop=8");
    data.doCommand("delfunction! MyIndent | delfunction! KeepIndent");
}

void FakeVimTester::test_vim_script_method_call_blanks()
{
    // White space may stand before "->", and a continuation line of a Vim9 script
    // may begin with it, which is how Vim's own indent file writes a chain of
    // them. Values taken from Vim 9.1.
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
    QCOMPARE(value("get(g:, 'nope', {}) ->get('k', 7)"), QLatin1String("7"));
    QCOMPARE(value("[1, 2, 3] ->len()"), QLatin1String("3"));
    QCOMPARE(value("'ab' ->toupper() ->strlen()"), QLatin1String("2"));
    // The same over two lines, where the second begins with the arrow.
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    QFile f(dir.path() + "/m.vim");
    QVERIFY(f.open(QIODevice::WriteOnly));
    f.write("vim9script\n"
            "var v = get(g:, 'nope', {})\n"
            "      ->get('k', 9)\n"
            "g:chained = v\n");
    f.close();
    data.doCommand("source " + dir.path() + "/m.vim");
    QCOMPARE(value("g:chained"), QLatin1String("9"));
    data.doCommand("unlet g:chained");
}

void FakeVimTester::test_vim_pattern_white_space()
{
    // Vim's white space is a space or a tab, so a pattern using it does not reach
    // over the end of a line - only "\n" and the "\_x" classes may. Values taken
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
    QCOMPARE(value("matchstr(\"a\\tb\", '\\s')"), QLatin1String("\t"));
    QCOMPARE(value("matchstr('  x', '\\S')"), QLatin1String("x"));
    QCOMPARE(value("match(\"aa\\nbb\", 'aa\\s*bb')"), QLatin1String("-1"));
    data.setText("|aa" N "bb");
    QCOMPARE(value("search('aa\\s*bb', 'cnW')"), QLatin1String("0"));
    QCOMPARE(value("search('aa\\_s*bb', 'cnW')"), QLatin1String("1"));
    QCOMPARE(value("search('aa\\nbb', 'cnW')"), QLatin1String("1"));
}

void FakeVimTester::test_vim_pattern_column()
{
    // A column in a pattern says where in the line the character after it has to
    // stand, wherever in the pattern it is written, and a "." asks for the line or
    // the column the cursor is in. Values taken from Vim 9.1.
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
    // A column, counted from one, and the cursor's own place.
    data.setText("aaaa" N "bb|bb" N "cccc"); // the cursor sits at 2,3
    QCOMPARE(value("getcurpos()[1 : 2]"), QLatin1String("[2, 3]"));
    QCOMPARE(value("matchstr('var d = {', '\\%3c.')"), QLatin1String("r"));
    QCOMPARE(value("matchstr('var d = {', '\\%.c.')"), QLatin1String("r"));
    QCOMPARE(value("matchstr('abc', '\\%1c.')"), QLatin1String("a"));
    QCOMPARE(value("matchstr('abcdef', '\\%.v.')"), QLatin1String("c"));
    QCOMPARE(value("matchstr('abcdef', '\\%<.c.')"), QLatin1String("a"));
    QCOMPARE(value("matchstr('abcdef', '\\%>.c.')"), QLatin1String("d"));
    // A line of its own is nothing a string has, so a pattern asking for one
    // matches nowhere in it.
    QCOMPARE(value("matchstr('abc', '\\%.l.')"), QString());
    QCOMPARE(value("matchstr('abc', '\\%1l.')"), QString());
    // The atom holds wherever it stands, not only at the start of the pattern,
    // which is how matchit says where in the line it is looking.
    QCOMPARE(value("matchstr('if x if y', '^.*\\%6c\\%(if\\)')"),
             QLatin1String("if x if"));
    QCOMPARE(value("matchstr('if x if y', '^.*\\%7c\\%(if\\)')"), QString());
    QCOMPARE(value("matchstr('abcdef', '\\(^.*\\%<4c\\)\\zs.')"), QLatin1String("c"));
    QCOMPARE(value("matchstr('abcdef', '\\(\\%>3c.*$\\)\\@=.')"), QLatin1String("d"));
    // In the buffer, where a line is something to compare.
    QCOMPARE(value("search('\\%.c.', 'cnW')"), QLatin1String("2"));
    QCOMPARE(value("search('\\%.lb', 'cnW')"), QLatin1String("2"));
    QCOMPARE(value("search('\\%>.l.', 'cnW')"), QLatin1String("3"));
    QCOMPARE(value("search('\\%<.l.', 'cnW')"), QLatin1String("0"));
}

void FakeVimTester::test_vim_script_wanted_column()
{
    // getcurpos() also says which column the cursor wants to be in, which is
    // v:maxcol where it is to stay at the end of the line. A function moving the
    // cursor says which column it landed in; setpos() leaves the wish alone.
    // Values taken from Vim 9.1.
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
    data.setText("|aaaaaaaa" N "bbbbbbbb" N "cccc");
    data.doCommand("call cursor(2, 3)");
    QCOMPARE(value("getcurpos()"), QLatin1String("[0, 2, 3, 0, 3]"));
    QCOMPARE(value("getpos('.')"), QLatin1String("[0, 2, 3, 0]"));
    data.doKeys("$");
    QCOMPARE(value("getcurpos()"), QLatin1String("[0, 2, 8, 0, 2147483647]"));
    data.doKeys("0");
    QCOMPARE(value("getcurpos()"), QLatin1String("[0, 2, 1, 0, 1]"));
    // A vertical move keeps the column the cursor wants, even where it cannot be.
    data.doCommand("call cursor(2, 5)");
    data.doKeys("j");
    QCOMPARE(value("getcurpos()"), QLatin1String("[0, 3, 4, 0, 5]"));
    data.doCommand("call cursor(2, 3)");
    data.doCommand("call setpos('.', [0, 3, 2, 0])");
    QCOMPARE(value("getcurpos()"), QLatin1String("[0, 3, 2, 0, 3]"));

    // searchpair() moving the cursor takes the wish along; with "n" it does not.
    data.setText("|one two" N "three (four" N "five) six");
    data.doCommand("call cursor(2, 8)");
    QCOMPARE(value("searchpair('(', '', ')', 'W')"), QLatin1String("3"));
    QCOMPARE(value("getcurpos()"), QLatin1String("[0, 3, 5, 0, 5]"));
    data.doCommand("call cursor(2, 8)");
    QCOMPARE(value("searchpair('(', '', ')', 'nW')"), QLatin1String("3"));
    QCOMPARE(value("getcurpos()"), QLatin1String("[0, 2, 8, 0, 8]"));
    data.doCommand("call cursor(3, 3)");
    QCOMPARE(value("searchpair('(', '', ')', 'bW')"), QLatin1String("2"));
    QCOMPARE(value("getcurpos()"), QLatin1String("[0, 2, 7, 0, 7]"));
}

void FakeVimTester::test_vim_script_literal_key_dict()
{
    // "#{...}" is a dictionary whose keys stand for themselves, which is how a
    // legacy script writes one without quoting every key - Vim's own Python
    // indent file holds its settings that way. A key may hold letters, digits,
    // "_" and "-". Values taken from Vim 9.1.
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
    data.doCommand("let g:d = #{a: 1, b: 'x'}");
    QCOMPARE(value("string(sort(keys(g:d)))"), QLatin1String("['a', 'b']"));
    QCOMPARE(value("g:d.a .. '/' .. g:d['b']"), QLatin1String("1/x"));
    QCOMPARE(value("type(g:d)"), QLatin1String("4"));
    QCOMPARE(value("string(#{})"), QLatin1String("{}"));
    data.doCommand("let g:f = #{one-two: 1, k9: 2, _u: 3}");
    QCOMPARE(value("string(sort(keys(g:f)))"), QLatin1String("['_u', 'k9', 'one-two']"));
    QCOMPARE(value("g:f['one-two'] .. g:f.k9 .. g:f._u"), QLatin1String("123"));
    QCOMPARE(value("string(#{outer: #{inner: [1, 2]}})"),
             QLatin1String("{'outer': {'inner': [1, 2]}}"));
    // The same written over several lines, a comment among them.
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    QFile f(dir.path() + "/d.vim");
    QVERIFY(f.open(QIODevice::WriteOnly));
    f.write("let g:cont = #{\n"
            "      \\ a: 1,\n"
            "      \"\\ a comment among the lines\n"
            "      \\ b: 2,\n"
            "      \\ }\n");
    f.close();
    data.doCommand("source " + dir.path() + "/d.vim");
    QCOMPARE(value("string(g:cont)"), QLatin1String("{'a': 1, 'b': 2}"));
    data.doCommand("unlet! g:d g:f g:cont");
}

void FakeVimTester::test_vim_script_lazy_ternary()
{
    // Only the arm a ternary takes is worked out, as in Vim, which is what lets
    // "a:0 > 0 ? a:1 : 0" ask about an argument that may not be there at all.
    // Values taken from Vim 9.1.
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
    QCOMPARE(value("0 ? g:nosuchvar : 5"), QLatin1String("5"));
    QCOMPARE(value("1 ? 6 : g:nosuchvar"), QLatin1String("6"));
    QCOMPARE(value("1 ?? g:nosuchvar"), QLatin1String("1"));
    QCOMPARE(value("0 ?? 7"), QLatin1String("7"));
    // Nothing of the arm not taken happens, not even a call.
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    QFile f(dir.path() + "/t.vim");
    QVERIFY(f.open(QIODevice::WriteOnly));
    f.write("let g:seen = 0\n"
            "function! Mark()\n"
            "  let g:seen += 1\n"
            "  return 1\n"
            "endfunction\n"
            "function! Varg(a, ...)\n"
            "  return [a:0, string(a:000), a:0 > 0 ? a:1 : 'none']\n"
            "endfunction\n");
    f.close();
    data.doCommand("source " + dir.path() + "/t.vim");
    QCOMPARE(value("0 ? Mark() : 9"), QLatin1String("9"));
    QCOMPARE(value("g:seen"), QLatin1String("0"));
    // What is read past is read the same way as what is worked out: a "." after
    // a string is the concatenation there too, and nothing in it is called.
    data.doCommand("let g:F = function('strlen')");
    QCOMPARE(value("0 ? g:F('abc') : 'ok'"), QLatin1String("ok"));
    QCOMPARE(value("0 && g:F('abc')"), QLatin1String("0"));
    QCOMPARE(value("0 ? \"0\" .. substitute('a','a','b','') : 'F'"), QLatin1String("F"));
    QCOMPARE(value("0 ? \"0\".substitute('a','a','b','') : 'F'"), QLatin1String("F"));
    QCOMPARE(value("1 ? \"0\".substitute('a','a','b','') : 'F'"), QLatin1String("0b"));
    QCOMPARE(value("0 ? {'k': 'K'}.k : 'H'"), QLatin1String("H"));
    QCOMPARE(value("1 ? {'k': 'K'}.k : 'H'"), QLatin1String("K"));
    QCOMPARE(value("string(Varg(1))"), QLatin1String("[0, '[]', 'none']"));
    QCOMPARE(value("string(Varg(1, 2, 3))"), QLatin1String("[2, '[2, 3]', 2]"));
    data.doCommand("unlet! g:seen | unlet! g:F");
    data.doCommand("delfunction! Mark | delfunction! Varg");
}

void FakeVimTester::test_vim_plugin_pythonindent()
{
    // Vim's own indent file for Python, a legacy script asking an autoload
    // function what the indent of a line is. It hangs a bracket block by
    // 'shiftwidth' twice over, so what comes out is not what went in - the
    // values are what Vim 9.1 makes of the same lines with 'sw' 4, 'ts' 8 and
    // 'expandtab'.
    const QString indentFile = "/usr/share/vim/vim91/indent/python.vim";
    if (!QFileInfo::exists(indentFile))
        QSKIP("Vim's indent file for Python is not installed");
    TestData data;
    setup(&data);
    data.doCommand("set shiftwidth=4 | set tabstop=8 | set expandtab");
    data.doCommand("set runtimepath+=/usr/share/vim/vim91");
    data.doCommand("unlet! b:did_indent");
    data.doCommand("source " + indentFile);
    data.setText(X
                 "import sys" N
                 "" N
                 "" N
                 "def fib(n):" N
                 "    if n < 2:" N
                 "        return n" N
                 "    total = 0" N
                 "    for i in range(n):" N
                 "        if i % 2 == 0:" N
                 "            total += i" N
                 "        else:" N
                 "            total -= i" N
                 "    return total" N
                 "" N
                 "" N
                 "class Thing:" N
                 "    def __init__(self, name):" N
                 "        self.name = name" N
                 "        self.items = [" N
                 "            1," N
                 "            2," N
                 "        ]" N
                 "" N
                 "    def show(self):" N
                 "        print(self.name," N
                 "              self.items)" N
                 "        try:" N
                 "            value = int(self.name)" N
                 "        except ValueError:" N
                 "            value = 0" N
                 "        finally:" N
                 "            print(\"done\")" N
                 "        return value" N
                 "" N
                 "" N
                 "d = {" N
                 "    \"a\": 1," N
                 "    \"b\": 2," N
                 "}" N
                 "if len(sys.argv) > 1:" N
                 "    print(fib(int(sys.argv[1])))");
    KEYS("gg=G",
         X
         "import sys" N
         "" N
         "" N
         "def fib(n):" N
         "    if n < 2:" N
         "        return n" N
         "    total = 0" N
         "    for i in range(n):" N
         "        if i % 2 == 0:" N
         "            total += i" N
         "        else:" N
         "            total -= i" N
         "    return total" N
         "" N
         "" N
         "class Thing:" N
         "    def __init__(self, name):" N
         "        self.name = name" N
         "        self.items = [" N
         "                1," N
         "                2," N
         "                ]" N
         "" N
         "    def show(self):" N
         "        print(self.name," N
         "              self.items)" N
         "        try:" N
         "            value = int(self.name)" N
         "        except ValueError:" N
         "            value = 0" N
         "        finally:" N
         "            print(\"done\")" N
         "        return value" N
         "" N
         "" N
         "d = {" N
         "        \"a\": 1," N
         "        \"b\": 2," N
         "        }" N
         "if len(sys.argv) > 1:" N
         "    print(fib(int(sys.argv[1])))");
    data.doCommand("unlet! b:did_indent");
    data.doCommand("set indentexpr= | set noautoindent");
    data.doCommand("set shiftwidth=8 | set tabstop=8 | set noexpandtab");
}

void FakeVimTester::test_vim_pattern_lazy_multi()
{
    // A multi Vim writes as "{-n,m}" takes as few characters as will do, which
    // is the idiom Vim's own Lua indent file leans on. "{}" and "{-}" stand for
    // any number at all. Values taken from Vim 9.1.
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
    QCOMPARE(value("matchstr('<a><b>', '<.\\{-}>')"), QLatin1String("<a>"));
    QCOMPARE(value("matchstr('<a><b>', '<.*>')"), QLatin1String("<a><b>"));
    QCOMPARE(value("matchstr('<a><b>', '\\v\\<.{-}\\>')"), QLatin1String("<a>"));
    QCOMPARE(value("matchstr('xaaab', 'a\\{-}b')"), QLatin1String("aaab"));
    QCOMPARE(value("matchstr('aaab', 'a\\{-1,}b')"), QLatin1String("aaab"));
    QCOMPARE(value("matchstr('aaab', 'a\\{-2}')"), QLatin1String("aa"));
    QCOMPARE(value("matchstr('aaaa', 'a\\{-,2}b\\?')"), QString());
    QCOMPARE(value("matchstr('aaa', 'a\\{2}')"), QLatin1String("aa"));
    QCOMPARE(value("matchstr('aaa', 'a\\{1,2}')"), QLatin1String("aa"));
    // What the Lua indent file asks about a line that opens a function.
    QCOMPARE(value("match('local function fib(n)', "
                   "'\\<function\\>\\s*\\%(\\k\\|[.:]\\)\\{-}\\s*(')"),
             QLatin1String("6"));
}

void FakeVimTester::test_vim_ex_bar_in_single_quotes()
{
    // A backslash has no power in a single-quoted string, so a "|" behind one
    // still stands inside the string and does not end the command - which is how
    // Vim's own YAML indent file writes its patterns. Values taken from Vim 9.1.
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
    data.doCommand("let g:one = 'a|b' | let g:two = 2");
    QCOMPARE(value("g:one .. ' ' .. g:two"), QLatin1String("a|b 2"));
    data.doCommand("let g:three = '\\v%(\\''x\\'')|y' | let g:four = 4");
    QCOMPARE(value("g:three"), QLatin1String("\\v%(\\'x\\')|y"));
    QCOMPARE(value("strlen(g:three) .. ' ' .. g:four"), QLatin1String("12 4"));
    // A double-quoted string does give a backslash its meaning.
    data.doCommand("let g:five = \"a|b\" | let g:six = 6");
    QCOMPARE(value("g:five .. ' ' .. g:six"), QLatin1String("a|b 6"));
    data.doCommand("unlet! g:one g:two g:three g:four g:five g:six");
}

void FakeVimTester::test_vim_script_identity_case()
{
    // "is#" and "is?" say what to do with the case of a string, as "==#" and
    // "==?" do; what a list is stays the same either way. Vim's own shell indent
    // file asks "&ft is# 'zsh'". Values taken from Vim 9.1.
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
    QCOMPARE(value("'a' is# 'a'"), QLatin1String("1"));
    QCOMPARE(value("'A' is# 'a'"), QLatin1String("0"));
    QCOMPARE(value("'A' is? 'a'"), QLatin1String("1"));
    QCOMPARE(value("'a' isnot# 'b'"), QLatin1String("1"));
    QCOMPARE(value("'A' isnot? 'a'"), QLatin1String("0"));
    QCOMPARE(value("&filetype is# 'zsh'"), QLatin1String("0"));
    data.doCommand("let g:l = [1, 2] | let g:m = g:l");
    QCOMPARE(value("(g:l is g:m) .. (g:l is# g:m) .. (g:l isnot [1, 2])"),
             QLatin1String("111"));
    data.doCommand("unlet! g:l g:m");
}

void FakeVimTester::test_vim_plugin_indent_files()
{
    // Vim's own indent files for other languages, which is what a plugin written
    // in Vim script looks like in the large: patterns of every shape, funcrefs to
    // builtins, searchpair() and the option and syntax questions around them.
    // Values taken from Vim 9.1 running "gg=G" over the same lines with 'sw' 4,
    // 'ts' 8 and 'expandtab'.
    if (!QFileInfo::exists("/usr/share/vim/vim91/indent/sh.vim"))
        QSKIP("Vim's indent files are not installed");
    TestData data;
    setup(&data);
    data.doCommand("set shiftwidth=4 | set tabstop=8 | set expandtab");
    data.doCommand("set runtimepath+=/usr/share/vim/vim91");
    // shell
    data.doCommand("unlet! b:did_indent");
    data.doCommand("source /usr/share/vim/vim91/indent/sh.vim");
    data.setText(X
                 "#!/bin/sh" N
                 "foo() {" N
                 "echo one" N
                 "if [ -n \"$1\" ]; then" N
                 "echo two" N
                 "else" N
                 "echo three" N
                 "fi" N
                 "for i in 1 2 3; do" N
                 "echo \"$i\"" N
                 "done" N
                 "case \"$1\" in" N
                 "a)" N
                 "echo a" N
                 ";;" N
                 "*)" N
                 "echo other" N
                 ";;" N
                 "esac" N
                 "while read -r line; do" N
                 "echo \"$line\"" N
                 "done" N
                 "}" N
                 "foo \"x\"");
    KEYS("gg=G",
         X
         "#!/bin/sh" N
         "foo() {" N
         "    echo one" N
         "    if [ -n \"$1\" ]; then" N
         "        echo two" N
         "    else" N
         "        echo three" N
         "    fi" N
         "    for i in 1 2 3; do" N
         "        echo \"$i\"" N
         "    done" N
         "    case \"$1\" in" N
         "        a)" N
         "            echo a" N
         "            ;;" N
         "        *)" N
         "            echo other" N
         "            ;;" N
         "    esac" N
         "    while read -r line; do" N
         "        echo \"$line\"" N
         "    done" N
         "}" N
         "foo \"x\"");

    // Lua
    data.doCommand("unlet! b:did_indent");
    data.doCommand("source /usr/share/vim/vim91/indent/lua.vim");
    data.setText(X
                 "local function fib(n)" N
                 "if n < 2 then" N
                 "return n" N
                 "end" N
                 "local total = 0" N
                 "for i = 1, n do" N
                 "if i % 2 == 0 then" N
                 "total = total + i" N
                 "else" N
                 "total = total - i" N
                 "end" N
                 "end" N
                 "local t = {" N
                 "1," N
                 "2," N
                 "}" N
                 "return total, t" N
                 "end" N
                 "print(fib(10))");
    KEYS("gg=G",
         X
         "local function fib(n)" N
         "    if n < 2 then" N
         "        return n" N
         "    end" N
         "    local total = 0" N
         "    for i = 1, n do" N
         "        if i % 2 == 0 then" N
         "            total = total + i" N
         "        else" N
         "            total = total - i" N
         "        end" N
         "    end" N
         "    local t = {" N
         "        1," N
         "        2," N
         "    }" N
         "    return total, t" N
         "end" N
         "print(fib(10))");

    // CMake
    data.doCommand("unlet! b:did_indent");
    data.doCommand("source /usr/share/vim/vim91/indent/cmake.vim");
    data.setText(X
                 "project(demo)" N
                 "if(WIN32)" N
                 "add_definitions(-DWIN)" N
                 "else()" N
                 "add_definitions(-DUNIX)" N
                 "endif()" N
                 "foreach(f a b c)" N
                 "message(STATUS \"${f}\")" N
                 "endforeach()" N
                 "function(my_fun arg)" N
                 "message(STATUS \"${arg}\")" N
                 "endfunction()" N
                 "add_executable(demo" N
                 "main.cpp" N
                 "other.cpp" N
                 ")");
    KEYS("gg=G",
         X
         "project(demo)" N
         "if(WIN32)" N
         "    add_definitions(-DWIN)" N
         "else()" N
         "    add_definitions(-DUNIX)" N
         "endif()" N
         "foreach(f a b c)" N
         "    message(STATUS \"${f}\")" N
         "endforeach()" N
         "function(my_fun arg)" N
         "    message(STATUS \"${arg}\")" N
         "endfunction()" N
         "add_executable(demo" N
         "    main.cpp" N
         "    other.cpp" N
         ")");

    // YAML
    data.doCommand("unlet! b:did_indent");
    data.doCommand("source /usr/share/vim/vim91/indent/yaml.vim");
    data.setText(X
                 "top:" N
                 "key: value" N
                 "list:" N
                 "- one" N
                 "- two" N
                 "nested:" N
                 "inner: 1" N
                 "other: 2" N
                 "last: done");
    KEYS("gg=G",
         X
         "top:" N
         "    key: value" N
         "list:" N
         "    - one" N
         "- two" N
         "nested:" N
         "    inner: 1" N
         "other: 2" N
         "last: done");

    data.doCommand("unlet! b:did_indent");
    data.doCommand("set indentexpr= | set noautoindent");
    data.doCommand("set shiftwidth=8 | set tabstop=8 | set noexpandtab");
}

void FakeVimTester::test_vim_script_searchpos()
{
    // searchpos() answers where a match is, as a line and a column counted from
    // one, and [0, 0] where there is none. With "e" both it and search() point at
    // the end of the match, which for one reaching over a line end is on the next
    // line. Values taken from Vim 9.1.
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
    data.setText("|alpha beta" N "gamma alpha" N "delta");
    QCOMPARE(value("string(searchpos('alpha', 'nW'))"), QLatin1String("[2, 7]"));
    QCOMPARE(value("string(searchpos('alpha', 'cnW'))"), QLatin1String("[1, 1]"));
    QCOMPARE(value("string(searchpos('alpha', 'bnW'))"), QLatin1String("[0, 0]"));
    QCOMPARE(value("string(searchpos('zeta', 'nW'))"), QLatin1String("[0, 0]"));
    data.doCommand("call cursor(2, 1)");
    QCOMPARE(value("string(searchpos('alpha', 'nW'))"), QLatin1String("[2, 7]"));
    QCOMPARE(value("string(searchpos('alpha', 'bnW'))"), QLatin1String("[1, 1]"));
    QCOMPARE(value("string(searchpos('alpha', 'nWe'))"), QLatin1String("[2, 11]"));
    QCOMPARE(value("string(searchpos('delta', 'nW', 2))"), QLatin1String("[0, 0]"));
    // It moves the cursor where it is not told to keep it.
    data.doCommand("call cursor(1, 1)");
    QCOMPARE(value("string(searchpos('gamma', 'W'))"), QLatin1String("[2, 1]"));
    QCOMPARE(value("string(getcurpos()[1 : 2])"), QLatin1String("[2, 1]"));
    // What {skip} leaves out is not answered with.
    data.doCommand("call cursor(1, 1)");
    QCOMPARE(value("string(searchpos('alpha', 'nW', 0, 0, {-> line('.') == 2}))"),
             QLatin1String("[0, 0]"));

    // A match reaching over the end of a line, where "e" is on the line below.
    data.setText("|aa" N "bb" N "cc");
    QCOMPARE(value("string(searchpos('aa\\nbb', 'cnWe'))"), QLatin1String("[2, 2]"));
    QCOMPARE(value("search('aa\\nbb', 'cnWe')"), QLatin1String("2"));
    QCOMPARE(value("string(searchpos('aa\\nbb', 'cnW'))"), QLatin1String("[1, 1]"));
    QCOMPARE(value("search('aa\\nbb', 'cnW')"), QLatin1String("1"));
    QCOMPARE(value("exists('*searchpos')"), QLatin1String("1"));
}

void FakeVimTester::test_vim_langmap()
{
    // 'langmap' says what a typed character stands for where it is read as a
    // command, so a keyboard laid out for another language can drive Vim without
    // being switched. It holds in normal, visual and operator-pending mode,
    // register and mark names among them, but not for text in insert mode, not on
    // the command line and not for the character f, t or r waits for. Vim
    // translates before it looks a mapping up. Values taken from Vim 9.1.
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
    const char *lines = X "alpha beta gamma" N "second line" N "third line";

    // A command of its own, and the pair and the "from;to" form of the option.
    data.doCommand("set langmap=qj");
    data.setText(lines);
    KEYS("q", "alpha beta gamma" N X "second line" N "third line");
    data.doCommand("set langmap=ab;xy");
    data.setText(lines);
    KEYS("a", X "lpha beta gamma" N "second line" N "third line");

    // An operator, with the motion after it and with a count before it.
    data.doCommand("set langmap=xd");
    data.setText(lines);
    KEYS("xw", X "beta gamma" N "second line" N "third line");
    data.setText(lines);
    KEYS("2xw", X "gamma" N "second line" N "third line");

    // A count of its own is a command too where the option says so.
    data.doCommand("set langmap=2j");
    data.setText(lines);
    KEYS("2", "alpha beta gamma" N X "second line" N "third line");

    // In visual mode as well, and for an upper-case character.
    data.doCommand("set langmap=ql");
    data.setText(lines);
    KEYS("vqd", X "pha beta gamma" N "second line" N "third line");
    data.doCommand("set langmap=Jd");
    data.setText(lines);
    KEYS("Jw", X "beta gamma" N "second line" N "third line");

    // Text in insert mode stands for itself, and so does what f or r waits for.
    data.doCommand("set langmap=ab");
    data.setText(lines);
    KEYS("ia<Esc>", X "aalpha beta gamma" N "second line" N "third line");
    data.doCommand("set langmap=xz");
    data.setText(X "a z b x c");
    KEYS("0fx", "a z b " X "x c");
    data.setText(lines);
    KEYS("rx", X "xlpha beta gamma" N "second line" N "third line");

    // A register and a mark are named by what the character stands for.
    data.doCommand("set langmap=xa");
    data.setText(lines);
    data.doCommand("call setreg('a', 'ZZ')");
    KEYS("\"xP", "Z" X "Zalpha beta gamma" N "second line" N "third line");
    data.setText(lines);
    data.doCommand("call setpos(\"'a\", [0, 3, 1, 0])");
    KEYS("'x", "alpha beta gamma" N "second line" N X "third line");

    // The character as typed is translated first, so a mapping of it never runs
    // while one of what it stands for does.
    data.doCommand("set langmap=xD");
    data.doCommand("nnoremap x ihello<Esc>");
    data.setText(lines);
    KEYS("x", X "" N "second line" N "third line");
    data.doCommand("nunmap x | nnoremap D ithere<Esc>");
    data.setText(lines);
    KEYS("x", "ther" X "ealpha beta gamma" N "second line" N "third line");
    data.doCommand("nunmap D");

    // The register CTRL-R asks for in insert mode is a name too.
    data.doCommand("set langmap=xa");
    data.doCommand("call setreg('a', 'ZZ') | call setreg('x', 'XX')");
    data.setText(lines);
    KEYS("i<C-r>x<Esc>", "Z" X "Zalpha beta gamma" N "second line" N "third line");

    // What a mapping holds up is left alone, unless 'langremap' asks for it.
    data.doCommand("set langmap=xD | set nolangremap");
    data.doCommand("nnoremap Q x");
    data.setText(lines);
    KEYS("Q", X "lpha beta gamma" N "second line" N "third line");
    data.doCommand("set langremap");
    data.setText(lines);
    KEYS("Q", X "" N "second line" N "third line");
    data.doCommand("set nolangremap | nunmap Q");

    // A register is run the way a mapping is, but what it holds is translated
    // either way - and what it holds is what was typed, before any translation.
    data.setText(lines);
    KEYS("qqxqj@q", "" N X "" N "third line");
    QCOMPARE(value("getreg('q')"), QLatin1String("x"));
    data.doCommand("call setreg('q', '')");

    // The command line is not translated at all.
    data.doCommand("set langmap=ab");
    QCOMPARE(value("'a'"), QLatin1String("a"));
    data.doCommand("set langmap=");
}

void FakeVimTester::test_vim_replace_mode_register()
{
    // CTRL-R in replace mode writes what the register holds over what stands
    // there, as typing does: it reaches past the end of a line, and a backspace
    // afterwards puts back what was written over. Values taken from Vim 9.1.
    TestData data;
    setup(&data);
    data.doCommand("call setreg('a', 'ZZ')");
    data.setText(X "alpha beta gamma");
    KEYS("R<C-r>a<Esc>", "Z" X "Zpha beta gamma");
    // In the middle of a line, and with typing carrying on after it.
    data.setText(X "alpha beta gamma");
    KEYS("lllR<C-r>a<Esc>", "alpZ" X "Z beta gamma");
    data.setText(X "alpha beta gamma");
    KEYS("R<C-r>aQ<Esc>", "ZZ" X "Qha beta gamma");
    // Past the end of the line the register only makes it longer.
    data.doCommand("call setreg('a', 'ZZZZZZZZZZZZZZZZZZZZ')");
    data.setText(X "alpha beta gamma");
    KEYS("R<C-r>a<Esc>", "ZZZZZZZZZZZZZZZZZZZ" X "Z");
    // A backspace puts back what the register was written over.
    data.doCommand("call setreg('a', 'ZZ')");
    data.setText(X "alpha beta gamma");
    KEYS("R<C-r>a<BS><BS><Esc>", X "alpha beta gamma");
    // Escape while it waits for the register name leaves replace mode alone.
    data.setText(X "alpha beta gamma");
    KEYS("R<C-r><Esc>Q<Esc>", X "Qlpha beta gamma");
}

void FakeVimTester::test_vim_iskeyword()
{
    // What 'iskeyword' says a word is made of, which is what "w" and its kin ask
    // about: "@" stands for every letter there is, the "@" of its own is written
    // "@-@", a "^" takes a part away again and a part may be a number or a range.
    // A filetype plugin sets this, so it has to hold as soon as it is written.
    // Values taken from Vim 9.1.
    TestData data;
    setup(&data);
    const char *keyword = "set iskeyword=@,48-57,_,192-255";
    data.doCommand(keyword);
    data.setText(X "foo@bar baz");
    KEYS("w", "foo" X "@bar baz");
    data.setText(X "foo@bar baz");
    KEYS("dw", X "@bar baz");
    data.setText(X "foo@bar baz");
    KEYS("diw", X "@bar baz");

    // With the "@" itself among them the whole of "foo@bar" is one word.
    data.doCommand("set iskeyword=@,48-57,_,192-255,@-@");
    data.setText(X "foo@bar baz");
    KEYS("w", "foo@bar " X "baz");
    data.setText(X "foo@bar baz");
    KEYS("dw", X "baz");

    // A number names a character, here the "-".
    data.doCommand("set iskeyword=@,45");
    data.setText(X "a-b c");
    KEYS("dw", X "c");

    // A "^" takes a part away: without "a", "cat" is three words.
    data.doCommand("set iskeyword=@,^a");
    data.setText(X "cat dog");
    KEYS("dw", X "at dog");

    // A range of characters of its own.
    data.doCommand("set iskeyword=a-c");
    data.setText(X "cat dog");
    KEYS("dw", X "t dog");
    data.doCommand(keyword);
}

void FakeVimTester::test_vim_cword()
{
    // "<cword>" is the word under the cursor or, where there is none, the first
    // one after it in the line, and what a word is made of is what 'iskeyword'
    // says. "<cWORD>" is delimited by blanks instead. Values taken from Vim 9.1.
    TestData data;
    setup(&data);
    QString message;
    data.handler->commandBufferChanged.set(
        [&](const QString &msg, int, int, int) {
            if (!msg.startsWith("--"))
                message = msg;
        });
    const auto words = [&](const QString &where) {
        data.doCommand("call cursor(" + where + ")");
        message.clear();
        data.doCommand("echo '[' .. expand('<cword>') .. '][' .. expand('<cWORD>') .. ']'");
        return message;
    };
    data.doCommand("set iskeyword=@,48-57,_,192-255");
    data.setText(X "foo@bar baz");
    QCOMPARE(words("1, 1"), QLatin1String("[foo][foo@bar]"));
    // On the "@" there is no keyword, so the one after it answers.
    QCOMPARE(words("1, 4"), QLatin1String("[bar][foo@bar]"));
    // Nor is there one after it: then what stands there answers.
    data.setText(X "foo@@@");
    QCOMPARE(words("1, 6"), QLatin1String("[@@@][foo@@@]"));
    data.setText(X "@@@");
    QCOMPARE(words("1, 1"), QLatin1String("[@@@][@@@]"));
    // A blank under the cursor is read past, in both of them.
    data.setText(X "a  b");
    QCOMPARE(words("1, 2"), QLatin1String("[b][b]"));
    // With the "@" among the keyword characters the whole of it is one word.
    data.doCommand("set iskeyword=@,48-57,_,192-255,@-@");
    data.setText(X "foo@bar baz");
    QCOMPARE(words("1, 4"), QLatin1String("[foo@bar][foo@bar]"));
    data.doCommand("set iskeyword=@,45");
    data.setText(X "a-b c");
    QCOMPARE(words("1, 2"), QLatin1String("[a-b][a-b]"));
    data.doCommand("set iskeyword=@,48-57,_,192-255");
}

void FakeVimTester::test_vim_join_comment_leader()
{
    // "j" among 'formatoptions' says that joining lines takes the comment leader
    // off the line that comes up, and 'comments' says what a leader is: a list of
    // "{flags}:{leader}" parts, where a "b" among the flags wants a blank behind
    // the leader for it to count. Values taken from Vim 9.1.
    TestData data;
    setup(&data);
    data.doCommand("set comments=:///,://");

    // Without the flag the leader stays where it is.
    data.doCommand("set formatoptions=croql");
    data.setText(X "// one" N "// two");
    KEYS("J", "// one" X " // two");
    data.doCommand("set formatoptions=croqlj");
    data.setText(X "// one" N "// two");
    KEYS("J", "// one" X " two");
    // The blanks in front of the leader go as well, and a count joins more.
    data.setText(X "// one" N "   // indented");
    KEYS("J", "// one" X " indented");
    data.setText(X "// one" N "// two" N "// three");
    KEYS("3J", "// one two" X " three");
    // The line joined to has to hold a comment itself.
    data.setText(X "plain" N "// two");
    KEYS("J", "plain" X " // two");
    // A comment after code is one too, which is the example the option documents.
    data.setText(X "int i;   // index" N "         // in list");
    KEYS("J", "int i;   // index" X " in list");

    // Whatever 'comments' says is a leader, here a hash and a Lua comment.
    data.doCommand("set comments=b:#");
    data.setText(X "# one" N "# two");
    KEYS("J", "# one" X " two");
    // With "b" a blank has to follow the leader, so this one is no leader at all.
    data.setText(X "#no blank" N "#nor here");
    KEYS("J", "#no blank" X " #nor here");
    data.doCommand("set comments=:#");
    data.setText(X "#no blank" N "#nor here");
    KEYS("J", "#no blank" X " nor here");
    data.doCommand("set comments=:---,:--");
    data.setText(X "-- one" N "-- two");
    KEYS("J", "-- one" X " two");
    // The three-piece form, where the middle of a block comment is the leader.
    data.doCommand("set comments=sO:* -,mO:*  ,exO:*/,s1:/*,mb:*,ex:*/,:///,://");
    data.setText(X "/* one" N " * two" N " */");
    KEYS("J", "/* one" X " two" N " */");
    data.doCommand("set formatoptions= | set comments=s1:/*,mb:*,ex:*/,://,b:#,:%,:XCOMM,n:>,fb:-");
}

void FakeVimTester::test_vim_cfile()
{
    // "<cfile>" is the file name under the cursor or, where there is none, the
    // first one after it in the line, and what may stand in a name is what
    // 'isfname' says. Values taken from Vim 9.1.
    TestData data;
    setup(&data);
    QString message;
    data.handler->commandBufferChanged.set(
        [&](const QString &msg, int, int, int) {
            if (!msg.startsWith("--"))
                message = msg;
        });
    const auto name = [&](const QString &where) {
        data.doCommand("call cursor(" + where + ")");
        message.clear();
        data.doCommand("echo '[' .. expand('<cfile>') .. ']'");
        return message;
    };
    data.doCommand("set isfname=@,48-57,/,.,-,_,+,,,#,$,%,~,=");
    // A name in quotes, the quotes not being part of one.
    data.setText(X "#include \"foo/bar.h\"");
    QCOMPARE(name("1, 12"), QLatin1String("[foo/bar.h]"));
    data.setText(X "see /usr/share/vim/vimrc for it");
    QCOMPARE(name("1, 6"), QLatin1String("[/usr/share/vim/vimrc]"));
    data.setText(X "a file.txt here");
    QCOMPARE(name("1, 3"), QLatin1String("[file.txt]"));
    // On something that may not stand in a name, the one after it answers.
    data.setText(X "no name here (x)");
    QCOMPARE(name("1, 14"), QLatin1String("[x]"));
    data.setText(X "spaces  before");
    QCOMPARE(name("1, 7"), QLatin1String("[before]"));
    // What the option leaves out ends the name: here the slash.
    data.doCommand("set isfname=@,48-57,_");
    data.setText(X "no/slash/here");
    QCOMPARE(name("1, 4"), QLatin1String("[slash]"));
    data.doCommand("set isfname=@,48-57,/,.,-,_,+,,,#,$,%,~,=");
}

void FakeVimTester::test_vim_goto_file()
{
    // "gf" opens the file named under the cursor, looked for beside the file being
    // edited and with each ending 'suffixesadd' names tried as well; "gF" goes to
    // the line the number behind the name says. Values taken from Vim 9.1 opening
    // the same names in the same layout.
    TestData data;
    setup(&data);
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    QVERIFY(QDir(dir.path()).mkpath("sub"));
    const auto write = [&](const QString &name, const QString &text) {
        QFile f(dir.path() + '/' + name);
        QVERIFY(f.open(QIODevice::WriteOnly));
        f.write(text.toUtf8());
    };
    write("sub/inc.h", "// included\n");
    write("other.txt", "other\n");
    write("mod.py", "module\n");
    data.handler->setCurrentFileName(dir.path() + "/main.c");
    data.doCommand("set path=.,,");

    QString opened;
    int openedLine = -1;
    data.handler->fileOpenRequested.set([&](const QString &fileName, int line) {
        opened = fileName;
        openedLine = line;
    });
    QString message;
    data.handler->commandBufferChanged.set(
        [&](const QString &msg, int, int, int) {
            if (!msg.startsWith("--"))
                message = msg;
        });
    const auto go = [&](const char *keys) {
        opened.clear();
        openedLine = -1;
        message.clear();
        data.doKeys(keys);
        return opened.isEmpty() ? message : opened + '@' + QString::number(openedLine);
    };

    // A name in quotes, found beside the file being edited.
    data.setText(X "#include \"sub/inc.h\"");
    QCOMPARE(go("11lgf"), dir.path() + "/sub/inc.h@0");
    // An ending from 'suffixesadd' where the name alone is no file.
    data.doCommand("set suffixesadd=.txt");
    data.setText(X "see other here");
    QCOMPARE(go("4lgf"), dir.path() + "/other.txt@0");
    data.doCommand("set suffixesadd=.py");
    data.setText(X "look at mod");
    QCOMPARE(go("8lgf"), dir.path() + "/mod.py@0");
    // No such file, which Vim says this way.
    data.doCommand("set suffixesadd=");
    data.setText(X "and missing.h");
    QCOMPARE(go("4lgf"), QLatin1String("E447: Can't find file \"missing.h\" in path"));
    // Where to look is what 'path' says: a "." is the directory of the file being
    // edited, and the parts are tried in the order they stand there.
    data.doCommand("set suffixesadd=");
    QVERIFY(QDir(dir.path()).mkpath("there"));
    QVERIFY(QDir(dir.path()).mkpath("A"));
    QVERIFY(QDir(dir.path()).mkpath("B"));
    write("there/target.h", "in there\n");
    write("A/x.txt", "A has it\n");
    write("B/x", "B has it\n");
    data.setText(X "look at target.h");
    data.doCommand("set path=.");
    QCOMPARE(go("8lgf"), QLatin1String("E447: Can't find file \"target.h\" in path"));
    data.doCommand("set path=.," + dir.path() + "/there");
    QCOMPARE(go("8lgf"), dir.path() + "/there/target.h@0");
    // The first part that holds the name wins, the endings being tried inside it.
    data.doCommand("set suffixesadd=.txt");
    data.setText(X "name x here");
    data.doCommand("set path=" + dir.path() + "/A," + dir.path() + "/B");
    QCOMPARE(go("5lgf"), dir.path() + "/A/x.txt@0");
    data.setText(X "name x here");
    data.doCommand("set path=" + dir.path() + "/B," + dir.path() + "/A");
    QCOMPARE(go("5lgf"), dir.path() + "/B/x@0");
    data.doCommand("set path=.,, | set suffixesadd=");

    // "gF" reads the number behind the name as the line to go to, where exactly
    // one character stands between the two - a quote and a colon are two, so
    // there is no number to read there.
    data.setText(X "sub/inc.h:7");
    QCOMPARE(go("3lgF"), dir.path() + "/sub/inc.h@7");
    data.setText(X "sub/inc.h 9");
    QCOMPARE(go("3lgF"), dir.path() + "/sub/inc.h@9");
    data.setText(X "#include \"sub/inc.h\":42");
    QCOMPARE(go("11lgF"), dir.path() + "/sub/inc.h@0");
    data.handler->fileOpenRequested.set([](const QString &, int) {});
}

void FakeVimTester::test_vim_script_findfile()
{
    // findfile() and finddir() look for a name where 'path' says, or where the
    // list handed to them says, and answer with the first place it stands in - the
    // count'th where one is given, every one of them as a list where the count is
    // below zero, and nothing at all where it stands nowhere. Values taken from
    // Vim 9.1 over the same layout.
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
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    QVERIFY(QDir(dir.path()).mkpath("A/deep"));
    QVERIFY(QDir(dir.path()).mkpath("B"));
    const auto write = [&](const QString &name) {
        QFile f(dir.path() + '/' + name);
        QVERIFY(f.open(QIODevice::WriteOnly));
        f.write("x\n");
    };
    write("A/dup.h");
    write("B/dup.h");
    data.handler->setCurrentFileName(dir.path() + "/main.c");
    const QString a = dir.path() + "/A";
    const QString b = dir.path() + "/B";
    data.doCommand("set path=.," + a + "," + b);

    QCOMPARE(value("findfile('dup.h')"), a + "/dup.h");
    QCOMPARE(value("findfile('dup.h', '', 2)"), b + "/dup.h");
    QCOMPARE(value("string(findfile('dup.h', '', -1))"),
             "['" + a + "/dup.h', '" + b + "/dup.h']");
    QCOMPARE(value("findfile('dup.h', '" + b + "')"), b + "/dup.h");
    QCOMPARE(value("findfile('nosuch.h')"), QString());
    QCOMPARE(value("findfile('" + a + "/dup.h')"), a + "/dup.h");
    // finddir() answers about directories the same way, and about a file not at all.
    QCOMPARE(value("finddir('deep')"), a + "/deep");
    QCOMPARE(value("finddir('dup.h')"), QString());
    QCOMPARE(value("finddir('nodir')"), QString());
    QCOMPARE(value("exists('*findfile') .. exists('*finddir')"), QLatin1String("11"));
    data.doCommand("set path=.,/usr/include,,");
}

void FakeVimTester::test_vim_script_line2byte()
{
    // line2byte() answers where a line starts, counted in bytes from one with
    // every line ending counting as one; byte2line() which line a byte stands in.
    // One line past the last answers with the size of the whole text and one
    // more, and anything outside answers -1. Values taken from Vim 9.1.
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
    data.setText(X "abc" N "de" N "f");
    QCOMPARE(value("line2byte(0)"), QLatin1String("-1"));
    QCOMPARE(value("line2byte(1)"), QLatin1String("1"));
    QCOMPARE(value("line2byte(2)"), QLatin1String("5"));
    QCOMPARE(value("line2byte(3)"), QLatin1String("8"));
    QCOMPARE(value("line2byte(4)"), QLatin1String("10"));
    QCOMPARE(value("line2byte(5)"), QLatin1String("-1"));
    QCOMPARE(value("byte2line(0)"), QLatin1String("-1"));
    QCOMPARE(value("byte2line(1)"), QLatin1String("1"));
    // The line ending belongs to the line it ends.
    QCOMPARE(value("byte2line(4)"), QLatin1String("1"));
    QCOMPARE(value("byte2line(5)"), QLatin1String("2"));
    QCOMPARE(value("byte2line(7)"), QLatin1String("2"));
    QCOMPARE(value("byte2line(8)"), QLatin1String("3"));
    QCOMPARE(value("byte2line(10)"), QLatin1String("-1"));
    QCOMPARE(value("byte2line(100)"), QLatin1String("-1"));
    // Bytes, not characters: the "a-umlaut" typed here takes two of them. The
    // text has to be typed rather than set, the harness reading what is set as
    // Latin-1.
    data.setText(X "b");
    data.doKeys(QString::fromUtf8("Oa\xc3\xa4\x1b"));
    QCOMPARE(value("getline(1)->strlen()"), QLatin1String("2"));
    QCOMPARE(value("line2byte(2)"), QLatin1String("5"));
    QCOMPARE(value("byte2line(4)"), QLatin1String("1"));
    QCOMPARE(value("exists('*line2byte') .. exists('*byte2line')"), QLatin1String("11"));
}

void FakeVimTester::test_vim_script_simplify()
{
    // simplify() takes out the parts of a path that say nothing: a "." of its own
    // goes unless it stands first, a ".." takes the part in front of it with it,
    // and two slashes side by side become one. What stands first and a slash at
    // the end are kept, and nothing reaches above the root. Values taken from
    // Vim 9.1.
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
    QCOMPARE(value("simplify('./foo/bar')"), QLatin1String("./foo/bar"));
    QCOMPARE(value("simplify('foo/../bar')"), QLatin1String("bar"));
    QCOMPARE(value("simplify('foo/./bar')"), QLatin1String("foo/bar"));
    QCOMPARE(value("simplify('/a/b/../c')"), QLatin1String("/a/c"));
    QCOMPARE(value("simplify('a//b')"), QLatin1String("a/b"));
    QCOMPARE(value("simplify('.//a')"), QLatin1String("./a"));
    QCOMPARE(value("simplify('../a')"), QLatin1String("../a"));
    QCOMPARE(value("simplify('a/b/../../c')"), QLatin1String("c"));
    QCOMPARE(value("simplify('a/..')"), QLatin1String("."));
    QCOMPARE(value("simplify('./')"), QLatin1String("./"));
    QCOMPARE(value("simplify('.')"), QLatin1String("."));
    QCOMPARE(value("simplify('..')"), QLatin1String(".."));
    QCOMPARE(value("simplify('/..')"), QLatin1String("/"));
    QCOMPARE(value("simplify('a/b/')"), QLatin1String("a/b/"));
    QCOMPARE(value("simplify('')"), QLatin1String(""));
    QCOMPARE(value("simplify('a/./../b')"), QLatin1String("b"));
    QCOMPARE(value("exists('*simplify')"), QLatin1String("1"));
}

void FakeVimTester::test_vim_script_file_functions()
{
    // What Vim's own gzip, tar and zip plugins do to files: delete(), rename(),
    // mkdir() and tempname(). Zero says a file is gone or moved and -1 that it is
    // not; mkdir() answers the other way round, one saying the directory is there
    // now. Values taken from Vim 9.1.
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
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString d = dir.path();
    data.doCommand("let g:d = '" + d + "'");
    data.doCommand("call writefile(['x'], g:d .. '/a.txt')");

    QCOMPARE(value("delete(g:d .. '/a.txt')"), QLatin1String("0"));
    QVERIFY(!QFileInfo::exists(d + "/a.txt"));
    QCOMPARE(value("delete(g:d .. '/nosuch')"), QLatin1String("-1"));

    QCOMPARE(value("mkdir(g:d .. '/one')"), QLatin1String("1"));
    QVERIFY(QFileInfo(d + "/one").isDir());
    // One that is there already counts as made only where "p" asks for it.
    QCOMPARE(value("mkdir(g:d .. '/one')"), QLatin1String("0"));
    QCOMPARE(value("mkdir(g:d .. '/one', 'p')"), QLatin1String("1"));
    QCOMPARE(value("mkdir(g:d .. '/two/three', 'p')"), QLatin1String("1"));
    QVERIFY(QFileInfo(d + "/two/three").isDir());
    // Without "p" nothing above the directory is made, so this one is not either.
    QCOMPARE(value("mkdir(g:d .. '/four/five')"), QLatin1String("0"));

    // A directory goes only where "d" says so, and one holding something only
    // where "r" does.
    QCOMPARE(value("delete(g:d .. '/one', 'd')"), QLatin1String("0"));
    QCOMPARE(value("delete(g:d .. '/two', 'd')"), QLatin1String("-1"));
    QCOMPARE(value("delete(g:d .. '/two', 'rf')"), QLatin1String("0"));
    QVERIFY(!QFileInfo::exists(d + "/two"));

    data.doCommand("call writefile(['y'], g:d .. '/from.txt')");
    QCOMPARE(value("rename(g:d .. '/from.txt', g:d .. '/to.txt')"), QLatin1String("0"));
    QVERIFY(QFileInfo::exists(d + "/to.txt"));
    QCOMPARE(value("rename(g:d .. '/nosuch', g:d .. '/x')"), QLatin1String("-1"));

    // tempname() answers with a name no file goes by yet, a different one each
    // time it is asked.
    data.doCommand("let g:t = tempname()");
    QCOMPARE(value("(g:t[0] == '/') .. filereadable(g:t) .. (g:t != tempname())"),
             QLatin1String("101"));
    data.doCommand("unlet! g:d g:t");
}

void FakeVimTester::test_vim_script_append()
{
    // append({lnum}, {text}) puts the text in behind that line, a zero putting it
    // in front of the first, a list one line each. Zero says it went in and one
    // that the line was no line at all - the other way round from most of the
    // answers. Values taken from Vim 9.1.
    TestData data;
    setup(&data);
    QString message;
    data.handler->commandBufferChanged.set(
        [&](const QString &msg, int, int, int) {
            if (!msg.startsWith("--"))
                message = msg;
        });
    const auto call = [&](const QString &expr) {
        message.clear();
        data.doCommand("echo " + expr);
        return message;
    };
    data.setText("a" N X "b");
    QCOMPARE(call("append(1, 'x')"), QLatin1String("0"));
    QCOMPARE(QString::fromUtf8(data.text()), QString("a\nx\nb"));
    // The line the cursor was in is a line further down now, as in Vim.
    QCOMPARE(call("line('.')"), QLatin1String("3"));
    data.setText(X "a" N "b");
    QCOMPARE(call("append(0, 'y')"), QLatin1String("0"));
    QCOMPARE(QString::fromUtf8(data.text()), QString("y\na\nb"));
    data.setText(X "a" N "b");
    QCOMPARE(call("append(2, ['p', 'q'])"), QLatin1String("0"));
    QCOMPARE(QString::fromUtf8(data.text()), QString("a\nb\np\nq"));
    // A line that is no line leaves the text alone, and says so with a one.
    data.setText(X "a" N "b");
    QCOMPARE(call("append(99, 'z')"), QLatin1String("1"));
    QCOMPARE(QString::fromUtf8(data.text()), QString("a\nb"));
    // The last line may be named "$", and a number goes in as its text.
    data.setText(X "a" N "b");
    QCOMPARE(call("append('$', 'w')"), QLatin1String("0"));
    QCOMPARE(QString::fromUtf8(data.text()), QString("a\nb\nw"));
    data.setText(X "a" N "b");
    QCOMPARE(call("append(1, 42)"), QLatin1String("0"));
    QCOMPARE(QString::fromUtf8(data.text()), QString("a\n42\nb"));
    // A list of nothing puts nothing in, and says it went in all the same.
    data.setText(X "a");
    QCOMPARE(call("append(1, [])"), QLatin1String("0"));
    QCOMPARE(QString::fromUtf8(data.text()), QString("a"));
}

void FakeVimTester::test_vim_script_json()
{
    // JSON as Vim reads and writes it: an object is a dictionary, an array a list,
    // and "true", "false" and "null" are v:true, v:false and v:null. Values taken
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
    QCOMPARE(value("string(json_decode('{\"a\": 1, \"b\": [1, 2]}'))"),
             QLatin1String("{'a': 1, 'b': [1, 2]}"));
    QCOMPARE(value("string(json_decode('[1, \"x\", true, false, null]'))"),
             QLatin1String("[1, 'x', v:true, v:false, v:null]"));
    QCOMPARE(value("string(json_decode('42'))"), QLatin1String("42"));
    QCOMPARE(value("string(json_decode('\"str\"'))"), QLatin1String("'str'"));
    QCOMPARE(value("string(json_decode('3.5'))"), QLatin1String("3.5"));
    QCOMPARE(value("string(json_decode('{}'))"), QLatin1String("{}"));
    QCOMPARE(value("string(json_decode('[]'))"), QLatin1String("[]"));
    QCOMPARE(value("string(json_decode('true'))"), QLatin1String("v:true"));
    QCOMPARE(value("string(json_decode('null'))"), QLatin1String("v:null"));

    QCOMPARE(value("json_encode({'a': 1, 'b': [1, 2]})"), QLatin1String("{\"a\":1,\"b\":[1,2]}"));
    QCOMPARE(value("json_encode([1, 'x', v:true, v:false, v:null])"),
             QLatin1String("[1,\"x\",true,false,null]"));
    QCOMPARE(value("json_encode(42)"), QLatin1String("42"));
    QCOMPARE(value("json_encode('str')"), QLatin1String("\"str\""));
    QCOMPARE(value("json_encode(3.5)"), QLatin1String("3.5"));
    // What comes out of one goes back into the other unchanged.
    QCOMPARE(value("json_encode(json_decode('{\"k\": [1, {\"n\": null}]}'))"),
             QLatin1String("{\"k\":[1,{\"n\":null}]}"));
}

void FakeVimTester::test_vim_script_glob()
{
    // glob() answers with the names a pattern stands for, one per line, or as a
    // list where the third argument says so; globpath() does that in every part of
    // a path in turn, in the order they stand there. Values taken from Vim 9.1
    // over the same layout.
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
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString d = dir.path();
    QVERIFY(QDir(d).mkpath("sub"));
    QVERIFY(QDir(d).mkpath("other"));
    for (const QString &name : QStringList{"a.txt", "b.txt", "c.vim", "sub/d.txt",
                                          "other/e.txt"}) {
        QFile f(d + '/' + name);
        QVERIFY(f.open(QIODevice::WriteOnly));
        f.write("x\n");
    }
    data.doCommand("let g:d = '" + d + "'");

    QCOMPARE(value("glob(g:d .. '/*.txt')"), d + "/a.txt\n" + d + "/b.txt");
    QCOMPARE(value("string(glob(g:d .. '/*.txt', 0, 1))"),
             "['" + d + "/a.txt', '" + d + "/b.txt']");
    QCOMPARE(value("glob(g:d .. '/?.vim')"), d + "/c.vim");
    QCOMPARE(value("string(glob(g:d .. '/[ab].txt', 0, 1))"),
             "['" + d + "/a.txt', '" + d + "/b.txt']");
    // Nothing standing for it answers with nothing, as a string or as a list.
    QCOMPARE(value("glob(g:d .. '/nosuch*')"), QString());
    QCOMPARE(value("string(glob(g:d .. '/nosuch*', 0, 1))"), QLatin1String("[]"));
    // Directories are among the names, and they come in the order of their names.
    QCOMPARE(value("string(glob(g:d .. '/*', 0, 1))"),
             "['" + d + "/a.txt', '" + d + "/b.txt', '" + d + "/c.vim', '"
                 + d + "/other', '" + d + "/sub']");
    // globpath() walks the parts of the path in turn.
    QCOMPARE(value("string(globpath(g:d .. '/sub,' .. g:d .. '/other', '*.txt', 0, 1))"),
             "['" + d + "/sub/d.txt', '" + d + "/other/e.txt']");
    QCOMPARE(value("globpath(g:d, 'nosuch')"), QString());
    QCOMPARE(value("exists('*glob') .. exists('*globpath')"), QLatin1String("11"));
    data.doCommand("unlet! g:d");
}

void FakeVimTester::test_vim_script_bufname()
{
    // bufname() answers with the name of a buffer, shortened against the directory
    // the editor was started in. Nothing, "", "%", a zero and its own number all
    // ask about the buffer the handler is in; a number no buffer goes by answers
    // with nothing. Values taken from Vim 9.1.
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
    data.handler->setCurrentFileName("/tmp/there/y.c");
    QCOMPARE(value("bufname()"), QLatin1String("/tmp/there/y.c"));
    QCOMPARE(value("bufname('%')"), QLatin1String("/tmp/there/y.c"));
    QCOMPARE(value("bufname('')"), QLatin1String("/tmp/there/y.c"));
    QCOMPARE(value("bufname(0)"), QLatin1String("/tmp/there/y.c"));
    QCOMPARE(value("bufname(bufnr('%'))"), QLatin1String("/tmp/there/y.c"));
    QCOMPARE(value("bufname(999)"), QString());
    // A buffer with no name answers with nothing.
    data.handler->setCurrentFileName(QString());
    QCOMPARE(value("bufname()"), QString());
    QCOMPARE(value("exists('*bufname')"), QLatin1String("1"));
}

void FakeVimTester::test_vim_reflow_comment()
{
    // "gq" inside a comment keeps the leader on every line it makes, 'comments'
    // saying what the leader is. The text is what Vim 9.1 makes of the same lines
    // with 'textwidth' 20; where the cursor is left afterwards is another matter,
    // which this does not ask about.
    TestData data;
    setup(&data);
    const auto reflow = [&](const char *text, const char *keys) {
        data.setText(text);
        data.doKeys(keys);
        return QString::fromUtf8(data.text());
    };
    data.doCommand("set textwidth=20 | set comments=:///,://");
    QCOMPARE(reflow(X "// aaa bbb ccc ddd eee fff ggg hhh", "gqq"),
             QString("// aaa bbb ccc ddd\n// eee fff ggg hhh"));
    // Lines already wrapped are joined and wrapped again.
    QCOMPARE(reflow(X "// aaa bbb ccc ddd" N "// eee fff ggg hhh", "gqj"),
             QString("// aaa bbb ccc ddd\n// eee fff ggg hhh"));
    // The indent in front of the leader comes along.
    QCOMPARE(reflow(X "  // indented aaa bbb ccc ddd eee", "gqq"),
             QString("  // indented aaa\n  // bbb ccc ddd eee"));
    // The middle piece of a block comment is a leader too.
    data.doCommand("set comments=s1:/*,mb:*,ex:*/,://");
    QCOMPARE(reflow(X " * aaa bbb ccc ddd eee fff ggg", "gqq"),
             QString(" * aaa bbb ccc ddd\n * eee fff ggg"));
    data.doCommand("set comments=b:#");
    QCOMPARE(reflow(X "# aaa bbb ccc ddd eee fff ggg hhh", "gqq"),
             QString("# aaa bbb ccc ddd\n# eee fff ggg hhh"));
    // A paragraph that is no comment is wrapped as before.
    data.doCommand("set comments=:///,://");
    QCOMPARE(reflow(X "aaa bbb ccc ddd eee fff ggg hhh", "gqq"),
             QString("aaa bbb ccc ddd eee\nfff ggg hhh"));
    data.doCommand("set textwidth=0 | set comments=s1:/*,mb:*,ex:*/,://,b:#,:%,:XCOMM,n:>,fb:-");
}

void FakeVimTester::test_vim_script_winsaveview()
{
    // winsaveview() hands back where the window is looking, winrestview() puts it
    // back - which is how a plugin leaves the cursor where it found it. The column
    // of a view is counted from zero where col() counts from one, and a view that
    // names only some of its parts leaves the rest alone. Values taken from Vim 9.1.
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
    data.setText(X "line one" N "line two" N "line three" N "line four" N "line five");
    data.doCommand("call cursor(2, 4)");
    QCOMPARE(value("string(sort(keys(winsaveview())))"),
             QLatin1String("['col', 'coladd', 'curswant', 'leftcol', 'lnum', 'skipcol',"
                           " 'topfill', 'topline']"));
    QCOMPARE(value("winsaveview().lnum .. ',' .. winsaveview().col"), QLatin1String("2,3"));
    data.doCommand("let g:v = winsaveview()");
    data.doCommand("call cursor(5, 1)");
    data.doCommand("call winrestview(g:v)");
    QCOMPARE(value("line('.') .. ',' .. col('.')"), QLatin1String("2,4"));
    // Only what the view names is put back.
    data.doCommand("call winrestview({'lnum': 4, 'col': 2})");
    QCOMPARE(value("line('.') .. ',' .. col('.')"), QLatin1String("4,3"));
    data.doCommand("unlet! g:v");
}

void FakeVimTester::test_vim_script_matchadd()
{
    // matchadd() has a place painted and says by which number it goes, counting
    // from 1000; a number of its own can be asked for, and what is taken or
    // reserved is refused. getmatches() says what is painted, the ones of lower
    // priority first. Values taken from Vim 9.1.
    TestData data;
    setup(&data);
    QString message;
    data.handler->commandBufferChanged.set(
        [&](const QString &msg, int, int, int) {
            if (!msg.startsWith("--"))
                message = msg;
        });
    QString painted;
    data.handler->selectionChanged.set(
        [&](const QList<QTextEdit::ExtraSelection> &selections) {
            painted.clear();
            for (const QTextEdit::ExtraSelection &selection : selections) {
                const int from = qMin(selection.cursor.position(), selection.cursor.anchor());
                const int length = qAbs(selection.cursor.position() - selection.cursor.anchor());
                painted += QString("%1+%2 ").arg(from).arg(length);
            }
        });
    const auto value = [&](const QString &expr) {
        message.clear();
        data.doCommand("echo " + expr);
        return message;
    };
    data.setText(X "alpha beta" N "gamma delta" N "epsilon");
    QCOMPARE(value("matchadd('Search', 'alpha')"), QLatin1String("1000"));
    QCOMPARE(value("matchadd('ErrorMsg', 'beta', 20)"), QLatin1String("1001"));
    // A group there is no highlight for is refused.
    message.clear();
    QCOMPARE(value("matchadd('NoSuchGroup', 'x')"), QLatin1String("-1"));
    QCOMPARE(value("v:errmsg"),
             QLatin1String("E28: No such highlight group name: NoSuchGroup"));
    // What is painted, and in which order.
    QCOMPARE(value("string(map(getmatches(), {i, v -> v.id .. v.group .. v.priority}))"),
             QLatin1String("['1000Search10', '1001ErrorMsg20']"));
    QCOMPARE(value("getmatches()[0].pattern"), QLatin1String("alpha"));
    // A number of its own, and what happens when it is taken or reserved.
    QCOMPARE(value("matchadd('Search', 'gamma', 30, 42)"), QLatin1String("42"));
    QCOMPARE(value("matchadd('Search', 'delta', 30, 42)"), QLatin1String("-1"));
    QCOMPARE(value("v:errmsg"), QLatin1String("E801: ID already taken: 42"));
    QCOMPARE(value("matchadd('Search', 'delta', 30, 2)"), QLatin1String("-1"));
    QCOMPARE(value("v:errmsg"), QLatin1String("E798: ID is reserved for \":match\": 2"));
    // Taking one away, and taking away one that is not there.
    QCOMPARE(value("matchdelete(42)"), QLatin1String("0"));
    QCOMPARE(value("matchdelete(42)"), QLatin1String("-1"));
    QCOMPARE(value("v:errmsg"), QLatin1String("E803: ID not found: 42"));
    // The places of matchaddpos() are kept as they were given.
    QCOMPARE(value("matchaddpos('Search', [[2, 1, 3], 3])"), QLatin1String("1002"));
    QCOMPARE(value("string(getmatches()[1].pos1) .. string(getmatches()[1].pos2)"),
             QLatin1String("[2, 1, 3][3]"));
    // And what is painted are those places, the ones of lower priority first:
    // "alpha" at the start, the three characters of line 2 and the whole of line
    // 3 from matchaddpos(), and "beta" last, its priority being the highest.
    QCOMPARE(painted, QLatin1String("0+5 11+3 23+7 6+4 "));
    // Everything can go at once, and the numbers carry on where they were.
    QCOMPARE(value("clearmatches()"), QLatin1String("0"));
    QCOMPARE(value("string(getmatches())"), QLatin1String("[]"));
    QCOMPARE(painted, QString());
    QCOMPARE(value("matchadd('Search', 'x')"), QLatin1String("1003"));
    data.doCommand("call clearmatches()");
    // A script asks whether a function is there before it calls it, so each of
    // these has to own up to existing.
    for (const QString &fn : {QString("matchadd"), QString("matchaddpos"),
                              QString("matchdelete"), QString("clearmatches"),
                              QString("getmatches")}) {
        QCOMPARE(value("exists('*" + fn + "')"), QLatin1String("1"));
    }
}

void FakeVimTester::test_vim_reflow_numbered_list()
{
    // An "n" among 'formatoptions' lines the text of a numbered list up under
    // itself, the number staying on the first line and what follows indented as
    // far as 'formatlistpat' reaches. Values taken from Vim 9.1.
    TestData data;
    setup(&data);
    data.doCommand("set textwidth=30 formatoptions=tcq");
    data.setText(X "1. alpha beta gamma delta epsilon zeta eta");
    KEYS("gqq", "1. alpha beta gamma delta" N X "epsilon zeta eta");
    data.doCommand("set formatoptions=tcqn");
    data.setText(X "1. alpha beta gamma delta epsilon zeta eta");
    KEYS("gqq", "1. alpha beta gamma delta" N "   " X "epsilon zeta eta");
    // The indent of the line counts towards it.
    data.setText(X "  3. alpha beta gamma delta epsilon zeta");
    KEYS("gqq", "  3. alpha beta gamma delta" N "     " X "epsilon zeta");
    // Another marker the option knows.
    data.setText(X "2) alpha beta gamma delta epsilon zeta eta");
    KEYS("gqq", "2) alpha beta gamma delta" N "   " X "epsilon zeta eta");
    // Text that is no list is left lined up as it was.
    data.setText(X "plain text one two three four five six seven eight");
    KEYS("gqq", "plain text one two three four" N X "five six seven eight");
    data.doCommand("set formatoptions=tcq textwidth=0");
}

void FakeVimTester::test_vim_command_undojoin()
{
    // ":undojoin" makes the change after it belong to the block before it, so
    // that one undo takes both - which is how a script makes several changes that
    // are undone together. A buffer of its own for each case, since an undo walks
    // over whatever the buffer was put through before. Values taken from Vim 9.1.
    TestData plain;
    setup(&plain);
    plain.setText(X "one" N "two" N "three");
    plain.doKeys("x");
    plain.doCommand("2s/t/T/");
    QCOMPARE(plain.text(), QByteArray("ne\nTwo\nthree"));
    plain.doKeys("u");
    QCOMPARE(plain.text(), QByteArray("ne\ntwo\nthree"));

    TestData joined;
    setup(&joined);
    joined.setText(X "one" N "two" N "three");
    joined.doKeys("x");
    joined.doCommand("undojoin | 2s/t/T/");
    QCOMPARE(joined.text(), QByteArray("ne\nTwo\nthree"));
    joined.doKeys("u");
    QCOMPARE(joined.text(), QByteArray("one\ntwo\nthree"));
}

void FakeVimTester::test_vim_command_file()
{
    // ":file" says which file this is and where the cursor stands in it, the same
    // as CTRL-G, and takes a name to call it by. Where a ruler says the place
    // already, Vim leaves it out and says how long the file is instead.
    // Values taken from Vim 9.1.
    TestData data;
    setup(&data);
    QString message;
    data.handler->commandBufferChanged.set(
        [&](const QString &msg, int, int, int) {
            if (!msg.startsWith("--"))
                message = msg;
        });
    // The buffer of a test always counts as modified, which Vim says as well.
    data.doCommand("set noruler");
    data.handler->setCurrentFileName("fi.txt");
    data.setText(X "one" N "two" N "three" N "four");
    message.clear();
    data.doCommand("file");
    QCOMPARE(message, QLatin1String("\"fi.txt\" [Modified] line 1 of 4 --25%-- col 1"));
    // Where the cursor stands is part of it.
    KEYS("2G", "one" N X "two" N "three" N "four");
    message.clear();
    data.doCommand("file");
    QCOMPARE(message, QLatin1String("\"fi.txt\" [Modified] line 2 of 4 --50%-- col 1"));
    // CTRL-G says the same.
    message.clear();
    KEYS("<C-g>", "one" N X "two" N "three" N "four");
    QCOMPARE(message, QLatin1String("\"fi.txt\" [Modified] line 2 of 4 --50%-- col 1"));
    // With a ruler the place is left out and the length takes its place.
    data.doCommand("set ruler");
    message.clear();
    data.doCommand("file");
    QCOMPARE(message, QLatin1String("\"fi.txt\" [Modified] 4 lines --50%--"));
    // One line is one line.
    data.setText(X "only");
    message.clear();
    data.doCommand("file");
    QCOMPARE(message, QLatin1String("\"fi.txt\" [Modified] 1 line --100%--"));
    // A name of its own can be given.
    data.doCommand("file other.txt");
    QCOMPARE(message, QLatin1String("\"other.txt\" [Modified] 1 line --100%--"));
    QCOMPARE(data.handler->currentFileName(), QLatin1String("other.txt"));

    // A rename fires BufFilePre with the OLD name and BufFilePost with the
    // NEW one - measured directly, both as <afile> and <amatch>. No argument
    // at all fires neither (measured explicitly, not assumed).
    data.doCommand("let g:bf = []");
    data.doCommand("autocmd FvBf BufFilePre * "
                    "call add(g:bf, 'pre:' . expand('<afile>') . ':' . expand('<amatch>'))");
    data.doCommand("autocmd FvBf BufFilePost * "
                    "call add(g:bf, 'post:' . expand('<afile>') . ':' . expand('<amatch>'))");
    data.doCommand("file renamed.txt");
    message.clear();
    data.doCommand("echo string(g:bf)");
    QCOMPARE(message,
             QLatin1String("['pre:other.txt:other.txt', 'post:renamed.txt:renamed.txt']"));
    data.doCommand("let g:bf = []");
    data.doCommand("file");
    message.clear();
    data.doCommand("echo string(g:bf)");
    QCOMPARE(message, QLatin1String("[]"));
    data.doCommand("autocmd! FvBf");
    data.doCommand("unlet! g:bf");

    data.doCommand("set noruler");
}

void FakeVimTester::test_vim_command_startinsert()
{
    // ":startinsert" and ":startreplace" reach insert and replace mode the way a
    // mapping or a script does, a "!" behind them beginning at the end of the
    // line, and ":stopinsert" leaves again. Values taken from Vim 9.1.
    TestData data;
    setup(&data);
    data.setText(X "abcd" N "efgh");
    KEYS(":startinsert<CR>XY<Esc>", "X" X "Yabcd" N "efgh");
    data.setText(X "abcd" N "efgh");
    KEYS(":startinsert!<CR>XY<Esc>", "abcdX" X "Y" N "efgh");
    data.setText(X "abcd" N "efgh");
    KEYS(":startreplace<CR>XY<Esc>", "X" X "Ycd" N "efgh");
    data.setText(X "abcd" N "efgh");
    KEYS(":startreplace!<CR>XY<Esc>", "abcdX" X "Y" N "efgh");
    // One after the other, each beginning where the cursor stands.
    data.setText(X "abcd" N "efgh");
    KEYS(":startinsert<CR>Q<Esc>:startinsert<CR>Z<Esc>", X "ZQabcd" N "efgh");
    // Leaving without having begun is no error.
    data.setText(X "abcd" N "efgh");
    KEYS(":stopinsert<CR>x", X "bcd" N "efgh");
    // And a mapping can leave insert mode in the middle of it.
    data.doCommand("inoremap <C-l> <Cmd>stopinsert<CR>");
    data.setText(X "abcd" N "efgh");
    KEYS("2|iQ<C-l>ll", "aQb" X "cd" N "efgh");
    data.doCommand("iunmap <C-l>");
}

void FakeVimTester::test_vim_whichwrap()
{
    // 'whichwrap' says which keys reach around the end of a line: a space and the
    // backspace do by default, "h" and "l" only where the option names them, and
    // a "~" carries on into the next line after changing the character.
    // Values taken from Vim 9.1.
    TestData data;
    setup(&data);
    // Another test may have left "tildeop" set, which would make "~" an operator.
    data.doCommand("set whichwrap=b,s notildeop");
    // "l" stops at the end of the line, a space does not.
    data.setText(X "ab" N "cd");
    KEYS("$l", "a" X "b" N "cd");
    data.setText(X "ab" N "cd");
    KEYS("$ ", "ab" N X "cd");
    // "h" stops at the start of it, the backspace does not.
    data.setText("ab" N X "cd");
    KEYS("0h", "ab" N X "cd");
    data.setText("ab" N X "cd");
    KEYS("0<BS>", "a" X "b" N "cd");
    // With the option naming them, they reach around as well.
    data.doCommand("set whichwrap=b,s,h,l");
    data.setText(X "ab" N "cd");
    KEYS("$l", "ab" N X "cd");
    data.setText("ab" N X "cd");
    KEYS("0h", "a" X "b" N "cd");
    // And with none of them named, neither does the space.
    data.doCommand("set whichwrap=");
    data.setText(X "ab" N "cd");
    KEYS("$ ", "a" X "b" N "cd");
    // A "~" changes the character either way, and moves on where it is named.
    data.setText(X "ab" N "cd");
    KEYS("$~", "a" X "B" N "cd");
    data.doCommand("set whichwrap=~");
    data.setText(X "ab" N "cd");
    KEYS("$~", "aB" N X "cd");
    data.doCommand("set whichwrap=b,s");
}

void FakeVimTester::test_vim_matchpairs()
{
    // What "%" jumps between is what 'matchpairs' names, and the first such
    // character from the cursor on is the one it starts at. Values taken from
    // Vim 9.1.
    TestData data;
    setup(&data);
    data.doCommand("set matchpairs=(:),{:},[:]");
    // The ones it knows by default.
    data.setText("a " X "(b) c");
    KEYS("%", "a (b" X ") c");
    KEYS("%", "a " X "(b) c");
    // A pair of its own, once the option says so.
    data.setText("a " X "<b> c");
    KEYS("%", "a " X "<b> c");
    data.doCommand("set matchpairs+=<:>");
    data.setText("a " X "<b> c");
    KEYS("%", "a <b" X "> c");
    KEYS("%", "a " X "<b> c");
    // Nested ones are counted.
    data.setText("a " X "<b <c> d> e");
    KEYS("%", "a <b <c> d" X "> e");
    // The first pair character from the cursor on is where it begins.
    data.setText(X "a=b<c>d");
    KEYS("%", "a=b<c" X ">d");
    // Where there is none, the cursor stays.
    data.setText(X "a=b<c");
    KEYS("%", X "a=b<c");
    // Letters can be a pair as well.
    data.doCommand("set matchpairs=t:d");
    data.setText(X "if a then b end");
    KEYS("%", "if a then b en" X "d");
    // With none named nothing is a pair.
    data.doCommand("set matchpairs=");
    data.setText("x " X "(y) z");
    KEYS("%", "x " X "(y) z");
    data.doCommand("set matchpairs=(:),{:},[:]");
}

void FakeVimTester::test_vim_joinspaces_gdefault()
{
    // 'joinspaces' puts two spaces behind what ends a sentence when lines are
    // joined, and 'gdefault' lets a substitute reach every place of a line by
    // itself - a "g" among the flags then saying to reach only the first.
    // Values taken from Vim 9.1.
    TestData data;
    setup(&data);
    data.doCommand("set nojoinspaces");
    data.setText(X "End of it." N "Next one");
    KEYS("J", "End of it." X " Next one");
    data.doCommand("set joinspaces");
    data.setText(X "End of it." N "Next one");
    KEYS("J", "End of it." X "  Next one");
    data.setText(X "Really?" N "Yes");
    KEYS("J", "Really?" X "  Yes");
    data.setText(X "Wow!" N "Yes");
    KEYS("J", "Wow!" X "  Yes");
    // Anything else keeps the one space.
    data.setText(X "a word" N "and more");
    KEYS("J", "a word" X " and more");
    data.doCommand("set nojoinspaces");
    // A substitute reaches every place where 'gdefault' says so.
    data.doCommand("set gdefault");
    data.setText(X "aXbXc");
    COMMAND("s/X/-/", X "a-b-c");
    data.setText(X "aXbXc");
    COMMAND("s/X/-/g", X "a-bXc");
    data.doCommand("set nogdefault");
    data.setText(X "aXbXc");
    COMMAND("s/X/-/", X "a-bXc");
}

void FakeVimTester::test_vim_visual_numbers()
{
    // Over a selection CTRL-A and CTRL-X change the first number of every line in
    // it, and with a "g" in front of them by one step more for each line they
    // changed - a line without a number counting for nothing. The cursor ends up
    // where the selection began. Values taken from Vim 9.1.
    TestData data;
    setup(&data);
    data.setText(X "a 1" N "b 2" N "c 3");
    KEYS("VG<C-a>", X "a 2" N "b 3" N "c 4");
    data.setText(X "a 1" N "b 2" N "c 3");
    KEYS("VGg<C-a>", X "a 2" N "b 4" N "c 6");
    data.setText(X "a 1" N "b 2" N "c 3");
    KEYS("VG3<C-a>", X "a 4" N "b 5" N "c 6");
    data.setText(X "a 1" N "b 2" N "c 3");
    KEYS("VG3g<C-a>", X "a 4" N "b 8" N "c 12");
    data.setText(X "a 1" N "b 2" N "c 3");
    KEYS("VG<C-x>", X "a 0" N "b 1" N "c 2");
    // A line without a number is one the steps do not count.
    data.setText(X "a 1" N "nothing" N "c 3");
    KEYS("VGg<C-a>", X "a 2" N "nothing" N "c 5");
    // Only what the selection covers is changed.
    data.setText(X "a 1" N "b 2" N "c 3");
    KEYS("vjl<C-a>", X "a 2" N "b 2" N "c 3");
    data.setText(X "x1 y2" N "x3 y4");
    KEYS("wvj$<C-a>", "x1 " X "y3" N "x4 y4");
}

void FakeVimTester::test_vim_insert_ctrl_a_e_y()
{
    // In insert mode CTRL-A puts in the text of the insert before it, CTRL-E the
    // character below the cursor and CTRL-Y the one above - and nothing at all
    // where that line does not reach so far. Values taken from Vim 9.1.
    TestData data;
    setup(&data);
    data.setText(X "one" N "two");
    KEYS("ifoo<Esc>", "fo" X "oone" N "two");
    KEYS("ji<C-a><Esc>", "fooone" N "twfo" X "oo");
    data.setText(X "one" N "two");
    KEYS("ifoo<Esc>A<C-a><Esc>", "fooonefo" X "o" N "two");
    // The characters below, one after the other.
    data.setText(X "abcd" N "wxyz");
    KEYS("i<C-e><C-e><Esc>", "w" X "xabcd" N "wxyz");
    // And the ones above.
    data.setText("abcd" N X "wxyz");
    KEYS("i<C-y><C-y><Esc>", "abcd" N "a" X "bwxyz");
    // Where the other line is shorter than the cursor stands, nothing comes in.
    data.setText(X "abcd" N "wxyz");
    KEYS("A<C-e><Esc>", "abc" X "d" N "wxyz");
    data.setText(X "abcd");
    KEYS("i<C-e><Esc>", X "abcd");
    // The character taken is the one in the column the cursor stands in.
    data.setText(X "ab" N "wxyz");
    KEYS("llA<C-e><Esc>", "ab" X "y" N "wxyz");
}

void FakeVimTester::test_vim_nrformats()
{
    // What CTRL-A steps on is what 'nrformats' allows: binary and hexadecimal and
    // octal numbers by default, letters only where "alpha" is among them. The
    // first number or letter from the cursor on is the one taken, and the cursor
    // ends up on its last character. Values taken from Vim 9.1.
    TestData data;
    setup(&data);
    data.doCommand("set nrformats=bin,octal,hex");
    // Binary, with the width kept and a carry where it is needed.
    data.setText(X "0b101");
    KEYS("<C-a>", "0b11" X "0");
    data.setText(X "0b111");
    KEYS("<C-a>", "0b100" X "0");
    data.setText(X "x0b101");
    KEYS("<C-a>", "x0b11" X "0");
    // Hexadecimal and octal as before, and a decimal number where the octal
    // digits stop.
    data.setText(X "0x0f");
    KEYS("<C-a>", "0x1" X "0");
    data.setText(X "007");
    KEYS("<C-a>", "01" X "0");
    // With "octal" allowed, a "08" is a decimal number without its leading zero.
    data.setText(X "08");
    KEYS("<C-a>", X "9");
    data.setText(X "-1");
    KEYS("<C-a>", X "0");
    // What is left out of 'nrformats' is read as a decimal number instead.
    data.doCommand("set nrformats=bin,octal");
    data.setText(X "0x0f");
    KEYS("<C-a>", X "1x0f");
    data.doCommand("set nrformats=octal,hex");
    data.setText(X "0b101");
    KEYS("<C-a>", X "1b101");
    // Where no octal number could be read, a decimal one keeps its leading zeros.
    data.doCommand("set nrformats=bin,hex");
    data.setText(X "08");
    KEYS("<C-a>", "0" X "9");
    data.setText(X "0099");
    KEYS("<C-a>", "010" X "0");
    data.setText(X "099");
    KEYS("<C-a>", "10" X "0");
    // A letter is only stepped on where "alpha" says so, and the alphabet is as
    // far as it goes.
    data.doCommand("set nrformats=bin,octal,hex");
    data.setText(X "a");
    KEYS("<C-a>", X "a");
    data.doCommand("set nrformats=bin,octal,hex,alpha");
    data.setText(X "a");
    KEYS("<C-a>", X "b");
    data.setText(X "a");
    KEYS("3<C-a>", X "d");
    data.setText(X "z");
    KEYS("<C-a>", X "z");
    data.setText(X "Y");
    KEYS("3<C-a>", X "Z");
    // The first number or letter from the cursor on is the one taken.
    data.setText(X "a 5");
    KEYS("<C-a>", X "b 5");
    data.setText(X "5 a");
    KEYS("<C-a>", X "6 a");
    data.setText(X "word 5");
    KEYS("<C-a>", X "xord 5");
    data.doCommand("set nrformats=bin,octal,hex");
}

void FakeVimTester::test_vim_search_messages()
{
    // What a search says when it finds nothing, when it comes round the end of
    // the buffer, and when 'wrapscan' forbids that - the numbers among it being
    // what a script reads out of v:errmsg. Values taken from Vim 9.1.
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
    data.doCommand("set wrapscan");
    data.setText(X "one foo" N "two" N "three foo" N "four");
    message.clear();
    KEYS("/nomatchhere<CR>", X "one foo" N "two" N "three foo" N "four");
    QCOMPARE(message, QLatin1String("E486: Pattern not found: nomatchhere"));
    QCOMPARE(value("v:errmsg"), QLatin1String("E486: Pattern not found: nomatchhere"));
    // Coming round the end of the buffer is worth a word, not an error.
    KEYS("/foo<CR>", "one " X "foo" N "two" N "three foo" N "four");
    KEYS("n", "one foo" N "two" N "three " X "foo" N "four");
    message.clear();
    KEYS("n", "one " X "foo" N "two" N "three foo" N "four");
    QCOMPARE(message, QLatin1String("search hit BOTTOM, continuing at TOP"));
    // With 'nowrapscan' it stops where the buffer does.
    data.doCommand("set nowrapscan");
    data.setText("one foo" N "two" N "three foo" N X "four");
    message.clear();
    KEYS("/foo<CR>", "one foo" N "two" N "three foo" N X "four");
    QCOMPARE(message, QLatin1String("E385: Search hit BOTTOM without match for: foo"));
    data.setText(X "one foo" N "two" N "three foo" N "four");
    message.clear();
    KEYS("?foo?<CR>", X "one foo" N "two" N "three foo" N "four");
    QCOMPARE(message, QLatin1String("E384: Search hit TOP without match for: foo"));
    data.doCommand("set wrapscan");
}

void FakeVimTester::test_vim_search_offset()
{
    // A search can say where to leave the cursor: an "e" at the end of the match,
    // a "b" or "s" at its start, either with a number behind it, and a number of
    // its own that many lines away. What "n" repeats keeps the offset, and an "e"
    // makes an operator reach over the match itself. Values taken from Vim 9.1.
    TestData data;
    setup(&data);
    data.setText(X "alpha foo beta" N "gamma foo delta" N "epsilon");
    KEYS("/foo/e<CR>", "alpha fo" X "o beta" N "gamma foo delta" N "epsilon");
    // What "n" says again keeps it.
    KEYS("n", "alpha foo beta" N "gamma fo" X "o delta" N "epsilon");
    data.setText(X "alpha foo beta" N "gamma foo delta" N "epsilon");
    KEYS("/foo/e+1<CR>", "alpha foo" X " beta" N "gamma foo delta" N "epsilon");
    data.setText(X "alpha foo beta" N "gamma foo delta" N "epsilon");
    KEYS("/foo/e-1<CR>", "alpha f" X "oo beta" N "gamma foo delta" N "epsilon");
    data.setText(X "alpha foo beta" N "gamma foo delta" N "epsilon");
    KEYS("/foo/b+2<CR>", "alpha fo" X "o beta" N "gamma foo delta" N "epsilon");
    data.setText(X "alpha foo beta" N "gamma foo delta" N "epsilon");
    KEYS("/foo/s-1<CR>", "alpha" X " foo beta" N "gamma foo delta" N "epsilon");
    // A number of its own counts lines, and the cursor goes to the first
    // character of the line it reaches.
    data.setText(X "alpha foo beta" N "gamma foo delta" N "epsilon");
    KEYS("/foo/+1<CR>", "alpha foo beta" N X "gamma foo delta" N "epsilon");
    data.setText(X "alpha foo beta" N "gamma foo delta" N "epsilon");
    KEYS("/foo/2<CR>", "alpha foo beta" N "gamma foo delta" N X "epsilon");
    // Backwards as well.
    data.setText(X "alpha foo beta" N "gamma foo delta" N "epsilon");
    KEYS("G?foo?e<CR>", "alpha foo beta" N "gamma fo" X "o delta" N "epsilon");
    // An "e" makes the operator reach over the last character of the match.
    data.setText(X "alpha foo beta" N "gamma foo delta" N "epsilon");
    KEYS("d/foo/e<CR>", X " beta" N "gamma foo delta" N "epsilon");
    // A search without one leaves no offset behind.
    data.setText(X "alpha foo beta" N "gamma foo delta" N "epsilon");
    KEYS("/foo/e<CR>/foo<CR>", "alpha foo beta" N "gamma " X "foo delta" N "epsilon");
}

void FakeVimTester::test_vim_line_change_reports()
{
    // Vim says how many lines a command changed, in words of its own for each
    // kind of change, and only where there are more of them than "report" says.
    // Values taken from Vim 9.1.
    TestData data;
    setup(&data);
    QString message;
    data.handler->commandBufferChanged.set(
        [&](const QString &msg, int, int, int) {
            if (!msg.startsWith("--"))
                message = msg;
        });
    const auto after = [&](const QString &keys) {
        message.clear();
        data.doKeys(keys);
        return message;
    };
    const auto afterCommand = [&](const QString &command) {
        message.clear();
        data.doCommand(command);
        return message;
    };
    data.doCommand("set report=0 shiftwidth=4 expandtab");
    const char *ten = X "1" N "2" N "3" N "4" N "5" N "6" N "7" N "8" N "9" N "10";

    // Lines that go are told apart from the one line that goes.
    data.setText(ten);
    QCOMPARE(after("3dd"), QLatin1String("3 fewer lines"));
    data.setText(ten);
    QCOMPARE(afterCommand("3,5d"), QLatin1String("3 fewer lines"));
    data.setText(ten);
    QCOMPARE(afterCommand("3d"), QLatin1String("1 line less"));
    // Lines yanked.
    data.setText(ten);
    QCOMPARE(after("5yy"), QLatin1String("5 lines yanked"));
    data.setText(ten);
    QCOMPARE(after("yy"), QLatin1String("1 line yanked"));
    data.setText(ten);
    QCOMPARE(afterCommand("3,7y"), QLatin1String("5 lines yanked"));
    // Lines that came, whether put or copied.
    data.setText(ten);
    data.doKeys("yy");
    QCOMPARE(after("3p"), QLatin1String("3 more lines"));
    QCOMPARE(after("p"), QLatin1String("1 more line"));
    data.setText(ten);
    QCOMPARE(afterCommand("3,5t 8"), QLatin1String("3 more lines"));
    // Lines shifted, and how many times.
    data.setText(ten);
    QCOMPARE(after("3>>"), QLatin1String("3 lines >ed 1 time"));
    data.setText(ten);
    QCOMPARE(afterCommand("3,6>"), QLatin1String("4 lines >ed 1 time"));
    data.setText(ten);
    QCOMPARE(afterCommand("3,6>>"), QLatin1String("4 lines >ed 2 times"));
    data.setText(ten);
    QCOMPARE(after("3<<"), QLatin1String("3 lines <ed 1 time"));
    // Lines moved.
    data.setText(ten);
    QCOMPARE(afterCommand("3,5m 8"), QLatin1String("3 lines moved"));
    data.setText(ten);
    QCOMPARE(afterCommand("3m 8"), QLatin1String("1 line moved"));
    // Nothing is said about as few lines as "report" allows.
    data.doCommand("set report=3");
    data.setText(ten);
    QCOMPARE(after("3dd"), QString());
    data.setText(ten);
    QCOMPARE(after("4dd"), QLatin1String("4 fewer lines"));
    data.doCommand("set report=2");
}

void FakeVimTester::test_vim_substitute_flags()
{
    // A substitute says how much it did, an "n" only counts what it would have
    // done, an "e" keeps quiet where there was nothing to do and an "I" holds on
    // to case however "ignorecase" stands. Values taken from Vim 9.1.
    TestData data;
    setup(&data);
    QString message;
    data.handler->commandBufferChanged.set(
        [&](const QString &msg, int, int, int) {
            if (!msg.startsWith("--"))
                message = msg;
        });
    data.doCommand("set report=0 noignorecase");
    data.setText(X "foo one foo" N "bar two" N "foo three");
    message.clear();
    COMMAND("%s/foo/X/g", "X one X" N "bar two" N X "X three");
    QCOMPARE(message, QLatin1String("3 substitutions on 2 lines"));
    // The "n" leaves the text as it is.
    data.setText(X "foo one foo" N "bar two" N "foo three");
    message.clear();
    COMMAND("%s/foo/X/gn", X "foo one foo" N "bar two" N "foo three");
    QCOMPARE(message, QLatin1String("3 matches on 2 lines"));
    // One is one.
    data.setText(X "foo one foo" N "bar two" N "foo three");
    message.clear();
    COMMAND("1s/foo/X/", X "X one foo" N "bar two" N "foo three");
    QCOMPARE(message, QLatin1String("1 substitution on 1 line"));
    // Nothing to substitute is worth saying, unless an "e" says not to.
    data.setText(X "foo one foo" N "bar two" N "foo three");
    message.clear();
    COMMAND("2s/foo/X/", X "foo one foo" N "bar two" N "foo three");
    QCOMPARE(message, QLatin1String("E486: Pattern not found: foo"));
    message.clear();
    COMMAND("2s/foo/X/e", X "foo one foo" N "bar two" N "foo three");
    QCOMPARE(message, QString());
    // With "ignorecase" a pattern of another case still matches, but not with "I".
    data.doCommand("set ignorecase");
    data.setText(X "foo one foo" N "bar two" N "foo three");
    message.clear();
    COMMAND("%s/FOO/X/g", "X one X" N "bar two" N X "X three");
    QCOMPARE(message, QLatin1String("3 substitutions on 2 lines"));
    data.setText(X "foo one foo" N "bar two" N "foo three");
    message.clear();
    COMMAND("%s/FOO/X/gI", X "foo one foo" N "bar two" N "foo three");
    QCOMPARE(message, QLatin1String("E486: Pattern not found: FOO"));
    data.doCommand("set noignorecase report=2");
}

void FakeVimTester::test_vim_script_buffer_lines()
{
    // getbufline() hands back the lines of a buffer as a list, however few are
    // asked for, and setbufline(), appendbufline() and deletebufline() write and
    // remove them, answering with a one where they cannot. Only the buffer this
    // handler works on can be named. Values taken from Vim 9.1.
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
    data.setText(X "one" N "two" N "three" N "four");
    QCOMPARE(value("string(getbufline('%', 2))"), QLatin1String("['two']"));
    QCOMPARE(value("string(getbufline('%', 2, 3))"), QLatin1String("['two', 'three']"));
    QCOMPARE(value("string(getbufline('%', 1, '$'))"),
             QLatin1String("['one', 'two', 'three', 'four']"));
    QCOMPARE(value("string(getbufline(bufnr(''), 1))"), QLatin1String("['one']"));
    // A line that is not there gives nothing, an end past the last one stops there.
    QCOMPARE(value("string(getbufline('%', 9))"), QLatin1String("[]"));
    QCOMPARE(value("string(getbufline('%', 3, 99))"), QLatin1String("['three', 'four']"));
    // Only this buffer can be asked about.
    QCOMPARE(value("string(getbufline(99, 1))"), QLatin1String("[]"));
    // A string writes one line, a list as many as it holds.
    QCOMPARE(value("setbufline('%', 2, 'TWO')"), QLatin1String("0"));
    QCOMPARE(data.text(), QByteArray("one\nTWO\nthree\nfour"));
    QCOMPARE(value("setbufline('%', 2, ['A', 'B'])"), QLatin1String("0"));
    QCOMPARE(data.text(), QByteArray("one\nA\nB\nfour"));
    // Past the last line nothing is written.
    QCOMPARE(value("setbufline('%', 99, 'X')"), QLatin1String("1"));
    QCOMPARE(data.text(), QByteArray("one\nA\nB\nfour"));
    QCOMPARE(value("setbufline(99, 1, 'X')"), QLatin1String("1"));
    QCOMPARE(data.text(), QByteArray("one\nA\nB\nfour"));
    // What is appended goes in behind the line named, a zero putting it in front
    // of the first.
    QCOMPARE(value("appendbufline('%', 1, 'INS')"), QLatin1String("0"));
    QCOMPARE(data.text(), QByteArray("one\nINS\nA\nB\nfour"));
    QCOMPARE(value("appendbufline('%', 0, 'TOP')"), QLatin1String("0"));
    QCOMPARE(data.text(), QByteArray("TOP\none\nINS\nA\nB\nfour"));
    // And lines can be taken away, one or several.
    QCOMPARE(value("deletebufline('%', 1)"), QLatin1String("0"));
    QCOMPARE(data.text(), QByteArray("one\nINS\nA\nB\nfour"));
    QCOMPARE(value("deletebufline('%', 1, 2)"), QLatin1String("0"));
    QCOMPARE(data.text(), QByteArray("A\nB\nfour"));
    QCOMPARE(value("deletebufline('%', 99)"), QLatin1String("1"));
    QCOMPARE(data.text(), QByteArray("A\nB\nfour"));
    QCOMPARE(value("deletebufline(99, 1)"), QLatin1String("1"));
    QCOMPARE(data.text(), QByteArray("A\nB\nfour"));
}

void FakeVimTester::test_vim_command_earlier_later()
{
    // ":earlier" goes back over the changes and ":later" forward again, a count
    // saying how many and a time - "1h" here - as far as it reaches. Neither goes
    // past what there is. Values taken from Vim 9.1.
    TestData data;
    setup(&data);
    data.setText(X "one" N "two" N "three");
    KEYS("ggxjxjx", "ne" N "wo" N X "hree");
    COMMAND("earlier 2", "ne" N X "two" N "three");
    COMMAND("later 1", "ne" N X "wo" N "three");
    // More than there is goes as far as there is.
    COMMAND("earlier 100", X "one" N "two" N "three");
    COMMAND("later 100", "ne" N "wo" N X "hree");
    // A time reaches over every change made just now.
    COMMAND("earlier 1h", X "one" N "two" N "three");
    COMMAND("later 1h", "ne" N "wo" N X "hree");
    // Without a count it is one change.
    COMMAND("earlier", "ne" N "wo" N X "three");
    COMMAND("later", "ne" N "wo" N X "hree");

    // At the ends there is nothing left to do.
    QString message;
    data.handler->commandBufferChanged.set(
        [&](const QString &msg, int, int, int) {
            if (!msg.startsWith("--"))
                message = msg;
        });
    COMMAND("later 1", "ne" N "wo" N X "hree");
    QCOMPARE(message, QLatin1String("Already at newest change."));
    COMMAND("earlier 100", X "one" N "two" N "three");
    message.clear();
    COMMAND("earlier 1", X "one" N "two" N "three");
    QCOMPARE(message, QLatin1String("Already at oldest change."));
}

void FakeVimTester::test_vim_script_list_functions()
{
    // uniq() leaves out only what stands next to something the same and takes it
    // out of the list itself, while mapnew() leaves the list it is given alone.
    // str2list() and list2str() say what characters a string is made of and put
    // one together again, keytrans() writes the keys of a string the way a mapping
    // writes them, and expandcmd() reads a command line the way ":" would.
    // Values taken from Vim 9.1.
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
    data.setText(X "hello world");
    QCOMPARE(value("string(uniq([1, 1, 2, 2, 2, 3, 1]))"), QLatin1String("[1, 2, 3, 1]"));
    QCOMPARE(value("string(uniq([3, 1, 2, 1]))"), QLatin1String("[3, 1, 2, 1]"));
    QCOMPARE(value("string(uniq(['a', 'A', 'b', 'b']))"), QLatin1String("['a', 'A', 'b']"));
    QCOMPARE(value("string(uniq(['a', 'A', 'b', 'b'], 'i'))"), QLatin1String("['a', 'b']"));
    QCOMPARE(value("string(uniq([1, 2, 2, 3], {a, b -> a == b ? 0 : 1}))"),
             QLatin1String("[1, 2, 3]"));
    // The list it is given is the one that loses the items.
    data.doCommand("let g:l = [1, 1, 2]");
    QCOMPARE(value("string(uniq(g:l)) .. ' ' .. string(g:l)"), QLatin1String("[1, 2] [1, 2]"));
    // mapnew() hands back a new one instead.
    data.doCommand("let g:l = [1, 2, 3]");
    QCOMPARE(value("string(mapnew(g:l, {i, v -> v * 2})) .. ' ' .. string(g:l)"),
             QLatin1String("[2, 4, 6] [1, 2, 3]"));
    QCOMPARE(value("string(mapnew({'x': 1, 'y': 2}, {k, v -> v + 1}))"),
             QLatin1String("{'x': 2, 'y': 3}"));
    // What a string is made of, and back again.
    QCOMPARE(value("string(str2list('abc'))"), QLatin1String("[97, 98, 99]"));
    QCOMPARE(value("string(str2list(''))"), QLatin1String("[]"));
    QCOMPARE(value("string(str2list('a' .. nr2char(228)))"), QLatin1String("[97, 228]"));
    QCOMPARE(value("string(list2str([97, 98, 99]))"), QLatin1String("'abc'"));
    QCOMPARE(value("string(list2str([]))"), QLatin1String("''"));
    QCOMPARE(value("str2list(list2str([97, 228]))[1]"), QLatin1String("228"));
    // The keys of a string, named the way a mapping names them.
    QCOMPARE(value("keytrans(\"\\<C-A>\")"), QLatin1String("<C-A>"));
    QCOMPARE(value("keytrans(\"\\<Esc>x\")"), QLatin1String("<Esc>x"));
    QCOMPARE(value("keytrans('abc')"), QLatin1String("abc"));
    QCOMPARE(value("keytrans(\"x\\<C-A>y\")"), QLatin1String("x<C-A>y"));
    QCOMPARE(value("keytrans(nr2char(31))"), QLatin1String("<C-_>"));
    QCOMPARE(value("keytrans(\"\\<Tab>\\<CR>\\<Space>\")"),
             QLatin1String("<Tab><CR><Space>"));
    // A command line the way ":" would read it.
    data.handler->setCurrentFileName("some/file.txt");
    QCOMPARE(value("expandcmd('echo %')"), QLatin1String("echo some/file.txt"));
    QCOMPARE(value("expandcmd('echo 100%')"), QLatin1String("echo 100some/file.txt"));
    QCOMPARE(value("expandcmd('echo %:t')"), QLatin1String("echo file.txt"));
    QCOMPARE(value("expandcmd('echo \\%')"), QLatin1String("echo %"));
    QCOMPARE(value("expandcmd('echo <cword>')"), QLatin1String("echo hello"));
    data.doCommand("unlet! g:l");
}

void FakeVimTester::test_vim_command_changelist()
{
    // "g;" goes back over the places changes were made, newest first, and "g,"
    // forward again. A change in the line of the newest one takes its place, and
    // any change begins the walk again. Values taken from Vim 9.1.
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
    const auto where = [&] { return value("line('.') .. ',' .. col('.')"); };
    // Nothing has been changed yet.
    data.setText(X "one two" N "three four" N "five six" N "seven" N "eight");
    KEYS("g;", X "one two" N "three four" N "five six" N "seven" N "eight");
    QCOMPARE(message, QLatin1String("E664: Changelist is empty"));
    // Three changes, on lines 1, 3 and 5.
    KEYS("ggxjjwxjjx", "ne two" N "three four" N "five ix" N "seven" N "eig" X "h");
    // The first "g;" goes to the newest of them, then back one at a time.
    KEYS("g;", "ne two" N "three four" N "five ix" N "seven" N "eig" X "h");
    QCOMPARE(where(), QLatin1String("5,4"));
    KEYS("g;", "ne two" N "three four" N "five " X "ix" N "seven" N "eigh");
    QCOMPARE(where(), QLatin1String("3,6"));
    KEYS("g;", X "ne two" N "three four" N "five ix" N "seven" N "eigh");
    QCOMPARE(where(), QLatin1String("1,1"));
    // There is nothing older.
    KEYS("g;", X "ne two" N "three four" N "five ix" N "seven" N "eigh");
    QCOMPARE(message, QLatin1String("E662: At start of changelist"));
    // "g," walks the other way, a count saying how far, and stops at the newest.
    KEYS("g,", "ne two" N "three four" N "five " X "ix" N "seven" N "eigh");
    QCOMPARE(where(), QLatin1String("3,6"));
    KEYS("2g,", "ne two" N "three four" N "five ix" N "seven" N "eig" X "h");
    QCOMPARE(where(), QLatin1String("5,4"));
    KEYS("g,", "ne two" N "three four" N "five ix" N "seven" N "eig" X "h");
    QCOMPARE(message, QLatin1String("E663: At end of changelist"));
    // Two changes in one line are one place. A buffer of its own, so that the
    // changes above are not among the ones walked over.
    TestData second;
    setup(&second);
    second.handler->commandBufferChanged.set(
        [&](const QString &msg, int, int, int) {
            if (!msg.startsWith("--"))
                message = msg;
        });
    second.setText(X "aaaa" N "bbbb" N "cccc");
    second.doKeys("ggxxjjx");
    QCOMPARE(second.text(), QByteArray("aa\nbbbb\nccc"));
    second.doKeys("g;");
    second.doKeys("g;");
    second.doCommand("echo line('.') .. ',' .. col('.')");
    QCOMPARE(message, QLatin1String("1,1"));
    message.clear();
    second.doKeys("g;");
    QCOMPARE(message, QLatin1String("E662: At start of changelist"));
}

void FakeVimTester::test_vim_command_gn()
{
    // "gn" reaches over the next place the last search pattern is found, which is
    // the one the cursor stands in where it stands in one, and "gN" the one before.
    // On its own it selects that place; with a count it takes a later one; and
    // since the dot command says the search again, "cgn" and "." change one after
    // the other. Values taken from Vim 9.1.
    TestData data;
    setup(&data);
    // The cursor stands in the first match, so that is the one taken.
    data.setText(X "foo bar foo" N "baz foo qux");
    KEYS("/foo<CR>gg0dgn", X " bar foo" N "baz foo qux");
    // Standing in the middle of a match is standing in it.
    data.setText(X "foo bar foo" N "baz foo qux");
    KEYS("/foo<CR>gg0lldgn", X " bar foo" N "baz foo qux");
    // Standing on the last character of one as well.
    data.setText(X "foo bar foo" N "baz foo qux");
    KEYS("/foo<CR>gg$dgn", "foo bar" X " " N "baz foo qux");
    // Standing outside a match, the next one is taken.
    data.setText(X "foo bar foo" N "baz foo qux");
    KEYS("/foo<CR>gg0wdgn", "foo bar" X " " N "baz foo qux");
    // A count says how many matches on.
    data.setText(X "foo bar foo" N "baz foo qux");
    KEYS("/foo<CR>ggd2gn", "foo bar" X " " N "baz foo qux");
    // The match may be on another line.
    data.setText(X "foo bar foo" N "baz foo qux");
    KEYS("/qux<CR>ggdgn", "foo bar foo" N "baz foo" X " ");
    // "gN" reaches back.
    data.setText(X "foo bar foo" N "baz foo qux");
    KEYS("/foo<CR>GdgN", "foo bar" X " " N "baz foo qux");
    // On its own it selects the match.
    data.setText(X "foo bar foo" N "baz foo qux");
    KEYS("/foo<CR>gg0wgnd", "foo bar" X " " N "baz foo qux");
    // In visual mode the selection reaches on to the end of the match.
    data.setText(X "foo bar foo");
    KEYS("/foo<CR>gg0wvgnd", "foo" X " ");
    // A change repeats with the search, which is what "cgn" is for.
    data.setText(X "foo bar foo" N "baz foo qux");
    KEYS("/foo<CR>ggcgnX<Esc>", X "X bar foo" N "baz foo qux");
    KEYS(".", "X bar " X "X" N "baz foo qux");
    // Single characters are matches too.
    data.setText(X "aXbXc");
    KEYS("/X<CR>gg0dgn", "a" X "bXc");
    // Where the pattern is nowhere to be found nothing happens.
    data.setText(X "foo bar foo");
    KEYS("/nomatchhere<CR>dgn", X "foo bar foo");
}

void FakeVimTester::test_vim_command_sort()
{
    // ":sort" compares whole lines, "i" without regard for case, "n" by the first
    // number in the line - a line without one coming first, keeping the order it
    // was in - and "x", "o", "b" and "f" by numbers of other kinds. A "/pattern/"
    // sorts by what follows the match, or by the match itself with an "r", and a
    // line the pattern misses sorts by nothing at all. A "u" drops a line equal to
    // the one before it, and a "!" turns the whole thing around. Values taken from
    // Vim 9.1.
    TestData data;
    setup(&data);
    data.setText(X "banana" N "Apple" N "cherry" N "apple" N "Banana");
    COMMAND("sort", X "Apple" N "Banana" N "apple" N "banana" N "cherry");
    data.setText(X "banana" N "Apple" N "cherry" N "apple" N "Banana");
    COMMAND("sort i", X "Apple" N "apple" N "banana" N "Banana" N "cherry");
    data.setText(X "banana" N "Apple" N "cherry" N "apple" N "Banana");
    COMMAND("sort iu", X "Apple" N "banana" N "cherry");
    // A line without a number keeps the place it had, in front of the rest.
    data.setText(X "item 10" N "item 2" N "no number" N "item -3" N "item 07" N "zzz");
    COMMAND("sort n", X "no number" N "zzz" N "item -3" N "item 2" N "item 07" N "item 10");
    data.setText(X "item 10" N "item 2" N "no number" N "item -3" N "item 07" N "zzz");
    COMMAND("sort! n", X "item 10" N "item 07" N "item 2" N "item -3" N "zzz" N "no number");
    // Hexadecimal, octal and binary, an "0x" or "0b" in front of them or not.
    data.setText(X "v 0x1f" N "v 0X0a" N "v ff" N "v 2");
    COMMAND("sort x", X "v 2" N "v 0X0a" N "v 0x1f" N "v ff");
    data.setText(X "n 010" N "n 9" N "n 07");
    COMMAND("sort o", X "n 9" N "n 07" N "n 010");
    data.setText(X "b 101" N "b 11" N "b 1000");
    COMMAND("sort b", X "b 11" N "b 101" N "b 1000");
    // A float is only read where the line begins with one, so these stay as they
    // are, while numbers of their own are put in order.
    data.setText(X "a 1.5" N "a 1.25" N "a 10" N "a -2.5");
    COMMAND("sort f", X "a 1.5" N "a 1.25" N "a 10" N "a -2.5");
    data.setText(X "1.5" N "1.25" N "10" N "-2.5");
    COMMAND("sort f", X "-2.5" N "1.25" N "1.5" N "10");
    // A pattern says what not to compare, an "r" what to compare.
    data.setText(X "x-b" N "nomatch" N "x-a" N "x-a");
    COMMAND("sort /x-/", X "nomatch" N "x-a" N "x-a" N "x-b");
    data.setText(X "x-b" N "nomatch" N "x-a" N "x-a");
    COMMAND("sort /x-/ u", X "nomatch" N "x-a" N "x-b");
    data.setText(X "xx-banana" N "yy-apple" N "zz-cherry");
    COMMAND("sort /[a-z][a-z]/ r", X "xx-banana" N "yy-apple" N "zz-cherry");
    data.setText(X "xx-banana" N "yy-apple" N "zz-cherry");
    COMMAND("sort /.*-/", X "yy-apple" N "xx-banana" N "zz-cherry");
    // What a "u" leaves out is a line equal to the one before it, not one holding
    // the same number.
    data.setText(X "b" N "a" N "b" N "a" N "c");
    COMMAND("sort nu", X "b" N "a" N "b" N "a" N "c");
    data.setText(X "i 3" N "i 1" N "i 3" N "plain" N "i 1" N "plain");
    COMMAND("sort nu", X "plain" N "i 1" N "i 3");
    // Only the lines of the range are sorted, and the cursor goes to the first
    // character of the first of them.
    data.setText("c" N "b" N "a" N "z" N X "y");
    COMMAND("2,4sort", "c" N X "a" N "b" N "z" N "y");
    data.setText(X "zz" N "    bb");
    COMMAND("sort", "    " X "bb" N "zz");
}

void FakeVimTester::test_vim_command_uniq()
{
    // ":uniq" removes ADJACENT duplicate lines only - unlike ":sort u", a
    // line equal to an EARLIER one that is not its immediate neighbour
    // survives. "!" keeps only lines that repeat (one of each); "u" keeps
    // only lines that do NOT repeat, and is ignored where "!" is also given.
    // "i" ignores case. A "/pattern/" narrows comparison to what comes AFTER
    // the match, or to the match itself with an "r"; an empty pattern reuses
    // the last search pattern. Values taken from Vim 9.1.
    TestData data;
    setup(&data);

    // The second "b" is not adjacent to the first, so it survives - the case
    // a set-based implementation gets wrong.
    data.setText(X "b" N "a" N "a" N "b" N "c" N "c" N "c");
    COMMAND("%uniq", X "b" N "a" N "b" N "c");

    // "!" keeps only what repeated, one of each.
    data.setText(X "b" N "a" N "a" N "b" N "c" N "c" N "c");
    COMMAND("%uniq!", X "a" N "c");

    // "i" ignores case; the FIRST of each run survives.
    data.setText(X "A" N "a" N "B" N "b");
    COMMAND("%uniq i", X "A" N "B");

    // "u" keeps only lines that do not repeat.
    data.setText(X "a" N "a" N "b" N "c" N "c");
    COMMAND("%uniq u", X "b");

    // "!" and "u" together: "!" wins, "u" is ignored.
    data.setText(X "a" N "a" N "b" N "c" N "c");
    COMMAND("%uniq! u", X "a" N "c");

    // A pattern with "r": compare the MATCHED text, not what follows it.
    data.setText(X "a,1" N "b,1" N "c,2");
    COMMAND("%uniq r /[^,]*,/", X "a,1" N "b,1" N "c,2");

    // Without "r": compare what comes AFTER the match.
    data.setText(X "a,1" N "b,1" N "c,2");
    COMMAND("%uniq /[^,]*,/", X "a,1" N "c,2");

    // An empty pattern reuses the last search pattern.
    data.setText(X "a,1" N "b,1" N "c,2");
    data.doCommand("let @/ = '[^,]*,'");
    COMMAND("%uniq //", X "a,1" N "c,2");

    // Only the lines of the range are affected.
    data.setText(X "x" N "a" N "a" N "y");
    COMMAND("2,3uniq", "x" N X "a" N "y");
}

void FakeVimTester::test_vim_command_smagic()
{
    // ":smagic"/":snomagic" are ":substitute" with the level the pattern
    // starts at forced on or off for that one command. Values taken from
    // Vim 9.1, measured on ['a.b', 'axb'] with a "%" range so BOTH lines
    // are tried - a range-less version only ever touches the current line
    // whatever the magic level, and looks deceptively like a pass either
    // way.
    TestData data;
    setup(&data);

    // Magic: "." is any character, so both lines match.
    data.setText(X "a.b" N "axb");
    COMMAND("%smagic /a.b/X/", "X" N X "X");
    // Nomagic: "." is a literal dot, so only the first line matches.
    data.setText(X "a.b" N "axb");
    COMMAND("%snomagic /a.b/X/", X "X" N "axb");

    // An explicit "\m"/"\M" in the pattern WINS over the forced level -
    // this is the part the ticket flagged as unmeasured, measured for it.
    data.setText(X "a.b" N "axb");
    COMMAND("%snomagic /\\ma.b/X/", "X" N X "X");
    data.setText(X "a.b" N "axb");
    COMMAND("%smagic /\\Ma.b/X/", X "X" N "axb");

    // Abbreviated, as Vim allows down to "sm" and "sno".
    data.setText(X "a.b" N "axb");
    COMMAND("%sm /a.b/X/", "X" N X "X");
    data.setText(X "a.b" N "axb");
    COMMAND("%sno /a.b/X/", X "X" N "axb");

    // Plain ":s" is unaffected, and stays magic.
    data.setText(X "a.b" N "axb");
    COMMAND("%s/a.b/X/", "X" N X "X");
}

void FakeVimTester::test_vim_script_execute_and_redir()
{
    // execute() hands back what the commands had to say instead of showing it,
    // each line of it on a line of its own - ":echon" writing on where the line
    // was left. ":redir" gives the same to a variable or a register, beginning
    // what every command line says with a blank line and ending with one of its
    // own. Values taken from Vim 9.1.
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
    data.setText(X "one" N "two" N "three");
    QCOMPARE(value("string(execute('echo \"a\"'))"), QLatin1String("'\na'"));
    QCOMPARE(value("string(execute(['echo \"a\"', 'echo \"b\"']))"), QLatin1String("'\na\nb'"));
    QCOMPARE(value("string(execute('echo 1 | echo 2'))"), QLatin1String("'\n1\n2'"));
    // ":echon" says its piece without a line break in front of it.
    QCOMPARE(value("string(execute('echon \"n\"'))"), QLatin1String("'n'"));
    // A command with nothing to say says nothing.
    QCOMPARE(value("string(execute('normal! x'))"), QLatin1String("''"));
    QCOMPARE(data.text(), QByteArray("ne\ntwo\nthree"));
    // What is shown in several lines is caught in several lines.
    data.setText(X "one" N "two" N "three");
    QCOMPARE(value("string(execute('1,2print'))"), QLatin1String("'\none\ntwo'"));
    // ":redir" writes a blank line before each command line and one at the end.
    data.doCommand("redir => g:caught");
    data.doCommand("echo 'first'");
    data.doCommand("echo 'second'");
    data.doCommand("redir END");
    QCOMPARE(value("string(g:caught)"), QLatin1String("'\n\nfirst\n\nsecond\n'"));
    // Catching nothing still ends the line.
    data.doCommand("redir => g:caught");
    data.doCommand("redir END");
    QCOMPARE(value("string(g:caught)"), QLatin1String("'\n'"));
    // The lines of one command line follow one another.
    data.doCommand("redir => g:caught");
    data.doCommand("1,3print");
    data.doCommand("redir END");
    QCOMPARE(value("string(g:caught)"), QLatin1String("'\n\none\ntwo\nthree\n'"));
    // "=>>" adds to what the variable holds.
    data.doCommand("let g:caught = 'X'");
    data.doCommand("redir =>> g:caught");
    data.doCommand("echo 'more'");
    data.doCommand("redir END");
    QCOMPARE(value("string(g:caught)"), QLatin1String("'X\n\nmore\n'"));
    // A register takes it as well.
    data.doCommand("redir @a");
    data.doCommand("echo 'toreg'");
    data.doCommand("redir END");
    QCOMPARE(value("string(@a)"), QLatin1String("'\n\ntoreg\n'"));
    data.doCommand("unlet! g:caught");
}

void FakeVimTester::test_vim_command_print()
{
    // ":print" shows the lines of its range with their tabs reaching to the next
    // tab stop, ":number" puts the line numbers in front of them and ":list" shows
    // a tab as "^I" and marks the end of the line with a "$". A count says how
    // many lines from the last one of the range, and the cursor ends up on the
    // last line shown. Values taken from Vim 9.1.
    TestData data;
    setup(&data);
    QString info;
    data.handler->extraInformationChanged.set([&](const QString &text) { info = text; });
    const auto shown = [&](const QString &command) {
        info.clear();
        data.doCommand(command);
        return info;
    };
    data.doCommand("set tabstop=8");
    data.setText(X "alpha" N "be\tta" N "gamma" N "    indented line");
    QCOMPARE(shown("2,3print"), QLatin1String("be      ta\ngamma"));
    QCOMPARE(shown("2,3number"), QLatin1String("  2 be      ta\n  3 gamma"));
    QCOMPARE(shown("2,3list"), QLatin1String("be^Ita$\ngamma$"));
    QCOMPARE(shown("2,3#"), QLatin1String("  2 be      ta\n  3 gamma"));
    // A count says how many lines, counted from the end of the range.
    QCOMPARE(shown("1p 3"), QLatin1String("alpha\nbe      ta\ngamma"));
    // The cursor lands on the first character of the last line shown.
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
    KEYS("1G", X "alpha" N "be\tta" N "gamma" N "    indented line");
    data.doCommand("1,4print");
    QCOMPARE(value("line('.') .. ',' .. col('.')"), QLatin1String("4,5"));
    // ":global" without a command of its own shows what it found, all of it.
    KEYS("1G", X "alpha" N "be\tta" N "gamma" N "    indented line");
    QCOMPARE(shown("g/a/"), QLatin1String("alpha\nbe      ta\ngamma"));
    QCOMPARE(shown("g/a/number"), QLatin1String("  1 alpha\n  2 be      ta\n  3 gamma"));
    QCOMPARE(value("line('.') .. ',' .. col('.')"), QLatin1String("3,1"));
}

void FakeVimTester::test_vim_command_z()
{
    // ":z[+-^.=][count]" prints a window of lines around the address, the
    // current line if none is given - richer than it looks, and each mark's
    // cursor placement was measured independently rather than trusted from
    // the doc's own "no mark is the same as +" line, which turned out to be
    // true of the RANGE only, not the cursor. Values taken from Vim 9.1 on
    // the ten-line buffer '1'..'10', address 5.
    TestData data;
    setup(&data);
    QString info;
    data.handler->extraInformationChanged.set([&](const QString &text) { info = text; });
    QString message;
    data.handler->commandBufferChanged.set(
        [&](const QString &msg, int, int, int) {
            if (!msg.isEmpty() && !msg.startsWith("--"))
                message = msg;
        });
    const auto ten = [&] {
        data.setText("1" N "2" N "3" N "4" N "5" N "6" N "7" N "8" N "9" N "10");
    };
    const auto shownAndCursor = [&](const QString &command) {
        info.clear();
        data.doCommand(command);
        message.clear();
        data.doCommand("echo line('.')");
        return info + " @" + message;
    };

    // Bare starts AT the address; "+" starts AFTER it - despite the doc's own
    // "no mark is the same as +" line, the two are NOT the same, in either
    // range or cursor.
    ten();
    QCOMPARE(shownAndCursor("5z3"), QLatin1String("5\n6\n7 @7"));
    ten();
    QCOMPARE(shownAndCursor("5z+3"), QLatin1String("6\n7\n8 @8"));
    // "-": ends AT the address; cursor stays on the address.
    ten();
    QCOMPARE(shownAndCursor("5z-3"), QLatin1String("3\n4\n5 @5"));
    // ".": centred, no decoration; cursor on the last line shown.
    ten();
    QCOMPARE(shownAndCursor("5z.3"), QLatin1String("4\n5\n6 @6"));
    // "^": a window back from "-"'s own window, which runs off the top and
    // CLAMPS - two lines survive, not three; cursor on the last line shown.
    ten();
    QCOMPARE(shownAndCursor("5z^3"), QLatin1String("1\n2 @2"));
    // The same "^" with enough headroom that nothing clamps - the clamped
    // case alone cannot tell the right offset from one off by a line, which
    // a first attempt got wrong (address - 2*count + 1 instead of the
    // correct address - 2*count) and this caught.
    data.setText("1" N "2" N "3" N "4" N "5" N "6" N "7" N "8" N "9" N "10" N "11" N "12" N
                 "13" N "14" N "15");
    QCOMPARE(shownAndCursor("10z^3"), QLatin1String("4\n5\n6\n7 @7"));
    // "=": decorates with a 79-dash separator around the address, which is
    // shown ONCE (not twice); cursor stays on the address. The half-window on
    // each side is (count+1)/2, not count/2 - measured by count 1..4, where
    // 1 and 2 give the same (smaller) window and 3 and 4 the same (larger)
    // one.
    const QString dashes(79, '-');
    ten();
    QCOMPARE(shownAndCursor("5z=3"),
             QString("3\n4\n" + dashes + "\n5\n" + dashes + "\n6\n7 @5"));
    ten();
    QCOMPARE(shownAndCursor("5z=1"),
             QString("4\n" + dashes + "\n5\n" + dashes + "\n6 @5"));

    // No address at all is ONE LINE FURTHER than the current line, not the
    // current line itself - measured directly: from the same cursor
    // position, an explicit address naming the current line and no address
    // at all give DIFFERENT windows.
    ten();
    KEYS("5G", "1" N "2" N "3" N "4" N X "5" N "6" N "7" N "8" N "9" N "10");
    QCOMPARE(shownAndCursor("z3"), QLatin1String("6\n7\n8 @8"));

    // Clamping at the bottom of the buffer: "+" starts one past the address
    // (line 10), which is already the last line, so only one line survives
    // regardless of count.
    ten();
    QCOMPARE(shownAndCursor("9z+3"), QLatin1String("10 @10"));
    ten();
    QCOMPARE(shownAndCursor("8z+3"), QLatin1String("9\n10 @10"));
}

void FakeVimTester::test_vim_command_align()
{
    // ":left", ":center" and ":right" line the text up, ":left" to an indent and
    // the other two within a width that is taken from "textwidth", or 80 where
    // that is not set. The cursor keeps its line and goes to the first character
    // on it, an empty line is left alone, and trailing whitespace stays where it
    // is without counting towards the width. Values taken from Vim 9.1.
    TestData data;
    setup(&data);
    data.doCommand("set expandtab tabstop=8 textwidth=0");
    data.setText(X "aaa" N "    bbb" N "longer line here" N "");
    COMMAND("1,3left", X "aaa" N "bbb" N "longer line here" N "");
    data.setText(X "aaa" N "    bbb" N "longer line here" N "");
    COMMAND("1,3left 6", "      " X "aaa" N "      bbb" N "      longer line here" N "");
    data.setText(X "aaa" N "    bbb" N "longer line here" N "");
    COMMAND("1,3center 20", "        " X "aaa" N "        bbb" N "  longer line here" N "");
    data.setText(X "aaa" N "    bbb" N "longer line here" N "");
    COMMAND("1,3right 20", "                 " X "aaa" N "                 bbb"
                           N "    longer line here" N "");
    // Without a width of its own, "textwidth" says how wide.
    data.doCommand("set textwidth=30");
    data.setText(X "aaa" N "bbb");
    COMMAND("1,2right", "                           " X "aaa" N "                           bbb");
    // With no textwidth either it is 80.
    data.doCommand("set textwidth=0");
    data.setText(X "aaa");
    COMMAND("1center", QString(38, ' ').toUtf8() + X "aaa");
    // The cursor stays on its line, and an empty line is left as it is.
    data.setText("aaa" N "bbb" N X "ccc" N "");
    COMMAND("1,4center 20", "        aaa" N "        bbb" N "        " X "ccc" N "");
    // Trailing whitespace is kept and does not count.
    data.setText(X "aa   " N "bb");
    COMMAND("1,2right 20", "                  " X "aa   " N "                  bb");
    // A tab inside the line moves along with the indent: the widest indent whose
    // line still fits is the one taken.
    data.setText(X "b\tc");
    COMMAND("1right 20", "              " X "b\tc");
    data.setText(X "b\tc");
    COMMAND("1right 12", "      " X "b\tc");
    // An indent is written with tabs where "expandtab" is off.
    data.doCommand("set noexpandtab");
    data.setText(X "aaa" N "bbb");
    COMMAND("1,2left 10", "\t  " X "aaa" N "\t  bbb");
    data.doCommand("set expandtab textwidth=0");
}

void FakeVimTester::test_vim_command_copy()
{
    // ":copy", also spelled ":t", leaves the lines where they are and puts a copy
    // behind the line its address names - in front of the first line for a "0" -
    // with the cursor on the last line copied. Values taken from Vim 9.1.
    TestData data;
    setup(&data);
    data.setText(X "one" N "two" N "three" N "four");
    COMMAND("1copy 3", "one" N "two" N "three" N X "one" N "four");
    data.setText(X "one" N "two" N "three" N "four");
    COMMAND("1,2copy 0", "one" N X "two" N "one" N "two" N "three" N "four");
    data.setText(X "one" N "two" N "three" N "four");
    COMMAND("2t .", "one" N X "two" N "two" N "three" N "four");
    data.setText(X "one" N "two" N "three" N "four");
    COMMAND("$copy 1", "one" N X "four" N "two" N "three" N "four");
    data.setText(X "one" N "two" N "three" N "four");
    COMMAND("1copy $", "one" N "two" N "three" N "four" N X "one");
    data.setText(X "one" N "two" N "three" N "four");
    COMMAND("2,3t 0", "two" N X "three" N "one" N "two" N "three" N "four");
    // Without an address there is nothing to copy to.
    data.setText(X "one" N "two");
    COMMAND("1copy", X "one" N "two");
}

void FakeVimTester::test_vim_script_histories()
{
    // The entries of a history are numbered from one, and Vim keeps the numbers
    // when one is removed - so a hole stays where a middle entry was, while a
    // negative index counts back over what is left. An entry written again moves
    // to the end under a new number. Values taken from Vim 9.1.
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
    data.setText(X "alpha" N "beta xyz" N "gamma");
    // Whatever earlier tests searched for is in the history too: with it gone the
    // numbers start over, as they do in Vim.
    data.doCommand("call histdel('search')");
    // A search is remembered, a substitute leaves its pattern there as well.
    KEYS("/beta<CR>", "alpha" N X "beta xyz" N "gamma");
    QCOMPARE(value("histnr('search')"), QLatin1String("1"));
    QCOMPARE(value("histget('search', -1)"), QLatin1String("beta"));
    QCOMPARE(value("histget('search', 1)"), QLatin1String("beta"));
    // Index zero names no entry.
    QCOMPARE(value("histget('search', 0)"), QLatin1String(""));
    QCOMPARE(value("histget('search', -5)"), QLatin1String(""));
    COMMAND("s/xyz/X/", "alpha" N X "beta X" N "gamma");
    QCOMPARE(value("histget('/', -1)"), QLatin1String("xyz"));
    QCOMPARE(value("histnr('/')"), QLatin1String("2"));
    // What histadd() writes is the newest entry.
    QCOMPARE(value("histadd('search', 'zeta')"), QLatin1String("1"));
    QCOMPARE(value("histget('search', -1)"), QLatin1String("zeta"));
    QCOMPARE(value("histnr('search')"), QLatin1String("3"));
    // Removing the middle one leaves its number unused. What is removed is what
    // the item matches as a pattern.
    QCOMPARE(value("histdel('search', 'xyz')"), QLatin1String("1"));
    QCOMPARE(value("histnr('search')"), QLatin1String("3"));
    QCOMPARE(value("histget('search', 1) . '/' . histget('search', 2)"
                   " . '/' . histget('search', 3)"), QLatin1String("beta//zeta"));
    QCOMPARE(value("histget('search', -2)"), QLatin1String("beta"));
    // An entry written again moves to the end.
    QCOMPARE(value("histadd('search', 'beta')"), QLatin1String("1"));
    QCOMPARE(value("histnr('search') . '/' . histget('search', -1)"
                   " . '/' . histget('search', -2)"), QLatin1String("4/beta/zeta"));
    // A pattern that matches nothing removes nothing.
    QCOMPARE(value("histdel('search', 'nomatch')"), QLatin1String("0"));
    // Without an item the whole history goes, leaving no number at all.
    QCOMPARE(value("histdel('search')"), QLatin1String("1"));
    QCOMPARE(value("histnr('search')"), QLatin1String("-1"));
    QCOMPARE(value("histget('search', -1)"), QLatin1String(""));
    // A history that is not known here is told apart from an empty one by neither.
    QCOMPARE(value("histnr('nosuch')"), QLatin1String("-1"));
    QCOMPARE(value("histget('nosuch', -1)"), QLatin1String(""));
    // The command lines are a history of their own.
    KEYS(":let g:x = 1<CR>", "alpha" N X "beta X" N "gamma");
    QCOMPARE(value("histget('cmd', -1)"), QLatin1String("let g:x = 1"));
    QCOMPARE(value("histget(':', -1)"), QLatin1String("let g:x = 1"));
    data.doCommand("unlet! g:x");
}

void FakeVimTester::test_vim_special_registers()
{
    // "/" is the last search pattern - readable and writable, so a script can
    // say what "n" looks for - while ".", "%" and ":" only tell what was last
    // inserted, which file this is and what the last command line was.
    // Values taken from Vim 9.1.
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
    data.setText(X "one two three" N "two one two" N "three two one");
    KEYS("/three<CR>", "one two " X "three" N "two one two" N "three two one");
    QCOMPARE(value("@/"), QLatin1String("three"));
    // What is written there is what "n" goes to next.
    data.doCommand("let @/ = 'two'");
    KEYS("n", "one two three" N X "two one two" N "three two one");
    QCOMPARE(value("@/"), QLatin1String("two"));
    // A substitute leaves its pattern there as well.
    data.setText(X "one two three");
    COMMAND("s/one/ONE/", X "ONE two three");
    QCOMPARE(value("@/"), QLatin1String("one"));
    // "." is the text of the last insert, without the indent that came by itself.
    data.setText(X "    indented" N "plain");
    KEYS("ox<Esc>", "    indented" N "    " X "x" N "plain");
    QCOMPARE(value("@."), QLatin1String("x"));
    KEYS("A y<Esc>", "    indented" N "    x " X "y" N "plain");
    QCOMPARE(value("@."), QLatin1String(" y"));
    // "%" is the name of the file being edited.
    data.handler->setCurrentFileName("some/file.txt");
    QCOMPARE(value("@%"), QLatin1String("some/file.txt"));
    QCOMPARE(value("@% == expand('%')"), QLatin1String("1"));
    // ":" is the last command line, without its colon.
    KEYS(":let g:x = 1<CR>", "    indented" N "    x " X "y" N "plain");
    QCOMPARE(value("@:"), QLatin1String("let g:x = 1"));
    // The three of them cannot be written to.
    data.doCommand("let @. = 'nope'");
    QCOMPARE(value("@."), QLatin1String(" y"));
    data.doCommand("unlet! g:x");
}

void FakeVimTester::test_vim_command_gi()
{
    // "gi" takes up where insert mode was left, which is what the "^" mark holds,
    // and "gI" puts the cursor in front of the line whatever its indent is.
    // Values taken from Vim 9.1.
    TestData data;
    setup(&data);
    data.setText(X "alpha" N "beta");
    KEYS("A!<Esc>j0giX<Esc>", "alpha!" X "X" N "beta");
    data.setText(X "alpha" N "beta");
    KEYS("ggIX<Esc>GgiY<Esc>", "X" X "Yalpha" N "beta");
    // Nothing inserted yet: the cursor stays where it is.
    data.setText(X "alpha");
    KEYS("giX<Esc>", X "Xalpha");
    // "gI" reaches in front of the indent.
    data.setText(X "    indented");
    KEYS("gIX<Esc>", X "X    indented");
    data.setText("    inden" X "ted");
    KEYS("gIX<Esc>", X "X    indented");
    // The mark says where insert mode was left, one past what was written.
    QString message;
    data.handler->commandBufferChanged.set(
        [&](const QString &msg, int, int, int) {
            if (!msg.startsWith("--"))
                message = msg;
        });
    data.setText(X "alpha" N "beta");
    data.doKeys("A!<Esc>");
    message.clear();
    data.doCommand("echo line(\"'^\") .. ',' .. col(\"'^\")");
    QCOMPARE(message, QLatin1String("1,7"));
}

void FakeVimTester::test_vim_command_g_underscore()
{
    // "g_" goes to the last character of the line that is not a blank, a count
    // taking as many lines down; "gp" and "gP" leave the cursor behind what they
    // put in rather than on it. Values taken from Vim 9.1.
    TestData data;
    setup(&data);
    data.setText(X "one two  " N "three");
    KEYS("g_x", "one tw" X "  " N "three");
    data.setText(X "  spaced  " N "x");
    KEYS("g_x", "  space" X "  " N "x");
    data.setText(X "one" N "two" N "three");
    KEYS("2g_x", "one" N "t" X "w" N "three");
    // A line put in with "gp" leaves the cursor on the line behind it.
    data.setText(X "alpha" N "beta");
    KEYS("yyjgp", "alpha" N "beta" N X "alpha");
    data.setText(X "alpha" N "beta" N "gamma");
    KEYS("yyjgp", "alpha" N "beta" N "alpha" N X "gamma");
    data.setText(X "alpha" N "beta");
    KEYS("yyjgP", "alpha" N "alpha" N X "beta");
    // Characters put in leave the cursor behind them as well.
    data.setText(X "abc");
    KEYS("ylgp", "aa" X "bc");
}

void FakeVimTester::test_vim_command_put_with_indent()
{
    // "]p" puts lines in behind this one and "[p" in front of it, each moved over
    // by as much as the first one needs to sit at this line's indent - so pasted
    // code takes the indent of where it lands, keeping what it had among itself.
    // The cursor stands on the first character of the first line put in. Values
    // taken from Vim 9.1 with 'expandtab' and 'shiftwidth' 4.
    TestData data;
    setup(&data);
    data.doCommand("set expandtab | set shiftwidth=4 | set tabstop=8");
    data.setText(X "        deep" N "    target");
    KEYS("yyj]p", "        deep" N "    target" N "    " X "deep");
    data.setText(X "    indented" N "x");
    KEYS("yyj]p", "    indented" N "x" N X "indented");
    data.setText(X "    indented" N "x");
    KEYS("yyj[p", "    indented" N X "indented" N "x");
    // Two lines keep the indent they have among themselves.
    data.setText(X "    a" N "        b" N "target");
    KEYS("Vjy" "G" "]p", "    a" N "        b" N "target" N X "a" N "    b");
    data.setText(X "    a" N "        b" N "    target");
    KEYS("Vjy" "G" "]p", "    a" N "        b" N "    target" N "    " X "a" N "        b");
    data.doCommand("set noexpandtab | set shiftwidth=8");
}

void FakeVimTester::test_vim_command_go()
{
    // "go" goes to a byte, counted from one as line2byte() counts, and ":goto"
    // does the same. A line ending belongs to its line, so the cursor stands on
    // the last character of it, and a byte past the end gives the last place
    // there is. Values taken from Vim 9.1.
    TestData data;
    setup(&data);
    data.setText("abc" N "de" N X "f");
    KEYS("5go", "abc" N X "de" N "f");
    data.setText("abc" N "de" N X "f");
    KEYS("1go", X "abc" N "de" N "f");
    data.setText("abc" N "de" N X "f");
    KEYS("go", X "abc" N "de" N "f");
    // Byte four is the line ending of the first line.
    data.setText("abc" N "de" N X "f");
    KEYS("4go", "ab" X "c" N "de" N "f");
    data.setText("abc" N "de" N X "f");
    KEYS("8go", "abc" N "de" N X "f");
    data.setText("abc" N "de" N X "f");
    KEYS("99go", "abc" N "de" N X "f");
    // The same as an ex command.
    data.setText("abc" N "de" N X "f");
    COMMAND("goto 5", "abc" N X "de" N "f");
    data.setText("abc" N "de" N X "f");
    COMMAND("go 1", X "abc" N "de" N "f");
}

void FakeVimTester::test_vim_command_g_ampersand()
{
    // What ":s" makes of the parts it is given again: a pattern of no length is
    // the last search pattern, a "~" in the replacement is the replacement that
    // was used, and an "&" among the flags keeps the flags. "g&" is ":%s//~/&",
    // so it says the last substitution again over every line. Values taken from
    // Vim 9.1.
    TestData data;
    setup(&data);
    // The ex forms first.
    data.setText(X "aa x" N "aa y");
    data.doCommand("s/a/X/");
    COMMAND("2s//Y/", "Xa x" N X "Ya y");
    data.setText(X "ab" N "ab");
    data.doCommand("s/a/X/");
    COMMAND("2s/b/~/", "Xb" N X "aX");
    data.setText(X "ab" N "ab");
    data.doCommand("s/a/X/");
    COMMAND("2s/b/p~q/", "Xb" N X "apXq");
    data.setText(X "ab" N "ab");
    data.doCommand("s/a/X/");
    COMMAND("2s/b/\\~/", "Xb" N X "a~");
    data.setText(X "aa x" N "aa y");
    data.doCommand("s/a/X/g");
    COMMAND("2s//Y/&", "XX x" N X "YY y");
    data.setText(X "aa x" N "aa y");
    data.doCommand("s/a/X/g");
    COMMAND("2s//Y/", "XX x" N X "Ya y");
    data.setText(X "aa x" N "aa y" N "aa z");
    data.doCommand("s/a/X/");
    KEYS("g&", "XX x" N "Xa y" N X "Xa z");
    data.setText(X "aa x" N "aa y" N "aa z");
    data.doCommand("s/a/X/g");
    KEYS("g&", "XX x" N "XX y" N X "XX z");
    // The line the last replacement happened in is where the cursor ends up.
    data.setText(X "foo1" N "foo2");
    data.doCommand("s/\\d/N/");
    KEYS("jg&", "fooN" N X "fooN");
    // A search in between says what is replaced.
    data.setText(X "aa x" N "aa y");
    data.doCommand("s/a/X/");
    KEYS("/y<CR>g&", "Xa x" N X "aa X");
}

void FakeVimTester::test_vim_pattern_class_and_lookaround()
{
    // What a character class means and what an operator behind it applies to.
    // A backslash inside a class is a character of its own, the "^" or "]" right
    // after the "[" stands for itself, and "\\@<=" and its kin look at the class
    // rather than at whatever stood before it. Values taken from Vim 9.1.
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
    // A class taking in a backslash, and the bar behind it looked back at.
    QCOMPARE(value("match('a|b', '[^|\\\\]\\@1<=|')"), QLatin1String("1"));
    QCOMPARE(value("match('a\\|b', '[^|\\\\]\\@1<=|')"), QLatin1String("-1"));
    // The first character of a class is itself, negation or bracket.
    QCOMPARE(value("matchstr('a=b', '[^=]')"), QLatin1String("a"));
    QCOMPARE(value("matchstr('a^b', '[\\^]')"), QLatin1String("^"));
    QCOMPARE(value("match('}', '^\\s*[]})]')"), QLatin1String("0"));
    QCOMPARE(value("match(']', '^\\s*[]})]')"), QLatin1String("0"));
    QCOMPARE(value("match('a', '[]a]')"), QLatin1String("0"));
    QCOMPARE(value("match('z', '[]})]')"), QLatin1String("-1"));
    // Where nothing closes the class, Vim reads the "[]" as the two characters.
    QCOMPARE(value("match('x[]y', '[]')"), QLatin1String("1"));
    // How far back Vim is told to look is Vim's own business, and the class is
    // what the operator applies to - not the "*" or whatever came before it.
    QCOMPARE(value("match('ab', 'a\\@1<=b')"), QLatin1String("1"));
    QCOMPARE(value("match('xab', 'a\\@2<=b')"), QLatin1String("2"));
    QCOMPARE(value("match('a(', '[^=]\\zs[[(]')"), QLatin1String("1"));
    QCOMPARE(value("match('a=(', '[^=]\\zs[[(]')"), QLatin1String("-1"));
    QCOMPARE(value("match('a||b', '||\\@!')"), QLatin1String("2"));
    // A "\\zs" inside a lookaround says nothing about where the match begins,
    // there being nothing of the lookaround in the match at all.
    QCOMPARE(value("matchstr('foo{', '\\%(z\\zs\\)\\@!.*\\zs{')"), QLatin1String("{"));
}

void FakeVimTester::test_vim_script_changedtick()
{
    // b:changedtick counts the changes made to the text, which is how a plugin
    // tells whether anything happened since it last looked - vim-repeat keeps
    // it to know whether "." should repeat its own mapping or the change. Vim
    // counts an undo as several changes where this counts one, so the test asks
    // what Vim answers about it and not for its numbers. Values taken from
    // Vim 9.1.
    TestData data;
    setup(&data);
    data.setText("one" N "two");
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
    QCOMPARE(value("exists('b:changedtick')"), QLatin1String("1"));
    QCOMPARE(value("type(b:changedtick)"), QLatin1String("0"));
    QCOMPARE(value("getbufvar('%', 'changedtick') == b:changedtick"), QLatin1String("1"));
    QCOMPARE(value("index(keys(b:), 'changedtick') >= 0"), QLatin1String("1"));
    // Moving about is not a change; changing the text is.
    data.doCommand("let g:t = b:changedtick");
    data.doKeys("lj");
    QCOMPARE(value("b:changedtick == g:t"), QLatin1String("1"));
    data.doKeys("ix<Esc>");
    QCOMPARE(value("b:changedtick > g:t"), QLatin1String("1"));
    // Neither writing nor deleting it is allowed.
    data.doCommand("let g:e = '' | try | let b:changedtick = 99"
                   " | catch | let g:e = v:exception | endtry");
    QVERIFY(value("g:e").contains(QLatin1String("E46")));
    data.doCommand("let g:e = '' | try | unlet b:changedtick"
                   " | catch | let g:e = v:exception | endtry");
    QVERIFY(value("g:e").contains(QLatin1String("E795")));
    QCOMPARE(value("exists('b:changedtick')"), QLatin1String("1"));
    data.doCommand("unlet g:t | unlet g:e");
}

void FakeVimTester::test_vim_script_delfunction()
{
    // ":delfunction" takes back a function, which is how a script replaces one
    // or cleans up after itself. Values taken from Vim 9.1.
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
    data.doCommand("function! Gone()");
    data.doCommand("  return 1");
    data.doCommand("endfunction");
    QCOMPARE(value("exists('*Gone')"), QLatin1String("1"));
    data.doCommand("delfunction Gone");
    QCOMPARE(value("exists('*Gone')"), QLatin1String("0"));
    // What is not there is an error, unless the "!" says it need not be.
    message.clear();
    data.doCommand("delfunction Gone");
    QCOMPARE(message, QLatin1String("E117: Unknown function: Gone"));
    message.clear();
    data.doCommand("delfunction! Gone");
    QCOMPARE(message, QString());
    // The command can be shortened, and a variable holding a funcref is not a
    // function.
    data.doCommand("function! Short()");
    data.doCommand("  return 1");
    data.doCommand("endfunction");
    data.doCommand("delfunc Short");
    QCOMPARE(value("exists('*Short')"), QLatin1String("0"));
    data.doCommand("let Ref = function('strlen')");
    message.clear();
    data.doCommand("delfunction Ref");
    QCOMPARE(message, QLatin1String("E117: Unknown function: Ref"));
    QCOMPARE(value("Ref('abc')"), QLatin1String("3"));
    data.doCommand("unlet Ref");
}

void FakeVimTester::test_vim_ex_bar_after_file_name()
{
    // A file name is full of slashes and none of them starts a pattern, so the
    // "|" after it still ends the command. Only a command that takes a pattern
    // holds a "|" of its own. Values taken from Vim 9.1.
    TestData data;
    setup(&data);
    data.setText("b" N "ccc");
    data.doKeys("ia|<Esc>"); // a literal bar, which setText would read as the cursor
    QString message;
    data.handler->commandBufferChanged.set(
        [&](const QString &msg, int, int, int) {
            if (!msg.startsWith("--"))
                message = msg;
        });
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    QVERIFY(QDir(dir.path()).mkpath("sub/deep"));
    QFile f(dir.path() + "/sub/deep/s.vim");
    QVERIFY(f.open(QIODevice::WriteOnly));
    f.write("let g:sourced = 1\n");
    f.close();
    data.doCommand("let g:sourced = 0 | let g:after = 0");
    data.doCommand("source " + dir.path() + "/sub/deep/s.vim | let g:after = 2");
    message.clear();
    data.doCommand("echo g:sourced . ' ' . g:after");
    QCOMPARE(message, QLatin1String("1 2"));
    data.doCommand("unlet g:sourced | unlet g:after");
    // Inside a substitute pattern the bar is an ordinary character.
    data.doKeys(":%s/a|b/X/<CR>");
    KEYS("", X "X" N "ccc");
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

    // The unnamed register is a pointer to whatever register was written last:
    // getreginfo() asked about it names that one, and asked about any other says
    // whether it is the one pointed to.
    const auto pointsTo = [&] { return echo("getreginfo('\"')['points_to']"); };
    data.setText(X "aaa" N "bbb" N "ccc" N "ddd" N "eee");
    data.doKeys("yy");
    QCOMPARE(pointsTo(), QLatin1String("0"));
    QCOMPARE(echo("string(getreginfo('\"'))"),
             QLatin1String("{'points_to': '0', 'regcontents': ['aaa'], 'regtype': 'V'}"));
    QCOMPARE(echo("string(getreginfo('0'))"),
             QLatin1String("{'isunnamed': v:true, 'regcontents': ['aaa'], 'regtype': 'V'}"));
    // A named yank points there, and the yank register is no longer the one.
    data.doKeys("j" "\"ayy");
    QCOMPARE(pointsTo(), QLatin1String("a"));
    QCOMPARE(echo("getreginfo('a')['isunnamed']"), QLatin1String("v:true"));
    QCOMPARE(echo("getreginfo('0')['isunnamed']"), QLatin1String("v:false"));
    // What whole lines are deleted from goes to register 1, which is pointed to
    // even where a register was named.
    data.doKeys("dd");
    QCOMPARE(pointsTo(), QLatin1String("1"));
    data.doKeys("\"bdd");
    QCOMPARE(pointsTo(), QLatin1String("1"));
    QCOMPARE(echo("getreg('b')"), QLatin1String("ccc\n"));
    QCOMPARE(echo("getreginfo('b')['isunnamed']"), QLatin1String("v:false"));
    QCOMPARE(echo("getreginfo('1')['isunnamed']"), QLatin1String("v:true"));
    // Less than a line goes to the small delete register, unless a register was
    // named, which is then the one pointed to.
    data.doKeys("x");
    QCOMPARE(pointsTo(), QLatin1String("-"));
    data.doKeys("\"cx");
    QCOMPARE(pointsTo(), QLatin1String("c"));
    // The black hole register tells nothing of what went into it.
    data.doKeys("\"_dd");
    QCOMPARE(pointsTo(), QLatin1String("c"));
    // Writing the unnamed register writes the yank register and points there.
    data.doCommand("let @@ = 'by hand'");
    QCOMPARE(pointsTo(), QLatin1String("0"));
    QCOMPARE(echo("getreg('0')"), QLatin1String("by hand"));
    // A block tells its width here too.
    data.setText(X "abcd" N "efgh");
    data.doKeys("<C-v>jl" "y");
    QCOMPARE(echo("getreginfo('\"')['regtype'][0] == nr2char(22)"), QLatin1String("1"));
    QCOMPARE(echo("getreginfo('\"')['regtype'][1:]"), QLatin1String("2"));
    QCOMPARE(echo("string(getreginfo('\"')['regcontents'])"), QLatin1String("['ab', 'ef']"));
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
    // mapping reaches a function of its own, and "<SNR>42_name" is the name that
    // one is held under. Values taken from Vim 9.1.
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
            "let g:ownName = s:name()\n"
            "let g:sid = expand('<SID>')\n"
            "let g:snrInSfile = matchstr(g:ownName, '^<SNR>\\d\\+_')\n");
    f.close();
    data.doCommand("source " + dir.path() + "/s.vim");

    message.clear();
    data.doCommand("echo g:viaSid");
    QCOMPARE(message, QLatin1String("from the script"));
    // The name it found is one that can be called again.
    message.clear();
    data.doCommand("echo exists('*' . g:ownName)");
    QCOMPARE(message, QLatin1String("1"));
    // Vim spells such a name "<SNR>42_name", and a plugin reads that prefix out
    // of it to build the name of another function of its own. The number depends
    // on how many scripts were sourced before, so only the shape is checked.
    message.clear();
    data.doCommand("echo g:snrInSfile != '' ? 'has prefix' : 'bare ' . g:ownName");
    QCOMPARE(message, QLatin1String("has prefix"));
    message.clear();
    data.doCommand("echo g:sid =~# '^<SNR>\\d\\+_$'");
    QCOMPARE(message, QLatin1String("1"));
    // And what it names can be called again, however it is spelled.
    message.clear();
    data.doCommand("echo exists('*' . g:sid . 'answer')");
    QCOMPARE(message, QLatin1String("1"));
    data.doCommand("unlet g:viaSid | unlet g:ownName | unlet g:sid | unlet g:snrInSfile");
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

void FakeVimTester::test_vim_script_const()
{
    // ":const {name} = {expr}" is ":let" immediately locked, reusing the same
    // lock ":lockvar" already bites with (E741). Values taken from Vim 9.1.
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

    data.doCommand("const g:cc = 5");
    QCOMPARE(echo("g:cc"), QLatin1String("5"));
    message.clear();
    data.doCommand("let g:cc = 6");
    QCOMPARE(message, QLatin1String("Uncaught exception: E741: Value is locked: g:cc"));
    QCOMPARE(echo("g:cc"), QLatin1String("5"));

    // Abbreviated, as Vim allows down to "cons".
    data.doCommand("cons g:dd = 7");
    message.clear();
    data.doCommand("let g:dd = 8");
    QCOMPARE(message, QLatin1String("Uncaught exception: E741: Value is locked: g:dd"));

    data.doCommand("unlockvar g:cc | unlet g:cc");
    data.doCommand("unlockvar g:dd | unlet g:dd");
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

    // <args> and <q-args>. Both say "-nargs=1", since a command that says
    // nothing about it takes no argument at all - in Vim as here.
    data.doCommand("command -nargs=1 SetX let g:x = <args>");
    data.doCommand("SetX 42");
    QCOMPARE(echo("g:x"), QLatin1String("42"));
    data.doCommand("command -nargs=1 Say let g:said = <q-args>");
    data.doCommand("Say hi there");
    QCOMPARE(echo("g:said"), QLatin1String("hi there"));

    // <bang>.
    data.doCommand("command Bang let g:bang = \"<bang>\"");
    data.doCommand("Bang!");
    QCOMPARE(echo("\"[\" . g:bang . \"]\""), QLatin1String("[!]"));
    data.doCommand("Bang");
    QCOMPARE(echo("\"[\" . g:bang . \"]\""), QLatin1String("[]"));

    // Attributes are accepted; "-nargs" is acted on, the rest passed over.
    data.doCommand("command -nargs=1 Double let g:dbl = <args> * 2");
    data.doCommand("Double 21");
    QCOMPARE(echo("g:dbl"), QLatin1String("42"));

    // <f-args> for passing to a function.
    data.doCommand("function Store(a, b) | let g:pair = a:a . \",\" . a:b | endfunction");
    data.doCommand("command -nargs=+ Pair call Store(<f-args>)");
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

void FakeVimTester::test_vim_motion_nowhere_to_go()
{
    // A motion with a count moves as far as it can and fails only where it
    // cannot move at all: "5j" on the last line, "3l" at the end of it. What
    // fails takes the keys that came with it down: the "x" after it is not
    // handled, and an operator waiting for the motion gives up rather than
    // taking the line the cursor is on. Values taken from Vim 9.1 running the
    // same ":normal" over the same three lines.
    TestData data;
    setup(&data);
    QString message;
    data.handler->commandBufferChanged.set(
        [&](const QString &msg, int, int, int) {
            if (!msg.startsWith("--"))
                message = msg;
        });
    const auto run = [&](const char *keys) -> QString {
        data.setText(X "one" N "two" N "three");
        data.doCommand(QString("normal! ") + keys);
        message.clear();
        data.doCommand("echo line('.') . ',' . col('.')");
        return QString::fromUtf8(data.text()).replace(QLatin1Char('\n'), QLatin1String("/"))
               + " at " + message;
    };
    // As far as there is room for, the last line or the last character.
    QCOMPARE(run("1G5jx"), QLatin1String("one/two/hree at 3,1"));
    QCOMPARE(run("1G1jx"), QLatin1String("one/wo/three at 2,1"));
    QCOMPARE(run("3G5kx"), QLatin1String("ne/two/three at 1,1"));
    QCOMPARE(run("1G9lx"), QLatin1String("on/two/three at 1,2"));
    QCOMPARE(run("1G100Gx"), QLatin1String("one/two/hree at 3,1"));
    QCOMPARE(run("2G3d_"), QLatin1String("one at 1,1"));
    QCOMPARE(run("1G3d_"), QLatin1String(" at 1,1"));
    QCOMPARE(run("2G3ddx"), QLatin1String("ne at 1,1"));
    // Nowhere at all: nothing happens, and the "x" is dropped with it.
    QCOMPARE(run("3G5jx"), QLatin1String("one/two/three at 3,1"));
    QCOMPARE(run("1G5kx"), QLatin1String("one/two/three at 1,1"));
    QCOMPARE(run("3G$3lx"), QLatin1String("one/two/three at 3,5"));
    QCOMPARE(run("1G3hx"), QLatin1String("one/two/three at 1,1"));
    QCOMPARE(run("3G2d_x"), QLatin1String("one/two/three at 3,1"));
    QCOMPARE(run("3Gdjx"), QLatin1String("one/two/three at 3,1"));
    QCOMPARE(run("1Gdkx"), QLatin1String("one/two/three at 1,1"));
    QCOMPARE(run("3G2yyx"), QLatin1String("one/two/three at 3,1"));
}

void FakeVimTester::test_vim_marks_follow_the_text()
{
    // A mark follows the text: one below lines that came or went moves with
    // them, and one on a line that is gone is forgotten. A selection of whole
    // lines is named from the start of its first line to one past the last
    // character of its last, which is where Vim puts "'>". Values taken from
    // Vim 9.1 pressing the same keys.
    TestData data;
    setup(&data);
    QString message;
    data.handler->commandBufferChanged.set(
        [&](const QString &msg, int, int, int) {
            if (!msg.startsWith("--"))
                message = msg;
        });
    const auto run = [&](const char *keys, const QString &expr) {
        data.setText(X "one" N "two" N "three" N "four");
        data.doKeys(keys);
        message.clear();
        data.doCommand("echo " + expr);
        return message;
    };
    const QString ma = "line(\"'a\") . ',' . col(\"'a\")";
    const QString mv = "line(\"'<\") . ',' . col(\"'<\") . '-'"
                       " . line(\"'>\") . ',' . col(\"'>\")";
    // Lines taken away above it, and lines added above it.
    QCOMPARE(run("3Gma1Gdd", ma), QLatin1String("2,1"));
    QCOMPARE(run("4Gma1G2dd", ma), QLatin1String("2,1"));
    QCOMPARE(run("3Gma1GO9<ESC>", ma), QLatin1String("4,1"));
    QCOMPARE(run("1Gma1GO9<ESC>", ma), QLatin1String("2,1"));
    // The line it stood on taken away, and it is gone with it.
    QCOMPARE(run("3Gma3Gdd", ma), QLatin1String("0,0"));
    // Whole lines are named to one past the end of the last, whatever column
    // the selection was drawn from.
    QCOMPARE(run("1GVj<ESC>", mv), QLatin1String("1,1-2,4"));
    QCOMPARE(run("1G2lVj<ESC>", mv), QLatin1String("1,1-2,4"));
    QCOMPARE(run("3G2lV<ESC>", mv), QLatin1String("3,1-3,6"));
    // A selection of characters is named as it was drawn.
    QCOMPARE(run("1Gvjl<ESC>", mv), QLatin1String("1,1-2,2"));
    // The visual marks are kept where the text stood, not forgotten with it.
    QCOMPARE(run("1GVj<ESC>1Gdd", mv), QLatin1String("1,1-1,4"));
}

void FakeVimTester::test_vim_visual_marks_when_left()
{
    // "'<" and "'>" name the selection that was LEFT: while one is being drawn
    // they still hold the one before it, and they are written when visual mode
    // ends - by hand, by an operator, or by the ":" that a plugin mapping opens.
    // An undo does not take them back with the text either. Values taken from
    // Vim 9.1 pressing the same keys.
    TestData data;
    setup(&data);
    data.setText("one" N "two" N "three" N "four");
    QString message;
    data.handler->commandBufferChanged.set(
        [&](const QString &msg, int, int, int) {
            if (!msg.startsWith("--"))
                message = msg;
        });
    // Read from a mapping that keeps the mode it is called in, which is how a
    // plugin looks at them: the columns are left out of it, as Vim puts "'>" at
    // the end of the last line of a linewise selection and this keeps the column.
    const QString marks = "line(\"'<\") . '-' . line(\"'>\")";
    data.doCommand("nnoremap ,l <Cmd>echo " + marks + "<CR>");
    data.doCommand("xnoremap ,v <Cmd>echo " + marks + "<CR>");
    const auto read = [&](const char *keys) {
        message.clear();
        data.doKeys(keys);
        return message;
    };
    // Nothing selected yet, and drawing one does not write them.
    QCOMPARE(read(",l"), QLatin1String("0-0"));
    QCOMPARE(read("1GVj,v"), QLatin1String("0-0"));
    // Leaving does.
    QCOMPARE(read("<ESC>,l"), QLatin1String("1-2"));
    // The next selection is still the one before while it is being drawn.
    QCOMPARE(read("3GVj,v"), QLatin1String("1-2"));
    QCOMPARE(read("<ESC>,l"), QLatin1String("3-4"));
    // An operator applied to a selection writes them as well, and an undo of it
    // leaves them naming what was taken back.
    QCOMPARE(read("1GVj~,l"), QLatin1String("1-2"));
    QCOMPARE(read("u,l"), QLatin1String("1-2"));
    // "gv" selects what they name; in visual mode the two areas are exchanged.
    QCOMPARE(read("3GV,v"), QLatin1String("1-2"));
    QCOMPARE(read("gv,v"), QLatin1String("3-3"));
    QCOMPARE(read("gv,v"), QLatin1String("1-2"));
    data.doKeys("<ESC>");
    data.doCommand("nunmap ,l | xunmap ,v");
}

void FakeVimTester::test_vim_script_hlsearch()
{
    // v:hlsearch says whether the matches of the last search are highlighted,
    // which is what a plugin asks before taking the highlighting away. It follows
    // 'hlsearch' and ":nohlsearch", and writing it does what ":nohlsearch" does.
    // Values taken from Vim 9.1.
    TestData data;
    setup(&data);
    data.setText("one two one" N "three one");
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
    data.doCommand("set hlsearch");
    // Nothing has been searched for yet, and Vim still says the highlighting is on.
    QCOMPARE(value("v:hlsearch"), QLatin1String("1"));
    data.doKeys("/one<CR>");
    QCOMPARE(value("v:hlsearch"), QLatin1String("1"));
    data.doCommand("nohlsearch");
    QCOMPARE(value("v:hlsearch"), QLatin1String("0"));
    // A search of its own brings it back.
    data.doKeys("n");
    QCOMPARE(value("v:hlsearch"), QLatin1String("1"));
    // Written either way.
    data.doCommand("let v:hlsearch = 0");
    QCOMPARE(value("v:hlsearch"), QLatin1String("0"));
    data.doCommand("let v:hlsearch = 1");
    QCOMPARE(value("v:hlsearch"), QLatin1String("1"));
    // And nothing is highlighted where the option says so.
    data.doCommand("set nohlsearch");
    QCOMPARE(value("v:hlsearch"), QLatin1String("0"));
    data.doCommand("set hlsearch");
}

void FakeVimTester::test_vim_script_heredoc_and_comments()
{
    // What a Vim9 script writes that used to be read as something else: "=<<"
    // inside a string is part of it, a heredoc may begin on a line right after
    // another statement, and a "#" comment may stand behind the code, a
    // continuation line carrying one of its own. Values taken from Vim 9.1.
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
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const auto source = [&](const QString &name, const QByteArray &text) {
        QFile f(dir.path() + '/' + name);
        [[maybe_unused]] const bool ok = f.open(QIODevice::WriteOnly);
        f.write(text);
        f.close();
        data.doCommand("source " + dir.path() + '/' + name);
    };
    // A pattern for the heredoc operator is a string, not a heredoc: this used to
    // swallow the rest of the file (it is what vimindent.vim keeps).
    source("s.vim", "let g:pat = '\\s=<<\\s\\@=\\%(\\s\\+\\%(trim\\|eval\\)\\)\\{,2}'\n"
                    "let g:after = 'still here'\n");
    QCOMPARE(value("g:pat"), QLatin1String("\\s=<<\\s\\@=\\%(\\s\\+\\%(trim\\|eval\\)\\)\\{,2}"));
    QCOMPARE(value("g:after"), QLatin1String("still here"));
    // A heredoc right behind another statement, with nothing between them.
    source("h.vim", "let g:a = 1\n"
                    "let g:body =<< trim END\n"
                    "  one\n"
                    "  two\n"
                    "END\n");
    QCOMPARE(value("g:a"), QLatin1String("1"));
    QCOMPARE(value("string(g:body)"), QLatin1String("['one', 'two']"));
    // A comment behind the code, and one on each line of a continuation.
    source("c.vim", "vim9script\n"
                    "var x = 'a'  # a comment\n"
                    "      .. 'b'  # another\n"
                    "g:joined = x\n"
                    "var y = 'a#b'   # a hash inside a string stays\n"
                    "g:instring = y\n"
                    "var n = 1 + # trailing on an operator line\n"
                    "      2\n"
                    "g:sum = n\n");
    QCOMPARE(value("g:joined"), QLatin1String("ab"));
    QCOMPARE(value("g:instring"), QLatin1String("a#b"));
    QCOMPARE(value("g:sum"), QLatin1String("3"));
    data.doCommand("unlet g:pat | unlet g:after | unlet g:a | unlet g:body");
    data.doCommand("unlet g:joined | unlet g:instring | unlet g:sum");
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
    // Replayed on the last line, the "j" in the middle of the macro has nowhere
    // to go: it fails and what is left of the macro is dropped, so no "- opq"
    // is inserted this time (measured in Vim 9.1, which ends at 5,5 as well).
    KEYS("@x" , "abc" N "- xyZ" N "- opq" N "def" N "- xy" X "Z");

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

void FakeVimTester::test_mcp_keys()
{
    FvBoolAspect &useFakeVim = FakeVim::Internal::settings().useFakeVim;
    const bool savedUseFakeVim = useFakeVim.value();
    useFakeVim.setValue(true);

    TestData data;
    setup(&data);
    data.setText("some text");

    // Keys sent through the tool edit the current editor, and the reported
    // cursor is where the keys left it: after "ihello <Esc>" it sits on the
    // inserted space, column 6 of line 1.
    const Utils::Result<Mcp::Schema::CallToolResult> result = Mcp::ToolRegistry::callToolForTests(
        "fakevim_keys",
        Mcp::Schema::CallToolRequestParams{}.arguments(QJsonObject{{"keys", "ihello <Esc>"}}));
    QVERIFY2(result, result ? "" : qPrintable(result.error()));
    QVERIFY(!result->isError().value_or(false));
    QCOMPARE(data.text(), QByteArray("hello some text"));
    const QJsonObject content = result->structuredContentAsObject();
    QCOMPARE(content.value("line").toInt(), 1);
    QCOMPARE(content.value("column").toInt(), 6);

    // With FakeVim disabled the tool refuses instead of editing the buffer
    // through the still existing handler.
    useFakeVim.setValue(false);
    const Utils::Result<Mcp::Schema::CallToolResult> disabled = Mcp::ToolRegistry::callToolForTests(
        "fakevim_keys",
        Mcp::Schema::CallToolRequestParams{}.arguments(QJsonObject{{"keys", "x"}}));
    QVERIFY(!disabled);
    QVERIFY2(disabled.error().contains("disabled"), qPrintable(disabled.error()));
    QCOMPARE(data.text(), QByteArray("hello some text"));

    useFakeVim.setValue(savedUseFakeVim);
}

void FakeVimTester::test_mcp_argument_validation()
{
    using Mcp::ToolRegistry;
    using Params = Mcp::Schema::CallToolRequestParams;

    FvBoolAspect &useFakeVim = FakeVim::Internal::settings().useFakeVim;
    const bool savedUseFakeVim = useFakeVim.value();
    useFakeVim.setValue(true);
    TestData data;
    setup(&data);
    data.setText("some text");

    // An argument the tool does not declare is refused, and the message says
    // which names it does take.
    const auto unknown = ToolRegistry::callToolForTests(
        "fakevim_keys",
        Params{}.arguments(QJsonObject{{"keys", "x"}, {"mode", "keys"}}));
    QVERIFY(!unknown);
    QVERIFY2(unknown.error().contains("Unknown argument \"mode\""), qPrintable(unknown.error()));
    QVERIFY2(unknown.error().contains("keys"), qPrintable(unknown.error()));
    // And it is refused BEFORE the tool runs, so the buffer is untouched.
    QCOMPARE(data.text(), QByteArray("some text"));

    // A required argument that is not there is refused by name.
    const auto missing = ToolRegistry::callToolForTests("fakevim_keys", Params{});
    QVERIFY(!missing);
    QVERIFY2(missing.error().contains("Missing required argument \"keys\""),
             qPrintable(missing.error()));

    // A value of the wrong kind is refused, saying what was wanted.
    const auto wrongType = ToolRegistry::callToolForTests(
        "fakevim_keys", Params{}.arguments(QJsonObject{{"keys", 42}}));
    QVERIFY(!wrongType);
    QVERIFY2(wrongType.error().contains("wants a string"), qPrintable(wrongType.error()));
    QCOMPARE(data.text(), QByteArray("some text"));

    // And a call that keeps to the schema still goes through.
    const auto good = ToolRegistry::callToolForTests(
        "fakevim_keys", Params{}.arguments(QJsonObject{{"keys", "x"}}));
    QVERIFY2(good, good ? "" : qPrintable(good.error()));
    QCOMPARE(data.text(), QByteArray("ome text"));

    useFakeVim.setValue(savedUseFakeVim);
}

} // FakeVim::Internal

#undef N
#undef X

#include "fakevim_test.moc"
