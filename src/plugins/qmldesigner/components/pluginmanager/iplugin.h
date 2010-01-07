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

#ifndef IPLUGIN_H
#define IPLUGIN_H

#include <QtCore/QObject>

#define QMLDESIGNER_PLUGIN_INTERFACE "com.Nokia.QmlDesigner.IPlugin.v10"

namespace QmlDesigner {

// QmlDesigner "base" plugin with initialization method in which
// it can retriece the core via its static accessor and do magic.

class IPlugin
{
public:
    virtual ~IPlugin() {}

    virtual bool isInitialized() const = 0;

    virtual bool initialize(QString *errorMessage) = 0;
};

} // namespace QmlDesigner

Q_DECLARE_INTERFACE(QmlDesigner::IPlugin, QMLDESIGNER_PLUGIN_INTERFACE)

#endif // IPLUGIN_H
