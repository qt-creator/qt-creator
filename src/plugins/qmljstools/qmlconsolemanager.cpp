/**************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
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

#include <extensionsystem/pluginmanager.h>

#include <debugger/debuggerengine.h>

#include <QScriptEngine>
#include <QVariant>

namespace QmlJSTools {

QmlConsoleManager *QmlConsoleManager::m_instance = 0;

class QmlConsoleManagerPrivate
{
public:
    QScriptEngine *scriptEngine;
    Internal::QmlConsoleItemModel *qmlConsoleItemModel;
    Internal::QmlConsolePane *qmlConsolePane;
    Debugger::DebuggerEngine *debuggerEngine;
};

QmlConsoleManager::QmlConsoleManager(QObject *parent)
    : QObject(parent),
      d(new QmlConsoleManagerPrivate)
{
    m_instance = this;
    d->scriptEngine = new QScriptEngine(this);
    d->qmlConsoleItemModel = new Internal::QmlConsoleItemModel(this);
    d->qmlConsoleItemModel->setHasEditableRow(true);
    d->qmlConsolePane = new Internal::QmlConsolePane(this);
    ExtensionSystem::PluginManager::addObject(d->qmlConsolePane);
    d->debuggerEngine = 0;
}

QmlConsoleManager::~QmlConsoleManager()
{
    if (d->qmlConsolePane) {
        ExtensionSystem::PluginManager::removeObject(d->qmlConsolePane);
    }
    delete d;
    m_instance = 0;
}

void QmlConsoleManager::showConsolePane()
{
    if (d->qmlConsolePane)
        d->qmlConsolePane->popup(Core::IOutputPane::ModeSwitch);
}

QmlConsoleItem *QmlConsoleManager::rootItem() const
{
    return d->qmlConsoleItemModel->root();
}

void QmlConsoleManager::setDebuggerEngine(Debugger::DebuggerEngine *debuggerEngine)
{
    d->debuggerEngine = debuggerEngine;
    if (!debuggerEngine)
        setContext(QString());
}

void QmlConsoleManager::setContext(const QString &context)
{
    d->qmlConsolePane->setContext(context);
}

void QmlConsoleManager::printToConsolePane(QmlConsoleItem::ItemType itemType,
                                           const QString &text, bool bringToForeground)
{
    if (!d->qmlConsolePane)
        return;
    if (itemType == QmlConsoleItem::ErrorType)
        bringToForeground = true;
    if (bringToForeground)
        d->qmlConsolePane->popup(Core::IOutputPane::ModeSwitch);
    d->qmlConsoleItemModel->appendMessage(itemType, text);
}

void QmlConsoleManager::printToConsolePane(QmlConsoleItem *item, bool bringToForeground)
{
    if (!d->qmlConsolePane)
        return;
    if (item->itemType == QmlConsoleItem::ErrorType)
        bringToForeground = true;
    if (bringToForeground)
        d->qmlConsolePane->popup(Core::IOutputPane::ModeSwitch);
    d->qmlConsoleItemModel->appendItem(item);
}

namespace Internal {

QmlConsoleItem *constructLogItemTree(QmlConsoleItem *parent, const QVariant &result,
                                     const QString &key = QString())
{
    if (!result.isValid())
        return 0;

    QmlConsoleItem *item = new QmlConsoleItem(parent);
    if (result.type() == QVariant::Map) {
        if (key.isEmpty())
            item->setText(QLatin1String("Object"));
        else
            item->setText(key + QLatin1String(" : Object"));

        QMapIterator<QString, QVariant> i(result.toMap());
        while (i.hasNext()) {
            i.next();
            QmlConsoleItem *child = constructLogItemTree(item, i.value(), i.key());
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
            QmlConsoleItem *child = constructLogItemTree(item, resultList.at(i),
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
    QmlConsoleManager *manager = QmlConsoleManager::instance();
    if (manager)
        return manager->d->qmlConsoleItemModel;
    return 0;
}

void QmlConsoleModel::evaluate(const QString &expression)
{
    QmlConsoleManager *manager = QmlConsoleManager::instance();
    if (manager) {
        if (manager->d->debuggerEngine) {
            QmlConsoleModel::qmlConsoleItemModel()->appendEditableRow();
            manager->d->debuggerEngine->evaluateScriptExpression(expression);
        } else {
            QVariant result = manager->d->scriptEngine->evaluate(expression).toVariant();
            QmlConsoleItem *root = manager->rootItem();
            QmlConsoleItem *item = constructLogItemTree(root, result);
            if (item) {
                QmlConsoleModel::qmlConsoleItemModel()->appendEditableRow();
                manager->printToConsolePane(item);
            }
        }
    }
}

} // Internal
} // QmlJSTools
