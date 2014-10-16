/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://www.qt.io/licensing.  For further information
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
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef QT5NODEINSTANCESERVER_H
#define QT5NODEINSTANCESERVER_H

#include <QtGlobal>

#include "nodeinstanceserver.h"
#include "designersupport.h"

QT_BEGIN_NAMESPACE
class QQuickItem;
QT_END_NAMESPACE

namespace QmlDesigner {

class Qt5NodeInstanceServer : public NodeInstanceServer
{
    Q_OBJECT
public:
    Qt5NodeInstanceServer(NodeInstanceClientInterface *nodeInstanceClient);
    ~Qt5NodeInstanceServer();

    QQuickView *quickView() const Q_DECL_OVERRIDE;
    QQmlView *declarativeView() const Q_DECL_OVERRIDE;
    QQmlEngine *engine() const Q_DECL_OVERRIDE;
    void refreshBindings() Q_DECL_OVERRIDE;

    DesignerSupport *designerSupport();

    void createScene(const CreateSceneCommand &command) Q_DECL_OVERRIDE;
    void clearScene(const ClearSceneCommand &command) Q_DECL_OVERRIDE;
    void reparentInstances(const ReparentInstancesCommand &command) Q_DECL_OVERRIDE;

protected:
    void initializeView() Q_DECL_OVERRIDE;
    void resizeCanvasSizeToRootItemSize() Q_DECL_OVERRIDE;
    void resetAllItems();
    QList<ServerNodeInstance> setupScene(const CreateSceneCommand &command) Q_DECL_OVERRIDE;
    QList<QQuickItem*> allItems() const;

private:
    QPointer<QQuickView> m_quickView;
    DesignerSupport m_designerSupport;
};

} // QmlDesigner

#endif // QT5NODEINSTANCESERVER_H
