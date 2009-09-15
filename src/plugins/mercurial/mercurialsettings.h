#ifndef MERCURIALSETTINGS_H
#define MERCURIALSETTINGS_H

#include <QtCore/QString>
#include <QtCore/QStringList>

namespace Mercurial {
namespace Internal {

class MercurialSettings
{
public:
    MercurialSettings();

    QString binary();
    QString application();
    QStringList standardArguments();
    QString userName();
    QString email();
    int logCount();
    int timeout();
    int timeoutSeconds();
    bool prompt();
    void writeSettings(const QString &application, const QString &userName,
                       const QString &email, int logCount, int timeout, bool prompt);
private:

    void readSettings();
    void setBinAndArgs();

    QString bin; // used because windows requires cmd.exe to run the mercurial binary
                 // in this case the actual mercurial binary will be part of the standard args
    QString app; // this is teh actual mercurial executable
    QStringList standardArgs;
    QString user;
    QString mail;
    int m_logCount;
    int m_timeout;
    bool m_prompt;
};

} //namespace Internal
} //namespace Mercurial

#endif // MERCURIALSETTINGS_H
