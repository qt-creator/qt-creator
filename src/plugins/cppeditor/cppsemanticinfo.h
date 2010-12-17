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

#ifndef CPPSEMANTICINFO_H
#define CPPSEMANTICINFO_H

#include <cplusplus/CppDocument.h>
#include <cplusplus/LookupContext.h>
#include <QtCore/QHash>

namespace CppEditor {
namespace Internal {

class CPPEditor;

class SemanticInfo
{
public:
    struct Use {
        unsigned line;
        unsigned column;
        unsigned length;
        unsigned kind;

        enum Kind {
            Type = 0,
            Local,
            Field,
            Static,
            VirtualMethod
        };

        Use(unsigned line = 0, unsigned column = 0, unsigned length = 0, unsigned kind = Type)
            : line(line), column(column), length(length), kind(kind) {}
    };

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
    QList<CPlusPlus::Document::DiagnosticMessage> diagnosticMessages;
};

} // end of namespace Internal
} // end of namespace CppEditor;

#endif // CPPSEMANTICINFO_H
