/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef FAKEVIMPLUGIN_H
#define FAKEVIMPLUGIN_H

#include <extensionsystem/iplugin.h>

namespace FakeVim {
namespace Internal {

class FakeVimHandler;
class FakeVimPluginPrivate;

class FakeVimPlugin : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "FakeVim.json")

public:
    FakeVimPlugin();
    ~FakeVimPlugin();

private:
    // implementation of ExtensionSystem::IPlugin
    bool initialize(const QStringList &arguments, QString *errorMessage);
    ShutdownFlag aboutToShutdown();
    void extensionsInitialized();

private:
    friend class FakeVimPluginPrivate;
    FakeVimPluginPrivate *d;

#ifdef WITH_TESTS
private slots:
    void cleanup();
    void test_vim_movement();
    void test_vim_insert();
    void test_vim_fFtT();
    void test_vim_transform_numbers();
    void test_vim_delete();
    void test_vim_delete_inner_word();
    void test_vim_delete_a_word();
    void test_vim_change_a_word();
    void test_vim_change_replace();
    void test_vim_block_selection();
    void test_vim_repeat();
    void test_vim_search();
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
    void test_vim_ex_yank();
    void test_vim_ex_delete();
    void test_vim_ex_change();
    void test_vim_ex_shift();
    void test_vim_ex_move();
    void test_vim_ex_join();
    void test_advanced_commands();

//public slots:
//    void changeStatusData(const QString &info) { m_statusData = info; }
//    void changeStatusMessage(const QString &info, int) { m_statusMessage = info; }
//    void changeExtraInformation(const QString &info) { m_infoMessage = info; }

//private slots:
//    // functional tests
    void test_vim_indentation();

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

    void test_vim_visual_d();
    void test_vim_Visual_d();

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
    void setupTest(QString *title, FakeVimHandler **handler, QWidget **edit);
#endif
};

} // namespace Internal
} // namespace FakeVim

#endif // FAKEVIMPLUGIN_H
