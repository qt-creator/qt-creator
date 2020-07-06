/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
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

#ifndef ACTIONEDITOR_H
#define ACTIONEDITOR_H

#include <bindingeditor/bindingeditordialog.h>
#include <qmldesignercorelib_global.h>
#include <modelnode.h>

#include <QtQml>
#include <QObject>
#include <QPointer>

namespace QmlDesigner {

class ActionEditor : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString text READ bindingValue WRITE setBindingValue)

public:
    ActionEditor(QObject *parent = nullptr);
    ~ActionEditor();

    static void registerDeclarativeType();

    Q_INVOKABLE void showWidget();
    Q_INVOKABLE void showWidget(int x, int y);
    Q_INVOKABLE void hideWidget();

    QString bindingValue() const;
    void setBindingValue(const QString &text);

    bool hasModelIndex() const;
    void resetModelIndex();
    QModelIndex modelIndex() const;
    void setModelIndex(const QModelIndex &index);

    Q_INVOKABLE void updateWindowName();

signals:
    void accepted();
    void rejected();

private:
    QVariant backendValue() const;
    QVariant modelNodeBackend() const;
    QVariant stateModelNode() const;
    void prepareDialog();

private:
    QPointer<BindingEditorDialog> m_dialog;
    QModelIndex m_index;
};

}

QML_DECLARE_TYPE(QmlDesigner::ActionEditor)

#endif //ACTIONEDITOR_H
