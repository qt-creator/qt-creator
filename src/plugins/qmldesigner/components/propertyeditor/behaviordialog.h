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

#ifndef BEHAVIORDIALOG_H
#define BEHAVIORDIALOG_H

#include <modelnode.h>
#include <propertyeditorvalue.h>

#include <QPushButton>
#include <QDialog>
#include <QWeakPointer>
#include <QScopedPointer>
#include <qdeclarative.h>

#include "ui_behaviordialog.h"

namespace QmlDesigner {

class BehaviorDialog;

class BehaviorWidget : public QPushButton
{
    Q_PROPERTY(PropertyEditorNodeWrapper* complexNode READ complexNode WRITE setComplexNode)

    Q_OBJECT

public:
    explicit BehaviorWidget(QWidget *parent = 0);

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
    explicit BehaviorDialog(QWidget *parent = 0);
    void setup(const ModelNode &node, const QString propertyName);

public slots:
    virtual void accept();
    virtual void reject();

    static void registerDeclarativeType();

private:
    ModelNode m_modelNode;
    QString m_propertyName;
    QScopedPointer<Internal::Ui::BehaviorDialog> m_ui;
};


}

QML_DECLARE_TYPE(QmlDesigner::BehaviorWidget)

#endif// BEHAVIORDIALOG_H
