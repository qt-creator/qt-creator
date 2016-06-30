#pragma once

#include <QObject>
#include <QString>

class QTimer;

class MyType : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString timeText READ timeText WRITE setTimeText NOTIFY timeChanged)

public:
    MyType(QObject *parent = 0);

    QString timeText() const;
    void setTimeText(const QString &text);

signals:
    void timeChanged(const QString &newText);

private:
    void updateTimerText();

    QString m_timeText;
    QTimer *m_timer;

    Q_DISABLE_COPY(MyType)
};
