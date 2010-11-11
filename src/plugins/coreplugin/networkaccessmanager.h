#include "core_global.h"

#include <QtCore/QUrl>
#include <QtNetwork/QNetworkAccessManager>

namespace Core {

class CORE_EXPORT NetworkAccessManager : public QNetworkAccessManager
{
    Q_OBJECT
public:
    NetworkAccessManager(QObject *parent = 0);

public slots:
    void getUrl(const QUrl &url);

protected:
    virtual QNetworkReply* createRequest(Operation op, const QNetworkRequest &request, QIODevice *outgoingData);
};


} // namespace utils
