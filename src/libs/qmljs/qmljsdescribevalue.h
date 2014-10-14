/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://www.qt.io/licensing.  For further information
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
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef QMLJSDESCRIBEVALUE_H
#define QMLJSDESCRIBEVALUE_H

#include "qmljs_global.h"
#include "qmljsinterpreter.h"

#include <QString>
#include <QSet>

namespace QmlJS {

class QMLJS_EXPORT DescribeValueVisitor : public ValueVisitor
{
public:
    static QString describe(const Value *value, int depth = 1, ContextPtr context = ContextPtr());

    DescribeValueVisitor(int detailDepth = 1, int startIndent = 0, int indentIncrement = 2,
                         ContextPtr context = ContextPtr());
    virtual ~DescribeValueVisitor();

    QString operator()(const Value *value);
    void visit(const NullValue *) Q_DECL_OVERRIDE;
    void visit(const UndefinedValue *) Q_DECL_OVERRIDE;
    void visit(const UnknownValue *) Q_DECL_OVERRIDE;
    void visit(const NumberValue *) Q_DECL_OVERRIDE;
    void visit(const BooleanValue *) Q_DECL_OVERRIDE;
    void visit(const StringValue *) Q_DECL_OVERRIDE;
    void visit(const ObjectValue *) Q_DECL_OVERRIDE;
    void visit(const FunctionValue *) Q_DECL_OVERRIDE;
    void visit(const Reference *) Q_DECL_OVERRIDE;
    void visit(const ColorValue *) Q_DECL_OVERRIDE;
    void visit(const AnchorLineValue *) Q_DECL_OVERRIDE;
    QString description() const;
    void basicDump(const char *baseName, const Value *value, bool opensContext);
    void dumpNewline();
    void openContext(const char *openStr = "{");
    void closeContext(const char *closeStr = "}");
    void dump(const char *toAdd);
    void dump(const QString &toAdd);
private:
    int m_depth;
    int m_indent;
    int m_indentIncrement;
    bool m_emptyContext;
    ContextPtr m_context;
    QSet<const Value *> m_visited;
    QString m_description;
};

} // namespace QmlJS
#endif // QMLJSDESCRIBEVALUE_H
