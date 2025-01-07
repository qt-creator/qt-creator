// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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
