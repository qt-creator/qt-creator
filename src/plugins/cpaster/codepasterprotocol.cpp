/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "codepasterprotocol.h"
#include "codepastersettings.h"
#include "cpasterplugin.h"
#include "cgi.h"

#include <coreplugin/coreconstants.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/icore.h>
#include <coreplugin/messagemanager.h>
#include <coreplugin/messageoutputwindow.h>

#include <utils/qtcassert.h>

#include <QtGui/QListWidget>
#include <QtNetwork/QNetworkReply>
#include <QtCore/QDebug>

enum { debug = 0 };

namespace CodePaster {

CodePasterProtocol::CodePasterProtocol(const NetworkAccessManagerProxyPtr &nw) :
    NetworkProtocol(nw),
    m_page(new CodePaster::CodePasterSettingsPage),
    m_pasteReply(0),
    m_fetchReply(0),
    m_listReply(0),
    m_fetchId(-1)
{
}

CodePasterProtocol::~CodePasterProtocol()
{
}

QString CodePasterProtocol::name() const
{
    return QLatin1String("CodePaster");
}

unsigned CodePasterProtocol::capabilities() const
{
    return ListCapability|PostCommentCapability|PostDescriptionCapability;
}

bool CodePasterProtocol::isValidHostName(const QString& hostName)
{
    if (hostName.isEmpty()) {
        Core::ICore::instance()->messageManager()->printToOutputPane(
#ifdef Q_OS_MAC
                       tr("No Server defined in the CodePaster preferences."),
#else
                       tr("No Server defined in the CodePaster options."),
#endif
                       true /*error*/);
        Core::ICore::instance()->messageManager()->showOutputPane();
        return false;
    }
    return true;
}

void CodePasterProtocol::fetch(const QString &id)
{
    QTC_ASSERT(!m_fetchReply, return; )

    QString hostName = m_page->hostName();
    if (!isValidHostName(hostName))
        return;
    QString link = "http://";
    link.append(hostName);
    link.append("/?format=raw&id=");
    link.append(id);
    m_fetchReply = httpGet(link);
    connect(m_fetchReply, SIGNAL(finished()), this, SLOT(fetchFinished()));
    m_fetchId = id;
}

void CodePasterProtocol::list()
{
    QTC_ASSERT(!m_listReply, return; )

    QString hostName = m_page->hostName();
    if (!isValidHostName(hostName))
        return;
    QString link = QLatin1String("http://");
    link += hostName;
    link += QLatin1String("/?command=browse&format=raw");
    m_listReply = httpGet(link);
    connect(m_listReply, SIGNAL(finished()), this, SLOT(listFinished()));
}

void CodePasterProtocol::paste(const QString &text,
                               ContentType /* ct */,
                               const QString &username,
                               const QString &comment,
                               const QString &description)
{
    QTC_ASSERT(!m_pasteReply, return; )
    const QString hostName = m_page->hostName();
    if (!isValidHostName(hostName))
        return;

    QByteArray data = "command=processcreate&submit=submit&highlight_type=0&description=";
    data += CGI::encodeURL(description).toLatin1();
    data += "&comment=";
    data += CGI::encodeURL(comment).toLatin1();
    data += "&code=";
    data += CGI::encodeURL(fixNewLines(text)).toLatin1();
    data += "&poster=";
    data += CGI::encodeURL(username).toLatin1();

    m_pasteReply = httpPost(QLatin1String("http://") + hostName, data);
    connect(m_pasteReply, SIGNAL(finished()), this, SLOT(pasteFinished()));
}

void CodePasterProtocol::pasteFinished()
{
    if (m_pasteReply->error()) {
        qWarning("Error pasting: %s", qPrintable(m_pasteReply->errorString()));
    } else {
        // Cut out the href-attribute
        QString contents = QString::fromAscii(m_pasteReply->readAll());
        int hrefPos = contents.indexOf(QLatin1String("href=\""));
        if (hrefPos != -1) {
            hrefPos += 6;
            const int endPos = contents.indexOf(QLatin1Char('"'), hrefPos);
            if (endPos != -1)
                emit pasteDone(contents.mid(hrefPos, endPos - hrefPos));
        }
    }
    m_pasteReply->deleteLater();
    m_pasteReply = 0;
}

bool CodePasterProtocol::hasSettings() const
{
    return true;
}

Core::IOptionsPage *CodePasterProtocol::settingsPage()
{
    return m_page;
}

void CodePasterProtocol::fetchFinished()
{
    QString title;
    QString content;
    bool error = m_fetchReply->error();
    if (error) {
        content = m_fetchReply->errorString();
    } else {
        content = m_fetchReply->readAll();
        if (debug)
            qDebug() << content;
        if (content.contains("<B>No such paste!</B>")) {
            content = tr("No such paste");
            error = true;
        }
        title = QString::fromLatin1("Codepaster: %1").arg(m_fetchId);
    }
    m_fetchReply->deleteLater();
    m_fetchReply = 0;
    emit fetchDone(title, content, error);
}

void CodePasterProtocol::listFinished()
{
    if (m_listReply->error()) {
        Core::ICore::instance()->messageManager()->printToOutputPane(m_listReply->errorString(), true);
    } else {
        const QByteArray data = m_listReply->readAll();
        const QStringList lines = QString::fromAscii(data).split(QLatin1Char('\n'));
        emit listDone(name(), lines);
    }
    m_listReply->deleteLater();
    m_listReply = 0;
}

} // namespace CodePaster
