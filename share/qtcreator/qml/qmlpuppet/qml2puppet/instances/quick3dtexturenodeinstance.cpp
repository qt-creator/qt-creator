/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
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

#include "quick3dtexturenodeinstance.h"
#include <QQuickItem>
#include <QTimer>

namespace QmlDesigner {
namespace Internal {

Quick3DTextureNodeInstance::Quick3DTextureNodeInstance(QObject *object)
    : ObjectNodeInstance(object)
{
}

Quick3DTextureNodeInstance::Pointer Quick3DTextureNodeInstance::create(QObject *object)
{
    Pointer instance(new Quick3DTextureNodeInstance(object));

    QTimer::singleShot(0, [object](){
        // Texture will be upside down for unknown reason unless we force flip update at start
        QVariant prop = object->property("flipV");
        QVariant prop2(!prop.toBool());
        object->setProperty("flipV", prop2);
        object->setProperty("flipV", prop);
    });

    instance->populateResetHashes();
    return instance;
}

void Quick3DTextureNodeInstance::setPropertyBinding(const PropertyName &name,
                                                    const QString &expression)
{
    ObjectNodeInstance::setPropertyBinding(name, expression);

    if (name == "sourceItem") {
        bool targetNeed = true;
        if (expression.isEmpty())
            targetNeed = false;
        if (targetNeed != m_multiPassNeeded) {
            m_multiPassNeeded = targetNeed;
            if (targetNeed)
                nodeInstanceServer()->incrementNeedsExtraRender();
            else
                nodeInstanceServer()->decrementNeedsExtraRender();
        }
    }
}

void Quick3DTextureNodeInstance::resetProperty(const PropertyName &name)
{
    ObjectNodeInstance::resetProperty(name);

    if (name == "sourceItem") {
        if (m_multiPassNeeded) {
            m_multiPassNeeded = false;
            nodeInstanceServer()->decrementNeedsExtraRender();
        }
    }
}

} // namespace Internal
} // namespace QmlDesigner
