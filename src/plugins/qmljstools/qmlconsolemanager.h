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

#ifndef QMLCONSOLEMANAGER_H
#define QMLCONSOLEMANAGER_H

#include "qmljstools_global.h"

#include <qmljs/consolemanagerinterface.h>

#include <QObject>

namespace QmlJS {
class IScriptEvaluator;
}
namespace QmlJSTools {

namespace Internal {
class QmlConsoleModel;
}

class QmlConsoleManagerPrivate;
class QMLJSTOOLS_EXPORT QmlConsoleManager : public QmlJS::ConsoleManagerInterface
{
    Q_OBJECT
public:
    QmlConsoleManager(QObject *parent);
    ~QmlConsoleManager();

    void showConsolePane();

    QmlJS::ConsoleItem *rootItem() const;

    void setScriptEvaluator(QmlJS::IScriptEvaluator *scriptEvaluator);
    void setContext(const QString &context);

    void printToConsolePane(QmlJS::ConsoleItem::ItemType itemType, const QString &text,
                            bool bringToForeground = false);
    void printToConsolePane(QmlJS::ConsoleItem *item, bool bringToForeground = false);

private:
    QmlConsoleManagerPrivate *d;
    friend class Internal::QmlConsoleModel;
};

} // namespace QmlJSTools

#endif // QMLCONSOLEMANAGER_H
