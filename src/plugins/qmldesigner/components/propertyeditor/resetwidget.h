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

#ifndef ResetWidget_h
#define ResetWidget_h

#include <QGroupBox>
#include <QPushButton>

QT_BEGIN_NAMESPACE
class QListWidget;
class QVBoxLayout;
class QTableWidget;
QT_END_NAMESPACE

namespace QmlDesigner {

class ResetWidget : public QGroupBox
{

    Q_OBJECT

    Q_PROPERTY(QObject *backendObject READ backendObject WRITE setBackendObject)

public slots:
    void resetView();

public:
    ResetWidget(QWidget *parent = 0);
    QObject* backendObject()
    {
        return m_backendObject;
    }

    void setBackendObject(QObject* backendObject)
    {
        m_backendObject = backendObject;
        setupView();
    }

    static void registerDeclarativeType();

public slots:
    void buttonPressed(const QString &name);


private:
    void setupView();

    void addPropertyItem(const QString &name, int row);

    QObject* m_backendObject;
    QVBoxLayout *m_vlayout;
    QTableWidget *m_tableWidget;

};

class ResetWidgetPushButton : public QPushButton
{
    Q_OBJECT

public slots:
    void myPressed();
    void setName(const QString &name)
    { m_name = name; }

signals:
    void pressed(const QString &name);

public:
    ResetWidgetPushButton(QWidget *parent = 0);
private:
    QString m_name;

};

}

#endif //ResetWidget_h

