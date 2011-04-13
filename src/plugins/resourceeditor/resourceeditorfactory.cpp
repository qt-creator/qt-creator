/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "resourceeditorfactory.h"
#include "resourceeditorw.h"
#include "resourceeditorplugin.h"
#include "resourceeditorconstants.h"

#include <coreplugin/fileiconprovider.h>
#include <coreplugin/editormanager/editormanager.h>

#include <QtCore/QFileInfo>
#include <QtCore/qdebug.h>

using namespace ResourceEditor::Internal;
using namespace ResourceEditor::Constants;

ResourceEditorFactory::ResourceEditorFactory(ResourceEditorPlugin *plugin) :
    Core::IEditorFactory(plugin),
    m_mimeTypes(QStringList(QLatin1String("application/vnd.nokia.xml.qt.resource"))),
    m_context(ResourceEditor::Constants::C_RESOURCEEDITOR),
    m_plugin(plugin)
{
    Core::FileIconProvider *iconProvider = Core::FileIconProvider::instance();
    iconProvider->registerIconOverlayForSuffix(QIcon(QLatin1String(":/resourceeditor/images/qt_qrc.png")),
                                               QLatin1String("qrc"));
}

QString ResourceEditorFactory::id() const
{
    return QLatin1String(RESOURCEEDITOR_ID);
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
