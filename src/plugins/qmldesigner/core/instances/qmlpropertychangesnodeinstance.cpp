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

#include "qmlpropertychangesnodeinstance.h"
#include "qmlstatenodeinstance.h"
#include <QDeclarativeEngine>
#include <QDeclarativeContext>
#include <QDeclarativeExpression>
#include <private/qdeclarativebinding_p.h>
#include <metainfo.h>
#include <QMutableListIterator>

#include <private/qdeclarativestate_p_p.h>
#include <private/qdeclarativestategroup_p.h>
#include <private/qdeclarativeproperty_p.h>

namespace QmlDesigner {
namespace Internal {

QmlPropertyChangesObject::QmlPropertyChangesObject() :
        QDeclarativeStateOperation(),
        m_restoreEntryValues(true),
        m_isExplicit(false)
{
}

QDeclarativeStateOperation::ActionList QmlPropertyChangesObject::actions()
{
    QMutableListIterator<QDeclarativeAction> actionIterator(m_qmlActionList);

    while (actionIterator.hasNext()) {
        QDeclarativeAction &action = actionIterator.next();
        action.fromBinding = QDeclarativePropertyPrivate::binding(action.property);
        action.fromValue = action.property.read();
        if (m_expressionHash.contains(action.specifiedProperty)) {
            if(m_expressionHash[action.specifiedProperty].second.isNull()) {
                QDeclarativeBinding *binding = new QDeclarativeBinding(m_expressionHash[action.specifiedProperty].first, targetObject(), QDeclarativeEngine::contextForObject(targetObject()), this);
                binding->setTarget(action.property);
                binding->setNotifyOnValueChanged(true);
                action.toBinding = binding;
                action.toValue = binding->value();
                m_expressionHash.insert(action.specifiedProperty, ExpressionPair(m_expressionHash[action.specifiedProperty].first, binding));
            } else {
                action.toBinding = m_expressionHash[action.specifiedProperty].second.data();
                action.toValue = m_expressionHash[action.specifiedProperty].second->value();
            }
        } else {
            action.toBinding = 0;
        }
    }

    return m_qmlActionList;
}

QObject *QmlPropertyChangesObject::targetObject() const
{
    return m_targetObject.data();
}

void QmlPropertyChangesObject::setTargetObject(QObject *object)
{
    m_targetObject = object;

    QMutableListIterator<QDeclarativeAction> actionIterator(m_qmlActionList);
    while (actionIterator.hasNext()) {
        QDeclarativeAction &qmlAction = actionIterator.next();
        qmlAction.specifiedObject = object;
        qmlAction.property = createMetaProperty(qmlAction.specifiedProperty);
        qmlAction.fromValue = qmlAction.property.read();
        qmlAction.fromBinding = QDeclarativePropertyPrivate::binding(qmlAction.property);
    }
}

bool QmlPropertyChangesObject::restoreEntryValues() const
{
    return m_restoreEntryValues;
}

void QmlPropertyChangesObject::setRestoreEntryValues(bool restore)
{
    m_restoreEntryValues = restore;
}

bool QmlPropertyChangesObject::isExplicit() const
{
    return m_isExplicit;
}

void QmlPropertyChangesObject::setIsExplicit(bool isExplicit)
{
    m_isExplicit = isExplicit;
}

QDeclarativeProperty QmlPropertyChangesObject::createMetaProperty(const QString &property)
{
    QDeclarativeProperty prop(m_targetObject.data(), property);
    if (!prop.isValid()) {
        qWarning() << "Cannot assign to non-existent property" << property;
        return QDeclarativeProperty();
    } else if (!prop.isWritable()) {
        qWarning() << "Cannot assign to read-only property" << property;
        return QDeclarativeProperty();
    }
    return prop;
}

QmlPropertyChangesNodeInstance::QmlPropertyChangesNodeInstance(QmlPropertyChangesObject *propertyChangesObject) :
        ObjectNodeInstance(propertyChangesObject)
{
}

QmlPropertyChangesNodeInstance::Pointer
        QmlPropertyChangesNodeInstance::create(const NodeMetaInfo & /*metaInfo*/,
                                               QDeclarativeContext *context,
                                               QObject *objectToBeWrapped)
{
    Q_ASSERT(!objectToBeWrapped);

    QmlPropertyChangesObject *object = new QmlPropertyChangesObject;
    QDeclarativeEngine::setContextForObject(object, context);
    Pointer instance(new QmlPropertyChangesNodeInstance(object));
    return instance;
}

void QmlPropertyChangesNodeInstance::setPropertyVariant(const QString &name, const QVariant &value)
{
    QMetaObject metaObject = QmlPropertyChangesObject::staticMetaObject;

    if (metaObject.indexOfProperty(name.toLatin1()) > 0) { // 'restoreEntryValues', 'explicit'
        ObjectNodeInstance::setPropertyVariant(name, value);
    } else {
        changesObject()->setVariantValue(name, value);
        QObject *targetObject = changesObject()->targetObject();
        if (targetObject && nodeInstanceView()->activeStateInstance().isWrappingThisObject(changesObject()->state())) {
            changesObject()->updateRevertValueAndBinding(name);
            NodeInstance targetInstance = nodeInstanceView()->instanceForObject(targetObject);
            targetInstance.setPropertyVariant(name, value);
        }
    }
}

void QmlPropertyChangesNodeInstance::setPropertyBinding(const QString &name, const QString &expression)
{
    QMetaObject metaObject = QmlPropertyChangesObject::staticMetaObject;

    if (metaObject.indexOfProperty(name.toLatin1()) > 0) { // 'restoreEntryValues', 'explicit'
        ObjectNodeInstance::setPropertyBinding(name, expression);
    } else {
        changesObject()->setExpression(name, expression);
    }
}

QVariant QmlPropertyChangesNodeInstance::property(const QString &name) const
{
    if (changesObject()->hasVariantValue(name))
        return changesObject()->variantValue(name);
    if (changesObject()->hasExpression(name))
        return QVariant(changesObject()->expression(name));

    return QVariant();
}

void QmlPropertyChangesNodeInstance::resetProperty(const QString &name)
{
    changesObject()->resetProperty(name);
}


void QmlPropertyChangesNodeInstance::reparent(const NodeInstance &oldParentInstance, const QString &oldParentProperty, const NodeInstance &newParentInstance, const QString &newParentProperty)
{
    if (oldParentProperty == "changes") {
        if (changesObject()) {
            changesObject()->removeFromStateRevertList();
        }
    }

    ObjectNodeInstance::reparent(oldParentInstance, oldParentProperty, newParentInstance, newParentProperty);

    if (newParentProperty == "changes") {
        if (changesObject()) {
            changesObject()->addToStateRevertList();
        }
    }
}

QDeclarativeState *QmlPropertyChangesObject::state() const
{
    if (!parent())
        return 0;

    Q_ASSERT(qobject_cast<QDeclarativeState*>(parent()));
    return static_cast<QDeclarativeState*>(parent());
}

QDeclarativeStateGroup *QmlPropertyChangesObject::stateGroup() const
{
    if (!state())
        return 0;

    return state()->stateGroup();
}

QDeclarativeStatePrivate *QmlPropertyChangesObject::statePrivate() const
{
    if (!parent())
        return 0;

    Q_ASSERT(qobject_cast<QDeclarativeState*>(parent()));
    return static_cast<QDeclarativeStatePrivate*>(QObjectPrivate::get(parent()));
}

QmlPropertyChangesObject *QmlPropertyChangesNodeInstance::changesObject() const
{
    Q_ASSERT(qobject_cast<QmlPropertyChangesObject*>(object()));
    return static_cast<QmlPropertyChangesObject*>(object());
}


QDeclarativeAction QmlPropertyChangesObject::createQDeclarativeAction(const QString &propertyName)
{
    QDeclarativeProperty qmlMetaProperty = createMetaProperty(propertyName);

    QDeclarativeAction qmlAction;
    qmlAction.restore = true;
    qmlAction.property = qmlMetaProperty;
    qmlAction.fromValue = qmlMetaProperty.read();
    qmlAction.fromBinding = QDeclarativePropertyPrivate::binding(qmlMetaProperty);
    qmlAction.specifiedObject = m_targetObject.data();
    qmlAction.specifiedProperty = propertyName;
    qmlAction.event = 0;

    return qmlAction;
}

bool QmlPropertyChangesObject::hasActionForProperty(const QString &propertyName) const
{
    for(ActionList::iterator actionIterator = m_qmlActionList.begin();
        actionIterator != m_qmlActionList.end();
        ++actionIterator) {
            QDeclarativeAction &qmlAction = *actionIterator;
        if (qmlAction.specifiedProperty == propertyName) {
            return true;
        }
    }

        return false;
}

QDeclarativeAction &QmlPropertyChangesObject::qmlActionForProperty(const QString &propertyName) const
{
    for(ActionList::iterator actionIterator = m_qmlActionList.begin();
        actionIterator != m_qmlActionList.end();
        ++actionIterator) {
            QDeclarativeAction &qmlAction = *actionIterator;
            if (qmlAction.specifiedProperty == propertyName)
                return qmlAction;
    }

    Q_ASSERT(false);

    return m_qmlActionList[0];
}

void QmlPropertyChangesObject::removeActionForProperty(const QString &propertyName)
{
    QMutableListIterator<QDeclarativeAction> actionIterator(m_qmlActionList);
    while (actionIterator.hasNext()) {
        QDeclarativeAction &qmlAction = actionIterator.next();
        if (qmlAction.specifiedProperty == propertyName)
            actionIterator.remove();
    }

    QMutableListIterator<QDeclarativeSimpleAction> simpleActionIterator(statePrivate()->revertList);
    while (simpleActionIterator.hasNext()) {
        QDeclarativeSimpleAction &qmlSimpleAction = simpleActionIterator.next();
        if (qmlSimpleAction.specifiedProperty == propertyName && qmlSimpleAction.specifiedObject == targetObject()) {
            simpleActionIterator.remove();
        }
    }
}

bool QmlPropertyChangesObject::isActive() const
{
    if (state() && stateGroup())
        return  state()->name() == stateGroup()->state();

    return false;
}
QmlPropertyChangesObject::~QmlPropertyChangesObject()
{
    removeFromStateRevertList();
}

void QmlPropertyChangesObject::removeFromStateRevertList()
{
    if (statePrivate()) {
        QMutableListIterator<QDeclarativeSimpleAction> simpleActionIterator(statePrivate()->revertList);
        while(simpleActionIterator.hasNext()) {
            QDeclarativeSimpleAction &simpleAction = simpleActionIterator.next();
            if (simpleAction.specifiedObject == targetObject()) {
                Q_ASSERT(simpleAction.property.isValid());
                if (simpleAction.binding) {
                    QDeclarativePropertyPrivate::setBinding(simpleAction.property, simpleAction.binding);
                } else if (simpleAction.value.isValid()) {
                    QDeclarativePropertyPrivate::setBinding(simpleAction.property, 0);
                    simpleAction.property.write(simpleAction.value);
                }
                simpleActionIterator.remove();
            }
        }
    }
}

void QmlPropertyChangesObject::addToStateRevertList()
{
    if (isActive() && statePrivate()) {
        QListIterator<QDeclarativeAction> actionIterator(m_qmlActionList);
        while (actionIterator.hasNext()) {
            const QDeclarativeAction &qmlAction = actionIterator.next();
            QDeclarativeSimpleAction simpleAction(qmlAction);
            simpleAction.binding = qmlAction.fromBinding;
            simpleAction.value = qmlAction.fromValue;
            statePrivate()->revertList.append(simpleAction);
            if (qmlAction.toBinding)
                QDeclarativePropertyPrivate::setBinding(qmlAction.property, qmlAction.toBinding);
            else
                qmlAction.property.write(qmlAction.toValue);
        }
    }
}

void QmlPropertyChangesObject::updateRevertValueAndBinding(const QString &name)
{
    if (isActive() && statePrivate()) {
        typedef QList<QDeclarativeSimpleAction> SimpleActionList;
        if (!statePrivate()->revertList.isEmpty()) {
            for(SimpleActionList::iterator actionIterator = statePrivate()->revertList.begin();
            actionIterator != statePrivate()->revertList.end();
            ++actionIterator) {
                //simple action defines values and bindings to revert current state
                QDeclarativeSimpleAction &simpleAction = *actionIterator;
                if (simpleAction.specifiedObject == targetObject()
                    && simpleAction.specifiedProperty == name) {
                    const QDeclarativeAction &qmlAction = qmlActionForProperty(name);
                    simpleAction.value = qmlAction.fromValue;
                    simpleAction.binding = qmlAction.fromBinding;
                    return; //return since we just had to update exisisting simple action
                }
            }
        }

        //simple action does not exist, yet

        const QDeclarativeAction &qmlAction = qmlActionForProperty(name);
        QDeclarativeSimpleAction simpleAction(qmlAction);
        simpleAction.binding = qmlAction.fromBinding;
        simpleAction.value = qmlAction.fromValue;
        statePrivate()->revertList.append(simpleAction);
    }
}

void QmlPropertyChangesObject::setVariantValue(const QString &name, const QVariant & value)
{
    if (!hasActionForProperty(name)) {
        m_qmlActionList.append(createQDeclarativeAction(name));
    }

    QDeclarativeAction &qmlAction = qmlActionForProperty(name);
    qmlAction.toValue = value;
}

void QmlPropertyChangesObject::setExpression(const QString &name, const QString &expression)
{
    if (!hasActionForProperty(name)) {
        m_qmlActionList.append(createQDeclarativeAction(name));
    }

    QDeclarativeAction &qmlAction = qmlActionForProperty(name);
    if (m_expressionHash.contains(name) && m_expressionHash[name].second) {
        m_expressionHash[name].second->destroy();
    }

    QDeclarativeContext *context = QDeclarativeEngine::contextForObject(targetObject());
    QDeclarativeBinding *binding = 0;
    QDeclarativeProperty metaProperty(targetObject(), name, context);
    if (metaProperty.isValid() && metaProperty.isProperty()) {
        binding = new QDeclarativeBinding(expression, targetObject(), context, this);
        binding->setTarget(metaProperty);
        binding->setNotifyOnValueChanged(true);
        qmlAction.toBinding = binding;
        m_expressionHash.insert(name, ExpressionPair(expression, binding));
    } else {
        qWarning() << "Cannot set binding for property" << name << ": property is unknown for type";
    }

    updateRevertValueAndBinding(name);
    if (isActive())
        QDeclarativePropertyPrivate::setBinding(metaProperty, binding,  QDeclarativePropertyPrivate::BypassInterceptor | QDeclarativePropertyPrivate::DontRemoveBinding);
}

void QmlPropertyChangesObject::removeVariantValue(const QString &name)
{
    if (hasActionForProperty(name)) {
        QDeclarativeAction &qmlAction = qmlActionForProperty(name);
        if (hasExpression(name)) {
            qmlAction.toValue = QVariant();
            updateRevertValueAndBinding(name);
        } else {
            if (qmlAction.fromBinding)
                QDeclarativePropertyPrivate::setBinding(qmlAction.property, qmlAction.fromBinding);
            else
                qmlAction.property.write(qmlAction.fromValue);
            removeActionForProperty(name);
        }
    }
}

void QmlPropertyChangesObject::removeExpression(const QString &name)
{
    if (hasActionForProperty(name)) {
        QDeclarativeAction &qmlAction = qmlActionForProperty(name);
        if (hasVariantValue(name)) {
            ExpressionPair expr = m_expressionHash.take(name);
            if (expr.second)
                expr.second.data()->destroy();
            qmlAction.toBinding = 0;
            updateRevertValueAndBinding(name);
            qmlAction.property.write(qmlAction.toValue);
        } else {
            if (qmlAction.fromBinding)
                QDeclarativePropertyPrivate::setBinding(qmlAction.property, qmlAction.fromBinding);
            else
                qmlAction.property.write(qmlAction.fromValue);
            removeActionForProperty(name);
        }
    }
}

void QmlPropertyChangesObject::resetProperty(const QString &name)
{
    if (statePrivate()) {
        QMutableListIterator<QDeclarativeSimpleAction> simpleActionIterator(statePrivate()->revertList);
        while (simpleActionIterator.hasNext()) {
            QDeclarativeSimpleAction &qmlSimpleAction = simpleActionIterator.next();
            if (qmlSimpleAction.specifiedProperty == name && qmlSimpleAction.specifiedObject == targetObject()) {
                if (qmlSimpleAction.binding) {
                    qDebug() << qmlSimpleAction.binding->expression();
                    QDeclarativePropertyPrivate::setBinding(qmlSimpleAction.property, qmlSimpleAction.binding);
                } else {
                    QDeclarativePropertyPrivate::setBinding(qmlSimpleAction.property, 0);
                    qmlSimpleAction.property.write(qmlSimpleAction.value);
                }
            }
        }
    }

    if (m_expressionHash.contains(name)) {
        if (m_expressionHash[name].second)
            m_expressionHash[name].second.data()->destroy();
        m_expressionHash.remove(name);
    }

    if (hasActionForProperty(name)) {
        removeActionForProperty(name);
    }


}

QVariant QmlPropertyChangesObject::variantValue(const QString &name) const
{
    if (hasActionForProperty(name)) {
        QDeclarativeAction &qmlAction = qmlActionForProperty(name);
        return qmlAction.toValue;
    }

    return QVariant();
}

QString QmlPropertyChangesObject::expression(const QString &name) const
{
    if (hasActionForProperty(name)) {
        QDeclarativeAction &qmlAction = qmlActionForProperty(name);
        if (qmlAction.toBinding)
            return qmlAction.toBinding->expression();
    }

    return QString();
}

bool QmlPropertyChangesObject::hasVariantValue(const QString &name) const
{
    if (hasActionForProperty(name))
        return qmlActionForProperty(name).toValue.isValid();

    return false;
}

bool QmlPropertyChangesObject::hasExpression(const QString &name) const
{
    return m_expressionHash.contains(name);
}

bool QmlPropertyChangesObject::updateStateVariant(const QString &name, const QVariant &value)
{
    if (hasActionForProperty(name)) {
        QDeclarativeAction &qmlAction = qmlActionForProperty(name);
        if (qmlAction.fromValue.isValid()) {
            qmlAction.fromValue = value;
            updateRevertValueAndBinding(name);

            return true;
        }
    }

    return false;
}

bool QmlPropertyChangesObject::updateStateBinding(const QString &name, const QString &expression)
{
    if (hasActionForProperty(name)) {
        QDeclarativeContext *context = QDeclarativeEngine::contextForObject(targetObject());
        QDeclarativeProperty metaProperty(targetObject(), name, context);
        QDeclarativeAction &qmlAction = qmlActionForProperty(name);

        if (qmlAction.fromBinding) {
            if (QDeclarativePropertyPrivate::binding(qmlAction.property) == qmlAction.fromBinding)
                QDeclarativePropertyPrivate::setBinding(qmlAction.property, 0);
            qmlAction.fromBinding->destroy();

        }

        QDeclarativeBinding *binding = new QDeclarativeBinding(expression, targetObject(), context, this);
        binding->setTarget(metaProperty);
        binding->setNotifyOnValueChanged(true);
        qmlAction.fromBinding = binding;

        if (m_expressionHash.contains(name))
            QDeclarativePropertyPrivate::setBinding(qmlAction.property, binding);

        updateRevertValueAndBinding(name);

        return true;
    }

    return false;
}

bool QmlPropertyChangesObject::resetStateProperty(const QString &name, const QVariant &resetValue)
{
    if (hasActionForProperty(name)) {
        QDeclarativeContext *context = QDeclarativeEngine::contextForObject(targetObject());
        QDeclarativeProperty metaProperty(targetObject(), name, context);
        QDeclarativeAction &qmlAction = qmlActionForProperty(name);
        if (m_expressionHash.contains(name) && m_expressionHash[name].second) {
            m_expressionHash[name].second.data()->destroy();
            qmlAction.toBinding = 0;
        }

        qmlAction.fromValue = resetValue;

        updateRevertValueAndBinding(name);

        removeActionForProperty(name);

        return true;
    }

    return false;
}

} // namespace Internal
} // namespace QmlDesigner
