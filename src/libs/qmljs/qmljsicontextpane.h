/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef QMLJSICONTEXTPANE_H
#define QMLJSICONTEXTPANE_H

#include <QObject>
#include "qmljs_global.h"
#include <qmljs/parser/qmljsastfwd_p.h>
#include <qmljs/qmljslookupcontext.h>


namespace TextEditor {

class BaseTextEditorEditable;

} //TextEditor

namespace QmlJS {


class QMLJS_EXPORT IContextPane : public QObject
{
     Q_OBJECT

public:
    IContextPane(QObject *parent = 0) : QObject(parent) {}
    virtual ~IContextPane() {}
    virtual void apply(TextEditor::BaseTextEditorEditable *editor, Document::Ptr document, LookupContext::Ptr lookupContext, AST::Node *node, bool update, bool force = false) = 0;
    virtual void setEnabled(bool) = 0;
    virtual bool isAvailable(TextEditor::BaseTextEditorEditable *editor, Document::Ptr document, AST::Node *node) = 0;
    virtual QWidget* widget() = 0;
signals:
    void closed();
};

} // namespace QmlJS

#endif // QMLJSICONTEXTPANE_H
