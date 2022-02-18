/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#define QT_NO_CAST_FROM_ASCII

#include "qmimetypeparser_p.h"

#include "qmimetype_p.h"
#include "qmimemagicrulematcher_p.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QDebug>
#include <QtCore/QDir>
#include <QtCore/QXmlStreamReader>
#include <QtCore/QXmlStreamWriter>
#include <QtCore/QStack>

QT_BEGIN_NAMESPACE

// XML tags in MIME files
static const char mimeInfoTagC[] = "mime-info";
static const char mimeTypeTagC[] = "mime-type";
static const char mimeTypeAttributeC[] = "type";
static const char subClassTagC[] = "sub-class-of";
static const char commentTagC[] = "comment";
static const char genericIconTagC[] = "generic-icon";
static const char iconTagC[] = "icon";
static const char nameAttributeC[] = "name";
static const char globTagC[] = "glob";
static const char globDeleteAllTagC[] = "glob-deleteall";
static const char aliasTagC[] = "alias";
static const char patternAttributeC[] = "pattern";
static const char weightAttributeC[] = "weight";
static const char caseSensitiveAttributeC[] = "case-sensitive";
static const char localeAttributeC[] = "xml:lang";

static const char magicTagC[] = "magic";
static const char priorityAttributeC[] = "priority";

static const char matchTagC[] = "match";
static const char matchValueAttributeC[] = "value";
static const char matchTypeAttributeC[] = "type";
static const char matchOffsetAttributeC[] = "offset";
static const char matchMaskAttributeC[] = "mask";

/*!
    \class QMimeTypeParser
    \inmodule QtCore
    \internal
    \brief The QMimeTypeParser class parses MIME types, and builds a MIME database hierarchy by adding to QMimeDatabase.

    Populates QMimeDataBase

    \sa QMimeDatabase, QMimeMagicRuleMatcher, MagicRule, MagicStringRule, MagicByteRule, GlobPattern
    \sa QMimeTypeParser
*/

/*!
    \class QMimeTypeParserBase
    \inmodule QtCore
    \internal
    \brief The QMimeTypeParserBase class parses for a sequence of <mime-type> in a generic way.

    Calls abstract handler function process for QMimeType it finds.

    \sa QMimeDatabase, QMimeMagicRuleMatcher, MagicRule, MagicStringRule, MagicByteRule, GlobPattern
    \sa QMimeTypeParser
*/

/*!
    \fn virtual bool QMimeTypeParserBase::process(const QMimeType &t, QString *errorMessage) = 0;
    Overwrite to process the sequence of parsed data
*/

QMimeTypeParserBase::ParseState QMimeTypeParserBase::nextState(ParseState currentState, QStringView startElement)
{
    switch (currentState) {
    case ParseBeginning:
        if (startElement == QLatin1String(mimeInfoTagC))
            return ParseMimeInfo;
        if (startElement == QLatin1String(mimeTypeTagC))
            return ParseMimeType;
        return ParseError;
    case ParseMimeInfo:
        return startElement == QLatin1String(mimeTypeTagC) ? ParseMimeType : ParseError;
    case ParseMimeType:
    case ParseComment:
    case ParseGenericIcon:
    case ParseIcon:
    case ParseGlobPattern:
    case ParseGlobDeleteAll:
    case ParseSubClass:
    case ParseAlias:
    case ParseOtherMimeTypeSubTag:
    case ParseMagicMatchRule:
        if (startElement == QLatin1String(mimeTypeTagC)) // Sequence of <mime-type>
            return ParseMimeType;
        if (startElement == QLatin1String(commentTagC))
            return ParseComment;
        if (startElement == QLatin1String(genericIconTagC))
            return ParseGenericIcon;
        if (startElement == QLatin1String(iconTagC))
            return ParseIcon;
        if (startElement == QLatin1String(globTagC))
            return ParseGlobPattern;
        if (startElement == QLatin1String(globDeleteAllTagC))
            return ParseGlobDeleteAll;
        if (startElement == QLatin1String(subClassTagC))
            return ParseSubClass;
        if (startElement == QLatin1String(aliasTagC))
            return ParseAlias;
        if (startElement == QLatin1String(magicTagC))
            return ParseMagic;
        if (startElement == QLatin1String(matchTagC))
            return ParseMagicMatchRule;
        return ParseOtherMimeTypeSubTag;
    case ParseMagic:
        if (startElement == QLatin1String(matchTagC))
            return ParseMagicMatchRule;
        break;
    case ParseError:
        break;
    }
    return ParseError;
}

// Parse int number from an (attribute) string
bool QMimeTypeParserBase::parseNumber(QStringView n, int *target, QString *errorMessage)
{
    bool ok;
    *target = n.toInt(&ok);
    if (Q_UNLIKELY(!ok)) {
        if (errorMessage)
            *errorMessage = QLatin1String("Not a number '") + n + QLatin1String("'.");
        return false;
    }
    return true;
}

#ifndef QT_NO_XMLSTREAMREADER
struct CreateMagicMatchRuleResult
{
    QString errorMessage; // must be first
    QMimeMagicRule rule;

    CreateMagicMatchRuleResult(QStringView type, QStringView value, QStringView offsets, QStringView mask)
        : errorMessage(), rule(type.toString(), value.toUtf8(), offsets.toString(), mask.toLatin1(), &errorMessage)
    {

    }
};

static CreateMagicMatchRuleResult createMagicMatchRule(const QXmlStreamAttributes &atts)
{
    const auto type = atts.value(QLatin1String(matchTypeAttributeC));
    const auto value = atts.value(QLatin1String(matchValueAttributeC));
    const auto offsets = atts.value(QLatin1String(matchOffsetAttributeC));
    const auto mask = atts.value(QLatin1String(matchMaskAttributeC));
    return CreateMagicMatchRuleResult(type, value, offsets, mask);
}
#endif

bool QMimeTypeParserBase::parse(QIODevice *dev, const QString &fileName, QString *errorMessage)
{
#ifdef QT_NO_XMLSTREAMREADER
    Q_UNUSED(dev);
    if (errorMessage)
        *errorMessage = QString::fromLatin1("QXmlStreamReader is not available, cannot parse '%1'.").arg(fileName);
    return false;
#else
    QMimeTypePrivate data;
    data.loaded = true;
    int priority = 50;
    QStack<QMimeMagicRule *> currentRules; // stack for the nesting of rules
    QList<QMimeMagicRule> rules; // toplevel rules
    QXmlStreamReader reader(dev);
    ParseState ps = ParseBeginning;
    while (!reader.atEnd()) {
        switch (reader.readNext()) {
        case QXmlStreamReader::StartElement: {
            ps = nextState(ps, reader.name());
            const QXmlStreamAttributes atts = reader.attributes();
            switch (ps) {
            case ParseMimeType: { // start parsing a MIME type name
                const QString name = atts.value(QLatin1String(mimeTypeAttributeC)).toString();
                if (name.isEmpty()) {
                    reader.raiseError(QStringLiteral("Missing 'type'-attribute"));
                } else {
                    data.name = name;
                }
            }
                break;
            case ParseGenericIcon:
                data.genericIconName = atts.value(QLatin1String(nameAttributeC)).toString();
                break;
            case ParseIcon:
                data.iconName = atts.value(QLatin1String(nameAttributeC)).toString();
                break;
            case ParseGlobPattern: {
                const QString pattern = atts.value(QLatin1String(patternAttributeC)).toString();
                unsigned weight = atts.value(QLatin1String(weightAttributeC)).toInt();
                const bool caseSensitive = atts.value(QLatin1String(caseSensitiveAttributeC)) == QLatin1String("true");

                if (weight == 0)
                    weight = QMimeGlobPattern::DefaultWeight;

                Q_ASSERT(!data.name.isEmpty());
                const QMimeGlobPattern glob(pattern, data.name, weight, caseSensitive ? Qt::CaseSensitive : Qt::CaseInsensitive);
                if (!process(glob, errorMessage))   // for actual glob matching
                    return false;
                data.addGlobPattern(pattern); // just for QMimeType::globPatterns()
            }
                break;
            case ParseGlobDeleteAll:
                data.globPatterns.clear();
                break;
            case ParseSubClass: {
                const QString inheritsFrom = atts.value(QLatin1String(mimeTypeAttributeC)).toString();
                if (!inheritsFrom.isEmpty())
                    processParent(data.name, inheritsFrom);
            }
                break;
            case ParseComment: {
                // comments have locale attributes.
                QString locale = atts.value(QLatin1String(localeAttributeC)).toString();
                const QString comment = reader.readElementText();
                if (locale.isEmpty())
                    locale = QString::fromLatin1("default");
                data.localeComments.insert(locale, comment);
            }
                break;
            case ParseAlias: {
                const QString alias = atts.value(QLatin1String(mimeTypeAttributeC)).toString();
                if (!alias.isEmpty())
                    processAlias(alias, data.name);
            }
                break;
            case ParseMagic: {
                priority = 50;
                const auto priorityS = atts.value(QLatin1String(priorityAttributeC));
                if (!priorityS.isEmpty()) {
                    if (!parseNumber(priorityS, &priority, errorMessage))
                        return false;

                }
                currentRules.clear();
                //qDebug() << "MAGIC start for mimetype" << data.name;
            }
                break;
            case ParseMagicMatchRule: {
                auto result = createMagicMatchRule(atts);
                if (Q_UNLIKELY(!result.rule.isValid()))
                    qWarning("QMimeDatabase: Error parsing %ls\n%ls",
                             qUtf16Printable(fileName), qUtf16Printable(result.errorMessage));
                QList<QMimeMagicRule> *ruleList;
                if (currentRules.isEmpty())
                    ruleList = &rules;
                else // nest this rule into the proper parent
                    ruleList = &currentRules.top()->m_subMatches;
                ruleList->append(std::move(result.rule));
                //qDebug() << " MATCH added. Stack size was" << currentRules.size();
                currentRules.push(&ruleList->last());
                break;
            }
            case ParseError:
                reader.raiseError(QLatin1String("Unexpected element <") + reader.name() + QLatin1Char('>'));
                break;
            default:
                break;
            }
        }
            break;
        // continue switch QXmlStreamReader::Token...
        case QXmlStreamReader::EndElement: // Finished element
        {
            const auto elementName = reader.name();
            if (elementName == QLatin1String(mimeTypeTagC)) {
                if (!process(QMimeType(data), errorMessage))
                    return false;
                data.clear();
            } else if (elementName == QLatin1String(matchTagC)) {
                // Closing a <match> tag, pop stack
                currentRules.pop();
                //qDebug() << " MATCH closed. Stack size is now" << currentRules.size();
            } else if (elementName == QLatin1String(magicTagC)) {
                //qDebug() << "MAGIC ended, we got" << rules.count() << "rules, with prio" << priority;
                // Finished a <magic> sequence
                QMimeMagicRuleMatcher ruleMatcher(data.name, priority);
                ruleMatcher.addRules(rules);
                processMagicMatcher(ruleMatcher);
                rules.clear();
            }
            break;
        }
        default:
            break;
        }
    }

    if (Q_UNLIKELY(reader.hasError())) {
        if (errorMessage) {
            *errorMessage = QString::asprintf("An error has been encountered at line %lld of %ls: %ls:",
                                              reader.lineNumber(),
                                              qUtf16Printable(fileName),
                                              qUtf16Printable(reader.errorString()));
        }
        return false;
    }

    return true;
#endif //QT_NO_XMLSTREAMREADER
}

QT_END_NAMESPACE
