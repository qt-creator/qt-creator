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

#include "basevcssubmiteditorfactory.h"
#include "vcsbasesubmiteditor.h"

#include <coreplugin/editormanager/editormanager.h>

namespace VCSBase {

struct BaseVCSSubmitEditorFactoryPrivate
{
    BaseVCSSubmitEditorFactoryPrivate(const VCSBaseSubmitEditorParameters *parameters);

    const VCSBaseSubmitEditorParameters *m_parameters;
    const QString m_kind;
    const QStringList m_mimeTypes;
};

BaseVCSSubmitEditorFactoryPrivate::BaseVCSSubmitEditorFactoryPrivate(const VCSBaseSubmitEditorParameters *parameters) :
    m_parameters(parameters),
    m_kind(QLatin1String(parameters->kind)),
    m_mimeTypes(QLatin1String(parameters->mimeType))
{
}

BaseVCSSubmitEditorFactory::BaseVCSSubmitEditorFactory(const VCSBaseSubmitEditorParameters *parameters) :
    m_d(new BaseVCSSubmitEditorFactoryPrivate(parameters))
{
}

BaseVCSSubmitEditorFactory::~BaseVCSSubmitEditorFactory()
{
    delete m_d;
}

Core::IEditor *BaseVCSSubmitEditorFactory::createEditor(QWidget *parent)
{
    return createBaseSubmitEditor(m_d->m_parameters, parent);
}

QString BaseVCSSubmitEditorFactory::kind() const
{
    return m_d->m_kind;
}

QStringList BaseVCSSubmitEditorFactory::mimeTypes() const
{
    return m_d->m_mimeTypes;
}

Core::IFile *BaseVCSSubmitEditorFactory::open(const QString &fileName)
{
    if (Core::IEditor *iface = Core::EditorManager::instance()->openEditor(fileName, kind()))
        return iface->file();
    return 0;
}

} // namespace VCSBase
