/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
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
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "resourceeditorfactory.h"
#include "resourceeditorw.h"
#include "resourceeditorplugin.h"
#include "resourceeditorconstants.h"

#include <coreplugin/uniqueidmanager.h>
#include <coreplugin/fileiconprovider.h>
#include <coreplugin/editormanager/editormanager.h>

#include <QtCore/QFileInfo>
#include <QtCore/qdebug.h>

using namespace ResourceEditor::Internal;
using namespace ResourceEditor::Constants;

ResourceEditorFactory::ResourceEditorFactory(ResourceEditorPlugin *plugin) :
    Core::IEditorFactory(plugin),
    m_mimeTypes(QStringList(QLatin1String("application/vnd.nokia.xml.qt.resource"))),
    m_plugin(plugin)
{
    m_context += Core::UniqueIDManager::instance()
                 ->uniqueIdentifier(QLatin1String(ResourceEditor::Constants::C_RESOURCEEDITOR_ID));
    Core::FileIconProvider *iconProvider = Core::FileIconProvider::instance();
    iconProvider->registerIconOverlayForSuffix(QIcon(":/resourceeditor/images/qt_qrc.png"),
                                               QLatin1String("qrc"));
}

QString ResourceEditorFactory::id() const
{
    return QLatin1String(C_RESOURCEEDITOR_ID);
}

QString ResourceEditorFactory::displayName() const
{
    return tr(C_RESOURCEEDITOR_DISPLAY_NAME);
}

Core::IFile *ResourceEditorFactory::open(const QString &fileName)
{
    Core::IEditor *iface = Core::EditorManager::instance()->openEditor(fileName, id());
    if (!iface) {
        qWarning() << "ResourceEditorFactory::open: openEditor failed for " << fileName;
        return 0;
    }
    return iface->file();
}

Core::IEditor *ResourceEditorFactory::createEditor(QWidget *parent)
{
    return new ResourceEditorW(m_context, m_plugin, parent);
}

QStringList ResourceEditorFactory::mimeTypes() const
{
    return m_mimeTypes;
}
