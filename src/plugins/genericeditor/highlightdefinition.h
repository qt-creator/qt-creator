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

#ifndef HIGHLIGHTDEFINITION_H
#define HIGHLIGHTDEFINITION_H

#include <QtCore/QString>
#include <QtCore/QHash>
#include <QtCore/QSharedPointer>

namespace GenericEditor {
namespace Internal {

class KeywordList;
class Context;
class ItemData;

class HighlightDefinition
{
public:
    HighlightDefinition();
    ~HighlightDefinition();

    const QSharedPointer<KeywordList> &createKeywordList(const QString &list);
    const QSharedPointer<KeywordList> &keywordList(const QString &list);

    const QSharedPointer<Context> &createContext(const QString &context, bool initial);
    const QSharedPointer<Context> &initialContext() const;
    const QSharedPointer<Context> &context(const QString &context) const;
    const QHash<QString, QSharedPointer<Context> > &contexts() const;

    const QSharedPointer<ItemData> &createItemData(const QString &itemData);
    const QSharedPointer<ItemData> &itemData(const QString &itemData) const;

    void setKeywordsSensitive(const QString &sensitivity);
    Qt::CaseSensitivity keywordsSensitive() const;

    void addDelimiters(const QString &characters);
    void removeDelimiters(const QString &characters);
    //Todo: wordWrapDelimiters?
    bool isDelimiter(const QChar &character) const;    

    void setSingleLineComment(const QString &start);
    const QString &singleLineComment() const;

    void setSingleLineCommentPosition(const QString &position);
    int singleLineCommentPosition() const;
    bool isSingleLineCommentAfterWhiteSpaces() const;

    void setMultiLineCommentStart(const QString &start);
    const QString &multiLineCommentStart() const;

    void setMultiLineCommentEnd(const QString &end);
    const QString &multiLineCommentEnd() const;

    void setMultiLineCommentRegion(const QString &region);
    const QString &multiLineCommentRegion() const;

    void setLanguageName(const QString &name);
    const QString &languageName() const;

    //Todo: Will use?
    void setFileExtensions(const QString &extensions);    

private:

    HighlightDefinition(const HighlightDefinition &);
    HighlightDefinition &operator=(const HighlightDefinition &);

    struct GenericHelper
    {
        template <class Element, class Container>
        const QSharedPointer<Element> &create(const QString &name, Container &container);

        template <class Element, class Container>
        const QSharedPointer<Element> &find(const QString &name, const Container &container) const;
    };
    GenericHelper m_helper;

    QHash<QString, QSharedPointer<KeywordList> > m_lists;
    QHash<QString, QSharedPointer<Context> > m_contexts;
    QHash<QString, QSharedPointer<ItemData> > m_itemsData;

    QString m_initialContext;

    QString m_delimiters;

    QString m_singleLineComment;
    int m_singleLineCommentPosition;
    bool m_singleLineCommentAfterWhiteSpaces;

    QString m_multiLineCommentStart;
    QString m_multiLineCommentEnd;
    QString m_multiLineCommentRegion;

    Qt::CaseSensitivity m_keywordCaseSensitivity;

    QString m_languageName;
    QString m_fileExtensions;
};

} // namespace Internal
} // namespace GenericEditor

#endif // HIGHLIGHTDEFINITION_H
