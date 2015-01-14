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

#ifndef PROPERTYEDITORVALUE_H
#define PROPERTYEDITORVALUE_H

#include <qmldesignercorelib_global.h>

#include <QObject>
#include <QQmlPropertyMap>
#include <QtQml>
#include <modelnode.h>
#include <enumeration.h>

class PropertyEditorValue;

class PropertyEditorNodeWrapper : public QObject {
    Q_OBJECT

    Q_PROPERTY(bool exists READ exists NOTIFY existsChanged)
    Q_PROPERTY(QQmlPropertyMap* properties READ properties NOTIFY propertiesChanged)
    Q_PROPERTY(QString type READ type NOTIFY typeChanged)

public:
    PropertyEditorNodeWrapper(QObject *parent=0);
    PropertyEditorNodeWrapper(PropertyEditorValue* parent);
    bool exists();
    QString type();
    QQmlPropertyMap* properties();
    QmlDesigner::ModelNode parentModelNode() const;
    QmlDesigner::PropertyName propertyName() const;

public slots:
    void add(const QString &type = QString());
    void remove();
    void changeValue(const QString &propertyName);
    void update();

signals:
    void existsChanged();
    void propertiesChanged();
    void typeChanged();

private:
    void setup();

    QmlDesigner::ModelNode m_modelNode;
    QQmlPropertyMap m_valuesPropertyMap;
    PropertyEditorValue* m_editorValue;
};

class PropertyEditorValue : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QVariant value READ value WRITE setValueWithEmit NOTIFY valueChangedQml)
    Q_PROPERTY(QVariant enumeration READ enumeration NOTIFY valueChangedQml)
    Q_PROPERTY(QString expression READ expression WRITE setExpressionWithEmit NOTIFY expressionChanged FINAL)
    Q_PROPERTY(QString valueToString READ valueToString NOTIFY valueChangedQml FINAL)
    Q_PROPERTY(bool isInModel READ isInModel NOTIFY valueChangedQml FINAL)
    Q_PROPERTY(bool isInSubState READ isInSubState NOTIFY valueChangedQml FINAL)
    Q_PROPERTY(bool isBound READ isBound NOTIFY isBoundChanged FINAL)
    Q_PROPERTY(bool isValid READ isValid NOTIFY isValidChanged FINAL)
    Q_PROPERTY(bool isTranslated READ isTranslated NOTIFY expressionChanged FINAL)

    Q_PROPERTY(QString name READ name FINAL)
    Q_PROPERTY(PropertyEditorNodeWrapper* complexNode READ complexNode NOTIFY complexNodeChanged FINAL)

public:
    PropertyEditorValue(QObject *parent=0);

    QVariant value() const;
    void setValueWithEmit(const QVariant &value);
    void setValue(const QVariant &value);

    QString enumeration() const;

    QString expression() const;
    void setExpressionWithEmit(const QString &expression);
    void setExpression(const QString &expression);

    QString valueToString() const;

    bool isInSubState() const;

    bool isInModel() const;

    bool isBound() const;
    bool isValid() const;

    void setIsValid(bool valid);

    bool isTranslated() const;

    QmlDesigner::PropertyName name() const;
    void setName(const QmlDesigner::PropertyName &name);

    QmlDesigner::ModelNode modelNode() const;
    void setModelNode(const QmlDesigner::ModelNode &modelNode);

    PropertyEditorNodeWrapper* complexNode();

    static void registerDeclarativeTypes();

public slots:
    void resetValue();
    void setEnumeration(const QString &scope, const QString &name);

signals:
    void valueChanged(const QString &name, const QVariant&);
    void valueChangedQml();

    void expressionChanged(const QString &name);

    void modelStateChanged();
    void modelNodeChanged();
    void complexNodeChanged();
    void isBoundChanged();
    void isValidChanged();

private: //variables
    QmlDesigner::ModelNode m_modelNode;
    QVariant m_value;
    QString m_expression;
    QmlDesigner::PropertyName m_name;
    bool m_isInSubState;
    bool m_isInModel;
    bool m_isBound;
    bool m_isValid; // if the property value belongs to a non-existing complexProperty it is invalid
    PropertyEditorNodeWrapper *m_complexNode;
};

QML_DECLARE_TYPE(PropertyEditorValue)
QML_DECLARE_TYPE(PropertyEditorNodeWrapper)
QML_DECLARE_TYPE(QQmlPropertyMap)


#endif // PROPERTYEDITORVALUE_H
