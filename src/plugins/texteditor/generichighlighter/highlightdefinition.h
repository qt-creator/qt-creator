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

#pragma once

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

    QHash<QString, QSharedPointer<KeywordList> > m_lists;
    QHash<QString, QSharedPointer<Context> > m_contexts;
    QHash<QString, QSharedPointer<ItemData> > m_itemsData;

    QString m_initialContext;

    QString m_singleLineComment;

    QString m_multiLineCommentStart;
    QString m_multiLineCommentEnd;
    QString m_multiLineCommentRegion;

    Qt::CaseSensitivity m_keywordCaseSensitivity;

    bool m_singleLineCommentAfterWhiteSpaces;
    bool m_indentationBasedFolding;

    QSet<QChar> m_delimiters;
};

} // namespace Internal
} // namespace TextEditor
