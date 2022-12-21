// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "cppindexingsupport.h"
#include "cppmodelmanager.h"

#include <utils/futuresynchronizer.h>

namespace CppEditor::Internal {

class BuiltinIndexingSupport: public CppIndexingSupport {
public:
    BuiltinIndexingSupport();
    ~BuiltinIndexingSupport() override;

    QFuture<void> refreshSourceFiles(const QSet<QString> &sourceFiles,
                                     CppModelManager::ProgressNotificationMode mode) override;
    SymbolSearcher *createSymbolSearcher(const SymbolSearcher::Parameters &parameters,
                                         const QSet<QString> &fileNames) override;

public:
    static bool isFindErrorsIndexingActive();

private:
    Utils::FutureSynchronizer m_synchronizer;
};

} // namespace CppEditor::Internal
