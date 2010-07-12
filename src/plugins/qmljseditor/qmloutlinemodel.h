#ifndef QMLOUTLINEMODEL_H
#define QMLOUTLINEMODEL_H

#include <qmljs/qmljsdocument.h>
#include <qmljs/qmljsicons.h>

#include <QStandardItemModel>

namespace QmlJSEditor {
namespace Internal {

class QmlOutlineModel : public QStandardItemModel
{
    Q_OBJECT
public:
    enum CustomRoles {
        SourceLocationRole = Qt::UserRole + 1
    };

    QmlOutlineModel(QObject *parent = 0);

    void update(QmlJS::Document::Ptr doc);

    QModelIndex enterElement(const QString &typeName, const QmlJS::AST::SourceLocation &location);
    void leaveElement();

    QModelIndex enterProperty(const QString &name, const QmlJS::AST::SourceLocation &location);
    void leaveProperty();

signals:
    void updated();

private:
    QStandardItem *enterNode(const QmlJS::AST::SourceLocation &location);
    void leaveNode();

    QStandardItem *parentItem();

    QList<int> m_treePos;
    QStandardItem *m_currentItem;
    QmlJS::Icons m_icons;
};

} // namespace Internal
} // namespace QmlJSEditor

Q_DECLARE_METATYPE(QmlJS::AST::SourceLocation);

#endif // QMLOUTLINEMODEL_H
