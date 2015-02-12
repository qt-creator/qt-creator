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

#ifndef CPLUSPLUS_FASTPREPROCESSOR_H
#define CPLUSPLUS_FASTPREPROCESSOR_H

#include "PreprocessorClient.h"
#include "CppDocument.h"
#include "pp.h"

#include <cplusplus/Control.h>

#include <QSet>
#include <QString>

namespace CPlusPlus {

class CPLUSPLUS_EXPORT FastPreprocessor: public Client
{
    Environment _env;
    Snapshot _snapshot;
    Preprocessor _preproc;
    QSet<QString> _merged;
    Document::Ptr _currentDoc;
    bool _addIncludesToCurrentDoc;

    void mergeEnvironment(const QString &fileName);

public:
    FastPreprocessor(const Snapshot &snapshot);

    QByteArray run(Document::Ptr newDoc, const QByteArray &source);

    // CPlusPlus::Client
    virtual void sourceNeeded(unsigned line, const QString &fileName, IncludeType mode,
                              const QStringList &initialIncludes = QStringList());

    virtual void macroAdded(const Macro &);

    virtual void passedMacroDefinitionCheck(unsigned, unsigned, unsigned, const Macro &);
    virtual void failedMacroDefinitionCheck(unsigned, unsigned, const ByteArrayRef &);

    virtual void notifyMacroReference(unsigned, unsigned, unsigned, const Macro &);

    virtual void startExpandingMacro(unsigned,
                                     unsigned,
                                     unsigned,
                                     const Macro &,
                                     const QVector<MacroArgumentReference> &);
    virtual void stopExpandingMacro(unsigned, const Macro &) {}
    virtual void markAsIncludeGuard(const QByteArray &macroName);

    virtual void startSkippingBlocks(unsigned) {}
    virtual void stopSkippingBlocks(unsigned) {}
};

} // namespace CPlusPlus

#endif // CPLUSPLUS_FASTPREPROCESSOR_H
