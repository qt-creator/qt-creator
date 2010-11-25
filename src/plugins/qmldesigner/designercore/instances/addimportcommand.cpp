#include "addimportcommand.h"

namespace QmlDesigner {

AddImportCommand::AddImportCommand()
{
}

AddImportCommand::AddImportCommand(const QUrl &url, const QString &fileName, const QString &version, const QString &alias, const QStringList &importPathList)
    : m_url(url),
      m_fileName(fileName),
      m_version(version),
      m_alias(alias),
      m_importPathList(importPathList)
{
}

QUrl AddImportCommand::url() const
{
    return m_url;
}

QString AddImportCommand::fileName() const
{
    return m_fileName;
}

QString AddImportCommand::version() const
{
    return m_version;
}

QString AddImportCommand::alias() const
{
    return m_alias;
}

QStringList AddImportCommand::importPaths() const
{
    return m_importPathList;
}

QDataStream &operator<<(QDataStream &out, const AddImportCommand &command)
{
    out << command.url();
    out << command.fileName();
    out << command.version();
    out << command.alias();
    out << command.importPaths();

    return out;
}

QDataStream &operator>>(QDataStream &in, AddImportCommand &command)
{
    in >> command.m_url;
    in >> command.m_fileName;
    in >> command.m_version;
    in >> command.m_alias;
    in >> command.m_importPathList;

    return in;
}

} // namespace QmlDesigner
