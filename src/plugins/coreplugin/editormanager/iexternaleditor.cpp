// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "iexternaleditor.h"

#include "ieditorfactory_p.h"

namespace Core {

/*!
    \class Core::IExternalEditor
    \inheaderfile coreplugin/editormanager/iexternaleditor.h
    \inmodule QtCreator
    \ingroup mainclasses

    \brief The IExternalEditor class enables registering an external
    editor in the \uicontrol{Open With} dialog.
*/

/*!
    \fn bool Core::IExternalEditor::startEditor(const Utils::FilePath &fileName, QString *errorMessage)

    Opens the editor with \a fileName. Returns \c true on success or \c false
    on failure along with the error in \a errorMessage.
*/

static QList<IExternalEditor *> g_externalEditors;

/*!
    \internal
*/
IExternalEditor::IExternalEditor()
{
    g_externalEditors.append(this);
}

/*!
    \internal
*/
IExternalEditor::~IExternalEditor()
{
    g_externalEditors.removeOne(this);
}

/*!
    Returns all available external editors.
*/
const ExternalEditorList IExternalEditor::allExternalEditors()
{
    return g_externalEditors;
}

/*!
    Returns all external editors available for this \a mimeType in the default
    order (editors ordered by MIME type hierarchy).
*/
const ExternalEditorList IExternalEditor::externalEditors(const Utils::MimeType &mimeType)
{
    ExternalEditorList rc;
    const ExternalEditorList allEditors = IExternalEditor::allExternalEditors();
    Internal::mimeTypeFactoryLookup(mimeType, allEditors, &rc);
    return rc;
}

} // Core
