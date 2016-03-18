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

#pragma once

#include <QObject>
#include "qmt/infrastructure/exceptions.h"
#include "qmt/stereotype/toolbar.h"

#include "sourcepos.h"

#include <QPair>
#include <QHash>

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

public:
    explicit StereotypeDefinitionParser(QObject *parent = 0);
    ~StereotypeDefinitionParser() override;

signals:
    void iconParsed(const StereotypeIcon &stereotypeIcon);
    void toolbarParsed(const Toolbar &toolbar);

public:
    void parse(ITextSource *source);

private:
    void parseFile();

    void parseIcon();
    static QPair<int, IconCommandParameter> SCALED(int keyword);
    static QPair<int, IconCommandParameter> FIX(int keyword);
    static QPair<int, IconCommandParameter> ABSOLUTE(int keyword);
    void parseIconShape(StereotypeIcon *stereotypeIcon);
    QHash<int, ShapeValueF> parseIconShapeProperties(const QHash<int, IconCommandParameter> &parameters);

    void parseToolbar();
    void parseToolbarTools(Toolbar *toolbar);
    void parseToolbarTool(Toolbar::Tool *tool);

    QString parseStringProperty();
    int parseIntProperty();
    qreal parseFloatProperty();
    QString parseIdentifierProperty();
    QList<QString> parseIdentifierListProperty();
    bool parseBoolProperty();
    QColor parseColorProperty();

    QString parseStringExpression();
    qreal parseFloatExpression();
    int parseIntExpression();
    QString parseIdentifierExpression();
    bool parseBoolExpression();
    QColor parseColorExpression();

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
