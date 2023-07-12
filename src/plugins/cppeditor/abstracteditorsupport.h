// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "cppeditor_global.h"

#include <utils/filepath.h>

#include <QObject>

namespace ProjectExplorer { class Project; }

namespace CppEditor {

class CPPEDITOR_EXPORT AbstractEditorSupport : public QObject
{
    Q_OBJECT

public:
    explicit AbstractEditorSupport(QObject *parent = nullptr);
    ~AbstractEditorSupport() override;

    /// \returns the contents, encoded as UTF-8
    virtual QByteArray contents() const = 0;
    virtual Utils::FilePath filePath() const = 0;
    virtual Utils::FilePath sourceFilePath() const = 0;

    void updateDocument();
    void notifyAboutUpdatedContents() const;
    unsigned revision() const { return m_revision; }

    static QString licenseTemplate(ProjectExplorer::Project *project,
                                   const Utils::FilePath &filePath = {},
                                   const QString &className = {});
    static bool usePragmaOnce(ProjectExplorer::Project *project);

private:
    unsigned m_revision;
};

} // CppEditor
