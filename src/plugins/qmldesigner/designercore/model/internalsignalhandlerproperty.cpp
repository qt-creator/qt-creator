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
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPLv3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
****************************************************************************/

#include "internalsignalhandlerproperty.h"

namespace QmlDesigner {
namespace Internal {

InternalSignalHandlerProperty::InternalSignalHandlerProperty(const PropertyName &name, const InternalNodePointer &propertyOwner)
    : InternalProperty(name, propertyOwner)
{
}


InternalSignalHandlerProperty::Pointer InternalSignalHandlerProperty::create(const PropertyName &name, const InternalNodePointer &propertyOwner)
{
    InternalSignalHandlerProperty *newPointer(new InternalSignalHandlerProperty(name, propertyOwner));
    InternalSignalHandlerProperty::Pointer smartPointer(newPointer);

    newPointer->setInternalWeakPointer(smartPointer);

    return smartPointer;
}

bool InternalSignalHandlerProperty::isValid() const
{
    return InternalProperty::isValid() && isSignalHandlerProperty();
}

QString InternalSignalHandlerProperty::source() const
{
    return m_source;
}
void InternalSignalHandlerProperty::setSource(const QString &source)
{
    m_source = source;
}

bool InternalSignalHandlerProperty::isSignalHandlerProperty() const
{
    return true;
}

} // namespace Internal
} // namespace QmlDesigner
