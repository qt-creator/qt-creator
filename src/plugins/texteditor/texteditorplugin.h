/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#pragma once

#include <extensionsystem/iplugin.h>

namespace TextEditor {
namespace Internal {

class LineNumberFilter;

class TextEditorPlugin : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "TextEditor.json")

public:
    TextEditorPlugin();
    ~TextEditorPlugin() final;

    static TextEditorPlugin *instance();
    static LineNumberFilter *lineNumberFilter();

private:
    bool initialize(const QStringList &arguments, QString *errorMessage) final;
    void extensionsInitialized() final;

    class TextEditorPluginPrivate *d = nullptr;

#ifdef WITH_TESTS
private slots:
    void testSnippetParsing_data();
    void testSnippetParsing();

    void testBlockSelectionTransformation_data();
    void testBlockSelectionTransformation();
    void testBlockSelectionInsert_data();
    void testBlockSelectionInsert();
    void testBlockSelectionRemove_data();
    void testBlockSelectionRemove();
    void testBlockSelectionCopy_data();
    void testBlockSelectionCopy();

    void testIndentationClean_data();
    void testIndentationClean();
#endif
};

} // namespace Internal
} // namespace TextEditor
