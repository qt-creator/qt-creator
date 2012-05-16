#ifndef SIMPLENETWORKPROTOCOL_H
#define SIMPLENETWORKPROTOCOL_H

#include "protocol.h"

namespace CodePaster {

class UrlOpenProtocol : public NetworkProtocol
{
    Q_OBJECT
public:
    UrlOpenProtocol(const NetworkAccessManagerProxyPtr &nw);

    QString name() const;
    unsigned capabilities() const;
    void fetch(const QString &url);
    void paste(const QString &, ContentType, const QString &, const QString &, const QString &);

private slots:
    void fetchFinished();

private:
    QNetworkReply *m_fetchReply;
};

}

#endif // SIMPLENETWORKPROTOCOL_H
