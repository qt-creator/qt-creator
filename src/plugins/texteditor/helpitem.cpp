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

#include <coreplugin/helpmanager.h>
#include <utils/htmldocextractor.h>

using namespace TextEditor;

HelpItem::HelpItem()
{}

HelpItem::HelpItem(const QString &helpId, Category category) :
    m_helpId(helpId), m_docMark(helpId), m_category(category)
{}

HelpItem::HelpItem(const QString &helpId, const QString &docMark, Category category) :
    m_helpId(helpId), m_docMark(docMark), m_category(category)
{}

HelpItem::HelpItem(const QString &helpId, const QString &docMark, Category category,
                   const QMap<QString, QUrl> &helpLinks) :
    m_helpId(helpId), m_docMark(docMark), m_category(category), m_helpLinks(helpLinks)
{}

HelpItem::~HelpItem()
{}

void HelpItem::setHelpId(const QString &id)
{ m_helpId = id; }

const QString &HelpItem::helpId() const
{ return m_helpId; }

void HelpItem::setDocMark(const QString &mark)
{ m_docMark = mark; }

const QString &HelpItem::docMark() const
{ return m_docMark; }

void HelpItem::setCategory(Category cat)
{ m_category = cat; }

HelpItem::Category HelpItem::category() const
{ return m_category; }

bool HelpItem::isValid() const
{
    if (m_helpId.isEmpty())
        return false;
    if (!retrieveHelpLinks().isEmpty())
        return true;
    if (QUrl(m_helpId).isValid())
        return true;
    return false;
}

QString HelpItem::extractContent(bool extended) const
{
    Utils::HtmlDocExtractor htmlExtractor;
    if (extended)
        htmlExtractor.setMode(Utils::HtmlDocExtractor::Extended);
    else
        htmlExtractor.setMode(Utils::HtmlDocExtractor::FirstParagraph);

    QString contents;
    QMap<QString, QUrl> helpLinks = retrieveHelpLinks();
    if (helpLinks.isEmpty()) {
        // Maybe this is already an URL...
        QUrl url(m_helpId);
        if (url.isValid())
            helpLinks.insert(m_helpId, m_helpId);
    }
    foreach (const QUrl &url, helpLinks) {
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

QMap<QString, QUrl> HelpItem::retrieveHelpLinks() const
{
    if (m_helpLinks.isEmpty())
        m_helpLinks = Core::HelpManager::linksForIdentifier(m_helpId);
    return m_helpLinks;
}
