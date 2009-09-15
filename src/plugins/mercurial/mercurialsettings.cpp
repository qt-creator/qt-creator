#include "mercurialsettings.h"
#include "constants.h"

#include <coreplugin/icore.h>

#include <QtCore/QSettings>


using namespace Mercurial::Internal;

MercurialSettings::MercurialSettings()
{
    readSettings();
}

QString MercurialSettings::binary()
{
    return bin;
}

QString MercurialSettings::application()
{
    return app;
}

QStringList MercurialSettings::standardArguments()
{
    return standardArgs;
}

QString MercurialSettings::userName()
{
    return user;
}

QString MercurialSettings::email()
{
    return mail;
}

int MercurialSettings::logCount()
{
    return m_logCount;
}

int MercurialSettings::timeout()
{
    //return timeout is in Ms
    return m_timeout * 1000;
}

int MercurialSettings::timeoutSeconds()
{
    //return timeout in seconds (as the user specifies on the options page
    return m_timeout;
}

bool MercurialSettings::prompt()
{
    return m_prompt;
}

void MercurialSettings::writeSettings(const QString &application, const QString &userName,
                                      const QString &email, int logCount, int timeout, bool prompt)
{
    QSettings *settings = Core::ICore::instance()->settings();
    if (settings) {
        settings->beginGroup("Mercurial");
        settings->setValue(Constants::MERCURIALPATH, application);
        settings->setValue(Constants::MERCURIALUSERNAME, userName);
        settings->setValue(Constants::MERCURIALEMAIL, email);
        settings->setValue(Constants::MERCURIALLOGCOUNT, logCount);
        settings->setValue(Constants::MERCURIALTIMEOUT, timeout);
        settings->setValue(Constants::MERCURIALPROMPTSUBMIT, prompt);
        settings->endGroup();
    }

    app = application;
    user = userName;
    mail = email;
    m_logCount = logCount;
    m_timeout = timeout;
    m_prompt = prompt;
    setBinAndArgs();
}

void MercurialSettings::readSettings()
{
    QSettings *settings = Core::ICore::instance()->settings();
    if (settings) {
        settings->beginGroup("Mercurial");
        app = settings->value(Constants::MERCURIALPATH, Constants::MERCURIALDEFAULT).toString();
        user = settings->value(Constants::MERCURIALUSERNAME, "").toString();
        mail = settings->value(Constants::MERCURIALEMAIL, "").toString();
        m_logCount = settings->value(Constants::MERCURIALLOGCOUNT, 0).toInt();
        m_timeout = settings->value(Constants::MERCURIALTIMEOUT, 30).toInt();
        m_prompt = settings->value(Constants::MERCURIALPROMPTSUBMIT, true).toBool();
        settings->endGroup();
    }

    setBinAndArgs();
}

void MercurialSettings::setBinAndArgs()
{
    standardArgs.clear();

#ifdef Q_OS_WIN
    bin = QLatin1String("cmd.exe");
    standardArgs << "/c" << app;
#else
    bin = app;
#endif
}
