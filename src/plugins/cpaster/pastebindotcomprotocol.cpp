/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "pastebindotcomprotocol.h"

#include <coreplugin/icore.h>

#include <utils/qtcassert.h>

#include <QDebug>
#include <QStringList>
#include <QTextStream>
#include <QXmlStreamReader>
#include <QXmlStreamAttributes>
#include <QByteArray>

#include <QNetworkReply>

enum { debug = 0 };

static const char PASTEBIN_BASE[]="http://pastebin.com/";
static const char PASTEBIN_API[]="api/api_post.php";
static const char PASTEBIN_RAW[]="raw.php";
static const char PASTEBIN_ARCHIVE[]="archive";

static const char API_KEY[]="api_dev_key=516686fc461fb7f9341fd7cf2af6f829&"; // user: qtcreator_apikey

namespace CodePaster {
PasteBinDotComProtocol::PasteBinDotComProtocol(const NetworkAccessManagerProxyPtr &nw) :
    NetworkProtocol(nw),
    m_fetchReply(0),
    m_pasteReply(0),
    m_listReply(0),
    m_fetchId(-1),
    m_postId(-1),
    m_hostChecked(false)
{
}

QString PasteBinDotComProtocol::protocolName()
{
    return QLatin1String("Pastebin.Com");
}

unsigned PasteBinDotComProtocol::capabilities() const
{
    return ListCapability;
}

static inline QByteArray format(Protocol::ContentType ct)
{
    QByteArray format = "api_paste_format=";
    switch (ct) {
    case Protocol::C:
        format += 'c';
        break;
    case Protocol::Cpp:
        format += "cpp-qt";
        break;
    case Protocol::JavaScript:
        format += "javascript";
        break;
    case Protocol::Diff:
        format += "diff";
        break;
    case Protocol::Xml:
        format += "xml";
        break;
    case Protocol::Text:
        // fallthrough!
    default:
        format += "text";
    }
    format += '&';
    return format;
}

void PasteBinDotComProtocol::paste(const QString &text,
                                   ContentType ct,
                                   const QString &username,
                                   const QString &comment,
                                   const QString &description)
{
    Q_UNUSED(comment);
    Q_UNUSED(description);
    QTC_ASSERT(!m_pasteReply, return);

    // Format body
    QByteArray pasteData = API_KEY;
    pasteData += "api_option=paste&";
    pasteData += "api_paste_expire_date=1M&";
    pasteData += format(ct);
    pasteData += "api_paste_name=";
    pasteData += QUrl::toPercentEncoding(username);

    pasteData += "&api_paste_code=";
    pasteData += QUrl::toPercentEncoding(fixNewLines(text));

    // fire request
    m_pasteReply = httpPost(QLatin1String(PASTEBIN_BASE) + QLatin1String(PASTEBIN_API), pasteData);
    connect(m_pasteReply, SIGNAL(finished()), this, SLOT(pasteFinished()));
    if (debug)
        qDebug() << "paste: sending " << m_pasteReply << pasteData;
}

void PasteBinDotComProtocol::pasteFinished()
{
    if (m_pasteReply->error())
        qWarning("Pastebin.com protocol error: %s", qPrintable(m_pasteReply->errorString()));
    else
        emit pasteDone(QString::fromLatin1(m_pasteReply->readAll()));

    m_pasteReply->deleteLater();
    m_pasteReply = 0;
}

void PasteBinDotComProtocol::fetch(const QString &id)
{
    // Did we get a complete URL or just an id. Insert a call to the php-script
    QString link = QLatin1String(PASTEBIN_BASE) + QLatin1String(PASTEBIN_RAW);
    link.append(QLatin1String("?i="));

    if (id.startsWith(QLatin1String("http://")))
        link.append(id.mid(id.lastIndexOf(QLatin1Char('/')) + 1));
    else
        link.append(id);

    if (debug)
        qDebug() << "fetch: sending " << link;

    m_fetchReply = httpGet(link);
    connect(m_fetchReply, SIGNAL(finished()), this, SLOT(fetchFinished()));
    m_fetchId = id;
}

void PasteBinDotComProtocol::fetchFinished()
{
    QString title;
    QString content;
    const bool error = m_fetchReply->error();
    if (error) {
        content = m_fetchReply->errorString();
        if (debug)
            qDebug() << "fetchFinished: error" << m_fetchId << content;
    } else {
        title = QString::fromLatin1("Pastebin.com: %1").arg(m_fetchId);
        content = QString::fromLatin1(m_fetchReply->readAll());
        // Cut out from '<pre>' formatting
        const int preEnd = content.lastIndexOf(QLatin1String("</pre>"));
        if (preEnd != -1)
            content.truncate(preEnd);
        const int preStart = content.indexOf(QLatin1String("<pre>"));
        if (preStart != -1)
            content.remove(0, preStart + 5);
        // Make applicable as patch.
        content = Protocol::textFromHtml(content);
        content += QLatin1Char('\n');
        // -------------
        if (debug) {
            QDebug nsp = qDebug().nospace();
            nsp << "fetchFinished: " << content.size() << " Bytes";
            if (debug > 1)
                nsp << content;
        }
    }
    m_fetchReply->deleteLater();
    m_fetchReply = 0;
    emit fetchDone(title, content, error);
}

void PasteBinDotComProtocol::list()
{
    QTC_ASSERT(!m_listReply, return);

    const QString url = QLatin1String(PASTEBIN_BASE) + QLatin1String(PASTEBIN_ARCHIVE);
    m_listReply = httpGet(url);
    connect(m_listReply, SIGNAL(finished()), this, SLOT(listFinished()));
    if (debug)
        qDebug() << "list: sending " << url << m_listReply;
}

static inline void padString(QString *s, int len)
{
    const int missing = len - s->length();
    if (missing > 0)
        s->append(QString(missing, QLatin1Char(' ')));
}

/* Quick & dirty: Parse out the 'archive' table as of 16.3.2011:
 \code
<table class="maintable" cellspacing="0">
    <tr class="top">
        <th scope="col" align="left">Name / Title</th>
        <th scope="col" align="left">Posted</th>
        <th scope="col" align="right">Syntax</th>
    </tr>
    <tr>
        <td><img src="/i/t.gif"  class="i_p0" alt="" border="0" /><a href="/cvWciF4S">Vector 1</a></td>
        <td>2 sec ago</td>
        <td align="right"><a href="/archive/cpp">C++</a></td>
    </tr>
    ...
-\endcode */
enum ParseState
{
    OutSideTable, WithinTable, WithinTableRow, WithinTableHeaderElement,
    WithinTableElement, WithinTableElementAnchor, ParseError
};

QDebug operator<<(QDebug d, const QXmlStreamAttributes &al)
{
    QDebug nospace = d.nospace();
    foreach (const QXmlStreamAttribute &a, al)
        nospace << a.name().toString() << '=' << a.value().toString() << ' ';
    return d;
}

static inline ParseState nextOpeningState(ParseState current, const QXmlStreamReader &reader)
{
    const QStringRef &element = reader.name();
    switch (current) {
    case OutSideTable:
        // Trigger on main table only.
        if (element == QLatin1String("table")
           && reader.attributes().value(QLatin1String("class")) == QLatin1String("maintable"))
            return WithinTable;
        return OutSideTable;
    case WithinTable:
        if (element == QLatin1String("tr"))
            return WithinTableRow;
        break;
    case WithinTableRow:
        if (element == QLatin1String("td"))
            return WithinTableElement;
        if (element == QLatin1String("th"))
            return WithinTableHeaderElement;
        break;
    case WithinTableElement:
        if (element == QLatin1String("img"))
            return WithinTableElement;
        if (element == QLatin1String("a"))
            return WithinTableElementAnchor;
        break;
    case WithinTableHeaderElement:
    case WithinTableElementAnchor:
    case ParseError:
        break;
    }
    return ParseError;
}

static inline ParseState nextClosingState(ParseState current, const QStringRef &element)
{
    switch (current) {
    case OutSideTable:
        return OutSideTable;
    case WithinTable:
        if (element == QLatin1String("table"))
            return OutSideTable;
        break;
    case WithinTableRow:
        if (element == QLatin1String("tr"))
            return WithinTable;
        break;
    case WithinTableElement:
        if (element == QLatin1String("td"))
            return WithinTableRow;
        if (element == QLatin1String("img"))
            return WithinTableElement;
        break;
    case WithinTableHeaderElement:
        if (element == QLatin1String("th"))
            return WithinTableRow;
        break;
    case WithinTableElementAnchor:
        if (element == QLatin1String("a"))
            return WithinTableElement;
        break;
   case ParseError:
        break;
   }
   return ParseError;
}

static inline QStringList parseLists(QIODevice *io)
{
    enum { maxEntries = 200 }; // Limit the archive, which can grow quite large.

    QStringList rc;
    QXmlStreamReader reader(io);
    ParseState state = OutSideTable;
    int tableRow = 0;
    int tableColumn = 0;

    const QString hrefAttribute = QLatin1String("href");
    QString link;
    QString title;
    QString age;

    while (!reader.atEnd()) {
        switch (reader.readNext()) {
        case QXmlStreamReader::StartElement:
            state = nextOpeningState(state, reader);
            switch (state) {
            case WithinTableRow:
                tableColumn = 0;
                break;
            case OutSideTable:
            case WithinTable:
            case WithinTableHeaderElement:
            case WithinTableElement:
                break;
            case WithinTableElementAnchor: // 'href="/svb5K8wS"'
                if (tableColumn == 0) {
                    link = reader.attributes().value(hrefAttribute).toString();
                    if (link.startsWith(QLatin1Char('/')))
                        link.remove(0, 1);
                }
                break;
            case ParseError:
                return rc;
            } // switch startelement state
            break;
        case QXmlStreamReader::EndElement:
            state = nextClosingState(state, reader.name());
            switch (state) {
            case OutSideTable:
                if (tableRow) // Seen the table, bye.
                    return rc;
                break;
            case WithinTable:
                // User can occasionally be empty.
                if (tableRow && !link.isEmpty() && !title.isEmpty() && !age.isEmpty()) {
                    QString entry = link;
                    entry += QLatin1Char(' ');
                    entry += title;
                    entry += QLatin1String(" (");
                    entry += age;
                    entry += QLatin1Char(')');
                    rc.push_back(entry);
                    if (rc.size() >= maxEntries)
                        return rc;
                }
                tableRow++;
                age.clear();
                link.clear();
                title.clear();
                break;
            case WithinTableRow:
                tableColumn++;
                break;
            case WithinTableHeaderElement:
            case WithinTableElement:
            case WithinTableElementAnchor:
                break;
            case ParseError:
                return rc;
            } // switch endelement state
            break;
        case QXmlStreamReader::Characters:
            switch (state) {
            case WithinTableElement:
                if (tableColumn == 1)
                    age = reader.text().toString();
                break;
            case WithinTableElementAnchor:
                if (tableColumn == 0)
                    title = reader.text().toString();
                break;
            default:
                break;
            } // switch characters read state
            break;
       default:
            break;
        } // switch reader state
    }
    return rc;
}

void PasteBinDotComProtocol::listFinished()
{
    const bool error = m_listReply->error();
    if (error) {
        if (debug)
            qDebug() << "listFinished: error" << m_listReply->errorString();
    } else {
        QStringList list = parseLists(m_listReply);
        emit listDone(name(), list);
        if (debug)
            qDebug() << list;
    }
    m_listReply->deleteLater();
    m_listReply = 0;
}

} // namespace CodePaster
