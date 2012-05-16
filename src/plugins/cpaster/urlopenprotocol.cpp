#include "urlopenprotocol.h"

#include <utils/qtcassert.h>

#include <QFileInfo>
#include <QNetworkReply>

using namespace CodePaster;

UrlOpenProtocol::UrlOpenProtocol(const NetworkAccessManagerProxyPtr &nw)
    : NetworkProtocol(nw), m_fetchReply(0)
{
}

QString UrlOpenProtocol::name() const
{
    return QLatin1String("Open URL"); // unused
}

unsigned UrlOpenProtocol::capabilities() const
{
    return 0;
}

void UrlOpenProtocol::fetch(const QString &url)
{
    QTC_ASSERT(!m_fetchReply, return);
    m_fetchReply = httpGet(url);
    connect(m_fetchReply, SIGNAL(finished()), this, SLOT(fetchFinished()));
}

void UrlOpenProtocol::fetchFinished()
{
    const QString title = m_fetchReply->url().toString();
    QString content;
    const bool error = m_fetchReply->error();
    if (error) {
        content = m_fetchReply->errorString();
    } else {
        content = QString::fromUtf8(m_fetchReply->readAll());
    }
    m_fetchReply->deleteLater();
    m_fetchReply = 0;
    emit fetchDone(title, content, error);
}

void UrlOpenProtocol::paste(const QString &, ContentType, const QString &,
                            const QString &, const QString &)
{
}
