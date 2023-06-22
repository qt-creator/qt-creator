// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "FastPreprocessor.h"

#include <cplusplus/Literals.h>
#include <cplusplus/TranslationUnit.h>

#include <utils/algorithm.h>

#include <QDir>

using namespace Utils;
using namespace CPlusPlus;

FastPreprocessor::FastPreprocessor(const Snapshot &snapshot)
    : _snapshot(snapshot)
    , _preproc(this, &_env)
    , _addIncludesToCurrentDoc(false)
{ }

QByteArray FastPreprocessor::run(Document::Ptr newDoc,
                                 const QByteArray &source,
                                 bool mergeDefinedMacrosOfDocument)
{
    std::swap(newDoc, _currentDoc);
    _addIncludesToCurrentDoc = _currentDoc->resolvedIncludes().isEmpty()
            && _currentDoc->unresolvedIncludes().isEmpty();
    const FilePath filePath = _currentDoc->filePath();
    _preproc.setExpandFunctionlikeMacros(false);
    _preproc.setKeepComments(true);

    if (Document::Ptr doc = _snapshot.document(filePath)) {
        _merged.insert(filePath);

        for (Snapshot::const_iterator i = _snapshot.begin(), ei = _snapshot.end(); i != ei; ++i) {
            if (isInjectedFile(i.key().toString()))
                mergeEnvironment(i.key());
        }

        const QList<Document::Include> includes = doc->resolvedIncludes();
        for (const Document::Include &i : includes)
            mergeEnvironment(i.resolvedFileName());

        if (mergeDefinedMacrosOfDocument)
            _env.addMacros(_currentDoc->definedMacros());
    }

    const QByteArray preprocessed = _preproc.run(filePath, source);
//    qDebug("FastPreprocessor::run for %s produced [[%s]]", fileName.toUtf8().constData(), preprocessed.constData());
    std::swap(newDoc, _currentDoc);
    return preprocessed;
}

void FastPreprocessor::sourceNeeded(int line, const FilePath &filePath, IncludeType mode,
                                    const FilePaths &initialIncludes)
{
    Q_UNUSED(initialIncludes)
    Q_ASSERT(_currentDoc);
    if (_addIncludesToCurrentDoc) {
        // CHECKME: Is that cleanPath needed?
        const FilePath cleanPath = filePath.cleanPath();
        _currentDoc->addIncludeFile(Document::Include(filePath.toString(), cleanPath, line, mode));
    }
    mergeEnvironment(filePath);
}

void FastPreprocessor::mergeEnvironment(const FilePath &filePath)
{
    if (Utils::insert(_merged, filePath)) {
        if (Document::Ptr doc = _snapshot.document(filePath)) {
            const QList<Document::Include> includes = doc->resolvedIncludes();
            for (const Document::Include &i : includes)
                mergeEnvironment(i.resolvedFileName());

            _env.addMacros(doc->definedMacros());
        }
    }
}

void FastPreprocessor::macroAdded(const Macro &macro)
{
    Q_ASSERT(_currentDoc);

    _currentDoc->appendMacro(macro);
}

static const Macro revision(const Snapshot &s, const Macro &m)
{
    if (Document::Ptr d = s.document(m.filePath())) {
        Macro newMacro(m);
        newMacro.setFileRevision(d->revision());
        return newMacro;
    }

    return m;
}

void FastPreprocessor::passedMacroDefinitionCheck(int bytesOffset, int utf16charsOffset,
                                                  int line, const Macro &macro)
{
    Q_ASSERT(_currentDoc);

    _currentDoc->addMacroUse(revision(_snapshot, macro),
                             bytesOffset, macro.name().size(),
                             utf16charsOffset, macro.nameToQString().size(),
                             line, QVector<MacroArgumentReference>());
}

void FastPreprocessor::failedMacroDefinitionCheck(int bytesOffset, int utf16charsOffset,
                                                  const ByteArrayRef &name)
{
    Q_ASSERT(_currentDoc);

    _currentDoc->addUndefinedMacroUse(QByteArray(name.start(), name.size()),
                                      bytesOffset, utf16charsOffset);
}

void FastPreprocessor::notifyMacroReference(int bytesOffset, int utf16charsOffset,
                                            int line, const Macro &macro)
{
    Q_ASSERT(_currentDoc);

    _currentDoc->addMacroUse(revision(_snapshot, macro),
                             bytesOffset, macro.name().size(),
                             utf16charsOffset, macro.nameToQString().size(),
                             line, QVector<MacroArgumentReference>());
}

void FastPreprocessor::startExpandingMacro(int bytesOffset, int utf16charsOffset,
                                           int line, const Macro &macro,
                                           const QVector<MacroArgumentReference> &actuals)
{
    Q_ASSERT(_currentDoc);

    _currentDoc->addMacroUse(revision(_snapshot, macro),
                             bytesOffset, macro.name().size(),
                             utf16charsOffset, macro.nameToQString().size(),
                             line, actuals);
}

void FastPreprocessor::markAsIncludeGuard(const QByteArray &macroName)
{
    if (!_currentDoc)
        return;

    _currentDoc->setIncludeGuardMacroName(macroName);
}
