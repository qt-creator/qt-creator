/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "tokeninfocontainer.h"

#include <QDebug>

namespace ClangBackEnd {

#define RETURN_TEXT_FOR_CASE(enumValue) case HighlightingType::enumValue: return #enumValue
static const char *highlightingTypeToCStringLiteral(HighlightingType type)
{
    switch (type) {
        RETURN_TEXT_FOR_CASE(Invalid);
        RETURN_TEXT_FOR_CASE(Comment);
        RETURN_TEXT_FOR_CASE(Keyword);
        RETURN_TEXT_FOR_CASE(StringLiteral);
        RETURN_TEXT_FOR_CASE(NumberLiteral);
        RETURN_TEXT_FOR_CASE(Function);
        RETURN_TEXT_FOR_CASE(VirtualFunction);
        RETURN_TEXT_FOR_CASE(Type);
        RETURN_TEXT_FOR_CASE(LocalVariable);
        RETURN_TEXT_FOR_CASE(GlobalVariable);
        RETURN_TEXT_FOR_CASE(Field);
        RETURN_TEXT_FOR_CASE(Enumeration);
        RETURN_TEXT_FOR_CASE(Operator);
        RETURN_TEXT_FOR_CASE(OverloadedOperator);
        RETURN_TEXT_FOR_CASE(Preprocessor);
        RETURN_TEXT_FOR_CASE(Label);
        RETURN_TEXT_FOR_CASE(FunctionDefinition);
        RETURN_TEXT_FOR_CASE(OutputArgument);
        RETURN_TEXT_FOR_CASE(PreprocessorDefinition);
        RETURN_TEXT_FOR_CASE(PreprocessorExpansion);
        RETURN_TEXT_FOR_CASE(Punctuation);
        RETURN_TEXT_FOR_CASE(Namespace);
        RETURN_TEXT_FOR_CASE(Class);
        RETURN_TEXT_FOR_CASE(Struct);
        RETURN_TEXT_FOR_CASE(Enum);
        RETURN_TEXT_FOR_CASE(Union);
        RETURN_TEXT_FOR_CASE(TypeAlias);
        RETURN_TEXT_FOR_CASE(Typedef);
        RETURN_TEXT_FOR_CASE(QtProperty);
        RETURN_TEXT_FOR_CASE(ObjectiveCClass);
        RETURN_TEXT_FOR_CASE(ObjectiveCCategory);
        RETURN_TEXT_FOR_CASE(ObjectiveCProtocol);
        RETURN_TEXT_FOR_CASE(ObjectiveCInterface);
        RETURN_TEXT_FOR_CASE(ObjectiveCImplementation);
        RETURN_TEXT_FOR_CASE(ObjectiveCProperty);
        RETURN_TEXT_FOR_CASE(ObjectiveCMethod);
        RETURN_TEXT_FOR_CASE(PrimitiveType);
        RETURN_TEXT_FOR_CASE(Declaration);
        RETURN_TEXT_FOR_CASE(TemplateTypeParameter);
        RETURN_TEXT_FOR_CASE(TemplateTemplateParameter);
        default: return "UnhandledHighlightingType";
    }
}
#undef RETURN_TEXT_FOR_CASE

QDebug operator<<(QDebug debug, const ExtraInfo &extraInfo)
{
    debug.nospace() << "ExtraInfo("
                    << extraInfo.token << ", "
                    << extraInfo.typeSpelling << ", "
                    << extraInfo.semanticParentTypeSpelling << ", "
                    << extraInfo.cursorRange << ", "
                    << extraInfo.lexicalParentIndex << ", "
                    << static_cast<uint>(extraInfo.accessSpecifier) << ", "
                    << static_cast<uint>(extraInfo.storageClass) << ", "
                    << extraInfo.identifier << ", "
                    << extraInfo.includeDirectivePath << ", "
                    << extraInfo.declaration << ", "
                    << extraInfo.definition << ", "
                    << extraInfo.signal << ", "
                    << extraInfo.slot << ", "
                    << ")";
    return debug;
}

QDebug operator<<(QDebug debug, const TokenInfoContainer &container)
{
    debug.nospace() << "TokenInfosContainer("
                    << container.line << ", "
                    << container.column << ", "
                    << container.length << ", "
                    << highlightingTypeToCStringLiteral(container.types.mainHighlightingType) << ", "
                    << container.extraInfo
                    << ")";

    return debug;
}

} // namespace ClangBackEnd
