// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifdef WITH_TESTS

#include "qmllsclientsettings_test.h"

#include "qmllsclientsettings.h"

#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/ieditor.h>

#include <languageclient/client.h>
#include <languageclient/languageclientmanager.h>

#include <projectexplorer/projectmanager.h>

#include <texteditor/textdocument.h>

#include <utils/temporarydirectory.h>

#include <QPointer>
#include <QScopeGuard>
#include <QTest>

using namespace Core;
using namespace LanguageClient;
using namespace Utils;

namespace QmlJSEditor::Internal {

static int softAssertCount = 0;
static QtMessageHandler previousMessageHandler = nullptr;

static void countSoftAsserts(
    QtMsgType type, const QMessageLogContext &context, const QString &message)
{
    if (message.contains("SOFT ASSERT"))
        ++softAssertCount;
    if (previousMessageHandler)
        previousMessageHandler(type, context, message);
}

class QmllsClientSettingsTest final : public QObject
{
    Q_OBJECT

private slots:
    void cleanup();

    void testQmllsStartsForFileWithoutProject();
    void testStoredStartBehaviorIsMigrated();
    void testNoClientWithoutUsableQmlls();

private:
    QPointer<IDocument> m_document;
    QPointer<Client> m_client;
};

void QmllsClientSettingsTest::cleanup()
{
    if (m_document)
        EditorManager::closeDocuments({m_document.data()}, false);
    if (m_client) {
        LanguageClientManager::shutdownClient(m_client);
        QTRY_VERIFY(!m_client);
    }
}

void QmllsClientSettingsTest::testQmllsStartsForFileWithoutProject()
{
    QmllsClientSettings *settings = qmllsSettings();
    if (!settings)
        QSKIP("The qmlls client settings are not registered.");
    if (!ProjectExplorer::ProjectManager::projects().isEmpty())
        QSKIP("A project is open, so the file would not be project-less.");
    if (!settings->isValidOnBuildConfiguration(nullptr))
        QSKIP("No usable qmlls was found in any Qt version.");

    TemporaryDirectory directory("QmllsClientSettingsTest");
    const FilePath file = directory.path() / "Standalone.qml";
    QVERIFY(file.writeFileContents("import QtQuick\n\nItem {}\n"));

    IEditor *editor = EditorManager::openEditor(file);
    QVERIFY(editor);
    m_document = editor->document();

    QTRY_VERIFY(
        LanguageClientManager::clientForDocument(
            qobject_cast<TextEditor::TextDocument *>(m_document.data())));
    m_client = LanguageClientManager::clientForDocument(
        qobject_cast<TextEditor::TextDocument *>(m_document.data()));
    QVERIFY(m_client);
    QVERIFY(!m_client->buildConfiguration());
    QVERIFY(LanguageClientManager::clientsForSetting(settings).contains(m_client.data()));
    QTRY_COMPARE(m_client->state(), Client::Initialized);
    QTRY_VERIFY(
        m_client->documentOpen(qobject_cast<TextEditor::TextDocument *>(m_document.data())));
}

void QmllsClientSettingsTest::testStoredStartBehaviorIsMigrated()
{
    QmllsClientSettings *settings = qmllsSettings();
    if (!settings)
        QSKIP("The qmlls client settings are not registered.");

    Store map;
    settings->toMap(map);
    map.insert("startupBehavior", int(BaseSettings::RequiresProject));

    QmllsClientSettings loaded;
    loaded.fromMap(map);
    QCOMPARE(loaded.startBehavior(), BaseSettings::RequiresProjectOrFile);
}

void QmllsClientSettingsTest::testNoClientWithoutUsableQmlls()
{
    QmllsClientSettings *settings = qmllsSettings();
    if (!settings)
        QSKIP("The qmlls client settings are not registered.");
    if (!ProjectExplorer::ProjectManager::projects().isEmpty())
        QSKIP("A project is open, so the file would not be project-less.");

    const auto previousSelection = settings->executableSelection();
    const FilePath previousExecutable = settings->executable();
    settings->executableSelection.setValue(QmllsClientSettings::FromUser);
    settings->executable.setValue(FilePath());
    const auto restoreSettings = qScopeGuard([=] {
        settings->executableSelection.setValue(previousSelection);
        settings->executable.setValue(previousExecutable);
    });

    softAssertCount = 0;
    previousMessageHandler = qInstallMessageHandler(countSoftAsserts);
    const auto restoreMessageHandler = qScopeGuard([] {
        qInstallMessageHandler(previousMessageHandler);
    });

    TemporaryDirectory directory("QmllsClientSettingsTest");
    const FilePath file = directory.path() / "Standalone.qml";
    QVERIFY(file.writeFileContents("import QtQuick\n\nItem {}\n"));

    IEditor *editor = EditorManager::openEditor(file);
    QVERIFY(editor);
    m_document = editor->document();

    QCOMPARE(softAssertCount, 0);
    QVERIFY(LanguageClientManager::clientsForSetting(settings).isEmpty());
}

QObject *createQmllsClientSettingsTest()
{
    return new QmllsClientSettingsTest;
}

} // namespace QmlJSEditor::Internal

#include "qmllsclientsettings_test.moc"

#endif // WITH_TESTS
