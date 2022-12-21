// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <qmljs/qmljs_global.h>
#include <qmljs/parser/qmljsastfwd_p.h>
#include <utils/changeset.h>

#include <QStringList>

namespace QmlJS {

class QMLJS_EXPORT Rewriter
{
public:
    enum BindingType {
        ScriptBinding,
        ObjectBinding,
        ArrayBinding
    };

    typedef Utils::ChangeSet::Range Range;

public:
    Rewriter(const QString &originalText,
             Utils::ChangeSet *changeSet,
             const QStringList &propertyOrder);

    Range addBinding(AST::UiObjectInitializer *ast,
                     const QString &propertyName,
                     const QString &propertyValue,
                     BindingType bindingType);

    Range addBinding(AST::UiObjectInitializer *ast,
                     const QString &propertyName,
                     const QString &propertyValue,
                     BindingType bindingType,
                     AST::UiObjectMemberList *insertAfter);

    void changeBinding(AST::UiObjectInitializer *ast,
                       const QString &propertyName,
                       const QString &newValue,
                       BindingType binding);

    void removeBindingByName(AST::UiObjectInitializer *ast, const QString &propertyName);

    void appendToArrayBinding(AST::UiArrayBinding *arrayBinding,
                              const QString &content);

    Range addObject(AST::UiObjectInitializer *ast, const QString &content);
    Range addObject(AST::UiObjectInitializer *ast, const QString &content, AST::UiObjectMemberList *insertAfter);
    Range addObject(AST::UiArrayBinding *ast, const QString &content);
    Range addObject(AST::UiArrayBinding *ast, const QString &content, AST::UiArrayMemberList *insertAfter);

    void removeObjectMember(AST::Node *member, AST::UiObjectMember *parent);

    static AST::UiObjectMemberList *searchMemberToInsertAfter(AST::UiObjectMemberList *members, const QStringList &propertyOrder);
    static AST::UiArrayMemberList *searchMemberToInsertAfter(AST::UiArrayMemberList *members, const QStringList &propertyOrder);
    static AST::UiObjectMemberList *searchMemberToInsertAfter(AST::UiObjectMemberList *members, const QString &propertyName, const QStringList &propertyOrder);

    static bool includeSurroundingWhitespace(const QString &source, int &start, int &end);
    static void includeLeadingEmptyLine(const QString &source, int &start);
    static void includeEmptyGroupedProperty(AST::UiObjectDefinition *groupedProperty, AST::UiObjectMember *memberToBeRemoved, int &start, int &end);

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
