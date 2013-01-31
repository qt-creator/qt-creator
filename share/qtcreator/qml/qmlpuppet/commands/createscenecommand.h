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

#ifndef CREATESCENECOMMAND_H
#define CREATESCENECOMMAND_H

#include <qmetatype.h>
#include <QUrl>
#include <QVector>

#include "instancecontainer.h"
#include "reparentcontainer.h"
#include "idcontainer.h"
#include "propertyvaluecontainer.h"
#include "propertybindingcontainer.h"
#include "addimportcontainer.h"

namespace QmlDesigner {

class CreateSceneCommand
{
    friend QDataStream &operator>>(QDataStream &in, CreateSceneCommand &command);

public:
    CreateSceneCommand();
    CreateSceneCommand(const QVector<InstanceContainer> &instanceContainer,
                       const QVector<ReparentContainer> &reparentContainer,
                       const QVector<IdContainer> &idVector,
                       const QVector<PropertyValueContainer> &valueChangeVector,
                       const QVector<PropertyBindingContainer> &bindingChangeVector,
                       const QVector<PropertyValueContainer> &auxiliaryChangeVector,
                       const QVector<AddImportContainer> &importVector,
                       const QUrl &fileUrl);

    QVector<InstanceContainer> instances() const;
    QVector<ReparentContainer> reparentInstances() const;
    QVector<IdContainer> ids() const;
    QVector<PropertyValueContainer> valueChanges() const;
    QVector<PropertyBindingContainer> bindingChanges() const;
    QVector<PropertyValueContainer> auxiliaryChanges() const;
    QVector<AddImportContainer> imports() const;
    QUrl fileUrl() const;

private:
    QVector<InstanceContainer> m_instanceVector;
    QVector<ReparentContainer> m_reparentInstanceVector;
    QVector<IdContainer> m_idVector;
    QVector<PropertyValueContainer> m_valueChangeVector;
    QVector<PropertyBindingContainer> m_bindingChangeVector;
    QVector<PropertyValueContainer> m_auxiliaryChangeVector;
    QVector<AddImportContainer> m_importVector;
    QUrl m_fileUrl;
};

QDataStream &operator<<(QDataStream &out, const CreateSceneCommand &command);
QDataStream &operator>>(QDataStream &in, CreateSceneCommand &command);

}

Q_DECLARE_METATYPE(QmlDesigner::CreateSceneCommand)

#endif // CREATESCENECOMMAND_H
