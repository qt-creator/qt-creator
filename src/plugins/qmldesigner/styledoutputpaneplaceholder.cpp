#include "styledoutputpaneplaceholder.h"

#include <QtCore/QChildEvent>
#include <QtCore/QFile>
#include <QtGui/QTabWidget>
#include <QtGui/QStackedWidget>
#include <QDebug>

StyledOutputpanePlaceHolder::StyledOutputpanePlaceHolder(Core::IMode *mode, QSplitter *parent) : Core::OutputPanePlaceHolder(mode, parent)
{
    QFile file(":/qmldesigner/outputpane-style.css");
    file.open(QFile::ReadOnly);
    QFile file2(":/qmldesigner/scrollbar.css");
    file2.open(QFile::ReadOnly);
    m_customStylesheet = file.readAll() + file2.readAll();
    file.close();
    file2.close();
}

void StyledOutputpanePlaceHolder::childEvent(QChildEvent *event)
{
    Core::OutputPanePlaceHolder::childEvent(event);

    if (event->type() == QEvent::ChildAdded) {
        QWidget *child = qobject_cast<QWidget*>(event->child());
        if (child) {
            QList<QTabWidget*> widgets = child->findChildren<QTabWidget*>();
            if (!widgets.isEmpty()) {
                widgets.first()->parentWidget()->ensurePolished();
                widgets.first()->parentWidget()->setStyleSheet(m_customStylesheet);
            }

        }
    } else if (event->type() == QEvent::ChildRemoved) {
        QWidget *child = qobject_cast<QWidget*>(event->child());
        if (child) {
            QList<QTabWidget*> widgets = child->findChildren<QTabWidget*>();
            if (!widgets.isEmpty())
                widgets.first()->parentWidget()->setStyleSheet(QString());
        }
    }
}
