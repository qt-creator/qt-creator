// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "CppDocument.h"
#include "PreprocessorClient.h"
#include "PreprocessorEnvironment.h"
#include "pp-engine.h"

#include <cplusplus/Control.h>

#include <utils/filepath.h>

#include <QSet>

namespace CPlusPlus {

class CPLUSPLUS_EXPORT FastPreprocessor: public Client
{
    Environment _env;
    Snapshot _snapshot;
    Preprocessor _preproc{this, &_env};
    QSet<Utils::FilePath> _merged;
    Document::Ptr _currentDoc;
    bool _addIncludesToCurrentDoc = false;
    const bool _expandFunctionLikeMacros;

    void mergeEnvironment(const Utils::FilePath &filePath);

public:
    FastPreprocessor(const Snapshot &snapshot, bool expandFunctionLikeMacros);

    QByteArray run(Document::Ptr newDoc,
                   const QByteArray &source,
                   bool mergeDefinedMacrosOfDocument = false);

    // CPlusPlus::Client
    virtual void sourceNeeded(int line, const Utils::FilePath &filePath, IncludeType mode,
                              const Utils::FilePaths &initialIncludes = {});

    virtual void macroAdded(const Macro &);
    virtual void pragmaAdded(const Pragma &pragma);
    virtual void passedMacroDefinitionCheck(int, int, int, const Macro &);
    virtual void failedMacroDefinitionCheck(int, int, const ByteArrayRef &);

    virtual void notifyMacroReference(int, int, int, const Macro &);

    virtual void startExpandingMacro(int,
                                     int,
                                     int,
                                     const Macro &,
                                     const QList<MacroArgumentReference> &);
    virtual void stopExpandingMacro(int, const Macro &) {}
    virtual void markAsIncludeGuard(const QByteArray &macroName);

    virtual void startSkippingBlocks(int) {}
    virtual void stopSkippingBlocks(int) {}
};

} // namespace CPlusPlus
