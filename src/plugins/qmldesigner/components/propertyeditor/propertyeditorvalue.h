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

#ifndef PROPERTYEDITORVALUE_H
#define PROPERTYEDITORVALUE_H

#include <QObject>
#include <QDeclarativePropertyMap>
#include <qdeclarative.h>
#include <modelnode.h>
#include <qmlitemnode.h>
#include <rewritertransaction.h>

class PropertyEditorValue;

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
    QmlDesigner::ModelNode parentModelNode() const;
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
    Q_PROPERTY(QVariant value READ value WRITE setValueWithEmit NOTIFY valueChangedQml)
    Q_PROPERTY(QString expression READ expression WRITE setExpressionWithEmit NOTIFY expressionChanged FINAL)
    Q_PROPERTY(QString valueToString READ valueToString NOTIFY valueChangedQml FINAL)
    Q_PROPERTY(bool isInModel READ isInModel NOTIFY valueChangedQml FINAL)
    Q_PROPERTY(bool isInSubState READ isInSubState NOTIFY valueChangedQml FINAL)
    Q_PROPERTY(bool isBound READ isBound NOTIFY isBoundChanged FINAL)
    Q_PROPERTY(bool isValid READ isValid NOTIFY isValidChanged FINAL)
    Q_PROPERTY(bool isTranslated READ isTranslated NOTIFY valueChangedQml FINAL)

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

    QString valueToString() const;

    bool isInSubState() const;

    bool isInModel() const;

    bool isBound() const;
    bool isValid() const;

    void setIsValid(bool valid);

    bool isTranslated() const;

    QString name() const;
    void setName(const QString &name);

    QmlDesigner::ModelNode modelNode() const;
    void setModelNode(const QmlDesigner::ModelNode &modelNode);

    PropertyEditorNodeWrapper* complexNode();

    static void registerDeclarativeTypes();

public slots:
    void resetValue();

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
    QString m_name;
    bool m_isInSubState;
    bool m_isInModel;
    bool m_isBound;
    bool m_isValid; // if the property value belongs to a non-existing complexProperty it is invalid
    PropertyEditorNodeWrapper *m_complexNode;
};

QML_DECLARE_TYPE(PropertyEditorValue)
QML_DECLARE_TYPE(PropertyEditorNodeWrapper)
QML_DECLARE_TYPE(QDeclarativePropertyMap)


#endif // PROPERTYEDITORVALUE_H
