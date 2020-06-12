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

#include "ieditor.h"

/*!
  \class Core::IEditor
  \inheaderfile coreplugin/editormanager/ieditor.h
  \inmodule QtCreator

  \brief The IEditor class is an interface for providing suitable editors for
  documents according to their MIME type.

  Classes that implement this interface are for example the editors for
  C++ files, UI files, and resource files.

  Whenever a user wants to edit or create a document, the EditorManager
  scans all IEditorFactory interfaces for suitable editors. The selected
  IEditorFactory is then asked to create an editor, which must implement
  this interface.

  \sa Core::IEditorFactory, Core::EditorManager
*/

/*!
    \fn Core::IDocument *Core::IEditor::document() const
    Returns the document to open in an editor.
*/

/*!
    \fn Core::IEditor *Core::IEditor::duplicate()
    Duplicates the editor.

    \sa duplicateSupported()
*/

/*!
    \fn QByteArray Core::IEditor::saveState() const
    Saves the state of the document.
*/

/*!
    \fn bool Core::IEditor::restoreState(const QByteArray &state)
    Restores the \a state of the document.

    Returns \c true on success.
*/

/*!
    \fn int Core::IEditor::currentLine() const
    Returns the current line in the document.
*/

/*!
    \fn int Core::IEditor::currentColumn() const
    Returns the current column in the document.
*/

/*!
    \fn void Core::IEditor::gotoLine(int line, int column = 0, bool centerLine = true)
    Goes to \a line and \a column in the document. If \a centerLine is
    \c true, centers the line in the editor.
*/

/*!
    \fn QWidget Core::IEditor::toolBar()
    Returns the toolbar for the editor.

    The editor toolbar is located at the top of the editor view. The editor
    toolbar is context sensitive and shows items relevant to the document
    currently open in the editor.
*/

/*! \fn bool Core::IEditor::isDesignModePreferred() const
    Indicates whether the document should be opened in the Design mode.
    Returns \c false unless Design mode is preferred.
*/

namespace Core {

/*!
    \internal
*/
IEditor::IEditor(QObject *parent)
    : IContext(parent), m_duplicateSupported(false)
{}

/*!
    Returns whether duplication is supported.
*/
bool IEditor::duplicateSupported() const
{
    return m_duplicateSupported;
}

/*!
    Sets whether duplication is supported to \a duplicatesSupported.
*/
void IEditor::setDuplicateSupported(bool duplicatesSupported)
{
    m_duplicateSupported = duplicatesSupported;
}

} // namespace Core
