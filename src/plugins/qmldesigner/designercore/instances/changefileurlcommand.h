#ifndef CHANGEFILEURLCOMMAND_H
#define CHANGEFILEURLCOMMAND_H

#include <qmetatype.h>
#include <QUrl>

namespace QmlDesigner {

class ChangeFileUrlCommand
{
    friend QDataStream &operator>>(QDataStream &in, ChangeFileUrlCommand &command);
public:
    ChangeFileUrlCommand();
    ChangeFileUrlCommand(const QUrl &fileUrl);

    QUrl fileUrl() const;

private:
    QUrl m_fileUrl;
};


QDataStream &operator<<(QDataStream &out, const ChangeFileUrlCommand &command);
QDataStream &operator>>(QDataStream &in, ChangeFileUrlCommand &command);

} // namespace QmlDesigner

Q_DECLARE_METATYPE(QmlDesigner::ChangeFileUrlCommand);

#endif // CHANGEFILEURLCOMMAND_H
