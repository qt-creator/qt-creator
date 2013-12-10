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

#include "unit.h"
#include "unsavedfiledata.h"
#include "utils_p.h"
#include "raii/scopedclangoptions.h"

#include <clang-c/Index.h>

#include <QtCore/QByteArray>
#include <QtCore/QVector>
#include <QtCore/QSharedData>
#include <QtCore/QDateTime>
#include <QtAlgorithms>

#ifdef DEBUG_UNIT_COUNT
#  include <QAtomicInt>
#  include <QDebug>
static QBasicAtomicInt unitDataCount = Q_BASIC_ATOMIC_INITIALIZER(0);
#endif // DEBUG_UNIT_COUNT

namespace ClangCodeModel {
namespace Internal {

class UnitData : public QSharedData
{
public:
    UnitData();
    UnitData(const QString &fileName);
    ~UnitData();

    void swap(UnitData *unitData);

    void unload();
    bool isLoaded() const;

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

} // Internal
} // Clang

using namespace ClangCodeModel;
using namespace ClangCodeModel::Internal;

UnitData::UnitData()
    : m_index(0)
    , m_tu(0)
    , m_managementOptions(0)
{
}

static const int DisplayDiagnostics = qgetenv("QTC_CLANG_VERBOSE").isEmpty() ? 0 : 1;

UnitData::UnitData(const QString &fileName)
    : m_index(clang_createIndex(/*excludeDeclsFromPCH*/ 1, DisplayDiagnostics))
    , m_tu(0)
    , m_fileName(fileName.toUtf8())
    , m_managementOptions(0)
{
}

UnitData::~UnitData()
{
    unload();
    clang_disposeIndex(m_index);
    m_index = 0;
}

void UnitData::swap(UnitData *other)
{
    qSwap(m_index, other->m_index);
    qSwap(m_tu, other->m_tu);
    qSwap(m_fileName, other->m_fileName);
    qSwap(m_compOptions, other->m_compOptions);
    qSwap(m_sharedCompOptions, other->m_sharedCompOptions);
    qSwap(m_managementOptions, other->m_managementOptions);
    qSwap(m_unsaved, other->m_unsaved);
    qSwap(m_timeStamp, other->m_timeStamp);
}

void UnitData::unload()
{
    if (m_tu) {
        clang_disposeTranslationUnit(m_tu);
        m_tu = 0;

#ifdef DEBUG_UNIT_COUNT
        qDebug() << "# translation units:" << (unitDataCount.fetchAndAddOrdered(-1) - 1);
#endif // DEBUG_UNIT_COUNT
    }
}

bool UnitData::isLoaded() const
{
    return m_tu && m_index;
}

void UnitData::updateTimeStamp()
{
    m_timeStamp = QDateTime::currentDateTime();
}

Unit::Unit()
    : m_data(new UnitData)
{}

Unit::Unit(const QString &fileName)
    : m_data(new UnitData(fileName))
{}

Unit::Unit(const Unit &unit)
    : m_data(unit.m_data)
{}

Unit &Unit::operator =(const Unit &unit)
{
    if (this != &unit)
        m_data = unit.m_data;
    return *this;
}

Unit::~Unit()
{}

const QString Unit::fileName() const
{
    const QByteArray &name = m_data->m_fileName;
    return QString::fromUtf8(name.data(), name.size());
}

bool Unit::isLoaded() const
{
    return m_data->isLoaded();
}

const QDateTime &Unit::timeStamp() const
{
    return m_data->m_timeStamp;
}

QStringList Unit::compilationOptions() const
{
    return m_data->m_compOptions;
}

void Unit::setCompilationOptions(const QStringList &compOptions)
{
    m_data->m_compOptions = compOptions;
    m_data->m_sharedCompOptions.reloadOptions(compOptions);
}

UnsavedFiles Unit::unsavedFiles() const
{
    return m_data->m_unsaved;
}

void Unit::setUnsavedFiles(const UnsavedFiles &unsavedFiles)
{
    m_data->m_unsaved = unsavedFiles;
}

unsigned Unit::managementOptions() const
{
    return m_data->m_managementOptions;
}

void Unit::setManagementOptions(unsigned managementOptions)
{
    m_data->m_managementOptions = managementOptions;
}

bool Unit::isUnique() const
{
#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
    return m_data->ref.load() == 1;
#else
    return m_data->ref == 1;
#endif
}

void Unit::makeUnique()
{
    UnitData *uniqueData = new UnitData;
    m_data->swap(uniqueData); // Notice we swap the data itself and not the shared pointer.
    m_data = QExplicitlySharedDataPointer<UnitData>(uniqueData);
}

void Unit::parse()
{
    m_data->unload();

    m_data->updateTimeStamp();

    UnsavedFileData unsaved(m_data->m_unsaved);
    m_data->m_tu = clang_parseTranslationUnit(m_data->m_index,
                                              m_data->m_fileName.constData(),
                                              m_data->m_sharedCompOptions.data(),
                                              m_data->m_sharedCompOptions.size(),
                                              unsaved.files(),
                                              unsaved.count(),
                                              m_data->m_managementOptions);
}

void Unit::reparse()
{
    Q_ASSERT(isLoaded());

    UnsavedFileData unsaved(m_data->m_unsaved);
    const unsigned opts = clang_defaultReparseOptions(m_data->m_tu);
    if (clang_reparseTranslationUnit(m_data->m_tu, unsaved.count(), unsaved.files(), opts) != 0)
        m_data->unload();
}

void Unit::create()
{
    // @TODO
}

void Unit::createFromSourceFile()
{
    // @TODO
}

int Unit::save(const QString &unitFileName)
{
    Q_ASSERT(isLoaded());

    return clang_saveTranslationUnit(m_data->m_tu,
                                     unitFileName.toUtf8().constData(),
                                     clang_defaultSaveOptions(m_data->m_tu));
}

void Unit::unload()
{
    m_data->unload();
}

CXFile Unit::getFile() const
{
    Q_ASSERT(isLoaded());

    return clang_getFile(m_data->m_tu, m_data->m_fileName.constData());
}

CXCursor Unit::getCursor(const CXSourceLocation &location) const
{
    Q_ASSERT(isLoaded());

    return clang_getCursor(m_data->m_tu, location);
}

CXSourceLocation Unit::getLocation(const CXFile &file, unsigned line, unsigned column) const
{
    Q_ASSERT(isLoaded());

    return clang_getLocation(m_data->m_tu, file, line, column);
}

void Unit::codeCompleteAt(unsigned line, unsigned column, ScopedCXCodeCompleteResults &results)
{
    unsigned flags = clang_defaultCodeCompleteOptions();
#if defined(CINDEX_VERSION) && (CINDEX_VERSION > 5)
    flags |= CXCodeComplete_IncludeBriefComments;
#endif

    UnsavedFileData unsaved(m_data->m_unsaved);
    results.reset(clang_codeCompleteAt(m_data->m_tu, m_data->m_fileName.constData(),
                                       line, column,
                                       unsaved.files(), unsaved.count(), flags));
}

void Unit::tokenize(CXSourceRange range, CXToken **tokens, unsigned *tokenCount) const
{
    Q_ASSERT(isLoaded());
    Q_ASSERT(tokens);
    Q_ASSERT(tokenCount);
    Q_ASSERT(!clang_Range_isNull(range));

    clang_tokenize(m_data->m_tu, range, tokens, tokenCount);
}

void Unit::disposeTokens(CXToken *tokens, unsigned tokenCount) const
{
    Q_ASSERT(isLoaded());

    clang_disposeTokens(m_data->m_tu, tokens, tokenCount);
}

CXSourceRange Unit::getTokenExtent(const CXToken &token) const
{
    Q_ASSERT(isLoaded());

    return clang_getTokenExtent(m_data->m_tu, token);
}

void Unit::annotateTokens(CXToken *tokens, unsigned tokenCount, CXCursor *cursors) const
{
    Q_ASSERT(isLoaded());
    Q_ASSERT(tokens);
    Q_ASSERT(cursors);

    clang_annotateTokens(m_data->m_tu, tokens, tokenCount, cursors);
}

CXTranslationUnit Unit::clangTranslationUnit() const
{
    Q_ASSERT(isLoaded());

    return m_data->m_tu;
}

CXIndex Unit::clangIndex() const
{
    Q_ASSERT(isLoaded());

    return m_data->m_index;
}

QString Unit::getTokenSpelling(const CXToken &tok) const
{
    Q_ASSERT(isLoaded());

    return getQString(clang_getTokenSpelling(m_data->m_tu, tok));
}

CXCursor Unit::getTranslationUnitCursor() const
{
    Q_ASSERT(isLoaded());

    return clang_getTranslationUnitCursor(m_data->m_tu);
}

CXString Unit::getTranslationUnitSpelling() const
{
    Q_ASSERT(isLoaded());

    return clang_getTranslationUnitSpelling(m_data->m_tu);
}

void Unit::getInclusions(CXInclusionVisitor visitor, CXClientData clientData) const
{
    Q_ASSERT(isLoaded());

    clang_getInclusions(m_data->m_tu, visitor, clientData);
}

unsigned Unit::getNumDiagnostics() const
{
    Q_ASSERT(isLoaded());

    return clang_getNumDiagnostics(m_data->m_tu);
}

CXDiagnostic Unit::getDiagnostic(unsigned index) const
{
    Q_ASSERT(isLoaded());

    return clang_getDiagnostic(m_data->m_tu, index);
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
