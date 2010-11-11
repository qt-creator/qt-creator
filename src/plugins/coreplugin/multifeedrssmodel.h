#ifndef MULTIFEEDRSSMODEL_H
#define MULTIFEEDRSSMODEL_H

#include "core_global.h"

#include <QtCore/QAbstractListModel>
#include <QtCore/QStringList>
#include <QtCore/QDateTime>

QT_BEGIN_NAMESPACE
class QThread;
class QNetworkReply;
QT_END_NAMESPACE

namespace Core {

namespace Internal {

struct RssItem {
    QString source;
    QString title;
    QString link;
    QString description;
    QString blogName;
    QString blogIcon;
    QDateTime pubDate;

};
typedef QList<RssItem> RssItemList;

} // namespace Internal

class NetworkAccessManager;

enum RssRoles { TitleRole = Qt::UserRole+1, DescriptionRole, LinkRole,
                PubDateRole, BlogNameRole, BlogIconRole };

class CORE_EXPORT MultiFeedRssModel : public QAbstractListModel {
    Q_OBJECT
    Q_PROPERTY(int articleCount READ articleCount WRITE setArticleCount NOTIFY articleCountChanged)
public:
    explicit MultiFeedRssModel(QObject *parent);
    ~MultiFeedRssModel();
    void addFeed(const QString& feed);
    void removeFeed(const QString& feed);

    virtual int rowCount(const QModelIndex &parent = QModelIndex()) const;
    virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;

    int articleCount() const { return m_articleCount; }

public slots:
    void setArticleCount(int arg)
    {
        if (m_articleCount != arg) {
            m_articleCount = arg;
            emit articleCountChanged(arg);
        }
    }

signals:
    void articleCountChanged(int arg);

private slots:
    void appendFeedData(QNetworkReply *reply);

private:
    QStringList m_sites;
    Internal::RssItemList m_aggregatedFeed;
    NetworkAccessManager *m_networkAccessManager;
    QThread *m_namThread;
    int m_articleCount;
};

} // namespace Utils

#endif // MULTIFEEDRSSMODEL_H


