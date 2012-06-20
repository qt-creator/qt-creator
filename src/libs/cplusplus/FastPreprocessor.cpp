/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
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
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "FastPreprocessor.h"
#include <Literals.h>
#include <TranslationUnit.h>
#include <QDebug>

using namespace CPlusPlus;

FastPreprocessor::FastPreprocessor(const Snapshot &snapshot)
    : _snapshot(snapshot),
      _preproc(this, &_env)
{ }

QByteArray FastPreprocessor::run(QString fileName, const QString &source)
{
    _preproc.setExpandMacros(false);
    _preproc.setKeepComments(true);

    if (Document::Ptr doc = _snapshot.document(fileName)) {
        _merged.insert(fileName);

        mergeEnvironment(QLatin1String("<configuration>"));
        foreach (const Document::Include &i, doc->includes())
            mergeEnvironment(i.fileName());
    }

    const QByteArray preprocessed = _preproc.run(fileName, source);
//    qDebug("FastPreprocessor::run for %s produced [[%s]]", fileName.toUtf8().constData(), preprocessed.constData());
    return preprocessed;
}

void FastPreprocessor::sourceNeeded(unsigned, QString &fileName, IncludeType)
{ mergeEnvironment(fileName); }

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
