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

#include "basevcssubmiteditorfactory.h"
#include "vcsbasesubmiteditor.h"

#include <coreplugin/editormanager/editormanager.h>

namespace VcsBase {

namespace Internal {

class BaseVcsSubmitEditorFactoryPrivate
{
public:
    BaseVcsSubmitEditorFactoryPrivate(const VcsBaseSubmitEditorParameters *parameters);

    const VcsBaseSubmitEditorParameters *m_parameters;
    const Core::Id m_id;
    const QString m_displayName;
    const QStringList m_mimeTypes;
};

BaseVcsSubmitEditorFactoryPrivate::BaseVcsSubmitEditorFactoryPrivate(const VcsBaseSubmitEditorParameters *parameters) :
    m_parameters(parameters),
    m_id(QByteArray(parameters->id)),
    m_displayName(QLatin1String(parameters->displayName)),
    m_mimeTypes(QLatin1String(parameters->mimeType))
{
}

} // namespace Internal

BaseVcsSubmitEditorFactory::BaseVcsSubmitEditorFactory(const VcsBaseSubmitEditorParameters *parameters) :
    d(new Internal::BaseVcsSubmitEditorFactoryPrivate(parameters))
{
}

BaseVcsSubmitEditorFactory::~BaseVcsSubmitEditorFactory()
{
    delete d;
}

Core::IEditor *BaseVcsSubmitEditorFactory::createEditor(QWidget *parent)
{
    return createBaseSubmitEditor(d->m_parameters, parent);
}

Core::Id BaseVcsSubmitEditorFactory::id() const
{
    return d->m_id;
}

QString BaseVcsSubmitEditorFactory::displayName() const
{
    return d->m_displayName;
}


QStringList BaseVcsSubmitEditorFactory::mimeTypes() const
{
    return d->m_mimeTypes;
}

} // namespace VcsBase
