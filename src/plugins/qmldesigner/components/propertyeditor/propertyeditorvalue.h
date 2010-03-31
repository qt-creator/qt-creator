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

#ifndef PROPERTYEDITORVALUE_H
#define PROPERTYEDITORVALUE_H

#include <QObject>
#include <QDeclarativePropertyMap>
#include <qdeclarative.h>
#include <modelnode.h>
#include <qmlitemnode.h>
#include <rewritertransaction.h>

class PropertyEditorValue;

typedef QmlDesigner::ModelNode ModelNode;


class PropertyEditorNodeWrapper : public QObject {
    Q_OBJECT

    Q_PROPERTY(bool exists READ exists NOTIFY existsChanged)
    Q_PROPERTY(QDeclarativePropertyMap* properties READ properties NOTIFY propertiesChanged)
    Q_PROPERTY(QString type READ type NOTIFY typeChanged)

public:
    PropertyEditorNodeWrapper(QObject *parent=0);
    PropertyEditorNodeWrapper(PropertyEditorValue* parent);
    bool exists();
    QString type();
    QDeclarativePropertyMap* properties();
    ModelNode parentModelNode() const;
    QString propertyName() const;

public slots:
    void add(const QString &type = QString());
    void remove();
    void changeValue(const QString &name);
    void update();

signals:
    void existsChanged();
    void propertiesChanged();
    void typeChanged();

private:
    void setup();

    QmlDesigner::ModelNode m_modelNode;
    QDeclarativePropertyMap m_valuesPropertyMap;
    PropertyEditorValue* m_editorValue;
};

class PropertyEditorValue : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QVariant value READ value WRITE setValueWithEmit NOTIFY valueChanged)
    Q_PROPERTY(QString expression READ expression WRITE setExpressionWithEmit NOTIFY expressionChanged FINAL)
    Q_PROPERTY(bool isInModel READ isInModel NOTIFY valueChanged FINAL)
    Q_PROPERTY(bool isInSubState READ isInSubState NOTIFY valueChanged FINAL)
    Q_PROPERTY(bool isBound READ isBound NOTIFY isBoundChanged FINAL)
    Q_PROPERTY(bool isValid READ isValid NOTIFY isValid FINAL)
    Q_PROPERTY(QString name READ name FINAL)
    Q_PROPERTY(PropertyEditorNodeWrapper* complexNode READ complexNode NOTIFY complexNodeChanged FINAL)

public:
    PropertyEditorValue(QObject *parent=0);

    QVariant value() const;
    void setValueWithEmit(const QVariant &value);
    void setValue(const QVariant &value);

    QString expression() const;
    void setExpressionWithEmit(const QString &expression);
    void setExpression(const QString &expression);

    bool isInSubState() const;

    bool isInModel() const;

    bool isBound() const;
    bool isValid() const;

    void setIsValid(bool valid);

    QString name() const;
    void setName(const QString &name);

    ModelNode modelNode() const;
    void setModelNode(const ModelNode &modelNode);

    PropertyEditorNodeWrapper* complexNode();

    static void registerDeclarativeTypes();

public slots:
    void resetValue();

signals:
    void valueChanged(const QString &name, const QVariant&);

    void expressionChanged(const QString &name);

    void modelStateChanged();
    void modelNodeChanged();
    void complexNodeChanged();
    void isBoundChanged();
    void isValidChanged();

private: //variables
    ModelNode m_modelNode;
    QVariant m_value;
    QString m_expression;
    QString m_name;
    bool m_isInSubState;
    bool m_isInModel;
    bool m_isBound;
    bool m_isValid; // if the property value belongs to a non-existing complexProperty it is invalid
    PropertyEditorNodeWrapper *m_complexNode;    
};

QML_DECLARE_TYPE(PropertyEditorValue);
QML_DECLARE_TYPE(PropertyEditorNodeWrapper);
QML_DECLARE_TYPE(QDeclarativePropertyMap);


#endif // PROPERTYEDITORVALUE_H
