// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/filepath.h>

#include <QCoreApplication>
#include <QTextDocument>

namespace TextEditor { class TextDocument; }
namespace ProjectExplorer { class RunConfiguration; }

namespace Python::Internal {

class PySideInstaller : public QObject
{
    Q_OBJECT

public:
    static PySideInstaller *instance();
    static void checkPySideInstallation(const Utils::FilePath &python,
                                        TextEditor::TextDocument *document);

signals:
    void pySideInstalled(const Utils::FilePath &python, const QString &pySide);

private:
    PySideInstaller();

    void installPyside(const Utils::FilePath &python,
                       const QString &pySide, TextEditor::TextDocument *document);
    void handlePySideMissing(const Utils::FilePath &python,
                             const QString &pySide,
                             TextEditor::TextDocument *document);

    void runPySideChecker(const Utils::FilePath &python,
                          const QString &pySide,
                          TextEditor::TextDocument *document);
    static bool missingPySideInstallation(const Utils::FilePath &python, const QString &pySide);
    static QString importedPySide(const QString &text);

    QHash<Utils::FilePath, QList<TextEditor::TextDocument *>> m_infoBarEntries;
};

} // Python::Internal
