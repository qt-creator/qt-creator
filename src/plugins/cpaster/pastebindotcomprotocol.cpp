// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "pastebindotcomprotocol.h"

#include <utils/qtcassert.h>

#include <QDebug>
#include <QRegularExpression>
#include <QStringList>
#include <QByteArray>

#include <QNetworkReply>

enum { debug = 0 };

namespace CodePaster {

const char PASTEBIN_BASE[]="https://pastebin.com/";
const char PASTEBIN_API[]="api/api_post.php";
const char PASTEBIN_RAW[]="raw/";
const char PASTEBIN_ARCHIVE[]="archive";

const char API_KEY[]="api_dev_key=516686fc461fb7f9341fd7cf2af6f829&"; // user: qtcreator_apikey

const char PROTOCOL_NAME[] = "Pastebin.Com";

QString PasteBinDotComProtocol::protocolName()
{
    return QLatin1String(PROTOCOL_NAME);
}

unsigned PasteBinDotComProtocol::capabilities() const
{
    return ListCapability | PostDescriptionCapability;
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

// According to documentation, Pastebin.com accepts only fixed expiry specifications:
// N = Never, 10M = 10 Minutes, 1H = 1 Hour, 1D = 1 Day, 1W = 1 Week, 2W = 2 Weeks, 1M = 1 Month,
// however, 1W and 2W do not work.

static inline QByteArray expirySpecification(int expiryDays)
{
    if (expiryDays <= 1)
        return QByteArray("1D");
    if (expiryDays <= 31)
        return QByteArray("1M");
    return QByteArray("N");
}

void PasteBinDotComProtocol::paste(
        const QString &text,
        ContentType ct,
        int expiryDays,
        const QString & /* username */, // Not used unless registered user
        const QString &comment,
        const QString &description
        )
{
    Q_UNUSED(comment)
    Q_UNUSED(description)
    QTC_ASSERT(!m_pasteReply, return);

    // Format body
    QByteArray pasteData = API_KEY;
    pasteData += "api_option=paste&";
    pasteData += "api_paste_expire_date=";
    pasteData += expirySpecification(expiryDays);
    pasteData += '&';
    pasteData += format(ct);
    pasteData += "api_paste_name="; // Title or name.
    pasteData += QUrl::toPercentEncoding(description);
    pasteData += "&api_paste_code=";
    pasteData += QUrl::toPercentEncoding(fixNewLines(text));
    // fire request
    m_pasteReply = httpPost(QLatin1String(PASTEBIN_BASE) + QLatin1String(PASTEBIN_API), pasteData);
    connect(m_pasteReply, &QNetworkReply::finished, this, &PasteBinDotComProtocol::pasteFinished);
    if (debug)
        qDebug() << "paste: sending " << m_pasteReply << pasteData;
}

void PasteBinDotComProtocol::pasteFinished()
{
    if (m_pasteReply->error())
        qWarning("%s protocol error: %s", PROTOCOL_NAME, qPrintable(m_pasteReply->errorString()));
    else
        emit pasteDone(QString::fromLatin1(m_pasteReply->readAll()));

    m_pasteReply->deleteLater();
    m_pasteReply = nullptr;
}

void PasteBinDotComProtocol::fetch(const QString &id)
{
    // Did we get a complete URL or just an id. Insert a call to the php-script
    QString link = QLatin1String(PASTEBIN_BASE) + QLatin1String(PASTEBIN_RAW);

    if (id.startsWith(QLatin1String("http://")))
        link.append(id.mid(id.lastIndexOf(QLatin1Char('/')) + 1));
    else
        link.append(id);

    if (debug)
        qDebug() << "fetch: sending " << link;

    m_fetchReply = httpGet(link);
    connect(m_fetchReply, &QNetworkReply::finished, this, &PasteBinDotComProtocol::fetchFinished);
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
        title = QLatin1String(PROTOCOL_NAME) + QLatin1String(": ") + m_fetchId;
        content = QString::fromUtf8(m_fetchReply->readAll());
        if (debug) {
            QDebug nsp = qDebug().nospace();
            nsp << "fetchFinished: " << content.size() << " Bytes";
            if (debug > 1)
                nsp << content;
        }
    }
    m_fetchReply->deleteLater();
    m_fetchReply = nullptr;
    emit fetchDone(title, content, error);
}

void PasteBinDotComProtocol::list()
{
    QTC_ASSERT(!m_listReply, return);

    const QString url = QLatin1String(PASTEBIN_BASE) + QLatin1String(PASTEBIN_ARCHIVE);
    m_listReply = httpGet(url);
    connect(m_listReply, &QNetworkReply::finished, this, &PasteBinDotComProtocol::listFinished);
    if (debug)
        qDebug() << "list: sending " << url << m_listReply;
}

/* Quick & dirty: Parse out the 'archive' table as of 16.3.2016:
 \code
<table class="maintable">
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

static QString replaceEntities(const QString &original)
{
    QString result(original);
    static const QRegularExpression regex("&#((x[[:xdigit:]]+)|(\\d+));");

    QRegularExpressionMatchIterator it = regex.globalMatch(original);
    while (it.hasNext()) {
        const QRegularExpressionMatch match = it.next();
        const QString value = match.captured(1);
        if (value.startsWith('x'))
            result.replace(match.captured(0), QChar(value.mid(1).toInt(nullptr, 16)));
        else
            result.replace(match.captured(0), QChar(value.toInt(nullptr, 10)));
    }

    return result;
}

namespace {
struct Attribute {
    QString name;
    QString value;
};
}

static QList<Attribute> toAttributes(QStringView attributes)
{
    QList<Attribute> result;
    const QRegularExpression att("\\s+([a-zA-Z]+)\\s*=\\s*('.*?'|\".*?\")");
    QRegularExpressionMatchIterator it = att.globalMatch(attributes.toString());
    while (it.hasNext()) {
        const QRegularExpressionMatch match = it.next();
        QString val = match.captured(2); // including quotes
        if (val.size() > 2)
            val = val.mid(1, val.size() - 2);
        else
            val= QString();

        result.append(Attribute{match.captured(1), val});
    }
    return result;
}

static inline ParseState nextOpeningState(ParseState current, QStringView tagView,
                                          QStringView attributesView)
{
    switch (current) {
    case OutSideTable:
        // Trigger on main table only.
        if (tagView == QLatin1String("table")) {
            const QList<Attribute> attributes = toAttributes(attributesView);
            for (const Attribute &att : attributes) {
                if (att.name == "class" && att.value == "maintable")
                    return WithinTable;
            }
        }
        return OutSideTable;
    case WithinTable:
        if (tagView == QLatin1String("tr"))
            return WithinTableRow;
        break;
    case WithinTableRow:
        if (tagView == QLatin1String("td"))
            return WithinTableElement;
        if (tagView == QLatin1String("th"))
            return WithinTableHeaderElement;
        break;
    case WithinTableElement:
        if (tagView == QLatin1String("img"))
            return WithinTableElement;
        if (tagView == QLatin1String("a"))
            return WithinTableElementAnchor;
        break;
    case WithinTableHeaderElement:
    case WithinTableElementAnchor:
    case ParseError:
        break;
    }
    if (tagView == QString("div") || tagView == QString("span") || tagView == QString("tbody"))
        return current;  // silently ignore
    return ParseError;
}

static inline ParseState nextClosingState(ParseState current, QStringView element)
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
        if (element == QString("tr")) // html file may have wrong XML syntax, but browsers ignore
            return WithinTable;
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
   if (element == QString("div") || element == QString("span") || element == QString("tbody"))
       return current; // silently ignore
   return ParseError;
}

static inline QStringList parseLists(QIODevice *io, QString *errorMessage)
{
    enum { maxEntries = 200 }; // Limit the archive, which can grow quite large.

    QStringList rc;
    // Read answer and delete part up to the main table since the input form has
    // parts that can no longer be parsed using XML parsers (<input type="text" x-webkit-speech />)
    QByteArray data = io->readAll();
    const int tablePos = data.indexOf("<table class=\"maintable\">");
    if (tablePos < 0) {
        *errorMessage = QLatin1String("Could not locate beginning of table.");
        return rc;
    }
    data.remove(0, tablePos);

    ParseState state = OutSideTable;
    int tableRow = 0;
    int tableColumn = 0;

    QString link;
    QString title;
    QString age;


    QString dataStr = QString::fromUtf8(data);
    // remove comments if any
    const QRegularExpression comment("<!--.*--!>", QRegularExpression::MultilineOption);
    for ( ;; ) {
        const QRegularExpressionMatch match = comment.match(dataStr);
        if (!match.hasMatch())
            break;
        dataStr.remove(match.capturedStart(), match.capturedLength());
    }

    const QRegularExpression tag("<(/?)\\s*([a-zA-Z][a-zA-Z0-9]*)(.*?)(/?)\\s*>",
                                 QRegularExpression::MultilineOption);
    const QRegularExpression wsOnly("^\\s+$", QRegularExpression::MultilineOption);
    QRegularExpressionMatchIterator it = tag.globalMatch(dataStr);
    while (it.hasNext()) {
        const QRegularExpressionMatch match = it.next();

        bool startElement = match.captured(4).length() == 0 && match.captured(1).length() == 0;
        if (startElement) {
            state = nextOpeningState(state, match.capturedView(2), match.capturedView(3));
            switch (state) {
            case WithinTableRow:
                tableColumn = 0;
                break;
            case OutSideTable:
            case WithinTable:
            case WithinTableHeaderElement:
            case WithinTableElement:
                break;
            case WithinTableElementAnchor:
                if (tableColumn == 0) {
                    const QList<Attribute> attributes = toAttributes(match.capturedView(3));
                    for (const Attribute &att : attributes) {
                        if (att.name == "href") {
                            link = att.value;
                            if (link.startsWith('/'))
                                link.remove(0, 1);
                            break;
                        }
                    }
                }
                break;
            case ParseError:
                return rc;
            }
        } else { // not a start element
            state = nextClosingState(state, match.capturedView(2));
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
            }
        }
        // check and handle pure text
        if (match.capturedEnd() + 1 < dataStr.size() - 1) {
            int nextStartTag = dataStr.indexOf(tag, match.capturedEnd() + 1);
            if (nextStartTag != -1) {
                const QString text = replaceEntities(
                            dataStr.mid(match.capturedEnd(), nextStartTag - match.capturedEnd()));
                if (!wsOnly.match(text).hasMatch()) {
                    switch (state) {
                    case WithinTableElement:
                        if (tableColumn == 1)
                            age = text;
                        break;
                    case WithinTableElementAnchor:
                        if (tableColumn == 0)
                            title = text;
                        break;
                    default:
                        break;
                    } // switch characters read state

                }
            }
        }
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
        if (m_listReply->hasRawHeader("Content-Type")) {
            // if the content type changes to xhtml we should switch back to QXmlStreamReader
            const QByteArray contentType = m_listReply->rawHeader("Content-Type");
            if (!contentType.startsWith("text/html"))
                qWarning() << "Content type has changed to" << contentType;
        }
        QString errorMessage;
        const QStringList list = parseLists(m_listReply, &errorMessage);
        if (list.isEmpty())
            qWarning().nospace() << "Failed to read list from " << PASTEBIN_BASE <<  ':' << errorMessage;
        emit listDone(name(), list);
        if (debug)
            qDebug() << list;
    }
    m_listReply->deleteLater();
    m_listReply = nullptr;
}

} // CodePaster
