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
