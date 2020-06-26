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
    \fn QString Core::IExternalEditor::displayName() const
    Returns a user-visible description of the editor type.
*/

/*!
    \fn Utils::Id Core::IExternalEditor::id() const
    Returns the ID of the factory or editor type.
*/

/*!
    \fn QStringList Core::IExternalEditor::mimeTypes() const
    Returns a list of MIME types that the editor supports
*/

/*!

    \fn bool Core::IExternalEditor::startEditor(const QString &fileName, QString *errorMessage) = 0;

    Opens the editor with \a fileName. Returns \c true on success or \c false
    on failure along with the error in \a errorMessage.
*/

static QList<IExternalEditor *> g_externalEditors;

/*!
    \internal
*/
IExternalEditor::IExternalEditor(QObject *parent)
    : QObject(parent)
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
