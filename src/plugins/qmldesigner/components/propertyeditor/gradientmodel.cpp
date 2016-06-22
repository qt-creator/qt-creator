/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "gradientmodel.h"

#include "qmlanchorbindingproxy.h"

#include <nodeproperty.h>
#include <nodelistproperty.h>
#include <variantproperty.h>
#include <abstractview.h>
#include <nodemetainfo.h>
#include <rewritertransaction.h>

GradientModel::GradientModel(QObject *parent) :
    QAbstractListModel(parent), m_lock(false)
{
}

int GradientModel::rowCount(const QModelIndex & /*parent*/) const
{
    if (m_itemNode.isValid()) {
        if (m_itemNode.modelNode().hasNodeProperty(gradientPropertyName().toUtf8())) {
            QmlDesigner::ModelNode gradientNode =
                    m_itemNode.modelNode().nodeProperty(gradientPropertyName().toUtf8()).modelNode();

            if (gradientNode.isValid() && gradientNode.hasNodeListProperty("stops"))
                return gradientNode.nodeListProperty("stops").toModelNodeList().count();
        }
    }

    return 0;
}

QHash<int, QByteArray> GradientModel::roleNames() const
{
    static QHash<int, QByteArray> roleNames{
        {Qt::UserRole + 1, "position"},
        {Qt::UserRole + 2, "color"},
        {Qt::UserRole + 3, "readOnly"},
        {Qt::UserRole + 4, "index"}
    };

    return roleNames;
}

QVariant GradientModel::data(const QModelIndex &index, int role) const
{
    if (index.isValid() && index.row() < rowCount()) {

        if (role == Qt::UserRole + 3) {
            if (index.row() == 0 || index.row() == (rowCount() - 1))
                return true;
            return false;
        } else if (role == Qt::UserRole + 4) {
            return index.row();
        }

        if (role == Qt::UserRole + 1)
            return getPosition(index.row());
        else if (role == Qt::UserRole + 2)
            return getColor(index.row());

        qWarning() << Q_FUNC_INFO << "invalid role";
    } else {
        qWarning() << Q_FUNC_INFO << "invalid index";
    }

    return QVariant();
}

int GradientModel::addStop(qreal position, const QColor &color)
{
    if (m_lock)
        return -1;

    if (!m_itemNode.isValid() || gradientPropertyName().isEmpty())
        return -1;

    if (m_itemNode.modelNode().hasNodeProperty(gradientPropertyName().toUtf8())) {
        //QmlDesigner::RewriterTransaction transaction = m_itemNode.modelNode().view()->beginRewriterTransaction();
        //### TODO does not work

        QmlDesigner::ModelNode gradientNode =  m_itemNode.modelNode().nodeProperty(gradientPropertyName().toUtf8()).modelNode();

        QmlDesigner::ModelNode gradientStopNode =
                m_itemNode.modelNode().view()->createModelNode("QtQuick.GradientStop",
                                                               m_itemNode.modelNode().view()->majorQtQuickVersion(), 0);
        gradientStopNode.variantProperty("position").setValue(position);
        gradientStopNode.variantProperty("color").setValue(color);
        gradientNode.nodeListProperty("stops").reparentHere(gradientStopNode);

        const QList<QmlDesigner::ModelNode> stopNodes = gradientNode.nodeListProperty("stops").toModelNodeList();

        int properPos = 0;
        for (int i = 0; i < stopNodes.count(); i++) {
            if (QmlDesigner::QmlObjectNode(stopNodes.at(i)).modelValue("position").toReal() < position)
                properPos = i + 1;
        }
        gradientNode.nodeListProperty("stops").slide(stopNodes.count() - 1, properPos);

        setupModel();

        return properPos;
    }

    return -1;
}

void GradientModel::addGradient()
{
    if (m_lock)
        return;

    if (!m_itemNode.isValid() || gradientPropertyName().isEmpty())
        return;

    if (!m_itemNode.modelNode().hasNodeProperty(gradientPropertyName().toUtf8())) {

        QColor color = m_itemNode.instanceValue("color").value<QColor>();

        if (!color.isValid())
            color = QColor(Qt::white);

        QmlDesigner::RewriterTransaction transaction = m_itemNode.modelNode().view()->beginRewriterTransaction(QByteArrayLiteral("GradientModel::addGradient"));

        QmlDesigner::ModelNode gradientNode =
                m_itemNode.modelNode().view()->createModelNode("QtQuick.Gradient",
                                                               m_itemNode.modelNode().view()->majorQtQuickVersion(), 0);
        m_itemNode.modelNode().nodeProperty(gradientPropertyName().toUtf8()).reparentHere(gradientNode);


        QmlDesigner::ModelNode gradientStopNode =
                m_itemNode.modelNode().view()->createModelNode("QtQuick.GradientStop",
                                                             m_itemNode.modelNode().view()->majorQtQuickVersion(), 0);
        gradientStopNode.variantProperty("position").setValue(0.0);
        gradientStopNode.variantProperty("color").setValue(color);
        gradientNode.nodeListProperty("stops").reparentHere(gradientStopNode);

        gradientStopNode = m_itemNode.modelNode().view()->createModelNode(
                    "QtQuick.GradientStop",
                     m_itemNode.modelNode().view()->majorQtQuickVersion(), 0);
        gradientStopNode.variantProperty("position").setValue(1.0);
        gradientStopNode.variantProperty("color").setValue(QColor(Qt::black));
        gradientNode.nodeListProperty("stops").reparentHere(gradientStopNode);

    }
    setupModel();

    emit hasGradientChanged();
}

void GradientModel::setColor(int index, const QColor &color)
{
    if (m_lock)
        return;

    if (!m_itemNode.modelNode().isSelected())
        return;

    if (index < rowCount()) {
        QmlDesigner::ModelNode gradientNode =  m_itemNode.modelNode().nodeProperty(gradientPropertyName().toUtf8()).modelNode();
        QmlDesigner::QmlObjectNode stop = gradientNode.nodeListProperty("stops").at(index);
        if (stop.isValid())
            stop.setVariantProperty("color", color);
        setupModel();
    }
}

void GradientModel::setPosition(int index, qreal positition)
{
    if (m_lock)
        return;

    if (index < rowCount()) {
        QmlDesigner::ModelNode gradientNode =  m_itemNode.modelNode().nodeProperty(gradientPropertyName().toUtf8()).modelNode();
        QmlDesigner::QmlObjectNode stop = gradientNode.nodeListProperty("stops").at(index);
        if (stop.isValid())
            stop.setVariantProperty("position", positition);
        setupModel();
    }
}

QColor GradientModel::getColor(int index) const
{
    if (index < rowCount()) {
        QmlDesigner::ModelNode gradientNode =  m_itemNode.modelNode().nodeProperty(gradientPropertyName().toUtf8()).modelNode();
        QmlDesigner::QmlObjectNode stop = gradientNode.nodeListProperty("stops").at(index);
        if (stop.isValid())
            return stop.modelValue("color").value<QColor>();
    }
    qWarning() << Q_FUNC_INFO << "invalid color index";
    return QColor();
}

qreal GradientModel::getPosition(int index) const
{
    if (index < rowCount()) {
        QmlDesigner::ModelNode gradientNode =  m_itemNode.modelNode().nodeProperty(gradientPropertyName().toUtf8()).modelNode();
        QmlDesigner::QmlObjectNode stop = gradientNode.nodeListProperty("stops").at(index);
        if (stop.isValid())
            return stop.modelValue("position").toReal();
    }
    qWarning() << Q_FUNC_INFO << "invalid position index";
    return 0.0;
}

void GradientModel::removeStop(int index)
{
    if (index < rowCount() - 1 && index != 0) {
        QmlDesigner::RewriterTransaction transaction = m_itemNode.modelNode().view()->beginRewriterTransaction(QByteArrayLiteral("GradientModel::removeStop"));
        QmlDesigner::ModelNode gradientNode =  m_itemNode.modelNode().nodeProperty(gradientPropertyName().toUtf8()).modelNode();
        QmlDesigner::QmlObjectNode stop = gradientNode.nodeListProperty("stops").at(index);
        if (stop.isValid()) {
            stop.destroy();
            setupModel();
        }
    }
    qWarning() << Q_FUNC_INFO << "invalid index";
}


void GradientModel::deleteGradient()
{
    if (!m_itemNode.isValid())
        return;

    if (!m_itemNode.modelNode().metaInfo().hasProperty(gradientPropertyName().toUtf8()))
        return;

    QmlDesigner::ModelNode modelNode = m_itemNode.modelNode();

    if (m_itemNode.isInBaseState()) {
        if (modelNode.hasProperty(gradientPropertyName().toUtf8())) {
            QmlDesigner::RewriterTransaction transaction = m_itemNode.modelNode().view()->beginRewriterTransaction(QByteArrayLiteral("GradientModel::deleteGradient"));
            QmlDesigner::ModelNode gradientNode = modelNode.nodeProperty(gradientPropertyName().toUtf8()).modelNode();
            if (QmlDesigner::QmlObjectNode(gradientNode).isValid())
                QmlDesigner::QmlObjectNode(gradientNode).destroy();
        }
    }

    emit hasGradientChanged();
}

void GradientModel::lock()
{
    m_lock = true;
}

void GradientModel::unlock()
{
    m_lock = false;
}

void GradientModel::registerDeclarativeType()
{
    qmlRegisterType<GradientModel>("HelperWidgets",2,0,"GradientModel");
}

void GradientModel::setupModel()
{
    m_lock = true;
    beginResetModel();

    endResetModel();
    m_lock = false;
}

void GradientModel::setAnchorBackend(const QVariant &anchorBackend)
{
    QObject* anchorBackendObject = anchorBackend.value<QObject*>();

    const QmlDesigner::Internal::QmlAnchorBindingProxy *backendCasted =
            qobject_cast<const QmlDesigner::Internal::QmlAnchorBindingProxy *>(anchorBackendObject);

    if (backendCasted)
        m_itemNode = backendCasted->getItemNode();

    setupModel();

    m_lock = true;

    emit anchorBackendChanged();
    emit hasGradientChanged();

    m_lock = false;
}

QString GradientModel::gradientPropertyName() const
{
    return m_gradientPropertyName;
}

void GradientModel::setGradientPropertyName(const QString &name)
{
    m_gradientPropertyName = name;
}

bool GradientModel::hasGradient() const
{
    return m_itemNode.isValid()
            && m_itemNode.modelNode().hasProperty(gradientPropertyName().toUtf8());
}
