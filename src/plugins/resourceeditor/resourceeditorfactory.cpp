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

#include "resourceeditorfactory.h"
#include "resourceeditorw.h"
#include "resourceeditorplugin.h"
#include "resourceeditorconstants.h"

#include <coreplugin/fileiconprovider.h>
#include <coreplugin/editormanager/editormanager.h>

#include <QCoreApplication>
#include <QFileInfo>
#include <qdebug.h>

using namespace ResourceEditor::Internal;
using namespace ResourceEditor::Constants;

ResourceEditorFactory::ResourceEditorFactory(ResourceEditorPlugin *plugin) :
    Core::IEditorFactory(plugin),
    m_mimeTypes(QStringList(QLatin1String("application/vnd.nokia.xml.qt.resource"))),
    m_plugin(plugin)
{
    Core::FileIconProvider *iconProvider = Core::FileIconProvider::instance();
    iconProvider->registerIconOverlayForSuffix(QIcon(QLatin1String(":/resourceeditor/images/qt_qrc.png")),
                                               QLatin1String("qrc"));
}

Core::Id ResourceEditorFactory::id() const
{
    return Core::Id(RESOURCEEDITOR_ID);
}

QString ResourceEditorFactory::displayName() const
{
    return qApp->translate("OpenWith::Editors", C_RESOURCEEDITOR_DISPLAY_NAME);
}

Core::IEditor *ResourceEditorFactory::createEditor(QWidget *parent)
{
    Core::Context context(ResourceEditor::Constants::C_RESOURCEEDITOR);
    return new ResourceEditorW(context, m_plugin, parent);
}

QStringList ResourceEditorFactory::mimeTypes() const
{
    return m_mimeTypes;
}
