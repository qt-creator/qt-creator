/***************************************************************************
**
** Copyright (C) 2015 Jochen Becher
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "stereotypedefinitionparser.h"

#include "textscanner.h"
#include "token.h"
#include "qmt/infrastructure/qmtassert.h"
#include "qmt/stereotype/stereotypeicon.h"
#include "qmt/stereotype/shapevalue.h"
#include "qmt/stereotype/toolbar.h"

#include <QHash>
#include <QSet>
#include <QPair>
#include <QDebug>

namespace qmt {

// Icon Definition
static const int KEYWORD_ICON          =  1;
static const int KEYWORD_TITLE         =  2;
static const int KEYWORD_ELEMENTS      =  3;
static const int KEYWORD_STEREOTYPE    =  4;
static const int KEYWORD_WIDTH         =  5;
static const int KEYWORD_HEIGHT        =  6;
static const int KEYWORD_MINWIDTH      =  7;
static const int KEYWORD_MINHEIGHT     =  8;
static const int KEYWORD_LOCK_SIZE     =  9;
static const int KEYWORD_DISPLAY       = 10;
static const int KEYWORD_TEXTALIGN     = 11;
static const int KEYWORD_BASECOLOR     = 12;

static const int KEYWORD_BEGIN         = 20;
static const int KEYWORD_END           = 21;

// Icon Commands
static const int KEYWORD_CIRCLE        = 30;
static const int KEYWORD_ELLIPSE       = 31;
static const int KEYWORD_LINE          = 32;
static const int KEYWORD_RECT          = 33;
static const int KEYWORD_ROUNDEDRECT   = 34;
static const int KEYWORD_ARC           = 35;
static const int KEYWORD_MOVETO        = 36;
static const int KEYWORD_LINETO        = 37;
static const int KEYWORD_ARCMOVETO     = 38;
static const int KEYWORD_ARCTO         = 39;
static const int KEYWORD_CLOSE         = 40;

// Toolbar Definition
static const int KEYWORD_TOOLBAR       = 50;
static const int KEYWORD_PRIORITY      = 51;
static const int KEYWORD_TOOL          = 52;
static const int KEYWORD_SEPARATOR     = 53;

// Operatoren
static const int OPERATOR_SEMICOLON  =  1;
static const int OPERATOR_COLON      =  2;
static const int OPERATOR_COMMA      =  3;
static const int OPERATOR_PERIOD     =  4;
static const int OPERATOR_MINUS      =  5;

template <typename T, typename U>
QHash<T, U> operator<<(QHash<T, U> hash, QPair<T, U> pair) {
    hash.insert(pair.first, pair.second);
    return hash;
}

StereotypeDefinitionParserError::StereotypeDefinitionParserError(const QString &errorMsg, const SourcePos &sourcePos)
    : Exception(errorMsg),
      m_sourcePos(sourcePos)
{
}

StereotypeDefinitionParserError::~StereotypeDefinitionParserError()
{
}

class StereotypeDefinitionParser::StereotypeDefinitionParserPrivate
{
public:
    TextScanner *m_scanner = 0;

};

class StereotypeDefinitionParser::IconCommandParameter
{
public:
    IconCommandParameter(ShapeValueF::Unit unit, ShapeValueF::Origin origin = ShapeValueF::OriginSmart)
        : m_unit(unit),
          m_origin(origin)
    {
    }

    ShapeValueF::Unit m_unit;
    ShapeValueF::Origin m_origin;
};

StereotypeDefinitionParser::StereotypeDefinitionParser(QObject *parent) :
    QObject(parent),
    d(new StereotypeDefinitionParserPrivate)
{
}

StereotypeDefinitionParser::~StereotypeDefinitionParser()
{
    delete d;
}

void StereotypeDefinitionParser::parse(ITextSource *source)
{
    TextScanner textScanner;
    textScanner.setKeywords(
                QList<QPair<QString, int> >()
                << qMakePair(QString(QStringLiteral("icon")), KEYWORD_ICON)
                << qMakePair(QString(QStringLiteral("title")), KEYWORD_TITLE)
                << qMakePair(QString(QStringLiteral("elements")), KEYWORD_ELEMENTS)
                << qMakePair(QString(QStringLiteral("stereotype")), KEYWORD_STEREOTYPE)
                << qMakePair(QString(QStringLiteral("width")), KEYWORD_WIDTH)
                << qMakePair(QString(QStringLiteral("height")), KEYWORD_HEIGHT)
                << qMakePair(QString(QStringLiteral("minwidth")), KEYWORD_MINWIDTH)
                << qMakePair(QString(QStringLiteral("minheight")), KEYWORD_MINHEIGHT)
                << qMakePair(QString(QStringLiteral("locksize")), KEYWORD_LOCK_SIZE)
                << qMakePair(QString(QStringLiteral("display")), KEYWORD_DISPLAY)
                << qMakePair(QString(QStringLiteral("textalignment")), KEYWORD_TEXTALIGN)
                << qMakePair(QString(QStringLiteral("basecolor")), KEYWORD_BASECOLOR)
                << qMakePair(QString(QStringLiteral("begin")), KEYWORD_BEGIN)
                << qMakePair(QString(QStringLiteral("end")), KEYWORD_END)
                << qMakePair(QString(QStringLiteral("circle")), KEYWORD_CIRCLE)
                << qMakePair(QString(QStringLiteral("ellipse")), KEYWORD_ELLIPSE)
                << qMakePair(QString(QStringLiteral("line")), KEYWORD_LINE)
                << qMakePair(QString(QStringLiteral("rect")), KEYWORD_RECT)
                << qMakePair(QString(QStringLiteral("roundedrect")), KEYWORD_ROUNDEDRECT)
                << qMakePair(QString(QStringLiteral("arc")), KEYWORD_ARC)
                << qMakePair(QString(QStringLiteral("moveto")), KEYWORD_MOVETO)
                << qMakePair(QString(QStringLiteral("lineto")), KEYWORD_LINETO)
                << qMakePair(QString(QStringLiteral("arcmoveto")), KEYWORD_ARCMOVETO)
                << qMakePair(QString(QStringLiteral("arcto")), KEYWORD_ARCTO)
                << qMakePair(QString(QStringLiteral("close")), KEYWORD_CLOSE)
                << qMakePair(QString(QStringLiteral("toolbar")), KEYWORD_TOOLBAR)
                << qMakePair(QString(QStringLiteral("priority")), KEYWORD_PRIORITY)
                << qMakePair(QString(QStringLiteral("tool")), KEYWORD_TOOL)
                << qMakePair(QString(QStringLiteral("separator")), KEYWORD_SEPARATOR)
                );
    textScanner.setOperators(
                QList<QPair<QString, int> >()
                << qMakePair(QString(QStringLiteral(";")), OPERATOR_SEMICOLON)
                << qMakePair(QString(QStringLiteral(":")), OPERATOR_COLON)
                << qMakePair(QString(QStringLiteral(",")), OPERATOR_COMMA)
                << qMakePair(QString(QStringLiteral(".")), OPERATOR_PERIOD)
                << qMakePair(QString(QStringLiteral("-")), OPERATOR_MINUS)
                );
    textScanner.setSource(source);

    d->m_scanner = &textScanner;
    try {
        parseFile();
    } catch (...) {
        d->m_scanner = 0;
        throw;
    }
    d->m_scanner = 0;
}

void StereotypeDefinitionParser::parseFile()
{
    for (;;) {
        Token token = readNextToken();
        if (token.type() == Token::TokenEndOfInput)
            break;
        else if (token.type() != Token::TokenKeyword || token.subtype() == KEYWORD_ICON)
            parseIcon();
        else if (token.type() != Token::TokenKeyword || token.subtype() == KEYWORD_TOOLBAR)
            parseToolbar();
        else
            throw StereotypeDefinitionParserError(QStringLiteral("Expected 'icon' or 'toolbar'."), token.sourcePos());
        token = d->m_scanner->read();
        if (token.type() == Token::TokenOperator && token.subtype() == OPERATOR_PERIOD)
            break;
        else if (token.type() != Token::TokenOperator || token.subtype() != OPERATOR_SEMICOLON)
            d->m_scanner->unread(token);
    }
}

void StereotypeDefinitionParser::parseIcon()
{
    Token token = d->m_scanner->read();
    if (token.type() != Token::TokenIdentifier && token.type() != Token::TokenKeyword)
        throw StereotypeDefinitionParserError(QStringLiteral("Expected identifier."), token.sourcePos());
    QString id = token.text();
    StereotypeIcon stereotypeIcon;
    stereotypeIcon.setId(id);
    parseIconProperties(&stereotypeIcon);
    token = readNextToken();
    if (token.type() != Token::TokenKeyword || token.subtype() != KEYWORD_BEGIN)
        throw StereotypeDefinitionParserError(QStringLiteral("Expected token 'begin'."), token.sourcePos());
    parseIconCommands(&stereotypeIcon);
    token = readNextToken();
    if (token.type() != Token::TokenKeyword || token.subtype() != KEYWORD_END)
        throw StereotypeDefinitionParserError(QStringLiteral("Expected token 'end'."), token.sourcePos());
    emit iconParsed(stereotypeIcon);
}

void StereotypeDefinitionParser::parseIconProperties(StereotypeIcon *stereotypeIcon)
{
    Token token;
    bool loop = true;
    QSet<StereotypeIcon::Element> elements;
    QSet<QString> stereotypes;
    while (loop) {
        token = readNextToken();
        if (token.type() != Token::TokenKeyword) {
            loop = false;
        } else {
            switch (token.subtype()) {
            case KEYWORD_TITLE:
                stereotypeIcon->setTitle(parseStringProperty());
                expectSemicolonOrEndOfLine();
                break;
            case KEYWORD_ELEMENTS:
            {
                QList<QString> identifiers = parseIdentifierListProperty();
                foreach (const QString &identifier, identifiers) {
                    static QHash<QString, StereotypeIcon::Element> elementNames = QHash<QString, StereotypeIcon::Element>()
                            << qMakePair(QString(QStringLiteral("package")), StereotypeIcon::ElementPackage)
                            << qMakePair(QString(QStringLiteral("component")), StereotypeIcon::ElementComponent)
                            << qMakePair(QString(QStringLiteral("class")), StereotypeIcon::ElementClass)
                            << qMakePair(QString(QStringLiteral("diagram")), StereotypeIcon::ElementDiagram)
                            << qMakePair(QString(QStringLiteral("item")), StereotypeIcon::ElementItem);
                    QString elementName = identifier.toLower();
                    if (!elementNames.contains(elementName))
                        throw StereotypeDefinitionParserError(QString(QStringLiteral("Unexpected value \"%1\" for element.")).arg(identifier), token.sourcePos());
                    elements.insert(elementNames.value(elementName));
                }
                expectSemicolonOrEndOfLine();
                break;
            }
            case KEYWORD_STEREOTYPE:
                stereotypes.insert(parseStringProperty());
                expectSemicolonOrEndOfLine();
                break;
            case KEYWORD_WIDTH:
                stereotypeIcon->setWidth(parseFloatProperty());
                expectSemicolonOrEndOfLine();
                break;
            case KEYWORD_HEIGHT:
                stereotypeIcon->setHeight(parseFloatProperty());
                expectSemicolonOrEndOfLine();
                break;
            case KEYWORD_MINWIDTH:
                stereotypeIcon->setMinWidth(parseFloatProperty());
                expectSemicolonOrEndOfLine();
                break;
            case KEYWORD_MINHEIGHT:
                stereotypeIcon->setMinHeight(parseFloatProperty());
                expectSemicolonOrEndOfLine();
                break;
            case KEYWORD_LOCK_SIZE:
            {
                QString lockValue = parseIdentifierProperty();
                QString lockName = lockValue.toLower();
                static QHash<QString, StereotypeIcon::SizeLock> lockNames = QHash<QString, StereotypeIcon::SizeLock>()
                        << qMakePair(QString(QStringLiteral("none")), StereotypeIcon::LockNone)
                        << qMakePair(QString(QStringLiteral("width")), StereotypeIcon::LockWidth)
                        << qMakePair(QString(QStringLiteral("height")), StereotypeIcon::LockHeight)
                        << qMakePair(QString(QStringLiteral("size")), StereotypeIcon::LockSize)
                        << qMakePair(QString(QStringLiteral("ratio")), StereotypeIcon::LockRatio);
                if (lockNames.contains(lockName)) {
                    StereotypeIcon::SizeLock sizeLock = lockNames.value(lockName);
                    stereotypeIcon->setSizeLock(sizeLock);
                } else {
                    throw StereotypeDefinitionParserError(QString(QStringLiteral("Unexpected value \"%1\" for size lock.")).arg(lockValue), token.sourcePos());
                }
                break;
            }
            case KEYWORD_DISPLAY:
            {
                QString displayValue = parseIdentifierProperty();
                QString displayName = displayValue.toLower();
                static QHash<QString, StereotypeIcon::Display> displayNames = QHash<QString, StereotypeIcon::Display>()
                        << qMakePair(QString(QStringLiteral("none")), StereotypeIcon::DisplayNone)
                        << qMakePair(QString(QStringLiteral("label")), StereotypeIcon::DisplayLabel)
                        << qMakePair(QString(QStringLiteral("decoration")), StereotypeIcon::DisplayDecoration)
                        << qMakePair(QString(QStringLiteral("icon")), StereotypeIcon::DisplayIcon)
                        << qMakePair(QString(QStringLiteral("smart")), StereotypeIcon::DisplaySmart);
                if (displayNames.contains(displayName)) {
                    StereotypeIcon::Display display = displayNames.value(displayName);
                    stereotypeIcon->setDisplay(display);
                } else {
                    throw StereotypeDefinitionParserError(QString(QStringLiteral("Unexpected value \"%1\" for stereotype display.")).arg(displayValue), token.sourcePos());
                }
                break;
            }
            case KEYWORD_TEXTALIGN:
            {
                QString alignValue = parseIdentifierProperty();
                QString alignName = alignValue.toLower();
                static QHash<QString, StereotypeIcon::TextAlignment> alignNames = QHash<QString, StereotypeIcon::TextAlignment>()
                        << qMakePair(QString(QStringLiteral("below")), StereotypeIcon::TextalignBelow)
                        << qMakePair(QString(QStringLiteral("center")), StereotypeIcon::TextalignCenter)
                        << qMakePair(QString(QStringLiteral("none")), StereotypeIcon::TextalignNone);
                if (alignNames.contains(alignName)) {
                    StereotypeIcon::TextAlignment textAlignment = alignNames.value(alignName);
                    stereotypeIcon->setTextAlignment(textAlignment);
                } else {
                    throw StereotypeDefinitionParserError(QString(QStringLiteral("Unexpected value \"%1\" for text alignment.")).arg(alignValue), token.sourcePos());
                }
                break;
            }
            case KEYWORD_BASECOLOR:
                stereotypeIcon->setBaseColor(parseColorProperty());
                expectSemicolonOrEndOfLine();
                break;
            default:
                loop = false;
                break;
            }
        }
    }
    stereotypeIcon->setElements(elements);
    stereotypeIcon->setStereotypes(stereotypes);
    d->m_scanner->unread(token);
}

void StereotypeDefinitionParser::parseToolbar()
{
    Token token = d->m_scanner->read();
    if (token.type() != Token::TokenIdentifier && token.type() != Token::TokenKeyword)
        throw StereotypeDefinitionParserError(QStringLiteral("Expected identifier."), token.sourcePos());
    QString id = token.text();
    Toolbar toolbar;
    toolbar.setId(id);
    parseToolbarProperties(&toolbar);
    token = readNextToken();
    if (token.type() != Token::TokenKeyword || token.subtype() != KEYWORD_BEGIN)
        throw StereotypeDefinitionParserError(QStringLiteral("Expected token 'begin'."), token.sourcePos());
    parseToolbarCommands(&toolbar);
    token = readNextToken();
    if (token.type() != Token::TokenKeyword || token.subtype() != KEYWORD_END)
        throw StereotypeDefinitionParserError(QStringLiteral("Expected token 'end'."), token.sourcePos());
    emit toolbarParsed(toolbar);
}

void StereotypeDefinitionParser::parseToolbarProperties(Toolbar *toolbar)
{
    Token token;
    bool loop = true;
    while (loop) {
        token = readNextToken();
        if (token.type() != Token::TokenKeyword) {
            loop = false;
        } else {
            switch (token.subtype()) {
            case KEYWORD_PRIORITY:
                toolbar->setPriority(parseIntProperty());
                break;
            default:
                loop = false;
                break;
            }
        }
    }
    d->m_scanner->unread(token);
}

QString StereotypeDefinitionParser::parseStringProperty()
{
    expectColon();
    return parseStringExpression();
}

int StereotypeDefinitionParser::parseIntProperty()
{
    expectColon();
    return parseIntExpression();
}

qreal StereotypeDefinitionParser::parseFloatProperty()
{
    expectColon();
    return parseFloatExpression();
}

QString StereotypeDefinitionParser::parseIdentifierProperty()
{
    expectColon();
    return parseIdentifierExpression();
}

QList<QString> StereotypeDefinitionParser::parseIdentifierListProperty()
{
    QList<QString> identifiers;
    expectColon();
    for (;;) {
        Token token = d->m_scanner->read();
        if (token.type() != Token::TokenIdentifier && token.type() != Token::TokenKeyword) {
            qDebug() << "token" << token.type() << token.subtype() << token.text();
            throw StereotypeDefinitionParserError(QStringLiteral("Expected identifier."), token.sourcePos());
        }
        identifiers.append(token.text());
        token = d->m_scanner->read();
        if (token.type() != Token::TokenOperator || token.subtype() != OPERATOR_COMMA) {
            d->m_scanner->unread(token);
            break;
        }
    }
    return identifiers;
}

bool StereotypeDefinitionParser::parseBoolProperty()
{
    expectColon();
    return parseBoolExpression();
}

QColor StereotypeDefinitionParser::parseColorProperty()
{
    expectColon();
    return parseColorExpression();
}

void StereotypeDefinitionParser::parseIconCommands(StereotypeIcon *stereotypeIcon)
{
    Token token;
    bool loop = true;
    IconShape iconShape;
    QList<ShapeValueF> parameters;

    typedef QList<IconCommandParameter> Parameters;
    static const IconCommandParameter SCALED(ShapeValueF::UnitScaled);
    static const IconCommandParameter FIX(ShapeValueF::UnitRelative);
    static const IconCommandParameter ABSOLUTE(ShapeValueF::UnitAbsolute);

    while (loop) {
        token = readNextToken();
        if (token.type() != Token::TokenKeyword) {
            loop = false;
        } else {
            switch (token.subtype()) {
            case KEYWORD_CIRCLE:
                parameters = parseIconCommandParameters(Parameters() << SCALED << SCALED << SCALED);
                iconShape.addCircle(ShapePointF(parameters.at(0), parameters.at(1)), parameters.at(2));
                expectSemicolonOrEndOfLine();
                break;
            case KEYWORD_ELLIPSE:
                parameters = parseIconCommandParameters(Parameters() << SCALED << SCALED << SCALED << SCALED);
                iconShape.addEllipse(ShapePointF(parameters.at(0), parameters.at(1)),
                                     ShapeSizeF(parameters.at(2), parameters.at(3)));
                expectSemicolonOrEndOfLine();
                break;
            case KEYWORD_LINE:
                parameters = parseIconCommandParameters(Parameters() << SCALED << SCALED << SCALED << SCALED);
                iconShape.addLine(ShapePointF(parameters.at(0), parameters.at(1)),
                                  ShapePointF(parameters.at(2), parameters.at(3)));
                expectSemicolonOrEndOfLine();
                break;
            case KEYWORD_RECT:
                parameters = parseIconCommandParameters(Parameters() << SCALED << SCALED << SCALED << SCALED);
                iconShape.addRect(ShapePointF(parameters.at(0), parameters.at(1)),
                                  ShapeSizeF(parameters.at(2), parameters.at(3)));
                expectSemicolonOrEndOfLine();
                break;
            case KEYWORD_ROUNDEDRECT:
                parameters = parseIconCommandParameters(Parameters() << SCALED << SCALED << SCALED << SCALED << FIX);
                iconShape.addRoundedRect(ShapePointF(parameters.at(0), parameters.at(1)),
                                         ShapeSizeF(parameters.at(2), parameters.at(3)), parameters.at(4));
                expectSemicolonOrEndOfLine();
                break;
            case KEYWORD_ARC:
            {
                parameters = parseIconCommandParameters(
                            Parameters() << SCALED << SCALED << SCALED << SCALED << ABSOLUTE << ABSOLUTE);
                qreal startAngle = expectAbsoluteValue(parameters.at(4), d->m_scanner->sourcePos());
                qreal spanAngle = expectAbsoluteValue(parameters.at(5), d->m_scanner->sourcePos());
                iconShape.addArc(ShapePointF(parameters.at(0), parameters.at(1)),
                                 ShapeSizeF(parameters.at(2), parameters.at(3)), startAngle, spanAngle);
                expectSemicolonOrEndOfLine();
                break;
            }
            case KEYWORD_MOVETO:
                parameters = parseIconCommandParameters(Parameters() << SCALED << SCALED);
                iconShape.moveTo(ShapePointF(parameters.at(0), parameters.at(1)));
                expectSemicolonOrEndOfLine();
                break;
            case KEYWORD_LINETO:
                parameters = parseIconCommandParameters(Parameters() << SCALED << SCALED);
                iconShape.lineTo(ShapePointF(parameters.at(0), parameters.at(1)));
                expectSemicolonOrEndOfLine();
                break;
            case KEYWORD_ARCMOVETO:
            {
                parameters = parseIconCommandParameters(
                            Parameters() << SCALED << SCALED << SCALED << SCALED << ABSOLUTE);
                qreal angle = expectAbsoluteValue(parameters.at(4), d->m_scanner->sourcePos());
                iconShape.arcMoveTo(ShapePointF(parameters.at(0), parameters.at(1)),
                                    ShapeSizeF(parameters.at(2), parameters.at(3)), angle);
                expectSemicolonOrEndOfLine();
                break;
            }
            case KEYWORD_ARCTO:
            {
                parameters = parseIconCommandParameters(
                            Parameters() << SCALED << SCALED << SCALED << SCALED << ABSOLUTE << ABSOLUTE);
                qreal startAngle = expectAbsoluteValue(parameters.at(4), d->m_scanner->sourcePos());
                qreal sweepLength = expectAbsoluteValue(parameters.at(5), d->m_scanner->sourcePos());
                iconShape.arcTo(ShapePointF(parameters.at(0), parameters.at(1)),
                                ShapeSizeF(parameters.at(2), parameters.at(3)), startAngle, sweepLength);
                expectSemicolonOrEndOfLine();
                break;
            }
            case KEYWORD_CLOSE:
                iconShape.closePath();
                expectSemicolonOrEndOfLine();
                break;
            default:
                loop = false;
                break;
            }
        }
    }
    stereotypeIcon->setIconShape(iconShape);
    d->m_scanner->unread(token);
}

QList<ShapeValueF> StereotypeDefinitionParser::parseIconCommandParameters(const QList<IconCommandParameter> &parameters)
{
    QList<ShapeValueF> values;
    Token token;
    for (;;) {
        if (values.count() <= parameters.count())
            values << ShapeValueF(parseFloatExpression(), parameters.at(values.count()).m_unit,
                                  parameters.at(values.count()).m_origin);
        else
            values << ShapeValueF(parseFloatExpression());
        token = d->m_scanner->read();
        if (token.type() != Token::TokenOperator || token.subtype() != OPERATOR_COMMA) {
            d->m_scanner->unread(token);
            break;
        }
    }
    if (values.count() < parameters.count())
        throw StereotypeDefinitionParserError(QStringLiteral("More parameters expected."), token.sourcePos());
    else if (values.count() > parameters.count())
        throw StereotypeDefinitionParserError(QStringLiteral("Too many parameters given."), token.sourcePos());
    return values;
}

void StereotypeDefinitionParser::parseToolbarCommands(Toolbar *toolbar)
{
    Token token;
    QList<Toolbar::Tool> tools;
    bool loop = true;
    while (loop) {
        token = readNextToken();
        if (token.type() != Token::TokenKeyword) {
            loop = false;
        } else {
            switch (token.subtype()) {
            case KEYWORD_TOOL:
            {
                QString toolName = parseStringExpression();
                expectComma();
                QString element = parseIdentifierExpression();
                static QSet<QString> elementNames = QSet<QString>()
                        << QStringLiteral("package")
                        << QStringLiteral("component")
                        << QStringLiteral("class")
                        << QStringLiteral("item")
                        << QStringLiteral("annotation")
                        << QStringLiteral("boundary");
                QString elementName = element.toLower();
                if (!elementNames.contains(elementName))
                    throw StereotypeDefinitionParserError(QString(QStringLiteral("Unexpected value \"%1\" for element.")).arg(element), token.sourcePos());
                QString stereotype;
                if (nextIsComma())
                    stereotype = parseStringExpression();
                tools.append(Toolbar::Tool(toolName, element, stereotype));
                expectSemicolonOrEndOfLine();
                break;
            }
            case KEYWORD_SEPARATOR:
                tools.append(Toolbar::Tool());
                expectSemicolonOrEndOfLine();
                break;
            default:
                loop = false;
                break;
            }
        }
    }
    toolbar->setTools(tools);
    d->m_scanner->unread(token);
}

QString StereotypeDefinitionParser::parseStringExpression()
{
    Token token = d->m_scanner->read();
    if (token.type() != Token::TokenString)
        throw StereotypeDefinitionParserError(QStringLiteral("Expected string constant."), token.sourcePos());
    return token.text();
}

qreal StereotypeDefinitionParser::parseFloatExpression()
{
    Token token;
    token = d->m_scanner->read();
    if (token.type() == Token::TokenOperator && token.subtype() == OPERATOR_MINUS) {
        return -parseFloatExpression();
    } else {
        bool ok = false;
        if (token.type() == Token::TokenInteger) {
            int value = token.text().toInt(&ok);
            QMT_CHECK(ok);
            return value;
        } else if (token.type() == Token::TokenFloat) {
            qreal value = token.text().toDouble(&ok);
            QMT_CHECK(ok);
            return value;
        } else {
            throw StereotypeDefinitionParserError(QStringLiteral("Expected number constant."), token.sourcePos());
        }
    }
}

int StereotypeDefinitionParser::parseIntExpression()
{
    Token token;
    token = d->m_scanner->read();
    if (token.type() == Token::TokenOperator && token.subtype() == OPERATOR_MINUS) {
        return -parseIntExpression();
    } else {
        bool ok = false;
        if (token.type() == Token::TokenInteger) {
            int value = token.text().toInt(&ok);
            QMT_CHECK(ok);
            return value;
        } else {
            throw StereotypeDefinitionParserError(QStringLiteral("Expected integer constant."), token.sourcePos());
        }
    }
}

QString StereotypeDefinitionParser::parseIdentifierExpression()
{
    Token token = d->m_scanner->read();
    if (token.type() != Token::TokenIdentifier && token.type() != Token::TokenKeyword)
        throw StereotypeDefinitionParserError(QStringLiteral("Expected identifier."), token.sourcePos());
    return token.text();
}

bool StereotypeDefinitionParser::parseBoolExpression()
{
    Token token = d->m_scanner->read();
    if (token.type() == Token::TokenIdentifier) {
        QString value = token.text().toLower();
        if (value == QStringLiteral("yes") || value == QStringLiteral("true"))
            return true;
        else if (value == QStringLiteral("no") || value == QStringLiteral("false"))
            return false;
    }
    throw StereotypeDefinitionParserError(QStringLiteral("Expected 'yes', 'no', 'true' or 'false'."), token.sourcePos());
}

QColor StereotypeDefinitionParser::parseColorExpression()
{
    Token token = d->m_scanner->read();
    if (token.type() == Token::TokenIdentifier || token.type() == Token::TokenColor) {
        QString value = token.text().toLower();
        QColor color;
        if (QColor::isValidColor(value)) {
            color.setNamedColor(value);
            return color;
        }
    }
    throw StereotypeDefinitionParserError(QStringLiteral("Expected color name."), token.sourcePos());
}

Token StereotypeDefinitionParser::readNextToken()
{
    Token token;
    for (;;) {
        token = d->m_scanner->read();
        if (token.type() != Token::TokenEndOfLine)
            return token;
    }
}

qreal StereotypeDefinitionParser::expectAbsoluteValue(const ShapeValueF &value, const SourcePos &sourcePos)
{
    if (value.unit() != ShapeValueF::UnitAbsolute || value.origin() != ShapeValueF::OriginSmart)
        throw StereotypeDefinitionParserError(QStringLiteral("Expected absolute value"), sourcePos);
    return value.value();
}

void StereotypeDefinitionParser::expectSemicolonOrEndOfLine()
{
    Token token = d->m_scanner->read();
    if (token.type() != Token::TokenEndOfLine
            && (token.type() != Token::TokenOperator || token.subtype() != OPERATOR_SEMICOLON)) {
        throw StereotypeDefinitionParserError(QStringLiteral("Expected ';' or end-of-line."), token.sourcePos());
    }
}

bool StereotypeDefinitionParser::nextIsComma()
{
    Token token = d->m_scanner->read();
    if (token.type() != Token::TokenOperator || token.subtype() != OPERATOR_COMMA) {
        d->m_scanner->unread(token);
        return false;
    }
    return true;
}

void StereotypeDefinitionParser::expectOperator(int op, const QString &opName)
{
    Token token = d->m_scanner->read();
    if (token.type() != Token::TokenOperator || token.subtype() != op)
        throw StereotypeDefinitionParserError(QString(QStringLiteral("Expected '%1'.")).arg(opName), token.sourcePos());
}

void StereotypeDefinitionParser::expectComma()
{
    expectOperator(OPERATOR_COMMA, QStringLiteral(","));
}

void StereotypeDefinitionParser::expectColon()
{
    expectOperator(OPERATOR_COLON, QStringLiteral(":"));
}

} // namespace qmt
