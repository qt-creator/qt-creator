#ifndef INFORMATIONCHANGEDCOMMAND_H
#define INFORMATIONCHANGEDCOMMAND_H

#include <QMetaType>
#include <QVector>

#include "informationcontainer.h"

namespace QmlDesigner {

class InformationChangedCommand
{
    friend QDataStream &operator>>(QDataStream &in, InformationChangedCommand &command);

public:
    InformationChangedCommand();
    InformationChangedCommand(const QVector<InformationContainer> &informationVector);

    QVector<InformationContainer> informations() const;

private:
    QVector<InformationContainer> m_informationVector;
};

QDataStream &operator<<(QDataStream &out, const InformationChangedCommand &command);
QDataStream &operator>>(QDataStream &in, InformationChangedCommand &command);

} // namespace QmlDesigner

Q_DECLARE_METATYPE(QmlDesigner::InformationChangedCommand)

#endif // INFORMATIONCHANGEDCOMMAND_H
