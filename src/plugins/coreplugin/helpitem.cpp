/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "helpitem.h"
#include "helpmanager.h"

#include <utils/algorithm.h>
#include <utils/htmldocextractor.h>

using namespace Core;

HelpItem::HelpItem() = default;

HelpItem::HelpItem(const char *helpId)
    : HelpItem(QStringList(QString::fromUtf8(helpId)), {}, Unknown)
{}

HelpItem::HelpItem(const QString &helpId)
    : HelpItem(QStringList(helpId), {}, Unknown)
{}

HelpItem::HelpItem(const QUrl &url)
    : m_helpUrl(url)
{}

HelpItem::HelpItem(const QUrl &url, const QString &docMark, HelpItem::Category category)
    : m_helpUrl(url)
    , m_docMark(docMark)
    , m_category(category)
{}

HelpItem::HelpItem(const QString &helpId, const QString &docMark, Category category)
    : HelpItem(QStringList(helpId), docMark, category)
{}

HelpItem::HelpItem(const QStringList &helpIds, const QString &docMark, Category category)
    : m_docMark(docMark)
    , m_category(category)
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

static std::pair<QUrl, int> extractVersion(const QUrl &url)
{
    const QString host = url.host();
    const QStringList hostParts = host.split('.');
    if (hostParts.size() == 4 && (host.startsWith("com.trolltech.")
            || host.startsWith("org.qt-project."))) {
        bool ok = false;
        // the following is only correct under the specific current conditions, and it will
        // always be quite some guessing as long as the version information does not
        // include separators for major vs minor vs patch version
        const int version = hostParts.at(3).toInt(&ok);
        if (ok) {
            QUrl urlWithoutVersion(url);
            urlWithoutVersion.setHost(hostParts.mid(0, 3).join('.'));
            return {urlWithoutVersion, version};
        }
    }
    return {url, 0};
}

// sort primary by "url without version" and seconday by "version"
static bool helpUrlLessThan(const QUrl &a, const QUrl &b)
{
    const std::pair<QUrl, int> va = extractVersion(a);
    const std::pair<QUrl, int> vb = extractVersion(b);
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
            QMap<QString, QUrl> helpLinks;
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
            QMapIterator<QString, QUrl> it(helpLinks);
            while (it.hasNext()) {
                it.next();
                m_helpLinks->emplace_back(it.key(), it.value());
            }
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
        const QUrl unversionedUrl = extractVersion(link.second).first;
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
    int highestVersion = -1;
    HelpItem::Link bestLink;
    for (const HelpItem::Link &link : links) {
        const int version = extractVersion(link.second).second;
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
    return getBestLink(links());
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
