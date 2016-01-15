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

#include "highlightdefinition.h"
#include "highlighterexception.h"
#include "context.h"
#include "keywordlist.h"
#include "itemdata.h"
#include "reuse.h"

#include <QCoreApplication>
#include <QString>

using namespace TextEditor;
using namespace Internal;

namespace {

template <class Element, class Container>
QSharedPointer<Element> createHelper(const QString &name, Container &container)
{
    if (name.isEmpty()) {
        throw HighlighterException(
                    QCoreApplication::translate("GenericHighlighter", "Element name is empty."));
    }

    if (container.contains(name)) {
        throw HighlighterException(
                QCoreApplication::translate("GenericHighlighter",
                                            "Duplicate element name \"%1\".").arg(name));
    }

    return container.insert(name, QSharedPointer<Element>(new Element)).value();
}

template <class Element, class Container>
QSharedPointer<Element>
findHelper(const QString &name, const Container &container)
{
    typename Container::const_iterator it = container.find(name);
    if (it == container.end()) {
        throw HighlighterException(
                    QCoreApplication::translate("GenericHighlighter",
                                                "Name \"%1\" not found.").arg(name));
    }

    return it.value();
}

} // anon namespace

HighlightDefinition::HighlightDefinition() :
    m_keywordCaseSensitivity(Qt::CaseSensitive),
    m_singleLineCommentAfterWhiteSpaces(false),
    m_indentationBasedFolding(false)
{
    QString s(QLatin1String(".():!+,-<=>%&/;?[]^{|}~\\*, \t"));
    foreach (const QChar &c, s)
        m_delimiters.insert(c);
}

HighlightDefinition::~HighlightDefinition()
{}

bool HighlightDefinition::isValid() const
{
    return !m_initialContext.isEmpty();
}

QSharedPointer<KeywordList> HighlightDefinition::createKeywordList(const QString &list)
{
    return createHelper<KeywordList>(list, m_lists);
}

QSharedPointer<KeywordList> HighlightDefinition::keywordList(const QString &list)
{
    return findHelper<KeywordList>(list, m_lists);
}

QSharedPointer<Context> HighlightDefinition::createContext(const QString &context, bool initial)
{
    if (initial)
        m_initialContext = context;

    QSharedPointer<Context> newContext = createHelper<Context>(context, m_contexts);
    newContext->setName(context);
    return newContext;
}

QSharedPointer<Context> HighlightDefinition::initialContext() const
{
    return findHelper<Context>(m_initialContext, m_contexts);
}

QSharedPointer<Context> HighlightDefinition::context(const QString &context) const
{
    return findHelper<Context>(context, m_contexts);
}

const QHash<QString, QSharedPointer<Context> > &HighlightDefinition::contexts() const
{
    return m_contexts;
}

QSharedPointer<ItemData> HighlightDefinition::createItemData(const QString &itemData)
{
    return createHelper<ItemData>(itemData, m_itemsData);
}

QSharedPointer<ItemData> HighlightDefinition::itemData(const QString &itemData) const
{
    return findHelper<ItemData>(itemData, m_itemsData);
}

void HighlightDefinition::setSingleLineComment(const QString &start)
{ m_singleLineComment = start; }

const QString &HighlightDefinition::singleLineComment() const
{ return m_singleLineComment; }

void HighlightDefinition::setCommentAfterWhitespaces(const QString &after)
{
    if (after == QLatin1String("afterwhitespace"))
        m_singleLineCommentAfterWhiteSpaces = true;
}

bool HighlightDefinition::isCommentAfterWhiteSpaces() const
{ return m_singleLineCommentAfterWhiteSpaces; }

void HighlightDefinition::setMultiLineCommentStart(const QString &start)
{ m_multiLineCommentStart = start; }

const QString &HighlightDefinition::multiLineCommentStart() const
{ return m_multiLineCommentStart; }

void HighlightDefinition::setMultiLineCommentEnd(const QString &end)
{ m_multiLineCommentEnd = end; }

const QString &HighlightDefinition::multiLineCommentEnd() const
{ return m_multiLineCommentEnd; }

void HighlightDefinition::setMultiLineCommentRegion(const QString &region)
{ m_multiLineCommentRegion = region; }

const QString &HighlightDefinition::multiLineCommentRegion() const
{ return m_multiLineCommentRegion; }

void HighlightDefinition::removeDelimiters(const QString &characters)
{
    for (int i = 0; i < characters.length(); ++i)
        m_delimiters.remove(characters.at(i));
}

void HighlightDefinition::addDelimiters(const QString &characters)
{
    for (int i = 0; i < characters.length(); ++i) {
        if (!m_delimiters.contains(characters.at(i)))
            m_delimiters.insert(characters.at(i));
    }
}

bool HighlightDefinition::isDelimiter(const QChar &character) const
{
    if (m_delimiters.contains(character))
        return true;
    return false;
}

void HighlightDefinition::setKeywordsSensitive(const QString &sensitivity)
{
    if (!sensitivity.isEmpty())
        m_keywordCaseSensitivity = toCaseSensitivity(toBool(sensitivity));
}

Qt::CaseSensitivity HighlightDefinition::keywordsSensitive() const
{ return m_keywordCaseSensitivity; }

void HighlightDefinition::setIndentationBasedFolding(const QString &indentationBasedFolding)
{ m_indentationBasedFolding = toBool(indentationBasedFolding); }

bool HighlightDefinition::isIndentationBasedFolding() const
{ return m_indentationBasedFolding; }
