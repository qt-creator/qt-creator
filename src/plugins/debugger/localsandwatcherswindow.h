#ifndef LOCALSANDWATCHERSWIDGET_H
#define LOCALSANDWATCHERSWIDGET_H

#include <QWidget>

QT_BEGIN_NAMESPACE
class QSplitter;
class QStackedWidget;
QT_END_NAMESPACE

namespace Debugger {
namespace Internal {

class LocalsAndWatchersWindow : public QWidget
{
    Q_OBJECT
public:
    explicit LocalsAndWatchersWindow(
            QWidget *locals, QWidget *inspector,
            QWidget *returnWidget, QWidget *watchers, QWidget *parent = 0);

    void setShowLocals(bool showLocals);

private:
    QSplitter *m_splitter;
    QStackedWidget *m_localsAndInspector;
};

} // namespace Internal
} // namespace Debugger

#endif // LOCALSANDWATCHERSWIDGET_H
