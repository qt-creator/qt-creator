#ifndef ADDIMPORTCOMMAND_H
#define ADDIMPORTCOMMAND_H

#include <QMetaType>
#include <QUrl>
#include <QString>
#include <QStringList>

namespace QmlDesigner {

class AddImportCommand
{
    friend QDataStream &operator>>(QDataStream &in, AddImportCommand &command);
public:
    AddImportCommand();
    AddImportCommand(const QUrl &url, const QString &fileName, const QString &version, const QString &alias, const QStringList &mportPathList);

    QUrl url() const;
    QString fileName() const;
    QString version() const;
    QString alias() const;
    QStringList importPaths() const;

private:
    QUrl m_url;
    QString m_fileName;
    QString m_version;
    QString m_alias;
    QStringList m_importPathList;
};

QDataStream &operator<<(QDataStream &out, const AddImportCommand &command);
QDataStream &operator>>(QDataStream &in, AddImportCommand &command);

} // namespace QmlDesigner

Q_DECLARE_METATYPE(QmlDesigner::AddImportCommand);

#endif // ADDIMPORTCOMMAND_H
