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

#ifndef QT5NODEINSTANCESERVER_H
#define QT5NODEINSTANCESERVER_H

#include <QtGlobal>

#include "nodeinstanceserver.h"
#include <designersupportdelegate.h>

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

    QQuickView *quickView() const override;
    QQmlView *declarativeView() const override;
    QQmlEngine *engine() const override;
    void refreshBindings() override;

    DesignerSupport *designerSupport();

    void createScene(const CreateSceneCommand &command) override;
    void clearScene(const ClearSceneCommand &command) override;
    void reparentInstances(const ReparentInstancesCommand &command) override;

protected:
    void initializeView() override;
    void resizeCanvasSizeToRootItemSize() override;
    void resetAllItems();
    void setupScene(const CreateSceneCommand &command) override;
    QList<QQuickItem*> allItems() const;

private:
    QPointer<QQuickView> m_quickView;
    DesignerSupport m_designerSupport;
};

} // QmlDesigner

#endif // QT5NODEINSTANCESERVER_H
