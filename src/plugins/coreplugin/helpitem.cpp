// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "helpitem.h"
#include "helpmanager.h"

#include <utils/algorithm.h>
#include <utils/htmldocextractor.h>

#include <QVersionNumber>

using namespace Core;

Q_GLOBAL_STATIC(HelpItem::LinkNarrower, m_linkNarrower);

HelpItem::HelpItem() = default;

HelpItem::HelpItem(const char *helpId)
    : HelpItem(QStringList(QString::fromUtf8(helpId)), {}, {}, Unknown)
{}

HelpItem::HelpItem(const QString &helpId)
    : HelpItem(QStringList(helpId), {}, {}, Unknown)
{}

HelpItem::HelpItem(const QUrl &url)
    : m_helpUrl(url)
{}

HelpItem::HelpItem(const QUrl &url, const QString &docMark, HelpItem::Category category)
    : m_helpUrl(url)
    , m_docMark(docMark)
    , m_category(category)
{}

HelpItem::HelpItem(const QString &helpId,
                   const Utils::FilePath &filePath,
                   const QString &docMark,
                   Category category)
    : HelpItem(QStringList(helpId), filePath, docMark, category)
{}

HelpItem::HelpItem(const QStringList &helpIds,
                   const Utils::FilePath &filePath,
                   const QString &docMark,
                   Category category)
    : m_docMark(docMark)
    , m_category(category)
    , m_filePath(filePath)
{
    setHelpIds(helpIds);
}

void HelpItem::setHelpUrl(const QUrl &url)
{
    m_helpUrl = url;
}

const QUrl &HelpItem::helpUrl() const
{
    return m_helpUrl;
}

void HelpItem::setHelpIds(const QStringList &ids)
{
    m_helpIds = Utils::filteredUnique(
        Utils::filtered(ids, [](const QString &s) { return !s.isEmpty(); }));
}

const QStringList &HelpItem::helpIds() const
{
    return m_helpIds;
}

void HelpItem::setDocMark(const QString &mark)
{ m_docMark = mark; }

const QString &HelpItem::docMark() const
{ return m_docMark; }

void HelpItem::setCategory(Category cat)
{ m_category = cat; }

HelpItem::Category HelpItem::category() const
{ return m_category; }

/*!
    Sets the \a filePath that this help item originates from, for example
    in case of context help.
*/
void HelpItem::setFilePath(const Utils::FilePath &filePath)
{
    m_filePath = filePath;
}

/*!
    Returns the filePath that this help item originates from, for example
    in case of context help.
*/
Utils::FilePath HelpItem::filePath() const
{
    return m_filePath;
}

bool HelpItem::isEmpty() const
{
    return m_helpUrl.isEmpty() && m_helpIds.isEmpty();
}

bool HelpItem::isValid() const
{
    if (m_helpUrl.isEmpty() && m_helpIds.isEmpty())
        return false;
    return !links().empty();
}

QString HelpItem::firstParagraph() const
{
    if (!m_firstParagraph)
        m_firstParagraph = extractContent(false);
    return *m_firstParagraph;
}

QString HelpItem::extractContent(bool extended) const
{
    Utils::HtmlDocExtractor htmlExtractor;
    if (extended)
        htmlExtractor.setMode(Utils::HtmlDocExtractor::Extended);
    else
        htmlExtractor.setMode(Utils::HtmlDocExtractor::FirstParagraph);

    QString contents;
    for (const Link &item : links()) {
        const QUrl url = item.second;
        const QString html = QString::fromUtf8(Core::HelpManager::fileData(url));
        switch (m_category) {
        case Brief:
            contents = htmlExtractor.getClassOrNamespaceBrief(html, m_docMark);
            break;
        case ClassOrNamespace:
            contents = htmlExtractor.getClassOrNamespaceDescription(html, m_docMark);
            break;
        case Function:
            contents = htmlExtractor.getFunctionDescription(html, m_docMark);
            break;
        case Enum:
            contents = htmlExtractor.getEnumDescription(html, m_docMark);
            break;
        case Typedef:
            contents = htmlExtractor.getTypedefDescription(html, m_docMark);
            break;
        case Macro:
            contents = htmlExtractor.getMacroDescription(html, m_docMark);
            break;
        case QmlComponent:
            contents = htmlExtractor.getQmlComponentDescription(html, m_docMark);
            break;
        case QmlProperty:
            contents = htmlExtractor.getQmlPropertyDescription(html, m_docMark);
            break;
        case QMakeVariableOfFunction:
            contents = htmlExtractor.getQMakeVariableOrFunctionDescription(html, m_docMark);
            break;

        default:
            break;
        }

        if (!contents.isEmpty())
            break;
    }
    return contents;
}

// The following is only correct under the specific current conditions, and it will
// always be quite some guessing as long as the version information does not
// include separators for major vs minor vs patch version.
static QVersionNumber qtVersionHeuristic(const QString &digits)
{
    if (digits.size() > 6 || digits.size() < 3)
        return {}; // suspicious version number

    for (const QChar &digit : digits)
        if (!digit.isDigit())
            return {}; // we should have only digits

    // When we have 3 digits, we split it like: ABC -> A.B.C
    // When we have 4 digits, we split it like: ABCD -> A.BC.D
    // When we have 5 digits, we split it like: ABCDE -> A.BC.DE
    // When we have 6 digits, we split it like: ABCDEF -> AB.CD.EF
    switch (digits.size()) {
    case 3:
        return QVersionNumber(digits.mid(0, 1).toInt(),
                              digits.mid(1, 1).toInt(),
                              digits.mid(2, 1).toInt());
    case 4:
        return QVersionNumber(digits.mid(0, 1).toInt(),
                              digits.mid(1, 2).toInt(),
                              digits.mid(3, 1).toInt());
    case 5:
        return QVersionNumber(digits.mid(0, 1).toInt(),
                              digits.mid(1, 2).toInt(),
                              digits.mid(3, 2).toInt());
    case 6:
        return QVersionNumber(digits.mid(0, 2).toInt(),
                              digits.mid(2, 2).toInt(),
                              digits.mid(4, 2).toInt());
    default:
        break;
    }
    return {};
}

std::pair<QUrl, QVersionNumber> HelpItem::extractQtVersionNumber(const QUrl &url)
{
    const QString host = url.host();
    const QStringList hostParts = host.split('.');
    if (hostParts.size() == 4 && (host.startsWith("com.trolltech.")
            || host.startsWith("org.qt-project."))) {
        const QVersionNumber version = qtVersionHeuristic(hostParts.at(3));
        if (!version.isNull()) {
            QUrl urlWithoutVersion(url);
            urlWithoutVersion.setHost(hostParts.mid(0, 3).join('.'));
            return {urlWithoutVersion, version};
        }
    }
    return {url, {}};
}

// sort primary by "url without version" and seconday by "version"
static bool helpUrlLessThan(const QUrl &a, const QUrl &b)
{
    const std::pair<QUrl, QVersionNumber> va = HelpItem::extractQtVersionNumber(a);
    const std::pair<QUrl, QVersionNumber> vb = HelpItem::extractQtVersionNumber(b);
    const QString sa = va.first.toString();
    const QString sb = vb.first.toString();
    if (sa == sb)
        return va.second > vb.second;
    return sa < sb;
}

static bool linkLessThan(const HelpItem::Link &a, const HelpItem::Link &b)
{
    return helpUrlLessThan(a.second, b.second);
}

// links are sorted with highest "version" first (for Qt help urls)
const HelpItem::Links &HelpItem::links() const
{
    if (!m_helpLinks) {
        if (!m_helpUrl.isEmpty()) {
            m_keyword = m_helpUrl.toString();
            m_helpLinks.emplace(Links{{m_keyword, m_helpUrl}});
        } else {
            m_helpLinks.emplace(); // set a value even if there are no help IDs
            QMultiMap<QString, QUrl> helpLinks;
            for (const QString &id : m_helpIds) {
                helpLinks = Core::HelpManager::linksForIdentifier(id);
                if (!helpLinks.isEmpty()) {
                    m_keyword = id;
                    break;
                }
            }
            if (helpLinks.isEmpty()) { // perform keyword lookup as well as a fallback
                for (const QString &id : m_helpIds) {
                    helpLinks = Core::HelpManager::linksForKeyword(id);
                    if (!helpLinks.isEmpty()) {
                        m_keyword = id;
                        m_isFuzzyMatch = true;
                        break;
                    }
                }
            }
            for (auto it = helpLinks.cbegin(), end = helpLinks.cend(); it != end; ++it)
                m_helpLinks->emplace_back(it.key(), it.value());
        }
        Utils::sort(*m_helpLinks, linkLessThan);
    }
    return *m_helpLinks;
}

static const HelpItem::Links getBestLinks(const HelpItem::Links &links)
{
    // extract the highest version (== first) link of each individual topic
    HelpItem::Links bestLinks;
    QUrl currentUnversionedUrl;
    for (const HelpItem::Link &link : links) {
        const QUrl unversionedUrl = HelpItem::extractQtVersionNumber(link.second).first;
        if (unversionedUrl != currentUnversionedUrl) {
            currentUnversionedUrl = unversionedUrl;
            bestLinks.push_back(link);
        }
    }
    return bestLinks;
}

static const HelpItem::Links getBestLink(const HelpItem::Links &links)
{
    if (links.empty())
        return {};
    // Extract single link with highest version, from all topics.
    // This is to ensure that if we succeeded with an ID lookup, and we have e.g. Qt5 and Qt4
    // documentation, that we only return the Qt5 link even though the Qt5 and Qt4 URLs look
    // different.
    QVersionNumber highestVersion;
    // Default to first link if version extraction failed, possibly because it is not a Qt doc link
    HelpItem::Link bestLink = links.front();
    for (const HelpItem::Link &link : links) {
        const QVersionNumber version = HelpItem::extractQtVersionNumber(link.second).second;
        if (version > highestVersion) {
            highestVersion = version;
            bestLink = link;
        }
    }
    return {bestLink};
}

const HelpItem::Links HelpItem::bestLinks() const
{
    if (isFuzzyMatch())
        return getBestLinks(links());
    const Links filteredLinks = *m_linkNarrower ? (*m_linkNarrower)(*this, links()) : links();
    return getBestLink(filteredLinks);
}

const QString HelpItem::keyword() const
{
    return m_keyword;
}

bool HelpItem::isFuzzyMatch() const
{
    // make sure m_isFuzzyMatch is correct
    links();
    return m_isFuzzyMatch;
}

void HelpItem::setLinkNarrower(const LinkNarrower &narrower)
{
    *m_linkNarrower = narrower;
}
