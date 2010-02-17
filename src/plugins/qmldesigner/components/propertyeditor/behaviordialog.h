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

#ifndef BEHAVIORDIALOG_H
#define BEHAVIORDIALOG_H

#include <modelnode.h>
#include <propertyeditorvalue.h>

#include <QPushButton>
#include <QDialog>
#include <QWeakPointer>
#include <QScopedPointer>
#include <qml.h>

#include "ui_behaviordialog.h"

namespace QmlDesigner {

class BehaviorDialog;

class BehaviorWidget : public QPushButton
{
    Q_PROPERTY(PropertyEditorNodeWrapper* complexNode READ complexNode WRITE setComplexNode)

    Q_OBJECT

public:

    BehaviorWidget();
    BehaviorWidget(QWidget* parent);

    ModelNode modelNode() const {return m_modelNode; }
    QString propertyName() const {return m_propertyName; }

    PropertyEditorNodeWrapper* complexNode() const;
    void setComplexNode(PropertyEditorNodeWrapper* complexNode);

public slots:
    void buttonPressed(bool);

private:
    ModelNode m_modelNode;
    QString m_propertyName;
    PropertyEditorNodeWrapper* m_complexNode;
    QScopedPointer<BehaviorDialog> m_BehaviorDialog;
};

class BehaviorDialog : public QDialog
{
    Q_OBJECT
public:
    BehaviorDialog(QWidget *parent);
    void setup(const ModelNode &node, const QString propertyName);

public slots:
    virtual void accept();
    virtual void reject();


private:
    ModelNode m_modelNode;
    QString m_propertyName;
    QScopedPointer<Ui_BehaviorDialog> m_ui;
};


};

QT_BEGIN_NAMESPACE
QML_DECLARE_TYPE(QmlDesigner::BehaviorWidget);
QT_END_NAMESPACE

#endif// BEHAVIORDIALOG_H
