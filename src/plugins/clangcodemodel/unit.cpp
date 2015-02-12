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

#include "unit.h"

#include "clangutils.h"
#include "unsavedfiledata.h"
#include "utils_p.h"

#include <clang-c/Index.h>

#include <QtCore/QByteArray>
#include <QtCore/QVector>
#include <QtCore/QSharedData>

#ifdef DEBUG_UNIT_COUNT
#  include <QAtomicInt>
#  include <QDebug>
static QBasicAtomicInt unitDataCount = Q_BASIC_ATOMIC_INITIALIZER(0);
#endif // DEBUG_UNIT_COUNT

using namespace ClangCodeModel;
using namespace ClangCodeModel::Internal;

Unit::Unit()
    : m_index(0)
    , m_tu(0)
    , m_managementOptions(0)
{}

Unit::Unit(const QString &fileName)
    : m_index(clang_createIndex(/*excludeDeclsFromPCH*/ 1, Utils::verboseRunLog().isDebugEnabled()))
    , m_tu(0)
    , m_fileName(fileName.toUtf8())
    , m_managementOptions(0)
{}

Unit::~Unit()
{
    unload();
    clang_disposeIndex(m_index);
    m_index = 0;
}

Unit::Ptr Unit::create()
{
    return Unit::Ptr(new Unit);
}

Unit::Ptr Unit::create(const QString &fileName)
{
    return Unit::Ptr(new Unit(fileName));
}

const QString Unit::fileName() const
{
    return QString::fromUtf8(m_fileName.data(), m_fileName.size());
}

bool Unit::isLoaded() const
{
    return m_tu && m_index;
}

const QDateTime &Unit::timeStamp() const
{
    return m_timeStamp;
}

QStringList Unit::compilationOptions() const
{
    return m_compOptions;
}

void Unit::setCompilationOptions(const QStringList &compOptions)
{
    m_compOptions = compOptions;
    m_sharedCompOptions.reloadOptions(compOptions);
}

UnsavedFiles Unit::unsavedFiles() const
{
    return m_unsaved;
}

void Unit::setUnsavedFiles(const UnsavedFiles &unsavedFiles)
{
    m_unsaved = unsavedFiles;
}

unsigned Unit::managementOptions() const
{
    return m_managementOptions;
}

void Unit::setManagementOptions(unsigned managementOptions)
{
    m_managementOptions = managementOptions;
}

void Unit::parse()
{
    unload();

    updateTimeStamp();

    UnsavedFileData unsaved(m_unsaved);
    m_tu = clang_parseTranslationUnit(m_index, m_fileName.constData(),
                                      m_sharedCompOptions.data(), m_sharedCompOptions.size(),
                                      unsaved.files(), unsaved.count(),
                                      m_managementOptions);
}

void Unit::reparse()
{
    Q_ASSERT(isLoaded());

    UnsavedFileData unsaved(m_unsaved);
    const unsigned opts = clang_defaultReparseOptions(m_tu);
    if (clang_reparseTranslationUnit(m_tu, unsaved.count(), unsaved.files(), opts) != 0)
        unload();
}

int Unit::save(const QString &unitFileName)
{
    Q_ASSERT(isLoaded());

    return clang_saveTranslationUnit(m_tu, unitFileName.toUtf8().constData(),
                                     clang_defaultSaveOptions(m_tu));
}

void Unit::unload()
{
    if (m_tu) {
        clang_disposeTranslationUnit(m_tu);
        m_tu = 0;

#ifdef DEBUG_UNIT_COUNT
        qDebug() << "# translation units:" << (unitDataCount.fetchAndAddOrdered(-1) - 1);
#endif // DEBUG_UNIT_COUNT
    }
}

CXFile Unit::getFile() const
{
    Q_ASSERT(isLoaded());

    return clang_getFile(m_tu, m_fileName.constData());
}

CXCursor Unit::getCursor(const CXSourceLocation &location) const
{
    Q_ASSERT(isLoaded());

    return clang_getCursor(m_tu, location);
}

CXSourceLocation Unit::getLocation(const CXFile &file, unsigned line, unsigned column) const
{
    Q_ASSERT(isLoaded());

    return clang_getLocation(m_tu, file, line, column);
}

void Unit::codeCompleteAt(unsigned line, unsigned column, ScopedCXCodeCompleteResults &results)
{
    unsigned flags = clang_defaultCodeCompleteOptions();
#if defined(CINDEX_VERSION) && (CINDEX_VERSION > 5)
    flags |= CXCodeComplete_IncludeBriefComments;
#endif

    UnsavedFileData unsaved(m_unsaved);
    results.reset(clang_codeCompleteAt(m_tu, m_fileName.constData(),
                                       line, column,
                                       unsaved.files(), unsaved.count(),
                                       flags));
}

void Unit::tokenize(CXSourceRange range, CXToken **tokens, unsigned *tokenCount) const
{
    Q_ASSERT(isLoaded());
    Q_ASSERT(tokens);
    Q_ASSERT(tokenCount);
    Q_ASSERT(!clang_Range_isNull(range));

    clang_tokenize(m_tu, range, tokens, tokenCount);
}

void Unit::disposeTokens(CXToken *tokens, unsigned tokenCount) const
{
    Q_ASSERT(isLoaded());

    clang_disposeTokens(m_tu, tokens, tokenCount);
}

CXSourceRange Unit::getTokenExtent(const CXToken &token) const
{
    Q_ASSERT(isLoaded());

    return clang_getTokenExtent(m_tu, token);
}

void Unit::annotateTokens(CXToken *tokens, unsigned tokenCount, CXCursor *cursors) const
{
    Q_ASSERT(isLoaded());
    Q_ASSERT(tokens);
    Q_ASSERT(cursors);

    clang_annotateTokens(m_tu, tokens, tokenCount, cursors);
}

CXTranslationUnit Unit::clangTranslationUnit() const
{
    Q_ASSERT(isLoaded());

    return m_tu;
}

CXIndex Unit::clangIndex() const
{
    Q_ASSERT(isLoaded());

    return m_index;
}

QString Unit::getTokenSpelling(const CXToken &tok) const
{
    Q_ASSERT(isLoaded());

    return getQString(clang_getTokenSpelling(m_tu, tok));
}

CXCursor Unit::getTranslationUnitCursor() const
{
    Q_ASSERT(isLoaded());

    return clang_getTranslationUnitCursor(m_tu);
}

CXString Unit::getTranslationUnitSpelling() const
{
    Q_ASSERT(isLoaded());

    return clang_getTranslationUnitSpelling(m_tu);
}

void Unit::getInclusions(CXInclusionVisitor visitor, CXClientData clientData) const
{
    Q_ASSERT(isLoaded());

    clang_getInclusions(m_tu, visitor, clientData);
}

unsigned Unit::getNumDiagnostics() const
{
    Q_ASSERT(isLoaded());

    return clang_getNumDiagnostics(m_tu);
}

CXDiagnostic Unit::getDiagnostic(unsigned index) const
{
    Q_ASSERT(isLoaded());

    return clang_getDiagnostic(m_tu, index);
}

void Unit::updateTimeStamp()
{
    m_timeStamp = QDateTime::currentDateTime();
}

IdentifierTokens::IdentifierTokens(const Unit &unit, unsigned firstLine, unsigned lastLine)
    : m_unit(unit)
    , m_tokenCount(0)
    , m_tokens(0)
    , m_cursors(0)
    , m_extents(0)
{
    Q_ASSERT(unit.isLoaded());

    // Calculate the range:
    CXFile file = unit.getFile();
    CXSourceLocation startLocation = unit.getLocation(file, firstLine, 1);
    CXSourceLocation endLocation = unit.getLocation(file, lastLine, 1);
    CXSourceRange range = clang_getRange(startLocation, endLocation);

    // Retrieve all identifier tokens:
    unit.tokenize(range, &m_tokens, &m_tokenCount);
    if (m_tokenCount == 0)
        return;

    // Get the cursors for the tokens:
    m_cursors = new CXCursor[m_tokenCount];
    unit.annotateTokens(m_tokens,
                        m_tokenCount,
                        m_cursors);

    m_extents = new CXSourceRange[m_tokenCount];
    // Create the markers using the cursor to check the types:
    for (unsigned i = 0; i < m_tokenCount; ++i)
        m_extents[i] = unit.getTokenExtent(m_tokens[i]);
}

IdentifierTokens::~IdentifierTokens()
{
    dispose();
}

void IdentifierTokens::dispose()
{
    if (!m_unit.isLoaded())
        return;

    if (m_tokenCount && m_tokens) {
        m_unit.disposeTokens(m_tokens, m_tokenCount);
        m_tokens = 0;
        m_tokenCount = 0;
    }

    if (m_cursors) {
        delete[] m_cursors;
        m_cursors = 0;
    }

    if (m_extents) {
        delete[] m_extents;
        m_extents = 0;
    }
}
