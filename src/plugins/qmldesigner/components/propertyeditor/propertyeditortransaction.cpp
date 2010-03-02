#include "propertyeditortransaction.h"

#include <QDebug>

namespace QmlDesigner {

PropertyEditorTransaction::PropertyEditorTransaction(QmlDesigner::PropertyEditor *propertyEditor) : QObject(propertyEditor), m_propertyEditor(propertyEditor)
{
}

void PropertyEditorTransaction::start()
{
    if (m_rewriterTransaction.isValid())
        m_rewriterTransaction.commit();
    m_rewriterTransaction = m_propertyEditor->beginRewriterTransaction();
    startTimer(4000);
}

void PropertyEditorTransaction::end()
{
    if (m_rewriterTransaction.isValid())
        m_rewriterTransaction.commit();
}

void PropertyEditorTransaction::timerEvent(QTimerEvent *timerEvent)
{
    qDebug() << "timer";
    killTimer(timerEvent->timerId());
    if (m_rewriterTransaction.isValid())
        m_rewriterTransaction.commit();
}

} //QmlDesigner

