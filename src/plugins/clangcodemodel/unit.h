/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef UNIT_H
#define UNIT_H

#include "utils.h"
#include "raii/scopedclangoptions.h"
#include "cxraii.h"

#include <QtCore/QDateTime>
#include <QSharedPointer>
#include <QMetaType>
#include <QString>
#include <QStringList>
#include <QVarLengthArray>

QT_BEGIN_NAMESPACE
class QDateTime;
QT_END_NAMESPACE

namespace ClangCodeModel {
namespace Internal {

class UnitData;

/*
 * This is a minimal wrapper around clang's translation unit functionality.
 * It should should contain only the very basic primitives which allow other
 * components such as code completion, code navigation, and others to access
 * data which directly depends on the translation unit.
 *
 * In other words, what's wrapped here is only the functions that receive a
 * CXTranslationUnit as a parameter. And this class itself is then the corresponding
 * abstraction of the CXTranslationUnit.
 *
 * Notes:
 *  - This class is not thread-safe.
 *  - It's responsibility of the client to make sure that the wrapped translation
 *    unit is consistent with the other data such as cursor and locations being used.
 *
 * @TODO: This is similar but not exactly the same as the current ClangWrapper class.
 * That class is now tuned to specific features, so it's not generic enough to be used
 * an underlying component and aslo don't provide the data in a fine granularity as
 * needed here. At some point we should split ClangWrapper into its different logical
 * components and use this is the underlying structure.
 */
class Unit
{
    Q_DISABLE_COPY(Unit)

    Unit();
    explicit Unit(const QString &fileName);

public:
    ~Unit();

    typedef QSharedPointer<Unit> Ptr;
    static Ptr create();
    static Ptr create(const QString &fileName);

    bool isLoaded() const;

    const QString fileName() const;

    const QDateTime &timeStamp() const;

    QStringList compilationOptions() const;
    void setCompilationOptions(const QStringList &compOptions);

    UnsavedFiles unsavedFiles() const;
    void setUnsavedFiles(const UnsavedFiles &unsavedFiles);

    unsigned managementOptions() const;
    void setManagementOptions(unsigned managementOptions);

    // Methods for generating the TU. Name mappings are direct, for example:
    //   - parse corresponds to clang_parseTranslationUnit
    //   - createFromSourceFile corresponds to clang_createTranslationUnitFromSourceFile
    void parse();
    void reparse();
    int save(const QString &unitFileName);
    void unload();

    // Simple forwarding methods, separated by clang categories for convenience.
    // As above, the names are directly mapped. Separated by categories as clang for convenience.
    // Note that only methods that take the TU as a parameter should be wrapped.

    // - Diagnostic reporting
    unsigned getNumDiagnostics() const;
    CXDiagnostic getDiagnostic(unsigned index) const;

    // - Translation unit manipulation
    CXString getTranslationUnitSpelling() const;

    // - File manipulation routines
    CXFile getFile() const;

    // - Mapping between cursors and source code
    CXCursor getCursor(const CXSourceLocation &location) const;

    // - Miscellaneous utility functions
    void getInclusions(CXInclusionVisitor visitor, CXClientData clientData) const;

    // - Cursor manipulations
    CXCursor getTranslationUnitCursor() const;

    // - Physical source locations
    CXSourceLocation getLocation(const CXFile &file, unsigned line, unsigned column) const;

    void codeCompleteAt(unsigned line, unsigned column, ScopedCXCodeCompleteResults &results);

    void tokenize(CXSourceRange range, CXToken **tokens, unsigned *tokenCount) const;
    void disposeTokens(CXToken *tokens, unsigned tokenCount) const;
    CXSourceRange getTokenExtent(const CXToken &token) const;
    void annotateTokens(CXToken *tokens, unsigned tokenCount, CXCursor *cursors) const;

    CXTranslationUnit clangTranslationUnit() const;
    CXIndex clangIndex() const;

    QString getTokenSpelling(const CXToken &tok) const;

private:
    void updateTimeStamp();

    CXIndex m_index;
    CXTranslationUnit m_tu;
    QByteArray m_fileName;
    QStringList m_compOptions;
    SharedClangOptions m_sharedCompOptions;
    unsigned m_managementOptions;
    UnsavedFiles m_unsaved;
    QDateTime m_timeStamp;
};

class IdentifierTokens
{
    Q_DISABLE_COPY(IdentifierTokens)

public:
    IdentifierTokens(const Unit &m_unit, unsigned firstLine, unsigned lastLine);
    ~IdentifierTokens();

    unsigned count() const
    { return m_tokenCount; }

    const CXToken &token(unsigned nr) const
    { Q_ASSERT(nr < count()); return m_tokens[nr]; }

    const CXCursor &cursor(unsigned nr) const
    { Q_ASSERT(nr < count()); return m_cursors[nr]; }

    const CXSourceRange &extent(unsigned nr) const
    { Q_ASSERT(nr < count()); return m_extents[nr]; }

private:
    void dispose();

private:
    const Unit &m_unit;
    unsigned m_tokenCount;
    CXToken *m_tokens;

    CXCursor *m_cursors;
    CXSourceRange *m_extents;
};

} // Internal
} // Clang

Q_DECLARE_METATYPE(ClangCodeModel::Internal::Unit::Ptr)

#endif // UNIT_H
