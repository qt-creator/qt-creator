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

#include "glsleditorfactory.h"
#include "glsleditoreditable.h"
#include "glsleditor.h"
#include "glsleditorconstants.h"
#include "glsleditorplugin.h"
#include "glslindenter.h"

#include <extensionsystem/pluginmanager.h>
#include <extensionsystem/pluginspec.h>

#include <coreplugin/icore.h>
#include <texteditor/texteditoractionhandler.h>
#include <texteditor/texteditorsettings.h>

#include <QCoreApplication>
#include <QSettings>

using namespace GLSLEditor::Internal;
using namespace GLSLEditor::Constants;

GLSLEditorFactory::GLSLEditorFactory(QObject *parent)
  : Core::IEditorFactory(parent)
{
    setId(C_GLSLEDITOR_ID);
    setDisplayName(qApp->translate("OpenWith::Editors", C_GLSLEDITOR_DISPLAY_NAME));
    addMimeType(GLSLEditor::Constants::GLSL_MIMETYPE);
    addMimeType(GLSLEditor::Constants::GLSL_MIMETYPE_VERT);
    addMimeType(GLSLEditor::Constants::GLSL_MIMETYPE_FRAG);
    addMimeType(GLSLEditor::Constants::GLSL_MIMETYPE_VERT_ES);
    addMimeType(GLSLEditor::Constants::GLSL_MIMETYPE_FRAG_ES);
    new TextEditor::TextEditorActionHandler(this, Constants::C_GLSLEDITOR_ID,
                                TextEditor::TextEditorActionHandler::Format
                                | TextEditor::TextEditorActionHandler::UnCommentSelection
                                | TextEditor::TextEditorActionHandler::UnCollapseAll);

}

Core::IEditor *GLSLEditorFactory::createEditor()
{
    auto doc = new TextEditor::BaseTextDocument;
    doc->setId(C_GLSLEDITOR_ID);
    doc->setIndenter(new GLSLIndenter);
    GlslEditorWidget *rc = new GlslEditorWidget(doc, 0);
    TextEditor::TextEditorSettings::initializeEditor(rc);
    return rc->editor();
}
