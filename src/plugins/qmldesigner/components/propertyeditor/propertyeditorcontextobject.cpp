/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
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

#include "propertyeditorcontextobject.h"

#include <QQmlContext>

namespace QmlDesigner {

PropertyEditorContextObject::PropertyEditorContextObject(QObject *parent) :
    QObject(parent),
    m_isBaseState(false),
    m_selectionChanged(false),
    m_backendValues(0),
    m_majorVersion(-1),
    m_minorVersion(-1),
    m_majorQtQuickVersion(-1),
    m_qmlComponent(0),
    m_qmlContext(0)
{

}

int PropertyEditorContextObject::majorVersion() const
{
    return m_majorVersion;

}

int PropertyEditorContextObject::majorQtQuickVersion() const
{
      return m_majorQtQuickVersion;
}

void PropertyEditorContextObject::setMajorVersion(int majorVersion)
{
    if (m_majorVersion == majorVersion)
        return;

    m_majorVersion = majorVersion;

    emit majorVersionChanged();
}

void PropertyEditorContextObject::setMajorQtQuickVersion(int majorVersion)
{
    if (m_majorQtQuickVersion == majorVersion)
        return;

    m_majorQtQuickVersion = majorVersion;

    emit majorQtQuickVersionChanged();

}

int PropertyEditorContextObject::minorVersion() const
{
    return m_minorVersion;
}

void PropertyEditorContextObject::setMinorVersion(int minorVersion)
{
    if (m_minorVersion == minorVersion)
        return;

    m_minorVersion = minorVersion;

    emit minorVersionChanged();
}

void PropertyEditorContextObject::insertInQmlContext(QQmlContext *context)
{
    m_qmlContext = context;
    m_qmlContext->setContextObject(this);
}

QQmlComponent *PropertyEditorContextObject::specificQmlComponent()
{
    if (m_qmlComponent)
        return m_qmlComponent;

    m_qmlComponent = new QQmlComponent(m_qmlContext->engine(), this);

    m_qmlComponent->setData(m_specificQmlData.toUtf8(), QUrl::fromLocalFile(QStringLiteral("specfics.qml")));

    return m_qmlComponent;
}

void PropertyEditorContextObject::setGlobalBaseUrl(const QUrl &newBaseUrl)
{
    if (newBaseUrl == m_globalBaseUrl)
        return;

    m_globalBaseUrl = newBaseUrl;
    emit globalBaseUrlChanged();
}

void PropertyEditorContextObject::setSpecificsUrl(const QUrl &newSpecificsUrl)
{
    if (newSpecificsUrl == m_specificsUrl)
        return;

    m_specificsUrl = newSpecificsUrl;
    emit specificsUrlChanged();
}

void PropertyEditorContextObject::setSpecificQmlData(const QString &newSpecificQmlData)
{
    if (m_specificQmlData == newSpecificQmlData)
        return;

    m_specificQmlData = newSpecificQmlData;

    delete m_qmlComponent;
    m_qmlComponent = 0;

    emit specificQmlComponentChanged();
    emit specificQmlDataChanged();
}

void PropertyEditorContextObject::setStateName(const QString &newStateName)
{
    if (newStateName == m_stateName)
        return;

    m_stateName = newStateName;
    emit stateNameChanged();
}

void PropertyEditorContextObject::setIsBaseState(bool newIsBaseState)
{
    if (newIsBaseState ==  m_isBaseState)
        return;

    m_isBaseState = newIsBaseState;
    emit isBaseStateChanged();
}

void PropertyEditorContextObject::setSelectionChanged(bool newSelectionChanged)
{
    if (newSelectionChanged ==  m_selectionChanged)
        return;

    m_selectionChanged = newSelectionChanged;
    emit selectionChangedChanged();
}

void PropertyEditorContextObject::setBackendValues(QQmlPropertyMap *newBackendValues)
{
    if (newBackendValues ==  m_backendValues)
        return;

    m_backendValues = newBackendValues;
    emit backendValuesChanged();
}

void PropertyEditorContextObject::triggerSelectionChanged()
{
    setSelectionChanged(!m_selectionChanged);
}

} //QmlDesigner
