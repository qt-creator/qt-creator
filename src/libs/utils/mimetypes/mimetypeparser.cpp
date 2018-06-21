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

#include "mimetypeparser_p.h"

#include "mimetype_p.h"
#include "mimemagicrulematcher_p.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QDebug>
#include <QtCore/QDir>
#include <QtCore/QPair>
#include <QtCore/QXmlStreamReader>
#include <QtCore/QXmlStreamWriter>
#include <QtCore/QStack>

using namespace Utils;
using namespace Utils::Internal;

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
    \class MimeTypeParser
    \inmodule QtCore
    \internal
    \brief The MimeTypeParser class parses MIME types, and builds a MIME database hierarchy by adding to MimeDatabasePrivate.

    Populates MimeDataBase

    \sa MimeDatabase, MimeMagicRuleMatcher, MagicRule, MagicStringRule, MagicByteRule, GlobPattern
    \sa MimeTypeParser
*/

/*!
    \class MimeTypeParserBase
    \inmodule QtCore
    \internal
    \brief The MimeTypeParserBase class parses for a sequence of <mime-type> in a generic way.

    Calls abstract handler function process for MimeType it finds.

    \sa MimeDatabase, MimeMagicRuleMatcher, MagicRule, MagicStringRule, MagicByteRule, GlobPattern
    \sa MimeTypeParser
*/

/*!
    \fn virtual bool MimeTypeParserBase::process(const MimeType &t, QString *errorMessage) = 0;
    Overwrite to process the sequence of parsed data
*/

MimeTypeParserBase::ParseState MimeTypeParserBase::nextState(ParseState currentState, const QStringRef &startElement)
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
    case ParseSubClass:
    case ParseAlias:
    case ParseOtherMimeTypeSubTag:
    case ParseMagicMatchRule:
        if (startElement == QLatin1String(mimeTypeTagC)) // Sequence of <mime-type>
            return ParseMimeType;
        if (startElement == QLatin1String(commentTagC ))
            return ParseComment;
        if (startElement == QLatin1String(genericIconTagC))
            return ParseGenericIcon;
        if (startElement == QLatin1String(iconTagC))
            return ParseIcon;
        if (startElement == QLatin1String(globTagC))
            return ParseGlobPattern;
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

// Parse int number from an (attribute) string)
static bool parseNumber(const QString &n, int *target, QString *errorMessage)
{
    bool ok;
    *target = n.toInt(&ok);
    if (!ok) {
        if (errorMessage)
            *errorMessage = QString::fromLatin1("Not a number '%1'.").arg(n);
        return false;
    }
    return true;
}

// Evaluate a magic match rule like
//  <match value="must be converted with BinHex" type="string" offset="11"/>
//  <match value="0x9501" type="big16" offset="0:64"/>
#ifndef QT_NO_XMLSTREAMREADER
static bool createMagicMatchRule(const QXmlStreamAttributes &atts,
                                 QString *errorMessage, MimeMagicRule *&rule)
{
    const QString type = atts.value(QLatin1String(matchTypeAttributeC)).toString();
    MimeMagicRule::Type magicType = MimeMagicRule::type(type.toLatin1());
    if (magicType == MimeMagicRule::Invalid) {
        qWarning("%s: match type %s is not supported.", Q_FUNC_INFO, type.toUtf8().constData());
        return true;
    }
    const QString value = atts.value(QLatin1String(matchValueAttributeC)).toString();
    // Parse for offset as "1" or "1:10"
    int startPos, endPos;
    const QString offsetS = atts.value(QLatin1String(matchOffsetAttributeC)).toString();
    const int colonIndex = offsetS.indexOf(QLatin1Char(':'));
    const QString startPosS = colonIndex == -1 ? offsetS : offsetS.mid(0, colonIndex);
    const QString endPosS   = colonIndex == -1 ? offsetS : offsetS.mid(colonIndex + 1);
    if (!parseNumber(startPosS, &startPos, errorMessage) || !parseNumber(endPosS, &endPos, errorMessage))
        return false;
    const QString mask = atts.value(QLatin1String(matchMaskAttributeC)).toString();

    MimeMagicRule *tempRule = new MimeMagicRule(magicType, value.toUtf8(), startPos, endPos,
                                                mask.toLatin1(), errorMessage);
    if (!tempRule->isValid()) {
        delete tempRule;
        return false;
    }

    rule = tempRule;
    return true;
}
#endif

bool MimeTypeParserBase::parse(const QByteArray &content, const QString &fileName, QString *errorMessage)
{
#ifdef QT_NO_XMLSTREAMREADER
    if (errorMessage)
        *errorMessage = QString::fromLatin1("QXmlStreamReader is not available, cannot parse.");
    return false;
#else
    MimeTypePrivate data;
    int priority = 50;
    QStack<MimeMagicRule *> currentRules; // stack for the nesting of rules
    QList<MimeMagicRule> rules; // toplevel rules
    QXmlStreamReader reader(content);
    ParseState ps = ParseBeginning;
    QXmlStreamAttributes atts;
    bool ignoreCurrentMimeType = false;
    while (!reader.atEnd()) {
        switch (reader.readNext()) {
        case QXmlStreamReader::StartElement:
            if (ignoreCurrentMimeType)
                continue;
            ps = nextState(ps, reader.name());
            atts = reader.attributes();
            switch (ps) {
            case ParseMimeType: { // start parsing a MIME type name
                const QString name = atts.value(QLatin1String(mimeTypeAttributeC)).toString();
                if (name.isEmpty()) {
                    reader.raiseError(QString::fromLatin1("Missing '%1'-attribute").arg(QString::fromLatin1(mimeTypeAttributeC)));
                } else {
                    if (mimeTypeExists(name))
                        ignoreCurrentMimeType = true;
                    else
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
                unsigned weight = atts.value(QLatin1String(weightAttributeC)).toString().toInt();
                const bool caseSensitive = atts.value(QLatin1String(caseSensitiveAttributeC)).toString() == QLatin1String("true");

                if (weight == 0)
                    weight = MimeGlobPattern::DefaultWeight;

                Q_ASSERT(!data.name.isEmpty());
                const MimeGlobPattern glob(pattern, data.name, weight, caseSensitive ? Qt::CaseSensitive : Qt::CaseInsensitive);
                if (!process(glob, errorMessage))   // for actual glob matching
                    return false;
                data.addGlobPattern(pattern); // just for MimeType::globPatterns()
            }
                break;
            case ParseSubClass: {
                const QString inheritsFrom = atts.value(QLatin1String(mimeTypeAttributeC)).toString();
                if (!inheritsFrom.isEmpty())
                    processParent(data.name, inheritsFrom);
            }
                break;
            case ParseComment: {
                // comments have locale attributes. We want the default, English one
                QString locale = atts.value(QLatin1String(localeAttributeC)).toString();
                const QString comment = reader.readElementText();
                if (locale.isEmpty())
                    locale = QString::fromLatin1("en_US");
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
                const QString priorityS = atts.value(QLatin1String(priorityAttributeC)).toString();
                if (!priorityS.isEmpty()) {
                    if (!parseNumber(priorityS, &priority, errorMessage))
                        return false;

                }
                currentRules.clear();
                //qDebug() << "MAGIC start for mimetype" << data.name;
            }
                break;
            case ParseMagicMatchRule: {
                MimeMagicRule *rule = 0;
                if (!createMagicMatchRule(atts, errorMessage, rule))
                    return false;
                QList<MimeMagicRule> *ruleList;
                if (currentRules.isEmpty())
                    ruleList = &rules;
                else // nest this rule into the proper parent
                    ruleList = &currentRules.top()->m_subMatches;
                ruleList->append(*rule);
                //qDebug() << " MATCH added. Stack size was" << currentRules.size();
                currentRules.push(&ruleList->last());
                delete rule;
                break;
            }
            case ParseError:
                reader.raiseError(QString::fromLatin1("Unexpected element <%1>").
                                  arg(reader.name().toString()));
                break;
            default:
                break;
            }
            break;
        // continue switch QXmlStreamReader::Token...
        case QXmlStreamReader::EndElement: // Finished element
        {
            const QStringRef elementName = reader.name();
            if (elementName == QLatin1String(mimeTypeTagC)) {
                if (!ignoreCurrentMimeType) {
                    if (!process(MimeType(data), errorMessage))
                        return false;
                }
                ignoreCurrentMimeType = false;
                data.clear();
            } else if (!ignoreCurrentMimeType) {
                if (elementName == QLatin1String(matchTagC)) {
                    // Closing a <match> tag, pop stack
                    currentRules.pop();
                    //qDebug() << " MATCH closed. Stack size is now" << currentRules.size();
                } else if (elementName == QLatin1String(magicTagC)) {
                    //qDebug() << "MAGIC ended, we got" << rules.count() << "rules, with prio" << priority;
                    // Finished a <magic> sequence
                    MimeMagicRuleMatcher ruleMatcher(data.name, priority);
                    ruleMatcher.addRules(rules);
                    processMagicMatcher(ruleMatcher);
                    rules.clear();
                }
            }
            break;
        }
        default:
            break;
        }
    }

    if (reader.hasError()) {
        if (errorMessage)
            *errorMessage = QString::fromLatin1("An error has been encountered at line %1 of %2: %3:").arg(reader.lineNumber()).arg(fileName, reader.errorString());
        return false;
    }

    return true;
#endif //QT_NO_XMLSTREAMREADER
}
