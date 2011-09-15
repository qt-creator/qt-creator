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
** Nokia at info@qt.nokia.com.
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
    const Core::Id m_id;
    const QString m_displayName;
    const QStringList m_mimeTypes;
};

BaseVCSSubmitEditorFactoryPrivate::BaseVCSSubmitEditorFactoryPrivate(const VCSBaseSubmitEditorParameters *parameters) :
    m_parameters(parameters),
    m_id(parameters->id),
    m_displayName(parameters->displayName),
    m_mimeTypes(QLatin1String(parameters->mimeType))
{
}

BaseVCSSubmitEditorFactory::BaseVCSSubmitEditorFactory(const VCSBaseSubmitEditorParameters *parameters) :
    d(new BaseVCSSubmitEditorFactoryPrivate(parameters))
{
}

BaseVCSSubmitEditorFactory::~BaseVCSSubmitEditorFactory()
{
    delete d;
}

Core::IEditor *BaseVCSSubmitEditorFactory::createEditor(QWidget *parent)
{
    return createBaseSubmitEditor(d->m_parameters, parent);
}

Core::Id BaseVCSSubmitEditorFactory::id() const
{
    return d->m_id;
}

QString BaseVCSSubmitEditorFactory::displayName() const
{
    return d->m_displayName;
}


QStringList BaseVCSSubmitEditorFactory::mimeTypes() const
{
    return d->m_mimeTypes;
}

Core::IFile *BaseVCSSubmitEditorFactory::open(const QString &fileName)
{
    if (Core::IEditor *iface = Core::EditorManager::instance()->openEditor(fileName, id()))
        return iface->file();
    return 0;
}

} // namespace VCSBase
