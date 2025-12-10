// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "cppeditor_global.h"

#include <utils/filepath.h>

namespace ProjectExplorer { class ExtraCompiler; }

namespace CppEditor {

class CPPEDITOR_EXPORT GeneratedFileSupport : public QObject
{
public:
    GeneratedFileSupport(ProjectExplorer::ExtraCompiler *generator,
                         const Utils::FilePath &generatedFile);
    ~GeneratedFileSupport() override;

    /// Returns the contents encoded in UTF-8.
    QByteArray contents() const;
    Utils::FilePath filePath() const; // The generated file
    Utils::FilePath sourceFilePath() const;

    void updateDocument();
    void notifyAboutUpdatedContents() const;
    unsigned revision() const { return m_revision; }

    static void update(const QList<ProjectExplorer::ExtraCompiler *> &generators);

private:
    void onContentsChanged(const Utils::FilePath &file);
    Utils::FilePath m_generatedFilePath;
    ProjectExplorer::ExtraCompiler * const m_generator;
    unsigned m_revision = 1;
};

} // CppEditor
