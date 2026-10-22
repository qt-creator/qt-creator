// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "workspaceeditpreview.h"

#include "client.h"
#include "languageclientsymbolsupport.h"
#include "languageclienttr.h"
#include "languageclientutils.h"

#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/find/searchresultwindow.h>

#include <utils/qtcassert.h>

#include <QPointer>

#ifdef WITH_TESTS
#include "languageclientinterface.h"
#include "languageclientmanager.h"

#include <languageserverprotocol/lspmessages.h>

#include <utils/algorithm.h>
#include <utils/temporarydirectory.h>

#include <QJsonDocument>
#include <QRegularExpression>
#include <QSignalSpy>
#include <QTest>
#include <QTimer>
#include <QToolButton>
#endif

using namespace LanguageServerProtocol;
using namespace Utils;

namespace LanguageClient {

/// Whether an edit or file operation in \a edit refers to one of its change
/// annotations.
bool hasChangeAnnotations(const WorkspaceEdit &edit)
{
    for (const WorkspaceEditDocumentChangesItem &change : edit.documentChanges().value_or(
             QList<WorkspaceEditDocumentChangesItem>())) {
        if (const auto documentEdit = std::get_if<TextDocumentEdit>(&change)) {
            for (const TextDocumentEditEditsItem &item : documentEdit->edits()) {
                if (changeAnnotation(edit, annotationId(item)))
                    return true;
            }
        } else if (changeAnnotation(edit, annotationId(change))) {
            return true;
        }
    }
    return false;
}

/// Puts \a item below the label of \a annotation and leaves it unchecked if
/// the annotation asks for confirmation.
void groupByAnnotation(SearchResultItem &item, const std::optional<ChangeAnnotation> &annotation)
{
    if (!annotation)
        return;
    QTC_ASSERT(item.filePath(), return);
    item.setPath({annotation->label(), item.filePath()->toUserOutput()});
    if (annotation->needsConfirmation().value_or(false))
        item.setSelectForReplacement(false);
}

/// Shows the line of \a range as it reads once \a newText replaced it, with the
/// new text marked. An edit spanning lines shows its first line as it is.
static void setTextEdit(
    SearchResultItem &item, const QStringList &lines, const Range &range, const QString &newText)
{
    const int line = range.start().line();
    const int column = range.start().character();
    const QString lineText = lines.value(line);
    if (range.end().line() == line && !newText.contains('\n')) {
        item.setLineText(lineText.left(column) + newText + lineText.mid(range.end().character()));
        item.setMainRange(line + 1, column, newText.size());
    } else {
        item.setLineText(lineText);
        item.setMainRange(line + 1, column, std::max(0, int(lineText.size()) - column));
    }
}

/// One search result per text edit and file operation in \a edit, grouped by
/// the label of their annotation, if any. The user data of a result holds the
/// index of its document change and the index of the edit in it, or -1 for a
/// file operation.
SearchResultItems workspaceEditPreviewItems(
    const WorkspaceEdit &edit, const std::function<FilePath(const QString &uri)> &filePathFor)
{
    SearchResultItems items;
    const QList<WorkspaceEditDocumentChangesItem> changes = edit.documentChanges().value_or(
        QList<WorkspaceEditDocumentChangesItem>());
    for (int changeIndex = 0; changeIndex < changes.size(); ++changeIndex) {
        const WorkspaceEditDocumentChangesItem &change = changes.at(changeIndex);
        SearchResultItem item;
        if (const auto documentEdit = std::get_if<TextDocumentEdit>(&change)) {
            item.setFilePath(filePathFor(documentEdit->textDocument().uri()));
            item.setUseTextEditorFont(true);
            const QStringList lines = SymbolSupport::getFileContents(*item.filePath());
            const QList<TextDocumentEditEditsItem> &edits = documentEdit->edits();
            for (int editIndex = 0; editIndex < edits.size(); ++editIndex) {
                const TextDocumentEditEditsItem &textEdit = edits.at(editIndex);
                const std::optional<TextEdit> plain = plainTextEdit(textEdit);
                if (!plain)
                    continue;
                SearchResultItem editItem = item;
                setTextEdit(editItem, lines, plain->range(), plain->newText());
                editItem.setUserData(QVariantList{changeIndex, editIndex});
                groupByAnnotation(editItem, changeAnnotation(edit, annotationId(textEdit)));
                items << editItem;
            }
            continue;
        }
        if (const auto op = std::get_if<LanguageServerProtocol::CreateFile>(&change)) {
            item.setFilePath(filePathFor(op->uri()));
            item.setLineText(Tr::tr("Create %1").arg(item.filePath()->toUserOutput()));
        } else if (const auto op = std::get_if<RenameFile>(&change)) {
            item.setFilePath(filePathFor(op->oldUri()));
            item.setLineText(Tr::tr("Rename %1 to %2")
                                 .arg(item.filePath()->toUserOutput(),
                                      filePathFor(op->newUri()).toUserOutput()));
        } else if (const auto op = std::get_if<LanguageServerProtocol::DeleteFile>(&change)) {
            item.setFilePath(filePathFor(op->uri()));
            item.setLineText(Tr::tr("Delete %1").arg(item.filePath()->toUserOutput()));
        }
        item.setUserData(QVariantList{changeIndex, -1});
        groupByAnnotation(item, changeAnnotation(edit, annotationId(change)));
        items << item;
    }
    return items;
}

/// \a edit with only the edits and file operations of \a checkedItems, which
/// come from workspaceEditPreviewItems(). Document changes left without any
/// edit are dropped, and so are the plain changes a server may send as a
/// fallback for them.
WorkspaceEdit selectedWorkspaceEdit(const WorkspaceEdit &edit, const SearchResultItems &checkedItems)
{
    QSet<std::pair<int, int>> selected;
    for (const SearchResultItem &item : checkedItems) {
        const QVariantList indexes = item.userData().toList();
        if (indexes.size() == 2)
            selected.insert({indexes.at(0).toInt(), indexes.at(1).toInt()});
    }
    const QList<WorkspaceEditDocumentChangesItem> changes = edit.documentChanges().value_or(
        QList<WorkspaceEditDocumentChangesItem>());
    QList<WorkspaceEditDocumentChangesItem> selectedChanges;
    for (int changeIndex = 0; changeIndex < changes.size(); ++changeIndex) {
        const WorkspaceEditDocumentChangesItem &change = changes.at(changeIndex);
        if (const auto documentEdit = std::get_if<TextDocumentEdit>(&change)) {
            QList<TextDocumentEditEditsItem> edits;
            for (int editIndex = 0; editIndex < documentEdit->edits().size(); ++editIndex) {
                if (selected.contains({changeIndex, editIndex}))
                    edits << documentEdit->edits().at(editIndex);
            }
            if (!edits.isEmpty())
                selectedChanges << TextDocumentEdit(*documentEdit).edits(edits);
        } else if (selected.contains({changeIndex, -1})) {
            selectedChanges << change;
        }
    }
    return WorkspaceEdit(edit).changes(std::nullopt).documentChanges(selectedChanges);
}

/// A hash of the lines \a linesFor gives for each document \a edit has text
/// edits for. Unlike a document version, it also changes for a file the client
/// has not opened.
QHash<FilePath, size_t> documentSnapshots(
    const WorkspaceEdit &edit,
    const std::function<FilePath(const QString &uri)> &filePathFor,
    const std::function<QStringList(const FilePath &filePath)> &linesFor)
{
    QHash<FilePath, size_t> snapshots;
    for (const WorkspaceEditDocumentChangesItem &change : edit.documentChanges().value_or(
             QList<WorkspaceEditDocumentChangesItem>())) {
        if (const auto documentEdit = std::get_if<TextDocumentEdit>(&change)) {
            const FilePath filePath = filePathFor(documentEdit->textDocument().uri());
            if (!snapshots.contains(filePath))
                snapshots.insert(filePath, qHash(linesFor(filePath)));
        }
    }
    return snapshots;
}

/// Whether \a selected, which comes from selectedWorkspaceEdit(), can be
/// applied to documents that had \a previewedSnapshots when the edit was
/// previewed. It cannot if nothing is selected, or if a document it edits has
/// changed since, as the edits would then land at the wrong places.
Result<> canApplySelectedEdit(
    const WorkspaceEdit &selected,
    const QHash<FilePath, size_t> &previewedSnapshots,
    const std::function<FilePath(const QString &uri)> &filePathFor,
    const std::function<QStringList(const FilePath &filePath)> &linesFor)
{
    if (selected.documentChanges().value_or(QList<WorkspaceEditDocumentChangesItem>()).isEmpty())
        return ResultError(Tr::tr("No change was selected, so nothing was applied."));
    QStringList changed;
    const QHash<FilePath, size_t> snapshots = documentSnapshots(selected, filePathFor, linesFor);
    for (auto it = snapshots.cbegin(); it != snapshots.cend(); ++it) {
        if (previewedSnapshots.value(it.key(), it.value()) != it.value())
            changed << it.key().toUserOutput();
    }
    if (!changed.isEmpty()) {
        changed.sort();
        return ResultError(Tr::tr("%1 changed after the changes were proposed, so nothing was "
                                  "applied.")
                               .arg(changed.join(", ")));
    }
    return ResultOk;
}

/*!
    Applies \a edit from \a client. An edit that refers to change annotations
    is first listed in the search results, grouped by annotation, with \a title
    as its search term, and only the results the user leaves checked are
    applied when they press Replace. Changes whose annotation asks for
    confirmation start unchecked. Nothing is applied if no result is checked
    or if a document the checked results edit changed in the meantime.

    \a done is called once with whether the changes were applied, which is
    false as well if the search results go away or the server of \a client
    finishes before the user applied them.
*/
void applyWorkspaceEditWithPreview(
    Client *client,
    const WorkspaceEdit &edit,
    const QString &title,
    const std::function<void(bool applied)> &done)
{
    QTC_ASSERT(client, if (done) done(false); return);
    if (!hasChangeAnnotations(edit)) {
        const bool applied = applyWorkspaceEdit(client, edit);
        if (done)
            done(applied);
        return;
    }

    Core::SearchResult *search = Core::SearchResultWindow::instance()->startNewSearch(
        Tr::tr("Changes proposed by %1:").arg(client->name()),
        {},
        title,
        Core::SearchResultWindow::SearchAndReplace,
        Core::SearchResultWindow::PreserveCaseDisabled);
    search->setReplaceTextVisible(false);
    search->addResults(
        workspaceEditPreviewItems(
            edit, [client](const QString &uri) { return client->filePathFor(uri); }),
        Core::SearchResult::AddOrdered);
    search->setReplaceEnabled(true);
    search->finishSearch(false);

    const auto pending = std::make_shared<std::function<void(bool)>>(done);
    const auto report = [pending](bool applied) {
        if (const std::function<void(bool)> callback = std::exchange(*pending, nullptr))
            callback(applied);
    };
    QObject::connect(search, &Core::SearchResult::activated, search, [](const SearchResultItem &item) {
        Core::EditorManager::openEditorAtSearchResult(item);
    });
    const auto filePathFor = [client = QPointer(client)](const QString &uri) {
        return client ? client->filePathFor(uri) : FilePath();
    };
    const auto clientGone = [search, report, clientName = client->name()] {
        search->finishSearch(true, Tr::tr("%1 is not reachable anymore.").arg(clientName));
        search->setReplaceEnabled(false);
        report(false);
    };
    const QList<QMetaObject::Connection> clientConnections{
        QObject::connect(client, &QObject::destroyed, search, clientGone),
        QObject::connect(client, &Client::finished, search, clientGone)};
    QObject::connect(
        search,
        &Core::SearchResult::replaceButtonClicked,
        search,
        [client = QPointer(client),
         search,
         edit,
         snapshots = documentSnapshots(edit, filePathFor, &SymbolSupport::getFileContents),
         filePathFor,
         clientConnections,
         report](const QString &, const SearchResultItems &checkedItems, bool) {
            for (const QMetaObject::Connection &connection : clientConnections)
                QObject::disconnect(connection);
            if (!client) {
                report(false);
                return;
            }
            const WorkspaceEdit selected = selectedWorkspaceEdit(edit, checkedItems);
            const Result<> applicable = canApplySelectedEdit(
                selected, snapshots, filePathFor, &SymbolSupport::getFileContents);
            if (!applicable) {
                search->finishSearch(true, applicable.error());
                report(false);
                return;
            }
            const bool applied = applyWorkspaceEdit(client, selected);
            if (!applied)
                search->finishSearch(true, Tr::tr("Some of the changes could not be applied."));
            report(applied);
        });
    QObject::connect(search, &QObject::destroyed, [report] { report(false); });
    search->popup();
}

} // namespace LanguageClient

#ifdef WITH_TESTS

namespace LanguageClient {

static AnnotatedTextEdit annotated(const QString &newText, const QString &annotationId)
{
    return AnnotatedTextEdit().newText(newText).annotationId(annotationId);
}

static TextDocumentEdit documentEdit(
    const QString &uri, const QList<TextDocumentEditEditsItem> &edits)
{
    return TextDocumentEdit()
        .textDocument(OptionalVersionedTextDocumentIdentifier().uri(uri))
        .edits(edits);
}

/// An edit as a server sends it, so that it also goes through parsing.
static WorkspaceEdit annotatedEdit()
{
    const WorkspaceEdit edit
        = WorkspaceEdit()
              .addDocumentChange(documentEdit(
                  "file:///a.cpp",
                  {TextEdit().newText("plain"),
                   annotated("confirm", "confirm"),
                   annotated("noConfirm", "noConfirm"),
                   annotated("unknown", "unknown")}))
              .addDocumentChange(documentEdit("file:///b.cpp", {annotated("confirm", "confirm")}))
              .addDocumentChange(
                  LanguageServerProtocol::DeleteFile().uri("file:///c.cpp").annotationId("delete"))
              .addDocumentChange(
                  LanguageServerProtocol::CreateFile().uri("file:///d.cpp").annotationId("noConfirm"))
              .addChangeAnnotation(
                  "confirm", ChangeAnnotation().label("Confirm").needsConfirmation(true))
              .addChangeAnnotation(
                  "noConfirm", ChangeAnnotation().label("No confirmation").needsConfirmation(false))
              .addChangeAnnotation(
                  "delete", ChangeAnnotation().label("Delete").needsConfirmation(true));
    const Utils::Result<WorkspaceEdit> parsed = fromJson<WorkspaceEdit>(toJson(edit));
    QTC_ASSERT(parsed, return {});
    return *parsed;
}

static FilePath filePathFor(const QString &uri)
{
    return FilePath::fromUserInput(uri.mid(QString("file://").size()));
}

static QString fileName(const QString &uri)
{
    return filePathFor(uri).toUserOutput();
}

class NullInterface final : public BaseClientInterface
{
    FilePath serverDeviceTemplate() const final { return {}; }
    void sendData(const QByteArray &) final {}
};

/// Answers initialize and shutdown, and records the responses the client sends.
class RecordingServerInterface final : public BaseClientInterface
{
public:
    void request(int id, const QString &method, const QJsonObject &params)
    {
        emit messageReceived(
            QJsonObject{{"jsonrpc", "2.0"}, {"id", id}, {"method", method}, {"params", params}});
    }

    QList<QJsonObject> responses(int id) const
    {
        return Utils::filtered(m_sent, [id](const QJsonObject &message) {
            return !message.contains("method") && message.value("id") == QJsonValue(id);
        });
    }

private:
    FilePath serverDeviceTemplate() const final { return {}; }

    void sendData(const QByteArray &data) final
    {
        m_received += data;
        while (true) {
            const qsizetype headerEnd = m_received.indexOf("\r\n\r\n");
            if (headerEnd < 0)
                return;
            static const QRegularExpression contentLength("Content-Length: (\\d+)");
            const QRegularExpressionMatch match = contentLength.match(
                QString::fromLatin1(m_received.left(headerEnd)));
            const qsizetype bodyStart = headerEnd + 4;
            const qsizetype bodyEnd = bodyStart + match.captured(1).toInt();
            if (!match.hasMatch() || m_received.size() < bodyEnd)
                return;
            const QJsonObject message
                = QJsonDocument::fromJson(m_received.mid(bodyStart, bodyEnd - bodyStart)).object();
            m_received.remove(0, bodyEnd);
            m_sent << message;
            const QString method = message.value("method").toString();
            if (method == "initialize")
                reply(message.value("id"), QJsonObject{{"capabilities", QJsonObject()}});
            else if (method == "shutdown")
                reply(message.value("id"), QJsonValue::Null);
            else if (method == "exit")
                QTimer::singleShot(0, this, [this] { emit finished(); });
        }
    }

    void reply(const QJsonValue &id, const QJsonValue &result)
    {
        const QJsonObject response{{"jsonrpc", "2.0"}, {"id", id}, {"result", result}};
        QTimer::singleShot(0, this, [this, response] { emit messageReceived(response); });
    }

    QByteArray m_received;
    QList<QJsonObject> m_sent;
};

class WorkspaceEditPreviewTest final : public QObject
{
    Q_OBJECT

private slots:
    void testHasChangeAnnotations()
    {
        QVERIFY(hasChangeAnnotations(annotatedEdit()));
        QVERIFY(!hasChangeAnnotations(
            WorkspaceEdit().addDocumentChange(documentEdit("file:///a.cpp", {TextEdit()}))));
        QVERIFY(!hasChangeAnnotations(WorkspaceEdit().addDocumentChange(
            documentEdit("file:///a.cpp", {annotated("x", "unknown")}))));
    }

    void testPreviewItems()
    {
        const SearchResultItems items = workspaceEditPreviewItems(annotatedEdit(), &filePathFor);
        const QList<QStringList> paths = Utils::transform(items, &SearchResultItem::path);
        QCOMPARE(paths,
                 QList<QStringList>(
                     {{fileName("file:///a.cpp")},
                      {"Confirm", fileName("file:///a.cpp")},
                      {"No confirmation", fileName("file:///a.cpp")},
                      {fileName("file:///a.cpp")},
                      {"Confirm", fileName("file:///b.cpp")},
                      {"Delete", fileName("file:///c.cpp")},
                      {"No confirmation", fileName("file:///d.cpp")}}));
        const QList<bool> selected = Utils::transform(items, &SearchResultItem::selectForReplacement);
        QCOMPARE(selected, QList<bool>({true, false, true, true, false, false, true}));
        QCOMPARE(items.at(1).lineText(), QString("confirm"));
        QCOMPARE(items.at(1).filePath(), filePathFor("file:///a.cpp"));
    }

    void testSelectedWorkspaceEdit()
    {
        const WorkspaceEdit edit = annotatedEdit();
        const SearchResultItems items = workspaceEditPreviewItems(edit, &filePathFor);
        QCOMPARE(selectedWorkspaceEdit(edit, items), edit);

        const SearchResultItems checked
            = Utils::filtered(items, &SearchResultItem::selectForReplacement);
        const WorkspaceEdit selected = selectedWorkspaceEdit(edit, checked);
        QVERIFY(selected.documentChanges());
        const QList<WorkspaceEditDocumentChangesItem> changes = *selected.documentChanges();
        QCOMPARE(changes.size(), 2);
        const auto documentEdit = std::get_if<TextDocumentEdit>(&changes.at(0));
        QVERIFY(documentEdit);
        QCOMPARE(documentEdit->textDocument().uri(), QString("file:///a.cpp"));
        const QStringList newTexts = Utils::transform(
            plainTextEdits(*documentEdit), [](const TextEdit &edit) { return edit.newText(); });
        QCOMPARE(newTexts, QStringList({"plain", "noConfirm", "unknown"}));
        QVERIFY(std::holds_alternative<LanguageServerProtocol::CreateFile>(changes.at(1)));
        QCOMPARE(selected.changeAnnotations(), edit.changeAnnotations());
    }

    void testSelectedWorkspaceEditDropsFallbackChanges()
    {
        const WorkspaceEdit edit = annotatedEdit().addChange(
            "file:///a.cpp", {TextEdit().newText("fallback")});
        const WorkspaceEdit selected = selectedWorkspaceEdit(edit, {});
        QVERIFY(!selected.changes());
        QVERIFY(selected.documentChanges());
        QVERIFY(selected.documentChanges()->isEmpty());
    }

    void testCanApplySelectedEdit()
    {
        const WorkspaceEdit edit = annotatedEdit();
        QHash<FilePath, QStringList> contents{{filePathFor("file:///a.cpp"), {"a"}},
                                              {filePathFor("file:///b.cpp"), {"b"}}};
        const auto linesFor = [&contents](const FilePath &filePath) {
            return contents.value(filePath);
        };
        const QHash<FilePath, size_t> previewed = documentSnapshots(edit, &filePathFor, linesFor);
        QCOMPARE(previewed.keys().size(), 2);

        const SearchResultItems items = workspaceEditPreviewItems(edit, &filePathFor);
        const SearchResultItems onlyA = items.mid(0, 1);
        QVERIFY(canApplySelectedEdit(
            selectedWorkspaceEdit(edit, onlyA), previewed, &filePathFor, linesFor));
        QVERIFY(!canApplySelectedEdit(
            selectedWorkspaceEdit(edit, {}), previewed, &filePathFor, linesFor));

        contents[filePathFor("file:///b.cpp")] = {"b", "changed"};
        QVERIFY(canApplySelectedEdit(
            selectedWorkspaceEdit(edit, onlyA), previewed, &filePathFor, linesFor));
        const Result<> stale = canApplySelectedEdit(
            selectedWorkspaceEdit(edit, items), previewed, &filePathFor, linesFor);
        QVERIFY(!stale);
        QVERIFY(stale.error().contains(fileName("file:///b.cpp")));
        QVERIFY(!stale.error().contains(fileName("file:///a.cpp")));
    }

    void testCanApplySelectedEditToFileChangedOnDisk()
    {
        const TemporaryDirectory dir("workspaceeditpreview");
        const FilePath file = dir.filePath("a.cpp");
        QVERIFY(file.writeFileContents("int a;\n"));
        const WorkspaceEdit edit = WorkspaceEdit().addDocumentChange(
            documentEdit("file://" + file.path(), {annotated("b", "confirm")}));
        const QHash<FilePath, size_t> previewed
            = documentSnapshots(edit, &filePathFor, &SymbolSupport::getFileContents);
        QVERIFY(canApplySelectedEdit(edit, previewed, &filePathFor, &SymbolSupport::getFileContents));

        QVERIFY(file.writeFileContents("int b;\n"));
        QVERIFY(!canApplySelectedEdit(edit, previewed, &filePathFor, &SymbolSupport::getFileContents));
    }

    void testApplyWorkspaceEditFailsIfAnyChangeFails()
    {
        Client client(new NullInterface);
        const TemporaryDirectory dir("workspaceeditpreview");
        const FilePath created = dir.filePath("created.cpp");
        const WorkspaceEdit edit
            = WorkspaceEdit()
                  .addDocumentChange(LanguageServerProtocol::CreateFile().uri(client.uriFor(created)))
                  .addDocumentChange(RenameFile()
                                         .oldUri(client.uriFor(dir.filePath("missing.cpp")))
                                         .newUri(client.uriFor(dir.filePath("renamed.cpp"))));
        QVERIFY(!applyWorkspaceEdit(&client, edit));
        QVERIFY(created.exists());
    }

    void testApplyEditRequestIsAnsweredOnceThePreviewIsGone()
    {
        auto server = new RecordingServerInterface;
        QPointer<Client> client = new Client(server);
        client->start();
        QTRY_COMPARE(client->state(), Client::Initialized);

        const TemporaryDirectory dir("workspaceeditpreview");
        const FilePath created = dir.filePath("created.cpp");
        const WorkspaceEdit edit
            = WorkspaceEdit()
                  .addDocumentChange(LanguageServerProtocol::CreateFile()
                                         .uri(client->uriFor(created))
                                         .annotationId("confirm"))
                  .addChangeAnnotation(
                      "confirm", ChangeAnnotation().label("Confirm").needsConfirmation(true));
        server->request(7, ApplyWorkspaceEditRequest::method, QJsonObject{{"edit", toJson(edit)}});
        server->request(8, WorkspaceFoldersRequest::method, {});
        QTRY_COMPARE(server->responses(8).size(), 1);
        QVERIFY(server->responses(7).isEmpty());

        Core::SearchResultWindow::instance()->clearContents();
        QTRY_COMPARE(server->responses(7).size(), 1);
        const QJsonObject result = server->responses(7).first().value("result").toObject();
        QCOMPARE(result.value("applied").toBool(true), false);
        QVERIFY(!result.value("failureReason").toString().isEmpty());
        QVERIFY(!created.exists());

        server->request(9, WorkspaceFoldersRequest::method, {});
        QTRY_COMPARE(server->responses(9).size(), 1);
        QCOMPARE(server->responses(7).size(), 1);

        LanguageClientManager::shutdownClient(client);
        QTRY_VERIFY(client.isNull());
    }

    void testApplyEditRequestIsNotAnsweredAfterARestart()
    {
        auto server = new RecordingServerInterface;
        QPointer<Client> client = new Client(server);
        client->start();
        QTRY_COMPARE(client->state(), Client::Initialized);

        const TemporaryDirectory dir("workspaceeditpreview");
        const WorkspaceEdit edit
            = WorkspaceEdit()
                  .addDocumentChange(LanguageServerProtocol::CreateFile()
                                         .uri(client->uriFor(dir.filePath("created.cpp")))
                                         .annotationId("confirm"))
                  .addChangeAnnotation(
                      "confirm", ChangeAnnotation().label("Confirm").needsConfirmation(true));
        server->request(7, ApplyWorkspaceEditRequest::method, QJsonObject{{"edit", toJson(edit)}});
        server->request(8, WorkspaceFoldersRequest::method, {});
        QTRY_COMPARE(server->responses(8).size(), 1);

        QSignalSpy initialized(client.get(), &Client::initialized);
        LanguageClientManager::restartClient(client);
        QTRY_COMPARE(initialized.size(), 1);
        QCOMPARE(client->state(), Client::Initialized);

        Core::SearchResultWindow::instance()->clearContents();
        server->request(9, WorkspaceFoldersRequest::method, {});
        QTRY_COMPARE(server->responses(9).size(), 1);
        QVERIFY(server->responses(7).isEmpty());

        LanguageClientManager::shutdownClient(client);
        QTRY_VERIFY(client.isNull());
    }

    void testPreviewIsAnsweredWhenTheClientGoes()
    {
        Core::SearchResultWindow::instance()->clearContents();
        auto client = new Client(new NullInterface);
        const auto applied = std::make_shared<std::optional<bool>>();
        applyWorkspaceEditWithPreview(
            client, annotatedEdit(), "title", [applied](bool result) { *applied = result; });
        QVERIFY(!*applied);
        const QList<QToolButton *> replaceButtons = Utils::filtered(
            Core::SearchResultWindow::instance()->outputWidget(nullptr)->findChildren<QToolButton *>(),
            [](const QToolButton *button) { return button->text() == "&Replace"; });
        QCOMPARE(replaceButtons.size(), 1);
        QVERIFY(replaceButtons.first()->isEnabled());

        delete client;
        QVERIFY(!replaceButtons.first()->isEnabled());
        const std::optional<bool> answer = *applied;
        Core::SearchResultWindow::instance()->clearContents();
        QCOMPARE(answer, std::optional<bool>(false));
    }
};

QObject *createWorkspaceEditPreviewTest()
{
    return new WorkspaceEditPreviewTest;
}

} // namespace LanguageClient

#include "workspaceeditpreview.moc"

#endif
