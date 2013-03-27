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

#include "FastPreprocessor.h"

#include <cplusplus/Literals.h>
#include <cplusplus/TranslationUnit.h>

#include <QDir>

using namespace CPlusPlus;

FastPreprocessor::FastPreprocessor(const Snapshot &snapshot)
    : _snapshot(snapshot)
    , _preproc(this, &_env)
{ }

QByteArray FastPreprocessor::run(Document::Ptr newDoc, const QString &source)
{
    std::swap(newDoc, _currentDoc);
    const QString fileName = _currentDoc->fileName();
    _preproc.setExpandFunctionlikeMacros(false);
    _preproc.setKeepComments(true);

    if (Document::Ptr doc = _snapshot.document(fileName)) {
        _merged.insert(fileName);

        mergeEnvironment(Preprocessor::configurationFileName);
        foreach (const Document::Include &i, doc->includes())
            mergeEnvironment(i.fileName());
    }

    const QByteArray preprocessed = _preproc.run(fileName, source);
//    qDebug("FastPreprocessor::run for %s produced [[%s]]", fileName.toUtf8().constData(), preprocessed.constData());
    std::swap(newDoc, _currentDoc);
    return preprocessed;
}

void FastPreprocessor::sourceNeeded(unsigned line, QString &fileName, IncludeType)
{
    Q_ASSERT(_currentDoc);
    // CHECKME: Is that cleanName needed?
    QString cleanName = QDir::cleanPath(fileName);
    _currentDoc->addIncludeFile(cleanName, line);

    mergeEnvironment(fileName);
}

void FastPreprocessor::mergeEnvironment(const QString &fileName)
{
    if (! _merged.contains(fileName)) {
        _merged.insert(fileName);

        if (Document::Ptr doc = _snapshot.document(fileName)) {
            foreach (const Document::Include &i, doc->includes())
                mergeEnvironment(i.fileName());

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

void FastPreprocessor::passedMacroDefinitionCheck(unsigned offset, unsigned line, const Macro &macro)
{
    Q_ASSERT(_currentDoc);

    _currentDoc->addMacroUse(revision(_snapshot, macro),
                             offset, macro.name().length(), line,
                             QVector<MacroArgumentReference>());
}

void FastPreprocessor::failedMacroDefinitionCheck(unsigned offset, const ByteArrayRef &name)
{
    Q_ASSERT(_currentDoc);

    _currentDoc->addUndefinedMacroUse(QByteArray(name.start(), name.size()), offset);
}

void FastPreprocessor::notifyMacroReference(unsigned offset, unsigned line, const Macro &macro)
{
    Q_ASSERT(_currentDoc);

    _currentDoc->addMacroUse(revision(_snapshot, macro),
                             offset, macro.name().length(), line,
                             QVector<MacroArgumentReference>());
}

void FastPreprocessor::startExpandingMacro(unsigned offset, unsigned line,
                                          const Macro &macro,
                                          const QVector<MacroArgumentReference> &actuals)
{
    Q_ASSERT(_currentDoc);

    _currentDoc->addMacroUse(revision(_snapshot, macro),
                             offset, macro.name().length(), line, actuals);
}
