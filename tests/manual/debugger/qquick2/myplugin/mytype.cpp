#include "mytype.h"

#include <QTimer>
#include <QTime>
#include <qqml.h>

MyType::MyType(QObject *parent)
    : QObject(parent)
{
    updateTimerText();
    m_timer = new QTimer(this);
    m_timer->setInterval(1000);
    connect(m_timer, &QTimer::timeout, this, &MyType::updateTimerText);
    m_timer->start();
}

QString MyType::timeText() const
{
    // bp here should be hit on every second
    return m_timeText;
}

void MyType::setTimeText(const QString &text)
{
    if (m_timeText != text) {
        m_timeText = text;
        emit timeChanged(m_timeText);
    }
}

void MyType::updateTimerText()
{
    const QTime t = QTime::currentTime();
    setTimeText(t.toString(QLatin1String("HH:mm:ss")));
}

QML_DECLARE_TYPE(MyType)
