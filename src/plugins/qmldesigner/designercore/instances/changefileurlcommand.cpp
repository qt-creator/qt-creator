#include "changefileurlcommand.h"

namespace QmlDesigner {

ChangeFileUrlCommand::ChangeFileUrlCommand()
{
}

ChangeFileUrlCommand::ChangeFileUrlCommand(const QUrl &fileUrl)
    : m_fileUrl(fileUrl)
{
}

QUrl ChangeFileUrlCommand::fileUrl() const
{
    return m_fileUrl;
}

QDataStream &operator<<(QDataStream &out, const ChangeFileUrlCommand &command)
{
    out << command.fileUrl();

    return out;
}

QDataStream &operator>>(QDataStream &in, ChangeFileUrlCommand &command)
{
    in >> command.m_fileUrl;

    return in;
}

} // namespace QmlDesigner
