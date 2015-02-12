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

#ifndef PROPERTYEDITORCONTEXTOBJECT_H
#define PROPERTYEDITORCONTEXTOBJECT_H

#include <QObject>
#include <QUrl>
#include <QQmlPropertyMap>
#include <QQmlComponent>
#include <QColor>

namespace QmlDesigner {

class PropertyEditorContextObject : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QUrl globalBaseUrl READ globalBaseUrl WRITE setGlobalBaseUrl NOTIFY globalBaseUrlChanged)
    Q_PROPERTY(QUrl specificsUrl READ specificsUrl WRITE setSpecificsUrl NOTIFY specificsUrlChanged)

    Q_PROPERTY(QString specificQmlData READ specificQmlData WRITE setSpecificQmlData NOTIFY specificQmlDataChanged)
    Q_PROPERTY(QString stateName READ stateName WRITE setStateName NOTIFY stateNameChanged)

    Q_PROPERTY(bool isBaseState READ isBaseState WRITE setIsBaseState NOTIFY isBaseStateChanged)
    Q_PROPERTY(bool selectionChanged READ selectionChanged WRITE setSelectionChanged NOTIFY selectionChangedChanged)

    Q_PROPERTY(int majorVersion READ majorVersion WRITE setMajorVersion NOTIFY majorVersionChanged)
    Q_PROPERTY(int minorVersion READ minorVersion WRITE setMinorVersion NOTIFY minorVersionChanged)
    Q_PROPERTY(int majorQtQuickVersion READ majorQtQuickVersion WRITE setMajorQtQuickVersion NOTIFY majorQtQuickVersionChanged)

    Q_PROPERTY(QQmlPropertyMap* backendValues READ backendValues WRITE setBackendValues NOTIFY backendValuesChanged)

    Q_PROPERTY(QQmlComponent* specificQmlComponent READ specificQmlComponent NOTIFY specificQmlComponentChanged)

public:
    PropertyEditorContextObject(QObject *parent = 0);

    QUrl globalBaseUrl() const {return m_globalBaseUrl; }
    QUrl specificsUrl() const {return m_specificsUrl; }
    QString specificQmlData() const {return m_specificQmlData; }
    QString stateName() const {return m_stateName; }

    bool isBaseState() const { return m_isBaseState; }
    bool selectionChanged() const { return m_selectionChanged; }

    QQmlPropertyMap* backendValues() const { return m_backendValues; }

    Q_INVOKABLE QString convertColorToString(const QColor &color);
    Q_INVOKABLE QColor colorFromString(const QString &colorString);

    int majorVersion() const;
    int majorQtQuickVersion() const;
    void setMajorVersion(int majorVersion);
    void setMajorQtQuickVersion(int majorVersion);
    int minorVersion() const;
    void setMinorVersion(int minorVersion);

    void insertInQmlContext(QQmlContext *context);
    QQmlComponent *specificQmlComponent();

signals:
    void globalBaseUrlChanged();
    void specificsUrlChanged();
    void specificQmlDataChanged();
    void stateNameChanged();
    void isBaseStateChanged();
    void selectionChangedChanged();
    void backendValuesChanged();
    void majorVersionChanged();
    void minorVersionChanged();
    void majorQtQuickVersionChanged();
    void specificQmlComponentChanged();

public slots:
     void setGlobalBaseUrl(const QUrl &newBaseUrl);

     void setSpecificsUrl(const QUrl &newSpecificsUrl);

     void setSpecificQmlData(const QString &newSpecificQmlData);

     void setStateName(const QString &newStateName);

     void setIsBaseState(bool newIsBaseState);

     void setSelectionChanged(bool newSelectionChanged);

     void setBackendValues(QQmlPropertyMap* newBackendValues);

    void triggerSelectionChanged();

private:
    QUrl m_globalBaseUrl;
    QUrl m_specificsUrl;

    QString m_specificQmlData;
    QString m_stateName;

    bool m_isBaseState;
    bool m_selectionChanged;

    QQmlPropertyMap* m_backendValues;

    int m_majorVersion;
    int m_minorVersion;
    int m_majorQtQuickVersion;
    QQmlComponent *m_qmlComponent;
    QQmlContext *m_qmlContext;
};

} //QmlDesigner {

#endif // PROPERTYEDITORCONTEXTOBJECT_H
