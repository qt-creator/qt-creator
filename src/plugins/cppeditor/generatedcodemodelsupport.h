// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "cppeditor_global.h"
#include "abstracteditorsupport.h"

namespace ProjectExplorer { class ExtraCompiler; }

namespace CppEditor {

class CPPEDITOR_EXPORT GeneratedCodeModelSupport : public AbstractEditorSupport
{
    Q_OBJECT

public:
    GeneratedCodeModelSupport(ProjectExplorer::ExtraCompiler *generator,
                              const Utils::FilePath &generatedFile);
    ~GeneratedCodeModelSupport() override;

    /// Returns the contents encoded in UTF-8.
    QByteArray contents() const override;
    Utils::FilePath filePath() const override; // The generated file
    Utils::FilePath sourceFilePath() const override;

    static void update(const QList<ProjectExplorer::ExtraCompiler *> &generators);

private:
    void onContentsChanged(const Utils::FilePath &file);
    Utils::FilePath m_generatedFilePath;
    ProjectExplorer::ExtraCompiler *m_generator;
};

} // CppEditor
