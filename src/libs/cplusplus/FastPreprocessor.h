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

#ifndef CPLUSPLUS_FASTPREPROCESSOR_H
#define CPLUSPLUS_FASTPREPROCESSOR_H

#include "PreprocessorClient.h"
#include "CppDocument.h"
#include "pp.h"

#include <Control.h>

#include <QSet>
#include <QString>

namespace CPlusPlus {

class CPLUSPLUS_EXPORT FastPreprocessor: public Client
{
    Environment _env;
    Snapshot _snapshot;
    Preprocessor _preproc;
    QSet<QString> _merged;

    void mergeEnvironment(const QString &fileName);

public:
    FastPreprocessor(const Snapshot &snapshot);

    QByteArray run(QString fileName, const QString &source);

    // CPlusPlus::Client
    virtual void sourceNeeded(unsigned, QString &fileName, IncludeType);

    virtual void macroAdded(const Macro &) {}

    virtual void passedMacroDefinitionCheck(unsigned, unsigned, const Macro &) {}
    virtual void failedMacroDefinitionCheck(unsigned, const ByteArrayRef &) {}

    virtual void notifyMacroReference(unsigned, unsigned, const Macro &) {}

    virtual void startExpandingMacro(unsigned,
                                     unsigned,
                                     const Macro &,
                                     const QVector<MacroArgumentReference> &) {}
    virtual void stopExpandingMacro(unsigned, const Macro &) {}

    virtual void startSkippingBlocks(unsigned) {}
    virtual void stopSkippingBlocks(unsigned) {}
};

} // namespace CPlusPlus

#endif // CPLUSPLUS_FASTPREPROCESSOR_H
