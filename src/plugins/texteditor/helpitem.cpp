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

#include "helpitem.h"

#include <coreplugin/helpmanager.h>
#include <utils/htmldocextractor.h>

#include <QtCore/QUrl>
#include <QtCore/QByteArray>
#include <QtCore/QMap>

using namespace TextEditor;

HelpItem::HelpItem()
{}

HelpItem::HelpItem(const QString &helpId, Category category) :
    m_helpId(helpId), m_docMark(helpId), m_category(category)
{}

HelpItem::HelpItem(const QString &helpId, const QString &docMark, Category category) :
    m_helpId(helpId), m_docMark(docMark), m_category(category)
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
    if (!Core::HelpManager::instance()->linksForIdentifier(m_helpId).isEmpty())
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
    QMap<QString, QUrl> helpLinks = Core::HelpManager::instance()->linksForIdentifier(m_helpId);
    foreach (const QUrl &url, helpLinks) {
        const QByteArray &html = Core::HelpManager::instance()->fileData(url);
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
        case QML:
            contents = htmlExtractor.getQMLItemDescription(html, m_docMark);
            break;

        default:
            break;
        }

        if (!contents.isEmpty())
            break;
    }
    return contents;
}
