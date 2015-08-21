/**************************************************************************
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

#include "qmlconsolemanager.h"
#include "qmlconsolepane.h"
#include "qmlconsoleitemmodel.h"
#include "qmlconsolemodel.h"

#include <extensionsystem/pluginmanager.h>

#include <qmljs/iscriptevaluator.h>

#include <QVariant>
#include <QCoreApplication>

using namespace QmlJS;

namespace QmlJSTools {

class QmlConsoleManagerPrivate
{
public:
    Internal::QmlConsoleItemModel *qmlConsoleItemModel;
    Internal::QmlConsolePane *qmlConsolePane;
    IScriptEvaluator *scriptEvaluator;
};

QmlConsoleManager::QmlConsoleManager(QObject *parent)
    : ConsoleManagerInterface(parent),
      d(new QmlConsoleManagerPrivate)
{
    d->qmlConsoleItemModel = new Internal::QmlConsoleItemModel(this);
    d->qmlConsolePane = new Internal::QmlConsolePane(this);
    d->scriptEvaluator = 0;
    ExtensionSystem::PluginManager::addObject(d->qmlConsolePane);
}

QmlConsoleManager::~QmlConsoleManager()
{
    if (d->qmlConsolePane)
        ExtensionSystem::PluginManager::removeObject(d->qmlConsolePane);
    delete d;
}

void QmlConsoleManager::showConsolePane()
{
    if (d->qmlConsolePane)
        d->qmlConsolePane->popup(Core::IOutputPane::ModeSwitch);
}

void QmlConsoleManager::setScriptEvaluator(IScriptEvaluator *scriptEvaluator)
{
    d->scriptEvaluator = scriptEvaluator;
    if (!scriptEvaluator)
        setContext(QString());
}

void QmlConsoleManager::setContext(const QString &context)
{
    d->qmlConsolePane->setContext(context);
}

void QmlConsoleManager::printToConsolePane(ConsoleItem::ItemType itemType,
                                           const QString &text, bool bringToForeground)
{
    if (!d->qmlConsolePane)
        return;
    if (itemType == ConsoleItem::ErrorType)
        bringToForeground = true;
    if (bringToForeground)
        d->qmlConsolePane->popup(Core::IOutputPane::ModeSwitch);
    d->qmlConsoleItemModel->appendMessage(itemType, text);
}

void QmlConsoleManager::printToConsolePane(ConsoleItem *item, bool bringToForeground)
{
    if (!d->qmlConsolePane)
        return;
    if (item->itemType() == ConsoleItem::ErrorType)
        bringToForeground = true;
    if (bringToForeground)
        d->qmlConsolePane->popup(Core::IOutputPane::ModeSwitch);
    d->qmlConsoleItemModel->appendItem(item);
}

namespace Internal {

QmlConsoleItemModel *QmlConsoleModel::qmlConsoleItemModel()
{
    QmlConsoleManager *manager = qobject_cast<QmlConsoleManager *>(QmlConsoleManager::instance());
    if (manager)
        return manager->d->qmlConsoleItemModel;
    return 0;
}

void QmlConsoleModel::evaluate(const QString &expression)
{
    QmlConsoleManager *manager = qobject_cast<QmlConsoleManager *>(QmlConsoleManager::instance());
    if (manager) {
        if (manager->d->scriptEvaluator) {
            QmlConsoleModel::qmlConsoleItemModel()->shiftEditableRow();
            manager->d->scriptEvaluator->evaluateScript(expression);
        } else {
            ConsoleItem *item = new ConsoleItem(
                        ConsoleItem::ErrorType, QCoreApplication::translate(
                            "QmlJSTools::Internal::QmlConsoleModel",
                            "Can only evaluate during a QML debug session."));
            if (item) {
                QmlConsoleModel::qmlConsoleItemModel()->shiftEditableRow();
                manager->printToConsolePane(item);
            }
        }
    }
}

} // Internal
} // QmlJSTools
