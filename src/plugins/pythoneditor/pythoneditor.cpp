/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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

/**
  \class PyEditor::Editor implements interface Core::IEditor
  This editor makes possible to edit Python source files
  */

#include "pythoneditor.h"
#include "pythoneditorconstants.h"
#include "pythoneditorplugin.h"
#include "pythoneditorwidget.h"

#include <coreplugin/icore.h>
#include <coreplugin/mimedatabase.h>
#include <texteditor/texteditorconstants.h>
#include <texteditor/texteditorsettings.h>

#include <QFileInfo>

namespace PythonEditor {

PythonEditor::PythonEditor(EditorWidget *editorWidget)
    :BaseTextEditor(editorWidget)
{
    setContext(Core::Context(Constants::C_PYTHONEDITOR_ID,
                             TextEditor::Constants::C_TEXTEDITOR));
}

PythonEditor::~PythonEditor()
{
}

Core::IEditor *PythonEditor::duplicate(QWidget *parent)
{
    EditorWidget *widget = new EditorWidget(parent);
    widget->duplicateFrom(editorWidget());
    PythonEditorPlugin::initializeEditor(widget);

    return widget->editor();
}

/**
 * @returns Unique editor class identifier, that is Constants::C_PYTHONEDITOR_ID
 */
Core::Id PythonEditor::id() const
{
    return Core::Id(Constants::C_PYTHONEDITOR_ID);
}

bool PythonEditor::open(QString *errorString,
                        const QString &fileName,
                        const QString &realFileName)
{
    Core::MimeType mimeType;
    Core::MimeDatabase *mimeDB = Core::ICore::instance()->mimeDatabase();

    mimeType = mimeDB->findByFile(QFileInfo(fileName));
    editorWidget()->setMimeType(mimeType.type());

    bool status = TextEditor::BaseTextEditor::open(errorString,
                                                   fileName,
                                                   realFileName);
    return status;
}

} // namespace PythonEditor
