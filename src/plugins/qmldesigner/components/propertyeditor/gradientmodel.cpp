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
#include "propertyeditorview.h"

#include <exception.h>
#include <nodeproperty.h>
#include <nodelistproperty.h>
#include <variantproperty.h>
#include <abstractview.h>
#include <nodemetainfo.h>
#include <rewritertransaction.h>

#include <utils/qtcassert.h>

#include <QTimer>

GradientModel::GradientModel(QObject *parent) :
    QAbstractListModel(parent), m_gradientTypeName("Gradient"), m_locked(false)
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
    if (m_locked)
        return -1;

    if (!m_itemNode.isValid() || gradientPropertyName().isEmpty())
        return -1;

    if (m_itemNode.modelNode().hasNodeProperty(gradientPropertyName().toUtf8())) {
        int properPos = 0;
        try {

            QmlDesigner::ModelNode gradientNode =  m_itemNode.modelNode().nodeProperty(gradientPropertyName().toUtf8()).modelNode();

            QmlDesigner::ModelNode gradientStopNode = createGradientStopNode();

            gradientStopNode.variantProperty("position").setValue(position);
            gradientStopNode.variantProperty("color").setValue(color);
            gradientNode.nodeListProperty("stops").reparentHere(gradientStopNode);

            const QList<QmlDesigner::ModelNode> stopNodes = gradientNode.nodeListProperty("stops").toModelNodeList();

            for (int i = 0; i < stopNodes.count(); i++) {
                if (QmlDesigner::QmlObjectNode(stopNodes.at(i)).modelValue("position").toReal() < position)
                    properPos = i + 1;
            }
            gradientNode.nodeListProperty("stops").slide(stopNodes.count() - 1, properPos);

            setupModel();
        } catch (const QmlDesigner::Exception &e) {
            e.showException();
        }

        return properPos;
    }

    return -1;
}

void GradientModel::addGradient()
{
    if (m_locked)
        return;

    if (!m_itemNode.isValid() || gradientPropertyName().isEmpty())
        return;

    if (!m_itemNode.modelNode().hasNodeProperty(gradientPropertyName().toUtf8())) {
        try {

            QColor color = m_itemNode.instanceValue("color").value<QColor>();

            if (!color.isValid())
                color = QColor(Qt::white);

            if (m_gradientTypeName != "Gradient")
                ensureShapesImport();

            QmlDesigner::RewriterTransaction transaction = view()->beginRewriterTransaction(QByteArrayLiteral("GradientModel::addGradient"));

            QmlDesigner::ModelNode gradientNode = createGradientNode();

            m_itemNode.modelNode().nodeProperty(gradientPropertyName().toUtf8()).reparentHere(gradientNode);

            QmlDesigner::ModelNode gradientStopNode = createGradientStopNode();
            gradientStopNode.variantProperty("position").setValue(0.0);
            gradientStopNode.variantProperty("color").setValue(color);
            gradientNode.nodeListProperty("stops").reparentHere(gradientStopNode);

            gradientStopNode = createGradientStopNode();
            gradientStopNode.variantProperty("position").setValue(1.0);
            gradientStopNode.variantProperty("color").setValue(QColor(Qt::black));
            gradientNode.nodeListProperty("stops").reparentHere(gradientStopNode);

        } catch (const QmlDesigner::Exception &e) {
            e.showException();
        }

    }
    setupModel();

    if (m_gradientTypeName != "Gradient")
        QTimer::singleShot(100, [this](){ view()->resetPuppet(); }); /*Unfortunately required */
    emit hasGradientChanged();
    emit gradientTypeChanged();
}

void GradientModel::setColor(int index, const QColor &color)
{
    if (locked())
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
    if (locked())
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
    return {};
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
        QmlDesigner::RewriterTransaction transaction = view()->beginRewriterTransaction(QByteArrayLiteral("GradientModel::removeStop"));
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
            QmlDesigner::RewriterTransaction transaction = view()->beginRewriterTransaction(QByteArrayLiteral("GradientModel::deleteGradient"));
            QmlDesigner::ModelNode gradientNode = modelNode.nodeProperty(gradientPropertyName().toUtf8()).modelNode();
            if (QmlDesigner::QmlObjectNode(gradientNode).isValid())
                QmlDesigner::QmlObjectNode(gradientNode).destroy();
        }
    }

    emit hasGradientChanged();
    emit gradientTypeChanged();
}

void GradientModel::lock()
{
    m_locked = true;
}

void GradientModel::unlock()
{
    m_locked = false;
}

void GradientModel::registerDeclarativeType()
{
    qmlRegisterType<GradientModel>("HelperWidgets",2,0,"GradientModel");
}

qreal GradientModel::readGradientProperty(const QString &propertyName) const
{
    if (!m_itemNode.isValid())
            return 0;

    QmlDesigner::QmlObjectNode gradient;

    if (m_itemNode.modelNode().hasProperty(gradientPropertyName().toUtf8()))
        gradient = m_itemNode.modelNode().nodeProperty(gradientPropertyName().toUtf8()).modelNode();

    if (!gradient.isValid())
            return 0;

    return gradient.modelValue(propertyName.toUtf8()).toReal();
}

void GradientModel::setupModel()
{
    m_locked = true;
    beginResetModel();

    endResetModel();
    m_locked = false;
}

void GradientModel::setAnchorBackend(const QVariant &anchorBackend)
{
    auto anchorBackendObject = anchorBackend.value<QObject*>();

    const auto backendCasted =
            qobject_cast<const QmlDesigner::Internal::QmlAnchorBindingProxy *>(anchorBackendObject);

    if (backendCasted)
        m_itemNode = backendCasted->getItemNode();

    if (m_itemNode.isValid()
            && m_itemNode.modelNode().hasNodeProperty(gradientPropertyName().toUtf8()))
        m_gradientTypeName = m_itemNode.modelNode().nodeProperty(gradientPropertyName().toUtf8()).modelNode().simplifiedTypeName();

    setupModel();

    m_locked = true;

    emit anchorBackendChanged();
    emit hasGradientChanged();
    emit gradientTypeChanged();

    m_locked = false;
}

QString GradientModel::gradientPropertyName() const
{
    return m_gradientPropertyName;
}

void GradientModel::setGradientPropertyName(const QString &name)
{
    m_gradientPropertyName = name;
}

QString GradientModel::gradientTypeName() const
{
    return m_gradientTypeName;
}

void GradientModel::setGradientTypeName(const QString &name)
{
     m_gradientTypeName = name;
}

bool GradientModel::hasGradient() const
{
    return m_itemNode.isValid()
            && m_itemNode.modelNode().hasProperty(gradientPropertyName().toUtf8());
}

bool GradientModel::locked() const
{
    if (m_locked)
        return true;

    auto editorView = qobject_cast<QmlDesigner::PropertyEditorView*>(view());

    return editorView && editorView->locked();
}

bool GradientModel::hasShapesImport() const
{
    if (m_itemNode.isValid()) {
        QmlDesigner::Import import = QmlDesigner::Import::createLibraryImport("QtQuick.Shapes", "1.0");
        return model()->hasImport(import, true, true);
    }

    return false;
}

void GradientModel::ensureShapesImport()
{
    if (!hasShapesImport()) {
        QmlDesigner::Import timelineImport = QmlDesigner::Import::createLibraryImport("QtQuick.Shapes", "1.0");
        model()->changeImports({timelineImport}, {});
    }
}

void GradientModel::setupGradientProperties(const QmlDesigner::ModelNode &gradient)
{
    QTC_ASSERT(m_itemNode.isValid(), return);

    QTC_ASSERT(gradient.isValid(), return);

    if (m_gradientTypeName == "Gradient") {
    } else if (m_gradientTypeName == "LinearGradient") {
        gradient.variantProperty("x1").setValue(0);
        gradient.variantProperty("x2").setValue(m_itemNode.instanceValue("width"));
        gradient.variantProperty("y1").setValue(0);
        gradient.variantProperty("y2").setValue(m_itemNode.instanceValue("height"));
    } else if (m_gradientTypeName == "RadialGradient") {
        qreal width = m_itemNode.instanceValue("width").toReal();
        qreal height = m_itemNode.instanceValue("height").toReal();
        gradient.variantProperty("centerX").setValue(width / 2.0);
        gradient.variantProperty("centerY").setValue(height / 2.0);

        gradient.variantProperty("focalX").setValue(width / 2.0);
        gradient.variantProperty("focalY").setValue(height / 2.0);

        qreal radius = qMin(width, height) / 2;

        gradient.variantProperty("centerRadius").setValue(radius);
        gradient.variantProperty("focalRadius").setValue(0);

    } else if (m_gradientTypeName == "ConicalGradient") {
        qreal width = m_itemNode.instanceValue("width").toReal();
        qreal height = m_itemNode.instanceValue("height").toReal();
        gradient.variantProperty("centerX").setValue(width / 2.0);
        gradient.variantProperty("centerY").setValue(height / 2.0);

        gradient.variantProperty("angle").setValue(0);
    }
}

QmlDesigner::Model *GradientModel::model() const
{
    QTC_ASSERT(m_itemNode.isValid(), return nullptr);
    return m_itemNode.view()->model();
}

QmlDesigner::AbstractView *GradientModel::view() const
{
    QTC_ASSERT(m_itemNode.isValid(), return nullptr);
    return m_itemNode.view();
}

QmlDesigner::ModelNode GradientModel::createGradientNode()
{
    QByteArray fullTypeName = m_gradientTypeName.toUtf8();

    if (m_gradientTypeName == "Gradient") {
        fullTypeName.prepend("QtQuick.");
    } else {
        fullTypeName.prepend("QtQuick.Shapes.");
    }

    auto metaInfo = model()->metaInfo(fullTypeName);

    int minorVersion = metaInfo.minorVersion();
    int majorVersion = metaInfo.majorVersion();

    auto gradientNode = view()->createModelNode(fullTypeName, majorVersion, minorVersion);

    setupGradientProperties(gradientNode);

    return gradientNode;
}

QmlDesigner::ModelNode GradientModel::createGradientStopNode()
{
    QByteArray fullTypeName = "QtQuick.GradientStop";
    auto metaInfo = model()->metaInfo(fullTypeName);

    int minorVersion = metaInfo.minorVersion();
    int majorVersion = metaInfo.majorVersion();

    return view()->createModelNode(fullTypeName, majorVersion, minorVersion);
}

void GradientModel::setGradientProperty(const QString &propertyName, qreal value)
{
    QTC_ASSERT(m_itemNode.isValid(), return);

    QmlDesigner::QmlObjectNode gradient;

    if (m_itemNode.modelNode().hasProperty(gradientPropertyName().toUtf8()))
        gradient = m_itemNode.modelNode().nodeProperty(gradientPropertyName().toUtf8()).modelNode();

    QTC_ASSERT(gradient.isValid(), return);

    try {
        gradient.setVariantProperty(propertyName.toUtf8(), value);
    } catch (const QmlDesigner::Exception &e) {
        e.showException();
    }
}
