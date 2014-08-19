/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
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

#include "profileeditorfactory.h"

#include "qmakeprojectmanager.h"
#include "qmakeprojectmanagerconstants.h"
#include "profileeditor.h"

#include <qtsupport/qtsupportconstants.h>
#include <coreplugin/fileiconprovider.h>
#include <texteditor/texteditoractionhandler.h>

#include <QCoreApplication>

using namespace QmakeProjectManager;
using namespace QmakeProjectManager::Internal;

ProFileEditorFactory::ProFileEditorFactory(QmakeManager *manager) :
    m_manager(manager)
{
    setId(QmakeProjectManager::Constants::PROFILE_EDITOR_ID);
    setDisplayName(qApp->translate("OpenWith::Editors", QmakeProjectManager::Constants::PROFILE_EDITOR_DISPLAY_NAME));
    addMimeType(QmakeProjectManager::Constants::PROFILE_MIMETYPE);
    addMimeType(QmakeProjectManager::Constants::PROINCLUDEFILE_MIMETYPE);
    addMimeType(QmakeProjectManager::Constants::PROFEATUREFILE_MIMETYPE);
    addMimeType(QmakeProjectManager::Constants::PROCONFIGURATIONFILE_MIMETYPE);
    addMimeType(QmakeProjectManager::Constants::PROCACHEFILE_MIMETYPE);
    addMimeType(QmakeProjectManager::Constants::PROSTASHFILE_MIMETYPE);
    new TextEditor::TextEditorActionHandler(this, Constants::C_PROFILEEDITOR,
                  TextEditor::TextEditorActionHandler::UnCommentSelection
                  | TextEditor::TextEditorActionHandler::JumpToFileUnderCursor);

    Core::FileIconProvider::registerIconOverlayForSuffix(QtSupport::Constants::ICON_QT_PROJECT, "pro");
    Core::FileIconProvider::registerIconOverlayForSuffix(QtSupport::Constants::ICON_QT_PROJECT, "pri");
    Core::FileIconProvider::registerIconOverlayForSuffix(QtSupport::Constants::ICON_QT_PROJECT, "prf");
}

Core::IEditor *ProFileEditorFactory::createEditor()
{
    ProFileEditorWidget *editor = new ProFileEditorWidget;
    editor->setTextDocument(TextEditor::BaseTextDocumentPtr(new ProFileDocument));
    return editor->editor();
}
