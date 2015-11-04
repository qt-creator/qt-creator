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

#ifndef QMT_STEREOTYPEDEFINITIONPARSER_H
#define QMT_STEREOTYPEDEFINITIONPARSER_H

#include <QObject>
#include "qmt/infrastructure/exceptions.h"

#include "sourcepos.h"


namespace qmt {

class ITextSource;
class Token;
class StereotypeIcon;
class Toolbar;
class ShapeValueF;

class QMT_EXPORT StereotypeDefinitionParserError :
        public Exception
{
public:

    StereotypeDefinitionParserError(const QString &errorMsg, const SourcePos &sourcePos);

    ~StereotypeDefinitionParserError();

public:

    SourcePos getSourcePos() const { return m_sourcePos; }

private:
    SourcePos m_sourcePos;
};


class QMT_EXPORT StereotypeDefinitionParser : public QObject
{
    Q_OBJECT

    struct StereotypeDefinitionParserPrivate;

    struct IconCommandParameter;

public:

    explicit StereotypeDefinitionParser(QObject *parent = 0);

    ~StereotypeDefinitionParser();

signals:

    void iconParsed(const StereotypeIcon &stereotypeIcon);

    void toolbarParsed(const Toolbar &toolbar);

public:

    void parse(ITextSource *source);

private:

    void parseFile();

    void parseIcon();

    void parseIconProperties(StereotypeIcon *stereotypeIcon);

    void parseToolbar();

    void parseToolbarProperties(Toolbar *toolbar);

    QString parseStringProperty();

    int parseIntProperty();

    qreal parseFloatProperty();

    QString parseIdentifierProperty();

    QList<QString> parseIdentifierListProperty();

    bool parseBoolProperty();

    QColor parseColorProperty();

    void parseIconCommands(StereotypeIcon *stereotypeIcon);

    QList<ShapeValueF> parseIconCommandParameters(const QList<IconCommandParameter> &parameters);

    void parseToolbarCommands(Toolbar *toolbar);

    QString parseStringExpression();

    qreal parseFloatExpression();

    int parseIntExpression();

    QString parseIdentifierExpression();

    bool parseBoolExpression();

    QColor parseColorExpression();

    Token readNextToken();

    qreal expectAbsoluteValue(const ShapeValueF &value, const SourcePos &sourcePos);

    void expectSemicolonOrEndOfLine();

    bool nextIsComma();

    void expectOperator(int op, const QString &opName);

    void expectComma();

    void expectColon();

private:

    StereotypeDefinitionParserPrivate *d;
};

}

#endif // QMT_STEREOTYPEDEFINITIONPARSER_H
