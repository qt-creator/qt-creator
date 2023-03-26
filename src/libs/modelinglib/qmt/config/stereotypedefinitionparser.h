// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QObject>
#include "qmt/infrastructure/exceptions.h"
#include "qmt/stereotype/customrelation.h"
#include "qmt/stereotype/toolbar.h"

#include "sourcepos.h"

#include <QPair>
#include <QHash>
#include <functional>

namespace qmt {

class ITextSource;
class Token;
class StereotypeIcon;
class Toolbar;
class ShapeValueF;

class QMT_EXPORT StereotypeDefinitionParserError : public Exception
{
public:
    StereotypeDefinitionParserError(const QString &errorMsg, const SourcePos &sourcePos);
    ~StereotypeDefinitionParserError() override;

    SourcePos sourcePos() const { return m_sourcePos; }

private:
    SourcePos m_sourcePos;
};

class QMT_EXPORT StereotypeDefinitionParser : public QObject
{
    Q_OBJECT
    class StereotypeDefinitionParserPrivate;
    class IconCommandParameter;
    class Value;

    enum Type {
        Void,
        Identifier,
        String,
        Int,
        Float,
        Boolean,
        Color
    };

public:
    explicit StereotypeDefinitionParser(QObject *parent = nullptr);
    ~StereotypeDefinitionParser() override;

signals:
    void iconParsed(const StereotypeIcon &stereotypeIcon);
    void relationParsed(const CustomRelation &relation);
    void toolbarParsed(const Toolbar &toolbar);

public:
    void parse(ITextSource *source);

private:
    void parseFile();

    void parseIcon();
    static QPair<int, IconCommandParameter> SCALED(int keyword);
    static QPair<int, IconCommandParameter> FIX(int keyword);
    static QPair<int, IconCommandParameter> ABSOLUTE(int keyword);
    static QPair<int, IconCommandParameter> BOOLEAN(int keyword);
    IconShape parseIconShape();
    QHash<int, IconCommandParameter> parseIconShapeProperties(const QHash<int, IconCommandParameter> &parameters);

    void parseRelation(CustomRelation::Element element);
    void parseRelationEnd(CustomRelation *relation);

    void parseToolbar();
    void parseToolbarTools(Toolbar *toolbar);
    void parseToolbarTool(const Toolbar *toolbar, Toolbar::Tool *tool);

    template<typename T>
    void parseEnums(const QList<QString> &identifiers, const QHash<QString, T> &identifierNames,
                    const SourcePos &sourcePos, std::function<void(T)> setter);

    template<typename T>
    void parseEnum(const QString &identifier, const QHash<QString, T> &identifierNames,
                   const SourcePos &sourcePos, std::function<void(T)> setter);

    QString parseStringProperty();
    int parseIntProperty();
    qreal parseFloatProperty();
    QString parseIdentifierProperty();
    QList<QString> parseIdentifierListProperty();
    bool parseBoolProperty();
    QColor parseColorProperty();
    Value parseProperty();

    QString parseStringExpression();
    qreal parseFloatExpression();
    int parseIntExpression();
    QString parseIdentifierExpression();
    bool parseBoolExpression();
    QColor parseColorExpression();
    Value parseExpression();

    void expectBlockBegin();
    bool readProperty(Token *token);
    void throwUnknownPropertyError(const Token &token);
    bool expectPropertySeparatorOrBlockEnd();
    void skipOptionalEmptyBlock();

    qreal expectAbsoluteValue(const ShapeValueF &value, const SourcePos &sourcePos);
    bool isOperator(const Token &token, int op) const;
    void expectOperator(int op, const QString &opName);
    void expectColon();

    void skipEOLTokens();
    Token readNextToken();

    StereotypeDefinitionParserPrivate *d;
};

} // namespace qmt
