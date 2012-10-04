/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**************************************************************************/

#ifndef QT5NODEINSTANCESERVER_H
#define QT5NODEINSTANCESERVER_H

#include <QtGlobal>

#include "nodeinstanceserver.h"

QT_BEGIN_NAMESPACE
class QQuickItem;
class DesignerSupport;
QT_END_NAMESPACE

namespace QmlDesigner {

class Qt5NodeInstanceServer : public NodeInstanceServer
{
    Q_OBJECT
public:
    Qt5NodeInstanceServer(NodeInstanceClientInterface *nodeInstanceClient);
    ~Qt5NodeInstanceServer();

    QQuickView *quickView() const;
    QQmlView *declarativeView() const;
    QQmlEngine *engine() const;
    void refreshBindings();

    DesignerSupport *designerSupport() const;

    void createScene(const CreateSceneCommand &command);
    void clearScene(const ClearSceneCommand &command);

protected:
    void initializeView(const QVector<AddImportContainer> &importVector);
    void resizeCanvasSizeToRootItemSize();
    void resetAllItems();
    QList<ServerNodeInstance> setupScene(const CreateSceneCommand &command);
    QList<QQuickItem*> allItems() const;

private:
    QPointer<QQuickView> m_quickView;
    DesignerSupport *m_designerSupport;
};

} // QmlDesigner

#endif // QT5NODEINSTANCESERVER_H
