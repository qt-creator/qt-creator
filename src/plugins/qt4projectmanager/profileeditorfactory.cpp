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

#include "profileeditorfactory.h"

#include "qt4projectmanager.h"
#include "qt4projectmanagerconstants.h"
#include "profileeditor.h"

#include <coreplugin/fileiconprovider.h>
#include <coreplugin/editormanager/editormanager.h>
#include <texteditor/texteditoractionhandler.h>

#include <QtCore/QFileInfo>
#include <QtGui/QAction>
#include <QtGui/QMenu>

using namespace Qt4ProjectManager;
using namespace Qt4ProjectManager::Internal;

ProFileEditorFactory::ProFileEditorFactory(Qt4Manager *manager, TextEditor::TextEditorActionHandler *handler) :
    m_kind(QLatin1String(Qt4ProjectManager::Constants::PROFILE_EDITOR)),
    m_mimeTypes(QStringList() << QLatin1String(Qt4ProjectManager::Constants::PROFILE_MIMETYPE)
                << QLatin1String(Qt4ProjectManager::Constants::PROINCLUDEFILE_MIMETYPE)),
    m_manager(manager),
    m_actionHandler(handler)
{
    Core::FileIconProvider *iconProvider = Core::FileIconProvider::instance();
    iconProvider->registerIconOverlayForSuffix(QIcon(":/qt4projectmanager/images/qt_project.png"),
                                        QLatin1String("pro"));
    iconProvider->registerIconOverlayForSuffix(QIcon(":/qt4projectmanager/images/qt_project.png"),
                                        QLatin1String("pri"));
}

ProFileEditorFactory::~ProFileEditorFactory()
{
}

QString ProFileEditorFactory::kind() const
{
    return m_kind;
}

Core::IFile *ProFileEditorFactory::open(const QString &fileName)
{
    Core::IEditor *iface = Core::EditorManager::instance()->openEditor(fileName, kind());
    return iface ? iface->file() : 0;
}

Core::IEditor *ProFileEditorFactory::createEditor(QWidget *parent)
{
    ProFileEditor *rc = new ProFileEditor(parent, this, m_actionHandler);
    rc->initialize();
    return rc->editableInterface();
}

QStringList ProFileEditorFactory::mimeTypes() const
{
    return m_mimeTypes;
}

void ProFileEditorFactory::initializeActions()
{
    m_actionHandler->initializeActions();
}
