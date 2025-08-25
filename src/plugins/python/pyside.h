// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <solutions/tasking/tasktreerunner.h>

#include <utils/filepath.h>

namespace Core { class IDocument; }
namespace TextEditor { class TextDocument; }

namespace Python::Internal {

class PySideTools
{
public:
    Utils::FilePath pySideProjectPath;
    Utils::FilePath pySideUicPath;
};

class PySideInstaller : public QObject
{
    Q_OBJECT

public:
    void checkPySideInstallation(const Utils::FilePath &python, TextEditor::TextDocument *document);
    static PySideInstaller &instance();
    void installPySide(const Utils::FilePath &python, const QString &pySide, bool quiet = false);

public slots:
    void installPySide(const QUrl &url);

signals:
    void pySideInstalled(const Utils::FilePath &pythonPath, const QString &pySide);

private:
    PySideInstaller();
    ~PySideInstaller();

    void handlePySideMissing(const Utils::FilePath &python,
                             const QString &pySide,
                             TextEditor::TextDocument *document);
    void handleDocumentOpened(Core::IDocument *document);

    void runPySideChecker(const Utils::FilePath &python,
                          const QString &pySide,
                          TextEditor::TextDocument *document);
    static QString usedPySide(const QString &text, const QString &mimeType);

    QHash<Utils::FilePath, QList<TextEditor::TextDocument *>> m_infoBarEntries;
    Tasking::MappedTaskTreeRunner<TextEditor::TextDocument *> m_taskTreeRunner;
    Tasking::SingleTaskTreeRunner m_pipInstallerRunner;
};

} // Python::Internal
