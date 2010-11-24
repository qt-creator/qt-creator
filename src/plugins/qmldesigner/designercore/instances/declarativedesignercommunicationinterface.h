#ifndef DECLARATIVEDESIGNERCOMMUNICATIONINTERFACE_H
#define DECLARATIVEDESIGNERCOMMUNICATIONINTERFACE_H

#include <QObject>

class DeclarativeDesignerCommunicationInterface : public QObject
{
    Q_OBJECT
public:
    explicit DeclarativeDesignerCommunicationInterface(QObject *parent = 0);

signals:

public slots:

};

#endif // DECLARATIVEDESIGNERCOMMUNICATIONINTERFACE_H
