#ifndef KEYWORD_H
#define KEYWORD_H

#include <QIcon>
#include <QColor>
#include <QString>
#include <QList>
#include <QMetaType>

class Keyword
{
public:
    Keyword();
    Keyword(QString name_, QIcon icon_, QColor warningColor_);
    QString name;
    QIcon icon;
    QColor warningColor;
};

typedef QList<Keyword> KeywordsList;

QDataStream &operator<<(QDataStream &out, const Keyword &myObj);
QDataStream &operator>>(QDataStream &in,  Keyword &myObj);


Q_DECLARE_METATYPE(KeywordsList)
Q_DECLARE_METATYPE(Keyword)

#endif // KEYWORD_H
