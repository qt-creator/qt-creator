/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/
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
    QByteArray data(source.toUtf8() + "\n");
    data.prepend(nodeInstanceServer()->importCode());

    component()->setData(data, QUrl(nodeInstanceServer()->fileUrl().toString() +
                                    QLatin1Char('_')+ id()));
    setId(id());

    if (component()->isError()) {
        foreach (const QQmlError &error, component()->errors())
            qWarning() << error;
    }

}

} // Internal
} // QmlDesigner
