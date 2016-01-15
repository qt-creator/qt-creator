/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
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
