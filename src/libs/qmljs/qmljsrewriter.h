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

#ifndef QMLJSREWRITER_H
#define QMLJSREWRITER_H

#include <qmljs/qmljs_global.h>

#include <qmljs/parser/qmljsastfwd_p.h>

#include <QtCore/QStringList>

namespace Utils {
class ChangeSet;
}

namespace QmlJS {

class QMLJS_EXPORT Rewriter
{
public:
    enum BindingType {
        ScriptBinding,
        ObjectBinding,
        ArrayBinding
    };

public:
    Rewriter(const QString &originalText,
             Utils::ChangeSet *changeSet,
             const QStringList &propertyOrder);

    void addBinding(AST::UiObjectInitializer *ast,
                    const QString &propertyName,
                    const QString &propertyValue,
                    BindingType bindingType);

    void changeProperty(AST::UiObjectInitializer *ast,
                        const QString &propertyName,
                        const QString &newValue,
                        BindingType binding);

    void removeProperty(AST::UiObjectInitializer *ast, const QString &propertyName);

    void appendToArrayBinding(AST::UiArrayBinding *arrayBinding,
                              const QString &content);
    void addObject(AST::UiObjectInitializer *ast, const QString &content);
    void removeObjectMember(AST::UiObjectMember *member, AST::UiObjectMember *parent);

    static AST::UiObjectMemberList *searchMemberToInsertAfter(AST::UiObjectMemberList *members, const QStringList &propertyOrder);
    static AST::UiObjectMemberList *searchMemberToInsertAfter(AST::UiObjectMemberList *members, const QString &propertyName, const QStringList &propertyOrder);
    static QString flatten(AST::UiQualifiedId *first);

    bool includeSurroundingWhitespace(int &start, int &end) const;
    void includeLeadingEmptyLine(int &start) const;

private:
    void replaceMemberValue(AST::UiObjectMember *propertyMember,
                            const QString &newValue,
                            bool needsSemicolon);
    static bool isMatchingPropertyMember(const QString &propertyName,
                                         AST::UiObjectMember *member);
    static bool nextMemberOnSameLine(AST::UiObjectMemberList *members);

    void insertIntoArray(AST::UiArrayBinding* ast, const QString &newValue);

    void removeMember(AST::UiObjectMember *member);
    void removeGroupedProperty(AST::UiObjectDefinition *ast,
                               const QString &propertyName);

    void extendToLeadingOrTrailingComma(AST::UiArrayBinding *parentArray,
                                        AST::UiObjectMember *member,
                                        int &start,
                                        int &end) const;

private:
    QString m_originalText;
    Utils::ChangeSet *m_changeSet;
    const QStringList m_propertyOrder;
};

} // namespace QmlJS

#endif // QMLJSREWRITER_H
