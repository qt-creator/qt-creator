/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef TEXTEDITORPLUGIN_H
#define TEXTEDITORPLUGIN_H

#include <extensionsystem/iplugin.h>

namespace TextEditor {

class FontSettings;
class TabSettings;
class TextEditorSettings;

namespace Internal {

class LineNumberFilter;
class OutlineFactory;
class TextMarkRegistry;

class TextEditorPlugin : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "TextEditor.json")

public:
    TextEditorPlugin();
    virtual ~TextEditorPlugin();

    // ExtensionSystem::IPlugin
    bool initialize(const QStringList &arguments, QString *errorMessage);
    void extensionsInitialized();

    static LineNumberFilter *lineNumberFilter();
    static TextMarkRegistry *baseTextMarkRegistry();

private slots:
    void updateSearchResultsFont(const TextEditor::FontSettings &);
    void updateSearchResultsTabWidth(const TextEditor::TabSettings &tabSettings);
    void updateCurrentSelection(const QString &text);

private:
    TextEditorSettings *m_settings;
    LineNumberFilter *m_lineNumberFilter;
    OutlineFactory *m_outlineFactory;
    TextMarkRegistry *m_baseTextMarkRegistry;

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
#endif

};

} // namespace Internal
} // namespace TextEditor

#endif // TEXTEDITORPLUGIN_H
