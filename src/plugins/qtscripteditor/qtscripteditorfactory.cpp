/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
**
** Non-Open Source Usage
**
** Licensees may use this file in accordance with the Qt Beta Version
** License Agreement, Agreement version 2.2 provided with the Software or,
** alternatively, in accordance with the terms contained in a written
** agreement between you and Nokia.
**
** GNU General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU General
** Public License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the packaging
** of this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
**
** http://www.fsf.org/licensing/licenses/info/GPLv2.html and
** http://www.gnu.org/copyleft/gpl.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt GPL Exception
** version 1.2, included in the file GPL_EXCEPTION.txt in this package.
**
***************************************************************************/
#include "qtscripteditorfactory.h"
#include "qtscripteditor.h"
#include "qtscripteditoractionhandler.h"
#include "qtscripteditorconstants.h"
#include "qtscripteditorplugin.h"

#include <coreplugin/editormanager/editormanager.h>

#include <QtCore/QFileInfo>
#include <QtCore/qdebug.h>

using namespace QtScriptEditor::Internal;
using namespace QtScriptEditor::Constants;

QtScriptEditorFactory::QtScriptEditorFactory(Core::ICore *core,
                                             const Context &context,
                                             QObject *parent) :
    Core::IEditorFactory(parent),
    m_kind(QLatin1String(C_QTSCRIPTEDITOR)),
    m_mimeTypes(QLatin1String(QtScriptEditor::Constants::C_QTSCRIPTEDITOR_MIMETYPE)),
    m_context(context),
    m_core(core),
    m_actionHandler(new QtScriptEditorActionHandler(core))
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
    Core::IEditor *iface = m_core->editorManager()->openEditor(fileName, kind());
    if (!iface) {
        qWarning() << "QtScriptEditorFactory::open: openEditor failed for " << fileName;
        return 0;
    }
    return iface->file();
}

Core::IEditor *QtScriptEditorFactory::createEditor(QWidget *parent)
{
    ScriptEditor *rc =  new ScriptEditor(m_context, m_core, m_actionHandler, parent);
    QtScriptEditorPlugin::initializeEditor(rc);
    return rc->editableInterface();
}

QStringList QtScriptEditorFactory::mimeTypes() const
{
    return m_mimeTypes;
}
