/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
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
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "FastPreprocessor.h"
#include <Literals.h>
#include <TranslationUnit.h>

using namespace CPlusPlus;

FastMacroResolver::FastMacroResolver(const Snapshot &snapshot)
    : _snapshot(snapshot)
{ }

bool FastMacroResolver::isMacro(TranslationUnit *unit, unsigned tokenIndex) const
{
    const Token &tk = unit->tokenAt(tokenIndex);
    if (tk.isNot(T_IDENTIFIER))
        return false;

    Identifier *id = tk.identifier;
    const QByteArray macroName = QByteArray::fromRawData(id->chars(), id->size());
    const QString fileName = QString::fromUtf8(unit->fileName(), unit->fileNameLength());

    bool done = false;
    QSet<QString> processed;

    if (isMacro_helper(macroName, fileName, &processed, &done))
        return true;

    return false;
}

bool FastMacroResolver::isMacro_helper(const QByteArray &macroName,
                                       const QString &fileName,
                                       QSet<QString> *processed,
                                       bool *done) const
{
    if (processed->contains(fileName))
        return false;

    processed->insert(fileName);

    if (Document::Ptr doc = _snapshot.value(fileName)) {
        const QList<Macro> definedMacros = doc->definedMacros();

        for (int i = definedMacros.size() - 1; i != -1; --i) {
            const Macro &macro = definedMacros.at(i);

            if (macro.name() == macroName) { // ### handle line numbers.
                if (macro.isHidden()) {
                    *done = true;
                    return false;
                }

                return true;
            }
        }

        foreach (const Document::Include &incl, doc->includes()) {
            if (isMacro_helper(macroName, incl.fileName(), processed, done))
                return true;
            else if (*done)
                return false;
        }
    }

    return false;
}

FastPreprocessor::FastPreprocessor(const Snapshot &snapshot)
    : _snapshot(snapshot),
      _preproc(this, &_env)
{ }

QByteArray FastPreprocessor::run(QString fileName, const QString &source)
{
    _preproc.setExpandMacros(false);

    if (Document::Ptr doc = _snapshot.value(fileName)) {
        _merged.insert(fileName);

        foreach (const Document::Include &i, doc->includes())
            mergeEnvironment(i.fileName());
    }

    const QByteArray preprocessed = _preproc(fileName, source);
    return preprocessed;
}

void FastPreprocessor::sourceNeeded(QString &fileName, IncludeType, unsigned)
{ mergeEnvironment(fileName); }

void FastPreprocessor::mergeEnvironment(const QString &fileName)
{
    if (! _merged.contains(fileName)) {
        _merged.insert(fileName);

        if (Document::Ptr doc = _snapshot.value(fileName)) {
            foreach (const Document::Include &i, doc->includes())
                mergeEnvironment(i.fileName());

            _env.addMacros(doc->definedMacros());
        }
    }
}
