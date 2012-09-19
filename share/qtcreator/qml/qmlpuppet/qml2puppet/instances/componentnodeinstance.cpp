/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
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
**
**************************************************************************/

#include "componentnodeinstance.h"

#include <QQmlComponent>
#include <QQmlContext>

#include <QDebug>

namespace QmlDesigner {
namespace Internal {


ComponentNodeInstance::ComponentNodeInstance(QQmlComponent *component)
   : ObjectNodeInstance(component)
{
}

QQmlComponent *ComponentNodeInstance::component() const
{
    Q_ASSERT(qobject_cast<QQmlComponent*>(object()));
    return static_cast<QQmlComponent*>(object());
}

ComponentNodeInstance::Pointer ComponentNodeInstance::create(QObject  *object)
{
    QQmlComponent *component = qobject_cast<QQmlComponent *>(object);

    Q_ASSERT(component);

    Pointer instance(new ComponentNodeInstance(component));

    instance->populateResetHashes();

    return instance;
}

bool ComponentNodeInstance::hasContent() const
{
    return true;
}

void ComponentNodeInstance::setNodeSource(const QString &source)
{
    QByteArray importArray;
    foreach (const QString &import, nodeInstanceServer()->imports()) {
        importArray.append(import.toUtf8());
    }

    QByteArray data(source.toUtf8());

    data.prepend(importArray);
    data.append("\n");

    component()->setData(data, QUrl(nodeInstanceServer()->fileUrl().toString() +
                                    QLatin1Char('_')+ id()));
    setId(id());

    if (component()->isError()) {
        foreach (const QQmlError &error, component()->errors())
            qDebug() << error;
    }

}

} // Internal
} // QmlDesigner
