/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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
#include <private/qmlstateoperations_p.h>

namespace QmlDesigner {

namespace Internal {

class QmlPropertyChangesNodeInstance;

// Original QmlPropertyChanges class requires a custom parser
// work around this by writing a replacement class
class QmlPropertyChangesObject : public QmlStateOperation
{
    Q_OBJECT
    Q_PROPERTY(QObject *target READ object WRITE setObject)
    Q_PROPERTY(bool restoreEntryValues READ restoreEntryValues WRITE setRestoreEntryValues)
    Q_PROPERTY(bool explicit READ isExplicit WRITE setIsExplicit)

public:
    QObject *object() const { return m_targetObject.data(); }
    void setObject(QObject *object) {m_targetObject = object; }

    bool restoreEntryValues() const { return m_restoreEntryValues; }
    void setRestoreEntryValues(bool restore) { m_restoreEntryValues = restore; }

    bool isExplicit() const { return m_isExplicit; }
    void setIsExplicit(bool isExplicit) { m_isExplicit = isExplicit; }

    virtual ActionList actions();

private:
    friend class QmlPropertyChangesNodeInstance;

    QmlPropertyChangesObject();
    QmlMetaProperty metaProperty(const QString &property);

    QWeakPointer<QObject> m_targetObject;
    bool m_restoreEntryValues;
    bool m_isExplicit;

    QHash<QString, QVariant> m_properties;
    QHash<QString, QString> m_expressions;
//    QList<QmlReplaceSignalHandler*> signalReplacements;
};

class QmlPropertyChangesNodeInstance : public ObjectNodeInstance
{
public:
    typedef QSharedPointer<QmlPropertyChangesNodeInstance> Pointer;
    typedef QWeakPointer<QmlPropertyChangesNodeInstance> WeakPointer;

    static Pointer create(const NodeMetaInfo &metaInfo, QmlContext *context, QObject *objectToBeWrapped);

    virtual void setPropertyVariant(const QString &name, const QVariant &value);
    virtual void setPropertyBinding(const QString &name, const QString &expression);
    virtual QVariant property(const QString &name) const;
    virtual void resetProperty(const QString &name);

    void updateStateInstance() const;

protected:
    QmlPropertyChangesNodeInstance(QmlPropertyChangesObject *object);
    QmlPropertyChangesObject *changesObject() const;
};

} // namespace Internal
} // namespace QmlDesigner

QT_BEGIN_NAMESPACE
QML_DECLARE_TYPE(QmlDesigner::Internal::QmlPropertyChangesObject)
QT_END_NAMESPACE

#endif // QMLPROPERTYCHANGESNODEINSTANCE_H
