// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmllsmovecomponent.h"

#include "qmljscomponentfromobjectdef.h"
#include "qmljscomponentnamedialog.h"
#include "qmljseditortr.h"

#include <coreplugin/icore.h>
#include <coreplugin/messagemanager.h>

#include <languageclient/client.h>
#include <languageclient/languageclientutils.h>

#include <qmljs/qmljsmodelmanagerinterface.h>

#include <qmljstools/qmljsrefactoringchanges.h>

#include <utils/changeset.h>
#include <utils/filepath.h>
#include <utils/qtcassert.h>

#include <QPointer>
#include <QSet>
#include <QTextDocument>
#include <QUrl>

#ifdef WITH_TESTS
#include <QTest>
#endif

using namespace LanguageServerProtocol;
using namespace Utils;

namespace QmlJSEditor::Internal {

struct MovableProperty
{
    ChangeAnnotationIdentifier annotationId;
    QString label;
    QString code;
};

static QList<WorkspaceEditDocumentChangesItem> documentChanges(const WorkspaceEdit &edit)
{
    return edit.documentChanges().value_or(QList<WorkspaceEditDocumentChangesItem>());
}

/// The uri of the file \a edit creates for the component.
static std::optional<QString> componentUri(const WorkspaceEdit &edit)
{
    for (const WorkspaceEditDocumentChangesItem &change : documentChanges(edit)) {
        const auto create = std::get_if<LanguageServerProtocol::CreateFile>(&change);
        if (create && create->uri().endsWith(".qml"))
            return create->uri();
    }
    return std::nullopt;
}

static QString componentName(const QString &uri)
{
    const QString fileName = QUrl(uri).fileName();
    return fileName.left(fileName.indexOf('.'));
}

/*!
    Whether \a action is qmlls' "Move Component into Separate File": it extracts
    code into a QML file it creates.
*/
bool isMoveComponentAction(const CodeAction &action)
{
    return action.kind().value_or(CodeActionKind()) == CodeActionKinds::RefactorExtract
           && action.edit() && componentUri(*action.edit());
}

/// The properties the user may keep in place. The server annotates the edits
/// moving a property with the same change annotation in both files.
static QList<MovableProperty> movableProperties(const WorkspaceEdit &edit, const QString &uri)
{
    QList<MovableProperty> properties;
    QSet<ChangeAnnotationIdentifier> seen;
    for (const WorkspaceEditDocumentChangesItem &change : documentChanges(edit)) {
        const auto documentEdit = std::get_if<TextDocumentEdit>(&change);
        if (!documentEdit || documentEdit->textDocument().uri() != uri)
            continue;
        for (const TextDocumentEditEditsItem &item : documentEdit->edits()) {
            const std::optional<ChangeAnnotationIdentifier> id = LanguageClient::annotationId(item);
            const std::optional<TextEdit> textEdit = LanguageClient::plainTextEdit(item);
            if (!id || !textEdit || seen.contains(*id))
                continue;
            seen.insert(*id);
            const std::optional<ChangeAnnotation> annotation
                = LanguageClient::changeAnnotation(edit, id);
            const QString label = annotation && !annotation->label().isEmpty()
                                      ? annotation->label()
                                      : *id;
            properties.append({*id, label, textEdit->newText().trimmed()});
        }
    }
    return properties;
}

/// \a edit with the component moved to \a uri and named \a name instead of
/// what the server proposed.
static WorkspaceEdit movedComponent(
    const WorkspaceEdit &edit, const QString &uri, const QString &name)
{
    const std::optional<QString> proposedUri = componentUri(edit);
    QTC_ASSERT(proposedUri, return edit);
    const QString proposedName = componentName(*proposedUri);

    QList<WorkspaceEditDocumentChangesItem> changes;
    for (WorkspaceEditDocumentChangesItem change : documentChanges(edit)) {
        if (const auto create = std::get_if<LanguageServerProtocol::CreateFile>(&change)) {
            if (create->uri() == *proposedUri)
                create->uri(uri);
        } else if (const auto documentEdit = std::get_if<TextDocumentEdit>(&change)) {
            OptionalVersionedTextDocumentIdentifier document = documentEdit->textDocument();
            if (document.uri() == *proposedUri) {
                document.uri(uri);
                documentEdit->textDocument(document);
            } else {
                QList<TextDocumentEditEditsItem> edits;
                for (TextDocumentEditEditsItem item : documentEdit->edits()) {
                    // The use of the component in the original file.
                    const auto textEdit = std::get_if<TextEdit>(&item);
                    if (textEdit && textEdit->newText() == proposedName)
                        textEdit->newText(name);
                    edits << item;
                }
                documentEdit->edits(edits);
            }
        }
        changes << change;
    }

    WorkspaceEdit result = edit;
    result.documentChanges(changes);
    return result;
}

/*!
    \a edit without the edits annotated with \a keptProperties, which leaves
    those properties in place.

    The result carries no change annotations, as the user decided on them.
*/
static WorkspaceEdit withoutKeptProperties(
    const WorkspaceEdit &edit, const QSet<ChangeAnnotationIdentifier> &keptProperties)
{
    QList<WorkspaceEditDocumentChangesItem> changes;
    for (WorkspaceEditDocumentChangesItem change : documentChanges(edit)) {
        if (const auto documentEdit = std::get_if<TextDocumentEdit>(&change)) {
            QList<TextDocumentEditEditsItem> edits;
            for (const TextDocumentEditEditsItem &item : documentEdit->edits()) {
                const std::optional<ChangeAnnotationIdentifier> id
                    = LanguageClient::annotationId(item);
                if (id && keptProperties.contains(*id))
                    continue;
                if (const std::optional<TextEdit> textEdit = LanguageClient::plainTextEdit(item))
                    edits << *textEdit;
                else
                    edits << item;
            }
            documentEdit->edits(edits);
        }
        changes << change;
    }

    WorkspaceEdit result = edit;
    result.documentChanges(changes);
    result.changeAnnotations(std::nullopt);
    return result;
}

/*!
    \a edit with the component moved to \a uri and named \a name instead of
    what the server proposed, leaving the properties annotated with
    \a keptProperties in place.
*/
static WorkspaceEdit adjustedEdit(
    const WorkspaceEdit &edit,
    const QString &uri,
    const QString &name,
    const QSet<ChangeAnnotationIdentifier> &keptProperties)
{
    return withoutKeptProperties(movedComponent(edit, uri, name), keptProperties);
}

/*!
    Removes the creation of the component file \a uri and its contents from
    \a edit and returns the contents.
*/
static QString takeComponentFile(WorkspaceEdit *edit, const QString &uri)
{
    QTextDocument document;
    QList<WorkspaceEditDocumentChangesItem> changes;
    for (const WorkspaceEditDocumentChangesItem &change : documentChanges(*edit)) {
        if (const auto create = std::get_if<LanguageServerProtocol::CreateFile>(&change)) {
            if (create->uri() == uri)
                continue;
        } else if (const auto documentEdit = std::get_if<TextDocumentEdit>(&change)) {
            if (documentEdit->textDocument().uri() == uri) {
                LanguageClient::editsToChangeSet(
                    LanguageClient::plainTextEdits(*documentEdit), &document)
                    .apply(&document);
                continue;
            }
        }
        changes << change;
    }
    edit->documentChanges(changes);
    return document.toPlainText();
}

class MoveComponentOperation final : public TextEditor::QuickFixOperation
{
public:
    MoveComponentOperation(
        const WorkspaceEdit &edit, LanguageClient::Client *client, const FilePath &filePath)
        : m_edit(edit)
        , m_client(client)
        , m_filePath(filePath)
    {
        setDescription(Tr::tr("Move Component into Separate File"));
    }

    void perform() final
    {
        if (!m_client)
            return;
        const std::optional<QString> proposedUri = componentUri(m_edit);
        QTC_ASSERT(proposedUri, return);

        const QList<MovableProperty> properties = movableProperties(m_edit, *proposedUri);
        QStringList labels;
        QStringList sourcePreview{QString()};
        for (const MovableProperty &property : properties) {
            labels << property.label;
            sourcePreview << "    " + property.code;
        }

        QString name = componentName(*proposedUri);
        QString path = m_client->filePathFor(*proposedUri).parentDir().toUserOutput();
        QString suffix;
        QStringList keptLabels;
        if (!ComponentNameDialog::go(
                &name,
                &path,
                &suffix,
                labels,
                sourcePreview,
                m_filePath.fileName(),
                &keptLabels,
                Core::ICore::dialogParent())) {
            return;
        }

        QSet<ChangeAnnotationIdentifier> keptProperties;
        for (const MovableProperty &property : properties) {
            if (keptLabels.contains(property.label))
                keptProperties.insert(property.annotationId);
        }

        const FilePath newFile = FilePath::fromUserInput(path).pathAppended(name + '.' + suffix);
        const QString uri = m_client->uriFor(newFile);
        WorkspaceEdit edit = adjustedEdit(m_edit, uri, name, keptProperties);
        const QString contents = takeComponentFile(&edit, uri);

        QmlJSTools::QmlJSRefactoringChanges refactoring(
            QmlJS::ModelManagerInterface::instance(),
            QmlJS::ModelManagerInterface::instance()->snapshot());
        const bool reindent = true;
        const bool openEditor = false;
        if (!refactoring.file(newFile)->create(contents, reindent, openEditor)) {
            Core::MessageManager::writeDisrupting(
                Tr::tr("Could not create \"%1\".").arg(newFile.toUserOutput()));
            return;
        }
        if (!LanguageClient::applyWorkspaceEdit(m_client, edit)) {
            Core::MessageManager::writeDisrupting(
                Tr::tr("Could not replace the component in \"%1\".").arg(m_filePath.toUserOutput()));
        }
        addComponentFile(m_filePath, newFile);
    }

private:
    const WorkspaceEdit m_edit;
    const QPointer<LanguageClient::Client> m_client;
    const FilePath m_filePath;
};

/*!
    Lets the user name and place the component \a action moves out of
    \a filePath and choose the properties that stay with its use before
    applying it.
*/
TextEditor::QuickFixOperation *createMoveComponentOperation(
    const CodeAction &action, LanguageClient::Client *client, const FilePath &filePath)
{
    QTC_ASSERT(action.edit(), return nullptr);
    return new MoveComponentOperation(*action.edit(), client, filePath);
}

#ifdef WITH_TESTS

static const char sourceUri[] = "file:///project/Main.qml";
static const char proposedUri[] = "file:///project/MyBox.qml";

static Range range(int startLine, int startCharacter, int endLine, int endCharacter)
{
    return Range{Position{startLine, startCharacter}, Position{endLine, endCharacter}};
}

static TextEdit insertion(const QString &text)
{
    return TextEdit().range(range(0, 0, 0, 0)).newText(text);
}

static AnnotatedTextEdit annotated(const TextEdit &edit, const QString &annotationId)
{
    return AnnotatedTextEdit()
        .range(edit.range())
        .newText(edit.newText())
        .annotationId(annotationId);
}

static TextDocumentEdit documentEdit(const QString &uri, const QList<TextDocumentEditEditsItem> &edits)
{
    return TextDocumentEdit()
        .textDocument(OptionalVersionedTextDocumentIdentifier().uri(uri))
        .edits(edits);
}

/// What qmlls proposes for moving a Rectangle with the properties x and color.
static WorkspaceEdit proposedEdit()
{
    const TextEdit removeX = TextEdit().range(range(8, 8, 8, 13)).newText({});
    const TextEdit removeColor
        = TextEdit().range(range(9, 8, 9, 20)).newText({});
    return WorkspaceEdit()
        .addDocumentChange(LanguageServerProtocol::CreateFile().uri(proposedUri))
        .addDocumentChange(documentEdit(
            proposedUri,
            {insertion("import QtQuick\n\n"),
             insertion("Rectangle {\n"),
             annotated(insertion("x: 10\n"), "x"),
             annotated(insertion("color: \"red\"\n"), "color"),
             insertion("}\n")}))
        .addDocumentChange(documentEdit(
            sourceUri,
            {TextEdit().range(range(6, 4, 6, 13)).newText("MyBox"),
             annotated(removeX, "x"),
             annotated(removeColor, "color")}))
        .addChangeAnnotation("x", ChangeAnnotation().label("x").needsConfirmation(true))
        .addChangeAnnotation("color", ChangeAnnotation().label("Color").needsConfirmation(true));
}

static QStringList newTexts(const WorkspaceEditDocumentChangesItem &change)
{
    const auto documentEdit = std::get_if<TextDocumentEdit>(&change);
    if (!documentEdit)
        return {};
    QStringList texts;
    for (const TextDocumentEditEditsItem &item : documentEdit->edits()) {
        if (std::holds_alternative<AnnotatedTextEdit>(item))
            texts << "annotated";
        else if (const std::optional<TextEdit> textEdit = LanguageClient::plainTextEdit(item))
            texts << textEdit->newText();
    }
    return texts;
}

class QmllsMoveComponentTest final : public QObject
{
    Q_OBJECT

private slots:
    void testIsMoveComponentAction()
    {
        const CodeAction action = CodeAction()
                                      .title("Move Component into Separate File")
                                      .kind(QString(CodeActionKinds::RefactorExtract))
                                      .edit(proposedEdit());
        QVERIFY(isMoveComponentAction(action));

        CodeAction rewrite = action;
        rewrite.kind(QString(CodeActionKinds::RefactorRewrite));
        QVERIFY(!isMoveComponentAction(rewrite));

        CodeAction withoutNewFile = action;
        withoutNewFile.edit(
            WorkspaceEdit().addDocumentChange(documentEdit(sourceUri, {insertion("x")})));
        QVERIFY(!isMoveComponentAction(withoutNewFile));
    }

    void testMovableProperties()
    {
        const QList<MovableProperty> properties = movableProperties(proposedEdit(), proposedUri);
        QCOMPARE(properties.size(), 2);
        QCOMPARE(properties.at(0).annotationId, QString("x"));
        QCOMPARE(properties.at(0).label, QString("x"));
        QCOMPARE(properties.at(0).code, QString("x: 10"));
        QCOMPARE(properties.at(1).annotationId, QString("color"));
        QCOMPARE(properties.at(1).label, QString("Color"));
        QCOMPARE(properties.at(1).code, QString("color: \"red\""));
    }

    void testAdjustedEdit()
    {
        const QString uri = "file:///project/parts/Box.qml";
        const WorkspaceEdit edit = adjustedEdit(proposedEdit(), uri, "Box", {"x"});

        QVERIFY(!edit.changeAnnotations());
        const QList<WorkspaceEditDocumentChangesItem> changes = documentChanges(edit);
        QCOMPARE(changes.size(), 3);

        const auto create = std::get_if<LanguageServerProtocol::CreateFile>(&changes.at(0));
        QVERIFY(create);
        QCOMPARE(create->uri(), uri);

        const auto component = std::get_if<TextDocumentEdit>(&changes.at(1));
        QVERIFY(component);
        QCOMPARE(component->textDocument().uri(), uri);
        QCOMPARE(
            newTexts(changes.at(1)),
            QStringList({"import QtQuick\n\n", "Rectangle {\n", "color: \"red\"\n", "}\n"}));

        const auto source = std::get_if<TextDocumentEdit>(&changes.at(2));
        QVERIFY(source);
        QCOMPARE(source->textDocument().uri(), QString(sourceUri));
        QCOMPARE(newTexts(changes.at(2)), QStringList({"Box", QString()}));
    }

    void testTakeComponentFile()
    {
        const QString uri = "file:///project/Box.qml";
        WorkspaceEdit edit = adjustedEdit(proposedEdit(), uri, "Box", {"x"});

        QCOMPARE(
            takeComponentFile(&edit, uri),
            QString("import QtQuick\n\nRectangle {\ncolor: \"red\"\n}\n"));

        const QList<WorkspaceEditDocumentChangesItem> changes = documentChanges(edit);
        QCOMPARE(changes.size(), 1);
        const auto source = std::get_if<TextDocumentEdit>(&changes.at(0));
        QVERIFY(source);
        QCOMPARE(source->textDocument().uri(), QString(sourceUri));
    }
};

QObject *createQmllsMoveComponentTest()
{
    return new QmllsMoveComponentTest;
}

#endif // WITH_TESTS

} // namespace QmlJSEditor::Internal

#ifdef WITH_TESTS
#include "qmllsmovecomponent.moc"
#endif
