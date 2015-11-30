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

#include "FastPreprocessor.h"

#include <cplusplus/Literals.h>
#include <cplusplus/TranslationUnit.h>

#include <QDir>

using namespace CPlusPlus;

FastPreprocessor::FastPreprocessor(const Snapshot &snapshot)
    : _snapshot(snapshot)
    , _preproc(this, &_env)
    , _addIncludesToCurrentDoc(false)
{ }

QByteArray FastPreprocessor::run(Document::Ptr newDoc, const QByteArray &source)
{
    std::swap(newDoc, _currentDoc);
    _addIncludesToCurrentDoc = _currentDoc->resolvedIncludes().isEmpty()
            && _currentDoc->unresolvedIncludes().isEmpty();
    const QString fileName = _currentDoc->fileName();
    _preproc.setExpandFunctionlikeMacros(false);
    _preproc.setKeepComments(true);

    if (Document::Ptr doc = _snapshot.document(fileName)) {
        _merged.insert(fileName);

        for (Snapshot::const_iterator i = _snapshot.begin(), ei = _snapshot.end(); i != ei; ++i) {
            if (isInjectedFile(i.key().toString()))
                mergeEnvironment(i.key().toString());
        }

        foreach (const Document::Include &i, doc->resolvedIncludes())
            mergeEnvironment(i.resolvedFileName());
    }

    const QByteArray preprocessed = _preproc.run(fileName, source);
//    qDebug("FastPreprocessor::run for %s produced [[%s]]", fileName.toUtf8().constData(), preprocessed.constData());
    std::swap(newDoc, _currentDoc);
    return preprocessed;
}

void FastPreprocessor::sourceNeeded(unsigned line, const QString &fileName, IncludeType mode,
                                    const QStringList &initialIncludes)
{
    Q_UNUSED(initialIncludes)
    Q_ASSERT(_currentDoc);
    if (_addIncludesToCurrentDoc) {
        // CHECKME: Is that cleanName needed?
        const QString cleanName = QDir::cleanPath(fileName);
        _currentDoc->addIncludeFile(Document::Include(fileName, cleanName, line, mode));
    }
    mergeEnvironment(fileName);
}

void FastPreprocessor::mergeEnvironment(const QString &fileName)
{
    if (! _merged.contains(fileName)) {
        _merged.insert(fileName);

        if (Document::Ptr doc = _snapshot.document(fileName)) {
            foreach (const Document::Include &i, doc->resolvedIncludes())
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
    if (Document::Ptr d = s.document(m.fileName())) {
        Macro newMacro(m);
        newMacro.setFileRevision(d->revision());
        return newMacro;
    }

    return m;
}

void FastPreprocessor::passedMacroDefinitionCheck(unsigned bytesOffset, unsigned utf16charsOffset,
                                                  unsigned line, const Macro &macro)
{
    Q_ASSERT(_currentDoc);

    _currentDoc->addMacroUse(revision(_snapshot, macro),
                             bytesOffset, macro.name().size(),
                             utf16charsOffset, macro.nameToQString().size(),
                             line, QVector<MacroArgumentReference>());
}

void FastPreprocessor::failedMacroDefinitionCheck(unsigned bytesOffset, unsigned utf16charsOffset,
                                                  const ByteArrayRef &name)
{
    Q_ASSERT(_currentDoc);

    _currentDoc->addUndefinedMacroUse(QByteArray(name.start(), name.size()),
                                      bytesOffset, utf16charsOffset);
}

void FastPreprocessor::notifyMacroReference(unsigned bytesOffset, unsigned utf16charsOffset,
                                            unsigned line, const Macro &macro)
{
    Q_ASSERT(_currentDoc);

    _currentDoc->addMacroUse(revision(_snapshot, macro),
                             bytesOffset, macro.name().size(),
                             utf16charsOffset, macro.nameToQString().size(),
                             line, QVector<MacroArgumentReference>());
}

void FastPreprocessor::startExpandingMacro(unsigned bytesOffset, unsigned utf16charsOffset,
                                           unsigned line, const Macro &macro,
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
