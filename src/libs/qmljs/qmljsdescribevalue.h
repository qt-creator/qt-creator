/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
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
    void visit(const NullValue *) override;
    void visit(const UndefinedValue *) override;
    void visit(const UnknownValue *) override;
    void visit(const NumberValue *) override;
    void visit(const BooleanValue *) override;
    void visit(const StringValue *) override;
    void visit(const ObjectValue *) override;
    void visit(const FunctionValue *) override;
    void visit(const Reference *) override;
    void visit(const ColorValue *) override;
    void visit(const AnchorLineValue *) override;
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
