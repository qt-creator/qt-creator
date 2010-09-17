#include "%ObjectName:l%.%CppHeaderSuffix%"

#include <QtCore/QTime>
#include <QtDeclarative/qdeclarative.h>

%ObjectName%::%ObjectName%(QObject *parent):
        QObject(parent)
{
    timer = new QTimer(this);
    timer->setInterval(750);
    connect(timer, SIGNAL(timeout()), this, SLOT(timerFired()));
    timer->start();
}

QString %ObjectName%::text() const
{
    return theText;
}

void %ObjectName%::setText(const QString &text)
{
    if (theText != text) {
        theText = text;
        emit textChanged(theText);
    }
}

void %ObjectName%::timerFired()
{
    QTime t = QTime::currentTime();
    setText(t.toString(QLatin1String("HH:mm:ss")));
}

QML_DECLARE_TYPE(%ObjectName%);
