/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#pragma once

#include "qtsupport_global.h"

#include <QObject>
#include <QStringList>

namespace QtSupport {

class QTSUPPORT_EXPORT CodeGenerator : public QObject
{
    Q_OBJECT

public:
    CodeGenerator(QObject *parent = 0) : QObject(parent) { }

    // Ui file related:
    // Change the class name in a UI XML form
    Q_INVOKABLE static QString changeUiClassName(const QString &uiXml, const QString &newUiClassName);

    // Low level method to get everything at the same time:
    static bool uiData(const QString &uiXml, QString *formBaseClass, QString *uiClassName);

    Q_INVOKABLE static QString uiClassName(const QString &uiXml);

    // Generic Qt:
    Q_INVOKABLE static QString qtIncludes(const QStringList &qt4, const QStringList &qt5);
};

} // namespace QtSupport
