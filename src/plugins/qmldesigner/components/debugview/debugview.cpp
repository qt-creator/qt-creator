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

#include "debugview.h"
#include "debugviewwidget.h"

#include <qmldesignerplugin.h>

#include <bindingproperty.h>
#include <signalhandlerproperty.h>
#include <nodeabstractproperty.h>
#include <variantproperty.h>

namespace   {
const QLatin1String lineBreak = QLatin1String("<br>");

bool isDebugViewEnabled()
{
    return (QmlDesigner::QmlDesignerPlugin::instance()->settings().enableDebugView);
}

bool isDebugViewShown()
{
    return (QmlDesigner::QmlDesignerPlugin::instance()->settings().showDebugView);
}

}

namespace QmlDesigner {

namespace  Internal {

DebugView::DebugView(QObject *parent) : QmlModelView(parent),
    m_debugViewWidget(new DebugViewWidget)
{
}

DebugView::~DebugView()
{
}

void DebugView::modelAttached(Model *model)
{
    log(tr("Model attached"), tr("FileName %1").arg(model->fileUrl().toLocalFile()));
    m_debugViewWidget->setDebugViewEnabled(isDebugViewEnabled());
    qDebug() << "enabled: " << isDebugViewEnabled();
    QmlModelView::modelAttached(model);
}

void DebugView::modelAboutToBeDetached(Model *model)
{
    log(tr("Model detached"), tr("FileName %1").arg(model->fileUrl().toLocalFile()));
    QmlModelView::modelAboutToBeDetached(model);
}

void DebugView::importsChanged(const QList<Import> &addedImports, const QList<Import> &removedImports)
{
    if (isDebugViewEnabled()) {
        QString message;
        message += tr("Added imports:") += lineBreak;
        foreach (const Import &import, addedImports) {
            message += import.toString() += lineBreak;
        }

        message += tr("Removed imports:") += lineBreak;
        foreach (const Import &import, removedImports) {
            message += import.toString() += lineBreak;
        }

        log(tr("Imports changed:"), message);
    }
}

void DebugView::nodeCreated(const ModelNode &createdNode)
{
    if (isDebugViewEnabled()) {
        QTextStream message;
        QString string;
        message.setString(&string);
        message << createdNode;
        log(tr("Node created:"), message.readAll());
    }
}

void DebugView::nodeAboutToBeRemoved(const ModelNode &removedNode)
{
    if (isDebugViewEnabled()) {
        QTextStream message;
        QString string;
        message.setString(&string);
        message << removedNode;
        log(tr("Node removed:"), message.readAll());
    }
}

void DebugView::nodeReparented(const ModelNode &node, const NodeAbstractProperty &newPropertyParent,
                               const NodeAbstractProperty &oldPropertyParent, AbstractView::PropertyChangeFlags propertyChange)
{
    if (isDebugViewEnabled()) {
        QTextStream message;
        QString string;
        message.setString(&string);
        message << node;
        message << tr("New parent property:");
        message << lineBreak;
        message << newPropertyParent;
        message << tr("Old parent property:");
        message << lineBreak;
        message << oldPropertyParent;
        message << tr("PropertyChangeFlag");
        message << lineBreak;
        message << propertyChange;
        log(tr("Node reparanted:"), message.readAll());
    }
}

void DebugView::nodeIdChanged(const ModelNode &node, const QString &newId, const QString &oldId)
{
    if (isDebugViewEnabled()) {
        QTextStream message;
        QString string;
        message.setString(&string);
        message << node;
        message << tr("New Id: ") << newId << lineBreak;
        message << tr("Old Id: ") << oldId << lineBreak;
        log(tr("Node id changed:"), string);
    }
}

void DebugView::propertiesAboutToBeRemoved(const QList<AbstractProperty> & /*propertyList*/)
{
}

void DebugView::variantPropertiesChanged(const QList<VariantProperty> &propertyList,
                                         AbstractView::PropertyChangeFlags /*propertyChange*/)
{
    if (isDebugViewEnabled()) {
        QTextStream message;
        QString string;
        message.setString(&string);
        foreach (const VariantProperty &property, propertyList) {
            message << property;
        }
        log(tr("VariantProperties changed:"), string);
    }
}

void DebugView::bindingPropertiesChanged(const QList<BindingProperty> &propertyList,
                                         AbstractView::PropertyChangeFlags /*propertyChange*/)
{
    if (isDebugViewEnabled()) {
        QTextStream message;
        QString string;
        message.setString(&string);
        foreach (const BindingProperty &property, propertyList) {
            message << property;
        }
        log(tr("BindingProperties changed:"), string);
    }
}

void DebugView::signalHandlerPropertiesChanged(const QVector<SignalHandlerProperty> &propertyList, AbstractView::PropertyChangeFlags /*propertyChange*/)
{
    if (isDebugViewEnabled()) {
        QTextStream message;
        QString string;
        message.setString(&string);
        foreach (const SignalHandlerProperty &property, propertyList) {
            message << property;
        }
        log(tr("SignalHandlerProperties changed:"), string);
    }
}

void DebugView::rootNodeTypeChanged(const QString &type, int majorVersion, int minorVersion)
{
    if (isDebugViewEnabled()) {
        QString message;
        message += type;
        message += QLatin1String(" ");
        message += QString::number(majorVersion);
        message += QLatin1String(" ");
        message += QString::number(minorVersion);
        log(tr("Node id changed:"), message);
    }
}

void DebugView::selectedNodesChanged(const QList<ModelNode> & /*selectedNodeList*/,
                                     const QList<ModelNode> & /*lastSelectedNodeList*/)
{
}

void DebugView::scriptFunctionsChanged(const ModelNode & /*node*/, const QStringList & /*scriptFunctionList*/)
{
}

void DebugView::propertiesRemoved(const QList<AbstractProperty> &propertyList)
{
    if (isDebugViewEnabled()) {
        QTextStream message;
        QString string;
        message.setString(&string);
        foreach (const AbstractProperty &property, propertyList) {
            message << property;
        }
        log(tr("Properties removed:"), string);
    }
}

void DebugView::auxiliaryDataChanged(const ModelNode &node, const PropertyName &name, const QVariant &data)
{
    if (isDebugViewEnabled()) {
        QTextStream message;
        QString string;
        message.setString(&string);

        message << node;
        message << name;
        message << data.toString();

        log(tr("Auxiliary Data Changed:"), string);
    }
}

void DebugView::rewriterBeginTransaction()
{
    if (isDebugViewEnabled())
        log(tr("Begin rewriter transaction"), QString(), true);
}

void DebugView::rewriterEndTransaction()
{
    if (isDebugViewEnabled())
        log(tr("End rewriter transaction"), QString(), true);
}

WidgetInfo DebugView::widgetInfo()
{
    return createWidgetInfo(m_debugViewWidget.data(), 0, QLatin1String("DebugView"), WidgetInfo::LeftPane, 0, tr("Debug View"));
}

bool DebugView::hasWidget() const
{
    if (isDebugViewShown())
        return true;

    return false;
}

void DebugView::instancePropertyChange(const QList<QPair<ModelNode, PropertyName> > &propertyList)
{
    if (isDebugViewEnabled()) {
        QTextStream message;
        QString string;
        message.setString(&string);

        typedef QPair<ModelNode, PropertyName> Pair;

        foreach (const Pair &pair, propertyList) {
            message << pair.first;
            message << lineBreak;
            message << pair.second;
        }

        logInstance(tr("Instance property change"), string);
    }
}

void DebugView::instancesCompleted(const QVector<ModelNode> &completedNodeList)
{
    if (isDebugViewEnabled()) {
        QTextStream message;
        QString string;
        message.setString(&string);

        foreach (const ModelNode &modelNode, completedNodeList) {
            message << modelNode;
        }

        logInstance(tr("Instance Completed"), string);
    }
}

void DebugView::instanceInformationsChange(const QMultiHash<ModelNode, InformationName> &informationChangeHash)
{
    if (isDebugViewEnabled()) {
        QTextStream message;
        QString string;
        message.setString(&string);

        foreach (const ModelNode &modelNode, informationChangeHash.keys()) {
            message << modelNode;
            message << informationChangeHash.value(modelNode);
        }

        logInstance(tr("Instance Completed"), string);
    }

}

void DebugView::instancesRenderImageChanged(const QVector<ModelNode> & /*nodeList*/)
{
}

void DebugView::instancesPreviewImageChanged(const QVector<ModelNode> & /*nodeList*/)
{
}

void DebugView::instancesChildrenChanged(const QVector<ModelNode> & /*nodeList*/)
{
}

void DebugView::customNotification(const AbstractView *view, const QString &identifier, const QList<ModelNode> &nodeList, const QList<QVariant> &data)
{
    if (isDebugViewEnabled()) {
        QTextStream message;
        QString string;
        message.setString(&string);

        message << view;
        message << identifier;
        foreach (const ModelNode &node, nodeList) {
            message << node;
        }

        foreach (const QVariant &variant, data) {
            message << variant.toString();
        }

        log(tr("Custom Notification:"), string);
    }
}

void DebugView::nodeSourceChanged(const ModelNode &modelNode, const QString &newNodeSource)
{
    if (isDebugViewEnabled()) {
        QTextStream message;
        QString string;
        message.setString(&string);

        message << modelNode;
        message << newNodeSource;

        log(tr("Node Source Changed:"), string);
    }
}

void DebugView::log(const QString &title, const QString &message, bool highlight)
{
    m_debugViewWidget->addLogMessage(title, message, highlight);
}

void DebugView::logInstance(const QString &title, const QString &message, bool highlight)
{
    m_debugViewWidget->addLogInstanceMessage(title, message, highlight);
}

} // namesapce Internal

} // namespace QmlDesigner
