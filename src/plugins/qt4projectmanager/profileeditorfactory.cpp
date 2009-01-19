/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008-2009 Nokia Corporation and/or its subsidiary(-ies).
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
** version 1.3, included in the file GPL_EXCEPTION.txt in this package.
**
***************************************************************************/

#include "profileeditorfactory.h"

#include "qt4projectmanager.h"
#include "qt4projectmanagerconstants.h"
#include "profileeditor.h"

#include <coreplugin/icore.h>
#include <coreplugin/fileiconprovider.h>
#include <coreplugin/editormanager/editormanager.h>
#include <extensionsystem/pluginmanager.h>
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
    iconProvider->registerIconForSuffix(QIcon(":/qt4projectmanager/images/qt_project.png"),
                                        QLatin1String("pro"));
    iconProvider->registerIconForSuffix(QIcon(":/qt4projectmanager/images/qt_project.png"),
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
    Core::ICore *core = ExtensionSystem::PluginManager::instance()->getObject<Core::ICore>();
    Core::IEditor *iface = core->editorManager()->openEditor(fileName, kind());
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
