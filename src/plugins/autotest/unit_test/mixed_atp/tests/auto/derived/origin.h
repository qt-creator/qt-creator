#ifndef ORIGIN_H
#define ORIGIN_H

#include <QObject>

class Origin : public QObject
{
    Q_OBJECT
public:
    Origin();
private slots:
    void testSchmu();
    void testStr();
    void testStr_data();
    void testBase1_data(); // makes no sense - but should be handled correctly
};

#endif // ORIGIN_H
