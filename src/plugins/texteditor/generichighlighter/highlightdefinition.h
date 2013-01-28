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

#ifndef HIGHLIGHTDEFINITION_H
#define HIGHLIGHTDEFINITION_H

#include <QString>
#include <QHash>
#include <QSet>
#include <QSharedPointer>

namespace TextEditor {
namespace Internal {

class KeywordList;
class Context;
class ItemData;

class HighlightDefinition
{
public:
    HighlightDefinition();
    ~HighlightDefinition();

    bool isValid() const;

    QSharedPointer<KeywordList> createKeywordList(const QString &list);
    QSharedPointer<KeywordList> keywordList(const QString &list);

    QSharedPointer<Context> createContext(const QString &context, bool initial);
    QSharedPointer<Context> initialContext() const;
    QSharedPointer<Context> context(const QString &context) const;
    const QHash<QString, QSharedPointer<Context> > &contexts() const;

    QSharedPointer<ItemData> createItemData(const QString &itemData);
    QSharedPointer<ItemData> itemData(const QString &itemData) const;

    void setKeywordsSensitive(const QString &sensitivity);
    Qt::CaseSensitivity keywordsSensitive() const;

    void addDelimiters(const QString &characters);
    void removeDelimiters(const QString &characters);
    bool isDelimiter(const QChar &character) const;

    void setSingleLineComment(const QString &start);
    const QString &singleLineComment() const;

    void setCommentAfterWhitespaces(const QString &after);
    bool isCommentAfterWhiteSpaces() const;

    void setMultiLineCommentStart(const QString &start);
    const QString &multiLineCommentStart() const;

    void setMultiLineCommentEnd(const QString &end);
    const QString &multiLineCommentEnd() const;

    void setMultiLineCommentRegion(const QString &region);
    const QString &multiLineCommentRegion() const;

    void setIndentationBasedFolding(const QString &indentationBasedFolding);
    bool isIndentationBasedFolding() const;

private:
    Q_DISABLE_COPY(HighlightDefinition)

    struct GenericHelper
    {
        template <class Element, class Container>
        QSharedPointer<Element> create(const QString &name, Container &container);

        template <class Element, class Container>
        QSharedPointer<Element> find(const QString &name, const Container &container) const;
    };
    GenericHelper m_helper;

    QHash<QString, QSharedPointer<KeywordList> > m_lists;
    QHash<QString, QSharedPointer<Context> > m_contexts;
    QHash<QString, QSharedPointer<ItemData> > m_itemsData;

    QString m_initialContext;

    QString m_singleLineComment;
    bool m_singleLineCommentAfterWhiteSpaces;

    QString m_multiLineCommentStart;
    QString m_multiLineCommentEnd;
    QString m_multiLineCommentRegion;

    Qt::CaseSensitivity m_keywordCaseSensitivity;

    bool m_indentationBasedFolding;

    QSet<QChar> m_delimiters;
};

} // namespace Internal
} // namespace TextEditor

#endif // HIGHLIGHTDEFINITION_H
