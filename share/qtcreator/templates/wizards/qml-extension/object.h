#ifndef %ObjectName:u%_H
#define %ObjectName:u%_H

#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QTimer>

class %ObjectName% : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString text READ text WRITE setText NOTIFY textChanged)

public:
    %ObjectName%(QObject *parent = 0);

    QString text() const;
    void setText(const QString &text);

signals:
    void textChanged(const QString &newText);

private slots:
    void timerFired();

private:
    QString theText;
    QTimer *timer;

    Q_DISABLE_COPY(%ObjectName%)
};

#endif // %ObjectName:u%_H
