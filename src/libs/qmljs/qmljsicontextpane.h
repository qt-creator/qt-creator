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

#ifndef QMLJSICONTEXTPANE_H
#define QMLJSICONTEXTPANE_H

#include <QObject>

#include "qmljs_global.h"

#include <qmljs/qmljsdocument.h>
#include <qmljs/parser/qmljsastfwd_p.h>

namespace TextEditor {
class BaseTextEditor;
} //TextEditor

namespace QmlJS {

class ScopeChain;

class QMLJS_EXPORT IContextPane : public QObject
{
     Q_OBJECT

public:
    IContextPane(QObject *parent = 0) : QObject(parent) {}
    virtual ~IContextPane() {}
    virtual void apply(TextEditor::BaseTextEditor *editor, Document::Ptr document, const ScopeChain *scopeChain, AST::Node *node, bool update, bool force = false) = 0;
    virtual void setEnabled(bool) = 0;
    virtual bool isAvailable(TextEditor::BaseTextEditor *editor, Document::Ptr document, AST::Node *node) = 0;
    virtual QWidget* widget() = 0;
signals:
    void closed();
};

} // namespace QmlJS

#endif // QMLJSICONTEXTPANE_H
