/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
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
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "qmlchangeset.h"
#include "bindingproperty.h"
#include "variantproperty.h"
#include "qmlmodelview.h"
#include <metainfo.h>

namespace QmlDesigner {

ModelNode QmlModelStateOperation::target() const
{
    if (modelNode().property("target").isBindingProperty()) {
        return modelNode().bindingProperty("target").resolveToModelNode();
    } else {
        return ModelNode(); //exception?
    }
}

void QmlModelStateOperation::setTarget(const ModelNode &target)
{
    modelNode().bindingProperty("target").setExpression(target.id());
}

bool QmlPropertyChanges::isValid() const
{
    return QmlModelNodeFacade::isValid() && modelNode().metaInfo().isSubclassOf("Qt/PropertyChanges", 4, 6);
}

bool QmlModelStateOperation::isValid() const
{
    return QmlModelNodeFacade::isValid() && modelNode().metaInfo().isSubclassOf("Qt/QmlPropertyChanges", 4, 6);
}

void QmlPropertyChanges::removeProperty(const QString &name)
{
    RewriterTransaction transaction(qmlModelView()->beginRewriterTransaction());
    if (name == "name")
        return;
    modelNode().removeProperty(name);
    if (modelNode().variantProperties().isEmpty() && modelNode().bindingProperties().count() < 2)
        modelNode().destroy();
}

} //QmlDesigner
