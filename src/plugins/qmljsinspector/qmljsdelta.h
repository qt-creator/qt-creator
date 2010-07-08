/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
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
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#ifndef QMLJSDELTA_H
#define QMLJSDELTA_H

#include "qmljsprivateapi.h"

#include <qmljs/qmljsdocument.h>
#include <qmljs/parser/qmljsastvisitor_p.h>
#include <qmljs/parser/qmljsastfwd_p.h>
#include <qmljs/parser/qmljsast_p.h>

using namespace QmlJS;
using namespace QmlJS::AST;

namespace QmlJSInspector {
namespace Internal {

class ScriptBindingParser : protected Visitor
{
public:
    QmlJS::Document::Ptr doc;
    QList<UiScriptBinding *> scripts;

    ScriptBindingParser(Document::Ptr doc,
                        const QList<QDeclarativeDebugObjectReference> &objectReferences = QList<QDeclarativeDebugObjectReference>());
    void process();

    UiObjectMember *parent(UiScriptBinding *script) const;
    UiScriptBinding *id(UiObjectMember *parent) const;
    QList<UiScriptBinding *> ids() const;

    QDeclarativeDebugObjectReference objectReferenceForPosition(const QUrl &url, int line, int col) const;

    QDeclarativeDebugObjectReference objectReferenceForScriptBinding(UiScriptBinding *binding) const;

    QDeclarativeDebugObjectReference objectReferenceForOffset(unsigned int offset);

    QString header(UiObjectMember *member) const;

    QString scriptCode(UiScriptBinding *script) const;
    QString methodName(UiSourceElement *source) const;
    QString methodCode(UiSourceElement *source) const;

protected:
    QDeclarativeDebugObjectReference objectReference(const QString &id) const;

    virtual bool visit(UiObjectDefinition *ast);
    virtual void endVisit(UiObjectDefinition *);
    virtual bool visit(UiObjectBinding *ast);
    virtual void endVisit(UiObjectBinding *);
    virtual bool visit(UiScriptBinding *ast);

private:
    QList<UiObjectMember *> objectStack;
    QHash<UiScriptBinding *, UiObjectMember *> _parent;
    QHash<UiObjectMember *, UiScriptBinding *> _id;
    QHash<UiScriptBinding *, QDeclarativeDebugObjectReference> _idBindings;

    QList<QDeclarativeDebugObjectReference> objectReferences;
    QDeclarativeDebugObjectReference m_foundObjectReference;
    unsigned int m_searchElementOffset;
};


class Delta
{
public:
    struct Change {
        Change(): script(0), isLiteral(false) {}

        QmlJS::AST::UiScriptBinding *script;
        bool isLiteral;
        QDeclarativeDebugObjectReference ref;
    };

public:
    void operator()(QmlJS::Document::Ptr doc, QmlJS::Document::Ptr previousDoc);

    QList<Change> changes() const;

    QmlJS::Document::Ptr document() const;
    QmlJS::Document::Ptr previousDocument() const;

private:
    void updateScriptBinding(const QDeclarativeDebugObjectReference &objectReference,
                             QmlJS::AST::UiScriptBinding *scriptBinding,
                             const QString &propertyName,
                             const QString &scriptCode);
    void updateMethodBody(const QDeclarativeDebugObjectReference &objectReference,
                            UiScriptBinding *scriptBinding,
                            const QString &methodName,
                            const QString &methodBody);

    bool compare(UiSourceElement *source, UiSourceElement *other);
    bool compare(QmlJS::AST::UiQualifiedId *id, QmlJS::AST::UiQualifiedId *other);
    QmlJS::AST::UiObjectMemberList *objectMembers(QmlJS::AST::UiObjectMember *object);
    QDeclarativeDebugObjectReference objectReferenceForUiObject(const ScriptBindingParser &bindingParser, UiObjectMember *object);

private:
    QmlJS::Document::Ptr _doc;
    QmlJS::Document::Ptr _previousDoc;
    QList<Change> _changes;
    QUrl _url;
};

} // namespace Internal
} // namespace QmlJSInspector

#endif // QMLJSDELTA_H

