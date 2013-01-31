/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef PARSEDQML_H
#define PARSEDQML_H

#include <QString>

#include <qmljsengine_p.h>
#include <qmljslexer_p.h>
#include <qmljsparser_p.h>
#include <qmljsnodepool_p.h>
#include <qmljsast_p.h>

namespace QmlDesigner {
namespace Internal {

class ParsedQML
{
public:
    explicit ParsedQML(const QString &sourceCode, const QString &fileName = QString());

    bool isValid() const { return m_ast != 0; }
    QmlJS::AST::UiProgram *ast() const { return m_ast; }

    QString sourceCode() const { return m_sourceCode; }

private:
    QString m_sourceCode;
    QmlJS::Engine m_engine;
    QmlJS::NodePool m_nodePool;
    QmlJS::AST::UiProgram* m_ast;
};

} // namespace Internal
} // namespace QmlDesigner

#endif // PARSEDQML_H
