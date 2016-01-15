/****************************************************************************
**
** Copyright (C) 2016 Jochen Becher
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
static const int KEYWORD_ID            =  2;
static const int KEYWORD_TITLE         =  3;
static const int KEYWORD_ELEMENTS      =  4;
static const int KEYWORD_STEREOTYPE    =  5;
static const int KEYWORD_WIDTH         =  6;
static const int KEYWORD_HEIGHT        =  7;
static const int KEYWORD_MINWIDTH      =  8;
static const int KEYWORD_MINHEIGHT     =  9;
static const int KEYWORD_LOCK_SIZE     = 10;
static const int KEYWORD_DISPLAY       = 11;
static const int KEYWORD_TEXTALIGN     = 12;
static const int KEYWORD_BASECOLOR     = 13;
static const int KEYWORD_SHAPE         = 14;

// Shape items
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

// Shape item parameters
static const int KEYWORD_X             = 50;
static const int KEYWORD_Y             = 51;
static const int KEYWORD_X0            = 52;
static const int KEYWORD_Y0            = 53;
static const int KEYWORD_X1            = 54;
static const int KEYWORD_Y1            = 55;
static const int KEYWORD_RADIUS        = 56;
static const int KEYWORD_RADIUS_X      = 57;
static const int KEYWORD_RADIUS_Y      = 58;
static const int KEYWORD_START         = 59;
static const int KEYWORD_SPAN          = 60;

// Toolbar Definition
static const int KEYWORD_TOOLBAR       = 70;
static const int KEYWORD_PRIORITY      = 71;
static const int KEYWORD_TOOLS         = 72;
static const int KEYWORD_TOOL          = 73;
static const int KEYWORD_ELEMENT       = 74;
static const int KEYWORD_SEPARATOR     = 75;

// Operatoren
static const int OPERATOR_SEMICOLON    =  1;
static const int OPERATOR_BRACE_OPEN   =  2;
static const int OPERATOR_BRACE_CLOSE  =  3;
static const int OPERATOR_COLON        =  4;
static const int OPERATOR_COMMA        =  5;
static const int OPERATOR_PERIOD       =  6;
static const int OPERATOR_MINUS        =  7;

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
    IconCommandParameter() = default;

    IconCommandParameter(int keyword, ShapeValueF::Unit unit, ShapeValueF::Origin origin = ShapeValueF::OriginSmart)
        : m_keyword(keyword),
          m_unit(unit),
          m_origin(origin)
    {
    }

    int m_keyword = -1;
    ShapeValueF::Unit m_unit;
    ShapeValueF::Origin m_origin;
};

StereotypeDefinitionParser::StereotypeDefinitionParser(QObject *parent)
    : QObject(parent),
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
                << qMakePair(QString(QStringLiteral("id")), KEYWORD_ID)
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
                << qMakePair(QString(QStringLiteral("shape")), KEYWORD_SHAPE)
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
                << qMakePair(QString(QStringLiteral("x")), KEYWORD_X)
                << qMakePair(QString(QStringLiteral("y")), KEYWORD_Y)
                << qMakePair(QString(QStringLiteral("x0")), KEYWORD_X0)
                << qMakePair(QString(QStringLiteral("y0")), KEYWORD_Y0)
                << qMakePair(QString(QStringLiteral("x1")), KEYWORD_X1)
                << qMakePair(QString(QStringLiteral("y1")), KEYWORD_Y1)
                << qMakePair(QString(QStringLiteral("radius")), KEYWORD_RADIUS)
                << qMakePair(QString(QStringLiteral("radiusx")), KEYWORD_RADIUS_X)
                << qMakePair(QString(QStringLiteral("radiusy")), KEYWORD_RADIUS_Y)
                << qMakePair(QString(QStringLiteral("start")), KEYWORD_START)
                << qMakePair(QString(QStringLiteral("span")), KEYWORD_SPAN)
                << qMakePair(QString(QStringLiteral("toolbar")), KEYWORD_TOOLBAR)
                << qMakePair(QString(QStringLiteral("priority")), KEYWORD_PRIORITY)
                << qMakePair(QString(QStringLiteral("tools")), KEYWORD_TOOLS)
                << qMakePair(QString(QStringLiteral("tool")), KEYWORD_TOOL)
                << qMakePair(QString(QStringLiteral("element")), KEYWORD_ELEMENT)
                << qMakePair(QString(QStringLiteral("separator")), KEYWORD_SEPARATOR)
                );
    textScanner.setOperators(
                QList<QPair<QString, int> >()
                << qMakePair(QString(QStringLiteral(";")), OPERATOR_SEMICOLON)
                << qMakePair(QString(QStringLiteral("{")), OPERATOR_BRACE_OPEN)
                << qMakePair(QString(QStringLiteral("}")), OPERATOR_BRACE_CLOSE)
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
        else if (token.type() == Token::TokenKeyword && token.subtype() == KEYWORD_ICON)
            parseIcon();
        else if (token.type() == Token::TokenKeyword && token.subtype() == KEYWORD_TOOLBAR)
            parseToolbar();
        else
            throw StereotypeDefinitionParserError(QStringLiteral("Expected 'Icon' or 'Toolbar'."), token.sourcePos());
    }
}

void StereotypeDefinitionParser::parseIcon()
{
    StereotypeIcon stereotypeIcon;
    QSet<StereotypeIcon::Element> elements;
    QSet<QString> stereotypes;
    expectBlockBegin();
    Token token;
    while (readProperty(&token)) {
        switch (token.subtype()) {
        case KEYWORD_ID:
            stereotypeIcon.setId(parseIdentifierProperty());
            break;
        case KEYWORD_TITLE:
            stereotypeIcon.setTitle(parseStringProperty());
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
            break;
        }
        case KEYWORD_STEREOTYPE:
            stereotypes.insert(parseStringProperty());
            break;
        case KEYWORD_WIDTH:
            stereotypeIcon.setWidth(parseFloatProperty());
            break;
        case KEYWORD_HEIGHT:
            stereotypeIcon.setHeight(parseFloatProperty());
            break;
        case KEYWORD_MINWIDTH:
            stereotypeIcon.setMinWidth(parseFloatProperty());
            break;
        case KEYWORD_MINHEIGHT:
            stereotypeIcon.setMinHeight(parseFloatProperty());
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
                stereotypeIcon.setSizeLock(sizeLock);
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
                stereotypeIcon.setDisplay(display);
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
                stereotypeIcon.setTextAlignment(textAlignment);
            } else {
                throw StereotypeDefinitionParserError(QString(QStringLiteral("Unexpected value \"%1\" for text alignment.")).arg(alignValue), token.sourcePos());
            }
            break;
        }
        case KEYWORD_BASECOLOR:
            stereotypeIcon.setBaseColor(parseColorProperty());
            break;
        case KEYWORD_SHAPE:
            parseIconShape(&stereotypeIcon);
            break;
        default:
            throwUnknownPropertyError(token);
        }
        if (!expectPropertySeparatorOrBlockEnd())
            break;
    }
    stereotypeIcon.setElements(elements);
    stereotypeIcon.setStereotypes(stereotypes);
    if (stereotypeIcon.id().isEmpty())
        throw StereotypeDefinitionParserError(QStringLiteral("Missing id in Icon definition."), d->m_scanner->sourcePos());
    emit iconParsed(stereotypeIcon);
}

QPair<int, StereotypeDefinitionParser::IconCommandParameter> StereotypeDefinitionParser::SCALED(int keyword)
{
    return qMakePair(keyword, IconCommandParameter(keyword, ShapeValueF::UnitScaled));
}

QPair<int, StereotypeDefinitionParser::IconCommandParameter> StereotypeDefinitionParser::FIX(int keyword)
{
    return qMakePair(keyword, IconCommandParameter(keyword, ShapeValueF::UnitRelative));
}

QPair<int, StereotypeDefinitionParser::IconCommandParameter> StereotypeDefinitionParser::ABSOLUTE(int keyword)
{
    return qMakePair(keyword, IconCommandParameter(keyword, ShapeValueF::UnitAbsolute));
}

void StereotypeDefinitionParser::parseIconShape(StereotypeIcon *stereotypeIcon)
{
    IconShape iconShape;
    QHash<int, ShapeValueF> values;
    typedef QHash<int, IconCommandParameter> Parameters;
    expectBlockBegin();
    Token token;
    while (readProperty(&token)) {
        switch (token.subtype()) {
        case KEYWORD_CIRCLE:
            values = parseIconShapeProperties(
                        Parameters() << SCALED(KEYWORD_X) << SCALED(KEYWORD_Y) << SCALED(KEYWORD_RADIUS));
            iconShape.addCircle(ShapePointF(values.value(KEYWORD_X), values.value(KEYWORD_Y)), values.value(KEYWORD_RADIUS));
            break;
        case KEYWORD_ELLIPSE:
            values = parseIconShapeProperties(
                        Parameters() << SCALED(KEYWORD_X) << SCALED(KEYWORD_Y)
                        << SCALED(KEYWORD_RADIUS_X) << SCALED(KEYWORD_RADIUS_Y));
            iconShape.addEllipse(ShapePointF(values.value(KEYWORD_X), values.value(KEYWORD_Y)),
                                 ShapeSizeF(values.value(KEYWORD_RADIUS_X), values.value(KEYWORD_RADIUS_Y)));
            break;
        case KEYWORD_LINE:
            values = parseIconShapeProperties(
                        Parameters() << SCALED(KEYWORD_X0) << SCALED(KEYWORD_Y0)
                        << SCALED(KEYWORD_X1) << SCALED(KEYWORD_Y1));
            iconShape.addLine(ShapePointF(values.value(KEYWORD_X0), values.value(KEYWORD_Y0)),
                              ShapePointF(values.value(KEYWORD_X1), values.value(KEYWORD_Y1)));
            break;
        case KEYWORD_RECT:
            values = parseIconShapeProperties(
                        Parameters() << SCALED(KEYWORD_X) << SCALED(KEYWORD_Y)
                        << SCALED(KEYWORD_WIDTH) << SCALED(KEYWORD_HEIGHT));
            iconShape.addRect(ShapePointF(values.value(KEYWORD_X), values.value(KEYWORD_Y)),
                              ShapeSizeF(values.value(KEYWORD_WIDTH), values.value(KEYWORD_HEIGHT)));
            break;
        case KEYWORD_ROUNDEDRECT:
            values = parseIconShapeProperties(
                        Parameters() << SCALED(KEYWORD_X) << SCALED(KEYWORD_Y)
                        << SCALED(KEYWORD_WIDTH) << SCALED(KEYWORD_HEIGHT) << FIX(KEYWORD_RADIUS));
            iconShape.addRoundedRect(ShapePointF(values.value(KEYWORD_X), values.value(KEYWORD_Y)),
                                     ShapeSizeF(values.value(KEYWORD_WIDTH), values.value(KEYWORD_HEIGHT)),
                                     values.value(KEYWORD_RADIUS));
            break;
        case KEYWORD_ARC:
        {
            values = parseIconShapeProperties(
                        Parameters() << SCALED(KEYWORD_X) << SCALED(KEYWORD_Y)
                        << SCALED(KEYWORD_RADIUS_X) << SCALED(KEYWORD_RADIUS_Y)
                        << ABSOLUTE(KEYWORD_START) << ABSOLUTE(KEYWORD_SPAN));
            qreal startAngle = expectAbsoluteValue(values.value(KEYWORD_START), d->m_scanner->sourcePos());
            qreal spanAngle = expectAbsoluteValue(values.value(KEYWORD_SPAN), d->m_scanner->sourcePos());
            iconShape.addArc(ShapePointF(values.value(KEYWORD_X), values.value(KEYWORD_Y)),
                             ShapeSizeF(values.value(KEYWORD_RADIUS_X), values.value(KEYWORD_RADIUS_Y)),
                             startAngle, spanAngle);
            break;
        }
        case KEYWORD_MOVETO:
            values = parseIconShapeProperties(Parameters() << SCALED(KEYWORD_X) << SCALED(KEYWORD_Y));
            iconShape.moveTo(ShapePointF(values.value(KEYWORD_X), values.value(KEYWORD_Y)));
            break;
        case KEYWORD_LINETO:
            values = parseIconShapeProperties(Parameters() << SCALED(KEYWORD_X) << SCALED(KEYWORD_Y));
            iconShape.lineTo(ShapePointF(values.value(KEYWORD_X), values.value(KEYWORD_Y)));
            break;
        case KEYWORD_ARCMOVETO:
        {
            values = parseIconShapeProperties(
                        Parameters() << SCALED(KEYWORD_X) << SCALED(KEYWORD_Y)
                        << SCALED(KEYWORD_RADIUS_X) << SCALED(KEYWORD_RADIUS_Y) << ABSOLUTE(KEYWORD_START));
            qreal angle = expectAbsoluteValue(values.value(KEYWORD_START), d->m_scanner->sourcePos());
            iconShape.arcMoveTo(ShapePointF(values.value(KEYWORD_X), values.value(KEYWORD_Y)),
                                ShapeSizeF(values.value(KEYWORD_RADIUS_X), values.value(KEYWORD_RADIUS_Y)), angle);
            break;
        }
        case KEYWORD_ARCTO:
        {
            values = parseIconShapeProperties(
                        Parameters() << SCALED(KEYWORD_X) << SCALED(KEYWORD_Y)
                        << SCALED(KEYWORD_RADIUS_X) << SCALED(KEYWORD_RADIUS_Y)
                        << ABSOLUTE(KEYWORD_START) << ABSOLUTE(KEYWORD_SPAN));
            qreal startAngle = expectAbsoluteValue(values.value(KEYWORD_START), d->m_scanner->sourcePos());
            qreal sweepLength = expectAbsoluteValue(values.value(KEYWORD_SPAN), d->m_scanner->sourcePos());
            iconShape.arcTo(ShapePointF(values.value(KEYWORD_X), values.value(KEYWORD_Y)),
                            ShapeSizeF(values.value(KEYWORD_RADIUS_X), values.value(KEYWORD_RADIUS_Y)),
                            startAngle, sweepLength);
            break;
        }
        case KEYWORD_CLOSE:
            iconShape.closePath();
            skipOptionalEmptyBlock();
            break;
        default:
            throwUnknownPropertyError(token);
        }
        if (!expectPropertySeparatorOrBlockEnd())
            break;
    }
    stereotypeIcon->setIconShape(iconShape);
}

QHash<int, ShapeValueF> StereotypeDefinitionParser::parseIconShapeProperties(const QHash<int, IconCommandParameter> &parameters)
{
    expectBlockBegin();
    QHash<int, ShapeValueF> values;
    Token token;
    while (readProperty(&token)) {
        if (parameters.contains(token.subtype())) {
            IconCommandParameter parameter = parameters.value(token.subtype());
            if (values.contains(token.subtype()))
                throw StereotypeDefinitionParserError(QStringLiteral("Property givent twice."), token.sourcePos());
            values.insert(token.subtype(), ShapeValueF(parseFloatProperty(), parameter.m_unit, parameter.m_origin));
        } else {
            throwUnknownPropertyError(token);
        }
        if (!expectPropertySeparatorOrBlockEnd())
            break;
    }
    if (values.count() < parameters.count())
        throw StereotypeDefinitionParserError(QStringLiteral("Missing some properties."), token.sourcePos());
    else if (values.count() > parameters.count())
        throw StereotypeDefinitionParserError(QStringLiteral("Too many properties given."), token.sourcePos());
    return values;
}

void StereotypeDefinitionParser::parseToolbar()
{
    Toolbar toolbar;
    expectBlockBegin();
    Token token;
    while (readProperty(&token)) {
        switch (token.subtype()) {
        case KEYWORD_ID:
            toolbar.setId(parseIdentifierProperty());
            break;
        case KEYWORD_TITLE:
            // TODO implement
            break;
        case KEYWORD_PRIORITY:
            toolbar.setPriority(parseIntProperty());
            break;
        case KEYWORD_TOOLS:
            parseToolbarTools(&toolbar);
            break;
        default:
            throwUnknownPropertyError(token);
        }
        if (!expectPropertySeparatorOrBlockEnd())
            break;
    }
    if (toolbar.id().isEmpty())
        throw StereotypeDefinitionParserError(QStringLiteral("Missing id in Toolbar definition."), d->m_scanner->sourcePos());
    emit toolbarParsed(toolbar);
}

void StereotypeDefinitionParser::parseToolbarTools(Toolbar *toolbar)
{
    QList<Toolbar::Tool> tools;
    expectBlockBegin();
    Token token;
    while (readProperty(&token)) {
        switch (token.subtype()) {
        case KEYWORD_TOOL:
        {
            Toolbar::Tool tool;
            tool.m_toolType = Toolbar::TooltypeTool;
            parseToolbarTool(&tool);
            tools.append(tool);
            break;
        }
        case KEYWORD_SEPARATOR:
            tools.append(Toolbar::Tool());
            skipOptionalEmptyBlock();
            break;
        default:
            throwUnknownPropertyError(token);
        }
        if (!expectPropertySeparatorOrBlockEnd())
            break;
    }
    toolbar->setTools(tools);
}

void StereotypeDefinitionParser::parseToolbarTool(Toolbar::Tool *tool)
{
    expectBlockBegin();
    Token token;
    while (readProperty(&token)) {
        switch (token.subtype()) {
        case KEYWORD_TITLE:
            tool->m_name = parseStringProperty();
            break;
        case KEYWORD_ELEMENT:
        {
            QString element = parseIdentifierProperty();
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
            tool->m_elementType = elementName;
            break;
        }
        case KEYWORD_STEREOTYPE:
            tool->m_stereotype = parseStringProperty();
            break;
        default:
           throwUnknownPropertyError(token);
        }
        if (!expectPropertySeparatorOrBlockEnd())
            break;
    }
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

void StereotypeDefinitionParser::expectBlockBegin()
{
    skipEOLTokens();
    expectOperator(OPERATOR_BRACE_OPEN, QStringLiteral("{"));
}

bool StereotypeDefinitionParser::readProperty(Token *token)
{
    *token = readNextToken();
    if (isOperator(*token, OPERATOR_BRACE_CLOSE))
        return false;
    else if (token->type() == Token::TokenKeyword)
        return true;
    else if (token->type() == Token::TokenIdentifier)
        throwUnknownPropertyError(*token);
    else
        throw StereotypeDefinitionParserError(QStringLiteral("Syntax error."), token->sourcePos());
    return false; // will never be reached but avoids compiler warning
}

void StereotypeDefinitionParser::throwUnknownPropertyError(const Token &token)
{
    throw StereotypeDefinitionParserError(QString(QStringLiteral("Unknown property '%1'.")).arg(token.text()), token.sourcePos());
}

bool StereotypeDefinitionParser::expectPropertySeparatorOrBlockEnd()
{
    bool ok = false;
    Token token = d->m_scanner->read();
    if (token.type() == Token::TokenEndOfLine) {
        skipEOLTokens();
        token = d->m_scanner->read();
        ok = true;
    }
    if (token.type() == Token::TokenOperator && token.subtype() == OPERATOR_SEMICOLON)
        ok = true;
    else if (token.type() == Token::TokenOperator && token.subtype() == OPERATOR_BRACE_CLOSE)
        return false;
    else
        d->m_scanner->unread(token);
    if (!ok)
        throw StereotypeDefinitionParserError(QStringLiteral("Expected ';', '}' or end-of-line."), token.sourcePos());
    return true;
}

void StereotypeDefinitionParser::skipOptionalEmptyBlock()
{
    Token token = d->m_scanner->read();
    if (token.type() == Token::TokenEndOfLine) {
        Token eolToken = token;
        for (;;) {
            token = d->m_scanner->read();
            if (token.type() != Token::TokenEndOfLine)
                break;
            eolToken = token;
        }
        if (isOperator(token, OPERATOR_BRACE_OPEN)) {
            token = readNextToken();
            if (!isOperator(token, OPERATOR_BRACE_CLOSE))
                throw StereotypeDefinitionParserError(QStringLiteral("Expected '}' in empty block."), token.sourcePos());
        } else {
            d->m_scanner->unread(token);
            d->m_scanner->unread(eolToken);
        }
    } else if (isOperator(token, OPERATOR_BRACE_OPEN)) {
        token = readNextToken();
        if (!isOperator(token, OPERATOR_BRACE_CLOSE))
            throw StereotypeDefinitionParserError(QStringLiteral("Expected '}' in empty block."), token.sourcePos());
    } else {
        d->m_scanner->unread(token);
    }
}

qreal StereotypeDefinitionParser::expectAbsoluteValue(const ShapeValueF &value, const SourcePos &sourcePos)
{
    if (value.unit() != ShapeValueF::UnitAbsolute || value.origin() != ShapeValueF::OriginSmart)
        throw StereotypeDefinitionParserError(QStringLiteral("Expected absolute value"), sourcePos);
    return value.value();
}

bool StereotypeDefinitionParser::isOperator(const Token &token, int op) const
{
    return token.type() == Token::TokenOperator && token.subtype() == op;
}

void StereotypeDefinitionParser::expectOperator(int op, const QString &opName)
{
    Token token = d->m_scanner->read();
    if (!isOperator(token, op))
        throw StereotypeDefinitionParserError(QString(QStringLiteral("Expected '%1'.")).arg(opName), token.sourcePos());
}

void StereotypeDefinitionParser::expectColon()
{
    expectOperator(OPERATOR_COLON, QStringLiteral(":"));
}

void StereotypeDefinitionParser::skipEOLTokens()
{
    Token token;
    for (;;) {
        token = d->m_scanner->read();
        if (token.type() != Token::TokenEndOfLine)
            break;
    }
    d->m_scanner->unread(token);
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

} // namespace qmt
