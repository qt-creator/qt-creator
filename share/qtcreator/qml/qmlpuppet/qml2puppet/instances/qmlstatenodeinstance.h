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
#ifndef QMLSTATENODEINSTANCE_H
#define QMLSTATENODEINSTANCE_H

#include "objectnodeinstance.h"

namespace QmlDesigner {

namespace Internal {

class QmlStateNodeInstance : public ObjectNodeInstance
{
public:
    typedef QSharedPointer<QmlStateNodeInstance> Pointer;
    typedef QWeakPointer<QmlStateNodeInstance> WeakPointer;

    static Pointer create(QObject *objectToBeWrapped);

    void setPropertyVariant(const PropertyName &name, const QVariant &value) override;
    void setPropertyBinding(const PropertyName &name, const QString &expression) override;

    void activateState() override;
    void deactivateState() override;

    bool updateStateVariant(const ObjectNodeInstance::Pointer &target, const PropertyName &propertyName, const QVariant &value) override;
    bool updateStateBinding(const ObjectNodeInstance::Pointer &target, const PropertyName &propertyName, const QString &expression) override;
    bool resetStateProperty(const ObjectNodeInstance::Pointer &target, const PropertyName &propertyName, const QVariant &resetValue) override;


protected:
    QmlStateNodeInstance(QObject *object);
};

} // namespace Internal
} // namespace QmlDesigner

#endif // QMLSTATENODEINSTANCE_H
