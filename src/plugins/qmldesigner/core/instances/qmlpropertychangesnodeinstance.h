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

#ifndef QMLPROPERTYCHANGESNODEINSTANCE_H
#define QMLPROPERTYCHANGESNODEINSTANCE_H

#include "objectnodeinstance.h"
#include <private/qdeclarativestateoperations_p.h>

#include <QPair>
#include <QWeakPointer>

QT_BEGIN_NAMESPACE
class QDeclarativeProperty;
QT_END_NAMESPACE

namespace QmlDesigner {

namespace Internal {

class QmlPropertyChangesNodeInstance;

// Original QmlPropertyChanges class requires a custom parser
// work around this by writing a replacement class
class QmlPropertyChangesObject : public QDeclarativeStateOperation
{
    Q_OBJECT
    Q_PROPERTY(QObject *target READ targetObject WRITE setTargetObject)
    Q_PROPERTY(bool restoreEntryValues READ restoreEntryValues WRITE setRestoreEntryValues)
    Q_PROPERTY(bool explicit READ isExplicit WRITE setIsExplicit)

    typedef QPair<QString, QWeakPointer<QDeclarativeBinding> >  ExpressionPair;
public:
    ~QmlPropertyChangesObject();
    QObject *targetObject() const;
    void setTargetObject(QObject *object);

    bool restoreEntryValues() const;
    void setRestoreEntryValues(bool restore);

    bool isExplicit() const;
    void setIsExplicit(bool isExplicit);

    virtual ActionList actions();

    void setVariantValue(const QString &name, const QVariant & value);
    void setExpression(const QString &name, const QString &expression);
    void removeVariantValue(const QString &name);
    void removeExpression(const QString &name);

    void resetProperty(const QString &name);

    QVariant variantValue(const QString &name) const;
    QString expression(const QString &name) const;

    bool hasVariantValue(const QString &name) const;
    bool hasExpression(const QString &name) const;

    QmlPropertyChangesObject();

    bool updateStateVariant(const QString &propertyName, const QVariant &value);
    bool updateStateBinding(const QString &propertyName, const QString &expression);
    bool resetStateProperty(const QString &propertyName, const QVariant &resetValue);

    QDeclarativeState *state() const;
    void updateRevertValueAndBinding(const QString &name);

    void removeFromStateRevertList();
    void addToStateRevertList();

private: // functions
    bool isActive() const;

    QDeclarativeStatePrivate *statePrivate() const;

    QDeclarativeStateGroup *stateGroup() const;
    QDeclarativeProperty createMetaProperty(const QString &property);

    QDeclarativeAction &qmlActionForProperty(const QString &propertyName) const;
    bool hasActionForProperty(const QString &propertyName) const;
    void removeActionForProperty(const QString &propertyName);

    QDeclarativeAction createQDeclarativeAction(const QString &propertyName);

private: // variables
    QWeakPointer<QObject> m_targetObject;
    bool m_restoreEntryValues;
    bool m_isExplicit;

    mutable ActionList m_qmlActionList;
    QHash<QString, ExpressionPair> m_expressionHash;
};

class QmlPropertyChangesNodeInstance : public ObjectNodeInstance
{
public:
    typedef QSharedPointer<QmlPropertyChangesNodeInstance> Pointer;
    typedef QWeakPointer<QmlPropertyChangesNodeInstance> WeakPointer;

    static Pointer create(const NodeMetaInfo &metaInfo, QDeclarativeContext *context, QObject *objectToBeWrapped);

    virtual void setPropertyVariant(const QString &name, const QVariant &value);
    virtual void setPropertyBinding(const QString &name, const QString &expression);
    virtual QVariant property(const QString &name) const;
    virtual void resetProperty(const QString &name);

    void reparent(const NodeInstance &oldParentInstance, const QString &oldParentProperty, const NodeInstance &newParentInstance, const QString &newParentProperty);

protected:
    QmlPropertyChangesNodeInstance(QmlPropertyChangesObject *object);
    QmlPropertyChangesObject *changesObject() const;
};

} // namespace Internal
} // namespace QmlDesigner

QML_DECLARE_TYPE(QmlDesigner::Internal::QmlPropertyChangesObject)

#endif // QMLPROPERTYCHANGESNODEINSTANCE_H
