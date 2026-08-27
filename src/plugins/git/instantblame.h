// Copyright (C) 2023 Andre Hartmann (aha_1980@gmx.de)
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "gitclient.h"

#include <texteditor/textmark.h>

#include <utils/filepath.h>

#include <QtTaskTree/QSingleTaskTreeRunner>

QT_BEGIN_NAMESPACE
class QLayout;
class QTimer;
QT_END_NAMESPACE

#ifdef WITH_TESTS
namespace ExtensionSystem { class IPlugin; }
#endif

namespace Git::Internal {

class BlameController;

// Shows blame annotations at a fixed revision for the line under the cursor
// of an editor widget whose document contains that revision of the file, for
// example the baseline view of the inline diff editor.
class BaselineBlame : public QObject
{
    Q_OBJECT

public:
    BaselineBlame(TextEditor::TextEditorWidget *widget,
                  const Utils::FilePath &topLevel,
                  const QString &ref,
                  const QString &relativeFile,
                  const Utils::FilePath &workingFilePath);

private:
    BlameController *m_controller = nullptr;
};

class InstantBlame : public QObject
{
    Q_OBJECT

public:
    InstantBlame();

    void setup();
    void repeat();
    void once();

private:
    void setupForCurrentEditor();
    bool setEditor(TextEditor::TextEditorWidget *widget);
    void scheduleInstantBlame();
    void stop();
    void slotDocumentChanged();

    BlameController *m_controller = nullptr;
    QPointer<TextEditor::TextDocument> m_document;
    bool m_modified = false;
    QMetaObject::Connection m_blameCursorPosConn;
    QMetaObject::Connection m_documentChangedConn;
};

#ifdef WITH_TESTS
void registerInstantBlameTests(ExtensionSystem::IPlugin *plugin);
#endif

} // Git::Internal
