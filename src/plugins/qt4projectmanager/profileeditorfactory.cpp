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

#include "profileeditorfactory.h"

#include "qt4projectmanager.h"
#include "qt4projectmanagerconstants.h"
#include "profileeditor.h"

#include <coreplugin/fileiconprovider.h>
#include <coreplugin/editormanager/editormanager.h>
#include <texteditor/texteditoractionhandler.h>
#include <texteditor/texteditorsettings.h>
#include <texteditor/completionsupport.h>

#include <QtCore/QFileInfo>
#include <QtGui/QAction>
#include <QtGui/QMenu>

using namespace Qt4ProjectManager;
using namespace Qt4ProjectManager::Internal;

ProFileEditorFactory::ProFileEditorFactory(Qt4Manager *manager, TextEditor::TextEditorActionHandler *handler) :
    m_mimeTypes(QStringList() << QLatin1String(Qt4ProjectManager::Constants::PROFILE_MIMETYPE)
                << QLatin1String(Qt4ProjectManager::Constants::PROINCLUDEFILE_MIMETYPE)
                << QLatin1String(Qt4ProjectManager::Constants::PROFEATUREFILE_MIMETYPE)),
    m_manager(manager),
    m_actionHandler(handler)
{
    Core::FileIconProvider *iconProvider = Core::FileIconProvider::instance();
    iconProvider->registerIconOverlayForSuffix(QIcon(QLatin1String(Constants::ICON_QT_PROJECT)),
                                        QLatin1String("pro"));
    iconProvider->registerIconOverlayForSuffix(QIcon(QLatin1String(Constants::ICON_QT_PROJECT)),
                                        QLatin1String("pri"));
    iconProvider->registerIconOverlayForSuffix(QIcon(QLatin1String(Constants::ICON_QT_PROJECT)),
                                        QLatin1String("prf"));
}

ProFileEditorFactory::~ProFileEditorFactory()
{
}

QString ProFileEditorFactory::id() const
{
    return QLatin1String(Qt4ProjectManager::Constants::PROFILE_EDITOR_ID);
}

QString ProFileEditorFactory::displayName() const
{
    return tr(Qt4ProjectManager::Constants::PROFILE_EDITOR_DISPLAY_NAME);
}

Core::IFile *ProFileEditorFactory::open(const QString &fileName)
{
    Core::IEditor *iface = Core::EditorManager::instance()->openEditor(fileName, id());
    return iface ? iface->file() : 0;
}

Core::IEditor *ProFileEditorFactory::createEditor(QWidget *parent)
{
    ProFileEditorWidget *editor = new ProFileEditorWidget(parent, this, m_actionHandler);
    TextEditor::TextEditorSettings::instance()->initializeEditor(editor);
    return editor->editor();
}

QStringList ProFileEditorFactory::mimeTypes() const
{
    return m_mimeTypes;
}
