#include "keyword.h"

Keyword::Keyword()
{
}

Keyword::Keyword(QString name_, QIcon icon_, QColor warningColor_) :
    name(name_), icon(icon_), warningColor(warningColor_)
{
}

QDataStream &operator<<(QDataStream &out, const Keyword &myObj)
{
    out << myObj.name;
    out << myObj.icon;
    out << myObj.warningColor;
    return out;
}

QDataStream &operator>>(QDataStream &in,  Keyword &myObj)
{
    in >> myObj.name;
    in >> myObj.icon;
    in >> myObj.warningColor;
    return in;
}
