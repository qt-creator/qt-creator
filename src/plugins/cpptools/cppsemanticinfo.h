/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**************************************************************************/

#ifndef CPPSEMANTICINFO_H
#define CPPSEMANTICINFO_H

#include "cpptools_global.h"

#include <cplusplus/CppDocument.h>
#include <cplusplus/LookupContext.h>
#include <texteditor/semantichighlighter.h>
#include <QHash>

namespace CppTools {

class CPPTOOLS_EXPORT SemanticInfo
{
public:
    enum UseKind {
        Unknown = 0,
        TypeUse,
        LocalUse,
        FieldUse,
        StaticUse,
        VirtualMethodUse,
        LabelUse,
        MacroUse,
        FunctionUse
    };
    typedef TextEditor::SemanticHighlighter::Result Use;

    typedef QHash<CPlusPlus::Symbol *, QList<Use> > LocalUseMap;
    typedef QHashIterator<CPlusPlus::Symbol *, QList<Use> > LocalUseIterator;

    SemanticInfo();

    unsigned revision;
    bool hasQ: 1;
    bool hasD: 1;
    bool forced: 1;
    CPlusPlus::Snapshot snapshot;
    CPlusPlus::Document::Ptr doc;
    LocalUseMap localUses;
    QList<Use> objcKeywords;
};

} // namespace CppTools

#endif // CPPSEMANTICINFO_H
