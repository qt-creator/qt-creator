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

#ifndef HIGHLIGHTDEFINITIONHANDLER_H
#define HIGHLIGHTDEFINITIONHANDLER_H

#include <QtCore/QString>
#include <QtCore/QList>
#include <QtCore/QSharedPointer>
#include <QtCore/QStack>

#include <QtXml/QXmlDefaultHandler>

namespace GenericEditor {
namespace Internal {

class KeywordList;
class Context;
class Rule;
class HighlightDefinition;

class HighlightDefinitionHandler : public QXmlDefaultHandler
{
public:
    HighlightDefinitionHandler(const QSharedPointer<HighlightDefinition> &definition);
    ~HighlightDefinitionHandler();

    bool startDocument();
    bool endDocument();
    bool startElement(const QString &namespaceURI, const QString &localName,
                      const QString &qName, const QXmlAttributes &atts);
    bool endElement(const QString &namespaceURI, const QString &localName, const QString &qName);
    bool characters(const QString &ch);

private:
    void listElementStarted(const QXmlAttributes &atts);
    void itemElementStarted();
    void contextElementStarted(const QXmlAttributes &atts);
    void itemDataElementStarted(const QXmlAttributes &atts) const;
    void commentElementStarted(const QXmlAttributes &atts) const;
    void keywordsElementStarted(const QXmlAttributes &atts) const;
    void ruleElementStarted(const QXmlAttributes &atts, const QSharedPointer<Rule> &rule);

    // Specific rules.
    void detectCharStarted(const QXmlAttributes &atts);
    void detect2CharsStarted(const QXmlAttributes &atts);
    void anyCharStarted(const QXmlAttributes &atts);
    void stringDetectedStarted(const QXmlAttributes &atts);
    void regExprStarted(const QXmlAttributes &atts);
    void keywordStarted(const QXmlAttributes &atts);
    void intStarted(const QXmlAttributes &atts);
    void floatStarted(const QXmlAttributes &atts);
    void hlCOctStarted(const QXmlAttributes &atts);
    void hlCHexStarted(const QXmlAttributes &atts);
    void hlCStringCharStarted(const QXmlAttributes &atts);
    void hlCCharStarted(const QXmlAttributes &atts);
    void rangeDetectStarted(const QXmlAttributes &atts);
    void lineContinue(const QXmlAttributes &atts);
    void includeRulesStarted(const QXmlAttributes &atts);
    void detectSpacesStarted(const QXmlAttributes &atts);
    void detectIdentifier(const QXmlAttributes &atts);

    void processIncludeRules() const;
    void processIncludeRules(const QSharedPointer<Context> &context) const;

    QSharedPointer<HighlightDefinition> m_definition;

    bool m_processingKeyword;
    QString m_currentKeyword;
    QSharedPointer<KeywordList> m_currentList;
    QSharedPointer<Context> m_currentContext;
    QStack<QSharedPointer<Rule> > m_currentRule;

    bool m_initialContext;
};

} // namespace Internal
} // namespace GenericEditor

#endif // HIGHLIGHTDEFINITIONHANDLER_H
