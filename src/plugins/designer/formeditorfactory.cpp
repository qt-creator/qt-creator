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

#include "formeditorfactory.h"
#include "formeditorw.h"
#include "formwindoweditor.h"
#include "designerconstants.h"

#include <coreplugin/icore.h>
#include <coreplugin/fileiconprovider.h>
#include <coreplugin/editormanager/editormanager.h>

#include <QtCore/QFileInfo>
#include <QtCore/QDebug>

using namespace Designer::Internal;
using namespace Designer::Constants;

FormEditorFactory::FormEditorFactory()
  : Core::IEditorFactory(Core::ICore::instance()),
    m_kind(QLatin1String(C_FORMEDITOR)),
    m_mimeTypes(QLatin1String(FORM_MIMETYPE))
{
    Core::FileIconProvider *iconProvider = Core::FileIconProvider::instance();
    iconProvider->registerIconOverlayForSuffix(QIcon(":/formeditor/images/qt_ui.png"),
                                        QLatin1String("ui"));
}

QString FormEditorFactory::kind() const
{
    return C_FORMEDITOR;
}

Core::IFile *FormEditorFactory::open(const QString &fileName)
{
    Core::IEditor *iface = Core::EditorManager::instance()->openEditor(fileName, kind());
    return iface ? iface->file() : 0;
}

Core::IEditor *FormEditorFactory::createEditor(QWidget *parent)
{
    return FormEditorW::instance()->createFormWindowEditor(parent);
}

QStringList FormEditorFactory::mimeTypes() const
{
    return m_mimeTypes;
}
