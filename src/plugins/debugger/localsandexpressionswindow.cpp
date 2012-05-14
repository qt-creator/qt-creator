#include "localsandexpressionswindow.h"
#include <QVBoxLayout>
#include <QSplitter>
#include <QStackedWidget>

namespace Debugger {
namespace Internal {

LocalsAndExpressionsWindow::LocalsAndExpressionsWindow(
        QWidget *locals, QWidget *inspector, QWidget *returnWidget,
        QWidget *watchers, QWidget *parent)
    : QWidget(parent)
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setMargin(0);
    layout->setSpacing(0);

    m_splitter = new QSplitter(Qt::Vertical);
    layout->addWidget(m_splitter);

    m_localsAndInspector = new QStackedWidget();
    m_localsAndInspector->addWidget(locals);
    m_localsAndInspector->addWidget(inspector);
    m_localsAndInspector->setCurrentWidget(inspector);

    m_splitter->addWidget(m_localsAndInspector);
    m_splitter->addWidget(returnWidget);
    m_splitter->addWidget(watchers);

    m_splitter->setStretchFactor(0, 3);
    m_splitter->setStretchFactor(2, 1);
    m_splitter->setStretchFactor(3, 1);
}

void LocalsAndExpressionsWindow::setShowLocals(bool showLocals)
{
    m_localsAndInspector->setCurrentIndex(showLocals ? 0 : 1);
}

} // namespace Internal
} // namespace Debugger
