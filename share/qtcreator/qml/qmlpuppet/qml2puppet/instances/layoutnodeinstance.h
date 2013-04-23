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

#ifndef LAYOUTNODEINSTANCE_H
#define LAYOUTNODEINSTANCE_H

#include "quickitemnodeinstance.h"

namespace QmlDesigner {
namespace Internal {

class LayoutNodeInstance : public QuickItemNodeInstance
{

public:
    typedef QSharedPointer<LayoutNodeInstance> Pointer;
    typedef QWeakPointer<LayoutNodeInstance> WeakPointer;

    static Pointer create(QObject *objectToBeWrapped);

    void setPropertyVariant(const PropertyName &name, const QVariant &value) Q_DECL_OVERRIDE;
    void setPropertyBinding(const PropertyName &name, const QString &expression) Q_DECL_OVERRIDE;

    bool isLayoutable() const Q_DECL_OVERRIDE;

    bool isResizable() const Q_DECL_OVERRIDE;

    void refreshLayoutable() Q_DECL_OVERRIDE;

protected:
    LayoutNodeInstance(QQuickItem *item);
};

} // namespace Internal
} // namespace QmlDesigner

#endif // LAYOUTNODEINSTANCE_H
