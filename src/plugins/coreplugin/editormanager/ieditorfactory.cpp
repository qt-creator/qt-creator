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

#include "ieditorfactory.h"
#include "ieditorfactory_p.h"
#include "editormanager.h"

#include <utils/mimetypes/mimedatabase.h>
#include <utils/qtcassert.h>

#include <QFileInfo>

namespace Core {

/*!
    \class Core::IEditorFactory
    \inheaderfile coreplugin/editormanager/ieditorfactory.h
    \inmodule QtCreator

    \brief The IEditorFactory class creates suitable editors for documents
    according to their MIME type.

    Whenever a user wants to edit or create a document, the EditorManager
    scans all IEditorFactory interfaces for suitable editors. The selected
    IEditorFactory is then asked to create an editor.

    Guidelines for the implementation:

    \list
        \li displayName() is used as a user visible description of the editor
            type that is created. For example, the name displayed in the
            \uicontrol {Open With} menu.
        \li If duplication is supported (IEditor::duplicateSupported()), you
            need to ensure that all duplicates return the same document().
    \endlist

    \sa Core::IEditor, Core::EditorManager
*/

/*!
    \fn void Core::IEditorFactory::addMimeType(const QString &mimeType)
    Adds \a mimeType to the list of MIME types supported by this editor type.
*/

/*!
    \fn QString Core::IEditorFactory::displayName() const
    Returns a user-visible description of the editor type.
*/

/*!
    \fn Utils::Id Core::IEditorFactory::id() const
    Returns the ID of the factory or editor type.
*/

/*!
    \fn QString Core::IEditorFactory::mimeTypes() const
    Returns a list of MIME types that the editor supports.
*/

/*!
    \fn void Core::IEditorFactory::setDisplayName(const QString &displayName)
    Sets the \a displayName of the factory or editor type.
*/

/*!
    \fn void Core::IEditorFactory::setId(Id id)
    Sets the \a id of the factory or editor type.
*/

/*!
    \fn void Core::IEditorFactory::setMimeTypes(const QStringList &mimeTypes)
    Sets the MIME types supported by the editor to \a mimeTypes.
*/

static QList<IEditorFactory *> g_editorFactories;
static QHash<Utils::MimeType, IEditorFactory *> g_userPreferredEditorFactories;

/*!
    \internal
*/
IEditorFactory::IEditorFactory()
{
    g_editorFactories.append(this);
}

/*!
    \internal
*/
IEditorFactory::~IEditorFactory()
{
    g_editorFactories.removeOne(this);
}

/*!
    \internal
*/
const EditorFactoryList IEditorFactory::allEditorFactories()
{
    return g_editorFactories;
}

/*!
    Returns all available editors for this \a mimeType in the default order
    (editors ordered by MIME type hierarchy).
*/
const EditorFactoryList IEditorFactory::defaultEditorFactories(const Utils::MimeType &mimeType)
{
    EditorFactoryList rc;
    const EditorFactoryList allFactories = IEditorFactory::allEditorFactories();
    Internal::mimeTypeFactoryLookup(mimeType, allFactories, &rc);
    return rc;
}

/*!
    Returns the available editors for \a fileName in order of preference.
    That is the default order for the document's MIME type but with a user
    overridden default editor first, and if the document is a too large
    text file, with the binary editor as the very first.
*/
const EditorFactoryList IEditorFactory::preferredEditorFactories(const QString &fileName)
{
    const QFileInfo fileInfo(fileName);
    // default factories by mime type
    const Utils::MimeType mimeType = Utils::mimeTypeForFile(fileInfo);
    EditorFactoryList factories = defaultEditorFactories(mimeType);
    const auto factories_moveToFront = [&factories](IEditorFactory *f) {
        factories.removeAll(f);
        factories.prepend(f);
    };
    // user preferred factory to front
    IEditorFactory *userPreferred = Internal::userPreferredEditorFactories().value(mimeType);
    if (userPreferred)
        factories_moveToFront(userPreferred);
    // open text files > 48 MB in binary editor
    if (fileInfo.size() > EditorManager::maxTextFileSize()
            && mimeType.inherits("text/plain")) {
        const Utils::MimeType binary = Utils::mimeTypeForName("application/octet-stream");
        const EditorFactoryList binaryEditors = defaultEditorFactories(binary);
        if (!binaryEditors.isEmpty())
            factories_moveToFront(binaryEditors.first());
    }
    return factories;
}

/*!
    Creates an editor.

    Either override this in a subclass, or set the function to use for
    creating an editor instance with setEditorCreator().
*/
IEditor *IEditorFactory::createEditor() const
{
    QTC_ASSERT(m_creator, return nullptr);
    return m_creator();
}

/*!
    Sets the function that is used to create an editor instance in
    createEditor() by default to \a creator.
*/
void IEditorFactory::setEditorCreator(const std::function<IEditor *()> &creator)
{
    m_creator = creator;
}

/*!
    \internal
*/
QHash<Utils::MimeType, Core::IEditorFactory *> Core::Internal::userPreferredEditorFactories()
{
    return g_userPreferredEditorFactories;
}

/*!
    \internal
*/
void Internal::setUserPreferredEditorFactories(const QHash<Utils::MimeType, IEditorFactory *> &factories)
{
    g_userPreferredEditorFactories = factories;
}

} // Core
