/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
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
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef ResetWidget_h
#define ResetWidget_h

#include <QtGui/QGroupBox>
#include <QtGui/QPushButton>

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

