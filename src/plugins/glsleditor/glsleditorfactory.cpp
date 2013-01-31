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

#include "glsleditorfactory.h"
#include "glsleditoreditable.h"
#include "glsleditor.h"
#include "glsleditoractionhandler.h"
#include "glsleditorconstants.h"
#include "glsleditorplugin.h"

#include <extensionsystem/pluginmanager.h>
#include <extensionsystem/pluginspec.h>

#include <coreplugin/icore.h>
#include <coreplugin/editormanager/editormanager.h>

#include <QCoreApplication>
#include <QFileInfo>
#include <QDebug>
#include <QSettings>
#include <QMessageBox>
#include <QPushButton>

using namespace GLSLEditor::Internal;
using namespace GLSLEditor::Constants;

GLSLEditorFactory::GLSLEditorFactory(QObject *parent)
  : Core::IEditorFactory(parent)
{
    m_mimeTypes
            << QLatin1String(GLSLEditor::Constants::GLSL_MIMETYPE)
            << QLatin1String(GLSLEditor::Constants::GLSL_MIMETYPE_VERT)
            << QLatin1String(GLSLEditor::Constants::GLSL_MIMETYPE_FRAG)
            << QLatin1String(GLSLEditor::Constants::GLSL_MIMETYPE_VERT_ES)
            << QLatin1String(GLSLEditor::Constants::GLSL_MIMETYPE_FRAG_ES)
            ;
}

Core::Id GLSLEditorFactory::id() const
{
    return Core::Id(C_GLSLEDITOR_ID);
}

QString GLSLEditorFactory::displayName() const
{
    return qApp->translate("OpenWith::Editors", C_GLSLEDITOR_DISPLAY_NAME);
}

Core::IEditor *GLSLEditorFactory::createEditor(QWidget *parent)
{
    GLSLEditor::GLSLTextEditorWidget *rc = new GLSLEditor::GLSLTextEditorWidget(parent);
    GLSLEditorPlugin::instance()->initializeEditor(rc);
    return rc->editor();
}

QStringList GLSLEditorFactory::mimeTypes() const
{
    return m_mimeTypes;
}

void GLSLEditorFactory::updateEditorInfoBar(Core::IEditor *)
{
}
