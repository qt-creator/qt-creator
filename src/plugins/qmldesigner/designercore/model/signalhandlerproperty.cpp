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

#include "signalhandlerproperty.h"
#include "internalproperty.h"
#include "invalidmodelnodeexception.h"
#include "invalidpropertyexception.h"
#include "invalidargumentexception.h"
#include "internalnode_p.h"
#include "model.h"
#include "model_p.h"
namespace QmlDesigner {

SignalHandlerProperty::SignalHandlerProperty()
{
}

SignalHandlerProperty::SignalHandlerProperty(const SignalHandlerProperty &property, AbstractView *view)
    : AbstractProperty(property.name(), property.internalNode(), property.model(), view)
{
}


SignalHandlerProperty::SignalHandlerProperty(const PropertyName &propertyName, const Internal::InternalNodePointer &internalNode, Model* model, AbstractView *view)
    : AbstractProperty(propertyName, internalNode, model, view)
{
}


void SignalHandlerProperty::setSource(const QString &source)
{
    Internal::WriteLocker locker(model());
    if (!isValid())
        throw InvalidModelNodeException(__LINE__, __FUNCTION__, __FILE__);

    if (name() == "id") { // the ID for a node is independent of the state, so it has to be set with ModelNode::setId
        throw InvalidPropertyException(__LINE__, __FUNCTION__, __FILE__, name());
    }

    if (source.isEmpty())
        throw InvalidArgumentException(__LINE__, __FUNCTION__, __FILE__, name());

    if (internalNode()->hasProperty(name())) { //check if oldValue != value
        Internal::InternalProperty::Pointer internalProperty = internalNode()->property(name());
        if (internalProperty->isSignalHandlerProperty()
            && internalProperty->toSignalHandlerProperty()->source() == source)

            return;
    }

    if (internalNode()->hasProperty(name()) && !internalNode()->property(name())->isSignalHandlerProperty())
        model()->d->removeProperty(internalNode()->property(name()));

    model()->d->setSignalHandlerProperty(internalNode(), name(), source);
}

QString SignalHandlerProperty::source() const
{
    if (internalNode()->hasProperty(name())
        && internalNode()->property(name())->isSignalHandlerProperty())
        return internalNode()->signalHandlerProperty(name())->source();

    return QString();
}

} // namespace QmlDesigner
