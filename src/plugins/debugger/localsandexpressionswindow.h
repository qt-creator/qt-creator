#ifndef LOCALSANDEXPRESSIONSWINDOW_H
#define LOCALSANDEXPRESSIONSWINDOW_H

#include <QWidget>

QT_BEGIN_NAMESPACE
class QSplitter;
class QStackedWidget;
QT_END_NAMESPACE

namespace Debugger {
namespace Internal {

class LocalsAndExpressionsWindow : public QWidget
{
    Q_OBJECT
public:
    explicit LocalsAndExpressionsWindow(
            QWidget *locals, QWidget *inspector,
            QWidget *returnWidget, QWidget *watchers, QWidget *parent = 0);

    void setShowLocals(bool showLocals);

private:
    QSplitter *m_splitter;
    QStackedWidget *m_localsAndInspector;
};

} // namespace Internal
} // namespace Debugger

#endif // LOCALSANDEXPRESSIONSWINDOW_H
