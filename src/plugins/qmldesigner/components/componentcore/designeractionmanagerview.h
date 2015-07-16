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

#ifndef QMLDESIGNER_DESIGNERACTIONMANAGERVIEW_H
#define QMLDESIGNER_DESIGNERACTIONMANAGERVIEW_H

#include <abstractview.h>

#include "designeractionmanager.h"

namespace QmlDesigner {

class ActionInterface;

class DesignerActionManagerView : public AbstractView
{
    Q_OBJECT
public:
    DesignerActionManagerView();

    void modelAttached(Model *model) override;
    void modelAboutToBeDetached(Model *model) override;
    void nodeCreated(const ModelNode &) override;
    void nodeRemoved(const ModelNode &, const NodeAbstractProperty &, PropertyChangeFlags) override;
    void nodeAboutToBeReparented(const ModelNode &,
                                         const NodeAbstractProperty &,
                                         const NodeAbstractProperty &,
                                         AbstractView::PropertyChangeFlags ) override;
    void nodeReparented(const ModelNode &, const NodeAbstractProperty &,
                                const NodeAbstractProperty &,
                                AbstractView::PropertyChangeFlags) override;
    void propertiesRemoved(const QList<AbstractProperty>&) override;
    void rootNodeTypeChanged(const QString &, int , int ) override;
    void rewriterBeginTransaction() override;
    void rewriterEndTransaction() override;
    void currentStateChanged(const ModelNode &) override;
    void selectedNodesChanged(const QList<ModelNode> &,
                                      const QList<ModelNode> &) override;
    void nodeOrderChanged(const NodeListProperty &, const ModelNode &, int ) override;
    void importsChanged(const QList<Import> &, const QList<Import> &) override;
    void signalHandlerPropertiesChanged(const QVector<SignalHandlerProperty> &/*propertyList*/, PropertyChangeFlags /*propertyChange*/) override;

    void setDesignerActionList(const QList<ActionInterface* > &designerActionList);
    DesignerActionManager &designerActionManager();
    const DesignerActionManager &designerActionManager() const;

protected:
    void setupContext();

private:
    DesignerActionManager m_designerActionManager;
    QList<ActionInterface* > m_designerActionList;
    bool m_isInRewriterTransaction;
    bool m_setupContextDirty;
};


} // namespace QmlDesigner

#endif // QMLDESIGNER_DESIGNERACTIONMANAGERVIEW_H
