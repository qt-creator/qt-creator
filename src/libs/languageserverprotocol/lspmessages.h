/*
 This file is auto-generated. Do not edit manually.
 Generated from the language server protocol meta model 3.18.0 with:

 python3 scripts/lsp_metamodel_to_json_schema.py src/libs/languageserverprotocol/metaModel.json src/libs/languageserverprotocol/lsp.schema.json --messages-output src/libs/languageserverprotocol/lspmessages.h
*/
#pragma once

#include "lsptypes.h"

namespace LanguageServerProtocol {

enum class MessageDirection { ClientToServer, ServerToClient, Both };

/**
 * A request to resolve the implementation locations of a symbol at a given text
 * document position. The request's parameter is of type {@link TextDocumentPositionParams}
 * the response is of type {@link Definition} or a Thenable that resolves to such.
 */
struct ImplementationRequest {
    static constexpr char method[] = "textDocument/implementation";
    static constexpr MessageDirection direction = MessageDirection::ClientToServer;
    static constexpr bool isRequest = true;
    using Params = ImplementationParams;
    using Result = ImplementationRequestResult;
    using PartialResult = ImplementationRequestPartialResult;
    using RegistrationOptions = ImplementationRegistrationOptions;
    using ErrorData = std::monostate;
};

/**
 * A request to resolve the type definition locations of a symbol at a given text
 * document position. The request's parameter is of type {@link TextDocumentPositionParams}
 * the response is of type {@link Definition} or a Thenable that resolves to such.
 */
struct TypeDefinitionRequest {
    static constexpr char method[] = "textDocument/typeDefinition";
    static constexpr MessageDirection direction = MessageDirection::ClientToServer;
    static constexpr bool isRequest = true;
    using Params = TypeDefinitionParams;
    using Result = TypeDefinitionRequestResult;
    using PartialResult = TypeDefinitionRequestPartialResult;
    using RegistrationOptions = TypeDefinitionRegistrationOptions;
    using ErrorData = std::monostate;
};

/**
 * The `workspace/workspaceFolders` is sent from the server to the client to fetch the open workspace folders.
 */
struct WorkspaceFoldersRequest {
    static constexpr char method[] = "workspace/workspaceFolders";
    static constexpr MessageDirection direction = MessageDirection::ServerToClient;
    static constexpr bool isRequest = true;
    using Params = std::monostate;
    using Result = WorkspaceFoldersRequestResult;
    using PartialResult = std::monostate;
    using RegistrationOptions = std::monostate;
    using ErrorData = std::monostate;
};

/**
 * The 'workspace/configuration' request is sent from the server to the client to fetch a certain
 * configuration setting.
 *
 * This pull model replaces the old push model were the client signaled configuration change via an
 * event. If the server still needs to react to configuration changes (since the server caches the
 * result of `workspace/configuration` requests) the server should register for an empty configuration
 * change event and empty the cache if such an event is received.
 */
struct ConfigurationRequest {
    static constexpr char method[] = "workspace/configuration";
    static constexpr MessageDirection direction = MessageDirection::ServerToClient;
    static constexpr bool isRequest = true;
    using Params = ConfigurationParams;
    using Result = ConfigurationRequestResult;
    using PartialResult = std::monostate;
    using RegistrationOptions = std::monostate;
    using ErrorData = std::monostate;
};

/**
 * A request to list all color symbols found in a given text document. The request's
 * parameter is of type {@link DocumentColorParams} the
 * response is of type {@link ColorInformation ColorInformation[]} or a Thenable
 * that resolves to such.
 */
struct DocumentColorRequest {
    static constexpr char method[] = "textDocument/documentColor";
    static constexpr MessageDirection direction = MessageDirection::ClientToServer;
    static constexpr bool isRequest = true;
    using Params = DocumentColorParams;
    using Result = DocumentColorRequestResult;
    using PartialResult = DocumentColorRequestPartialResult;
    using RegistrationOptions = DocumentColorRegistrationOptions;
    using ErrorData = std::monostate;
};

/**
 * A request to list all presentation for a color. The request's
 * parameter is of type {@link ColorPresentationParams} the
 * response is of type {@link ColorPresentation ColorPresentation[]} or a Thenable
 * that resolves to such.
 */
struct ColorPresentationRequest {
    static constexpr char method[] = "textDocument/colorPresentation";
    static constexpr MessageDirection direction = MessageDirection::ClientToServer;
    static constexpr bool isRequest = true;
    using Params = ColorPresentationParams;
    using Result = ColorPresentationRequestResult;
    using PartialResult = ColorPresentationRequestPartialResult;
    using RegistrationOptions = DocumentColorRegistrationOptions;
    using ErrorData = std::monostate;
};

/**
 * A request to provide folding ranges in a document. The request's
 * parameter is of type {@link FoldingRangeParams}, the
 * response is of type {@link FoldingRangeList} or a Thenable
 * that resolves to such.
 */
struct FoldingRangeRequest {
    static constexpr char method[] = "textDocument/foldingRange";
    static constexpr MessageDirection direction = MessageDirection::ClientToServer;
    static constexpr bool isRequest = true;
    using Params = FoldingRangeParams;
    using Result = FoldingRangeRequestResult;
    using PartialResult = FoldingRangeRequestPartialResult;
    using RegistrationOptions = FoldingRangeRegistrationOptions;
    using ErrorData = std::monostate;
};

/**
 * A request to refresh the folding ranges in a document.
 *
 * @since 3.18.0
 */
struct FoldingRangeRefreshRequest {
    static constexpr char method[] = "workspace/foldingRange/refresh";
    static constexpr MessageDirection direction = MessageDirection::ServerToClient;
    static constexpr bool isRequest = true;
    using Params = std::monostate;
    using Result = std::monostate;
    using PartialResult = std::monostate;
    using RegistrationOptions = std::monostate;
    using ErrorData = std::monostate;
};

/**
 * A request to resolve the type definition locations of a symbol at a given text
 * document position. The request's parameter is of type {@link TextDocumentPositionParams}
 * the response is of type {@link Declaration} or a typed array of {@link DeclarationLink}
 * or a Thenable that resolves to such.
 */
struct DeclarationRequest {
    static constexpr char method[] = "textDocument/declaration";
    static constexpr MessageDirection direction = MessageDirection::ClientToServer;
    static constexpr bool isRequest = true;
    using Params = DeclarationParams;
    using Result = DeclarationRequestResult;
    using PartialResult = DeclarationRequestPartialResult;
    using RegistrationOptions = DeclarationRegistrationOptions;
    using ErrorData = std::monostate;
};

/**
 * A request to provide selection ranges in a document. The request's
 * parameter is of type {@link SelectionRangeParams}, the
 * response is of type {@link SelectionRange SelectionRange[]} or a Thenable
 * that resolves to such.
 */
struct SelectionRangeRequest {
    static constexpr char method[] = "textDocument/selectionRange";
    static constexpr MessageDirection direction = MessageDirection::ClientToServer;
    static constexpr bool isRequest = true;
    using Params = SelectionRangeParams;
    using Result = SelectionRangeRequestResult;
    using PartialResult = SelectionRangeRequestPartialResult;
    using RegistrationOptions = SelectionRangeRegistrationOptions;
    using ErrorData = std::monostate;
};

/**
 * The `window/workDoneProgress/create` request is sent from the server to the client to initiate progress
 * reporting from the server.
 */
struct WorkDoneProgressCreateRequest {
    static constexpr char method[] = "window/workDoneProgress/create";
    static constexpr MessageDirection direction = MessageDirection::ServerToClient;
    static constexpr bool isRequest = true;
    using Params = WorkDoneProgressCreateParams;
    using Result = std::monostate;
    using PartialResult = std::monostate;
    using RegistrationOptions = std::monostate;
    using ErrorData = std::monostate;
};

/**
 * A request to result a `CallHierarchyItem` in a document at a given position.
 * Can be used as an input to an incoming or outgoing call hierarchy.
 *
 * @since 3.16.0
 */
struct CallHierarchyPrepareRequest {
    static constexpr char method[] = "textDocument/prepareCallHierarchy";
    static constexpr MessageDirection direction = MessageDirection::ClientToServer;
    static constexpr bool isRequest = true;
    using Params = CallHierarchyPrepareParams;
    using Result = CallHierarchyPrepareRequestResult;
    using PartialResult = std::monostate;
    using RegistrationOptions = CallHierarchyRegistrationOptions;
    using ErrorData = std::monostate;
};

/**
 * A request to resolve the incoming calls for a given `CallHierarchyItem`.
 *
 * @since 3.16.0
 */
struct CallHierarchyIncomingCallsRequest {
    static constexpr char method[] = "callHierarchy/incomingCalls";
    static constexpr MessageDirection direction = MessageDirection::ClientToServer;
    static constexpr bool isRequest = true;
    using Params = CallHierarchyIncomingCallsParams;
    using Result = CallHierarchyIncomingCallsRequestResult;
    using PartialResult = CallHierarchyIncomingCallsRequestPartialResult;
    using RegistrationOptions = std::monostate;
    using ErrorData = std::monostate;
};

/**
 * A request to resolve the outgoing calls for a given `CallHierarchyItem`.
 *
 * @since 3.16.0
 */
struct CallHierarchyOutgoingCallsRequest {
    static constexpr char method[] = "callHierarchy/outgoingCalls";
    static constexpr MessageDirection direction = MessageDirection::ClientToServer;
    static constexpr bool isRequest = true;
    using Params = CallHierarchyOutgoingCallsParams;
    using Result = CallHierarchyOutgoingCallsRequestResult;
    using PartialResult = CallHierarchyOutgoingCallsRequestPartialResult;
    using RegistrationOptions = std::monostate;
    using ErrorData = std::monostate;
};

/**
 * @since 3.16.0
 */
struct SemanticTokensRequest {
    static constexpr char method[] = "textDocument/semanticTokens/full";
    static constexpr MessageDirection direction = MessageDirection::ClientToServer;
    static constexpr bool isRequest = true;
    using Params = SemanticTokensParams;
    using Result = SemanticTokensRequestResult;
    using PartialResult = SemanticTokensPartialResult;
    using RegistrationOptions = SemanticTokensRegistrationOptions;
    using ErrorData = std::monostate;
};

/**
 * @since 3.16.0
 */
struct SemanticTokensDeltaRequest {
    static constexpr char method[] = "textDocument/semanticTokens/full/delta";
    static constexpr MessageDirection direction = MessageDirection::ClientToServer;
    static constexpr bool isRequest = true;
    using Params = SemanticTokensDeltaParams;
    using Result = SemanticTokensDeltaRequestResult;
    using PartialResult = SemanticTokensDeltaRequestPartialResult;
    using RegistrationOptions = SemanticTokensRegistrationOptions;
    using ErrorData = std::monostate;
};

/**
 * @since 3.16.0
 */
struct SemanticTokensRangeRequest {
    static constexpr char method[] = "textDocument/semanticTokens/range";
    static constexpr MessageDirection direction = MessageDirection::ClientToServer;
    static constexpr bool isRequest = true;
    using Params = SemanticTokensRangeParams;
    using Result = SemanticTokensRangeRequestResult;
    using PartialResult = SemanticTokensPartialResult;
    using RegistrationOptions = std::monostate;
    using ErrorData = std::monostate;
};

/**
 * @since 3.16.0
 */
struct SemanticTokensRefreshRequest {
    static constexpr char method[] = "workspace/semanticTokens/refresh";
    static constexpr MessageDirection direction = MessageDirection::ServerToClient;
    static constexpr bool isRequest = true;
    using Params = std::monostate;
    using Result = std::monostate;
    using PartialResult = std::monostate;
    using RegistrationOptions = std::monostate;
    using ErrorData = std::monostate;
};

/**
 * A request to show a document. This request might open an
 * external program depending on the value of the URI to open.
 * For example a request to open `https://code.visualstudio.com/`
 * will very likely open the URI in a WEB browser.
 *
 * @since 3.16.0
 */
struct ShowDocumentRequest {
    static constexpr char method[] = "window/showDocument";
    static constexpr MessageDirection direction = MessageDirection::ServerToClient;
    static constexpr bool isRequest = true;
    using Params = ShowDocumentParams;
    using Result = ShowDocumentResult;
    using PartialResult = std::monostate;
    using RegistrationOptions = std::monostate;
    using ErrorData = std::monostate;
};

/**
 * A request to provide ranges that can be edited together.
 *
 * @since 3.16.0
 */
struct LinkedEditingRangeRequest {
    static constexpr char method[] = "textDocument/linkedEditingRange";
    static constexpr MessageDirection direction = MessageDirection::ClientToServer;
    static constexpr bool isRequest = true;
    using Params = LinkedEditingRangeParams;
    using Result = LinkedEditingRangeRequestResult;
    using PartialResult = std::monostate;
    using RegistrationOptions = LinkedEditingRangeRegistrationOptions;
    using ErrorData = std::monostate;
};

/**
 * The will create files request is sent from the client to the server before files are actually
 * created as long as the creation is triggered from within the client.
 *
 * The request can return a `WorkspaceEdit` which will be applied to workspace before the
 * files are created. Hence the `WorkspaceEdit` can not manipulate the content of the file
 * to be created.
 *
 * @since 3.16.0
 */
struct WillCreateFilesRequest {
    static constexpr char method[] = "workspace/willCreateFiles";
    static constexpr MessageDirection direction = MessageDirection::ClientToServer;
    static constexpr bool isRequest = true;
    using Params = CreateFilesParams;
    using Result = WillCreateFilesRequestResult;
    using PartialResult = std::monostate;
    using RegistrationOptions = FileOperationRegistrationOptions;
    using ErrorData = std::monostate;
};

/**
 * The will rename files request is sent from the client to the server before files are actually
 * renamed as long as the rename is triggered from within the client.
 *
 * @since 3.16.0
 */
struct WillRenameFilesRequest {
    static constexpr char method[] = "workspace/willRenameFiles";
    static constexpr MessageDirection direction = MessageDirection::ClientToServer;
    static constexpr bool isRequest = true;
    using Params = RenameFilesParams;
    using Result = WillRenameFilesRequestResult;
    using PartialResult = std::monostate;
    using RegistrationOptions = FileOperationRegistrationOptions;
    using ErrorData = std::monostate;
};

/**
 * The did delete files notification is sent from the client to the server when
 * files were deleted from within the client.
 *
 * @since 3.16.0
 */
struct WillDeleteFilesRequest {
    static constexpr char method[] = "workspace/willDeleteFiles";
    static constexpr MessageDirection direction = MessageDirection::ClientToServer;
    static constexpr bool isRequest = true;
    using Params = DeleteFilesParams;
    using Result = WillDeleteFilesRequestResult;
    using PartialResult = std::monostate;
    using RegistrationOptions = FileOperationRegistrationOptions;
    using ErrorData = std::monostate;
};

/**
 * A request to get the moniker of a symbol at a given text document position.
 * The request parameter is of type {@link TextDocumentPositionParams}.
 * The response is of type {@link Moniker Moniker[]} or `null`.
 */
struct MonikerRequest {
    static constexpr char method[] = "textDocument/moniker";
    static constexpr MessageDirection direction = MessageDirection::ClientToServer;
    static constexpr bool isRequest = true;
    using Params = MonikerParams;
    using Result = MonikerRequestResult;
    using PartialResult = MonikerRequestPartialResult;
    using RegistrationOptions = MonikerRegistrationOptions;
    using ErrorData = std::monostate;
};

/**
 * A request to result a `TypeHierarchyItem` in a document at a given position.
 * Can be used as an input to a subtypes or supertypes type hierarchy.
 *
 * @since 3.17.0
 */
struct TypeHierarchyPrepareRequest {
    static constexpr char method[] = "textDocument/prepareTypeHierarchy";
    static constexpr MessageDirection direction = MessageDirection::ClientToServer;
    static constexpr bool isRequest = true;
    using Params = TypeHierarchyPrepareParams;
    using Result = TypeHierarchyPrepareRequestResult;
    using PartialResult = std::monostate;
    using RegistrationOptions = TypeHierarchyRegistrationOptions;
    using ErrorData = std::monostate;
};

/**
 * A request to resolve the supertypes for a given `TypeHierarchyItem`.
 *
 * @since 3.17.0
 */
struct TypeHierarchySupertypesRequest {
    static constexpr char method[] = "typeHierarchy/supertypes";
    static constexpr MessageDirection direction = MessageDirection::ClientToServer;
    static constexpr bool isRequest = true;
    using Params = TypeHierarchySupertypesParams;
    using Result = TypeHierarchySupertypesRequestResult;
    using PartialResult = TypeHierarchySupertypesRequestPartialResult;
    using RegistrationOptions = std::monostate;
    using ErrorData = std::monostate;
};

/**
 * A request to resolve the subtypes for a given `TypeHierarchyItem`.
 *
 * @since 3.17.0
 */
struct TypeHierarchySubtypesRequest {
    static constexpr char method[] = "typeHierarchy/subtypes";
    static constexpr MessageDirection direction = MessageDirection::ClientToServer;
    static constexpr bool isRequest = true;
    using Params = TypeHierarchySubtypesParams;
    using Result = TypeHierarchySubtypesRequestResult;
    using PartialResult = TypeHierarchySubtypesRequestPartialResult;
    using RegistrationOptions = std::monostate;
    using ErrorData = std::monostate;
};

/**
 * A request to provide inline values in a document. The request's parameter is of
 * type {@link InlineValueParams}, the response is of type
 * {@link InlineValue InlineValue[]} or a Thenable that resolves to such.
 *
 * @since 3.17.0
 */
struct InlineValueRequest {
    static constexpr char method[] = "textDocument/inlineValue";
    static constexpr MessageDirection direction = MessageDirection::ClientToServer;
    static constexpr bool isRequest = true;
    using Params = InlineValueParams;
    using Result = InlineValueRequestResult;
    using PartialResult = InlineValueRequestPartialResult;
    using RegistrationOptions = InlineValueRegistrationOptions;
    using ErrorData = std::monostate;
};

/**
 * @since 3.17.0
 */
struct InlineValueRefreshRequest {
    static constexpr char method[] = "workspace/inlineValue/refresh";
    static constexpr MessageDirection direction = MessageDirection::ServerToClient;
    static constexpr bool isRequest = true;
    using Params = std::monostate;
    using Result = std::monostate;
    using PartialResult = std::monostate;
    using RegistrationOptions = std::monostate;
    using ErrorData = std::monostate;
};

/**
 * A request to provide inlay hints in a document. The request's parameter is of
 * type {@link InlayHintsParams}, the response is of type
 * {@link InlayHint InlayHint[]} or a Thenable that resolves to such.
 *
 * @since 3.17.0
 */
struct InlayHintRequest {
    static constexpr char method[] = "textDocument/inlayHint";
    static constexpr MessageDirection direction = MessageDirection::ClientToServer;
    static constexpr bool isRequest = true;
    using Params = InlayHintParams;
    using Result = InlayHintRequestResult;
    using PartialResult = InlayHintRequestPartialResult;
    using RegistrationOptions = InlayHintRegistrationOptions;
    using ErrorData = std::monostate;
};

/**
 * A request to resolve additional properties for an inlay hint.
 * The request's parameter is of type {@link InlayHint}, the response is
 * of type {@link InlayHint} or a Thenable that resolves to such.
 *
 * @since 3.17.0
 */
struct InlayHintResolveRequest {
    static constexpr char method[] = "inlayHint/resolve";
    static constexpr MessageDirection direction = MessageDirection::ClientToServer;
    static constexpr bool isRequest = true;
    using Params = InlayHint;
    using Result = InlayHint;
    using PartialResult = std::monostate;
    using RegistrationOptions = std::monostate;
    using ErrorData = std::monostate;
};

/**
 * @since 3.17.0
 */
struct InlayHintRefreshRequest {
    static constexpr char method[] = "workspace/inlayHint/refresh";
    static constexpr MessageDirection direction = MessageDirection::ServerToClient;
    static constexpr bool isRequest = true;
    using Params = std::monostate;
    using Result = std::monostate;
    using PartialResult = std::monostate;
    using RegistrationOptions = std::monostate;
    using ErrorData = std::monostate;
};

/**
 * The document diagnostic request definition.
 *
 *
 * @since 3.17.0
 */
struct DocumentDiagnosticRequest {
    static constexpr char method[] = "textDocument/diagnostic";
    static constexpr MessageDirection direction = MessageDirection::ClientToServer;
    static constexpr bool isRequest = true;
    using Params = DocumentDiagnosticParams;
    using Result = DocumentDiagnosticReport;
    using PartialResult = DocumentDiagnosticReportProgress;
    using RegistrationOptions = DiagnosticRegistrationOptions;
    using ErrorData = DiagnosticServerCancellationData;
};

/**
 * The workspace diagnostic request definition.
 *
 * @since 3.17.0
 */
struct WorkspaceDiagnosticRequest {
    static constexpr char method[] = "workspace/diagnostic";
    static constexpr MessageDirection direction = MessageDirection::ClientToServer;
    static constexpr bool isRequest = true;
    using Params = WorkspaceDiagnosticParams;
    using Result = WorkspaceDiagnosticReport;
    using PartialResult = WorkspaceDiagnosticReportPartialResult;
    using RegistrationOptions = std::monostate;
    using ErrorData = DiagnosticServerCancellationData;
};

/**
 * The diagnostic refresh request definition.
 *
 * @since 3.17.0
 */
struct DiagnosticRefreshRequest {
    static constexpr char method[] = "workspace/diagnostic/refresh";
    static constexpr MessageDirection direction = MessageDirection::ServerToClient;
    static constexpr bool isRequest = true;
    using Params = std::monostate;
    using Result = std::monostate;
    using PartialResult = std::monostate;
    using RegistrationOptions = std::monostate;
    using ErrorData = std::monostate;
};

/**
 * A request to provide inline completions in a document. The request's parameter is of
 * type {@link InlineCompletionParams}, the response is of type
 * {@link InlineCompletion InlineCompletion[]} or a Thenable that resolves to such.
 *
 * @since 3.18.0
 */
struct InlineCompletionRequest {
    static constexpr char method[] = "textDocument/inlineCompletion";
    static constexpr MessageDirection direction = MessageDirection::ClientToServer;
    static constexpr bool isRequest = true;
    using Params = InlineCompletionParams;
    using Result = InlineCompletionRequestResult;
    using PartialResult = InlineCompletionRequestPartialResult;
    using RegistrationOptions = InlineCompletionRegistrationOptions;
    using ErrorData = std::monostate;
};

/**
 * The `workspace/textDocumentContent` request is sent from the client to the
 * server to request the content of a text document.
 *
 * @since 3.18.0
 */
struct TextDocumentContentRequest {
    static constexpr char method[] = "workspace/textDocumentContent";
    static constexpr MessageDirection direction = MessageDirection::ClientToServer;
    static constexpr bool isRequest = true;
    using Params = TextDocumentContentParams;
    using Result = TextDocumentContentResult;
    using PartialResult = std::monostate;
    using RegistrationOptions = TextDocumentContentRegistrationOptions;
    using ErrorData = std::monostate;
};

/**
 * The `workspace/textDocumentContent` request is sent from the server to the client to refresh
 * the content of a specific text document.
 *
 * @since 3.18.0
 */
struct TextDocumentContentRefreshRequest {
    static constexpr char method[] = "workspace/textDocumentContent/refresh";
    static constexpr MessageDirection direction = MessageDirection::ServerToClient;
    static constexpr bool isRequest = true;
    using Params = TextDocumentContentRefreshParams;
    using Result = std::monostate;
    using PartialResult = std::monostate;
    using RegistrationOptions = std::monostate;
    using ErrorData = std::monostate;
};

/**
 * The `client/registerCapability` request is sent from the server to the client to register a new capability
 * handler on the client side.
 */
struct RegistrationRequest {
    static constexpr char method[] = "client/registerCapability";
    static constexpr MessageDirection direction = MessageDirection::ServerToClient;
    static constexpr bool isRequest = true;
    using Params = RegistrationParams;
    using Result = std::monostate;
    using PartialResult = std::monostate;
    using RegistrationOptions = std::monostate;
    using ErrorData = std::monostate;
};

/**
 * The `client/unregisterCapability` request is sent from the server to the client to unregister a previously registered capability
 * handler on the client side.
 */
struct UnregistrationRequest {
    static constexpr char method[] = "client/unregisterCapability";
    static constexpr MessageDirection direction = MessageDirection::ServerToClient;
    static constexpr bool isRequest = true;
    using Params = UnregistrationParams;
    using Result = std::monostate;
    using PartialResult = std::monostate;
    using RegistrationOptions = std::monostate;
    using ErrorData = std::monostate;
};

/**
 * The initialize request is sent from the client to the server.
 * It is sent once as the request after starting up the server.
 * The requests parameter is of type {@link InitializeParams}
 * the response if of type {@link InitializeResult} of a Thenable that
 * resolves to such.
 */
struct InitializeRequest {
    static constexpr char method[] = "initialize";
    static constexpr MessageDirection direction = MessageDirection::ClientToServer;
    static constexpr bool isRequest = true;
    using Params = InitializeParams;
    using Result = InitializeResult;
    using PartialResult = std::monostate;
    using RegistrationOptions = std::monostate;
    using ErrorData = InitializeError;
};

/**
 * A shutdown request is sent from the client to the server.
 * It is sent once when the client decides to shutdown the
 * server. The only notification that is sent after a shutdown request
 * is the exit event.
 */
struct ShutdownRequest {
    static constexpr char method[] = "shutdown";
    static constexpr MessageDirection direction = MessageDirection::ClientToServer;
    static constexpr bool isRequest = true;
    using Params = std::monostate;
    using Result = std::monostate;
    using PartialResult = std::monostate;
    using RegistrationOptions = std::monostate;
    using ErrorData = std::monostate;
};

/**
 * The show message request is sent from the server to the client to show a message
 * and a set of options actions to the user.
 */
struct ShowMessageRequest {
    static constexpr char method[] = "window/showMessageRequest";
    static constexpr MessageDirection direction = MessageDirection::ServerToClient;
    static constexpr bool isRequest = true;
    using Params = ShowMessageRequestParams;
    using Result = ShowMessageRequestResult;
    using PartialResult = std::monostate;
    using RegistrationOptions = std::monostate;
    using ErrorData = std::monostate;
};

/**
 * A document will save request is sent from the client to the server before
 * the document is actually saved. The request can return an array of TextEdits
 * which will be applied to the text document before it is saved. Please note that
 * clients might drop results if computing the text edits took too long or if a
 * server constantly fails on this request. This is done to keep the save fast and
 * reliable.
 */
struct WillSaveTextDocumentWaitUntilRequest {
    static constexpr char method[] = "textDocument/willSaveWaitUntil";
    static constexpr MessageDirection direction = MessageDirection::ClientToServer;
    static constexpr bool isRequest = true;
    using Params = WillSaveTextDocumentParams;
    using Result = WillSaveTextDocumentWaitUntilRequestResult;
    using PartialResult = std::monostate;
    using RegistrationOptions = TextDocumentRegistrationOptions;
    using ErrorData = std::monostate;
};

/**
 * Request to request completion at a given text document position. The request's
 * parameter is of type {@link TextDocumentPosition} the response
 * is of type {@link CompletionItem CompletionItem[]} or {@link CompletionList}
 * or a Thenable that resolves to such.
 *
 * The request can delay the computation of the {@link CompletionItem.detail `detail`}
 * and {@link CompletionItem.documentation `documentation`} properties to the `completionItem/resolve`
 * request. However, properties that are needed for the initial sorting and filtering, like `sortText`,
 * `filterText`, `insertText`, and `textEdit`, must not be changed during resolve.
 */
struct CompletionRequest {
    static constexpr char method[] = "textDocument/completion";
    static constexpr MessageDirection direction = MessageDirection::ClientToServer;
    static constexpr bool isRequest = true;
    using Params = CompletionParams;
    using Result = CompletionRequestResult;
    using PartialResult = CompletionRequestPartialResult;
    using RegistrationOptions = CompletionRegistrationOptions;
    using ErrorData = std::monostate;
};

/**
 * Request to resolve additional information for a given completion item.The request's
 * parameter is of type {@link CompletionItem} the response
 * is of type {@link CompletionItem} or a Thenable that resolves to such.
 */
struct CompletionResolveRequest {
    static constexpr char method[] = "completionItem/resolve";
    static constexpr MessageDirection direction = MessageDirection::ClientToServer;
    static constexpr bool isRequest = true;
    using Params = CompletionItem;
    using Result = CompletionItem;
    using PartialResult = std::monostate;
    using RegistrationOptions = std::monostate;
    using ErrorData = std::monostate;
};

/**
 * Request to request hover information at a given text document position. The request's
 * parameter is of type {@link TextDocumentPosition} the response is of
 * type {@link Hover} or a Thenable that resolves to such.
 */
struct HoverRequest {
    static constexpr char method[] = "textDocument/hover";
    static constexpr MessageDirection direction = MessageDirection::ClientToServer;
    static constexpr bool isRequest = true;
    using Params = HoverParams;
    using Result = HoverRequestResult;
    using PartialResult = std::monostate;
    using RegistrationOptions = HoverRegistrationOptions;
    using ErrorData = std::monostate;
};

struct SignatureHelpRequest {
    static constexpr char method[] = "textDocument/signatureHelp";
    static constexpr MessageDirection direction = MessageDirection::ClientToServer;
    static constexpr bool isRequest = true;
    using Params = SignatureHelpParams;
    using Result = SignatureHelpRequestResult;
    using PartialResult = std::monostate;
    using RegistrationOptions = SignatureHelpRegistrationOptions;
    using ErrorData = std::monostate;
};

/**
 * A request to resolve the definition location of a symbol at a given text
 * document position. The request's parameter is of type {@link TextDocumentPosition}
 * the response is of either type {@link Definition} or a typed array of
 * {@link DefinitionLink} or a Thenable that resolves to such.
 */
struct DefinitionRequest {
    static constexpr char method[] = "textDocument/definition";
    static constexpr MessageDirection direction = MessageDirection::ClientToServer;
    static constexpr bool isRequest = true;
    using Params = DefinitionParams;
    using Result = DefinitionRequestResult;
    using PartialResult = DefinitionRequestPartialResult;
    using RegistrationOptions = DefinitionRegistrationOptions;
    using ErrorData = std::monostate;
};

/**
 * A request to resolve project-wide references for the symbol denoted
 * by the given text document position. The request's parameter is of
 * type {@link ReferenceParams} the response is of type
 * {@link Location Location[]} or a Thenable that resolves to such.
 */
struct ReferencesRequest {
    static constexpr char method[] = "textDocument/references";
    static constexpr MessageDirection direction = MessageDirection::ClientToServer;
    static constexpr bool isRequest = true;
    using Params = ReferenceParams;
    using Result = ReferencesRequestResult;
    using PartialResult = ReferencesRequestPartialResult;
    using RegistrationOptions = ReferenceRegistrationOptions;
    using ErrorData = std::monostate;
};

/**
 * Request to resolve a {@link DocumentHighlight} for a given
 * text document position. The request's parameter is of type {@link TextDocumentPosition}
 * the request response is an array of type {@link DocumentHighlight}
 * or a Thenable that resolves to such.
 */
struct DocumentHighlightRequest {
    static constexpr char method[] = "textDocument/documentHighlight";
    static constexpr MessageDirection direction = MessageDirection::ClientToServer;
    static constexpr bool isRequest = true;
    using Params = DocumentHighlightParams;
    using Result = DocumentHighlightRequestResult;
    using PartialResult = DocumentHighlightRequestPartialResult;
    using RegistrationOptions = DocumentHighlightRegistrationOptions;
    using ErrorData = std::monostate;
};

/**
 * A request to list all symbols found in a given text document. The request's
 * parameter is of type {@link TextDocumentIdentifier} the
 * response is of type {@link SymbolInformation SymbolInformation[]} or a Thenable
 * that resolves to such.
 */
struct DocumentSymbolRequest {
    static constexpr char method[] = "textDocument/documentSymbol";
    static constexpr MessageDirection direction = MessageDirection::ClientToServer;
    static constexpr bool isRequest = true;
    using Params = DocumentSymbolParams;
    using Result = DocumentSymbolRequestResult;
    using PartialResult = DocumentSymbolRequestPartialResult;
    using RegistrationOptions = DocumentSymbolRegistrationOptions;
    using ErrorData = std::monostate;
};

/**
 * A request to provide commands for the given text document and range.
 */
struct CodeActionRequest {
    static constexpr char method[] = "textDocument/codeAction";
    static constexpr MessageDirection direction = MessageDirection::ClientToServer;
    static constexpr bool isRequest = true;
    using Params = CodeActionParams;
    using Result = CodeActionRequestResult;
    using PartialResult = CodeActionRequestPartialResult;
    using RegistrationOptions = CodeActionRegistrationOptions;
    using ErrorData = std::monostate;
};

/**
 * Request to resolve additional information for a given code action.The request's
 * parameter is of type {@link CodeAction} the response
 * is of type {@link CodeAction} or a Thenable that resolves to such.
 */
struct CodeActionResolveRequest {
    static constexpr char method[] = "codeAction/resolve";
    static constexpr MessageDirection direction = MessageDirection::ClientToServer;
    static constexpr bool isRequest = true;
    using Params = CodeAction;
    using Result = CodeAction;
    using PartialResult = std::monostate;
    using RegistrationOptions = std::monostate;
    using ErrorData = std::monostate;
};

/**
 * A request to list project-wide symbols matching the query string given
 * by the {@link WorkspaceSymbolParams}. The response is
 * of type {@link SymbolInformation SymbolInformation[]} or a Thenable that
 * resolves to such.
 *
 * @since 3.17.0 - support for WorkspaceSymbol in the returned data. Clients
 *  need to advertise support for WorkspaceSymbols via the client capability
 *  `workspace.symbol.resolveSupport`.
 *
 */
struct WorkspaceSymbolRequest {
    static constexpr char method[] = "workspace/symbol";
    static constexpr MessageDirection direction = MessageDirection::ClientToServer;
    static constexpr bool isRequest = true;
    using Params = WorkspaceSymbolParams;
    using Result = WorkspaceSymbolRequestResult;
    using PartialResult = WorkspaceSymbolRequestPartialResult;
    using RegistrationOptions = WorkspaceSymbolRegistrationOptions;
    using ErrorData = std::monostate;
};

/**
 * A request to resolve the range inside the workspace
 * symbol's location.
 *
 * @since 3.17.0
 */
struct WorkspaceSymbolResolveRequest {
    static constexpr char method[] = "workspaceSymbol/resolve";
    static constexpr MessageDirection direction = MessageDirection::ClientToServer;
    static constexpr bool isRequest = true;
    using Params = WorkspaceSymbol;
    using Result = WorkspaceSymbol;
    using PartialResult = std::monostate;
    using RegistrationOptions = std::monostate;
    using ErrorData = std::monostate;
};

/**
 * A request to provide code lens for the given text document.
 */
struct CodeLensRequest {
    static constexpr char method[] = "textDocument/codeLens";
    static constexpr MessageDirection direction = MessageDirection::ClientToServer;
    static constexpr bool isRequest = true;
    using Params = CodeLensParams;
    using Result = CodeLensRequestResult;
    using PartialResult = CodeLensRequestPartialResult;
    using RegistrationOptions = CodeLensRegistrationOptions;
    using ErrorData = std::monostate;
};

/**
 * A request to resolve a command for a given code lens.
 */
struct CodeLensResolveRequest {
    static constexpr char method[] = "codeLens/resolve";
    static constexpr MessageDirection direction = MessageDirection::ClientToServer;
    static constexpr bool isRequest = true;
    using Params = CodeLens;
    using Result = CodeLens;
    using PartialResult = std::monostate;
    using RegistrationOptions = std::monostate;
    using ErrorData = std::monostate;
};

/**
 * A request to refresh all code actions
 *
 * @since 3.16.0
 */
struct CodeLensRefreshRequest {
    static constexpr char method[] = "workspace/codeLens/refresh";
    static constexpr MessageDirection direction = MessageDirection::ServerToClient;
    static constexpr bool isRequest = true;
    using Params = std::monostate;
    using Result = std::monostate;
    using PartialResult = std::monostate;
    using RegistrationOptions = std::monostate;
    using ErrorData = std::monostate;
};

/**
 * A request to provide document links
 */
struct DocumentLinkRequest {
    static constexpr char method[] = "textDocument/documentLink";
    static constexpr MessageDirection direction = MessageDirection::ClientToServer;
    static constexpr bool isRequest = true;
    using Params = DocumentLinkParams;
    using Result = DocumentLinkRequestResult;
    using PartialResult = DocumentLinkRequestPartialResult;
    using RegistrationOptions = DocumentLinkRegistrationOptions;
    using ErrorData = std::monostate;
};

/**
 * Request to resolve additional information for a given document link. The request's
 * parameter is of type {@link DocumentLink} the response
 * is of type {@link DocumentLink} or a Thenable that resolves to such.
 */
struct DocumentLinkResolveRequest {
    static constexpr char method[] = "documentLink/resolve";
    static constexpr MessageDirection direction = MessageDirection::ClientToServer;
    static constexpr bool isRequest = true;
    using Params = DocumentLink;
    using Result = DocumentLink;
    using PartialResult = std::monostate;
    using RegistrationOptions = std::monostate;
    using ErrorData = std::monostate;
};

/**
 * A request to format a whole document.
 */
struct DocumentFormattingRequest {
    static constexpr char method[] = "textDocument/formatting";
    static constexpr MessageDirection direction = MessageDirection::ClientToServer;
    static constexpr bool isRequest = true;
    using Params = DocumentFormattingParams;
    using Result = DocumentFormattingRequestResult;
    using PartialResult = std::monostate;
    using RegistrationOptions = DocumentFormattingRegistrationOptions;
    using ErrorData = std::monostate;
};

/**
 * A request to format a range in a document.
 */
struct DocumentRangeFormattingRequest {
    static constexpr char method[] = "textDocument/rangeFormatting";
    static constexpr MessageDirection direction = MessageDirection::ClientToServer;
    static constexpr bool isRequest = true;
    using Params = DocumentRangeFormattingParams;
    using Result = DocumentRangeFormattingRequestResult;
    using PartialResult = std::monostate;
    using RegistrationOptions = DocumentRangeFormattingRegistrationOptions;
    using ErrorData = std::monostate;
};

/**
 * A request to format ranges in a document.
 *
 * @since 3.18.0
 */
struct DocumentRangesFormattingRequest {
    static constexpr char method[] = "textDocument/rangesFormatting";
    static constexpr MessageDirection direction = MessageDirection::ClientToServer;
    static constexpr bool isRequest = true;
    using Params = DocumentRangesFormattingParams;
    using Result = DocumentRangesFormattingRequestResult;
    using PartialResult = std::monostate;
    using RegistrationOptions = DocumentRangeFormattingRegistrationOptions;
    using ErrorData = std::monostate;
};

/**
 * A request to format a document on type.
 */
struct DocumentOnTypeFormattingRequest {
    static constexpr char method[] = "textDocument/onTypeFormatting";
    static constexpr MessageDirection direction = MessageDirection::ClientToServer;
    static constexpr bool isRequest = true;
    using Params = DocumentOnTypeFormattingParams;
    using Result = DocumentOnTypeFormattingRequestResult;
    using PartialResult = std::monostate;
    using RegistrationOptions = DocumentOnTypeFormattingRegistrationOptions;
    using ErrorData = std::monostate;
};

/**
 * A request to rename a symbol.
 */
struct RenameRequest {
    static constexpr char method[] = "textDocument/rename";
    static constexpr MessageDirection direction = MessageDirection::ClientToServer;
    static constexpr bool isRequest = true;
    using Params = RenameParams;
    using Result = RenameRequestResult;
    using PartialResult = std::monostate;
    using RegistrationOptions = RenameRegistrationOptions;
    using ErrorData = std::monostate;
};

/**
 * A request to test and perform the setup necessary for a rename.
 *
 * @since 3.16 - support for default behavior
 */
struct PrepareRenameRequest {
    static constexpr char method[] = "textDocument/prepareRename";
    static constexpr MessageDirection direction = MessageDirection::ClientToServer;
    static constexpr bool isRequest = true;
    using Params = PrepareRenameParams;
    using Result = PrepareRenameRequestResult;
    using PartialResult = std::monostate;
    using RegistrationOptions = std::monostate;
    using ErrorData = std::monostate;
};

/**
 * A request send from the client to the server to execute a command. The request might return
 * a workspace edit which the client will apply to the workspace.
 */
struct ExecuteCommandRequest {
    static constexpr char method[] = "workspace/executeCommand";
    static constexpr MessageDirection direction = MessageDirection::ClientToServer;
    static constexpr bool isRequest = true;
    using Params = ExecuteCommandParams;
    using Result = ExecuteCommandRequestResult;
    using PartialResult = std::monostate;
    using RegistrationOptions = ExecuteCommandRegistrationOptions;
    using ErrorData = std::monostate;
};

/**
 * A request sent from the server to the client to modified certain resources.
 */
struct ApplyWorkspaceEditRequest {
    static constexpr char method[] = "workspace/applyEdit";
    static constexpr MessageDirection direction = MessageDirection::ServerToClient;
    static constexpr bool isRequest = true;
    using Params = ApplyWorkspaceEditParams;
    using Result = ApplyWorkspaceEditResult;
    using PartialResult = std::monostate;
    using RegistrationOptions = std::monostate;
    using ErrorData = std::monostate;
};

/**
 * The `workspace/didChangeWorkspaceFolders` notification is sent from the client to the server when the workspace
 * folder configuration changes.
 */
struct DidChangeWorkspaceFoldersNotification {
    static constexpr char method[] = "workspace/didChangeWorkspaceFolders";
    static constexpr MessageDirection direction = MessageDirection::ClientToServer;
    static constexpr bool isRequest = false;
    using Params = DidChangeWorkspaceFoldersParams;
    using Result = std::monostate;
    using PartialResult = std::monostate;
    using RegistrationOptions = std::monostate;
    using ErrorData = std::monostate;
};

/**
 * The `window/workDoneProgress/cancel` notification is sent from  the client to the server to cancel a progress
 * initiated on the server side.
 */
struct WorkDoneProgressCancelNotification {
    static constexpr char method[] = "window/workDoneProgress/cancel";
    static constexpr MessageDirection direction = MessageDirection::ClientToServer;
    static constexpr bool isRequest = false;
    using Params = WorkDoneProgressCancelParams;
    using Result = std::monostate;
    using PartialResult = std::monostate;
    using RegistrationOptions = std::monostate;
    using ErrorData = std::monostate;
};

/**
 * The did create files notification is sent from the client to the server when
 * files were created from within the client.
 *
 * @since 3.16.0
 */
struct DidCreateFilesNotification {
    static constexpr char method[] = "workspace/didCreateFiles";
    static constexpr MessageDirection direction = MessageDirection::ClientToServer;
    static constexpr bool isRequest = false;
    using Params = CreateFilesParams;
    using Result = std::monostate;
    using PartialResult = std::monostate;
    using RegistrationOptions = FileOperationRegistrationOptions;
    using ErrorData = std::monostate;
};

/**
 * The did rename files notification is sent from the client to the server when
 * files were renamed from within the client.
 *
 * @since 3.16.0
 */
struct DidRenameFilesNotification {
    static constexpr char method[] = "workspace/didRenameFiles";
    static constexpr MessageDirection direction = MessageDirection::ClientToServer;
    static constexpr bool isRequest = false;
    using Params = RenameFilesParams;
    using Result = std::monostate;
    using PartialResult = std::monostate;
    using RegistrationOptions = FileOperationRegistrationOptions;
    using ErrorData = std::monostate;
};

/**
 * The will delete files request is sent from the client to the server before files are actually
 * deleted as long as the deletion is triggered from within the client.
 *
 * @since 3.16.0
 */
struct DidDeleteFilesNotification {
    static constexpr char method[] = "workspace/didDeleteFiles";
    static constexpr MessageDirection direction = MessageDirection::ClientToServer;
    static constexpr bool isRequest = false;
    using Params = DeleteFilesParams;
    using Result = std::monostate;
    using PartialResult = std::monostate;
    using RegistrationOptions = FileOperationRegistrationOptions;
    using ErrorData = std::monostate;
};

/**
 * A notification sent when a notebook opens.
 *
 * @since 3.17.0
 */
struct DidOpenNotebookDocumentNotification {
    static constexpr char method[] = "notebookDocument/didOpen";
    static constexpr MessageDirection direction = MessageDirection::ClientToServer;
    static constexpr bool isRequest = false;
    using Params = DidOpenNotebookDocumentParams;
    using Result = std::monostate;
    using PartialResult = std::monostate;
    using RegistrationOptions = NotebookDocumentSyncRegistrationOptions;
    using ErrorData = std::monostate;
};

struct DidChangeNotebookDocumentNotification {
    static constexpr char method[] = "notebookDocument/didChange";
    static constexpr MessageDirection direction = MessageDirection::ClientToServer;
    static constexpr bool isRequest = false;
    using Params = DidChangeNotebookDocumentParams;
    using Result = std::monostate;
    using PartialResult = std::monostate;
    using RegistrationOptions = NotebookDocumentSyncRegistrationOptions;
    using ErrorData = std::monostate;
};

/**
 * A notification sent when a notebook document is saved.
 *
 * @since 3.17.0
 */
struct DidSaveNotebookDocumentNotification {
    static constexpr char method[] = "notebookDocument/didSave";
    static constexpr MessageDirection direction = MessageDirection::ClientToServer;
    static constexpr bool isRequest = false;
    using Params = DidSaveNotebookDocumentParams;
    using Result = std::monostate;
    using PartialResult = std::monostate;
    using RegistrationOptions = NotebookDocumentSyncRegistrationOptions;
    using ErrorData = std::monostate;
};

/**
 * A notification sent when a notebook closes.
 *
 * @since 3.17.0
 */
struct DidCloseNotebookDocumentNotification {
    static constexpr char method[] = "notebookDocument/didClose";
    static constexpr MessageDirection direction = MessageDirection::ClientToServer;
    static constexpr bool isRequest = false;
    using Params = DidCloseNotebookDocumentParams;
    using Result = std::monostate;
    using PartialResult = std::monostate;
    using RegistrationOptions = NotebookDocumentSyncRegistrationOptions;
    using ErrorData = std::monostate;
};

/**
 * The initialized notification is sent from the client to the
 * server after the client is fully initialized and the server
 * is allowed to send requests from the server to the client.
 */
struct InitializedNotification {
    static constexpr char method[] = "initialized";
    static constexpr MessageDirection direction = MessageDirection::ClientToServer;
    static constexpr bool isRequest = false;
    using Params = InitializedParams;
    using Result = std::monostate;
    using PartialResult = std::monostate;
    using RegistrationOptions = std::monostate;
    using ErrorData = std::monostate;
};

/**
 * The exit event is sent from the client to the server to
 * ask the server to exit its process.
 */
struct ExitNotification {
    static constexpr char method[] = "exit";
    static constexpr MessageDirection direction = MessageDirection::ClientToServer;
    static constexpr bool isRequest = false;
    using Params = std::monostate;
    using Result = std::monostate;
    using PartialResult = std::monostate;
    using RegistrationOptions = std::monostate;
    using ErrorData = std::monostate;
};

/**
 * The configuration change notification is sent from the client to the server
 * when the client's configuration has changed. The notification contains
 * the changed configuration as defined by the language client.
 */
struct DidChangeConfigurationNotification {
    static constexpr char method[] = "workspace/didChangeConfiguration";
    static constexpr MessageDirection direction = MessageDirection::ClientToServer;
    static constexpr bool isRequest = false;
    using Params = DidChangeConfigurationParams;
    using Result = std::monostate;
    using PartialResult = std::monostate;
    using RegistrationOptions = DidChangeConfigurationRegistrationOptions;
    using ErrorData = std::monostate;
};

/**
 * The show message notification is sent from a server to a client to ask
 * the client to display a particular message in the user interface.
 */
struct ShowMessageNotification {
    static constexpr char method[] = "window/showMessage";
    static constexpr MessageDirection direction = MessageDirection::ServerToClient;
    static constexpr bool isRequest = false;
    using Params = ShowMessageParams;
    using Result = std::monostate;
    using PartialResult = std::monostate;
    using RegistrationOptions = std::monostate;
    using ErrorData = std::monostate;
};

/**
 * The log message notification is sent from the server to the client to ask
 * the client to log a particular message.
 */
struct LogMessageNotification {
    static constexpr char method[] = "window/logMessage";
    static constexpr MessageDirection direction = MessageDirection::ServerToClient;
    static constexpr bool isRequest = false;
    using Params = LogMessageParams;
    using Result = std::monostate;
    using PartialResult = std::monostate;
    using RegistrationOptions = std::monostate;
    using ErrorData = std::monostate;
};

/**
 * The telemetry event notification is sent from the server to the client to ask
 * the client to log telemetry data.
 */
struct TelemetryEventNotification {
    static constexpr char method[] = "telemetry/event";
    static constexpr MessageDirection direction = MessageDirection::ServerToClient;
    static constexpr bool isRequest = false;
    using Params = QJsonValue;
    using Result = std::monostate;
    using PartialResult = std::monostate;
    using RegistrationOptions = std::monostate;
    using ErrorData = std::monostate;
};

/**
 * The document open notification is sent from the client to the server to signal
 * newly opened text documents. The document's truth is now managed by the client
 * and the server must not try to read the document's truth using the document's
 * uri. Open in this sense means it is managed by the client. It doesn't necessarily
 * mean that its content is presented in an editor. An open notification must not
 * be sent more than once without a corresponding close notification send before.
 * This means open and close notification must be balanced and the max open count
 * is one.
 */
struct DidOpenTextDocumentNotification {
    static constexpr char method[] = "textDocument/didOpen";
    static constexpr MessageDirection direction = MessageDirection::ClientToServer;
    static constexpr bool isRequest = false;
    using Params = DidOpenTextDocumentParams;
    using Result = std::monostate;
    using PartialResult = std::monostate;
    using RegistrationOptions = TextDocumentRegistrationOptions;
    using ErrorData = std::monostate;
};

/**
 * The document change notification is sent from the client to the server to signal
 * changes to a text document.
 */
struct DidChangeTextDocumentNotification {
    static constexpr char method[] = "textDocument/didChange";
    static constexpr MessageDirection direction = MessageDirection::ClientToServer;
    static constexpr bool isRequest = false;
    using Params = DidChangeTextDocumentParams;
    using Result = std::monostate;
    using PartialResult = std::monostate;
    using RegistrationOptions = TextDocumentChangeRegistrationOptions;
    using ErrorData = std::monostate;
};

/**
 * The document close notification is sent from the client to the server when
 * the document got closed in the client. The document's truth now exists where
 * the document's uri points to (e.g. if the document's uri is a file uri the
 * truth now exists on disk). As with the open notification the close notification
 * is about managing the document's content. Receiving a close notification
 * doesn't mean that the document was open in an editor before. A close
 * notification requires a previous open notification to be sent.
 */
struct DidCloseTextDocumentNotification {
    static constexpr char method[] = "textDocument/didClose";
    static constexpr MessageDirection direction = MessageDirection::ClientToServer;
    static constexpr bool isRequest = false;
    using Params = DidCloseTextDocumentParams;
    using Result = std::monostate;
    using PartialResult = std::monostate;
    using RegistrationOptions = TextDocumentRegistrationOptions;
    using ErrorData = std::monostate;
};

/**
 * The document save notification is sent from the client to the server when
 * the document got saved in the client.
 */
struct DidSaveTextDocumentNotification {
    static constexpr char method[] = "textDocument/didSave";
    static constexpr MessageDirection direction = MessageDirection::ClientToServer;
    static constexpr bool isRequest = false;
    using Params = DidSaveTextDocumentParams;
    using Result = std::monostate;
    using PartialResult = std::monostate;
    using RegistrationOptions = TextDocumentSaveRegistrationOptions;
    using ErrorData = std::monostate;
};

/**
 * A document will save notification is sent from the client to the server before
 * the document is actually saved.
 */
struct WillSaveTextDocumentNotification {
    static constexpr char method[] = "textDocument/willSave";
    static constexpr MessageDirection direction = MessageDirection::ClientToServer;
    static constexpr bool isRequest = false;
    using Params = WillSaveTextDocumentParams;
    using Result = std::monostate;
    using PartialResult = std::monostate;
    using RegistrationOptions = TextDocumentRegistrationOptions;
    using ErrorData = std::monostate;
};

/**
 * The watched files notification is sent from the client to the server when
 * the client detects changes to file watched by the language client.
 */
struct DidChangeWatchedFilesNotification {
    static constexpr char method[] = "workspace/didChangeWatchedFiles";
    static constexpr MessageDirection direction = MessageDirection::ClientToServer;
    static constexpr bool isRequest = false;
    using Params = DidChangeWatchedFilesParams;
    using Result = std::monostate;
    using PartialResult = std::monostate;
    using RegistrationOptions = DidChangeWatchedFilesRegistrationOptions;
    using ErrorData = std::monostate;
};

/**
 * Diagnostics notification are sent from the server to the client to signal
 * results of validation runs.
 */
struct PublishDiagnosticsNotification {
    static constexpr char method[] = "textDocument/publishDiagnostics";
    static constexpr MessageDirection direction = MessageDirection::ServerToClient;
    static constexpr bool isRequest = false;
    using Params = PublishDiagnosticsParams;
    using Result = std::monostate;
    using PartialResult = std::monostate;
    using RegistrationOptions = std::monostate;
    using ErrorData = std::monostate;
};

struct SetTraceNotification {
    static constexpr char method[] = "$/setTrace";
    static constexpr MessageDirection direction = MessageDirection::ClientToServer;
    static constexpr bool isRequest = false;
    using Params = SetTraceParams;
    using Result = std::monostate;
    using PartialResult = std::monostate;
    using RegistrationOptions = std::monostate;
    using ErrorData = std::monostate;
};

struct LogTraceNotification {
    static constexpr char method[] = "$/logTrace";
    static constexpr MessageDirection direction = MessageDirection::ServerToClient;
    static constexpr bool isRequest = false;
    using Params = LogTraceParams;
    using Result = std::monostate;
    using PartialResult = std::monostate;
    using RegistrationOptions = std::monostate;
    using ErrorData = std::monostate;
};

struct CancelNotification {
    static constexpr char method[] = "$/cancelRequest";
    static constexpr MessageDirection direction = MessageDirection::Both;
    static constexpr bool isRequest = false;
    using Params = CancelParams;
    using Result = std::monostate;
    using PartialResult = std::monostate;
    using RegistrationOptions = std::monostate;
    using ErrorData = std::monostate;
};

struct ProgressNotification {
    static constexpr char method[] = "$/progress";
    static constexpr MessageDirection direction = MessageDirection::Both;
    static constexpr bool isRequest = false;
    using Params = ProgressParams;
    using Result = std::monostate;
    using PartialResult = std::monostate;
    using RegistrationOptions = std::monostate;
    using ErrorData = std::monostate;
};

} // namespace LanguageServerProtocol
