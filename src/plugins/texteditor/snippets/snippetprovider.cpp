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

#include "snippetprovider.h"

#include "texteditorplugin.h"

#include <utils/algorithm.h>

using namespace TextEditor;

static QList<SnippetProvider> g_snippetProviders;

const QList<SnippetProvider> &SnippetProvider::snippetProviders()
{
    return g_snippetProviders;
}

/*!
    \group Snippets
    \title Snippets for Editors

    Snippets typically consist of chunks of code in a programming language (although they
    can also be plain text) which you would like to avoid re-typing every time. They
    are triggered in the same way as the completions (see \l{CodeAssist}{Providing
    Code Assist}).

    In order to create a new group of snippets two steps are necessary:
    \list
        \li Register the group with TextEditor::SnippetProvider::registerGroup
        \li Create an XML configuration file and place it in the
        /share/qtcreator/snippets directory. As an example of the file format
        please take a look at the already available ones. The meaning and consistency rules
        of the fields are described below:
        \list
            \li group - This is the group in which the snippet belongs in the user interface.
            It must match TextEditor::SnippetProvider::groupId().
            \li id - A unique string that identifies this snippet among all others available.
            The recommended practice is to prefix it with the group so it is easier to have
            such control on a file level.
            \li trigger - The sequence of characters to be compared by the completion engine
            in order to display this snippet as a code assist proposal.
            \li complement - Additional information that is displayed in the code assist
            proposal so it is possible to disambiguate similar snippets that have the
            same trigger.
        \endlist
    \endlist

    All XML configuration files found in the directory mentioned above are parsed by
    Qt Creator. However, only the ones which are associated with known groups (specified
    by a provider) are taken into consideration.
*/

/*!
    \class TextEditor::SnippetProvider
    \brief The SnippetProvider class acts as an interface for providing groups of snippets.
    \ingroup Snippets
*/

/*!
    Returns the unique group id to which this provider is associated.
*/
QString SnippetProvider::groupId() const
{
    return m_groupId;
}

/*!
    Returns the name to be displayed in the user interface for snippets that belong to the group
    associated with this provider.
*/
QString SnippetProvider::displayName() const
{
    return m_displayName;
}

/*!
    Applies customizations such as highlighting or indentation to the snippet editor.
 */
void SnippetProvider::decorateEditor(TextEditorWidget *editor, const QString &groupId)
{
    for (const SnippetProvider &provider : g_snippetProviders) {
        if (provider.m_groupId == groupId && provider.m_editorDecorator)
            provider.m_editorDecorator(editor);
    }
}

/*!
    Registers a snippet group with \a groupId, \a displayName and \a editorDecorator.
 */
void SnippetProvider::registerGroup(const QString &groupId, const QString &displayName,
                                     EditorDecorator editorDecorator)
{
    SnippetProvider provider;
    provider.m_groupId = groupId;
    provider.m_displayName = displayName;
    provider.m_editorDecorator = editorDecorator;
    g_snippetProviders.append(provider);
}
