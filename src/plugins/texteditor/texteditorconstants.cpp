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
    case C_SEARCH_SCOPE:        return "SearchScope";
    case C_PARENTHESES:         return "Parentheses";
    case C_CURRENT_LINE:        return "CurrentLine";
    case C_CURRENT_LINE_NUMBER: return "CurrentLineNumber";
    case C_OCCURRENCES:         return "Occurrences";
    case C_OCCURRENCES_UNUSED:  return "Occurrences.Unused";
    case C_OCCURRENCES_RENAME:  return "Occurrences.Rename";

    case C_NUMBER:              return "Number";
    case C_STRING:              return "String";
    case C_TYPE:                return "Type";
    case C_LOCAL:               return "Local";
    case C_FIELD:               return "Field";
    // TODO: Rename "Static" to "Enumeration" in next major update,
    // because renaming here would break styles defined by the user.
    case C_ENUMERATION:         return "Static";
    case C_VIRTUAL_METHOD:      return "VirtualMethod";
    case C_FUNCTION:            return "Function";
    case C_KEYWORD:             return "Keyword";
    case C_OPERATOR:            return "Operator";
    case C_PREPROCESSOR:        return "Preprocessor";
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
    return TextStyle();
}

} // namespace Constants
} // namespace TextEditor
