/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "glslhoverhandler.h"
#include "glsleditor.h"

#include <coreplugin/editormanager/ieditor.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/helpmanager.h>
#include <extensionsystem/pluginmanager.h>
#include <texteditor/itexteditor.h>
#include <texteditor/basetexteditor.h>

#include <QtGui/QTextCursor>
#include <QtCore/QUrl>

using namespace GLSLEditor;
using namespace GLSLEditor::Internal;
using namespace Core;

GLSLHoverHandler::GLSLHoverHandler(QObject *parent) : BaseHoverHandler(parent)
{}

GLSLHoverHandler::~GLSLHoverHandler()
{}

bool GLSLHoverHandler::acceptEditor(IEditor *editor)
{
    if (qobject_cast<GLSLEditorEditable *>(editor) != 0)
        return true;
    return false;
}

void GLSLHoverHandler::identifyMatch(TextEditor::ITextEditor *editor, int pos)
{
    if (GLSLTextEditorWidget *glslEditor = qobject_cast<GLSLTextEditorWidget *>(editor->widget())) {
        if (! glslEditor->extraSelectionTooltip(pos).isEmpty()) {
            setToolTip(glslEditor->extraSelectionTooltip(pos));
        }
    }
}

void GLSLHoverHandler::decorateToolTip()
{
    if (Qt::mightBeRichText(toolTip()))
        setToolTip(Qt::escape(toolTip()));
}
