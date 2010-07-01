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

#include "qmljsinspectorcontext.h"
#include "qmljsinspectorconstants.h"

#include <coreplugin/icore.h>
#include <QtGui/QWidget>
#include <QtCore/QDebug>

using namespace QmlJSInspector::Internal;
using namespace QmlJSInspector::Constants;

InspectorContext::InspectorContext(QWidget *widget)
  : IContext(widget),
<<<<<<< HEAD:src/plugins/qmljsinspector/qmljsinspectorcontext.cpp
    m_widget(widget),
    m_context(C_INSPECTOR)
=======
    m_context(Constants::C_INSPECTOR),
    m_widget(widget)
>>>>>>> 6dc4a039f2cb50a02a0e15265548547b94a6dc9d:src/plugins/qmlinspector/inspectorcontext.cpp
{
}

InspectorContext::~InspectorContext()
{
}

Core::Context InspectorContext::context() const
{
    return m_context;
}

QWidget *InspectorContext::widget()
{
    return m_widget;
}

void InspectorContext::setContextHelpId(const QString &helpId)
{
    m_contextHelpId = helpId;
}

QString InspectorContext::contextHelpId() const
{
    return m_contextHelpId;
}

QString InspectorContext::contextHelpIdForProperty(const QString &itemName, const QString &propName)
{
    // TODO this functionality is not supported yet as we don't have help id's for
    // properties.
    return QString("QML.").append(itemName).append(".").append(propName);
}

QString InspectorContext::contextHelpIdForItem(const QString &itemName)
{
    return QString("QML.").append(itemName);
}

