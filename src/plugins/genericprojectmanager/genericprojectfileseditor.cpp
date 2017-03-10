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

#include "genericprojectfileseditor.h"
#include "genericprojectconstants.h"

#include <coreplugin/editormanager/editormanager.h>
#include <texteditor/texteditoractionhandler.h>
#include <texteditor/textdocument.h>

#include <QCoreApplication>

using namespace TextEditor;

namespace GenericProjectManager {
namespace Internal {

//
// ProjectFilesFactory
//

ProjectFilesFactory::ProjectFilesFactory()
{
    setId(Constants::FILES_EDITOR_ID);
    setDisplayName(QCoreApplication::translate("OpenWith::Editors", ".files Editor"));
    addMimeType("application/vnd.qtcreator.generic.files");
    addMimeType("application/vnd.qtcreator.generic.includes");
    addMimeType("application/vnd.qtcreator.generic.config");

    setDocumentCreator([]() { return new TextDocument(Constants::FILES_EDITOR_ID); });
    setEditorActionHandlers(TextEditorActionHandler::None);
}

} // namespace Internal
} // namespace GenericProjectManager
