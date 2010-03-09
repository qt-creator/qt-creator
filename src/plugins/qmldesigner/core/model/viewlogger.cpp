/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "viewlogger.h"
#include <QDebug>
#include <QTemporaryFile>
#include <QDir>
#include <variantproperty.h>
#include <bindingproperty.h>
#include <nodeabstractproperty.h>
#include <nodelistproperty.h>

namespace QmlDesigner {
namespace Internal {

static QString serialize(AbstractView::PropertyChangeFlags change)
{
    QStringList tokenList;

    if (change.testFlag(AbstractView::PropertiesAdded))
        tokenList.append(QLatin1String("PropertiesAdded"));

    if (change.testFlag(AbstractView::EmptyPropertiesRemoved))
        tokenList.append(QLatin1String("EmptyPropertiesRemoved"));

    return tokenList.join(" ");

    return QString();
}

static QString indent(const QString &name = QString()) {
    return name.leftJustified(30, ' ');
}

QString ViewLogger::time() const
{
    return QString::number(m_timer.elapsed()).leftJustified(7, ' ');
}

ViewLogger::ViewLogger(QObject *parent)
    : AbstractView(parent)
{
#ifdef Q_OS_MAC
    const QString tempPath = "/tmp";
#else
    const QString tempPath = QDir::tempPath();
#endif

    QTemporaryFile *temporaryFile = new QTemporaryFile(tempPath + QString("/bauhaus-logger-%1-XXXXXX.txt").arg(QDateTime::currentDateTime().toString(Qt::ISODate).replace(":", "-")), this);

    temporaryFile->setAutoRemove(false);
    if (temporaryFile->open()) {
        qDebug() << "ViewLogger: TemporaryLoggerFile is:" << temporaryFile->fileName();
        m_output.setDevice(temporaryFile);
    } else {
        qDebug() << "ViewLogger: failed to open:" << temporaryFile->fileName();
    }

    m_timer.start();
}

void ViewLogger::modelAttached(Model *model)
{
    m_output << time() << indent("modelAttached:") << model << endl;
    AbstractView::modelAttached(model);
}

void ViewLogger::modelAboutToBeDetached(Model *model)
{
    m_output << time() << indent("modelAboutToBeDetached:") << model << endl;
    AbstractView::modelAboutToBeDetached(model);
}

void ViewLogger::nodeCreated(const ModelNode &createdNode)
{
    m_output << time() << indent("nodeCreated:") << createdNode << endl;
}

void ViewLogger::nodeAboutToBeRemoved(const ModelNode &removedNode)
{
    m_output << time() << indent("nodeAboutToBeRemoved:") << removedNode << endl;
}

void ViewLogger::nodeRemoved(const ModelNode &removedNode, const NodeAbstractProperty &parentProperty, PropertyChangeFlags propertyChange)
{
    m_output << time() << indent("nodeRemoved:") << removedNode << parentProperty << serialize(propertyChange) << endl;
}

void ViewLogger::nodeReparented(const ModelNode &node, const NodeAbstractProperty &newPropertyParent, const NodeAbstractProperty &oldPropertyParent, AbstractView::PropertyChangeFlags propertyChange)
{
    m_output << time() << indent("nodeReparented:") << node << "\t" << newPropertyParent << "\t" << oldPropertyParent << "\t" << serialize(propertyChange) << endl;
}

void ViewLogger::nodeIdChanged(const ModelNode& node, const QString& newId, const QString& oldId)
{
    m_output << time() << indent("nodeIdChanged:") << node << "\t" << newId << "\t" << oldId << endl;
}

void ViewLogger::propertiesAboutToBeRemoved(const QList<AbstractProperty>& propertyList)
{
    m_output << time() << indent("propertiesAboutToBeRemoved:") << endl;
    foreach (const AbstractProperty &property, propertyList)
        m_output << time() << indent() << property << endl;
}

void ViewLogger::propertiesRemoved(const QList<AbstractProperty> &propertyList)
{
    m_output << time() << indent("propertiesRemoved:") << endl;
    foreach (const AbstractProperty &property, propertyList)
        m_output << time() << indent() << property << endl;
}

void ViewLogger::variantPropertiesChanged(const QList<VariantProperty>& propertyList, PropertyChangeFlags propertyChange)
{
    m_output << time() << indent("variantPropertiesChanged:") << serialize(propertyChange) << endl;
    foreach(const VariantProperty &property, propertyList)
        m_output << time() << indent() << property << endl;
}

void ViewLogger::bindingPropertiesChanged(const QList<BindingProperty>& propertyList, PropertyChangeFlags propertyChange)
{
    m_output << time() << indent("bindingPropertiesChanged:") << serialize(propertyChange) << endl;
    foreach(const BindingProperty &property, propertyList)
        m_output << time() << indent() << property << endl;
}

void ViewLogger::rootNodeTypeChanged(const QString &type, int majorVersion, int minorVersion)
{
    m_output << time() << indent("rootNodeTypeChanged:") << rootModelNode() << type << majorVersion << minorVersion << endl;
}

void ViewLogger::selectedNodesChanged(const QList<ModelNode> &selectedNodeList,
                                  const QList<ModelNode> &lastSelectedNodeList)
{
    m_output << time() << indent("selectedNodesChanged:") << endl;
    foreach(const ModelNode &node, selectedNodeList)
        m_output << time() << indent("new: ") << node << endl;
    foreach(const ModelNode &node, lastSelectedNodeList)
        m_output << time() << indent("old: ") << node << endl;
}

void ViewLogger::fileUrlChanged(const QUrl &oldUrl, const QUrl &newUrl)
{
    m_output << time() << indent("fileUrlChanged:") << oldUrl.toString() << "\t" << newUrl.toString() << endl;
}

void ViewLogger::nodeOrderChanged(const NodeListProperty &listProperty, const ModelNode &movedNode, int oldIndex)
{
    m_output << time() << indent("nodeOrderChanged:") << listProperty << movedNode << oldIndex << endl;
}

void ViewLogger::importsChanged()
{
    m_output << time() << indent("importsChanged:") << endl;
}

void ViewLogger::auxiliaryDataChanged(const ModelNode &node, const QString &name, const QVariant &data)
{
    m_output << time() << indent("auxiliaryDataChanged:") << node << "\t" << name << "\t" << data.toString() << endl;
}

void ViewLogger::customNotification(const AbstractView *view, const QString &identifier, const QList<ModelNode> &nodeList, const QList<QVariant> &data)
{
    m_output << time() << indent("customNotification:") << view << identifier << endl;
    foreach(const ModelNode &node, nodeList)
        m_output << time() << indent("node: ") << node << endl;
    foreach(const QVariant &variant, data)
        m_output << time() << indent("data: ") << variant.toString() << endl;
}


} // namespace Internal
} // namespace QmlDesigner
