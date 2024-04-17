// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/filepath.h>

#include <QCoreApplication>
#include <QFutureWatcher>
#include <QPointer>
#include <QTextDocument>

namespace Core { class IDocument; }
namespace TextEditor { class TextDocument; }
namespace ProjectExplorer { class RunConfiguration; }

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

signals:
    void pySideInstalled(const Utils::FilePath &python, const QString &pySide);

private:
    PySideInstaller();

    void installPyside(const Utils::FilePath &python,
                       const QString &pySide, TextEditor::TextDocument *document);
    void handlePySideMissing(const Utils::FilePath &python,
                             const QString &pySide,
                             TextEditor::TextDocument *document);
    void handleDocumentOpened(Core::IDocument *document);

    void runPySideChecker(const Utils::FilePath &python,
                          const QString &pySide,
                          TextEditor::TextDocument *document);
    static bool missingPySideInstallation(const Utils::FilePath &python, const QString &pySide);
    static QString usedPySide(const QString &text, const QString &mimeType);

    QHash<Utils::FilePath, QList<TextEditor::TextDocument *>> m_infoBarEntries;
    QHash<TextEditor::TextDocument *, QPointer<QFutureWatcher<bool>>> m_futureWatchers;
};

} // Python::Internal
