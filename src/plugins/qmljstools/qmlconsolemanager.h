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
