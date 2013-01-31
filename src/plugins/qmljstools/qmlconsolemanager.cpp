/**************************************************************************
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
    QmlJS::IScriptEvaluator *scriptEvaluator;
};

QmlConsoleManager::QmlConsoleManager(QObject *parent)
    : ConsoleManagerInterface(parent),
      d(new QmlConsoleManagerPrivate)
{
    d->qmlConsoleItemModel = new Internal::QmlConsoleItemModel(this);
    d->qmlConsoleItemModel->setHasEditableRow(true);
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

ConsoleItem *QmlConsoleManager::rootItem() const
{
    return d->qmlConsoleItemModel->root();
}

void QmlConsoleManager::setScriptEvaluator(QmlJS::IScriptEvaluator *scriptEvaluator)
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
    if (item->itemType == ConsoleItem::ErrorType)
        bringToForeground = true;
    if (bringToForeground)
        d->qmlConsolePane->popup(Core::IOutputPane::ModeSwitch);
    d->qmlConsoleItemModel->appendItem(item);
}

namespace Internal {

ConsoleItem *constructLogItemTree(ConsoleItem *parent, const QVariant &result,
                                     const QString &key = QString())
{
    if (!result.isValid())
        return 0;

    ConsoleItem *item = new ConsoleItem(parent);
    if (result.type() == QVariant::Map) {
        if (key.isEmpty())
            item->setText(QLatin1String("Object"));
        else
            item->setText(key + QLatin1String(" : Object"));

        QMapIterator<QString, QVariant> i(result.toMap());
        while (i.hasNext()) {
            i.next();
            ConsoleItem *child = constructLogItemTree(item, i.value(), i.key());
            if (child)
                item->insertChild(child, true);
        }
    } else if (result.type() == QVariant::List) {
        if (key.isEmpty())
            item->setText(QLatin1String("List"));
        else
            item->setText(QString(QLatin1String("[%1] : List")).arg(key));
        QVariantList resultList = result.toList();
        for (int i = 0; i < resultList.count(); i++) {
            ConsoleItem *child = constructLogItemTree(item, resultList.at(i),
                                                          QString::number(i));
            if (child)
                item->insertChild(child, true);
        }
    } else if (result.canConvert(QVariant::String)) {
        item->setText(result.toString());
    } else {
        item->setText(QLatin1String("Unknown Value"));
    }

    return item;
}

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
            QmlConsoleModel::qmlConsoleItemModel()->appendEditableRow();
            manager->d->scriptEvaluator->evaluateScript(expression);
        } else {
            ConsoleItem *root = manager->rootItem();
            ConsoleItem *item = constructLogItemTree(
                        root, QCoreApplication::translate("QmlJSTools::Internal::QmlConsoleModel",
                                                          "Can only evaluate during a QML debug session."));
            if (item) {
                QmlConsoleModel::qmlConsoleItemModel()->appendEditableRow();
                manager->printToConsolePane(item);
            }
        }
    }
}

} // Internal
} // QmlJSTools
