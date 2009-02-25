/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
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
** If you are unsure which license is appropriate for your use, please
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#include "qtscripteditorfactory.h"
#include "qtscripteditor.h"
#include "qtscripteditoractionhandler.h"
#include "qtscripteditorconstants.h"
#include "qtscripteditorplugin.h"

#include <coreplugin/editormanager/editormanager.h>

#include <QtCore/QFileInfo>
#include <QtCore/QDebug>

using namespace QtScriptEditor::Internal;
using namespace QtScriptEditor::Constants;

QtScriptEditorFactory::QtScriptEditorFactory(const Context &context, QObject *parent)
  : Core::IEditorFactory(parent),
    m_kind(QLatin1String(C_QTSCRIPTEDITOR)),
    m_mimeTypes(QLatin1String(QtScriptEditor::Constants::C_QTSCRIPTEDITOR_MIMETYPE)),
    m_context(context),
    m_actionHandler(new QtScriptEditorActionHandler)
{
}

QtScriptEditorFactory::~QtScriptEditorFactory()
{
    delete m_actionHandler;
}

QString QtScriptEditorFactory::kind() const
{
    return m_kind;
}

Core::IFile *QtScriptEditorFactory::open(const QString &fileName)
{
    Core::IEditor *iface = Core::EditorManager::instance()->openEditor(fileName, kind());
    if (!iface) {
        qWarning() << "QtScriptEditorFactory::open: openEditor failed for " << fileName;
        return 0;
    }
    return iface->file();
}

Core::IEditor *QtScriptEditorFactory::createEditor(QWidget *parent)
{
    ScriptEditor *rc = new ScriptEditor(m_context, m_actionHandler, parent);
    QtScriptEditorPlugin::initializeEditor(rc);
    return rc->editableInterface();
}

QStringList QtScriptEditorFactory::mimeTypes() const
{
    return m_mimeTypes;
}
