// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "texteditorconstants.h"

#include <QByteArray>

namespace TextEditor {
namespace Constants {

const char *nameForStyle(TextStyle style)
{
    switch (style) {
    case C_TEXT:                return "Text";

    case C_LINK:                return "Link";
    case C_SELECTION:           return "Selection";
    case C_LINE_NUMBER:         return "LineNumber";
    case C_SEARCH_RESULT:       return "SearchResult";
    case C_SEARCH_RESULT_ALT1:  return "SearchResultAlt1";
    case C_SEARCH_RESULT_ALT2:  return "SearchResultAlt2";
    case C_SEARCH_RESULT_CONTAINING_FUNCTION: return "SearchResultContainingFunction";
    case C_SEARCH_SCOPE:        return "SearchScope";
    case C_PARENTHESES:         return "Parentheses";
    case C_PARENTHESES_MISMATCH:return "ParenthesesMismatch";
    case C_AUTOCOMPLETE:        return "AutoComplete";
    case C_CURRENT_LINE:        return "CurrentLine";
    case C_CURRENT_LINE_NUMBER: return "CurrentLineNumber";
    case C_OCCURRENCES:         return "Occurrences";
    case C_OCCURRENCES_UNUSED:  return "Occurrences.Unused";
    case C_OCCURRENCES_RENAME:  return "Occurrences.Rename";

    case C_NUMBER:              return "Number";
    case C_STRING:              return "String";
    case C_TYPE:                return "Type";
    case C_CONCEPT:             return "Concept";
    case C_NAMESPACE:           return "Namespace";
    case C_LOCAL:               return "Local";
    case C_PARAMETER:           return "Parameter";
    case C_GLOBAL:              return "Global";
    case C_FIELD:               return "Field";
    // TODO: Rename "Static" to "Enumeration" in next major update,
    // because renaming here would break styles defined by the user.
    case C_ENUMERATION:         return "Static";
    case C_VIRTUAL_METHOD:      return "VirtualMethod";
    case C_FUNCTION:            return "Function";
    case C_KEYWORD:             return "Keyword";
    case C_PRIMITIVE_TYPE:      return "PrimitiveType";
    case C_OPERATOR:            return "Operator";
    case C_OVERLOADED_OPERATOR: return "Overloaded Operator";
    case C_PUNCTUATION:         return "Punctuation";
    case C_PREPROCESSOR:        return "Preprocessor";
    case C_MACRO:               return "Macro";
    case C_LABEL:               return "Label";
    case C_COMMENT:             return "Comment";
    case C_DOXYGEN_COMMENT:     return "Doxygen.Comment";
    case C_DOXYGEN_TAG:         return "Doxygen.Tag";
    case C_VISUAL_WHITESPACE:   return "VisualWhitespace";
    case C_QML_LOCAL_ID:        return "QmlLocalId";
    case C_QML_EXTERNAL_ID:     return "QmlExternalId";
    case C_QML_TYPE_ID:         return "QmlTypeId";
    case C_QML_ROOT_OBJECT_PROPERTY:     return "QmlRootObjectProperty";
    case C_QML_SCOPE_OBJECT_PROPERTY:    return "QmlScopeObjectProperty";
    case C_QML_EXTERNAL_OBJECT_PROPERTY: return "QmlExternalObjectProperty";
    case C_JS_SCOPE_VAR:        return "JsScopeVar";
    case C_JS_IMPORT_VAR:       return "JsImportVar";
    case C_JS_GLOBAL_VAR:       return "JsGlobalVar";
    case C_QML_STATE_NAME:      return "QmlStateName";
    case C_BINDING:             return "Binding";

    case C_DISABLED_CODE:       return "DisabledCode";
    case C_ADDED_LINE:          return "AddedLine";
    case C_REMOVED_LINE:        return "RemovedLine";
    case C_DIFF_FILE:           return "DiffFile";
    case C_DIFF_LOCATION:       return "DiffLocation";

    case C_DIFF_FILE_LINE:      return "DiffFileLine";
    case C_DIFF_CONTEXT_LINE:   return "DiffContextLine";
    case C_DIFF_SOURCE_LINE:    return "DiffSourceLine";
    case C_DIFF_SOURCE_CHAR:    return "DiffSourceChar";
    case C_DIFF_DEST_LINE:      return "DiffDestLine";
    case C_DIFF_DEST_CHAR:      return "DiffDestChar";

    case C_LOG_CHANGE_LINE:     return "LogChangeLine";
    case C_LOG_AUTHOR_NAME:     return "LogAuthorName";
    case C_LOG_COMMIT_DATE:     return "LogCommitDate";
    case C_LOG_COMMIT_HASH:     return "LogCommitHash";
    case C_LOG_COMMIT_SUBJECT:  return "LogCommitSubject";
    case C_LOG_DECORATION:      return "LogDecoration";

    case C_ERROR:               return "Error";
    case C_ERROR_CONTEXT:       return "ErrorContext";
    case C_WARNING:             return "Warning";
    case C_WARNING_CONTEXT:     return "WarningContext";

    case C_DECLARATION:         return "Declaration";
    case C_FUNCTION_DEFINITION: return "FunctionDefinition";
    case C_OUTPUT_ARGUMENT:     return "OutputArgument";
    case C_STATIC_MEMBER:       return "StaticMember";

    case C_COCO_CODE_ADDED: return "CocoCodeAdded";
    case C_COCO_PARTIALLY_COVERED: return "CocoPartiallyCovered";
    case C_COCO_NOT_COVERED: return "CocoNotCovered";
    case C_COCO_FULLY_COVERED: return "CocoFullyCovered";
    case C_COCO_MANUALLY_VALIDATED: return "CocoManuallyValidated";
    case C_COCO_DEAD_CODE: return "CocoDeadCode";
    case C_COCO_EXECUTION_COUNT_TOO_LOW: return "CocoExecutionCountTooLow";
    case C_COCO_NOT_COVERED_INFO: return "CocoNotCoveredInfo";
    case C_COCO_COVERED_INFO: return "CocoCoveredInfo";
    case C_COCO_MANUALLY_VALIDATED_INFO: return "CocoManuallyValidatedInfo";


    case C_LAST_STYLE_SENTINEL: return "LastStyleSentinel";
    }
    return "Unknown Style";
}

TextStyle styleFromName(const char *name)
{
    for (int i = 0; i < C_LAST_STYLE_SENTINEL; ++i) {
        if (qstrcmp(name, nameForStyle(TextStyle(i))) == 0)
            return TextStyle(i);
    }
    return C_LAST_STYLE_SENTINEL;
}

} // namespace Constants
} // namespace TextEditor
