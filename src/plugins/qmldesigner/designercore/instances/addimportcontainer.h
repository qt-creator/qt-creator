#ifndef ADDIMPORTCONTAINER_H
#define ADDIMPORTCONTAINER_H

#include <QMetaType>
#include <QUrl>
#include <QString>
#include <QStringList>

namespace QmlDesigner {

class AddImportContainer
{
    friend QDataStream &operator>>(QDataStream &in, AddImportContainer &command);
public:
    AddImportContainer();
    AddImportContainer(const QUrl &url, const QString &fileName, const QString &version, const QString &alias, const QStringList &mportPathList);

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

QDataStream &operator<<(QDataStream &out, const AddImportContainer &command);
QDataStream &operator>>(QDataStream &in, AddImportContainer &command);

} // namespace QmlDesigner

Q_DECLARE_METATYPE(QmlDesigner::AddImportContainer)

#endif // ADDIMPORTCONTAINER_H
