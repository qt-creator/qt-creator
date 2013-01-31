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



#include "anchorchangesnodeinstance.h"

namespace QmlDesigner {

namespace Internal {

AnchorChangesNodeInstance::AnchorChangesNodeInstance(QObject *object) :
        ObjectNodeInstance(object)
{
}

AnchorChangesNodeInstance::Pointer AnchorChangesNodeInstance::create(QObject *object)
{
    Q_ASSERT(object);

    Pointer instance(new AnchorChangesNodeInstance(object));

    return instance;
}

void AnchorChangesNodeInstance::setPropertyVariant(const QString &/*name*/, const QVariant &/*value*/)
{
}

void AnchorChangesNodeInstance::setPropertyBinding(const QString &/*name*/, const QString &/*expression*/)
{
}

QVariant AnchorChangesNodeInstance::property(const QString &/*name*/) const
{
    return QVariant();
}

void AnchorChangesNodeInstance::resetProperty(const QString &/*name*/)
{
}


void AnchorChangesNodeInstance::reparent(const ServerNodeInstance &/*oldParentInstance*/,
                                         const QString &/*oldParentProperty*/,
                                         const ServerNodeInstance &/*newParentInstance*/,
                                         const QString &/*newParentProperty*/)
{
}

QObject *AnchorChangesNodeInstance::changesObject() const
{
    return object();
}

} // namespace Internal

} // namespace QmlDesigner
