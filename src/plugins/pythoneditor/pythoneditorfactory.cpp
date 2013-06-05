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

#include "pythoneditorfactory.h"
#include "pythoneditorconstants.h"
#include "pythoneditorwidget.h"
#include "pythoneditorplugin.h"

#include <coreplugin/icore.h>
#include <coreplugin/editormanager/editormanager.h>
#include <texteditor/texteditorsettings.h>

#include <QDebug>

namespace PythonEditor {

EditorFactory::EditorFactory(QObject *parent)
    : Core::IEditorFactory(parent)
{
    m_mimeTypes << QLatin1String(Constants::C_PY_MIMETYPE);
}

Core::Id EditorFactory::id() const
{
    return Constants::C_PYTHONEDITOR_ID;
}

QString EditorFactory::displayName() const
{
    return tr(Constants::C_EDITOR_DISPLAY_NAME);
}

Core::IEditor *EditorFactory::createEditor(QWidget *parent)
{
    EditorWidget *widget = new EditorWidget(parent);
    PythonEditorPlugin::initializeEditor(widget);

    return widget->editor();
}

QStringList EditorFactory::mimeTypes() const
{
    return m_mimeTypes;
}

} // namespace PythonEditor
