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
#include "qmt/stereotype/customrelation.h"
#include "qmt/stereotype/toolbar.h"

#include <QHash>
#include <QSet>
#include <QPair>
#include <QVariant>

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

// Relation Definition
static const int KEYWORD_RELATION      = 100;
static const int KEYWORD_DEPENDENCY    = 101;
static const int KEYWORD_INHERITANCE   = 102;
static const int KEYWORD_ASSOCIATION   = 103;
static const int KEYWORD_NAME          = 104;
static const int KEYWORD_DIRECTION     = 105;
static const int KEYWORD_ATOB          = 106;
static const int KEYWORD_BTOA          = 107;
static const int KEYWORD_BI            = 108;
static const int KEYWORD_END           = 109;
static const int KEYWORD_A             = 110;
static const int KEYWORD_B             = 111;
static const int KEYWORD_ROLE          = 112;
static const int KEYWORD_CARDINALITY   = 113;
static const int KEYWORD_NAVIGABLE     = 114;
static const int KEYWORD_RELATIONSHIP  = 115;
static const int KEYWORD_AGGREGATION   = 116;
static const int KEYWORD_COMPOSITION   = 117;
static const int KEYWORD_SHAFT         = 118;
static const int KEYWORD_HEAD          = 119;

// Relation Shapes
static const int KEYWORD_DIAMOND       = 130;
static const int KEYWORD_TRIANGLE      = 131;
static const int KEYWORD_FILLED        = 132;
static const int KEYWORD_PATTERN       = 133;
static const int KEYWORD_SOLID         = 134;
static const int KEYWORD_DOT           = 135;
static const int KEYWORD_DASH          = 136;
static const int KEYWORD_DASHDOT       = 137;
static const int KEYWORD_DASHDOTDOT    = 138;
static const int KEYWORD_COLOR         = 139;

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
    TextScanner *m_scanner = nullptr;

};

class StereotypeDefinitionParser::IconCommandParameter
{
public:
    enum Type {
        ShapeValue,
        Boolean
    };

    IconCommandParameter() = default;

    IconCommandParameter(int keyword, ShapeValueF::Unit unit, ShapeValueF::Origin origin = ShapeValueF::OriginSmart)
        : m_keyword(keyword),
          m_type(ShapeValue),
          m_unit(unit),
          m_origin(origin)
    {
    }

    IconCommandParameter(int keyword, Type type)
        : m_keyword(keyword),
          m_type(type)
    {
    }

    operator ShapeValueF() const { return m_shapeValue; }

    Type type() const { return m_type; }
    ShapeValueF::Unit unit() const { return m_unit; }
    ShapeValueF::Origin origin() const { return m_origin; }
    ShapeValueF shapeValue() const { return m_shapeValue; }
    void setShapeValue(const ShapeValueF &shapeValue) { m_shapeValue = shapeValue; }
    bool boolean() const { return m_boolean; }
    void setBoolean(bool boolean) { m_boolean = boolean; }

private:
    int m_keyword = -1;
    Type m_type = ShapeValue;
    ShapeValueF::Unit m_unit = ShapeValueF::UnitAbsolute;
    ShapeValueF::Origin m_origin = ShapeValueF::OriginSmart;
    ShapeValueF m_shapeValue;
    bool m_boolean = false;
};

class StereotypeDefinitionParser::Value
{
public:
    Value(StereotypeDefinitionParser::Type type, QVariant value)
        : m_type(type),
          m_value(value)
    {
    }

    StereotypeDefinitionParser::Type type() const { return m_type; }
    QVariant value() const { return m_value; }

private:
    StereotypeDefinitionParser::Type m_type = StereotypeDefinitionParser::Void;
    QVariant m_value;
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
                << qMakePair(QString("icon"), KEYWORD_ICON)
                << qMakePair(QString("id"), KEYWORD_ID)
                << qMakePair(QString("title"), KEYWORD_TITLE)
                << qMakePair(QString("elements"), KEYWORD_ELEMENTS)
                << qMakePair(QString("stereotype"), KEYWORD_STEREOTYPE)
                << qMakePair(QString("width"), KEYWORD_WIDTH)
                << qMakePair(QString("height"), KEYWORD_HEIGHT)
                << qMakePair(QString("minwidth"), KEYWORD_MINWIDTH)
                << qMakePair(QString("minheight"), KEYWORD_MINHEIGHT)
                << qMakePair(QString("locksize"), KEYWORD_LOCK_SIZE)
                << qMakePair(QString("display"), KEYWORD_DISPLAY)
                << qMakePair(QString("textalignment"), KEYWORD_TEXTALIGN)
                << qMakePair(QString("basecolor"), KEYWORD_BASECOLOR)
                << qMakePair(QString("shape"), KEYWORD_SHAPE)
                << qMakePair(QString("circle"), KEYWORD_CIRCLE)
                << qMakePair(QString("ellipse"), KEYWORD_ELLIPSE)
                << qMakePair(QString("line"), KEYWORD_LINE)
                << qMakePair(QString("rect"), KEYWORD_RECT)
                << qMakePair(QString("roundedrect"), KEYWORD_ROUNDEDRECT)
                << qMakePair(QString("arc"), KEYWORD_ARC)
                << qMakePair(QString("moveto"), KEYWORD_MOVETO)
                << qMakePair(QString("lineto"), KEYWORD_LINETO)
                << qMakePair(QString("arcmoveto"), KEYWORD_ARCMOVETO)
                << qMakePair(QString("arcto"), KEYWORD_ARCTO)
                << qMakePair(QString("close"), KEYWORD_CLOSE)
                << qMakePair(QString("x"), KEYWORD_X)
                << qMakePair(QString("y"), KEYWORD_Y)
                << qMakePair(QString("x0"), KEYWORD_X0)
                << qMakePair(QString("y0"), KEYWORD_Y0)
                << qMakePair(QString("x1"), KEYWORD_X1)
                << qMakePair(QString("y1"), KEYWORD_Y1)
                << qMakePair(QString("radius"), KEYWORD_RADIUS)
                << qMakePair(QString("radiusx"), KEYWORD_RADIUS_X)
                << qMakePair(QString("radiusy"), KEYWORD_RADIUS_Y)
                << qMakePair(QString("start"), KEYWORD_START)
                << qMakePair(QString("span"), KEYWORD_SPAN)
                << qMakePair(QString("toolbar"), KEYWORD_TOOLBAR)
                << qMakePair(QString("priority"), KEYWORD_PRIORITY)
                << qMakePair(QString("tools"), KEYWORD_TOOLS)
                << qMakePair(QString("tool"), KEYWORD_TOOL)
                << qMakePair(QString("element"), KEYWORD_ELEMENT)
                << qMakePair(QString("separator"), KEYWORD_SEPARATOR)
                << qMakePair(QString("relation"), KEYWORD_RELATION)
                << qMakePair(QString("dependency"), KEYWORD_DEPENDENCY)
                << qMakePair(QString("inheritance"), KEYWORD_INHERITANCE)
                << qMakePair(QString("association"), KEYWORD_ASSOCIATION)
                << qMakePair(QString("name"), KEYWORD_NAME)
                << qMakePair(QString("direction"), KEYWORD_DIRECTION)
                << qMakePair(QString("atob"), KEYWORD_ATOB)
                << qMakePair(QString("btoa"), KEYWORD_BTOA)
                << qMakePair(QString("bi"), KEYWORD_BI)
                << qMakePair(QString("end"), KEYWORD_END)
                << qMakePair(QString("a"), KEYWORD_A)
                << qMakePair(QString("b"), KEYWORD_B)
                << qMakePair(QString("role"), KEYWORD_ROLE)
                << qMakePair(QString("cardinality"), KEYWORD_CARDINALITY)
                << qMakePair(QString("navigable"), KEYWORD_NAVIGABLE)
                << qMakePair(QString("relationship"), KEYWORD_RELATIONSHIP)
                << qMakePair(QString("aggregation"), KEYWORD_AGGREGATION)
                << qMakePair(QString("composition"), KEYWORD_COMPOSITION)
                << qMakePair(QString("shaft"), KEYWORD_SHAFT)
                << qMakePair(QString("head"), KEYWORD_HEAD)
                << qMakePair(QString("diamond"), KEYWORD_DIAMOND)
                << qMakePair(QString("triangle"), KEYWORD_TRIANGLE)
                << qMakePair(QString("filled"), KEYWORD_FILLED)
                << qMakePair(QString("pattern"), KEYWORD_PATTERN)
                << qMakePair(QString("solid"), KEYWORD_SOLID)
                << qMakePair(QString("dot"), KEYWORD_DOT)
                << qMakePair(QString("dash"), KEYWORD_DASH)
                << qMakePair(QString("dashdot"), KEYWORD_DASHDOT)
                << qMakePair(QString("dashdotdot"), KEYWORD_DASHDOTDOT)
                << qMakePair(QString("color"), KEYWORD_COLOR)
                );
    textScanner.setOperators(
                QList<QPair<QString, int> >()
                << qMakePair(QString(";"), OPERATOR_SEMICOLON)
                << qMakePair(QString("{"), OPERATOR_BRACE_OPEN)
                << qMakePair(QString("}"), OPERATOR_BRACE_CLOSE)
                << qMakePair(QString(":"), OPERATOR_COLON)
                << qMakePair(QString(","), OPERATOR_COMMA)
                << qMakePair(QString("."), OPERATOR_PERIOD)
                << qMakePair(QString("-"), OPERATOR_MINUS)
                );
    textScanner.setSource(source);

    d->m_scanner = &textScanner;
    try {
        parseFile();
    } catch (...) {
        d->m_scanner = nullptr;
        throw;
    }
    d->m_scanner = nullptr;
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
        else if (token.type() == Token::TokenKeyword && token.subtype() == KEYWORD_RELATION)
            parseRelation(CustomRelation::Element::Relation);
        else if (token.type() == Token::TokenKeyword && token.subtype() == KEYWORD_DEPENDENCY)
            parseRelation(CustomRelation::Element::Dependency);
        else if (token.type() == Token::TokenKeyword && token.subtype() == KEYWORD_INHERITANCE)
            parseRelation(CustomRelation::Element::Inheritance);
        else if (token.type() == Token::TokenKeyword && token.subtype() == KEYWORD_ASSOCIATION)
            parseRelation(CustomRelation::Element::Association);
        else
            throw StereotypeDefinitionParserError("Expected 'Icon', 'Toolbar', 'Relation', 'Dependency', 'Inheritance' or 'Association'.", token.sourcePos());
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
            const static QHash<QString, StereotypeIcon::Element> elementNames = QHash<QString, StereotypeIcon::Element>()
                    << qMakePair(QString("package"), StereotypeIcon::ElementPackage)
                    << qMakePair(QString("component"), StereotypeIcon::ElementComponent)
                    << qMakePair(QString("class"), StereotypeIcon::ElementClass)
                    << qMakePair(QString("diagram"), StereotypeIcon::ElementDiagram)
                    << qMakePair(QString("item"), StereotypeIcon::ElementItem);
            parseEnums<StereotypeIcon::Element>(
                        parseIdentifierListProperty(), elementNames, token.sourcePos(),
                        [&](StereotypeIcon::Element element) { elements.insert(element); });
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
            const static QHash<QString, StereotypeIcon::SizeLock> lockNames = QHash<QString, StereotypeIcon::SizeLock>()
                    << qMakePair(QString("none"), StereotypeIcon::LockNone)
                    << qMakePair(QString("width"), StereotypeIcon::LockWidth)
                    << qMakePair(QString("height"), StereotypeIcon::LockHeight)
                    << qMakePair(QString("size"), StereotypeIcon::LockSize)
                    << qMakePair(QString("ratio"), StereotypeIcon::LockRatio);
            parseEnum<StereotypeIcon::SizeLock>(
                        parseIdentifierProperty(), lockNames, token.sourcePos(),
                        [&](StereotypeIcon::SizeLock lock) { stereotypeIcon.setSizeLock(lock); });
            break;
        }
        case KEYWORD_DISPLAY:
        {
            const static QHash<QString, StereotypeIcon::Display> displayNames = QHash<QString, StereotypeIcon::Display>()
                    << qMakePair(QString("none"), StereotypeIcon::DisplayNone)
                    << qMakePair(QString("label"), StereotypeIcon::DisplayLabel)
                    << qMakePair(QString("decoration"), StereotypeIcon::DisplayDecoration)
                    << qMakePair(QString("icon"), StereotypeIcon::DisplayIcon)
                    << qMakePair(QString("smart"), StereotypeIcon::DisplaySmart);
            parseEnum<StereotypeIcon::Display>(
                        parseIdentifierProperty(), displayNames, token.sourcePos(),
                        [&](StereotypeIcon::Display display) { stereotypeIcon.setDisplay(display); });
            break;
        }
        case KEYWORD_TEXTALIGN:
        {
            const static QHash<QString, StereotypeIcon::TextAlignment> alignNames = QHash<QString, StereotypeIcon::TextAlignment>()
                    << qMakePair(QString("below"), StereotypeIcon::TextalignBelow)
                    << qMakePair(QString("center"), StereotypeIcon::TextalignCenter)
                    << qMakePair(QString("none"), StereotypeIcon::TextalignNone);
            parseEnum<StereotypeIcon::TextAlignment>(
                        parseIdentifierProperty(), alignNames, token.sourcePos(),
                        [&](StereotypeIcon::TextAlignment align) { stereotypeIcon.setTextAlignment(align); });
            break;
        }
        case KEYWORD_BASECOLOR:
            stereotypeIcon.setBaseColor(parseColorProperty());
            break;
        case KEYWORD_SHAPE:
            stereotypeIcon.setIconShape(parseIconShape());
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
        throw StereotypeDefinitionParserError("Missing id in Icon definition.", d->m_scanner->sourcePos());
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

QPair<int, StereotypeDefinitionParser::IconCommandParameter> StereotypeDefinitionParser::BOOLEAN(int keyword)
{
    return qMakePair(keyword, IconCommandParameter(keyword, IconCommandParameter::Boolean));
}

IconShape StereotypeDefinitionParser::parseIconShape()
{
    IconShape iconShape;
    QHash<int, IconCommandParameter> values;
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
        case KEYWORD_DIAMOND:
            values = parseIconShapeProperties(
                        Parameters() << SCALED(KEYWORD_X) << SCALED(KEYWORD_Y)
                        << SCALED(KEYWORD_WIDTH) << SCALED(KEYWORD_HEIGHT)
                        << BOOLEAN(KEYWORD_FILLED));
            iconShape.addDiamond(ShapePointF(values.value(KEYWORD_X), values.value(KEYWORD_Y)),
                                ShapeSizeF(values.value(KEYWORD_WIDTH), values.value(KEYWORD_HEIGHT)),
                                values.value(KEYWORD_FILLED).boolean());
            break;
        case KEYWORD_TRIANGLE:
            values = parseIconShapeProperties(
                        Parameters() << SCALED(KEYWORD_X) << SCALED(KEYWORD_Y)
                        << SCALED(KEYWORD_WIDTH) << SCALED(KEYWORD_HEIGHT)
                        << BOOLEAN(KEYWORD_FILLED));
            iconShape.addTriangle(ShapePointF(values.value(KEYWORD_X), values.value(KEYWORD_Y)),
                                  ShapeSizeF(values.value(KEYWORD_WIDTH), values.value(KEYWORD_HEIGHT)),
                                  values.value(KEYWORD_FILLED).boolean());
            break;
        default:
            throwUnknownPropertyError(token);
        }
        if (!expectPropertySeparatorOrBlockEnd())
            break;
    }
    return iconShape;
}

QHash<int, StereotypeDefinitionParser::IconCommandParameter> StereotypeDefinitionParser::parseIconShapeProperties(const QHash<int, IconCommandParameter> &parameters)
{
    expectBlockBegin();
    QHash<int, IconCommandParameter> values;
    Token token;
    while (readProperty(&token)) {
        if (parameters.contains(token.subtype())) {
            if (values.contains(token.subtype()))
                throw StereotypeDefinitionParserError("Property given twice.", token.sourcePos());
            IconCommandParameter parameter = parameters.value(token.subtype());
            if (parameter.type() == IconCommandParameter::ShapeValue)
                parameter.setShapeValue(ShapeValueF(parseFloatProperty(), parameter.unit(), parameter.origin()));
            else if (parameter.type() == IconCommandParameter::Boolean)
                parameter.setBoolean(parseBoolProperty());
            else
                throw StereotypeDefinitionParserError("Unexpected type of property.", token.sourcePos());
            values.insert(token.subtype(), parameter);
        } else {
            throwUnknownPropertyError(token);
        }
        if (!expectPropertySeparatorOrBlockEnd())
            break;
    }
    if (values.count() < parameters.count())
        throw StereotypeDefinitionParserError("Missing some properties.", token.sourcePos());
    else if (values.count() > parameters.count())
        throw StereotypeDefinitionParserError("Too many properties given.", token.sourcePos());
    return values;
}

void StereotypeDefinitionParser::parseRelation(CustomRelation::Element element)
{
    CustomRelation relation;
    relation.setElement(element);
    QSet<QString> stereotypes;
    expectBlockBegin();
    Token token;
    while (readProperty(&token)) {
        switch (token.subtype()) {
        case KEYWORD_ID:
            relation.setId(parseIdentifierProperty());
            break;
        case KEYWORD_TITLE:
            relation.setTitle(parseStringProperty());
            break;
        case KEYWORD_ELEMENTS:
            relation.setEndItems(parseIdentifierListProperty());
            break;
        case KEYWORD_STEREOTYPE:
            stereotypes.insert(parseStringProperty());
            break;
        case KEYWORD_NAME:
            relation.setName(parseStringProperty());
            break;
        case KEYWORD_DIRECTION:
        {
            const static QHash<QString, CustomRelation::Direction> directionNames = QHash<QString, CustomRelation::Direction>()
                    << qMakePair(QString("atob"), CustomRelation::Direction::AtoB)
                    << qMakePair(QString("btoa"), CustomRelation::Direction::BToA)
                    << qMakePair(QString("bi"), CustomRelation::Direction::Bi);
            if (element != CustomRelation::Element::Dependency)
                throwUnknownPropertyError(token);
            parseEnum<CustomRelation::Direction>(
                        parseIdentifierProperty(), directionNames, token.sourcePos(),
                        [&](CustomRelation::Direction direction) { relation.setDirection(direction); });
            break;
        }
        case KEYWORD_PATTERN:
        {
            const static QHash<QString, CustomRelation::ShaftPattern> patternNames = QHash<QString, CustomRelation::ShaftPattern>()
                    << qMakePair(QString("solid"), CustomRelation::ShaftPattern::Solid)
                    << qMakePair(QString("dash"), CustomRelation::ShaftPattern::Dash)
                    << qMakePair(QString("dot"), CustomRelation::ShaftPattern::Dot)
                    << qMakePair(QString("dashdot"), CustomRelation::ShaftPattern::DashDot)
                    << qMakePair(QString("dashdotdot"), CustomRelation::ShaftPattern::DashDotDot);
            if (element != CustomRelation::Element::Relation)
                throwUnknownPropertyError(token);
            parseEnum<CustomRelation::ShaftPattern>(
                        parseIdentifierProperty(), patternNames, token.sourcePos(),
                        [&](CustomRelation::ShaftPattern pattern) { relation.setShaftPattern(pattern); });
            break;
        }
        case KEYWORD_COLOR:
        {
            if (element != CustomRelation::Element::Relation)
                throwUnknownPropertyError(token);
            Value expression = parseProperty();
            if (expression.type() == Color) {
                relation.setColorType(CustomRelation::ColorType::Custom);
                relation.setColor(expression.value().value<QColor>());
            } else if (expression.type() == Identifier) {
                QString colorValue = expression.value().toString();
                QString colorName = colorValue.toLower();
                if (colorName == "a") {
                    relation.setColorType(CustomRelation::ColorType::EndA);
                } else if (colorName == "b") {
                    relation.setColorType(CustomRelation::ColorType::EndB);
                } else if (QColor::isValidColor(colorName)) {
                    relation.setColorType(CustomRelation::ColorType::Custom);
                    relation.setColor(QColor(colorName));
                } else {
                    throw StereotypeDefinitionParserError(QString("Unexpected value \"%1\" for color.").arg(colorValue), token.sourcePos());
                }
            } else {
                throw StereotypeDefinitionParserError("Unexpected value for color.", token.sourcePos());
            }
            break;
        }
        case KEYWORD_END:
            parseRelationEnd(&relation);
            break;
        default:
            throwUnknownPropertyError(token);
        }
        if (!expectPropertySeparatorOrBlockEnd())
            break;
    }
    relation.setStereotypes(stereotypes);
    if (relation.id().isEmpty())
        throw StereotypeDefinitionParserError("Missing id in Relation definition.", d->m_scanner->sourcePos());
    emit relationParsed(relation);
}

void StereotypeDefinitionParser::parseRelationEnd(CustomRelation *relation)
{
    CustomRelation::End relationEnd;
    bool isEndB = false;
    expectBlockBegin();
    Token token;
    while (readProperty(&token)) {
        switch (token.subtype()) {
        case KEYWORD_END:
        {
            QString endValue = parseIdentifierProperty();
            QString endName = endValue.toLower();
            if (endName == "a")
                isEndB = false;
            else if (endName == "b")
                isEndB = true;
            else
                throw StereotypeDefinitionParserError(QString("Unexpected value \"%1\" for end.").arg(endValue), token.sourcePos());
            break;
        }
        case KEYWORD_ELEMENTS:
            if (relation->element() != CustomRelation::Element::Relation)
                throwUnknownPropertyError(token);
            relationEnd.setEndItems(parseIdentifierListProperty());
            break;
        case KEYWORD_ROLE:
            if (relation->element() != CustomRelation::Element::Relation && relation->element() != CustomRelation::Element::Association)
                throwUnknownPropertyError(token);
            relationEnd.setRole(parseStringProperty());
            break;
        case KEYWORD_CARDINALITY:
        {
            if (relation->element() != CustomRelation::Element::Relation && relation->element() != CustomRelation::Element::Association)
                throwUnknownPropertyError(token);
            Value expression = parseProperty();
            if (expression.type() == Int || expression.type() == String)
                relationEnd.setCardinality(expression.value().toString());
            else
                throw StereotypeDefinitionParserError("Wrong type for cardinality.", token.sourcePos());
            break;
        }
        case KEYWORD_NAVIGABLE:
            if (relation->element() != CustomRelation::Element::Relation && relation->element() != CustomRelation::Element::Association)
                throwUnknownPropertyError(token);
            relationEnd.setNavigable(parseBoolProperty());
            break;
        case KEYWORD_RELATIONSHIP:
        {
            if (relation->element() != CustomRelation::Element::Association)
                throwUnknownPropertyError(token);
            const static QHash<QString, CustomRelation::Relationship> relationshipNames = QHash<QString, CustomRelation::Relationship>()
                    << qMakePair(QString("association"), CustomRelation::Relationship::Association)
                    << qMakePair(QString("aggregation"), CustomRelation::Relationship::Aggregation)
                    << qMakePair(QString("composition"), CustomRelation::Relationship::Composition);
            parseEnum<CustomRelation::Relationship>(
                        parseIdentifierProperty(), relationshipNames, token.sourcePos(),
                        [&](CustomRelation::Relationship relationship) { relationEnd.setRelationship(relationship); });
            break;
        }
        case KEYWORD_HEAD:
        {
            if (relation->element() != CustomRelation::Element::Relation)
                throwUnknownPropertyError(token);
            const static QHash<QString, CustomRelation::Head> headNames = QHash<QString, CustomRelation::Head>()
                    << qMakePair(QString("none"), CustomRelation::Head::None)
                    << qMakePair(QString("arrow"), CustomRelation::Head::Arrow)
                    << qMakePair(QString("triangle"), CustomRelation::Head::Triangle)
                    << qMakePair(QString("filledtriangle"), CustomRelation::Head::FilledTriangle)
                    << qMakePair(QString("diamond"), CustomRelation::Head::Diamond)
                    << qMakePair(QString("filleddiamond"), CustomRelation::Head::FilledDiamond);
            parseEnum<CustomRelation::Head>(
                        parseIdentifierProperty(), headNames, token.sourcePos(),
                        [&](CustomRelation::Head head) { relationEnd.setHead(head); });
            break;
        }
        case KEYWORD_SHAPE:
            if (relation->element() != CustomRelation::Element::Relation)
                throwUnknownPropertyError(token);
            relationEnd.setHead(CustomRelation::Head::Shape);
            relationEnd.setShape(parseIconShape());
            break;
        default:
            throwUnknownPropertyError(token);
        }
        if (!expectPropertySeparatorOrBlockEnd())
            break;
    }
    if (isEndB)
        relation->setEndB(relationEnd);
    else
        relation->setEndA(relationEnd);
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
        case KEYWORD_ELEMENT:
            toolbar.setElementTypes(parseIdentifierListProperty());
            toolbar.setToolbarType(toolbar.elementTypes().isEmpty() ? Toolbar::ObjectToolbar : Toolbar::RelationToolbar);
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
        throw StereotypeDefinitionParserError("Missing id in Toolbar definition.", d->m_scanner->sourcePos());
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
            parseToolbarTool(toolbar, &tool);
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

void StereotypeDefinitionParser::parseToolbarTool(const Toolbar *toolbar, Toolbar::Tool *tool)
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
            if (toolbar->toolbarType() == Toolbar::ObjectToolbar) {
                static QSet<QString> elementNames = QSet<QString>()
                        << "package"
                        << "component"
                        << "class"
                        << "item"
                        << "annotation"
                        << "boundary"
                        << "swimlane";
                QString elementName = element.toLower();
                if (!elementNames.contains(elementName))
                    throw StereotypeDefinitionParserError(QString("Unexpected value \"%1\" for element.").arg(element), token.sourcePos());
                tool->m_elementType = elementName;
            } else {
                static QSet<QString> relationNames = QSet<QString>()
                        << "dependency"
                        << "inheritance"
                        << "association";
                QString relationName = element.toLower();
                if (relationNames.contains(relationName))
                    tool->m_elementType = relationName;
                else
                    tool->m_elementType = element;
            }
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

template<typename T>
void StereotypeDefinitionParser::parseEnums(const QList<QString> &identifiers,
                                            const QHash<QString, T> &identifierNames,
                                            const SourcePos &sourcePos,
                                            std::function<void (T)> setter)
{
    for (const QString &identifier : identifiers)
        parseEnum(identifier, identifierNames, sourcePos, setter);
}

template<typename T>
void StereotypeDefinitionParser::parseEnum(const QString &identifier,
                                           const QHash<QString, T> &identifierNames,
                                           const SourcePos &sourcePos,
                                           std::function<void (T)> setter)
{
    const QString name = identifier.toLower();
    if (!identifierNames.contains(name))
        throw StereotypeDefinitionParserError(QString("Unexpected value \"%1\".").arg(identifier), sourcePos);
    setter(identifierNames.value(name));
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
            throw StereotypeDefinitionParserError("Expected identifier.", token.sourcePos());
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

StereotypeDefinitionParser::Value StereotypeDefinitionParser::parseProperty()
{
    expectColon();
    return parseExpression();
}

QString StereotypeDefinitionParser::parseStringExpression()
{
    Token token = d->m_scanner->read();
    if (token.type() != Token::TokenString)
        throw StereotypeDefinitionParserError("Expected string constant.", token.sourcePos());
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
            throw StereotypeDefinitionParserError("Expected number constant.", token.sourcePos());
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
            throw StereotypeDefinitionParserError("Expected integer constant.", token.sourcePos());
        }
    }
}

QString StereotypeDefinitionParser::parseIdentifierExpression()
{
    Token token = d->m_scanner->read();
    if (token.type() != Token::TokenIdentifier && token.type() != Token::TokenKeyword)
        throw StereotypeDefinitionParserError("Expected identifier.", token.sourcePos());
    return token.text();
}

bool StereotypeDefinitionParser::parseBoolExpression()
{
    Token token = d->m_scanner->read();
    if (token.type() == Token::TokenIdentifier) {
        QString value = token.text().toLower();
        if (value == "yes" || value == "true")
            return true;
        else if (value == "no" || value == "false")
            return false;
    }
    throw StereotypeDefinitionParserError("Expected 'yes', 'no', 'true' or 'false'.", token.sourcePos());
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
    throw StereotypeDefinitionParserError("Expected color name.", token.sourcePos());
}

StereotypeDefinitionParser::Value StereotypeDefinitionParser::parseExpression()
{
    Token token = d->m_scanner->read();
    if (token.type() == Token::TokenString) {
        return Value(String, QVariant(token.text()));
    } else if (token.type() == Token::TokenOperator && token.subtype() == OPERATOR_MINUS) {
        Value v = parseExpression();
        if (v.type() == Int)
            return Value(Int, QVariant(-v.value().toInt()));
        else if (v.type() == Float)
            return Value(Float, QVariant(-v.value().toDouble()));
        else
            throw StereotypeDefinitionParserError("Illegal number expression.", token.sourcePos());
    } else if (token.type() == Token::TokenInteger) {
        bool ok = false;
        int value = token.text().toInt(&ok);
        QMT_CHECK(ok);
        return Value(Int, QVariant(value));
    } else if (token.type() == Token::TokenFloat) {
        bool ok = false;
        qreal value = token.text().toDouble(&ok);
        QMT_CHECK(ok);
        return Value(Float, QVariant(value));
    } else if (token.type() == Token::TokenColor) {
        QString value = token.text().toLower();
        QColor color;
        if (QColor::isValidColor(value)) {
            color.setNamedColor(value);
            return Value(Color, QVariant(color));
        } else {
            throw StereotypeDefinitionParserError("Invalid color.", token.sourcePos());
        }
    } else if (token.type() == Token::TokenIdentifier || token.type() == Token::TokenKeyword) {
        QString value = token.text().toLower();
        if (value == "yes" || value == "true")
            return Value(Boolean, QVariant(true));
        else if (value == "no" || value == "false")
            return Value(Boolean, QVariant(false));
        else
            return Value(Identifier, QVariant(token.text()));
    }
    throw StereotypeDefinitionParserError("Syntax error in expression.", token.sourcePos());
}

void StereotypeDefinitionParser::expectBlockBegin()
{
    skipEOLTokens();
    expectOperator(OPERATOR_BRACE_OPEN, "{");
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
        throw StereotypeDefinitionParserError("Syntax error.", token->sourcePos());
    return false; // will never be reached but avoids compiler warning
}

void StereotypeDefinitionParser::throwUnknownPropertyError(const Token &token)
{
    throw StereotypeDefinitionParserError(QString("Unknown property '%1'.").arg(token.text()), token.sourcePos());
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
        throw StereotypeDefinitionParserError("Expected ';', '}' or end-of-line.", token.sourcePos());
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
                throw StereotypeDefinitionParserError("Expected '}' in empty block.", token.sourcePos());
        } else {
            d->m_scanner->unread(token);
            d->m_scanner->unread(eolToken);
        }
    } else if (isOperator(token, OPERATOR_BRACE_OPEN)) {
        token = readNextToken();
        if (!isOperator(token, OPERATOR_BRACE_CLOSE))
            throw StereotypeDefinitionParserError("Expected '}' in empty block.", token.sourcePos());
    } else {
        d->m_scanner->unread(token);
    }
}

qreal StereotypeDefinitionParser::expectAbsoluteValue(const ShapeValueF &value, const SourcePos &sourcePos)
{
    if (value.unit() != ShapeValueF::UnitAbsolute || value.origin() != ShapeValueF::OriginSmart)
        throw StereotypeDefinitionParserError("Expected absolute value", sourcePos);
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
        throw StereotypeDefinitionParserError(QString("Expected '%1'.").arg(opName), token.sourcePos());
}

void StereotypeDefinitionParser::expectColon()
{
    expectOperator(OPERATOR_COLON, ":");
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
