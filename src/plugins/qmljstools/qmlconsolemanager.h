/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**************************************************************************/

#ifndef QMLCONSOLEMANAGER_H
#define QMLCONSOLEMANAGER_H

#include "qmljstools_global.h"
#include "qmlconsoleitem.h"

#include <QObject>

namespace Debugger {
class DebuggerEngine;
}
namespace QmlJSTools {

namespace Internal {
class QmlConsoleItemModel;
class QmlConsoleModel;
}

class QmlConsoleManagerPrivate;
class QMLJSTOOLS_EXPORT QmlConsoleManager : public QObject
{
    Q_OBJECT
public:
    QmlConsoleManager(QObject *parent);
    ~QmlConsoleManager();

    static QmlConsoleManager *instance() { return m_instance; }

    void showConsolePane();

    QmlConsoleItem *rootItem() const;

    void setDebuggerEngine(Debugger::DebuggerEngine *debuggerEngine);
    void setContext(const QString &context);

    void printToConsolePane(QmlConsoleItem::ItemType itemType, const QString &text,
                            bool bringToForeground = false);
    void printToConsolePane(QmlConsoleItem *item, bool bringToForeground = false);

private:
    QmlConsoleManagerPrivate *d;
    static QmlConsoleManager *m_instance;
    friend class Internal::QmlConsoleModel;
};

namespace Internal {

class QmlConsoleModel
{
public:
    static QmlConsoleItemModel *qmlConsoleItemModel();
    static void evaluate(const QString &expression);
};

}

} // namespace QmlJSTools

#endif // QMLCONSOLEMANAGER_H
