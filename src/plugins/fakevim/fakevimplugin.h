/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**************************************************************************/

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
    void test_vim_movement();
    void test_vim_fFtT();
    void test_vim_transform_numbers();
    void test_vim_delete();
    void test_vim_delete_inner_word();
    void test_vim_delete_a_word();
    void test_vim_change_a_word();
    void test_vim_block_selection();
    void test_vim_repeat();
    void test_vim_search();
    void test_vim_indent();
    void test_vim_marks();
    void test_vim_copy_paste();
    void test_vim_undo_redo();
    void test_advanced_commands();
    void test_map();
#endif
};

} // namespace Internal
} // namespace FakeVim

#endif // FAKEVIMPLUGIN_H
