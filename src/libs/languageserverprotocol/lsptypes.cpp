// This file is auto-generated. Do not edit manually.
#include "lsptypes.h"

namespace LanguageServerProtocol {

template<>
Utils::Result<Position> fromJson<Position>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for Position");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("line"))
        return Utils::ResultError("Missing required field: line");
    if (!obj.contains("character"))
        return Utils::ResultError("Missing required field: character");
    Position result;
    result._line = obj.value("line").toInt();
    result._character = obj.value("character").toInt();
    return result;
}

QJsonObject toJson(const Position &data)
{
    QJsonObject obj{
        {"line", data._line},
        {"character", data._character}
    };
    return obj;
}

template<>
Utils::Result<ProgressToken> fromJson<ProgressToken>(const QJsonValue &val)
{
    if (val.isDouble())
        return ProgressToken(static_cast<int>(val.toDouble()));
    if (val.isString())
        return ProgressToken(val.toString());
    return Utils::ResultError("Invalid ProgressToken");
}

QJsonValue toJsonValue(const ProgressToken &val)
{
    return std::visit([](const auto &v) -> QJsonValue {
        {
            return QVariant::fromValue(v).toJsonValue();
        }
    }, val);
}

template<>
Utils::Result<TextDocumentIdentifier> fromJson<TextDocumentIdentifier>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for TextDocumentIdentifier");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("uri"))
        return Utils::ResultError("Missing required field: uri");
    TextDocumentIdentifier result;
    result._uri = obj.value("uri").toString();
    return result;
}

QJsonObject toJson(const TextDocumentIdentifier &data)
{
    QJsonObject obj{{"uri", data._uri}};
    return obj;
}

template<>
Utils::Result<ImplementationParams> fromJson<ImplementationParams>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for ImplementationParams");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("textDocument"))
        return Utils::ResultError("Missing required field: textDocument");
    if (!obj.contains("position"))
        return Utils::ResultError("Missing required field: position");
    ImplementationParams result;
    if (obj.contains("textDocument") && obj["textDocument"].isObject()) {
        const auto res0 = fromJson<TextDocumentIdentifier>("textDocument", obj["textDocument"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._textDocument = *res0;
    }
    if (obj.contains("position") && obj["position"].isObject()) {
        const auto res1 = fromJson<Position>("position", obj["position"]);
        if (!res1)
            return Utils::ResultError(res1.error());
        result._position = *res1;
    }
    if (obj.contains("workDoneToken")) {
        const auto res2 = fromJson<ProgressToken>("workDoneToken", obj["workDoneToken"]);
        if (!res2)
            return Utils::ResultError(res2.error());
        result._workDoneToken = *res2;
    }
    if (obj.contains("partialResultToken")) {
        const auto res3 = fromJson<ProgressToken>("partialResultToken", obj["partialResultToken"]);
        if (!res3)
            return Utils::ResultError(res3.error());
        result._partialResultToken = *res3;
    }
    return result;
}

QJsonObject toJson(const ImplementationParams &data)
{
    QJsonObject obj{
        {"textDocument", toJson(data._textDocument)},
        {"position", toJson(data._position)}
    };
    if (data._workDoneToken.has_value())
        obj.insert("workDoneToken", toJsonValue(*data._workDoneToken));
    if (data._partialResultToken.has_value())
        obj.insert("partialResultToken", toJsonValue(*data._partialResultToken));
    return obj;
}

template<>
Utils::Result<Range> fromJson<Range>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for Range");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("start"))
        return Utils::ResultError("Missing required field: start");
    if (!obj.contains("end"))
        return Utils::ResultError("Missing required field: end");
    Range result;
    if (obj.contains("start") && obj["start"].isObject()) {
        const auto res0 = fromJson<Position>("start", obj["start"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._start = *res0;
    }
    if (obj.contains("end") && obj["end"].isObject()) {
        const auto res1 = fromJson<Position>("end", obj["end"]);
        if (!res1)
            return Utils::ResultError(res1.error());
        result._end = *res1;
    }
    return result;
}

QJsonObject toJson(const Range &data)
{
    QJsonObject obj{
        {"start", toJson(data._start)},
        {"end", toJson(data._end)}
    };
    return obj;
}

template<>
Utils::Result<Location> fromJson<Location>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for Location");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("uri"))
        return Utils::ResultError("Missing required field: uri");
    if (!obj.contains("range"))
        return Utils::ResultError("Missing required field: range");
    Location result;
    result._uri = obj.value("uri").toString();
    if (obj.contains("range") && obj["range"].isObject()) {
        const auto res0 = fromJson<Range>("range", obj["range"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._range = *res0;
    }
    return result;
}

QJsonObject toJson(const Location &data)
{
    QJsonObject obj{
        {"uri", data._uri},
        {"range", toJson(data._range)}
    };
    return obj;
}

template<> Utils::Result<Pattern> fromJson<Pattern>(const QJsonValue &val)
{
    if (!val.isString()) return Utils::ResultError("Expected string");
    return val.toString();
}

template<>
Utils::Result<WorkspaceFolder> fromJson<WorkspaceFolder>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for WorkspaceFolder");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("uri"))
        return Utils::ResultError("Missing required field: uri");
    if (!obj.contains("name"))
        return Utils::ResultError("Missing required field: name");
    WorkspaceFolder result;
    result._uri = obj.value("uri").toString();
    result._name = obj.value("name").toString();
    return result;
}

QJsonObject toJson(const WorkspaceFolder &data)
{
    QJsonObject obj{
        {"uri", data._uri},
        {"name", data._name}
    };
    return obj;
}

template<>
Utils::Result<RelativePatternBaseUri> fromJson<RelativePatternBaseUri>(const QJsonValue &val)
{
    if (val.isString())
        return RelativePatternBaseUri(val.toString());
    if (!val.isObject())
        return Utils::ResultError("Invalid RelativePatternBaseUri: expected object or array");
    const QJsonObject obj = val.toObject();
    if (obj.contains("name")) {
        const auto res0 = fromJson<WorkspaceFolder>(val);
        if (!res0)
            return Utils::ResultError(res0.error());
        return RelativePatternBaseUri(*res0);
    }
    return Utils::ResultError("Invalid RelativePatternBaseUri");
}

QJsonValue toJsonValue(const RelativePatternBaseUri &val)
{
    return std::visit([](const auto &v) -> QJsonValue {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, WorkspaceFolder>) {
            return toJson(v);
        } else
        {
            return QVariant::fromValue(v).toJsonValue();
        }
    }, val);
}

template<>
Utils::Result<RelativePattern> fromJson<RelativePattern>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for RelativePattern");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("baseUri"))
        return Utils::ResultError("Missing required field: baseUri");
    if (!obj.contains("pattern"))
        return Utils::ResultError("Missing required field: pattern");
    RelativePattern result;
    if (obj.contains("baseUri")) {
        const auto res0 = fromJson<RelativePatternBaseUri>("baseUri", obj["baseUri"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._baseUri = *res0;
    }
    if (obj.contains("pattern") && obj["pattern"].isString()) {
        const auto res1 = fromJson<Pattern>("pattern", obj["pattern"]);
        if (!res1)
            return Utils::ResultError(res1.error());
        result._pattern = *res1;
    }
    return result;
}

QJsonObject toJson(const RelativePattern &data)
{
    QJsonObject obj{
        {"baseUri", toJsonValue(data._baseUri)},
        {"pattern", data._pattern}
    };
    return obj;
}

template<>
Utils::Result<GlobPattern> fromJson<GlobPattern>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Invalid GlobPattern: expected object");
    const QJsonObject obj = val.toObject();
    if (obj.contains("baseUri")) {
        const auto res0 = fromJson<RelativePattern>(val);
        if (!res0)
            return Utils::ResultError(res0.error());
        return GlobPattern(*res0);
    }
    {
        auto result = fromJson<Pattern>(val);
        if (result) return GlobPattern(*result);
    }
    return Utils::ResultError("Invalid GlobPattern");
}

QJsonObject toJson(const GlobPattern &val)
{
    return std::visit([](const auto &v) -> QJsonObject {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, QJsonObject>) {
            return v;
        } else {
            return toJson(v);
        }
    }, val);
}

QJsonValue toJsonValue(const GlobPattern &val)
{
    return toJson(val);
}

template<>
Utils::Result<NotebookDocumentFilterNotebookType> fromJson<NotebookDocumentFilterNotebookType>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for NotebookDocumentFilterNotebookType");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("notebookType"))
        return Utils::ResultError("Missing required field: notebookType");
    NotebookDocumentFilterNotebookType result;
    result._notebookType = obj.value("notebookType").toString();
    if (obj.contains("scheme"))
        result._scheme = obj.value("scheme").toString();
    if (obj.contains("pattern")) {
        const auto res0 = fromJson<GlobPattern>("pattern", obj["pattern"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._pattern = *res0;
    }
    return result;
}

QJsonObject toJson(const NotebookDocumentFilterNotebookType &data)
{
    QJsonObject obj{{"notebookType", data._notebookType}};
    if (data._scheme.has_value())
        obj.insert("scheme", *data._scheme);
    if (data._pattern.has_value())
        obj.insert("pattern", toJsonValue(*data._pattern));
    return obj;
}

template<>
Utils::Result<NotebookDocumentFilterPattern> fromJson<NotebookDocumentFilterPattern>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for NotebookDocumentFilterPattern");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("pattern"))
        return Utils::ResultError("Missing required field: pattern");
    NotebookDocumentFilterPattern result;
    if (obj.contains("notebookType"))
        result._notebookType = obj.value("notebookType").toString();
    if (obj.contains("scheme"))
        result._scheme = obj.value("scheme").toString();
    if (obj.contains("pattern")) {
        const auto res0 = fromJson<GlobPattern>("pattern", obj["pattern"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._pattern = *res0;
    }
    return result;
}

QJsonObject toJson(const NotebookDocumentFilterPattern &data)
{
    QJsonObject obj{{"pattern", toJsonValue(data._pattern)}};
    if (data._notebookType.has_value())
        obj.insert("notebookType", *data._notebookType);
    if (data._scheme.has_value())
        obj.insert("scheme", *data._scheme);
    return obj;
}

template<>
Utils::Result<NotebookDocumentFilterScheme> fromJson<NotebookDocumentFilterScheme>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for NotebookDocumentFilterScheme");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("scheme"))
        return Utils::ResultError("Missing required field: scheme");
    NotebookDocumentFilterScheme result;
    if (obj.contains("notebookType"))
        result._notebookType = obj.value("notebookType").toString();
    result._scheme = obj.value("scheme").toString();
    if (obj.contains("pattern")) {
        const auto res0 = fromJson<GlobPattern>("pattern", obj["pattern"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._pattern = *res0;
    }
    return result;
}

QJsonObject toJson(const NotebookDocumentFilterScheme &data)
{
    QJsonObject obj{{"scheme", data._scheme}};
    if (data._notebookType.has_value())
        obj.insert("notebookType", *data._notebookType);
    if (data._pattern.has_value())
        obj.insert("pattern", toJsonValue(*data._pattern));
    return obj;
}

template<>
Utils::Result<NotebookDocumentFilter> fromJson<NotebookDocumentFilter>(const QJsonValue &val)
{
    if (val.isObject()) {
        auto result = fromJson<NotebookDocumentFilterNotebookType>(val);
        if (result) return NotebookDocumentFilter(*result);
    }
    if (val.isObject()) {
        auto result = fromJson<NotebookDocumentFilterScheme>(val);
        if (result) return NotebookDocumentFilter(*result);
    }
    if (val.isObject()) {
        auto result = fromJson<NotebookDocumentFilterPattern>(val);
        if (result) return NotebookDocumentFilter(*result);
    }
    return Utils::ResultError("Invalid NotebookDocumentFilter");
}

QJsonObject toJson(const NotebookDocumentFilter &val)
{
    return std::visit([](const auto &v) -> QJsonObject {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, QJsonObject>) {
            return v;
        } else {
            return toJson(v);
        }
    }, val);
}

QJsonValue toJsonValue(const NotebookDocumentFilter &val)
{
    return toJson(val);
}

template<>
Utils::Result<NotebookCellTextDocumentFilterNotebook> fromJson<NotebookCellTextDocumentFilterNotebook>(const QJsonValue &val)
{
    if (val.isString())
        return NotebookCellTextDocumentFilterNotebook(val.toString());
    {
        auto result = fromJson<NotebookDocumentFilter>(val);
        if (result) return NotebookCellTextDocumentFilterNotebook(*result);
    }
    return Utils::ResultError("Invalid NotebookCellTextDocumentFilterNotebook");
}

QJsonValue toJsonValue(const NotebookCellTextDocumentFilterNotebook &val)
{
    return std::visit([](const auto &v) -> QJsonValue {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, NotebookDocumentFilter>) {
            return toJsonValue(v);
        } else
        {
            return QVariant::fromValue(v).toJsonValue();
        }
    }, val);
}

template<>
Utils::Result<NotebookCellTextDocumentFilter> fromJson<NotebookCellTextDocumentFilter>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for NotebookCellTextDocumentFilter");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("notebook"))
        return Utils::ResultError("Missing required field: notebook");
    NotebookCellTextDocumentFilter result;
    if (obj.contains("notebook")) {
        const auto res0 = fromJson<NotebookCellTextDocumentFilterNotebook>("notebook", obj["notebook"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._notebook = *res0;
    }
    if (obj.contains("language"))
        result._language = obj.value("language").toString();
    return result;
}

QJsonObject toJson(const NotebookCellTextDocumentFilter &data)
{
    QJsonObject obj{{"notebook", toJsonValue(data._notebook)}};
    if (data._language.has_value())
        obj.insert("language", *data._language);
    return obj;
}

template<>
Utils::Result<TextDocumentFilterLanguage> fromJson<TextDocumentFilterLanguage>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for TextDocumentFilterLanguage");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("language"))
        return Utils::ResultError("Missing required field: language");
    TextDocumentFilterLanguage result;
    result._language = obj.value("language").toString();
    if (obj.contains("scheme"))
        result._scheme = obj.value("scheme").toString();
    if (obj.contains("pattern")) {
        const auto res0 = fromJson<GlobPattern>("pattern", obj["pattern"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._pattern = *res0;
    }
    return result;
}

QJsonObject toJson(const TextDocumentFilterLanguage &data)
{
    QJsonObject obj{{"language", data._language}};
    if (data._scheme.has_value())
        obj.insert("scheme", *data._scheme);
    if (data._pattern.has_value())
        obj.insert("pattern", toJsonValue(*data._pattern));
    return obj;
}

template<>
Utils::Result<TextDocumentFilterPattern> fromJson<TextDocumentFilterPattern>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for TextDocumentFilterPattern");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("pattern"))
        return Utils::ResultError("Missing required field: pattern");
    TextDocumentFilterPattern result;
    if (obj.contains("language"))
        result._language = obj.value("language").toString();
    if (obj.contains("scheme"))
        result._scheme = obj.value("scheme").toString();
    if (obj.contains("pattern")) {
        const auto res0 = fromJson<GlobPattern>("pattern", obj["pattern"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._pattern = *res0;
    }
    return result;
}

QJsonObject toJson(const TextDocumentFilterPattern &data)
{
    QJsonObject obj{{"pattern", toJsonValue(data._pattern)}};
    if (data._language.has_value())
        obj.insert("language", *data._language);
    if (data._scheme.has_value())
        obj.insert("scheme", *data._scheme);
    return obj;
}

template<>
Utils::Result<TextDocumentFilterScheme> fromJson<TextDocumentFilterScheme>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for TextDocumentFilterScheme");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("scheme"))
        return Utils::ResultError("Missing required field: scheme");
    TextDocumentFilterScheme result;
    if (obj.contains("language"))
        result._language = obj.value("language").toString();
    result._scheme = obj.value("scheme").toString();
    if (obj.contains("pattern")) {
        const auto res0 = fromJson<GlobPattern>("pattern", obj["pattern"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._pattern = *res0;
    }
    return result;
}

QJsonObject toJson(const TextDocumentFilterScheme &data)
{
    QJsonObject obj{{"scheme", data._scheme}};
    if (data._language.has_value())
        obj.insert("language", *data._language);
    if (data._pattern.has_value())
        obj.insert("pattern", toJsonValue(*data._pattern));
    return obj;
}

template<>
Utils::Result<TextDocumentFilter> fromJson<TextDocumentFilter>(const QJsonValue &val)
{
    if (val.isObject()) {
        auto result = fromJson<TextDocumentFilterLanguage>(val);
        if (result) return TextDocumentFilter(*result);
    }
    if (val.isObject()) {
        auto result = fromJson<TextDocumentFilterScheme>(val);
        if (result) return TextDocumentFilter(*result);
    }
    if (val.isObject()) {
        auto result = fromJson<TextDocumentFilterPattern>(val);
        if (result) return TextDocumentFilter(*result);
    }
    return Utils::ResultError("Invalid TextDocumentFilter");
}

QJsonObject toJson(const TextDocumentFilter &val)
{
    return std::visit([](const auto &v) -> QJsonObject {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, QJsonObject>) {
            return v;
        } else {
            return toJson(v);
        }
    }, val);
}

QJsonValue toJsonValue(const TextDocumentFilter &val)
{
    return toJson(val);
}

template<>
Utils::Result<DocumentFilter> fromJson<DocumentFilter>(const QJsonValue &val)
{
    if (!val.isObject()) {
        {
            auto result = fromJson<TextDocumentFilter>(val);
            if (result) return DocumentFilter(*result);
        }
        return Utils::ResultError("Invalid DocumentFilter: expected object");
    }
    const QJsonObject obj = val.toObject();
    if (obj.contains("notebook")) {
        const auto res0 = fromJson<NotebookCellTextDocumentFilter>(val);
        if (!res0)
            return Utils::ResultError(res0.error());
        return DocumentFilter(*res0);
    }
    {
        auto result = fromJson<TextDocumentFilter>(val);
        if (result) return DocumentFilter(*result);
    }
    return Utils::ResultError("Invalid DocumentFilter");
}

QJsonObject toJson(const DocumentFilter &val)
{
    return std::visit([](const auto &v) -> QJsonObject {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, QJsonObject>) {
            return v;
        } else {
            return toJson(v);
        }
    }, val);
}

QJsonValue toJsonValue(const DocumentFilter &val)
{
    return toJson(val);
}

template<>
Utils::Result<DocumentSelector> fromJson<DocumentSelector>(const QJsonValue &val)
{
    if (!val.isArray())
        return Utils::ResultError("Expected JSON array for DocumentSelector");
    DocumentSelector result;
    for (const QJsonValue &v : val.toArray()) {
        const auto res0 = fromJson<DocumentFilter>(v);
        if (!res0)
            return Utils::ResultError(res0.error());
        result.append(*res0);
    }
    return result;
}

QJsonArray toJson(const DocumentSelector &data)
{
    QJsonArray arr;
    for (const auto &v : data) arr.append(toJsonValue(v));
    return arr;
}

template<>
Utils::Result<ImplementationRegistrationOptions> fromJson<ImplementationRegistrationOptions>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for ImplementationRegistrationOptions");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("documentSelector"))
        return Utils::ResultError("Missing required field: documentSelector");
    ImplementationRegistrationOptions result;
    if (obj.contains("documentSelector") && !obj["documentSelector"].isNull()) {
        const auto res0 = fromJson<DocumentSelector>("documentSelector", obj["documentSelector"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._documentSelector = *res0;
    }
    if (obj.contains("workDoneProgress"))
        result._workDoneProgress = obj.value("workDoneProgress").toBool();
    if (obj.contains("id"))
        result._id = obj.value("id").toString();
    return result;
}

QJsonObject toJson(const ImplementationRegistrationOptions &data)
{
    QJsonObject obj;
    if (data._documentSelector.has_value())
        obj.insert("documentSelector", toJson(*data._documentSelector));
    else
        obj.insert("documentSelector", QJsonValue::Null);
    if (data._workDoneProgress.has_value())
        obj.insert("workDoneProgress", *data._workDoneProgress);
    if (data._id.has_value())
        obj.insert("id", *data._id);
    return obj;
}

template<>
Utils::Result<TypeDefinitionParams> fromJson<TypeDefinitionParams>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for TypeDefinitionParams");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("textDocument"))
        return Utils::ResultError("Missing required field: textDocument");
    if (!obj.contains("position"))
        return Utils::ResultError("Missing required field: position");
    TypeDefinitionParams result;
    if (obj.contains("textDocument") && obj["textDocument"].isObject()) {
        const auto res0 = fromJson<TextDocumentIdentifier>("textDocument", obj["textDocument"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._textDocument = *res0;
    }
    if (obj.contains("position") && obj["position"].isObject()) {
        const auto res1 = fromJson<Position>("position", obj["position"]);
        if (!res1)
            return Utils::ResultError(res1.error());
        result._position = *res1;
    }
    if (obj.contains("workDoneToken")) {
        const auto res2 = fromJson<ProgressToken>("workDoneToken", obj["workDoneToken"]);
        if (!res2)
            return Utils::ResultError(res2.error());
        result._workDoneToken = *res2;
    }
    if (obj.contains("partialResultToken")) {
        const auto res3 = fromJson<ProgressToken>("partialResultToken", obj["partialResultToken"]);
        if (!res3)
            return Utils::ResultError(res3.error());
        result._partialResultToken = *res3;
    }
    return result;
}

QJsonObject toJson(const TypeDefinitionParams &data)
{
    QJsonObject obj{
        {"textDocument", toJson(data._textDocument)},
        {"position", toJson(data._position)}
    };
    if (data._workDoneToken.has_value())
        obj.insert("workDoneToken", toJsonValue(*data._workDoneToken));
    if (data._partialResultToken.has_value())
        obj.insert("partialResultToken", toJsonValue(*data._partialResultToken));
    return obj;
}

template<>
Utils::Result<TypeDefinitionRegistrationOptions> fromJson<TypeDefinitionRegistrationOptions>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for TypeDefinitionRegistrationOptions");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("documentSelector"))
        return Utils::ResultError("Missing required field: documentSelector");
    TypeDefinitionRegistrationOptions result;
    if (obj.contains("documentSelector") && !obj["documentSelector"].isNull()) {
        const auto res0 = fromJson<DocumentSelector>("documentSelector", obj["documentSelector"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._documentSelector = *res0;
    }
    if (obj.contains("workDoneProgress"))
        result._workDoneProgress = obj.value("workDoneProgress").toBool();
    if (obj.contains("id"))
        result._id = obj.value("id").toString();
    return result;
}

QJsonObject toJson(const TypeDefinitionRegistrationOptions &data)
{
    QJsonObject obj;
    if (data._documentSelector.has_value())
        obj.insert("documentSelector", toJson(*data._documentSelector));
    else
        obj.insert("documentSelector", QJsonValue::Null);
    if (data._workDoneProgress.has_value())
        obj.insert("workDoneProgress", *data._workDoneProgress);
    if (data._id.has_value())
        obj.insert("id", *data._id);
    return obj;
}

template<>
Utils::Result<WorkspaceFoldersChangeEvent> fromJson<WorkspaceFoldersChangeEvent>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for WorkspaceFoldersChangeEvent");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("added"))
        return Utils::ResultError("Missing required field: added");
    if (!obj.contains("removed"))
        return Utils::ResultError("Missing required field: removed");
    WorkspaceFoldersChangeEvent result;
    if (obj.contains("added") && obj["added"].isArray()) {
        const QJsonArray arr = obj["added"].toArray();
        for (const QJsonValue &v : arr) {
            const auto res0 = fromJson<WorkspaceFolder>("added", v);
            if (!res0)
                return Utils::ResultError(res0.error());
            result._added.append(*res0);
        }
    }
    if (obj.contains("removed") && obj["removed"].isArray()) {
        const QJsonArray arr = obj["removed"].toArray();
        for (const QJsonValue &v : arr) {
            const auto res1 = fromJson<WorkspaceFolder>("removed", v);
            if (!res1)
                return Utils::ResultError(res1.error());
            result._removed.append(*res1);
        }
    }
    return result;
}

QJsonObject toJson(const WorkspaceFoldersChangeEvent &data)
{
    QJsonObject obj;
    QJsonArray arr_added;
    for (const auto &v : data._added) arr_added.append(toJson(v));
    obj.insert("added", arr_added);
    QJsonArray arr_removed;
    for (const auto &v : data._removed) arr_removed.append(toJson(v));
    obj.insert("removed", arr_removed);
    return obj;
}

template<>
Utils::Result<DidChangeWorkspaceFoldersParams> fromJson<DidChangeWorkspaceFoldersParams>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for DidChangeWorkspaceFoldersParams");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("event"))
        return Utils::ResultError("Missing required field: event");
    DidChangeWorkspaceFoldersParams result;
    if (obj.contains("event") && obj["event"].isObject()) {
        const auto res0 = fromJson<WorkspaceFoldersChangeEvent>("event", obj["event"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._event = *res0;
    }
    return result;
}

QJsonObject toJson(const DidChangeWorkspaceFoldersParams &data)
{
    QJsonObject obj{{"event", toJson(data._event)}};
    return obj;
}

template<>
Utils::Result<ConfigurationItem> fromJson<ConfigurationItem>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for ConfigurationItem");
    const QJsonObject obj = val.toObject();
    ConfigurationItem result;
    if (obj.contains("scopeUri"))
        result._scopeUri = obj.value("scopeUri").toString();
    if (obj.contains("section"))
        result._section = obj.value("section").toString();
    return result;
}

QJsonObject toJson(const ConfigurationItem &data)
{
    QJsonObject obj;
    if (data._scopeUri.has_value())
        obj.insert("scopeUri", *data._scopeUri);
    if (data._section.has_value())
        obj.insert("section", *data._section);
    return obj;
}

template<>
Utils::Result<ConfigurationParams> fromJson<ConfigurationParams>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for ConfigurationParams");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("items"))
        return Utils::ResultError("Missing required field: items");
    ConfigurationParams result;
    if (obj.contains("items") && obj["items"].isArray()) {
        const QJsonArray arr = obj["items"].toArray();
        for (const QJsonValue &v : arr) {
            const auto res0 = fromJson<ConfigurationItem>("items", v);
            if (!res0)
                return Utils::ResultError(res0.error());
            result._items.append(*res0);
        }
    }
    return result;
}

QJsonObject toJson(const ConfigurationParams &data)
{
    QJsonObject obj;
    QJsonArray arr_items;
    for (const auto &v : data._items) arr_items.append(toJson(v));
    obj.insert("items", arr_items);
    return obj;
}

template<>
Utils::Result<DocumentColorParams> fromJson<DocumentColorParams>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for DocumentColorParams");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("textDocument"))
        return Utils::ResultError("Missing required field: textDocument");
    DocumentColorParams result;
    if (obj.contains("workDoneToken")) {
        const auto res0 = fromJson<ProgressToken>("workDoneToken", obj["workDoneToken"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._workDoneToken = *res0;
    }
    if (obj.contains("partialResultToken")) {
        const auto res1 = fromJson<ProgressToken>("partialResultToken", obj["partialResultToken"]);
        if (!res1)
            return Utils::ResultError(res1.error());
        result._partialResultToken = *res1;
    }
    if (obj.contains("textDocument") && obj["textDocument"].isObject()) {
        const auto res2 = fromJson<TextDocumentIdentifier>("textDocument", obj["textDocument"]);
        if (!res2)
            return Utils::ResultError(res2.error());
        result._textDocument = *res2;
    }
    return result;
}

QJsonObject toJson(const DocumentColorParams &data)
{
    QJsonObject obj{{"textDocument", toJson(data._textDocument)}};
    if (data._workDoneToken.has_value())
        obj.insert("workDoneToken", toJsonValue(*data._workDoneToken));
    if (data._partialResultToken.has_value())
        obj.insert("partialResultToken", toJsonValue(*data._partialResultToken));
    return obj;
}

template<>
Utils::Result<Color> fromJson<Color>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for Color");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("red"))
        return Utils::ResultError("Missing required field: red");
    if (!obj.contains("green"))
        return Utils::ResultError("Missing required field: green");
    if (!obj.contains("blue"))
        return Utils::ResultError("Missing required field: blue");
    if (!obj.contains("alpha"))
        return Utils::ResultError("Missing required field: alpha");
    Color result;
    result._red = obj.value("red").toDouble();
    result._green = obj.value("green").toDouble();
    result._blue = obj.value("blue").toDouble();
    result._alpha = obj.value("alpha").toDouble();
    return result;
}

QJsonObject toJson(const Color &data)
{
    QJsonObject obj{
        {"red", data._red},
        {"green", data._green},
        {"blue", data._blue},
        {"alpha", data._alpha}
    };
    return obj;
}

template<>
Utils::Result<ColorInformation> fromJson<ColorInformation>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for ColorInformation");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("range"))
        return Utils::ResultError("Missing required field: range");
    if (!obj.contains("color"))
        return Utils::ResultError("Missing required field: color");
    ColorInformation result;
    if (obj.contains("range") && obj["range"].isObject()) {
        const auto res0 = fromJson<Range>("range", obj["range"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._range = *res0;
    }
    if (obj.contains("color") && obj["color"].isObject()) {
        const auto res1 = fromJson<Color>("color", obj["color"]);
        if (!res1)
            return Utils::ResultError(res1.error());
        result._color = *res1;
    }
    return result;
}

QJsonObject toJson(const ColorInformation &data)
{
    QJsonObject obj{
        {"range", toJson(data._range)},
        {"color", toJson(data._color)}
    };
    return obj;
}

template<>
Utils::Result<DocumentColorRegistrationOptions> fromJson<DocumentColorRegistrationOptions>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for DocumentColorRegistrationOptions");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("documentSelector"))
        return Utils::ResultError("Missing required field: documentSelector");
    DocumentColorRegistrationOptions result;
    if (obj.contains("documentSelector") && !obj["documentSelector"].isNull()) {
        const auto res0 = fromJson<DocumentSelector>("documentSelector", obj["documentSelector"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._documentSelector = *res0;
    }
    if (obj.contains("workDoneProgress"))
        result._workDoneProgress = obj.value("workDoneProgress").toBool();
    if (obj.contains("id"))
        result._id = obj.value("id").toString();
    return result;
}

QJsonObject toJson(const DocumentColorRegistrationOptions &data)
{
    QJsonObject obj;
    if (data._documentSelector.has_value())
        obj.insert("documentSelector", toJson(*data._documentSelector));
    else
        obj.insert("documentSelector", QJsonValue::Null);
    if (data._workDoneProgress.has_value())
        obj.insert("workDoneProgress", *data._workDoneProgress);
    if (data._id.has_value())
        obj.insert("id", *data._id);
    return obj;
}

template<>
Utils::Result<ColorPresentationParams> fromJson<ColorPresentationParams>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for ColorPresentationParams");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("textDocument"))
        return Utils::ResultError("Missing required field: textDocument");
    if (!obj.contains("color"))
        return Utils::ResultError("Missing required field: color");
    if (!obj.contains("range"))
        return Utils::ResultError("Missing required field: range");
    ColorPresentationParams result;
    if (obj.contains("workDoneToken")) {
        const auto res0 = fromJson<ProgressToken>("workDoneToken", obj["workDoneToken"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._workDoneToken = *res0;
    }
    if (obj.contains("partialResultToken")) {
        const auto res1 = fromJson<ProgressToken>("partialResultToken", obj["partialResultToken"]);
        if (!res1)
            return Utils::ResultError(res1.error());
        result._partialResultToken = *res1;
    }
    if (obj.contains("textDocument") && obj["textDocument"].isObject()) {
        const auto res2 = fromJson<TextDocumentIdentifier>("textDocument", obj["textDocument"]);
        if (!res2)
            return Utils::ResultError(res2.error());
        result._textDocument = *res2;
    }
    if (obj.contains("color") && obj["color"].isObject()) {
        const auto res3 = fromJson<Color>("color", obj["color"]);
        if (!res3)
            return Utils::ResultError(res3.error());
        result._color = *res3;
    }
    if (obj.contains("range") && obj["range"].isObject()) {
        const auto res4 = fromJson<Range>("range", obj["range"]);
        if (!res4)
            return Utils::ResultError(res4.error());
        result._range = *res4;
    }
    return result;
}

QJsonObject toJson(const ColorPresentationParams &data)
{
    QJsonObject obj{
        {"textDocument", toJson(data._textDocument)},
        {"color", toJson(data._color)},
        {"range", toJson(data._range)}
    };
    if (data._workDoneToken.has_value())
        obj.insert("workDoneToken", toJsonValue(*data._workDoneToken));
    if (data._partialResultToken.has_value())
        obj.insert("partialResultToken", toJsonValue(*data._partialResultToken));
    return obj;
}

template<>
Utils::Result<TextEdit> fromJson<TextEdit>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for TextEdit");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("range"))
        return Utils::ResultError("Missing required field: range");
    if (!obj.contains("newText"))
        return Utils::ResultError("Missing required field: newText");
    TextEdit result;
    if (obj.contains("range") && obj["range"].isObject()) {
        const auto res0 = fromJson<Range>("range", obj["range"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._range = *res0;
    }
    result._newText = obj.value("newText").toString();
    return result;
}

QJsonObject toJson(const TextEdit &data)
{
    QJsonObject obj{
        {"range", toJson(data._range)},
        {"newText", data._newText}
    };
    return obj;
}

template<>
Utils::Result<ColorPresentation> fromJson<ColorPresentation>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for ColorPresentation");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("label"))
        return Utils::ResultError("Missing required field: label");
    ColorPresentation result;
    result._label = obj.value("label").toString();
    if (obj.contains("textEdit") && obj["textEdit"].isObject()) {
        const auto res0 = fromJson<TextEdit>("textEdit", obj["textEdit"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._textEdit = *res0;
    }
    if (obj.contains("additionalTextEdits") && obj["additionalTextEdits"].isArray()) {
        const QJsonArray arr = obj["additionalTextEdits"].toArray();
        QList<TextEdit> list_additionalTextEdits;
        for (const QJsonValue &v : arr) {
            const auto res1 = fromJson<TextEdit>("additionalTextEdits", v);
            if (!res1)
                return Utils::ResultError(res1.error());
            list_additionalTextEdits.append(*res1);
        }
        result._additionalTextEdits = list_additionalTextEdits;
    }
    return result;
}

QJsonObject toJson(const ColorPresentation &data)
{
    QJsonObject obj{{"label", data._label}};
    if (data._textEdit.has_value())
        obj.insert("textEdit", toJson(*data._textEdit));
    if (data._additionalTextEdits.has_value()) {
        QJsonArray arr_additionalTextEdits;
        for (const auto &v : *data._additionalTextEdits) arr_additionalTextEdits.append(toJson(v));
        obj.insert("additionalTextEdits", arr_additionalTextEdits);
    }
    return obj;
}

template<>
Utils::Result<FoldingRangeParams> fromJson<FoldingRangeParams>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for FoldingRangeParams");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("textDocument"))
        return Utils::ResultError("Missing required field: textDocument");
    FoldingRangeParams result;
    if (obj.contains("workDoneToken")) {
        const auto res0 = fromJson<ProgressToken>("workDoneToken", obj["workDoneToken"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._workDoneToken = *res0;
    }
    if (obj.contains("partialResultToken")) {
        const auto res1 = fromJson<ProgressToken>("partialResultToken", obj["partialResultToken"]);
        if (!res1)
            return Utils::ResultError(res1.error());
        result._partialResultToken = *res1;
    }
    if (obj.contains("textDocument") && obj["textDocument"].isObject()) {
        const auto res2 = fromJson<TextDocumentIdentifier>("textDocument", obj["textDocument"]);
        if (!res2)
            return Utils::ResultError(res2.error());
        result._textDocument = *res2;
    }
    return result;
}

QJsonObject toJson(const FoldingRangeParams &data)
{
    QJsonObject obj{{"textDocument", toJson(data._textDocument)}};
    if (data._workDoneToken.has_value())
        obj.insert("workDoneToken", toJsonValue(*data._workDoneToken));
    if (data._partialResultToken.has_value())
        obj.insert("partialResultToken", toJsonValue(*data._partialResultToken));
    return obj;
}

template<>
Utils::Result<FoldingRange> fromJson<FoldingRange>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for FoldingRange");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("startLine"))
        return Utils::ResultError("Missing required field: startLine");
    if (!obj.contains("endLine"))
        return Utils::ResultError("Missing required field: endLine");
    FoldingRange result;
    result._startLine = obj.value("startLine").toInt();
    if (obj.contains("startCharacter"))
        result._startCharacter = obj.value("startCharacter").toInt();
    result._endLine = obj.value("endLine").toInt();
    if (obj.contains("endCharacter"))
        result._endCharacter = obj.value("endCharacter").toInt();
    if (obj.contains("kind") && obj["kind"].isString()) {
        const auto res0 = fromJson<FoldingRangeKind>("kind", obj["kind"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._kind = *res0;
    }
    if (obj.contains("collapsedText"))
        result._collapsedText = obj.value("collapsedText").toString();
    return result;
}

QJsonObject toJson(const FoldingRange &data)
{
    QJsonObject obj{
        {"startLine", data._startLine},
        {"endLine", data._endLine}
    };
    if (data._startCharacter.has_value())
        obj.insert("startCharacter", *data._startCharacter);
    if (data._endCharacter.has_value())
        obj.insert("endCharacter", *data._endCharacter);
    if (data._kind.has_value())
        obj.insert("kind", *data._kind);
    if (data._collapsedText.has_value())
        obj.insert("collapsedText", *data._collapsedText);
    return obj;
}

template<>
Utils::Result<FoldingRangeRegistrationOptions> fromJson<FoldingRangeRegistrationOptions>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for FoldingRangeRegistrationOptions");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("documentSelector"))
        return Utils::ResultError("Missing required field: documentSelector");
    FoldingRangeRegistrationOptions result;
    if (obj.contains("documentSelector") && !obj["documentSelector"].isNull()) {
        const auto res0 = fromJson<DocumentSelector>("documentSelector", obj["documentSelector"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._documentSelector = *res0;
    }
    if (obj.contains("workDoneProgress"))
        result._workDoneProgress = obj.value("workDoneProgress").toBool();
    if (obj.contains("id"))
        result._id = obj.value("id").toString();
    return result;
}

QJsonObject toJson(const FoldingRangeRegistrationOptions &data)
{
    QJsonObject obj;
    if (data._documentSelector.has_value())
        obj.insert("documentSelector", toJson(*data._documentSelector));
    else
        obj.insert("documentSelector", QJsonValue::Null);
    if (data._workDoneProgress.has_value())
        obj.insert("workDoneProgress", *data._workDoneProgress);
    if (data._id.has_value())
        obj.insert("id", *data._id);
    return obj;
}

template<>
Utils::Result<DeclarationParams> fromJson<DeclarationParams>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for DeclarationParams");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("textDocument"))
        return Utils::ResultError("Missing required field: textDocument");
    if (!obj.contains("position"))
        return Utils::ResultError("Missing required field: position");
    DeclarationParams result;
    if (obj.contains("textDocument") && obj["textDocument"].isObject()) {
        const auto res0 = fromJson<TextDocumentIdentifier>("textDocument", obj["textDocument"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._textDocument = *res0;
    }
    if (obj.contains("position") && obj["position"].isObject()) {
        const auto res1 = fromJson<Position>("position", obj["position"]);
        if (!res1)
            return Utils::ResultError(res1.error());
        result._position = *res1;
    }
    if (obj.contains("workDoneToken")) {
        const auto res2 = fromJson<ProgressToken>("workDoneToken", obj["workDoneToken"]);
        if (!res2)
            return Utils::ResultError(res2.error());
        result._workDoneToken = *res2;
    }
    if (obj.contains("partialResultToken")) {
        const auto res3 = fromJson<ProgressToken>("partialResultToken", obj["partialResultToken"]);
        if (!res3)
            return Utils::ResultError(res3.error());
        result._partialResultToken = *res3;
    }
    return result;
}

QJsonObject toJson(const DeclarationParams &data)
{
    QJsonObject obj{
        {"textDocument", toJson(data._textDocument)},
        {"position", toJson(data._position)}
    };
    if (data._workDoneToken.has_value())
        obj.insert("workDoneToken", toJsonValue(*data._workDoneToken));
    if (data._partialResultToken.has_value())
        obj.insert("partialResultToken", toJsonValue(*data._partialResultToken));
    return obj;
}

template<>
Utils::Result<DeclarationRegistrationOptions> fromJson<DeclarationRegistrationOptions>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for DeclarationRegistrationOptions");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("documentSelector"))
        return Utils::ResultError("Missing required field: documentSelector");
    DeclarationRegistrationOptions result;
    if (obj.contains("workDoneProgress"))
        result._workDoneProgress = obj.value("workDoneProgress").toBool();
    if (obj.contains("documentSelector") && !obj["documentSelector"].isNull()) {
        const auto res0 = fromJson<DocumentSelector>("documentSelector", obj["documentSelector"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._documentSelector = *res0;
    }
    if (obj.contains("id"))
        result._id = obj.value("id").toString();
    return result;
}

QJsonObject toJson(const DeclarationRegistrationOptions &data)
{
    QJsonObject obj;
    if (data._workDoneProgress.has_value())
        obj.insert("workDoneProgress", *data._workDoneProgress);
    if (data._documentSelector.has_value())
        obj.insert("documentSelector", toJson(*data._documentSelector));
    else
        obj.insert("documentSelector", QJsonValue::Null);
    if (data._id.has_value())
        obj.insert("id", *data._id);
    return obj;
}

template<>
Utils::Result<SelectionRangeParams> fromJson<SelectionRangeParams>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for SelectionRangeParams");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("textDocument"))
        return Utils::ResultError("Missing required field: textDocument");
    if (!obj.contains("positions"))
        return Utils::ResultError("Missing required field: positions");
    SelectionRangeParams result;
    if (obj.contains("workDoneToken")) {
        const auto res0 = fromJson<ProgressToken>("workDoneToken", obj["workDoneToken"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._workDoneToken = *res0;
    }
    if (obj.contains("partialResultToken")) {
        const auto res1 = fromJson<ProgressToken>("partialResultToken", obj["partialResultToken"]);
        if (!res1)
            return Utils::ResultError(res1.error());
        result._partialResultToken = *res1;
    }
    if (obj.contains("textDocument") && obj["textDocument"].isObject()) {
        const auto res2 = fromJson<TextDocumentIdentifier>("textDocument", obj["textDocument"]);
        if (!res2)
            return Utils::ResultError(res2.error());
        result._textDocument = *res2;
    }
    if (obj.contains("positions") && obj["positions"].isArray()) {
        const QJsonArray arr = obj["positions"].toArray();
        for (const QJsonValue &v : arr) {
            const auto res3 = fromJson<Position>("positions", v);
            if (!res3)
                return Utils::ResultError(res3.error());
            result._positions.append(*res3);
        }
    }
    return result;
}

QJsonObject toJson(const SelectionRangeParams &data)
{
    QJsonObject obj{{"textDocument", toJson(data._textDocument)}};
    if (data._workDoneToken.has_value())
        obj.insert("workDoneToken", toJsonValue(*data._workDoneToken));
    if (data._partialResultToken.has_value())
        obj.insert("partialResultToken", toJsonValue(*data._partialResultToken));
    QJsonArray arr_positions;
    for (const auto &v : data._positions) arr_positions.append(toJson(v));
    obj.insert("positions", arr_positions);
    return obj;
}

bool SelectionRange::operator==(const SelectionRange &other) const = default;

template<>
Utils::Result<SelectionRange> fromJson<SelectionRange>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for SelectionRange");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("range"))
        return Utils::ResultError("Missing required field: range");
    SelectionRange result;
    if (obj.contains("range") && obj["range"].isObject()) {
        const auto res0 = fromJson<Range>("range", obj["range"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._range = *res0;
    }
    if (obj.contains("parent") && obj["parent"].isObject()) {
        const auto res1 = fromJson<SelectionRange>("parent", obj["parent"]);
        if (!res1)
            return Utils::ResultError(res1.error());
        result._parent = *res1;
    }
    return result;
}

QJsonObject toJson(const SelectionRange &data)
{
    QJsonObject obj{{"range", toJson(data._range)}};
    if (data._parent.has_value())
        obj.insert("parent", toJson(*data._parent));
    return obj;
}

template<>
Utils::Result<SelectionRangeRegistrationOptions> fromJson<SelectionRangeRegistrationOptions>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for SelectionRangeRegistrationOptions");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("documentSelector"))
        return Utils::ResultError("Missing required field: documentSelector");
    SelectionRangeRegistrationOptions result;
    if (obj.contains("workDoneProgress"))
        result._workDoneProgress = obj.value("workDoneProgress").toBool();
    if (obj.contains("documentSelector") && !obj["documentSelector"].isNull()) {
        const auto res0 = fromJson<DocumentSelector>("documentSelector", obj["documentSelector"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._documentSelector = *res0;
    }
    if (obj.contains("id"))
        result._id = obj.value("id").toString();
    return result;
}

QJsonObject toJson(const SelectionRangeRegistrationOptions &data)
{
    QJsonObject obj;
    if (data._workDoneProgress.has_value())
        obj.insert("workDoneProgress", *data._workDoneProgress);
    if (data._documentSelector.has_value())
        obj.insert("documentSelector", toJson(*data._documentSelector));
    else
        obj.insert("documentSelector", QJsonValue::Null);
    if (data._id.has_value())
        obj.insert("id", *data._id);
    return obj;
}

template<>
Utils::Result<WorkDoneProgressCreateParams> fromJson<WorkDoneProgressCreateParams>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for WorkDoneProgressCreateParams");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("token"))
        return Utils::ResultError("Missing required field: token");
    WorkDoneProgressCreateParams result;
    if (obj.contains("token")) {
        const auto res0 = fromJson<ProgressToken>("token", obj["token"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._token = *res0;
    }
    return result;
}

QJsonObject toJson(const WorkDoneProgressCreateParams &data)
{
    QJsonObject obj{{"token", toJsonValue(data._token)}};
    return obj;
}

template<>
Utils::Result<WorkDoneProgressCancelParams> fromJson<WorkDoneProgressCancelParams>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for WorkDoneProgressCancelParams");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("token"))
        return Utils::ResultError("Missing required field: token");
    WorkDoneProgressCancelParams result;
    if (obj.contains("token")) {
        const auto res0 = fromJson<ProgressToken>("token", obj["token"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._token = *res0;
    }
    return result;
}

QJsonObject toJson(const WorkDoneProgressCancelParams &data)
{
    QJsonObject obj{{"token", toJsonValue(data._token)}};
    return obj;
}

template<>
Utils::Result<CallHierarchyPrepareParams> fromJson<CallHierarchyPrepareParams>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for CallHierarchyPrepareParams");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("textDocument"))
        return Utils::ResultError("Missing required field: textDocument");
    if (!obj.contains("position"))
        return Utils::ResultError("Missing required field: position");
    CallHierarchyPrepareParams result;
    if (obj.contains("textDocument") && obj["textDocument"].isObject()) {
        const auto res0 = fromJson<TextDocumentIdentifier>("textDocument", obj["textDocument"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._textDocument = *res0;
    }
    if (obj.contains("position") && obj["position"].isObject()) {
        const auto res1 = fromJson<Position>("position", obj["position"]);
        if (!res1)
            return Utils::ResultError(res1.error());
        result._position = *res1;
    }
    if (obj.contains("workDoneToken")) {
        const auto res2 = fromJson<ProgressToken>("workDoneToken", obj["workDoneToken"]);
        if (!res2)
            return Utils::ResultError(res2.error());
        result._workDoneToken = *res2;
    }
    return result;
}

QJsonObject toJson(const CallHierarchyPrepareParams &data)
{
    QJsonObject obj{
        {"textDocument", toJson(data._textDocument)},
        {"position", toJson(data._position)}
    };
    if (data._workDoneToken.has_value())
        obj.insert("workDoneToken", toJsonValue(*data._workDoneToken));
    return obj;
}

template<>
Utils::Result<CallHierarchyItem> fromJson<CallHierarchyItem>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for CallHierarchyItem");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("name"))
        return Utils::ResultError("Missing required field: name");
    if (!obj.contains("kind"))
        return Utils::ResultError("Missing required field: kind");
    if (!obj.contains("uri"))
        return Utils::ResultError("Missing required field: uri");
    if (!obj.contains("range"))
        return Utils::ResultError("Missing required field: range");
    if (!obj.contains("selectionRange"))
        return Utils::ResultError("Missing required field: selectionRange");
    CallHierarchyItem result;
    result._name = obj.value("name").toString();
    result._kind = obj.value("kind").toInt();
    if (obj.contains("tags") && obj["tags"].isArray()) {
        const QJsonArray arr = obj["tags"].toArray();
        QList<int> list_tags;
        for (const QJsonValue &v : arr) {
            list_tags.append(v.toInt());
        }
        result._tags = list_tags;
    }
    if (obj.contains("detail"))
        result._detail = obj.value("detail").toString();
    result._uri = obj.value("uri").toString();
    if (obj.contains("range") && obj["range"].isObject()) {
        const auto res0 = fromJson<Range>("range", obj["range"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._range = *res0;
    }
    if (obj.contains("selectionRange") && obj["selectionRange"].isObject()) {
        const auto res1 = fromJson<Range>("selectionRange", obj["selectionRange"]);
        if (!res1)
            return Utils::ResultError(res1.error());
        result._selectionRange = *res1;
    }
    if (obj.contains("data"))
        result._data = obj.value("data");
    return result;
}

QJsonObject toJson(const CallHierarchyItem &data)
{
    QJsonObject obj{
        {"name", data._name},
        {"kind", data._kind},
        {"uri", data._uri},
        {"range", toJson(data._range)},
        {"selectionRange", toJson(data._selectionRange)}
    };
    if (data._tags.has_value()) {
        QJsonArray arr_tags;
        for (const auto &v : *data._tags) arr_tags.append(v);
        obj.insert("tags", arr_tags);
    }
    if (data._detail.has_value())
        obj.insert("detail", *data._detail);
    if (data._data.has_value())
        obj.insert("data", *data._data);
    return obj;
}

template<>
Utils::Result<CallHierarchyRegistrationOptions> fromJson<CallHierarchyRegistrationOptions>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for CallHierarchyRegistrationOptions");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("documentSelector"))
        return Utils::ResultError("Missing required field: documentSelector");
    CallHierarchyRegistrationOptions result;
    if (obj.contains("documentSelector") && !obj["documentSelector"].isNull()) {
        const auto res0 = fromJson<DocumentSelector>("documentSelector", obj["documentSelector"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._documentSelector = *res0;
    }
    if (obj.contains("workDoneProgress"))
        result._workDoneProgress = obj.value("workDoneProgress").toBool();
    if (obj.contains("id"))
        result._id = obj.value("id").toString();
    return result;
}

QJsonObject toJson(const CallHierarchyRegistrationOptions &data)
{
    QJsonObject obj;
    if (data._documentSelector.has_value())
        obj.insert("documentSelector", toJson(*data._documentSelector));
    else
        obj.insert("documentSelector", QJsonValue::Null);
    if (data._workDoneProgress.has_value())
        obj.insert("workDoneProgress", *data._workDoneProgress);
    if (data._id.has_value())
        obj.insert("id", *data._id);
    return obj;
}

template<>
Utils::Result<CallHierarchyIncomingCallsParams> fromJson<CallHierarchyIncomingCallsParams>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for CallHierarchyIncomingCallsParams");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("item"))
        return Utils::ResultError("Missing required field: item");
    CallHierarchyIncomingCallsParams result;
    if (obj.contains("workDoneToken")) {
        const auto res0 = fromJson<ProgressToken>("workDoneToken", obj["workDoneToken"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._workDoneToken = *res0;
    }
    if (obj.contains("partialResultToken")) {
        const auto res1 = fromJson<ProgressToken>("partialResultToken", obj["partialResultToken"]);
        if (!res1)
            return Utils::ResultError(res1.error());
        result._partialResultToken = *res1;
    }
    if (obj.contains("item") && obj["item"].isObject()) {
        const auto res2 = fromJson<CallHierarchyItem>("item", obj["item"]);
        if (!res2)
            return Utils::ResultError(res2.error());
        result._item = *res2;
    }
    return result;
}

QJsonObject toJson(const CallHierarchyIncomingCallsParams &data)
{
    QJsonObject obj{{"item", toJson(data._item)}};
    if (data._workDoneToken.has_value())
        obj.insert("workDoneToken", toJsonValue(*data._workDoneToken));
    if (data._partialResultToken.has_value())
        obj.insert("partialResultToken", toJsonValue(*data._partialResultToken));
    return obj;
}

template<>
Utils::Result<CallHierarchyIncomingCall> fromJson<CallHierarchyIncomingCall>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for CallHierarchyIncomingCall");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("from"))
        return Utils::ResultError("Missing required field: from");
    if (!obj.contains("fromRanges"))
        return Utils::ResultError("Missing required field: fromRanges");
    CallHierarchyIncomingCall result;
    if (obj.contains("from") && obj["from"].isObject()) {
        const auto res0 = fromJson<CallHierarchyItem>("from", obj["from"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._from = *res0;
    }
    if (obj.contains("fromRanges") && obj["fromRanges"].isArray()) {
        const QJsonArray arr = obj["fromRanges"].toArray();
        for (const QJsonValue &v : arr) {
            const auto res1 = fromJson<Range>("fromRanges", v);
            if (!res1)
                return Utils::ResultError(res1.error());
            result._fromRanges.append(*res1);
        }
    }
    return result;
}

QJsonObject toJson(const CallHierarchyIncomingCall &data)
{
    QJsonObject obj{{"from", toJson(data._from)}};
    QJsonArray arr_fromRanges;
    for (const auto &v : data._fromRanges) arr_fromRanges.append(toJson(v));
    obj.insert("fromRanges", arr_fromRanges);
    return obj;
}

template<>
Utils::Result<CallHierarchyOutgoingCallsParams> fromJson<CallHierarchyOutgoingCallsParams>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for CallHierarchyOutgoingCallsParams");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("item"))
        return Utils::ResultError("Missing required field: item");
    CallHierarchyOutgoingCallsParams result;
    if (obj.contains("workDoneToken")) {
        const auto res0 = fromJson<ProgressToken>("workDoneToken", obj["workDoneToken"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._workDoneToken = *res0;
    }
    if (obj.contains("partialResultToken")) {
        const auto res1 = fromJson<ProgressToken>("partialResultToken", obj["partialResultToken"]);
        if (!res1)
            return Utils::ResultError(res1.error());
        result._partialResultToken = *res1;
    }
    if (obj.contains("item") && obj["item"].isObject()) {
        const auto res2 = fromJson<CallHierarchyItem>("item", obj["item"]);
        if (!res2)
            return Utils::ResultError(res2.error());
        result._item = *res2;
    }
    return result;
}

QJsonObject toJson(const CallHierarchyOutgoingCallsParams &data)
{
    QJsonObject obj{{"item", toJson(data._item)}};
    if (data._workDoneToken.has_value())
        obj.insert("workDoneToken", toJsonValue(*data._workDoneToken));
    if (data._partialResultToken.has_value())
        obj.insert("partialResultToken", toJsonValue(*data._partialResultToken));
    return obj;
}

template<>
Utils::Result<CallHierarchyOutgoingCall> fromJson<CallHierarchyOutgoingCall>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for CallHierarchyOutgoingCall");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("to"))
        return Utils::ResultError("Missing required field: to");
    if (!obj.contains("fromRanges"))
        return Utils::ResultError("Missing required field: fromRanges");
    CallHierarchyOutgoingCall result;
    if (obj.contains("to") && obj["to"].isObject()) {
        const auto res0 = fromJson<CallHierarchyItem>("to", obj["to"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._to = *res0;
    }
    if (obj.contains("fromRanges") && obj["fromRanges"].isArray()) {
        const QJsonArray arr = obj["fromRanges"].toArray();
        for (const QJsonValue &v : arr) {
            const auto res1 = fromJson<Range>("fromRanges", v);
            if (!res1)
                return Utils::ResultError(res1.error());
            result._fromRanges.append(*res1);
        }
    }
    return result;
}

QJsonObject toJson(const CallHierarchyOutgoingCall &data)
{
    QJsonObject obj{{"to", toJson(data._to)}};
    QJsonArray arr_fromRanges;
    for (const auto &v : data._fromRanges) arr_fromRanges.append(toJson(v));
    obj.insert("fromRanges", arr_fromRanges);
    return obj;
}

template<>
Utils::Result<SemanticTokensParams> fromJson<SemanticTokensParams>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for SemanticTokensParams");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("textDocument"))
        return Utils::ResultError("Missing required field: textDocument");
    SemanticTokensParams result;
    if (obj.contains("workDoneToken")) {
        const auto res0 = fromJson<ProgressToken>("workDoneToken", obj["workDoneToken"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._workDoneToken = *res0;
    }
    if (obj.contains("partialResultToken")) {
        const auto res1 = fromJson<ProgressToken>("partialResultToken", obj["partialResultToken"]);
        if (!res1)
            return Utils::ResultError(res1.error());
        result._partialResultToken = *res1;
    }
    if (obj.contains("textDocument") && obj["textDocument"].isObject()) {
        const auto res2 = fromJson<TextDocumentIdentifier>("textDocument", obj["textDocument"]);
        if (!res2)
            return Utils::ResultError(res2.error());
        result._textDocument = *res2;
    }
    return result;
}

QJsonObject toJson(const SemanticTokensParams &data)
{
    QJsonObject obj{{"textDocument", toJson(data._textDocument)}};
    if (data._workDoneToken.has_value())
        obj.insert("workDoneToken", toJsonValue(*data._workDoneToken));
    if (data._partialResultToken.has_value())
        obj.insert("partialResultToken", toJsonValue(*data._partialResultToken));
    return obj;
}

template<>
Utils::Result<SemanticTokens> fromJson<SemanticTokens>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for SemanticTokens");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("data"))
        return Utils::ResultError("Missing required field: data");
    SemanticTokens result;
    if (obj.contains("resultId"))
        result._resultId = obj.value("resultId").toString();
    if (obj.contains("data") && obj["data"].isArray()) {
        const QJsonArray arr = obj["data"].toArray();
        for (const QJsonValue &v : arr) {
            result._data.append(v.toInt());
        }
    }
    return result;
}

QJsonObject toJson(const SemanticTokens &data)
{
    QJsonObject obj;
    if (data._resultId.has_value())
        obj.insert("resultId", *data._resultId);
    QJsonArray arr_data;
    for (const auto &v : data._data) arr_data.append(v);
    obj.insert("data", arr_data);
    return obj;
}

template<>
Utils::Result<SemanticTokensPartialResult> fromJson<SemanticTokensPartialResult>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for SemanticTokensPartialResult");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("data"))
        return Utils::ResultError("Missing required field: data");
    SemanticTokensPartialResult result;
    if (obj.contains("data") && obj["data"].isArray()) {
        const QJsonArray arr = obj["data"].toArray();
        for (const QJsonValue &v : arr) {
            result._data.append(v.toInt());
        }
    }
    return result;
}

QJsonObject toJson(const SemanticTokensPartialResult &data)
{
    QJsonObject obj;
    QJsonArray arr_data;
    for (const auto &v : data._data) arr_data.append(v);
    obj.insert("data", arr_data);
    return obj;
}

template<>
Utils::Result<SemanticTokensFullDelta> fromJson<SemanticTokensFullDelta>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for SemanticTokensFullDelta");
    const QJsonObject obj = val.toObject();
    SemanticTokensFullDelta result;
    if (obj.contains("delta"))
        result._delta = obj.value("delta").toBool();
    return result;
}

QJsonObject toJson(const SemanticTokensFullDelta &data)
{
    QJsonObject obj;
    if (data._delta.has_value())
        obj.insert("delta", *data._delta);
    return obj;
}

template<>
Utils::Result<SemanticTokensLegend> fromJson<SemanticTokensLegend>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for SemanticTokensLegend");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("tokenTypes"))
        return Utils::ResultError("Missing required field: tokenTypes");
    if (!obj.contains("tokenModifiers"))
        return Utils::ResultError("Missing required field: tokenModifiers");
    SemanticTokensLegend result;
    if (obj.contains("tokenTypes") && obj["tokenTypes"].isArray()) {
        const QJsonArray arr = obj["tokenTypes"].toArray();
        for (const QJsonValue &v : arr) {
            result._tokenTypes.append(v.toString());
        }
    }
    if (obj.contains("tokenModifiers") && obj["tokenModifiers"].isArray()) {
        const QJsonArray arr = obj["tokenModifiers"].toArray();
        for (const QJsonValue &v : arr) {
            result._tokenModifiers.append(v.toString());
        }
    }
    return result;
}

QJsonObject toJson(const SemanticTokensLegend &data)
{
    QJsonObject obj;
    QJsonArray arr_tokenTypes;
    for (const auto &v : data._tokenTypes) arr_tokenTypes.append(v);
    obj.insert("tokenTypes", arr_tokenTypes);
    QJsonArray arr_tokenModifiers;
    for (const auto &v : data._tokenModifiers) arr_tokenModifiers.append(v);
    obj.insert("tokenModifiers", arr_tokenModifiers);
    return obj;
}

template<>
Utils::Result<SemanticTokensRegistrationOptionsRange> fromJson<SemanticTokensRegistrationOptionsRange>(const QJsonValue &val)
{
    if (val.isBool())
        return SemanticTokensRegistrationOptionsRange(val.toBool());
    if (val.isObject())
        return SemanticTokensRegistrationOptionsRange(val.toObject());
    return Utils::ResultError("Invalid SemanticTokensRegistrationOptionsRange");
}

QJsonValue toJsonValue(const SemanticTokensRegistrationOptionsRange &val)
{
    return std::visit([](const auto &v) -> QJsonValue {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, QJsonObject>) {
            return v;
        } else
        {
            return QVariant::fromValue(v).toJsonValue();
        }
    }, val);
}

template<>
Utils::Result<SemanticTokensRegistrationOptionsFull> fromJson<SemanticTokensRegistrationOptionsFull>(const QJsonValue &val)
{
    if (val.isBool())
        return SemanticTokensRegistrationOptionsFull(val.toBool());
    {
        auto result = fromJson<SemanticTokensFullDelta>(val);
        if (result) return SemanticTokensRegistrationOptionsFull(*result);
    }
    return Utils::ResultError("Invalid SemanticTokensRegistrationOptionsFull");
}

QJsonValue toJsonValue(const SemanticTokensRegistrationOptionsFull &val)
{
    return std::visit([](const auto &v) -> QJsonValue {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, SemanticTokensFullDelta>) {
            return toJson(v);
        } else
        {
            return QVariant::fromValue(v).toJsonValue();
        }
    }, val);
}

template<>
Utils::Result<SemanticTokensRegistrationOptions> fromJson<SemanticTokensRegistrationOptions>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for SemanticTokensRegistrationOptions");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("documentSelector"))
        return Utils::ResultError("Missing required field: documentSelector");
    if (!obj.contains("legend"))
        return Utils::ResultError("Missing required field: legend");
    SemanticTokensRegistrationOptions result;
    if (obj.contains("documentSelector") && !obj["documentSelector"].isNull()) {
        const auto res0 = fromJson<DocumentSelector>("documentSelector", obj["documentSelector"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._documentSelector = *res0;
    }
    if (obj.contains("workDoneProgress"))
        result._workDoneProgress = obj.value("workDoneProgress").toBool();
    if (obj.contains("legend") && obj["legend"].isObject()) {
        const auto res1 = fromJson<SemanticTokensLegend>("legend", obj["legend"]);
        if (!res1)
            return Utils::ResultError(res1.error());
        result._legend = *res1;
    }
    if (obj.contains("range")) {
        const auto res2 = fromJson<SemanticTokensRegistrationOptionsRange>("range", obj["range"]);
        if (!res2)
            return Utils::ResultError(res2.error());
        result._range = *res2;
    }
    if (obj.contains("full")) {
        const auto res3 = fromJson<SemanticTokensRegistrationOptionsFull>("full", obj["full"]);
        if (!res3)
            return Utils::ResultError(res3.error());
        result._full = *res3;
    }
    if (obj.contains("id"))
        result._id = obj.value("id").toString();
    return result;
}

QJsonObject toJson(const SemanticTokensRegistrationOptions &data)
{
    QJsonObject obj{{"legend", toJson(data._legend)}};
    if (data._documentSelector.has_value())
        obj.insert("documentSelector", toJson(*data._documentSelector));
    else
        obj.insert("documentSelector", QJsonValue::Null);
    if (data._workDoneProgress.has_value())
        obj.insert("workDoneProgress", *data._workDoneProgress);
    if (data._range.has_value())
        obj.insert("range", toJsonValue(*data._range));
    if (data._full.has_value())
        obj.insert("full", toJsonValue(*data._full));
    if (data._id.has_value())
        obj.insert("id", *data._id);
    return obj;
}

template<>
Utils::Result<SemanticTokensDeltaParams> fromJson<SemanticTokensDeltaParams>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for SemanticTokensDeltaParams");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("textDocument"))
        return Utils::ResultError("Missing required field: textDocument");
    if (!obj.contains("previousResultId"))
        return Utils::ResultError("Missing required field: previousResultId");
    SemanticTokensDeltaParams result;
    if (obj.contains("workDoneToken")) {
        const auto res0 = fromJson<ProgressToken>("workDoneToken", obj["workDoneToken"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._workDoneToken = *res0;
    }
    if (obj.contains("partialResultToken")) {
        const auto res1 = fromJson<ProgressToken>("partialResultToken", obj["partialResultToken"]);
        if (!res1)
            return Utils::ResultError(res1.error());
        result._partialResultToken = *res1;
    }
    if (obj.contains("textDocument") && obj["textDocument"].isObject()) {
        const auto res2 = fromJson<TextDocumentIdentifier>("textDocument", obj["textDocument"]);
        if (!res2)
            return Utils::ResultError(res2.error());
        result._textDocument = *res2;
    }
    result._previousResultId = obj.value("previousResultId").toString();
    return result;
}

QJsonObject toJson(const SemanticTokensDeltaParams &data)
{
    QJsonObject obj{
        {"textDocument", toJson(data._textDocument)},
        {"previousResultId", data._previousResultId}
    };
    if (data._workDoneToken.has_value())
        obj.insert("workDoneToken", toJsonValue(*data._workDoneToken));
    if (data._partialResultToken.has_value())
        obj.insert("partialResultToken", toJsonValue(*data._partialResultToken));
    return obj;
}

template<>
Utils::Result<SemanticTokensEdit> fromJson<SemanticTokensEdit>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for SemanticTokensEdit");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("start"))
        return Utils::ResultError("Missing required field: start");
    if (!obj.contains("deleteCount"))
        return Utils::ResultError("Missing required field: deleteCount");
    SemanticTokensEdit result;
    result._start = obj.value("start").toInt();
    result._deleteCount = obj.value("deleteCount").toInt();
    if (obj.contains("data") && obj["data"].isArray()) {
        const QJsonArray arr = obj["data"].toArray();
        QList<int> list_data;
        for (const QJsonValue &v : arr) {
            list_data.append(v.toInt());
        }
        result._data = list_data;
    }
    return result;
}

QJsonObject toJson(const SemanticTokensEdit &data)
{
    QJsonObject obj{
        {"start", data._start},
        {"deleteCount", data._deleteCount}
    };
    if (data._data.has_value()) {
        QJsonArray arr_data;
        for (const auto &v : *data._data) arr_data.append(v);
        obj.insert("data", arr_data);
    }
    return obj;
}

template<>
Utils::Result<SemanticTokensDelta> fromJson<SemanticTokensDelta>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for SemanticTokensDelta");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("edits"))
        return Utils::ResultError("Missing required field: edits");
    SemanticTokensDelta result;
    if (obj.contains("resultId"))
        result._resultId = obj.value("resultId").toString();
    if (obj.contains("edits") && obj["edits"].isArray()) {
        const QJsonArray arr = obj["edits"].toArray();
        for (const QJsonValue &v : arr) {
            const auto res0 = fromJson<SemanticTokensEdit>("edits", v);
            if (!res0)
                return Utils::ResultError(res0.error());
            result._edits.append(*res0);
        }
    }
    return result;
}

QJsonObject toJson(const SemanticTokensDelta &data)
{
    QJsonObject obj;
    if (data._resultId.has_value())
        obj.insert("resultId", *data._resultId);
    QJsonArray arr_edits;
    for (const auto &v : data._edits) arr_edits.append(toJson(v));
    obj.insert("edits", arr_edits);
    return obj;
}

template<>
Utils::Result<SemanticTokensDeltaPartialResult> fromJson<SemanticTokensDeltaPartialResult>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for SemanticTokensDeltaPartialResult");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("edits"))
        return Utils::ResultError("Missing required field: edits");
    SemanticTokensDeltaPartialResult result;
    if (obj.contains("edits") && obj["edits"].isArray()) {
        const QJsonArray arr = obj["edits"].toArray();
        for (const QJsonValue &v : arr) {
            const auto res0 = fromJson<SemanticTokensEdit>("edits", v);
            if (!res0)
                return Utils::ResultError(res0.error());
            result._edits.append(*res0);
        }
    }
    return result;
}

QJsonObject toJson(const SemanticTokensDeltaPartialResult &data)
{
    QJsonObject obj;
    QJsonArray arr_edits;
    for (const auto &v : data._edits) arr_edits.append(toJson(v));
    obj.insert("edits", arr_edits);
    return obj;
}

template<>
Utils::Result<SemanticTokensRangeParams> fromJson<SemanticTokensRangeParams>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for SemanticTokensRangeParams");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("textDocument"))
        return Utils::ResultError("Missing required field: textDocument");
    if (!obj.contains("range"))
        return Utils::ResultError("Missing required field: range");
    SemanticTokensRangeParams result;
    if (obj.contains("workDoneToken")) {
        const auto res0 = fromJson<ProgressToken>("workDoneToken", obj["workDoneToken"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._workDoneToken = *res0;
    }
    if (obj.contains("partialResultToken")) {
        const auto res1 = fromJson<ProgressToken>("partialResultToken", obj["partialResultToken"]);
        if (!res1)
            return Utils::ResultError(res1.error());
        result._partialResultToken = *res1;
    }
    if (obj.contains("textDocument") && obj["textDocument"].isObject()) {
        const auto res2 = fromJson<TextDocumentIdentifier>("textDocument", obj["textDocument"]);
        if (!res2)
            return Utils::ResultError(res2.error());
        result._textDocument = *res2;
    }
    if (obj.contains("range") && obj["range"].isObject()) {
        const auto res3 = fromJson<Range>("range", obj["range"]);
        if (!res3)
            return Utils::ResultError(res3.error());
        result._range = *res3;
    }
    return result;
}

QJsonObject toJson(const SemanticTokensRangeParams &data)
{
    QJsonObject obj{
        {"textDocument", toJson(data._textDocument)},
        {"range", toJson(data._range)}
    };
    if (data._workDoneToken.has_value())
        obj.insert("workDoneToken", toJsonValue(*data._workDoneToken));
    if (data._partialResultToken.has_value())
        obj.insert("partialResultToken", toJsonValue(*data._partialResultToken));
    return obj;
}

template<>
Utils::Result<ShowDocumentParams> fromJson<ShowDocumentParams>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for ShowDocumentParams");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("uri"))
        return Utils::ResultError("Missing required field: uri");
    ShowDocumentParams result;
    result._uri = obj.value("uri").toString();
    if (obj.contains("external"))
        result._external = obj.value("external").toBool();
    if (obj.contains("takeFocus"))
        result._takeFocus = obj.value("takeFocus").toBool();
    if (obj.contains("selection") && obj["selection"].isObject()) {
        const auto res0 = fromJson<Range>("selection", obj["selection"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._selection = *res0;
    }
    return result;
}

QJsonObject toJson(const ShowDocumentParams &data)
{
    QJsonObject obj{{"uri", data._uri}};
    if (data._external.has_value())
        obj.insert("external", *data._external);
    if (data._takeFocus.has_value())
        obj.insert("takeFocus", *data._takeFocus);
    if (data._selection.has_value())
        obj.insert("selection", toJson(*data._selection));
    return obj;
}

template<>
Utils::Result<ShowDocumentResult> fromJson<ShowDocumentResult>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for ShowDocumentResult");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("success"))
        return Utils::ResultError("Missing required field: success");
    ShowDocumentResult result;
    result._success = obj.value("success").toBool();
    return result;
}

QJsonObject toJson(const ShowDocumentResult &data)
{
    QJsonObject obj{{"success", data._success}};
    return obj;
}

template<>
Utils::Result<LinkedEditingRangeParams> fromJson<LinkedEditingRangeParams>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for LinkedEditingRangeParams");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("textDocument"))
        return Utils::ResultError("Missing required field: textDocument");
    if (!obj.contains("position"))
        return Utils::ResultError("Missing required field: position");
    LinkedEditingRangeParams result;
    if (obj.contains("textDocument") && obj["textDocument"].isObject()) {
        const auto res0 = fromJson<TextDocumentIdentifier>("textDocument", obj["textDocument"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._textDocument = *res0;
    }
    if (obj.contains("position") && obj["position"].isObject()) {
        const auto res1 = fromJson<Position>("position", obj["position"]);
        if (!res1)
            return Utils::ResultError(res1.error());
        result._position = *res1;
    }
    if (obj.contains("workDoneToken")) {
        const auto res2 = fromJson<ProgressToken>("workDoneToken", obj["workDoneToken"]);
        if (!res2)
            return Utils::ResultError(res2.error());
        result._workDoneToken = *res2;
    }
    return result;
}

QJsonObject toJson(const LinkedEditingRangeParams &data)
{
    QJsonObject obj{
        {"textDocument", toJson(data._textDocument)},
        {"position", toJson(data._position)}
    };
    if (data._workDoneToken.has_value())
        obj.insert("workDoneToken", toJsonValue(*data._workDoneToken));
    return obj;
}

template<>
Utils::Result<LinkedEditingRanges> fromJson<LinkedEditingRanges>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for LinkedEditingRanges");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("ranges"))
        return Utils::ResultError("Missing required field: ranges");
    LinkedEditingRanges result;
    if (obj.contains("ranges") && obj["ranges"].isArray()) {
        const QJsonArray arr = obj["ranges"].toArray();
        for (const QJsonValue &v : arr) {
            const auto res0 = fromJson<Range>("ranges", v);
            if (!res0)
                return Utils::ResultError(res0.error());
            result._ranges.append(*res0);
        }
    }
    if (obj.contains("wordPattern"))
        result._wordPattern = obj.value("wordPattern").toString();
    return result;
}

QJsonObject toJson(const LinkedEditingRanges &data)
{
    QJsonObject obj;
    QJsonArray arr_ranges;
    for (const auto &v : data._ranges) arr_ranges.append(toJson(v));
    obj.insert("ranges", arr_ranges);
    if (data._wordPattern.has_value())
        obj.insert("wordPattern", *data._wordPattern);
    return obj;
}

template<>
Utils::Result<LinkedEditingRangeRegistrationOptions> fromJson<LinkedEditingRangeRegistrationOptions>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for LinkedEditingRangeRegistrationOptions");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("documentSelector"))
        return Utils::ResultError("Missing required field: documentSelector");
    LinkedEditingRangeRegistrationOptions result;
    if (obj.contains("documentSelector") && !obj["documentSelector"].isNull()) {
        const auto res0 = fromJson<DocumentSelector>("documentSelector", obj["documentSelector"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._documentSelector = *res0;
    }
    if (obj.contains("workDoneProgress"))
        result._workDoneProgress = obj.value("workDoneProgress").toBool();
    if (obj.contains("id"))
        result._id = obj.value("id").toString();
    return result;
}

QJsonObject toJson(const LinkedEditingRangeRegistrationOptions &data)
{
    QJsonObject obj;
    if (data._documentSelector.has_value())
        obj.insert("documentSelector", toJson(*data._documentSelector));
    else
        obj.insert("documentSelector", QJsonValue::Null);
    if (data._workDoneProgress.has_value())
        obj.insert("workDoneProgress", *data._workDoneProgress);
    if (data._id.has_value())
        obj.insert("id", *data._id);
    return obj;
}

template<>
Utils::Result<FileCreate> fromJson<FileCreate>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for FileCreate");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("uri"))
        return Utils::ResultError("Missing required field: uri");
    FileCreate result;
    result._uri = obj.value("uri").toString();
    return result;
}

QJsonObject toJson(const FileCreate &data)
{
    QJsonObject obj{{"uri", data._uri}};
    return obj;
}

template<>
Utils::Result<CreateFilesParams> fromJson<CreateFilesParams>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for CreateFilesParams");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("files"))
        return Utils::ResultError("Missing required field: files");
    CreateFilesParams result;
    if (obj.contains("files") && obj["files"].isArray()) {
        const QJsonArray arr = obj["files"].toArray();
        for (const QJsonValue &v : arr) {
            const auto res0 = fromJson<FileCreate>("files", v);
            if (!res0)
                return Utils::ResultError(res0.error());
            result._files.append(*res0);
        }
    }
    return result;
}

QJsonObject toJson(const CreateFilesParams &data)
{
    QJsonObject obj;
    QJsonArray arr_files;
    for (const auto &v : data._files) arr_files.append(toJson(v));
    obj.insert("files", arr_files);
    return obj;
}

template<>
Utils::Result<ChangeAnnotation> fromJson<ChangeAnnotation>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for ChangeAnnotation");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("label"))
        return Utils::ResultError("Missing required field: label");
    ChangeAnnotation result;
    result._label = obj.value("label").toString();
    if (obj.contains("needsConfirmation"))
        result._needsConfirmation = obj.value("needsConfirmation").toBool();
    if (obj.contains("description"))
        result._description = obj.value("description").toString();
    return result;
}

QJsonObject toJson(const ChangeAnnotation &data)
{
    QJsonObject obj{{"label", data._label}};
    if (data._needsConfirmation.has_value())
        obj.insert("needsConfirmation", *data._needsConfirmation);
    if (data._description.has_value())
        obj.insert("description", *data._description);
    return obj;
}

template<>
Utils::Result<CreateFileOptions> fromJson<CreateFileOptions>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for CreateFileOptions");
    const QJsonObject obj = val.toObject();
    CreateFileOptions result;
    if (obj.contains("overwrite"))
        result._overwrite = obj.value("overwrite").toBool();
    if (obj.contains("ignoreIfExists"))
        result._ignoreIfExists = obj.value("ignoreIfExists").toBool();
    return result;
}

QJsonObject toJson(const CreateFileOptions &data)
{
    QJsonObject obj;
    if (data._overwrite.has_value())
        obj.insert("overwrite", *data._overwrite);
    if (data._ignoreIfExists.has_value())
        obj.insert("ignoreIfExists", *data._ignoreIfExists);
    return obj;
}

template<>
Utils::Result<CreateFile> fromJson<CreateFile>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for CreateFile");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("kind"))
        return Utils::ResultError("Missing required field: kind");
    if (!obj.contains("uri"))
        return Utils::ResultError("Missing required field: uri");
    CreateFile result;
    if (obj.value("kind").toString() != "create")
        return Utils::ResultError("Field 'kind' must be 'create', got: " + obj.value("kind").toString());
    if (obj.contains("annotationId") && obj["annotationId"].isString()) {
        const auto res0 = fromJson<ChangeAnnotationIdentifier>("annotationId", obj["annotationId"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._annotationId = *res0;
    }
    result._uri = obj.value("uri").toString();
    if (obj.contains("options") && obj["options"].isObject()) {
        const auto res1 = fromJson<CreateFileOptions>("options", obj["options"]);
        if (!res1)
            return Utils::ResultError(res1.error());
        result._options = *res1;
    }
    return result;
}

QJsonObject toJson(const CreateFile &data)
{
    QJsonObject obj{
        {"kind", QString("create")},
        {"uri", data._uri}
    };
    if (data._annotationId.has_value())
        obj.insert("annotationId", *data._annotationId);
    if (data._options.has_value())
        obj.insert("options", toJson(*data._options));
    return obj;
}

template<>
Utils::Result<DeleteFileOptions> fromJson<DeleteFileOptions>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for DeleteFileOptions");
    const QJsonObject obj = val.toObject();
    DeleteFileOptions result;
    if (obj.contains("recursive"))
        result._recursive = obj.value("recursive").toBool();
    if (obj.contains("ignoreIfNotExists"))
        result._ignoreIfNotExists = obj.value("ignoreIfNotExists").toBool();
    return result;
}

QJsonObject toJson(const DeleteFileOptions &data)
{
    QJsonObject obj;
    if (data._recursive.has_value())
        obj.insert("recursive", *data._recursive);
    if (data._ignoreIfNotExists.has_value())
        obj.insert("ignoreIfNotExists", *data._ignoreIfNotExists);
    return obj;
}

template<>
Utils::Result<DeleteFile> fromJson<DeleteFile>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for DeleteFile");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("kind"))
        return Utils::ResultError("Missing required field: kind");
    if (!obj.contains("uri"))
        return Utils::ResultError("Missing required field: uri");
    DeleteFile result;
    if (obj.value("kind").toString() != "delete")
        return Utils::ResultError("Field 'kind' must be 'delete', got: " + obj.value("kind").toString());
    if (obj.contains("annotationId") && obj["annotationId"].isString()) {
        const auto res0 = fromJson<ChangeAnnotationIdentifier>("annotationId", obj["annotationId"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._annotationId = *res0;
    }
    result._uri = obj.value("uri").toString();
    if (obj.contains("options") && obj["options"].isObject()) {
        const auto res1 = fromJson<DeleteFileOptions>("options", obj["options"]);
        if (!res1)
            return Utils::ResultError(res1.error());
        result._options = *res1;
    }
    return result;
}

QJsonObject toJson(const DeleteFile &data)
{
    QJsonObject obj{
        {"kind", QString("delete")},
        {"uri", data._uri}
    };
    if (data._annotationId.has_value())
        obj.insert("annotationId", *data._annotationId);
    if (data._options.has_value())
        obj.insert("options", toJson(*data._options));
    return obj;
}

template<>
Utils::Result<RenameFileOptions> fromJson<RenameFileOptions>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for RenameFileOptions");
    const QJsonObject obj = val.toObject();
    RenameFileOptions result;
    if (obj.contains("overwrite"))
        result._overwrite = obj.value("overwrite").toBool();
    if (obj.contains("ignoreIfExists"))
        result._ignoreIfExists = obj.value("ignoreIfExists").toBool();
    return result;
}

QJsonObject toJson(const RenameFileOptions &data)
{
    QJsonObject obj;
    if (data._overwrite.has_value())
        obj.insert("overwrite", *data._overwrite);
    if (data._ignoreIfExists.has_value())
        obj.insert("ignoreIfExists", *data._ignoreIfExists);
    return obj;
}

template<>
Utils::Result<RenameFile> fromJson<RenameFile>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for RenameFile");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("kind"))
        return Utils::ResultError("Missing required field: kind");
    if (!obj.contains("oldUri"))
        return Utils::ResultError("Missing required field: oldUri");
    if (!obj.contains("newUri"))
        return Utils::ResultError("Missing required field: newUri");
    RenameFile result;
    if (obj.value("kind").toString() != "rename")
        return Utils::ResultError("Field 'kind' must be 'rename', got: " + obj.value("kind").toString());
    if (obj.contains("annotationId") && obj["annotationId"].isString()) {
        const auto res0 = fromJson<ChangeAnnotationIdentifier>("annotationId", obj["annotationId"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._annotationId = *res0;
    }
    result._oldUri = obj.value("oldUri").toString();
    result._newUri = obj.value("newUri").toString();
    if (obj.contains("options") && obj["options"].isObject()) {
        const auto res1 = fromJson<RenameFileOptions>("options", obj["options"]);
        if (!res1)
            return Utils::ResultError(res1.error());
        result._options = *res1;
    }
    return result;
}

QJsonObject toJson(const RenameFile &data)
{
    QJsonObject obj{
        {"kind", QString("rename")},
        {"oldUri", data._oldUri},
        {"newUri", data._newUri}
    };
    if (data._annotationId.has_value())
        obj.insert("annotationId", *data._annotationId);
    if (data._options.has_value())
        obj.insert("options", toJson(*data._options));
    return obj;
}

template<>
Utils::Result<AnnotatedTextEdit> fromJson<AnnotatedTextEdit>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for AnnotatedTextEdit");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("range"))
        return Utils::ResultError("Missing required field: range");
    if (!obj.contains("newText"))
        return Utils::ResultError("Missing required field: newText");
    if (!obj.contains("annotationId"))
        return Utils::ResultError("Missing required field: annotationId");
    AnnotatedTextEdit result;
    if (obj.contains("range") && obj["range"].isObject()) {
        const auto res0 = fromJson<Range>("range", obj["range"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._range = *res0;
    }
    result._newText = obj.value("newText").toString();
    if (obj.contains("annotationId") && obj["annotationId"].isString()) {
        const auto res1 = fromJson<ChangeAnnotationIdentifier>("annotationId", obj["annotationId"]);
        if (!res1)
            return Utils::ResultError(res1.error());
        result._annotationId = *res1;
    }
    return result;
}

QJsonObject toJson(const AnnotatedTextEdit &data)
{
    QJsonObject obj{
        {"range", toJson(data._range)},
        {"newText", data._newText},
        {"annotationId", data._annotationId}
    };
    return obj;
}

template<>
Utils::Result<OptionalVersionedTextDocumentIdentifier> fromJson<OptionalVersionedTextDocumentIdentifier>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for OptionalVersionedTextDocumentIdentifier");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("uri"))
        return Utils::ResultError("Missing required field: uri");
    if (!obj.contains("version"))
        return Utils::ResultError("Missing required field: version");
    OptionalVersionedTextDocumentIdentifier result;
    result._uri = obj.value("uri").toString();
    if (!obj["version"].isNull()) {
        result._version = obj.value("version").toInt();
    }
    return result;
}

QJsonObject toJson(const OptionalVersionedTextDocumentIdentifier &data)
{
    QJsonObject obj{{"uri", data._uri}};
    if (data._version.has_value())
        obj.insert("version", *data._version);
    else
        obj.insert("version", QJsonValue::Null);
    return obj;
}

template<>
Utils::Result<StringValue> fromJson<StringValue>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for StringValue");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("kind"))
        return Utils::ResultError("Missing required field: kind");
    if (!obj.contains("value"))
        return Utils::ResultError("Missing required field: value");
    StringValue result;
    if (obj.value("kind").toString() != "snippet")
        return Utils::ResultError("Field 'kind' must be 'snippet', got: " + obj.value("kind").toString());
    result._value = obj.value("value").toString();
    return result;
}

QJsonObject toJson(const StringValue &data)
{
    QJsonObject obj{
        {"kind", QString("snippet")},
        {"value", data._value}
    };
    return obj;
}

template<>
Utils::Result<SnippetTextEdit> fromJson<SnippetTextEdit>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for SnippetTextEdit");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("range"))
        return Utils::ResultError("Missing required field: range");
    if (!obj.contains("snippet"))
        return Utils::ResultError("Missing required field: snippet");
    SnippetTextEdit result;
    if (obj.contains("range") && obj["range"].isObject()) {
        const auto res0 = fromJson<Range>("range", obj["range"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._range = *res0;
    }
    if (obj.contains("snippet") && obj["snippet"].isObject()) {
        const auto res1 = fromJson<StringValue>("snippet", obj["snippet"]);
        if (!res1)
            return Utils::ResultError(res1.error());
        result._snippet = *res1;
    }
    if (obj.contains("annotationId") && obj["annotationId"].isString()) {
        const auto res2 = fromJson<ChangeAnnotationIdentifier>("annotationId", obj["annotationId"]);
        if (!res2)
            return Utils::ResultError(res2.error());
        result._annotationId = *res2;
    }
    return result;
}

QJsonObject toJson(const SnippetTextEdit &data)
{
    QJsonObject obj{
        {"range", toJson(data._range)},
        {"snippet", toJson(data._snippet)}
    };
    if (data._annotationId.has_value())
        obj.insert("annotationId", *data._annotationId);
    return obj;
}

template<>
Utils::Result<TextDocumentEditEditsItem> fromJson<TextDocumentEditEditsItem>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Invalid TextDocumentEditEditsItem: expected object or array");
    const QJsonObject obj = val.toObject();
    if (obj.contains("snippet")) {
        const auto res0 = fromJson<SnippetTextEdit>(val);
        if (!res0)
            return Utils::ResultError(res0.error());
        return TextDocumentEditEditsItem(*res0);
    }
    {
        auto result = fromJson<TextEdit>(val);
        if (result) return TextDocumentEditEditsItem(*result);
    }
    {
        auto result = fromJson<AnnotatedTextEdit>(val);
        if (result) return TextDocumentEditEditsItem(*result);
    }
    return Utils::ResultError("Invalid TextDocumentEditEditsItem");
}

QJsonValue toJsonValue(const TextDocumentEditEditsItem &val)
{
    return std::visit([](const auto &v) -> QJsonValue {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, QJsonObject>) {
            return v;
        } else {
            return toJson(v);
        }
    }, val);
}

template<>
Utils::Result<TextDocumentEdit> fromJson<TextDocumentEdit>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for TextDocumentEdit");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("textDocument"))
        return Utils::ResultError("Missing required field: textDocument");
    if (!obj.contains("edits"))
        return Utils::ResultError("Missing required field: edits");
    TextDocumentEdit result;
    if (obj.contains("textDocument") && obj["textDocument"].isObject()) {
        const auto res0 = fromJson<OptionalVersionedTextDocumentIdentifier>("textDocument", obj["textDocument"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._textDocument = *res0;
    }
    if (obj.contains("edits") && obj["edits"].isArray()) {
        const QJsonArray arr = obj["edits"].toArray();
        for (const QJsonValue &v : arr) {
            const auto res1 = fromJson<TextDocumentEditEditsItem>("edits", v);
            if (!res1)
                return Utils::ResultError(res1.error());
            result._edits.append(*res1);
        }
    }
    return result;
}

QJsonObject toJson(const TextDocumentEdit &data)
{
    QJsonObject obj{{"textDocument", toJson(data._textDocument)}};
    QJsonArray arr_edits;
    for (const auto &v : data._edits) arr_edits.append(toJsonValue(v));
    obj.insert("edits", arr_edits);
    return obj;
}

template<>
Utils::Result<WorkspaceEditDocumentChangesItem> fromJson<WorkspaceEditDocumentChangesItem>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Invalid WorkspaceEditDocumentChangesItem: expected object or array");
    const QJsonObject obj = val.toObject();
    if (obj.contains("edits")) {
        const auto res0 = fromJson<TextDocumentEdit>(val);
        if (!res0)
            return Utils::ResultError(res0.error());
        return WorkspaceEditDocumentChangesItem(*res0);
    }
    if (obj.contains("newUri")) {
        const auto res1 = fromJson<RenameFile>(val);
        if (!res1)
            return Utils::ResultError(res1.error());
        return WorkspaceEditDocumentChangesItem(*res1);
    }
    {
        auto result = fromJson<CreateFile>(val);
        if (result) return WorkspaceEditDocumentChangesItem(*result);
    }
    {
        auto result = fromJson<DeleteFile>(val);
        if (result) return WorkspaceEditDocumentChangesItem(*result);
    }
    return Utils::ResultError("Invalid WorkspaceEditDocumentChangesItem");
}

QJsonValue toJsonValue(const WorkspaceEditDocumentChangesItem &val)
{
    return std::visit([](const auto &v) -> QJsonValue {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, QJsonObject>) {
            return v;
        } else {
            return toJson(v);
        }
    }, val);
}

template<>
Utils::Result<WorkspaceEdit> fromJson<WorkspaceEdit>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for WorkspaceEdit");
    const QJsonObject obj = val.toObject();
    WorkspaceEdit result;
    if (obj.contains("changes") && obj["changes"].isObject()) {
        const QJsonObject mapObj_changes = obj["changes"].toObject();
        QMap<QString, QList<TextEdit>> map_changes;
        for (auto it = mapObj_changes.constBegin(); it != mapObj_changes.constEnd(); ++it) {
            QList<TextEdit> list_changes;
            for (const QJsonValue &v : it.value().toArray()) {
                const auto res0 = fromJson<TextEdit>(v);
                if (!res0)
                    return Utils::ResultError(res0.error());
                list_changes.append(*res0);
            }
            map_changes.insert(it.key(), list_changes);
        }
        result._changes = map_changes;
    }
    if (obj.contains("documentChanges") && obj["documentChanges"].isArray()) {
        const QJsonArray arr = obj["documentChanges"].toArray();
        QList<WorkspaceEditDocumentChangesItem> list_documentChanges;
        for (const QJsonValue &v : arr) {
            const auto res1 = fromJson<WorkspaceEditDocumentChangesItem>("documentChanges", v);
            if (!res1)
                return Utils::ResultError(res1.error());
            list_documentChanges.append(*res1);
        }
        result._documentChanges = list_documentChanges;
    }
    if (obj.contains("changeAnnotations") && obj["changeAnnotations"].isObject()) {
        const QJsonObject mapObj_changeAnnotations = obj["changeAnnotations"].toObject();
        QMap<QString, ChangeAnnotation> map_changeAnnotations;
        for (auto it = mapObj_changeAnnotations.constBegin(); it != mapObj_changeAnnotations.constEnd(); ++it) {
            const auto res2 = fromJson<ChangeAnnotation>("changeAnnotations", it.value());
            if (!res2)
                return Utils::ResultError(res2.error());
            map_changeAnnotations.insert(it.key(), *res2);
        }
        result._changeAnnotations = map_changeAnnotations;
    }
    return result;
}

QJsonObject toJson(const WorkspaceEdit &data)
{
    QJsonObject obj;
    if (data._changes.has_value()) {
        QJsonObject map_changes;
        for (auto it = data._changes->constBegin(); it != data._changes->constEnd(); ++it) {
            QJsonArray arr_changes;
            for (const auto &v : it.value()) arr_changes.append(toJson(v));
            map_changes.insert(it.key(), arr_changes);
        }
        obj.insert("changes", map_changes);
    }
    if (data._documentChanges.has_value()) {
        QJsonArray arr_documentChanges;
        for (const auto &v : *data._documentChanges) arr_documentChanges.append(toJsonValue(v));
        obj.insert("documentChanges", arr_documentChanges);
    }
    if (data._changeAnnotations.has_value()) {
        QJsonObject map_changeAnnotations;
        for (auto it = data._changeAnnotations->constBegin(); it != data._changeAnnotations->constEnd(); ++it)
            map_changeAnnotations.insert(it.key(), toJson(it.value()));
        obj.insert("changeAnnotations", map_changeAnnotations);
    }
    return obj;
}

QString toString(FileOperationPatternKind v)
{
    switch(v) {
        case FileOperationPatternKind::file: return "file";
        case FileOperationPatternKind::folder: return "folder";
    }
    return {};
}

template<>
Utils::Result<FileOperationPatternKind> fromJson<FileOperationPatternKind>(const QJsonValue &val)
{
    if (!val.isString())
        return Utils::ResultError("Expected JSON string for FileOperationPatternKind");
    const QString str = val.toString();
    if (str == "file") return FileOperationPatternKind::file;
    if (str == "folder") return FileOperationPatternKind::folder;
    return Utils::ResultError("Invalid FileOperationPatternKind value: " + str);
}

QJsonValue toJsonValue(const FileOperationPatternKind &v)
{
    return toString(v);
}

template<>
Utils::Result<FileOperationPatternOptions> fromJson<FileOperationPatternOptions>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for FileOperationPatternOptions");
    const QJsonObject obj = val.toObject();
    FileOperationPatternOptions result;
    if (obj.contains("ignoreCase"))
        result._ignoreCase = obj.value("ignoreCase").toBool();
    return result;
}

QJsonObject toJson(const FileOperationPatternOptions &data)
{
    QJsonObject obj;
    if (data._ignoreCase.has_value())
        obj.insert("ignoreCase", *data._ignoreCase);
    return obj;
}

template<>
Utils::Result<FileOperationPattern> fromJson<FileOperationPattern>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for FileOperationPattern");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("glob"))
        return Utils::ResultError("Missing required field: glob");
    FileOperationPattern result;
    result._glob = obj.value("glob").toString();
    if (obj.contains("matches")) {
        const auto res0 = fromJson<FileOperationPatternKind>("matches", obj["matches"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._matches = *res0;
    }
    if (obj.contains("options") && obj["options"].isObject()) {
        const auto res1 = fromJson<FileOperationPatternOptions>("options", obj["options"]);
        if (!res1)
            return Utils::ResultError(res1.error());
        result._options = *res1;
    }
    return result;
}

QJsonObject toJson(const FileOperationPattern &data)
{
    QJsonObject obj{{"glob", data._glob}};
    if (data._matches.has_value())
        obj.insert("matches", toJsonValue(*data._matches));
    if (data._options.has_value())
        obj.insert("options", toJson(*data._options));
    return obj;
}

template<>
Utils::Result<FileOperationFilter> fromJson<FileOperationFilter>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for FileOperationFilter");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("pattern"))
        return Utils::ResultError("Missing required field: pattern");
    FileOperationFilter result;
    if (obj.contains("scheme"))
        result._scheme = obj.value("scheme").toString();
    if (obj.contains("pattern") && obj["pattern"].isObject()) {
        const auto res0 = fromJson<FileOperationPattern>("pattern", obj["pattern"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._pattern = *res0;
    }
    return result;
}

QJsonObject toJson(const FileOperationFilter &data)
{
    QJsonObject obj{{"pattern", toJson(data._pattern)}};
    if (data._scheme.has_value())
        obj.insert("scheme", *data._scheme);
    return obj;
}

template<>
Utils::Result<FileOperationRegistrationOptions> fromJson<FileOperationRegistrationOptions>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for FileOperationRegistrationOptions");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("filters"))
        return Utils::ResultError("Missing required field: filters");
    FileOperationRegistrationOptions result;
    if (obj.contains("filters") && obj["filters"].isArray()) {
        const QJsonArray arr = obj["filters"].toArray();
        for (const QJsonValue &v : arr) {
            const auto res0 = fromJson<FileOperationFilter>("filters", v);
            if (!res0)
                return Utils::ResultError(res0.error());
            result._filters.append(*res0);
        }
    }
    return result;
}

QJsonObject toJson(const FileOperationRegistrationOptions &data)
{
    QJsonObject obj;
    QJsonArray arr_filters;
    for (const auto &v : data._filters) arr_filters.append(toJson(v));
    obj.insert("filters", arr_filters);
    return obj;
}

template<>
Utils::Result<FileRename> fromJson<FileRename>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for FileRename");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("oldUri"))
        return Utils::ResultError("Missing required field: oldUri");
    if (!obj.contains("newUri"))
        return Utils::ResultError("Missing required field: newUri");
    FileRename result;
    result._oldUri = obj.value("oldUri").toString();
    result._newUri = obj.value("newUri").toString();
    return result;
}

QJsonObject toJson(const FileRename &data)
{
    QJsonObject obj{
        {"oldUri", data._oldUri},
        {"newUri", data._newUri}
    };
    return obj;
}

template<>
Utils::Result<RenameFilesParams> fromJson<RenameFilesParams>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for RenameFilesParams");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("files"))
        return Utils::ResultError("Missing required field: files");
    RenameFilesParams result;
    if (obj.contains("files") && obj["files"].isArray()) {
        const QJsonArray arr = obj["files"].toArray();
        for (const QJsonValue &v : arr) {
            const auto res0 = fromJson<FileRename>("files", v);
            if (!res0)
                return Utils::ResultError(res0.error());
            result._files.append(*res0);
        }
    }
    return result;
}

QJsonObject toJson(const RenameFilesParams &data)
{
    QJsonObject obj;
    QJsonArray arr_files;
    for (const auto &v : data._files) arr_files.append(toJson(v));
    obj.insert("files", arr_files);
    return obj;
}

template<>
Utils::Result<FileDelete> fromJson<FileDelete>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for FileDelete");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("uri"))
        return Utils::ResultError("Missing required field: uri");
    FileDelete result;
    result._uri = obj.value("uri").toString();
    return result;
}

QJsonObject toJson(const FileDelete &data)
{
    QJsonObject obj{{"uri", data._uri}};
    return obj;
}

template<>
Utils::Result<DeleteFilesParams> fromJson<DeleteFilesParams>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for DeleteFilesParams");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("files"))
        return Utils::ResultError("Missing required field: files");
    DeleteFilesParams result;
    if (obj.contains("files") && obj["files"].isArray()) {
        const QJsonArray arr = obj["files"].toArray();
        for (const QJsonValue &v : arr) {
            const auto res0 = fromJson<FileDelete>("files", v);
            if (!res0)
                return Utils::ResultError(res0.error());
            result._files.append(*res0);
        }
    }
    return result;
}

QJsonObject toJson(const DeleteFilesParams &data)
{
    QJsonObject obj;
    QJsonArray arr_files;
    for (const auto &v : data._files) arr_files.append(toJson(v));
    obj.insert("files", arr_files);
    return obj;
}

template<>
Utils::Result<MonikerParams> fromJson<MonikerParams>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for MonikerParams");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("textDocument"))
        return Utils::ResultError("Missing required field: textDocument");
    if (!obj.contains("position"))
        return Utils::ResultError("Missing required field: position");
    MonikerParams result;
    if (obj.contains("textDocument") && obj["textDocument"].isObject()) {
        const auto res0 = fromJson<TextDocumentIdentifier>("textDocument", obj["textDocument"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._textDocument = *res0;
    }
    if (obj.contains("position") && obj["position"].isObject()) {
        const auto res1 = fromJson<Position>("position", obj["position"]);
        if (!res1)
            return Utils::ResultError(res1.error());
        result._position = *res1;
    }
    if (obj.contains("workDoneToken")) {
        const auto res2 = fromJson<ProgressToken>("workDoneToken", obj["workDoneToken"]);
        if (!res2)
            return Utils::ResultError(res2.error());
        result._workDoneToken = *res2;
    }
    if (obj.contains("partialResultToken")) {
        const auto res3 = fromJson<ProgressToken>("partialResultToken", obj["partialResultToken"]);
        if (!res3)
            return Utils::ResultError(res3.error());
        result._partialResultToken = *res3;
    }
    return result;
}

QJsonObject toJson(const MonikerParams &data)
{
    QJsonObject obj{
        {"textDocument", toJson(data._textDocument)},
        {"position", toJson(data._position)}
    };
    if (data._workDoneToken.has_value())
        obj.insert("workDoneToken", toJsonValue(*data._workDoneToken));
    if (data._partialResultToken.has_value())
        obj.insert("partialResultToken", toJsonValue(*data._partialResultToken));
    return obj;
}

QString toString(MonikerKind v)
{
    switch(v) {
        case MonikerKind::import: return "import";
        case MonikerKind::export_: return "export";
        case MonikerKind::local: return "local";
    }
    return {};
}

template<>
Utils::Result<MonikerKind> fromJson<MonikerKind>(const QJsonValue &val)
{
    if (!val.isString())
        return Utils::ResultError("Expected JSON string for MonikerKind");
    const QString str = val.toString();
    if (str == "import") return MonikerKind::import;
    if (str == "export") return MonikerKind::export_;
    if (str == "local") return MonikerKind::local;
    return Utils::ResultError("Invalid MonikerKind value: " + str);
}

QJsonValue toJsonValue(const MonikerKind &v)
{
    return toString(v);
}

QString toString(UniquenessLevel v)
{
    switch(v) {
        case UniquenessLevel::document: return "document";
        case UniquenessLevel::project: return "project";
        case UniquenessLevel::group: return "group";
        case UniquenessLevel::scheme: return "scheme";
        case UniquenessLevel::global: return "global";
    }
    return {};
}

template<>
Utils::Result<UniquenessLevel> fromJson<UniquenessLevel>(const QJsonValue &val)
{
    if (!val.isString())
        return Utils::ResultError("Expected JSON string for UniquenessLevel");
    const QString str = val.toString();
    if (str == "document") return UniquenessLevel::document;
    if (str == "project") return UniquenessLevel::project;
    if (str == "group") return UniquenessLevel::group;
    if (str == "scheme") return UniquenessLevel::scheme;
    if (str == "global") return UniquenessLevel::global;
    return Utils::ResultError("Invalid UniquenessLevel value: " + str);
}

QJsonValue toJsonValue(const UniquenessLevel &v)
{
    return toString(v);
}

template<>
Utils::Result<Moniker> fromJson<Moniker>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for Moniker");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("scheme"))
        return Utils::ResultError("Missing required field: scheme");
    if (!obj.contains("identifier"))
        return Utils::ResultError("Missing required field: identifier");
    if (!obj.contains("unique"))
        return Utils::ResultError("Missing required field: unique");
    Moniker result;
    result._scheme = obj.value("scheme").toString();
    result._identifier = obj.value("identifier").toString();
    const auto res0 = fromJson<UniquenessLevel>("unique", obj["unique"]);
    if (!res0)
        return Utils::ResultError(res0.error());
    result._unique = *res0;
    if (obj.contains("kind")) {
        const auto res1 = fromJson<MonikerKind>("kind", obj["kind"]);
        if (!res1)
            return Utils::ResultError(res1.error());
        result._kind = *res1;
    }
    return result;
}

QJsonObject toJson(const Moniker &data)
{
    QJsonObject obj{
        {"scheme", data._scheme},
        {"identifier", data._identifier},
        {"unique", toJsonValue(data._unique)}
    };
    if (data._kind.has_value())
        obj.insert("kind", toJsonValue(*data._kind));
    return obj;
}

template<>
Utils::Result<MonikerRegistrationOptions> fromJson<MonikerRegistrationOptions>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for MonikerRegistrationOptions");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("documentSelector"))
        return Utils::ResultError("Missing required field: documentSelector");
    MonikerRegistrationOptions result;
    if (obj.contains("documentSelector") && !obj["documentSelector"].isNull()) {
        const auto res0 = fromJson<DocumentSelector>("documentSelector", obj["documentSelector"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._documentSelector = *res0;
    }
    if (obj.contains("workDoneProgress"))
        result._workDoneProgress = obj.value("workDoneProgress").toBool();
    return result;
}

QJsonObject toJson(const MonikerRegistrationOptions &data)
{
    QJsonObject obj;
    if (data._documentSelector.has_value())
        obj.insert("documentSelector", toJson(*data._documentSelector));
    else
        obj.insert("documentSelector", QJsonValue::Null);
    if (data._workDoneProgress.has_value())
        obj.insert("workDoneProgress", *data._workDoneProgress);
    return obj;
}

template<>
Utils::Result<TypeHierarchyPrepareParams> fromJson<TypeHierarchyPrepareParams>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for TypeHierarchyPrepareParams");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("textDocument"))
        return Utils::ResultError("Missing required field: textDocument");
    if (!obj.contains("position"))
        return Utils::ResultError("Missing required field: position");
    TypeHierarchyPrepareParams result;
    if (obj.contains("textDocument") && obj["textDocument"].isObject()) {
        const auto res0 = fromJson<TextDocumentIdentifier>("textDocument", obj["textDocument"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._textDocument = *res0;
    }
    if (obj.contains("position") && obj["position"].isObject()) {
        const auto res1 = fromJson<Position>("position", obj["position"]);
        if (!res1)
            return Utils::ResultError(res1.error());
        result._position = *res1;
    }
    if (obj.contains("workDoneToken")) {
        const auto res2 = fromJson<ProgressToken>("workDoneToken", obj["workDoneToken"]);
        if (!res2)
            return Utils::ResultError(res2.error());
        result._workDoneToken = *res2;
    }
    return result;
}

QJsonObject toJson(const TypeHierarchyPrepareParams &data)
{
    QJsonObject obj{
        {"textDocument", toJson(data._textDocument)},
        {"position", toJson(data._position)}
    };
    if (data._workDoneToken.has_value())
        obj.insert("workDoneToken", toJsonValue(*data._workDoneToken));
    return obj;
}

template<>
Utils::Result<TypeHierarchyItem> fromJson<TypeHierarchyItem>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for TypeHierarchyItem");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("name"))
        return Utils::ResultError("Missing required field: name");
    if (!obj.contains("kind"))
        return Utils::ResultError("Missing required field: kind");
    if (!obj.contains("uri"))
        return Utils::ResultError("Missing required field: uri");
    if (!obj.contains("range"))
        return Utils::ResultError("Missing required field: range");
    if (!obj.contains("selectionRange"))
        return Utils::ResultError("Missing required field: selectionRange");
    TypeHierarchyItem result;
    result._name = obj.value("name").toString();
    result._kind = obj.value("kind").toInt();
    if (obj.contains("tags") && obj["tags"].isArray()) {
        const QJsonArray arr = obj["tags"].toArray();
        QList<int> list_tags;
        for (const QJsonValue &v : arr) {
            list_tags.append(v.toInt());
        }
        result._tags = list_tags;
    }
    if (obj.contains("detail"))
        result._detail = obj.value("detail").toString();
    result._uri = obj.value("uri").toString();
    if (obj.contains("range") && obj["range"].isObject()) {
        const auto res0 = fromJson<Range>("range", obj["range"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._range = *res0;
    }
    if (obj.contains("selectionRange") && obj["selectionRange"].isObject()) {
        const auto res1 = fromJson<Range>("selectionRange", obj["selectionRange"]);
        if (!res1)
            return Utils::ResultError(res1.error());
        result._selectionRange = *res1;
    }
    if (obj.contains("data"))
        result._data = obj.value("data");
    return result;
}

QJsonObject toJson(const TypeHierarchyItem &data)
{
    QJsonObject obj{
        {"name", data._name},
        {"kind", data._kind},
        {"uri", data._uri},
        {"range", toJson(data._range)},
        {"selectionRange", toJson(data._selectionRange)}
    };
    if (data._tags.has_value()) {
        QJsonArray arr_tags;
        for (const auto &v : *data._tags) arr_tags.append(v);
        obj.insert("tags", arr_tags);
    }
    if (data._detail.has_value())
        obj.insert("detail", *data._detail);
    if (data._data.has_value())
        obj.insert("data", *data._data);
    return obj;
}

template<>
Utils::Result<TypeHierarchyRegistrationOptions> fromJson<TypeHierarchyRegistrationOptions>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for TypeHierarchyRegistrationOptions");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("documentSelector"))
        return Utils::ResultError("Missing required field: documentSelector");
    TypeHierarchyRegistrationOptions result;
    if (obj.contains("documentSelector") && !obj["documentSelector"].isNull()) {
        const auto res0 = fromJson<DocumentSelector>("documentSelector", obj["documentSelector"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._documentSelector = *res0;
    }
    if (obj.contains("workDoneProgress"))
        result._workDoneProgress = obj.value("workDoneProgress").toBool();
    if (obj.contains("id"))
        result._id = obj.value("id").toString();
    return result;
}

QJsonObject toJson(const TypeHierarchyRegistrationOptions &data)
{
    QJsonObject obj;
    if (data._documentSelector.has_value())
        obj.insert("documentSelector", toJson(*data._documentSelector));
    else
        obj.insert("documentSelector", QJsonValue::Null);
    if (data._workDoneProgress.has_value())
        obj.insert("workDoneProgress", *data._workDoneProgress);
    if (data._id.has_value())
        obj.insert("id", *data._id);
    return obj;
}

template<>
Utils::Result<TypeHierarchySupertypesParams> fromJson<TypeHierarchySupertypesParams>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for TypeHierarchySupertypesParams");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("item"))
        return Utils::ResultError("Missing required field: item");
    TypeHierarchySupertypesParams result;
    if (obj.contains("workDoneToken")) {
        const auto res0 = fromJson<ProgressToken>("workDoneToken", obj["workDoneToken"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._workDoneToken = *res0;
    }
    if (obj.contains("partialResultToken")) {
        const auto res1 = fromJson<ProgressToken>("partialResultToken", obj["partialResultToken"]);
        if (!res1)
            return Utils::ResultError(res1.error());
        result._partialResultToken = *res1;
    }
    if (obj.contains("item") && obj["item"].isObject()) {
        const auto res2 = fromJson<TypeHierarchyItem>("item", obj["item"]);
        if (!res2)
            return Utils::ResultError(res2.error());
        result._item = *res2;
    }
    return result;
}

QJsonObject toJson(const TypeHierarchySupertypesParams &data)
{
    QJsonObject obj{{"item", toJson(data._item)}};
    if (data._workDoneToken.has_value())
        obj.insert("workDoneToken", toJsonValue(*data._workDoneToken));
    if (data._partialResultToken.has_value())
        obj.insert("partialResultToken", toJsonValue(*data._partialResultToken));
    return obj;
}

template<>
Utils::Result<TypeHierarchySubtypesParams> fromJson<TypeHierarchySubtypesParams>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for TypeHierarchySubtypesParams");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("item"))
        return Utils::ResultError("Missing required field: item");
    TypeHierarchySubtypesParams result;
    if (obj.contains("workDoneToken")) {
        const auto res0 = fromJson<ProgressToken>("workDoneToken", obj["workDoneToken"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._workDoneToken = *res0;
    }
    if (obj.contains("partialResultToken")) {
        const auto res1 = fromJson<ProgressToken>("partialResultToken", obj["partialResultToken"]);
        if (!res1)
            return Utils::ResultError(res1.error());
        result._partialResultToken = *res1;
    }
    if (obj.contains("item") && obj["item"].isObject()) {
        const auto res2 = fromJson<TypeHierarchyItem>("item", obj["item"]);
        if (!res2)
            return Utils::ResultError(res2.error());
        result._item = *res2;
    }
    return result;
}

QJsonObject toJson(const TypeHierarchySubtypesParams &data)
{
    QJsonObject obj{{"item", toJson(data._item)}};
    if (data._workDoneToken.has_value())
        obj.insert("workDoneToken", toJsonValue(*data._workDoneToken));
    if (data._partialResultToken.has_value())
        obj.insert("partialResultToken", toJsonValue(*data._partialResultToken));
    return obj;
}

template<>
Utils::Result<InlineValueContext> fromJson<InlineValueContext>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for InlineValueContext");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("frameId"))
        return Utils::ResultError("Missing required field: frameId");
    if (!obj.contains("stoppedLocation"))
        return Utils::ResultError("Missing required field: stoppedLocation");
    InlineValueContext result;
    result._frameId = obj.value("frameId").toInt();
    if (obj.contains("stoppedLocation") && obj["stoppedLocation"].isObject()) {
        const auto res0 = fromJson<Range>("stoppedLocation", obj["stoppedLocation"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._stoppedLocation = *res0;
    }
    return result;
}

QJsonObject toJson(const InlineValueContext &data)
{
    QJsonObject obj{
        {"frameId", data._frameId},
        {"stoppedLocation", toJson(data._stoppedLocation)}
    };
    return obj;
}

template<>
Utils::Result<InlineValueParams> fromJson<InlineValueParams>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for InlineValueParams");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("textDocument"))
        return Utils::ResultError("Missing required field: textDocument");
    if (!obj.contains("range"))
        return Utils::ResultError("Missing required field: range");
    if (!obj.contains("context"))
        return Utils::ResultError("Missing required field: context");
    InlineValueParams result;
    if (obj.contains("workDoneToken")) {
        const auto res0 = fromJson<ProgressToken>("workDoneToken", obj["workDoneToken"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._workDoneToken = *res0;
    }
    if (obj.contains("textDocument") && obj["textDocument"].isObject()) {
        const auto res1 = fromJson<TextDocumentIdentifier>("textDocument", obj["textDocument"]);
        if (!res1)
            return Utils::ResultError(res1.error());
        result._textDocument = *res1;
    }
    if (obj.contains("range") && obj["range"].isObject()) {
        const auto res2 = fromJson<Range>("range", obj["range"]);
        if (!res2)
            return Utils::ResultError(res2.error());
        result._range = *res2;
    }
    if (obj.contains("context") && obj["context"].isObject()) {
        const auto res3 = fromJson<InlineValueContext>("context", obj["context"]);
        if (!res3)
            return Utils::ResultError(res3.error());
        result._context = *res3;
    }
    return result;
}

QJsonObject toJson(const InlineValueParams &data)
{
    QJsonObject obj{
        {"textDocument", toJson(data._textDocument)},
        {"range", toJson(data._range)},
        {"context", toJson(data._context)}
    };
    if (data._workDoneToken.has_value())
        obj.insert("workDoneToken", toJsonValue(*data._workDoneToken));
    return obj;
}

template<>
Utils::Result<InlineValueRegistrationOptions> fromJson<InlineValueRegistrationOptions>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for InlineValueRegistrationOptions");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("documentSelector"))
        return Utils::ResultError("Missing required field: documentSelector");
    InlineValueRegistrationOptions result;
    if (obj.contains("workDoneProgress"))
        result._workDoneProgress = obj.value("workDoneProgress").toBool();
    if (obj.contains("documentSelector") && !obj["documentSelector"].isNull()) {
        const auto res0 = fromJson<DocumentSelector>("documentSelector", obj["documentSelector"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._documentSelector = *res0;
    }
    if (obj.contains("id"))
        result._id = obj.value("id").toString();
    return result;
}

QJsonObject toJson(const InlineValueRegistrationOptions &data)
{
    QJsonObject obj;
    if (data._workDoneProgress.has_value())
        obj.insert("workDoneProgress", *data._workDoneProgress);
    if (data._documentSelector.has_value())
        obj.insert("documentSelector", toJson(*data._documentSelector));
    else
        obj.insert("documentSelector", QJsonValue::Null);
    if (data._id.has_value())
        obj.insert("id", *data._id);
    return obj;
}

template<>
Utils::Result<InlayHintParams> fromJson<InlayHintParams>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for InlayHintParams");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("textDocument"))
        return Utils::ResultError("Missing required field: textDocument");
    if (!obj.contains("range"))
        return Utils::ResultError("Missing required field: range");
    InlayHintParams result;
    if (obj.contains("workDoneToken")) {
        const auto res0 = fromJson<ProgressToken>("workDoneToken", obj["workDoneToken"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._workDoneToken = *res0;
    }
    if (obj.contains("textDocument") && obj["textDocument"].isObject()) {
        const auto res1 = fromJson<TextDocumentIdentifier>("textDocument", obj["textDocument"]);
        if (!res1)
            return Utils::ResultError(res1.error());
        result._textDocument = *res1;
    }
    if (obj.contains("range") && obj["range"].isObject()) {
        const auto res2 = fromJson<Range>("range", obj["range"]);
        if (!res2)
            return Utils::ResultError(res2.error());
        result._range = *res2;
    }
    return result;
}

QJsonObject toJson(const InlayHintParams &data)
{
    QJsonObject obj{
        {"textDocument", toJson(data._textDocument)},
        {"range", toJson(data._range)}
    };
    if (data._workDoneToken.has_value())
        obj.insert("workDoneToken", toJsonValue(*data._workDoneToken));
    return obj;
}

template<>
Utils::Result<Command> fromJson<Command>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for Command");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("title"))
        return Utils::ResultError("Missing required field: title");
    if (!obj.contains("command"))
        return Utils::ResultError("Missing required field: command");
    Command result;
    result._title = obj.value("title").toString();
    if (obj.contains("tooltip"))
        result._tooltip = obj.value("tooltip").toString();
    result._command = obj.value("command").toString();
    if (obj.contains("arguments") && obj["arguments"].isArray()) {
        const QJsonArray arr = obj["arguments"].toArray();
        QList<QJsonValue> list_arguments;
        for (const QJsonValue &v : arr) {
            list_arguments.append(v);
        }
        result._arguments = list_arguments;
    }
    return result;
}

QJsonObject toJson(const Command &data)
{
    QJsonObject obj{
        {"title", data._title},
        {"command", data._command}
    };
    if (data._tooltip.has_value())
        obj.insert("tooltip", *data._tooltip);
    if (data._arguments.has_value()) {
        QJsonArray arr_arguments;
        for (const auto &v : *data._arguments) arr_arguments.append(v);
        obj.insert("arguments", arr_arguments);
    }
    return obj;
}

QString toString(MarkupKind v)
{
    switch(v) {
        case MarkupKind::plaintext: return "plaintext";
        case MarkupKind::markdown: return "markdown";
    }
    return {};
}

template<>
Utils::Result<MarkupKind> fromJson<MarkupKind>(const QJsonValue &val)
{
    if (!val.isString())
        return Utils::ResultError("Expected JSON string for MarkupKind");
    const QString str = val.toString();
    if (str == "plaintext") return MarkupKind::plaintext;
    if (str == "markdown") return MarkupKind::markdown;
    return Utils::ResultError("Invalid MarkupKind value: " + str);
}

QJsonValue toJsonValue(const MarkupKind &v)
{
    return toString(v);
}

template<>
Utils::Result<MarkupContent> fromJson<MarkupContent>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for MarkupContent");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("kind"))
        return Utils::ResultError("Missing required field: kind");
    if (!obj.contains("value"))
        return Utils::ResultError("Missing required field: value");
    MarkupContent result;
    const auto res0 = fromJson<MarkupKind>("kind", obj["kind"]);
    if (!res0)
        return Utils::ResultError(res0.error());
    result._kind = *res0;
    result._value = obj.value("value").toString();
    return result;
}

QJsonObject toJson(const MarkupContent &data)
{
    QJsonObject obj{
        {"kind", toJsonValue(data._kind)},
        {"value", data._value}
    };
    return obj;
}

template<>
Utils::Result<InlayHintLabelPartTooltip> fromJson<InlayHintLabelPartTooltip>(const QJsonValue &val)
{
    if (val.isString())
        return InlayHintLabelPartTooltip(val.toString());
    if (!val.isObject())
        return Utils::ResultError("Invalid InlayHintLabelPartTooltip: expected object or array");
    const QJsonObject obj = val.toObject();
    if (obj.contains("kind")) {
        const auto res0 = fromJson<MarkupContent>(val);
        if (!res0)
            return Utils::ResultError(res0.error());
        return InlayHintLabelPartTooltip(*res0);
    }
    return Utils::ResultError("Invalid InlayHintLabelPartTooltip");
}

QJsonValue toJsonValue(const InlayHintLabelPartTooltip &val)
{
    return std::visit([](const auto &v) -> QJsonValue {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, MarkupContent>) {
            return toJson(v);
        } else
        {
            return QVariant::fromValue(v).toJsonValue();
        }
    }, val);
}

template<>
Utils::Result<InlayHintLabelPart> fromJson<InlayHintLabelPart>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for InlayHintLabelPart");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("value"))
        return Utils::ResultError("Missing required field: value");
    InlayHintLabelPart result;
    result._value = obj.value("value").toString();
    if (obj.contains("tooltip")) {
        const auto res0 = fromJson<InlayHintLabelPartTooltip>("tooltip", obj["tooltip"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._tooltip = *res0;
    }
    if (obj.contains("location") && obj["location"].isObject()) {
        const auto res1 = fromJson<Location>("location", obj["location"]);
        if (!res1)
            return Utils::ResultError(res1.error());
        result._location = *res1;
    }
    if (obj.contains("command") && obj["command"].isObject()) {
        const auto res2 = fromJson<Command>("command", obj["command"]);
        if (!res2)
            return Utils::ResultError(res2.error());
        result._command = *res2;
    }
    return result;
}

QJsonObject toJson(const InlayHintLabelPart &data)
{
    QJsonObject obj{{"value", data._value}};
    if (data._tooltip.has_value())
        obj.insert("tooltip", toJsonValue(*data._tooltip));
    if (data._location.has_value())
        obj.insert("location", toJson(*data._location));
    if (data._command.has_value())
        obj.insert("command", toJson(*data._command));
    return obj;
}

template<>
Utils::Result<InlayHintLabel> fromJson<InlayHintLabel>(const QJsonValue &val)
{
    if (val.isString())
        return InlayHintLabel(val.toString());
    if (val.isArray()) {
        QList<InlayHintLabelPart> list;
        for (const QJsonValue &v : val.toArray()) {
            const auto res0 = fromJson<InlayHintLabelPart>(v);
            if (!res0)
                return Utils::ResultError(res0.error());
            list.append(*res0);
        }
        return InlayHintLabel(std::move(list));
    }
    return Utils::ResultError("Invalid InlayHintLabel");
}

QJsonValue toJsonValue(const InlayHintLabel &val)
{
    return std::visit([](const auto &v) -> QJsonValue {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, QList<InlayHintLabelPart>>) {
            QJsonArray arr;
            for (const auto &elem : v)
                arr.append(toJson(elem));
            return arr;
        } else
        {
            return QVariant::fromValue(v).toJsonValue();
        }
    }, val);
}

template<>
Utils::Result<InlayHint> fromJson<InlayHint>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for InlayHint");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("position"))
        return Utils::ResultError("Missing required field: position");
    if (!obj.contains("label"))
        return Utils::ResultError("Missing required field: label");
    InlayHint result;
    if (obj.contains("position") && obj["position"].isObject()) {
        const auto res0 = fromJson<Position>("position", obj["position"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._position = *res0;
    }
    if (obj.contains("label")) {
        const auto res1 = fromJson<InlayHintLabel>("label", obj["label"]);
        if (!res1)
            return Utils::ResultError(res1.error());
        result._label = *res1;
    }
    if (obj.contains("kind") && obj["kind"].isDouble())
        result._kind = obj["kind"].toInt();
    if (obj.contains("textEdits") && obj["textEdits"].isArray()) {
        const QJsonArray arr = obj["textEdits"].toArray();
        QList<TextEdit> list_textEdits;
        for (const QJsonValue &v : arr) {
            const auto res2 = fromJson<TextEdit>("textEdits", v);
            if (!res2)
                return Utils::ResultError(res2.error());
            list_textEdits.append(*res2);
        }
        result._textEdits = list_textEdits;
    }
    if (obj.contains("tooltip")) {
        const auto res3 = fromJson<InlayHintLabelPartTooltip>("tooltip", obj["tooltip"]);
        if (!res3)
            return Utils::ResultError(res3.error());
        result._tooltip = *res3;
    }
    if (obj.contains("paddingLeft"))
        result._paddingLeft = obj.value("paddingLeft").toBool();
    if (obj.contains("paddingRight"))
        result._paddingRight = obj.value("paddingRight").toBool();
    if (obj.contains("data"))
        result._data = obj.value("data");
    return result;
}

QJsonObject toJson(const InlayHint &data)
{
    QJsonObject obj{
        {"position", toJson(data._position)},
        {"label", toJsonValue(data._label)}
    };
    if (data._kind.has_value())
        obj.insert("kind", *data._kind);
    if (data._textEdits.has_value()) {
        QJsonArray arr_textEdits;
        for (const auto &v : *data._textEdits) arr_textEdits.append(toJson(v));
        obj.insert("textEdits", arr_textEdits);
    }
    if (data._tooltip.has_value())
        obj.insert("tooltip", toJsonValue(*data._tooltip));
    if (data._paddingLeft.has_value())
        obj.insert("paddingLeft", *data._paddingLeft);
    if (data._paddingRight.has_value())
        obj.insert("paddingRight", *data._paddingRight);
    if (data._data.has_value())
        obj.insert("data", *data._data);
    return obj;
}

template<>
Utils::Result<InlayHintRegistrationOptions> fromJson<InlayHintRegistrationOptions>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for InlayHintRegistrationOptions");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("documentSelector"))
        return Utils::ResultError("Missing required field: documentSelector");
    InlayHintRegistrationOptions result;
    if (obj.contains("workDoneProgress"))
        result._workDoneProgress = obj.value("workDoneProgress").toBool();
    if (obj.contains("resolveProvider"))
        result._resolveProvider = obj.value("resolveProvider").toBool();
    if (obj.contains("documentSelector") && !obj["documentSelector"].isNull()) {
        const auto res0 = fromJson<DocumentSelector>("documentSelector", obj["documentSelector"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._documentSelector = *res0;
    }
    if (obj.contains("id"))
        result._id = obj.value("id").toString();
    return result;
}

QJsonObject toJson(const InlayHintRegistrationOptions &data)
{
    QJsonObject obj;
    if (data._workDoneProgress.has_value())
        obj.insert("workDoneProgress", *data._workDoneProgress);
    if (data._resolveProvider.has_value())
        obj.insert("resolveProvider", *data._resolveProvider);
    if (data._documentSelector.has_value())
        obj.insert("documentSelector", toJson(*data._documentSelector));
    else
        obj.insert("documentSelector", QJsonValue::Null);
    if (data._id.has_value())
        obj.insert("id", *data._id);
    return obj;
}

template<>
Utils::Result<DocumentDiagnosticParams> fromJson<DocumentDiagnosticParams>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for DocumentDiagnosticParams");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("textDocument"))
        return Utils::ResultError("Missing required field: textDocument");
    DocumentDiagnosticParams result;
    if (obj.contains("workDoneToken")) {
        const auto res0 = fromJson<ProgressToken>("workDoneToken", obj["workDoneToken"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._workDoneToken = *res0;
    }
    if (obj.contains("partialResultToken")) {
        const auto res1 = fromJson<ProgressToken>("partialResultToken", obj["partialResultToken"]);
        if (!res1)
            return Utils::ResultError(res1.error());
        result._partialResultToken = *res1;
    }
    if (obj.contains("textDocument") && obj["textDocument"].isObject()) {
        const auto res2 = fromJson<TextDocumentIdentifier>("textDocument", obj["textDocument"]);
        if (!res2)
            return Utils::ResultError(res2.error());
        result._textDocument = *res2;
    }
    if (obj.contains("identifier"))
        result._identifier = obj.value("identifier").toString();
    if (obj.contains("previousResultId"))
        result._previousResultId = obj.value("previousResultId").toString();
    return result;
}

QJsonObject toJson(const DocumentDiagnosticParams &data)
{
    QJsonObject obj{{"textDocument", toJson(data._textDocument)}};
    if (data._workDoneToken.has_value())
        obj.insert("workDoneToken", toJsonValue(*data._workDoneToken));
    if (data._partialResultToken.has_value())
        obj.insert("partialResultToken", toJsonValue(*data._partialResultToken));
    if (data._identifier.has_value())
        obj.insert("identifier", *data._identifier);
    if (data._previousResultId.has_value())
        obj.insert("previousResultId", *data._previousResultId);
    return obj;
}

template<>
Utils::Result<DiagnosticServerCancellationData> fromJson<DiagnosticServerCancellationData>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for DiagnosticServerCancellationData");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("retriggerRequest"))
        return Utils::ResultError("Missing required field: retriggerRequest");
    DiagnosticServerCancellationData result;
    result._retriggerRequest = obj.value("retriggerRequest").toBool();
    return result;
}

QJsonObject toJson(const DiagnosticServerCancellationData &data)
{
    QJsonObject obj{{"retriggerRequest", data._retriggerRequest}};
    return obj;
}

template<>
Utils::Result<DiagnosticRegistrationOptions> fromJson<DiagnosticRegistrationOptions>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for DiagnosticRegistrationOptions");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("documentSelector"))
        return Utils::ResultError("Missing required field: documentSelector");
    if (!obj.contains("interFileDependencies"))
        return Utils::ResultError("Missing required field: interFileDependencies");
    if (!obj.contains("workspaceDiagnostics"))
        return Utils::ResultError("Missing required field: workspaceDiagnostics");
    DiagnosticRegistrationOptions result;
    if (obj.contains("documentSelector") && !obj["documentSelector"].isNull()) {
        const auto res0 = fromJson<DocumentSelector>("documentSelector", obj["documentSelector"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._documentSelector = *res0;
    }
    if (obj.contains("workDoneProgress"))
        result._workDoneProgress = obj.value("workDoneProgress").toBool();
    if (obj.contains("identifier"))
        result._identifier = obj.value("identifier").toString();
    result._interFileDependencies = obj.value("interFileDependencies").toBool();
    result._workspaceDiagnostics = obj.value("workspaceDiagnostics").toBool();
    if (obj.contains("id"))
        result._id = obj.value("id").toString();
    return result;
}

QJsonObject toJson(const DiagnosticRegistrationOptions &data)
{
    QJsonObject obj{
        {"interFileDependencies", data._interFileDependencies},
        {"workspaceDiagnostics", data._workspaceDiagnostics}
    };
    if (data._documentSelector.has_value())
        obj.insert("documentSelector", toJson(*data._documentSelector));
    else
        obj.insert("documentSelector", QJsonValue::Null);
    if (data._workDoneProgress.has_value())
        obj.insert("workDoneProgress", *data._workDoneProgress);
    if (data._identifier.has_value())
        obj.insert("identifier", *data._identifier);
    if (data._id.has_value())
        obj.insert("id", *data._id);
    return obj;
}

template<>
Utils::Result<PreviousResultId> fromJson<PreviousResultId>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for PreviousResultId");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("uri"))
        return Utils::ResultError("Missing required field: uri");
    if (!obj.contains("value"))
        return Utils::ResultError("Missing required field: value");
    PreviousResultId result;
    result._uri = obj.value("uri").toString();
    result._value = obj.value("value").toString();
    return result;
}

QJsonObject toJson(const PreviousResultId &data)
{
    QJsonObject obj{
        {"uri", data._uri},
        {"value", data._value}
    };
    return obj;
}

template<>
Utils::Result<WorkspaceDiagnosticParams> fromJson<WorkspaceDiagnosticParams>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for WorkspaceDiagnosticParams");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("previousResultIds"))
        return Utils::ResultError("Missing required field: previousResultIds");
    WorkspaceDiagnosticParams result;
    if (obj.contains("workDoneToken")) {
        const auto res0 = fromJson<ProgressToken>("workDoneToken", obj["workDoneToken"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._workDoneToken = *res0;
    }
    if (obj.contains("partialResultToken")) {
        const auto res1 = fromJson<ProgressToken>("partialResultToken", obj["partialResultToken"]);
        if (!res1)
            return Utils::ResultError(res1.error());
        result._partialResultToken = *res1;
    }
    if (obj.contains("identifier"))
        result._identifier = obj.value("identifier").toString();
    if (obj.contains("previousResultIds") && obj["previousResultIds"].isArray()) {
        const QJsonArray arr = obj["previousResultIds"].toArray();
        for (const QJsonValue &v : arr) {
            const auto res2 = fromJson<PreviousResultId>("previousResultIds", v);
            if (!res2)
                return Utils::ResultError(res2.error());
            result._previousResultIds.append(*res2);
        }
    }
    return result;
}

QJsonObject toJson(const WorkspaceDiagnosticParams &data)
{
    QJsonObject obj;
    if (data._workDoneToken.has_value())
        obj.insert("workDoneToken", toJsonValue(*data._workDoneToken));
    if (data._partialResultToken.has_value())
        obj.insert("partialResultToken", toJsonValue(*data._partialResultToken));
    if (data._identifier.has_value())
        obj.insert("identifier", *data._identifier);
    QJsonArray arr_previousResultIds;
    for (const auto &v : data._previousResultIds) arr_previousResultIds.append(toJson(v));
    obj.insert("previousResultIds", arr_previousResultIds);
    return obj;
}

template<>
Utils::Result<CodeDescription> fromJson<CodeDescription>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for CodeDescription");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("href"))
        return Utils::ResultError("Missing required field: href");
    CodeDescription result;
    result._href = obj.value("href").toString();
    return result;
}

QJsonObject toJson(const CodeDescription &data)
{
    QJsonObject obj{{"href", data._href}};
    return obj;
}

template<>
Utils::Result<DiagnosticRelatedInformation> fromJson<DiagnosticRelatedInformation>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for DiagnosticRelatedInformation");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("location"))
        return Utils::ResultError("Missing required field: location");
    if (!obj.contains("message"))
        return Utils::ResultError("Missing required field: message");
    DiagnosticRelatedInformation result;
    if (obj.contains("location") && obj["location"].isObject()) {
        const auto res0 = fromJson<Location>("location", obj["location"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._location = *res0;
    }
    result._message = obj.value("message").toString();
    return result;
}

QJsonObject toJson(const DiagnosticRelatedInformation &data)
{
    QJsonObject obj{
        {"location", toJson(data._location)},
        {"message", data._message}
    };
    return obj;
}

template<>
Utils::Result<Diagnostic> fromJson<Diagnostic>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for Diagnostic");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("range"))
        return Utils::ResultError("Missing required field: range");
    if (!obj.contains("message"))
        return Utils::ResultError("Missing required field: message");
    Diagnostic result;
    if (obj.contains("range") && obj["range"].isObject()) {
        const auto res0 = fromJson<Range>("range", obj["range"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._range = *res0;
    }
    if (obj.contains("severity") && obj["severity"].isDouble())
        result._severity = obj["severity"].toInt();
    if (obj.contains("code")) {
        const auto res1 = fromJson<ProgressToken>("code", obj["code"]);
        if (!res1)
            return Utils::ResultError(res1.error());
        result._code = *res1;
    }
    if (obj.contains("codeDescription") && obj["codeDescription"].isObject()) {
        const auto res2 = fromJson<CodeDescription>("codeDescription", obj["codeDescription"]);
        if (!res2)
            return Utils::ResultError(res2.error());
        result._codeDescription = *res2;
    }
    if (obj.contains("source"))
        result._source = obj.value("source").toString();
    if (obj.contains("message")) {
        const auto res3 = fromJson<InlayHintLabelPartTooltip>("message", obj["message"]);
        if (!res3)
            return Utils::ResultError(res3.error());
        result._message = *res3;
    }
    if (obj.contains("tags") && obj["tags"].isArray()) {
        const QJsonArray arr = obj["tags"].toArray();
        QList<int> list_tags;
        for (const QJsonValue &v : arr) {
            list_tags.append(v.toInt());
        }
        result._tags = list_tags;
    }
    if (obj.contains("relatedInformation") && obj["relatedInformation"].isArray()) {
        const QJsonArray arr = obj["relatedInformation"].toArray();
        QList<DiagnosticRelatedInformation> list_relatedInformation;
        for (const QJsonValue &v : arr) {
            const auto res4 = fromJson<DiagnosticRelatedInformation>("relatedInformation", v);
            if (!res4)
                return Utils::ResultError(res4.error());
            list_relatedInformation.append(*res4);
        }
        result._relatedInformation = list_relatedInformation;
    }
    if (obj.contains("data"))
        result._data = obj.value("data");
    return result;
}

QJsonObject toJson(const Diagnostic &data)
{
    QJsonObject obj{
        {"range", toJson(data._range)},
        {"message", toJsonValue(data._message)}
    };
    if (data._severity.has_value())
        obj.insert("severity", *data._severity);
    if (data._code.has_value())
        obj.insert("code", toJsonValue(*data._code));
    if (data._codeDescription.has_value())
        obj.insert("codeDescription", toJson(*data._codeDescription));
    if (data._source.has_value())
        obj.insert("source", *data._source);
    if (data._tags.has_value()) {
        QJsonArray arr_tags;
        for (const auto &v : *data._tags) arr_tags.append(v);
        obj.insert("tags", arr_tags);
    }
    if (data._relatedInformation.has_value()) {
        QJsonArray arr_relatedInformation;
        for (const auto &v : *data._relatedInformation) arr_relatedInformation.append(toJson(v));
        obj.insert("relatedInformation", arr_relatedInformation);
    }
    if (data._data.has_value())
        obj.insert("data", *data._data);
    return obj;
}

template<>
Utils::Result<WorkspaceFullDocumentDiagnosticReport> fromJson<WorkspaceFullDocumentDiagnosticReport>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for WorkspaceFullDocumentDiagnosticReport");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("kind"))
        return Utils::ResultError("Missing required field: kind");
    if (!obj.contains("items"))
        return Utils::ResultError("Missing required field: items");
    if (!obj.contains("uri"))
        return Utils::ResultError("Missing required field: uri");
    if (!obj.contains("version"))
        return Utils::ResultError("Missing required field: version");
    WorkspaceFullDocumentDiagnosticReport result;
    if (obj.value("kind").toString() != "full")
        return Utils::ResultError("Field 'kind' must be 'full', got: " + obj.value("kind").toString());
    if (obj.contains("resultId"))
        result._resultId = obj.value("resultId").toString();
    if (obj.contains("items") && obj["items"].isArray()) {
        const QJsonArray arr = obj["items"].toArray();
        for (const QJsonValue &v : arr) {
            const auto res0 = fromJson<Diagnostic>("items", v);
            if (!res0)
                return Utils::ResultError(res0.error());
            result._items.append(*res0);
        }
    }
    result._uri = obj.value("uri").toString();
    if (!obj["version"].isNull()) {
        result._version = obj.value("version").toInt();
    }
    return result;
}

QJsonObject toJson(const WorkspaceFullDocumentDiagnosticReport &data)
{
    QJsonObject obj{
        {"kind", QString("full")},
        {"uri", data._uri}
    };
    if (data._resultId.has_value())
        obj.insert("resultId", *data._resultId);
    QJsonArray arr_items;
    for (const auto &v : data._items) arr_items.append(toJson(v));
    obj.insert("items", arr_items);
    if (data._version.has_value())
        obj.insert("version", *data._version);
    else
        obj.insert("version", QJsonValue::Null);
    return obj;
}

template<>
Utils::Result<WorkspaceUnchangedDocumentDiagnosticReport> fromJson<WorkspaceUnchangedDocumentDiagnosticReport>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for WorkspaceUnchangedDocumentDiagnosticReport");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("kind"))
        return Utils::ResultError("Missing required field: kind");
    if (!obj.contains("resultId"))
        return Utils::ResultError("Missing required field: resultId");
    if (!obj.contains("uri"))
        return Utils::ResultError("Missing required field: uri");
    if (!obj.contains("version"))
        return Utils::ResultError("Missing required field: version");
    WorkspaceUnchangedDocumentDiagnosticReport result;
    if (obj.value("kind").toString() != "unchanged")
        return Utils::ResultError("Field 'kind' must be 'unchanged', got: " + obj.value("kind").toString());
    result._resultId = obj.value("resultId").toString();
    result._uri = obj.value("uri").toString();
    if (!obj["version"].isNull()) {
        result._version = obj.value("version").toInt();
    }
    return result;
}

QJsonObject toJson(const WorkspaceUnchangedDocumentDiagnosticReport &data)
{
    QJsonObject obj{
        {"kind", QString("unchanged")},
        {"resultId", data._resultId},
        {"uri", data._uri}
    };
    if (data._version.has_value())
        obj.insert("version", *data._version);
    else
        obj.insert("version", QJsonValue::Null);
    return obj;
}

template<>
Utils::Result<WorkspaceDocumentDiagnosticReport> fromJson<WorkspaceDocumentDiagnosticReport>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Invalid WorkspaceDocumentDiagnosticReport: expected object");
    const QString dispatchValue = val.toObject().value("kind").toString();
    if (dispatchValue == "full") {
        const auto res0 = fromJson<WorkspaceFullDocumentDiagnosticReport>(val);
        if (!res0)
            return Utils::ResultError(res0.error());
        return WorkspaceDocumentDiagnosticReport(*res0);
    }
    else if (dispatchValue == "unchanged") {
        const auto res1 = fromJson<WorkspaceUnchangedDocumentDiagnosticReport>(val);
        if (!res1)
            return Utils::ResultError(res1.error());
        return WorkspaceDocumentDiagnosticReport(*res1);
    }
    return Utils::ResultError("Invalid WorkspaceDocumentDiagnosticReport: unknown kind \"" + dispatchValue + "\"");
}

QJsonObject toJson(const WorkspaceDocumentDiagnosticReport &val)
{
    return std::visit([](const auto &v) -> QJsonObject {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, QJsonObject>) {
            return v;
        } else {
            return toJson(v);
        }
    }, val);
}

QJsonValue toJsonValue(const WorkspaceDocumentDiagnosticReport &val)
{
    return toJson(val);
}

QString dispatchValue(const WorkspaceDocumentDiagnosticReport &val)
{
    return std::visit([](const auto &v) -> QString {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, WorkspaceFullDocumentDiagnosticReport>) return "full";
        else if constexpr (std::is_same_v<T, WorkspaceUnchangedDocumentDiagnosticReport>) return "unchanged";
        return {};
    }, val);
}

QString uri(const WorkspaceDocumentDiagnosticReport &val)
{
    return std::visit([](const auto &v) -> QString { return v._uri; }, val);
}

template<>
Utils::Result<WorkspaceDiagnosticReport> fromJson<WorkspaceDiagnosticReport>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for WorkspaceDiagnosticReport");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("items"))
        return Utils::ResultError("Missing required field: items");
    WorkspaceDiagnosticReport result;
    if (obj.contains("items") && obj["items"].isArray()) {
        const QJsonArray arr = obj["items"].toArray();
        for (const QJsonValue &v : arr) {
            const auto res0 = fromJson<WorkspaceDocumentDiagnosticReport>("items", v);
            if (!res0)
                return Utils::ResultError(res0.error());
            result._items.append(*res0);
        }
    }
    return result;
}

QJsonObject toJson(const WorkspaceDiagnosticReport &data)
{
    QJsonObject obj;
    QJsonArray arr_items;
    for (const auto &v : data._items) arr_items.append(toJsonValue(v));
    obj.insert("items", arr_items);
    return obj;
}

template<>
Utils::Result<WorkspaceDiagnosticReportPartialResult> fromJson<WorkspaceDiagnosticReportPartialResult>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for WorkspaceDiagnosticReportPartialResult");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("items"))
        return Utils::ResultError("Missing required field: items");
    WorkspaceDiagnosticReportPartialResult result;
    if (obj.contains("items") && obj["items"].isArray()) {
        const QJsonArray arr = obj["items"].toArray();
        for (const QJsonValue &v : arr) {
            const auto res0 = fromJson<WorkspaceDocumentDiagnosticReport>("items", v);
            if (!res0)
                return Utils::ResultError(res0.error());
            result._items.append(*res0);
        }
    }
    return result;
}

QJsonObject toJson(const WorkspaceDiagnosticReportPartialResult &data)
{
    QJsonObject obj;
    QJsonArray arr_items;
    for (const auto &v : data._items) arr_items.append(toJsonValue(v));
    obj.insert("items", arr_items);
    return obj;
}

template<>
Utils::Result<ExecutionSummary> fromJson<ExecutionSummary>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for ExecutionSummary");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("executionOrder"))
        return Utils::ResultError("Missing required field: executionOrder");
    ExecutionSummary result;
    result._executionOrder = obj.value("executionOrder").toInt();
    if (obj.contains("success"))
        result._success = obj.value("success").toBool();
    return result;
}

QJsonObject toJson(const ExecutionSummary &data)
{
    QJsonObject obj{{"executionOrder", data._executionOrder}};
    if (data._success.has_value())
        obj.insert("success", *data._success);
    return obj;
}

template<>
Utils::Result<NotebookCell> fromJson<NotebookCell>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for NotebookCell");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("kind"))
        return Utils::ResultError("Missing required field: kind");
    if (!obj.contains("document"))
        return Utils::ResultError("Missing required field: document");
    NotebookCell result;
    result._kind = obj.value("kind").toInt();
    result._document = obj.value("document").toString();
    if (obj.contains("metadata"))
        result._metadata = obj.value("metadata").toObject();
    if (obj.contains("executionSummary") && obj["executionSummary"].isObject()) {
        const auto res0 = fromJson<ExecutionSummary>("executionSummary", obj["executionSummary"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._executionSummary = *res0;
    }
    return result;
}

QJsonObject toJson(const NotebookCell &data)
{
    QJsonObject obj{
        {"kind", data._kind},
        {"document", data._document}
    };
    if (data._metadata.has_value())
        obj.insert("metadata", *data._metadata);
    if (data._executionSummary.has_value())
        obj.insert("executionSummary", toJson(*data._executionSummary));
    return obj;
}

template<>
Utils::Result<NotebookDocument> fromJson<NotebookDocument>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for NotebookDocument");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("uri"))
        return Utils::ResultError("Missing required field: uri");
    if (!obj.contains("notebookType"))
        return Utils::ResultError("Missing required field: notebookType");
    if (!obj.contains("version"))
        return Utils::ResultError("Missing required field: version");
    if (!obj.contains("cells"))
        return Utils::ResultError("Missing required field: cells");
    NotebookDocument result;
    result._uri = obj.value("uri").toString();
    result._notebookType = obj.value("notebookType").toString();
    result._version = obj.value("version").toInt();
    if (obj.contains("metadata"))
        result._metadata = obj.value("metadata").toObject();
    if (obj.contains("cells") && obj["cells"].isArray()) {
        const QJsonArray arr = obj["cells"].toArray();
        for (const QJsonValue &v : arr) {
            const auto res0 = fromJson<NotebookCell>("cells", v);
            if (!res0)
                return Utils::ResultError(res0.error());
            result._cells.append(*res0);
        }
    }
    return result;
}

QJsonObject toJson(const NotebookDocument &data)
{
    QJsonObject obj{
        {"uri", data._uri},
        {"notebookType", data._notebookType},
        {"version", data._version}
    };
    if (data._metadata.has_value())
        obj.insert("metadata", *data._metadata);
    QJsonArray arr_cells;
    for (const auto &v : data._cells) arr_cells.append(toJson(v));
    obj.insert("cells", arr_cells);
    return obj;
}

template<>
Utils::Result<TextDocumentItem> fromJson<TextDocumentItem>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for TextDocumentItem");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("uri"))
        return Utils::ResultError("Missing required field: uri");
    if (!obj.contains("languageId"))
        return Utils::ResultError("Missing required field: languageId");
    if (!obj.contains("version"))
        return Utils::ResultError("Missing required field: version");
    if (!obj.contains("text"))
        return Utils::ResultError("Missing required field: text");
    TextDocumentItem result;
    result._uri = obj.value("uri").toString();
    if (obj.contains("languageId") && obj["languageId"].isString()) {
        const auto res0 = fromJson<LanguageKind>("languageId", obj["languageId"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._languageId = *res0;
    }
    result._version = obj.value("version").toInt();
    result._text = obj.value("text").toString();
    return result;
}

QJsonObject toJson(const TextDocumentItem &data)
{
    QJsonObject obj{
        {"uri", data._uri},
        {"languageId", data._languageId},
        {"version", data._version},
        {"text", data._text}
    };
    return obj;
}

template<>
Utils::Result<DidOpenNotebookDocumentParams> fromJson<DidOpenNotebookDocumentParams>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for DidOpenNotebookDocumentParams");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("notebookDocument"))
        return Utils::ResultError("Missing required field: notebookDocument");
    if (!obj.contains("cellTextDocuments"))
        return Utils::ResultError("Missing required field: cellTextDocuments");
    DidOpenNotebookDocumentParams result;
    if (obj.contains("notebookDocument") && obj["notebookDocument"].isObject()) {
        const auto res0 = fromJson<NotebookDocument>("notebookDocument", obj["notebookDocument"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._notebookDocument = *res0;
    }
    if (obj.contains("cellTextDocuments") && obj["cellTextDocuments"].isArray()) {
        const QJsonArray arr = obj["cellTextDocuments"].toArray();
        for (const QJsonValue &v : arr) {
            const auto res1 = fromJson<TextDocumentItem>("cellTextDocuments", v);
            if (!res1)
                return Utils::ResultError(res1.error());
            result._cellTextDocuments.append(*res1);
        }
    }
    return result;
}

QJsonObject toJson(const DidOpenNotebookDocumentParams &data)
{
    QJsonObject obj{{"notebookDocument", toJson(data._notebookDocument)}};
    QJsonArray arr_cellTextDocuments;
    for (const auto &v : data._cellTextDocuments) arr_cellTextDocuments.append(toJson(v));
    obj.insert("cellTextDocuments", arr_cellTextDocuments);
    return obj;
}

template<>
Utils::Result<NotebookCellLanguage> fromJson<NotebookCellLanguage>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for NotebookCellLanguage");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("language"))
        return Utils::ResultError("Missing required field: language");
    NotebookCellLanguage result;
    result._language = obj.value("language").toString();
    return result;
}

QJsonObject toJson(const NotebookCellLanguage &data)
{
    QJsonObject obj{{"language", data._language}};
    return obj;
}

template<>
Utils::Result<NotebookDocumentFilterWithCells> fromJson<NotebookDocumentFilterWithCells>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for NotebookDocumentFilterWithCells");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("cells"))
        return Utils::ResultError("Missing required field: cells");
    NotebookDocumentFilterWithCells result;
    if (obj.contains("notebook")) {
        const auto res0 = fromJson<NotebookCellTextDocumentFilterNotebook>("notebook", obj["notebook"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._notebook = *res0;
    }
    if (obj.contains("cells") && obj["cells"].isArray()) {
        const QJsonArray arr = obj["cells"].toArray();
        for (const QJsonValue &v : arr) {
            const auto res1 = fromJson<NotebookCellLanguage>("cells", v);
            if (!res1)
                return Utils::ResultError(res1.error());
            result._cells.append(*res1);
        }
    }
    return result;
}

QJsonObject toJson(const NotebookDocumentFilterWithCells &data)
{
    QJsonObject obj;
    if (data._notebook.has_value())
        obj.insert("notebook", toJsonValue(*data._notebook));
    QJsonArray arr_cells;
    for (const auto &v : data._cells) arr_cells.append(toJson(v));
    obj.insert("cells", arr_cells);
    return obj;
}

template<>
Utils::Result<NotebookDocumentFilterWithNotebook> fromJson<NotebookDocumentFilterWithNotebook>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for NotebookDocumentFilterWithNotebook");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("notebook"))
        return Utils::ResultError("Missing required field: notebook");
    NotebookDocumentFilterWithNotebook result;
    if (obj.contains("notebook")) {
        const auto res0 = fromJson<NotebookCellTextDocumentFilterNotebook>("notebook", obj["notebook"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._notebook = *res0;
    }
    if (obj.contains("cells") && obj["cells"].isArray()) {
        const QJsonArray arr = obj["cells"].toArray();
        QList<NotebookCellLanguage> list_cells;
        for (const QJsonValue &v : arr) {
            const auto res1 = fromJson<NotebookCellLanguage>("cells", v);
            if (!res1)
                return Utils::ResultError(res1.error());
            list_cells.append(*res1);
        }
        result._cells = list_cells;
    }
    return result;
}

QJsonObject toJson(const NotebookDocumentFilterWithNotebook &data)
{
    QJsonObject obj{{"notebook", toJsonValue(data._notebook)}};
    if (data._cells.has_value()) {
        QJsonArray arr_cells;
        for (const auto &v : *data._cells) arr_cells.append(toJson(v));
        obj.insert("cells", arr_cells);
    }
    return obj;
}

template<>
Utils::Result<NotebookDocumentSyncRegistrationOptionsNotebookSelectorItem> fromJson<NotebookDocumentSyncRegistrationOptionsNotebookSelectorItem>(const QJsonValue &val)
{
    {
        auto result = fromJson<NotebookDocumentFilterWithNotebook>(val);
        if (result) return NotebookDocumentSyncRegistrationOptionsNotebookSelectorItem(*result);
    }
    {
        auto result = fromJson<NotebookDocumentFilterWithCells>(val);
        if (result) return NotebookDocumentSyncRegistrationOptionsNotebookSelectorItem(*result);
    }
    return Utils::ResultError("Invalid NotebookDocumentSyncRegistrationOptionsNotebookSelectorItem");
}

QJsonValue toJsonValue(const NotebookDocumentSyncRegistrationOptionsNotebookSelectorItem &val)
{
    return std::visit([](const auto &v) -> QJsonValue {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, QJsonObject>) {
            return v;
        } else {
            return toJson(v);
        }
    }, val);
}

template<>
Utils::Result<NotebookDocumentSyncRegistrationOptions> fromJson<NotebookDocumentSyncRegistrationOptions>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for NotebookDocumentSyncRegistrationOptions");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("notebookSelector"))
        return Utils::ResultError("Missing required field: notebookSelector");
    NotebookDocumentSyncRegistrationOptions result;
    if (obj.contains("notebookSelector") && obj["notebookSelector"].isArray()) {
        const QJsonArray arr = obj["notebookSelector"].toArray();
        for (const QJsonValue &v : arr) {
            const auto res0 = fromJson<NotebookDocumentSyncRegistrationOptionsNotebookSelectorItem>("notebookSelector", v);
            if (!res0)
                return Utils::ResultError(res0.error());
            result._notebookSelector.append(*res0);
        }
    }
    if (obj.contains("save"))
        result._save = obj.value("save").toBool();
    if (obj.contains("id"))
        result._id = obj.value("id").toString();
    return result;
}

QJsonObject toJson(const NotebookDocumentSyncRegistrationOptions &data)
{
    QJsonObject obj;
    QJsonArray arr_notebookSelector;
    for (const auto &v : data._notebookSelector) arr_notebookSelector.append(toJsonValue(v));
    obj.insert("notebookSelector", arr_notebookSelector);
    if (data._save.has_value())
        obj.insert("save", *data._save);
    if (data._id.has_value())
        obj.insert("id", *data._id);
    return obj;
}

template<>
Utils::Result<NotebookCellArrayChange> fromJson<NotebookCellArrayChange>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for NotebookCellArrayChange");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("start"))
        return Utils::ResultError("Missing required field: start");
    if (!obj.contains("deleteCount"))
        return Utils::ResultError("Missing required field: deleteCount");
    NotebookCellArrayChange result;
    result._start = obj.value("start").toInt();
    result._deleteCount = obj.value("deleteCount").toInt();
    if (obj.contains("cells") && obj["cells"].isArray()) {
        const QJsonArray arr = obj["cells"].toArray();
        QList<NotebookCell> list_cells;
        for (const QJsonValue &v : arr) {
            const auto res0 = fromJson<NotebookCell>("cells", v);
            if (!res0)
                return Utils::ResultError(res0.error());
            list_cells.append(*res0);
        }
        result._cells = list_cells;
    }
    return result;
}

QJsonObject toJson(const NotebookCellArrayChange &data)
{
    QJsonObject obj{
        {"start", data._start},
        {"deleteCount", data._deleteCount}
    };
    if (data._cells.has_value()) {
        QJsonArray arr_cells;
        for (const auto &v : *data._cells) arr_cells.append(toJson(v));
        obj.insert("cells", arr_cells);
    }
    return obj;
}

template<>
Utils::Result<NotebookDocumentCellChangeStructure> fromJson<NotebookDocumentCellChangeStructure>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for NotebookDocumentCellChangeStructure");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("array"))
        return Utils::ResultError("Missing required field: array");
    NotebookDocumentCellChangeStructure result;
    if (obj.contains("array") && obj["array"].isObject()) {
        const auto res0 = fromJson<NotebookCellArrayChange>("array", obj["array"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._array = *res0;
    }
    if (obj.contains("didOpen") && obj["didOpen"].isArray()) {
        const QJsonArray arr = obj["didOpen"].toArray();
        QList<TextDocumentItem> list_didOpen;
        for (const QJsonValue &v : arr) {
            const auto res1 = fromJson<TextDocumentItem>("didOpen", v);
            if (!res1)
                return Utils::ResultError(res1.error());
            list_didOpen.append(*res1);
        }
        result._didOpen = list_didOpen;
    }
    if (obj.contains("didClose") && obj["didClose"].isArray()) {
        const QJsonArray arr = obj["didClose"].toArray();
        QList<TextDocumentIdentifier> list_didClose;
        for (const QJsonValue &v : arr) {
            const auto res2 = fromJson<TextDocumentIdentifier>("didClose", v);
            if (!res2)
                return Utils::ResultError(res2.error());
            list_didClose.append(*res2);
        }
        result._didClose = list_didClose;
    }
    return result;
}

QJsonObject toJson(const NotebookDocumentCellChangeStructure &data)
{
    QJsonObject obj{{"array", toJson(data._array)}};
    if (data._didOpen.has_value()) {
        QJsonArray arr_didOpen;
        for (const auto &v : *data._didOpen) arr_didOpen.append(toJson(v));
        obj.insert("didOpen", arr_didOpen);
    }
    if (data._didClose.has_value()) {
        QJsonArray arr_didClose;
        for (const auto &v : *data._didClose) arr_didClose.append(toJson(v));
        obj.insert("didClose", arr_didClose);
    }
    return obj;
}

template<>
Utils::Result<TextDocumentContentChangePartial> fromJson<TextDocumentContentChangePartial>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for TextDocumentContentChangePartial");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("range"))
        return Utils::ResultError("Missing required field: range");
    if (!obj.contains("text"))
        return Utils::ResultError("Missing required field: text");
    TextDocumentContentChangePartial result;
    if (obj.contains("range") && obj["range"].isObject()) {
        const auto res0 = fromJson<Range>("range", obj["range"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._range = *res0;
    }
    if (obj.contains("rangeLength"))
        result._rangeLength = obj.value("rangeLength").toInt();
    result._text = obj.value("text").toString();
    return result;
}

QJsonObject toJson(const TextDocumentContentChangePartial &data)
{
    QJsonObject obj{
        {"range", toJson(data._range)},
        {"text", data._text}
    };
    if (data._rangeLength.has_value())
        obj.insert("rangeLength", *data._rangeLength);
    return obj;
}

template<>
Utils::Result<TextDocumentContentChangeWholeDocument> fromJson<TextDocumentContentChangeWholeDocument>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for TextDocumentContentChangeWholeDocument");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("text"))
        return Utils::ResultError("Missing required field: text");
    TextDocumentContentChangeWholeDocument result;
    result._text = obj.value("text").toString();
    return result;
}

QJsonObject toJson(const TextDocumentContentChangeWholeDocument &data)
{
    QJsonObject obj{{"text", data._text}};
    return obj;
}

template<>
Utils::Result<TextDocumentContentChangeEvent> fromJson<TextDocumentContentChangeEvent>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Invalid TextDocumentContentChangeEvent: expected object");
    const QJsonObject obj = val.toObject();
    if (obj.contains("range")) {
        const auto res0 = fromJson<TextDocumentContentChangePartial>(val);
        if (!res0)
            return Utils::ResultError(res0.error());
        return TextDocumentContentChangeEvent(*res0);
    }
    {
        auto result = fromJson<TextDocumentContentChangeWholeDocument>(val);
        if (result) return TextDocumentContentChangeEvent(*result);
    }
    return Utils::ResultError("Invalid TextDocumentContentChangeEvent");
}

QString text(const TextDocumentContentChangeEvent &val)
{
    return std::visit([](const auto &v) -> QString { return v._text; }, val);
}

QJsonObject toJson(const TextDocumentContentChangeEvent &val)
{
    return std::visit([](const auto &v) -> QJsonObject {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, QJsonObject>) {
            return v;
        } else {
            return toJson(v);
        }
    }, val);
}

QJsonValue toJsonValue(const TextDocumentContentChangeEvent &val)
{
    return toJson(val);
}

template<>
Utils::Result<VersionedTextDocumentIdentifier> fromJson<VersionedTextDocumentIdentifier>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for VersionedTextDocumentIdentifier");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("uri"))
        return Utils::ResultError("Missing required field: uri");
    if (!obj.contains("version"))
        return Utils::ResultError("Missing required field: version");
    VersionedTextDocumentIdentifier result;
    result._uri = obj.value("uri").toString();
    result._version = obj.value("version").toInt();
    return result;
}

QJsonObject toJson(const VersionedTextDocumentIdentifier &data)
{
    QJsonObject obj{
        {"uri", data._uri},
        {"version", data._version}
    };
    return obj;
}

template<>
Utils::Result<NotebookDocumentCellContentChanges> fromJson<NotebookDocumentCellContentChanges>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for NotebookDocumentCellContentChanges");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("document"))
        return Utils::ResultError("Missing required field: document");
    if (!obj.contains("changes"))
        return Utils::ResultError("Missing required field: changes");
    NotebookDocumentCellContentChanges result;
    if (obj.contains("document") && obj["document"].isObject()) {
        const auto res0 = fromJson<VersionedTextDocumentIdentifier>("document", obj["document"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._document = *res0;
    }
    if (obj.contains("changes") && obj["changes"].isArray()) {
        const QJsonArray arr = obj["changes"].toArray();
        for (const QJsonValue &v : arr) {
            const auto res1 = fromJson<TextDocumentContentChangeEvent>("changes", v);
            if (!res1)
                return Utils::ResultError(res1.error());
            result._changes.append(*res1);
        }
    }
    return result;
}

QJsonObject toJson(const NotebookDocumentCellContentChanges &data)
{
    QJsonObject obj{{"document", toJson(data._document)}};
    QJsonArray arr_changes;
    for (const auto &v : data._changes) arr_changes.append(toJsonValue(v));
    obj.insert("changes", arr_changes);
    return obj;
}

template<>
Utils::Result<NotebookDocumentCellChanges> fromJson<NotebookDocumentCellChanges>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for NotebookDocumentCellChanges");
    const QJsonObject obj = val.toObject();
    NotebookDocumentCellChanges result;
    if (obj.contains("structure") && obj["structure"].isObject()) {
        const auto res0 = fromJson<NotebookDocumentCellChangeStructure>("structure", obj["structure"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._structure = *res0;
    }
    if (obj.contains("data") && obj["data"].isArray()) {
        const QJsonArray arr = obj["data"].toArray();
        QList<NotebookCell> list_data;
        for (const QJsonValue &v : arr) {
            const auto res1 = fromJson<NotebookCell>("data", v);
            if (!res1)
                return Utils::ResultError(res1.error());
            list_data.append(*res1);
        }
        result._data = list_data;
    }
    if (obj.contains("textContent") && obj["textContent"].isArray()) {
        const QJsonArray arr = obj["textContent"].toArray();
        QList<NotebookDocumentCellContentChanges> list_textContent;
        for (const QJsonValue &v : arr) {
            const auto res2 = fromJson<NotebookDocumentCellContentChanges>("textContent", v);
            if (!res2)
                return Utils::ResultError(res2.error());
            list_textContent.append(*res2);
        }
        result._textContent = list_textContent;
    }
    return result;
}

QJsonObject toJson(const NotebookDocumentCellChanges &data)
{
    QJsonObject obj;
    if (data._structure.has_value())
        obj.insert("structure", toJson(*data._structure));
    if (data._data.has_value()) {
        QJsonArray arr_data;
        for (const auto &v : *data._data) arr_data.append(toJson(v));
        obj.insert("data", arr_data);
    }
    if (data._textContent.has_value()) {
        QJsonArray arr_textContent;
        for (const auto &v : *data._textContent) arr_textContent.append(toJson(v));
        obj.insert("textContent", arr_textContent);
    }
    return obj;
}

template<>
Utils::Result<NotebookDocumentChangeEvent> fromJson<NotebookDocumentChangeEvent>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for NotebookDocumentChangeEvent");
    const QJsonObject obj = val.toObject();
    NotebookDocumentChangeEvent result;
    if (obj.contains("metadata"))
        result._metadata = obj.value("metadata").toObject();
    if (obj.contains("cells") && obj["cells"].isObject()) {
        const auto res0 = fromJson<NotebookDocumentCellChanges>("cells", obj["cells"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._cells = *res0;
    }
    return result;
}

QJsonObject toJson(const NotebookDocumentChangeEvent &data)
{
    QJsonObject obj;
    if (data._metadata.has_value())
        obj.insert("metadata", *data._metadata);
    if (data._cells.has_value())
        obj.insert("cells", toJson(*data._cells));
    return obj;
}

template<>
Utils::Result<VersionedNotebookDocumentIdentifier> fromJson<VersionedNotebookDocumentIdentifier>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for VersionedNotebookDocumentIdentifier");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("version"))
        return Utils::ResultError("Missing required field: version");
    if (!obj.contains("uri"))
        return Utils::ResultError("Missing required field: uri");
    VersionedNotebookDocumentIdentifier result;
    result._version = obj.value("version").toInt();
    result._uri = obj.value("uri").toString();
    return result;
}

QJsonObject toJson(const VersionedNotebookDocumentIdentifier &data)
{
    QJsonObject obj{
        {"version", data._version},
        {"uri", data._uri}
    };
    return obj;
}

template<>
Utils::Result<DidChangeNotebookDocumentParams> fromJson<DidChangeNotebookDocumentParams>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for DidChangeNotebookDocumentParams");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("notebookDocument"))
        return Utils::ResultError("Missing required field: notebookDocument");
    if (!obj.contains("change"))
        return Utils::ResultError("Missing required field: change");
    DidChangeNotebookDocumentParams result;
    if (obj.contains("notebookDocument") && obj["notebookDocument"].isObject()) {
        const auto res0 = fromJson<VersionedNotebookDocumentIdentifier>("notebookDocument", obj["notebookDocument"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._notebookDocument = *res0;
    }
    if (obj.contains("change") && obj["change"].isObject()) {
        const auto res1 = fromJson<NotebookDocumentChangeEvent>("change", obj["change"]);
        if (!res1)
            return Utils::ResultError(res1.error());
        result._change = *res1;
    }
    return result;
}

QJsonObject toJson(const DidChangeNotebookDocumentParams &data)
{
    QJsonObject obj{
        {"notebookDocument", toJson(data._notebookDocument)},
        {"change", toJson(data._change)}
    };
    return obj;
}

template<>
Utils::Result<NotebookDocumentIdentifier> fromJson<NotebookDocumentIdentifier>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for NotebookDocumentIdentifier");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("uri"))
        return Utils::ResultError("Missing required field: uri");
    NotebookDocumentIdentifier result;
    result._uri = obj.value("uri").toString();
    return result;
}

QJsonObject toJson(const NotebookDocumentIdentifier &data)
{
    QJsonObject obj{{"uri", data._uri}};
    return obj;
}

template<>
Utils::Result<DidSaveNotebookDocumentParams> fromJson<DidSaveNotebookDocumentParams>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for DidSaveNotebookDocumentParams");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("notebookDocument"))
        return Utils::ResultError("Missing required field: notebookDocument");
    DidSaveNotebookDocumentParams result;
    if (obj.contains("notebookDocument") && obj["notebookDocument"].isObject()) {
        const auto res0 = fromJson<NotebookDocumentIdentifier>("notebookDocument", obj["notebookDocument"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._notebookDocument = *res0;
    }
    return result;
}

QJsonObject toJson(const DidSaveNotebookDocumentParams &data)
{
    QJsonObject obj{{"notebookDocument", toJson(data._notebookDocument)}};
    return obj;
}

template<>
Utils::Result<DidCloseNotebookDocumentParams> fromJson<DidCloseNotebookDocumentParams>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for DidCloseNotebookDocumentParams");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("notebookDocument"))
        return Utils::ResultError("Missing required field: notebookDocument");
    if (!obj.contains("cellTextDocuments"))
        return Utils::ResultError("Missing required field: cellTextDocuments");
    DidCloseNotebookDocumentParams result;
    if (obj.contains("notebookDocument") && obj["notebookDocument"].isObject()) {
        const auto res0 = fromJson<NotebookDocumentIdentifier>("notebookDocument", obj["notebookDocument"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._notebookDocument = *res0;
    }
    if (obj.contains("cellTextDocuments") && obj["cellTextDocuments"].isArray()) {
        const QJsonArray arr = obj["cellTextDocuments"].toArray();
        for (const QJsonValue &v : arr) {
            const auto res1 = fromJson<TextDocumentIdentifier>("cellTextDocuments", v);
            if (!res1)
                return Utils::ResultError(res1.error());
            result._cellTextDocuments.append(*res1);
        }
    }
    return result;
}

QJsonObject toJson(const DidCloseNotebookDocumentParams &data)
{
    QJsonObject obj{{"notebookDocument", toJson(data._notebookDocument)}};
    QJsonArray arr_cellTextDocuments;
    for (const auto &v : data._cellTextDocuments) arr_cellTextDocuments.append(toJson(v));
    obj.insert("cellTextDocuments", arr_cellTextDocuments);
    return obj;
}

template<>
Utils::Result<SelectedCompletionInfo> fromJson<SelectedCompletionInfo>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for SelectedCompletionInfo");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("range"))
        return Utils::ResultError("Missing required field: range");
    if (!obj.contains("text"))
        return Utils::ResultError("Missing required field: text");
    SelectedCompletionInfo result;
    if (obj.contains("range") && obj["range"].isObject()) {
        const auto res0 = fromJson<Range>("range", obj["range"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._range = *res0;
    }
    result._text = obj.value("text").toString();
    return result;
}

QJsonObject toJson(const SelectedCompletionInfo &data)
{
    QJsonObject obj{
        {"range", toJson(data._range)},
        {"text", data._text}
    };
    return obj;
}

template<>
Utils::Result<InlineCompletionContext> fromJson<InlineCompletionContext>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for InlineCompletionContext");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("triggerKind"))
        return Utils::ResultError("Missing required field: triggerKind");
    InlineCompletionContext result;
    result._triggerKind = obj.value("triggerKind").toInt();
    if (obj.contains("selectedCompletionInfo") && obj["selectedCompletionInfo"].isObject()) {
        const auto res0 = fromJson<SelectedCompletionInfo>("selectedCompletionInfo", obj["selectedCompletionInfo"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._selectedCompletionInfo = *res0;
    }
    return result;
}

QJsonObject toJson(const InlineCompletionContext &data)
{
    QJsonObject obj{{"triggerKind", data._triggerKind}};
    if (data._selectedCompletionInfo.has_value())
        obj.insert("selectedCompletionInfo", toJson(*data._selectedCompletionInfo));
    return obj;
}

template<>
Utils::Result<InlineCompletionParams> fromJson<InlineCompletionParams>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for InlineCompletionParams");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("textDocument"))
        return Utils::ResultError("Missing required field: textDocument");
    if (!obj.contains("position"))
        return Utils::ResultError("Missing required field: position");
    if (!obj.contains("context"))
        return Utils::ResultError("Missing required field: context");
    InlineCompletionParams result;
    if (obj.contains("textDocument") && obj["textDocument"].isObject()) {
        const auto res0 = fromJson<TextDocumentIdentifier>("textDocument", obj["textDocument"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._textDocument = *res0;
    }
    if (obj.contains("position") && obj["position"].isObject()) {
        const auto res1 = fromJson<Position>("position", obj["position"]);
        if (!res1)
            return Utils::ResultError(res1.error());
        result._position = *res1;
    }
    if (obj.contains("workDoneToken")) {
        const auto res2 = fromJson<ProgressToken>("workDoneToken", obj["workDoneToken"]);
        if (!res2)
            return Utils::ResultError(res2.error());
        result._workDoneToken = *res2;
    }
    if (obj.contains("context") && obj["context"].isObject()) {
        const auto res3 = fromJson<InlineCompletionContext>("context", obj["context"]);
        if (!res3)
            return Utils::ResultError(res3.error());
        result._context = *res3;
    }
    return result;
}

QJsonObject toJson(const InlineCompletionParams &data)
{
    QJsonObject obj{
        {"textDocument", toJson(data._textDocument)},
        {"position", toJson(data._position)},
        {"context", toJson(data._context)}
    };
    if (data._workDoneToken.has_value())
        obj.insert("workDoneToken", toJsonValue(*data._workDoneToken));
    return obj;
}

template<>
Utils::Result<InlineCompletionItemInsertText> fromJson<InlineCompletionItemInsertText>(const QJsonValue &val)
{
    if (val.isString())
        return InlineCompletionItemInsertText(val.toString());
    if (!val.isObject())
        return Utils::ResultError("Invalid InlineCompletionItemInsertText: expected object or array");
    const QString dispatchValue = val.toObject().value("kind").toString();
    if (dispatchValue == "snippet") {
        const auto res0 = fromJson<StringValue>(val);
        if (!res0)
            return Utils::ResultError(res0.error());
        return InlineCompletionItemInsertText(*res0);
    }
    return Utils::ResultError("Invalid InlineCompletionItemInsertText: unknown kind \"" + dispatchValue + "\"");
}

QJsonValue toJsonValue(const InlineCompletionItemInsertText &val)
{
    return std::visit([](const auto &v) -> QJsonValue {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, StringValue>) {
            return toJson(v);
        } else
        {
            return QVariant::fromValue(v).toJsonValue();
        }
    }, val);
}

template<>
Utils::Result<InlineCompletionItem> fromJson<InlineCompletionItem>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for InlineCompletionItem");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("insertText"))
        return Utils::ResultError("Missing required field: insertText");
    InlineCompletionItem result;
    if (obj.contains("insertText")) {
        const auto res0 = fromJson<InlineCompletionItemInsertText>("insertText", obj["insertText"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._insertText = *res0;
    }
    if (obj.contains("filterText"))
        result._filterText = obj.value("filterText").toString();
    if (obj.contains("range") && obj["range"].isObject()) {
        const auto res1 = fromJson<Range>("range", obj["range"]);
        if (!res1)
            return Utils::ResultError(res1.error());
        result._range = *res1;
    }
    if (obj.contains("command") && obj["command"].isObject()) {
        const auto res2 = fromJson<Command>("command", obj["command"]);
        if (!res2)
            return Utils::ResultError(res2.error());
        result._command = *res2;
    }
    return result;
}

QJsonObject toJson(const InlineCompletionItem &data)
{
    QJsonObject obj{{"insertText", toJsonValue(data._insertText)}};
    if (data._filterText.has_value())
        obj.insert("filterText", *data._filterText);
    if (data._range.has_value())
        obj.insert("range", toJson(*data._range));
    if (data._command.has_value())
        obj.insert("command", toJson(*data._command));
    return obj;
}

template<>
Utils::Result<InlineCompletionList> fromJson<InlineCompletionList>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for InlineCompletionList");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("items"))
        return Utils::ResultError("Missing required field: items");
    InlineCompletionList result;
    if (obj.contains("items") && obj["items"].isArray()) {
        const QJsonArray arr = obj["items"].toArray();
        for (const QJsonValue &v : arr) {
            const auto res0 = fromJson<InlineCompletionItem>("items", v);
            if (!res0)
                return Utils::ResultError(res0.error());
            result._items.append(*res0);
        }
    }
    return result;
}

QJsonObject toJson(const InlineCompletionList &data)
{
    QJsonObject obj;
    QJsonArray arr_items;
    for (const auto &v : data._items) arr_items.append(toJson(v));
    obj.insert("items", arr_items);
    return obj;
}

template<>
Utils::Result<InlineCompletionRegistrationOptions> fromJson<InlineCompletionRegistrationOptions>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for InlineCompletionRegistrationOptions");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("documentSelector"))
        return Utils::ResultError("Missing required field: documentSelector");
    InlineCompletionRegistrationOptions result;
    if (obj.contains("workDoneProgress"))
        result._workDoneProgress = obj.value("workDoneProgress").toBool();
    if (obj.contains("documentSelector") && !obj["documentSelector"].isNull()) {
        const auto res0 = fromJson<DocumentSelector>("documentSelector", obj["documentSelector"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._documentSelector = *res0;
    }
    if (obj.contains("id"))
        result._id = obj.value("id").toString();
    return result;
}

QJsonObject toJson(const InlineCompletionRegistrationOptions &data)
{
    QJsonObject obj;
    if (data._workDoneProgress.has_value())
        obj.insert("workDoneProgress", *data._workDoneProgress);
    if (data._documentSelector.has_value())
        obj.insert("documentSelector", toJson(*data._documentSelector));
    else
        obj.insert("documentSelector", QJsonValue::Null);
    if (data._id.has_value())
        obj.insert("id", *data._id);
    return obj;
}

template<>
Utils::Result<TextDocumentContentParams> fromJson<TextDocumentContentParams>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for TextDocumentContentParams");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("uri"))
        return Utils::ResultError("Missing required field: uri");
    TextDocumentContentParams result;
    result._uri = obj.value("uri").toString();
    return result;
}

QJsonObject toJson(const TextDocumentContentParams &data)
{
    QJsonObject obj{{"uri", data._uri}};
    return obj;
}

template<>
Utils::Result<TextDocumentContentResult> fromJson<TextDocumentContentResult>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for TextDocumentContentResult");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("text"))
        return Utils::ResultError("Missing required field: text");
    TextDocumentContentResult result;
    result._text = obj.value("text").toString();
    return result;
}

QJsonObject toJson(const TextDocumentContentResult &data)
{
    QJsonObject obj{{"text", data._text}};
    return obj;
}

template<>
Utils::Result<TextDocumentContentRegistrationOptions> fromJson<TextDocumentContentRegistrationOptions>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for TextDocumentContentRegistrationOptions");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("schemes"))
        return Utils::ResultError("Missing required field: schemes");
    TextDocumentContentRegistrationOptions result;
    if (obj.contains("schemes") && obj["schemes"].isArray()) {
        const QJsonArray arr = obj["schemes"].toArray();
        for (const QJsonValue &v : arr) {
            result._schemes.append(v.toString());
        }
    }
    if (obj.contains("id"))
        result._id = obj.value("id").toString();
    return result;
}

QJsonObject toJson(const TextDocumentContentRegistrationOptions &data)
{
    QJsonObject obj;
    QJsonArray arr_schemes;
    for (const auto &v : data._schemes) arr_schemes.append(v);
    obj.insert("schemes", arr_schemes);
    if (data._id.has_value())
        obj.insert("id", *data._id);
    return obj;
}

template<>
Utils::Result<TextDocumentContentRefreshParams> fromJson<TextDocumentContentRefreshParams>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for TextDocumentContentRefreshParams");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("uri"))
        return Utils::ResultError("Missing required field: uri");
    TextDocumentContentRefreshParams result;
    result._uri = obj.value("uri").toString();
    return result;
}

QJsonObject toJson(const TextDocumentContentRefreshParams &data)
{
    QJsonObject obj{{"uri", data._uri}};
    return obj;
}

template<>
Utils::Result<Registration> fromJson<Registration>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for Registration");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("id"))
        return Utils::ResultError("Missing required field: id");
    if (!obj.contains("method"))
        return Utils::ResultError("Missing required field: method");
    Registration result;
    result._id = obj.value("id").toString();
    result._method = obj.value("method").toString();
    if (obj.contains("registerOptions"))
        result._registerOptions = obj.value("registerOptions");
    return result;
}

QJsonObject toJson(const Registration &data)
{
    QJsonObject obj{
        {"id", data._id},
        {"method", data._method}
    };
    if (data._registerOptions.has_value())
        obj.insert("registerOptions", *data._registerOptions);
    return obj;
}

template<>
Utils::Result<RegistrationParams> fromJson<RegistrationParams>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for RegistrationParams");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("registrations"))
        return Utils::ResultError("Missing required field: registrations");
    RegistrationParams result;
    if (obj.contains("registrations") && obj["registrations"].isArray()) {
        const QJsonArray arr = obj["registrations"].toArray();
        for (const QJsonValue &v : arr) {
            const auto res0 = fromJson<Registration>("registrations", v);
            if (!res0)
                return Utils::ResultError(res0.error());
            result._registrations.append(*res0);
        }
    }
    return result;
}

QJsonObject toJson(const RegistrationParams &data)
{
    QJsonObject obj;
    QJsonArray arr_registrations;
    for (const auto &v : data._registrations) arr_registrations.append(toJson(v));
    obj.insert("registrations", arr_registrations);
    return obj;
}

template<>
Utils::Result<Unregistration> fromJson<Unregistration>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for Unregistration");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("id"))
        return Utils::ResultError("Missing required field: id");
    if (!obj.contains("method"))
        return Utils::ResultError("Missing required field: method");
    Unregistration result;
    result._id = obj.value("id").toString();
    result._method = obj.value("method").toString();
    return result;
}

QJsonObject toJson(const Unregistration &data)
{
    QJsonObject obj{
        {"id", data._id},
        {"method", data._method}
    };
    return obj;
}

template<>
Utils::Result<UnregistrationParams> fromJson<UnregistrationParams>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for UnregistrationParams");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("unregisterations"))
        return Utils::ResultError("Missing required field: unregisterations");
    UnregistrationParams result;
    if (obj.contains("unregisterations") && obj["unregisterations"].isArray()) {
        const QJsonArray arr = obj["unregisterations"].toArray();
        for (const QJsonValue &v : arr) {
            const auto res0 = fromJson<Unregistration>("unregisterations", v);
            if (!res0)
                return Utils::ResultError(res0.error());
            result._unregisterations.append(*res0);
        }
    }
    return result;
}

QJsonObject toJson(const UnregistrationParams &data)
{
    QJsonObject obj;
    QJsonArray arr_unregisterations;
    for (const auto &v : data._unregisterations) arr_unregisterations.append(toJson(v));
    obj.insert("unregisterations", arr_unregisterations);
    return obj;
}

template<>
Utils::Result<MarkdownClientCapabilities> fromJson<MarkdownClientCapabilities>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for MarkdownClientCapabilities");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("parser"))
        return Utils::ResultError("Missing required field: parser");
    MarkdownClientCapabilities result;
    result._parser = obj.value("parser").toString();
    if (obj.contains("version"))
        result._version = obj.value("version").toString();
    if (obj.contains("allowedTags") && obj["allowedTags"].isArray()) {
        const QJsonArray arr = obj["allowedTags"].toArray();
        QStringList list_allowedTags;
        for (const QJsonValue &v : arr) {
            list_allowedTags.append(v.toString());
        }
        result._allowedTags = list_allowedTags;
    }
    return result;
}

QJsonObject toJson(const MarkdownClientCapabilities &data)
{
    QJsonObject obj{{"parser", data._parser}};
    if (data._version.has_value())
        obj.insert("version", *data._version);
    if (data._allowedTags.has_value()) {
        QJsonArray arr_allowedTags;
        for (const auto &v : *data._allowedTags) arr_allowedTags.append(v);
        obj.insert("allowedTags", arr_allowedTags);
    }
    return obj;
}

template<>
Utils::Result<RegularExpressionsClientCapabilities> fromJson<RegularExpressionsClientCapabilities>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for RegularExpressionsClientCapabilities");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("engine"))
        return Utils::ResultError("Missing required field: engine");
    RegularExpressionsClientCapabilities result;
    if (obj.contains("engine") && obj["engine"].isString()) {
        const auto res0 = fromJson<RegularExpressionEngineKind>("engine", obj["engine"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._engine = *res0;
    }
    if (obj.contains("version"))
        result._version = obj.value("version").toString();
    return result;
}

QJsonObject toJson(const RegularExpressionsClientCapabilities &data)
{
    QJsonObject obj{{"engine", data._engine}};
    if (data._version.has_value())
        obj.insert("version", *data._version);
    return obj;
}

template<>
Utils::Result<StaleRequestSupportOptions> fromJson<StaleRequestSupportOptions>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for StaleRequestSupportOptions");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("cancel"))
        return Utils::ResultError("Missing required field: cancel");
    if (!obj.contains("retryOnContentModified"))
        return Utils::ResultError("Missing required field: retryOnContentModified");
    StaleRequestSupportOptions result;
    result._cancel = obj.value("cancel").toBool();
    if (obj.contains("retryOnContentModified") && obj["retryOnContentModified"].isArray()) {
        const QJsonArray arr = obj["retryOnContentModified"].toArray();
        for (const QJsonValue &v : arr) {
            result._retryOnContentModified.append(v.toString());
        }
    }
    return result;
}

QJsonObject toJson(const StaleRequestSupportOptions &data)
{
    QJsonObject obj{{"cancel", data._cancel}};
    QJsonArray arr_retryOnContentModified;
    for (const auto &v : data._retryOnContentModified) arr_retryOnContentModified.append(v);
    obj.insert("retryOnContentModified", arr_retryOnContentModified);
    return obj;
}

template<>
Utils::Result<GeneralClientCapabilities> fromJson<GeneralClientCapabilities>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for GeneralClientCapabilities");
    const QJsonObject obj = val.toObject();
    GeneralClientCapabilities result;
    if (obj.contains("staleRequestSupport") && obj["staleRequestSupport"].isObject()) {
        const auto res0 = fromJson<StaleRequestSupportOptions>("staleRequestSupport", obj["staleRequestSupport"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._staleRequestSupport = *res0;
    }
    if (obj.contains("regularExpressions") && obj["regularExpressions"].isObject()) {
        const auto res1 = fromJson<RegularExpressionsClientCapabilities>("regularExpressions", obj["regularExpressions"]);
        if (!res1)
            return Utils::ResultError(res1.error());
        result._regularExpressions = *res1;
    }
    if (obj.contains("markdown") && obj["markdown"].isObject()) {
        const auto res2 = fromJson<MarkdownClientCapabilities>("markdown", obj["markdown"]);
        if (!res2)
            return Utils::ResultError(res2.error());
        result._markdown = *res2;
    }
    if (obj.contains("positionEncodings") && obj["positionEncodings"].isArray()) {
        const QJsonArray arr = obj["positionEncodings"].toArray();
        QList<PositionEncodingKind> list_positionEncodings;
        for (const QJsonValue &v : arr) {
            const auto res3 = fromJson<PositionEncodingKind>("positionEncodings", v);
            if (!res3)
                return Utils::ResultError(res3.error());
            list_positionEncodings.append(*res3);
        }
        result._positionEncodings = list_positionEncodings;
    }
    return result;
}

QJsonObject toJson(const GeneralClientCapabilities &data)
{
    QJsonObject obj;
    if (data._staleRequestSupport.has_value())
        obj.insert("staleRequestSupport", toJson(*data._staleRequestSupport));
    if (data._regularExpressions.has_value())
        obj.insert("regularExpressions", toJson(*data._regularExpressions));
    if (data._markdown.has_value())
        obj.insert("markdown", toJson(*data._markdown));
    if (data._positionEncodings.has_value()) {
        QJsonArray arr_positionEncodings;
        for (const auto &v : *data._positionEncodings) arr_positionEncodings.append(v);
        obj.insert("positionEncodings", arr_positionEncodings);
    }
    return obj;
}

template<>
Utils::Result<NotebookDocumentSyncClientCapabilities> fromJson<NotebookDocumentSyncClientCapabilities>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for NotebookDocumentSyncClientCapabilities");
    const QJsonObject obj = val.toObject();
    NotebookDocumentSyncClientCapabilities result;
    if (obj.contains("dynamicRegistration"))
        result._dynamicRegistration = obj.value("dynamicRegistration").toBool();
    if (obj.contains("executionSummarySupport"))
        result._executionSummarySupport = obj.value("executionSummarySupport").toBool();
    return result;
}

QJsonObject toJson(const NotebookDocumentSyncClientCapabilities &data)
{
    QJsonObject obj;
    if (data._dynamicRegistration.has_value())
        obj.insert("dynamicRegistration", *data._dynamicRegistration);
    if (data._executionSummarySupport.has_value())
        obj.insert("executionSummarySupport", *data._executionSummarySupport);
    return obj;
}

template<>
Utils::Result<NotebookDocumentClientCapabilities> fromJson<NotebookDocumentClientCapabilities>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for NotebookDocumentClientCapabilities");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("synchronization"))
        return Utils::ResultError("Missing required field: synchronization");
    NotebookDocumentClientCapabilities result;
    if (obj.contains("synchronization") && obj["synchronization"].isObject()) {
        const auto res0 = fromJson<NotebookDocumentSyncClientCapabilities>("synchronization", obj["synchronization"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._synchronization = *res0;
    }
    return result;
}

QJsonObject toJson(const NotebookDocumentClientCapabilities &data)
{
    QJsonObject obj{{"synchronization", toJson(data._synchronization)}};
    return obj;
}

template<>
Utils::Result<CallHierarchyClientCapabilities> fromJson<CallHierarchyClientCapabilities>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for CallHierarchyClientCapabilities");
    const QJsonObject obj = val.toObject();
    CallHierarchyClientCapabilities result;
    if (obj.contains("dynamicRegistration"))
        result._dynamicRegistration = obj.value("dynamicRegistration").toBool();
    return result;
}

QJsonObject toJson(const CallHierarchyClientCapabilities &data)
{
    QJsonObject obj;
    if (data._dynamicRegistration.has_value())
        obj.insert("dynamicRegistration", *data._dynamicRegistration);
    return obj;
}

template<>
Utils::Result<ClientCodeActionKindOptions> fromJson<ClientCodeActionKindOptions>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for ClientCodeActionKindOptions");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("valueSet"))
        return Utils::ResultError("Missing required field: valueSet");
    ClientCodeActionKindOptions result;
    if (obj.contains("valueSet") && obj["valueSet"].isArray()) {
        const QJsonArray arr = obj["valueSet"].toArray();
        for (const QJsonValue &v : arr) {
            const auto res0 = fromJson<CodeActionKind>("valueSet", v);
            if (!res0)
                return Utils::ResultError(res0.error());
            result._valueSet.append(*res0);
        }
    }
    return result;
}

QJsonObject toJson(const ClientCodeActionKindOptions &data)
{
    QJsonObject obj;
    QJsonArray arr_valueSet;
    for (const auto &v : data._valueSet) arr_valueSet.append(v);
    obj.insert("valueSet", arr_valueSet);
    return obj;
}

template<>
Utils::Result<ClientCodeActionLiteralOptions> fromJson<ClientCodeActionLiteralOptions>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for ClientCodeActionLiteralOptions");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("codeActionKind"))
        return Utils::ResultError("Missing required field: codeActionKind");
    ClientCodeActionLiteralOptions result;
    if (obj.contains("codeActionKind") && obj["codeActionKind"].isObject()) {
        const auto res0 = fromJson<ClientCodeActionKindOptions>("codeActionKind", obj["codeActionKind"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._codeActionKind = *res0;
    }
    return result;
}

QJsonObject toJson(const ClientCodeActionLiteralOptions &data)
{
    QJsonObject obj{{"codeActionKind", toJson(data._codeActionKind)}};
    return obj;
}

template<>
Utils::Result<ClientCodeActionResolveOptions> fromJson<ClientCodeActionResolveOptions>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for ClientCodeActionResolveOptions");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("properties"))
        return Utils::ResultError("Missing required field: properties");
    ClientCodeActionResolveOptions result;
    if (obj.contains("properties") && obj["properties"].isArray()) {
        const QJsonArray arr = obj["properties"].toArray();
        for (const QJsonValue &v : arr) {
            result._properties.append(v.toString());
        }
    }
    return result;
}

QJsonObject toJson(const ClientCodeActionResolveOptions &data)
{
    QJsonObject obj;
    QJsonArray arr_properties;
    for (const auto &v : data._properties) arr_properties.append(v);
    obj.insert("properties", arr_properties);
    return obj;
}

template<>
Utils::Result<CodeActionTagOptions> fromJson<CodeActionTagOptions>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for CodeActionTagOptions");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("valueSet"))
        return Utils::ResultError("Missing required field: valueSet");
    CodeActionTagOptions result;
    if (obj.contains("valueSet") && obj["valueSet"].isArray()) {
        const QJsonArray arr = obj["valueSet"].toArray();
        for (const QJsonValue &v : arr) {
            result._valueSet.append(v.toInt());
        }
    }
    return result;
}

QJsonObject toJson(const CodeActionTagOptions &data)
{
    QJsonObject obj;
    QJsonArray arr_valueSet;
    for (const auto &v : data._valueSet) arr_valueSet.append(v);
    obj.insert("valueSet", arr_valueSet);
    return obj;
}

template<>
Utils::Result<CodeActionClientCapabilities> fromJson<CodeActionClientCapabilities>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for CodeActionClientCapabilities");
    const QJsonObject obj = val.toObject();
    CodeActionClientCapabilities result;
    if (obj.contains("dynamicRegistration"))
        result._dynamicRegistration = obj.value("dynamicRegistration").toBool();
    if (obj.contains("codeActionLiteralSupport") && obj["codeActionLiteralSupport"].isObject()) {
        const auto res0 = fromJson<ClientCodeActionLiteralOptions>("codeActionLiteralSupport", obj["codeActionLiteralSupport"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._codeActionLiteralSupport = *res0;
    }
    if (obj.contains("isPreferredSupport"))
        result._isPreferredSupport = obj.value("isPreferredSupport").toBool();
    if (obj.contains("disabledSupport"))
        result._disabledSupport = obj.value("disabledSupport").toBool();
    if (obj.contains("dataSupport"))
        result._dataSupport = obj.value("dataSupport").toBool();
    if (obj.contains("resolveSupport") && obj["resolveSupport"].isObject()) {
        const auto res1 = fromJson<ClientCodeActionResolveOptions>("resolveSupport", obj["resolveSupport"]);
        if (!res1)
            return Utils::ResultError(res1.error());
        result._resolveSupport = *res1;
    }
    if (obj.contains("honorsChangeAnnotations"))
        result._honorsChangeAnnotations = obj.value("honorsChangeAnnotations").toBool();
    if (obj.contains("documentationSupport"))
        result._documentationSupport = obj.value("documentationSupport").toBool();
    if (obj.contains("tagSupport") && obj["tagSupport"].isObject()) {
        const auto res2 = fromJson<CodeActionTagOptions>("tagSupport", obj["tagSupport"]);
        if (!res2)
            return Utils::ResultError(res2.error());
        result._tagSupport = *res2;
    }
    return result;
}

QJsonObject toJson(const CodeActionClientCapabilities &data)
{
    QJsonObject obj;
    if (data._dynamicRegistration.has_value())
        obj.insert("dynamicRegistration", *data._dynamicRegistration);
    if (data._codeActionLiteralSupport.has_value())
        obj.insert("codeActionLiteralSupport", toJson(*data._codeActionLiteralSupport));
    if (data._isPreferredSupport.has_value())
        obj.insert("isPreferredSupport", *data._isPreferredSupport);
    if (data._disabledSupport.has_value())
        obj.insert("disabledSupport", *data._disabledSupport);
    if (data._dataSupport.has_value())
        obj.insert("dataSupport", *data._dataSupport);
    if (data._resolveSupport.has_value())
        obj.insert("resolveSupport", toJson(*data._resolveSupport));
    if (data._honorsChangeAnnotations.has_value())
        obj.insert("honorsChangeAnnotations", *data._honorsChangeAnnotations);
    if (data._documentationSupport.has_value())
        obj.insert("documentationSupport", *data._documentationSupport);
    if (data._tagSupport.has_value())
        obj.insert("tagSupport", toJson(*data._tagSupport));
    return obj;
}

template<>
Utils::Result<ClientCodeLensResolveOptions> fromJson<ClientCodeLensResolveOptions>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for ClientCodeLensResolveOptions");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("properties"))
        return Utils::ResultError("Missing required field: properties");
    ClientCodeLensResolveOptions result;
    if (obj.contains("properties") && obj["properties"].isArray()) {
        const QJsonArray arr = obj["properties"].toArray();
        for (const QJsonValue &v : arr) {
            result._properties.append(v.toString());
        }
    }
    return result;
}

QJsonObject toJson(const ClientCodeLensResolveOptions &data)
{
    QJsonObject obj;
    QJsonArray arr_properties;
    for (const auto &v : data._properties) arr_properties.append(v);
    obj.insert("properties", arr_properties);
    return obj;
}

template<>
Utils::Result<CodeLensClientCapabilities> fromJson<CodeLensClientCapabilities>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for CodeLensClientCapabilities");
    const QJsonObject obj = val.toObject();
    CodeLensClientCapabilities result;
    if (obj.contains("dynamicRegistration"))
        result._dynamicRegistration = obj.value("dynamicRegistration").toBool();
    if (obj.contains("resolveSupport") && obj["resolveSupport"].isObject()) {
        const auto res0 = fromJson<ClientCodeLensResolveOptions>("resolveSupport", obj["resolveSupport"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._resolveSupport = *res0;
    }
    return result;
}

QJsonObject toJson(const CodeLensClientCapabilities &data)
{
    QJsonObject obj;
    if (data._dynamicRegistration.has_value())
        obj.insert("dynamicRegistration", *data._dynamicRegistration);
    if (data._resolveSupport.has_value())
        obj.insert("resolveSupport", toJson(*data._resolveSupport));
    return obj;
}

template<>
Utils::Result<ClientCompletionItemInsertTextModeOptions> fromJson<ClientCompletionItemInsertTextModeOptions>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for ClientCompletionItemInsertTextModeOptions");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("valueSet"))
        return Utils::ResultError("Missing required field: valueSet");
    ClientCompletionItemInsertTextModeOptions result;
    if (obj.contains("valueSet") && obj["valueSet"].isArray()) {
        const QJsonArray arr = obj["valueSet"].toArray();
        for (const QJsonValue &v : arr) {
            result._valueSet.append(v.toInt());
        }
    }
    return result;
}

QJsonObject toJson(const ClientCompletionItemInsertTextModeOptions &data)
{
    QJsonObject obj;
    QJsonArray arr_valueSet;
    for (const auto &v : data._valueSet) arr_valueSet.append(v);
    obj.insert("valueSet", arr_valueSet);
    return obj;
}

template<>
Utils::Result<ClientCompletionItemResolveOptions> fromJson<ClientCompletionItemResolveOptions>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for ClientCompletionItemResolveOptions");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("properties"))
        return Utils::ResultError("Missing required field: properties");
    ClientCompletionItemResolveOptions result;
    if (obj.contains("properties") && obj["properties"].isArray()) {
        const QJsonArray arr = obj["properties"].toArray();
        for (const QJsonValue &v : arr) {
            result._properties.append(v.toString());
        }
    }
    return result;
}

QJsonObject toJson(const ClientCompletionItemResolveOptions &data)
{
    QJsonObject obj;
    QJsonArray arr_properties;
    for (const auto &v : data._properties) arr_properties.append(v);
    obj.insert("properties", arr_properties);
    return obj;
}

template<>
Utils::Result<CompletionItemTagOptions> fromJson<CompletionItemTagOptions>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for CompletionItemTagOptions");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("valueSet"))
        return Utils::ResultError("Missing required field: valueSet");
    CompletionItemTagOptions result;
    if (obj.contains("valueSet") && obj["valueSet"].isArray()) {
        const QJsonArray arr = obj["valueSet"].toArray();
        for (const QJsonValue &v : arr) {
            result._valueSet.append(v.toInt());
        }
    }
    return result;
}

QJsonObject toJson(const CompletionItemTagOptions &data)
{
    QJsonObject obj;
    QJsonArray arr_valueSet;
    for (const auto &v : data._valueSet) arr_valueSet.append(v);
    obj.insert("valueSet", arr_valueSet);
    return obj;
}

template<>
Utils::Result<ClientCompletionItemOptions> fromJson<ClientCompletionItemOptions>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for ClientCompletionItemOptions");
    const QJsonObject obj = val.toObject();
    ClientCompletionItemOptions result;
    if (obj.contains("snippetSupport"))
        result._snippetSupport = obj.value("snippetSupport").toBool();
    if (obj.contains("commitCharactersSupport"))
        result._commitCharactersSupport = obj.value("commitCharactersSupport").toBool();
    if (obj.contains("documentationFormat") && obj["documentationFormat"].isArray()) {
        const QJsonArray arr = obj["documentationFormat"].toArray();
        QList<MarkupKind> list_documentationFormat;
        for (const QJsonValue &v : arr) {
            const auto res0 = fromJson<MarkupKind>("documentationFormat", v);
            if (!res0)
                return Utils::ResultError(res0.error());
            list_documentationFormat.append(*res0);
        }
        result._documentationFormat = list_documentationFormat;
    }
    if (obj.contains("deprecatedSupport"))
        result._deprecatedSupport = obj.value("deprecatedSupport").toBool();
    if (obj.contains("preselectSupport"))
        result._preselectSupport = obj.value("preselectSupport").toBool();
    if (obj.contains("tagSupport") && obj["tagSupport"].isObject()) {
        const auto res1 = fromJson<CompletionItemTagOptions>("tagSupport", obj["tagSupport"]);
        if (!res1)
            return Utils::ResultError(res1.error());
        result._tagSupport = *res1;
    }
    if (obj.contains("insertReplaceSupport"))
        result._insertReplaceSupport = obj.value("insertReplaceSupport").toBool();
    if (obj.contains("resolveSupport") && obj["resolveSupport"].isObject()) {
        const auto res2 = fromJson<ClientCompletionItemResolveOptions>("resolveSupport", obj["resolveSupport"]);
        if (!res2)
            return Utils::ResultError(res2.error());
        result._resolveSupport = *res2;
    }
    if (obj.contains("insertTextModeSupport") && obj["insertTextModeSupport"].isObject()) {
        const auto res3 = fromJson<ClientCompletionItemInsertTextModeOptions>("insertTextModeSupport", obj["insertTextModeSupport"]);
        if (!res3)
            return Utils::ResultError(res3.error());
        result._insertTextModeSupport = *res3;
    }
    if (obj.contains("labelDetailsSupport"))
        result._labelDetailsSupport = obj.value("labelDetailsSupport").toBool();
    return result;
}

QJsonObject toJson(const ClientCompletionItemOptions &data)
{
    QJsonObject obj;
    if (data._snippetSupport.has_value())
        obj.insert("snippetSupport", *data._snippetSupport);
    if (data._commitCharactersSupport.has_value())
        obj.insert("commitCharactersSupport", *data._commitCharactersSupport);
    if (data._documentationFormat.has_value()) {
        QJsonArray arr_documentationFormat;
        for (const auto &v : *data._documentationFormat) arr_documentationFormat.append(toJsonValue(v));
        obj.insert("documentationFormat", arr_documentationFormat);
    }
    if (data._deprecatedSupport.has_value())
        obj.insert("deprecatedSupport", *data._deprecatedSupport);
    if (data._preselectSupport.has_value())
        obj.insert("preselectSupport", *data._preselectSupport);
    if (data._tagSupport.has_value())
        obj.insert("tagSupport", toJson(*data._tagSupport));
    if (data._insertReplaceSupport.has_value())
        obj.insert("insertReplaceSupport", *data._insertReplaceSupport);
    if (data._resolveSupport.has_value())
        obj.insert("resolveSupport", toJson(*data._resolveSupport));
    if (data._insertTextModeSupport.has_value())
        obj.insert("insertTextModeSupport", toJson(*data._insertTextModeSupport));
    if (data._labelDetailsSupport.has_value())
        obj.insert("labelDetailsSupport", *data._labelDetailsSupport);
    return obj;
}

template<>
Utils::Result<ClientCompletionItemOptionsKind> fromJson<ClientCompletionItemOptionsKind>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for ClientCompletionItemOptionsKind");
    const QJsonObject obj = val.toObject();
    ClientCompletionItemOptionsKind result;
    if (obj.contains("valueSet") && obj["valueSet"].isArray()) {
        const QJsonArray arr = obj["valueSet"].toArray();
        QList<int> list_valueSet;
        for (const QJsonValue &v : arr) {
            list_valueSet.append(v.toInt());
        }
        result._valueSet = list_valueSet;
    }
    return result;
}

QJsonObject toJson(const ClientCompletionItemOptionsKind &data)
{
    QJsonObject obj;
    if (data._valueSet.has_value()) {
        QJsonArray arr_valueSet;
        for (const auto &v : *data._valueSet) arr_valueSet.append(v);
        obj.insert("valueSet", arr_valueSet);
    }
    return obj;
}

template<>
Utils::Result<CompletionListCapabilities> fromJson<CompletionListCapabilities>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for CompletionListCapabilities");
    const QJsonObject obj = val.toObject();
    CompletionListCapabilities result;
    if (obj.contains("itemDefaults") && obj["itemDefaults"].isArray()) {
        const QJsonArray arr = obj["itemDefaults"].toArray();
        QStringList list_itemDefaults;
        for (const QJsonValue &v : arr) {
            list_itemDefaults.append(v.toString());
        }
        result._itemDefaults = list_itemDefaults;
    }
    if (obj.contains("applyKindSupport"))
        result._applyKindSupport = obj.value("applyKindSupport").toBool();
    return result;
}

QJsonObject toJson(const CompletionListCapabilities &data)
{
    QJsonObject obj;
    if (data._itemDefaults.has_value()) {
        QJsonArray arr_itemDefaults;
        for (const auto &v : *data._itemDefaults) arr_itemDefaults.append(v);
        obj.insert("itemDefaults", arr_itemDefaults);
    }
    if (data._applyKindSupport.has_value())
        obj.insert("applyKindSupport", *data._applyKindSupport);
    return obj;
}

template<>
Utils::Result<CompletionClientCapabilities> fromJson<CompletionClientCapabilities>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for CompletionClientCapabilities");
    const QJsonObject obj = val.toObject();
    CompletionClientCapabilities result;
    if (obj.contains("dynamicRegistration"))
        result._dynamicRegistration = obj.value("dynamicRegistration").toBool();
    if (obj.contains("completionItem") && obj["completionItem"].isObject()) {
        const auto res0 = fromJson<ClientCompletionItemOptions>("completionItem", obj["completionItem"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._completionItem = *res0;
    }
    if (obj.contains("completionItemKind") && obj["completionItemKind"].isObject()) {
        const auto res1 = fromJson<ClientCompletionItemOptionsKind>("completionItemKind", obj["completionItemKind"]);
        if (!res1)
            return Utils::ResultError(res1.error());
        result._completionItemKind = *res1;
    }
    if (obj.contains("insertTextMode") && obj["insertTextMode"].isDouble())
        result._insertTextMode = obj["insertTextMode"].toInt();
    if (obj.contains("contextSupport"))
        result._contextSupport = obj.value("contextSupport").toBool();
    if (obj.contains("completionList") && obj["completionList"].isObject()) {
        const auto res2 = fromJson<CompletionListCapabilities>("completionList", obj["completionList"]);
        if (!res2)
            return Utils::ResultError(res2.error());
        result._completionList = *res2;
    }
    return result;
}

QJsonObject toJson(const CompletionClientCapabilities &data)
{
    QJsonObject obj;
    if (data._dynamicRegistration.has_value())
        obj.insert("dynamicRegistration", *data._dynamicRegistration);
    if (data._completionItem.has_value())
        obj.insert("completionItem", toJson(*data._completionItem));
    if (data._completionItemKind.has_value())
        obj.insert("completionItemKind", toJson(*data._completionItemKind));
    if (data._insertTextMode.has_value())
        obj.insert("insertTextMode", *data._insertTextMode);
    if (data._contextSupport.has_value())
        obj.insert("contextSupport", *data._contextSupport);
    if (data._completionList.has_value())
        obj.insert("completionList", toJson(*data._completionList));
    return obj;
}

template<>
Utils::Result<DeclarationClientCapabilities> fromJson<DeclarationClientCapabilities>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for DeclarationClientCapabilities");
    const QJsonObject obj = val.toObject();
    DeclarationClientCapabilities result;
    if (obj.contains("dynamicRegistration"))
        result._dynamicRegistration = obj.value("dynamicRegistration").toBool();
    if (obj.contains("linkSupport"))
        result._linkSupport = obj.value("linkSupport").toBool();
    return result;
}

QJsonObject toJson(const DeclarationClientCapabilities &data)
{
    QJsonObject obj;
    if (data._dynamicRegistration.has_value())
        obj.insert("dynamicRegistration", *data._dynamicRegistration);
    if (data._linkSupport.has_value())
        obj.insert("linkSupport", *data._linkSupport);
    return obj;
}

template<>
Utils::Result<DefinitionClientCapabilities> fromJson<DefinitionClientCapabilities>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for DefinitionClientCapabilities");
    const QJsonObject obj = val.toObject();
    DefinitionClientCapabilities result;
    if (obj.contains("dynamicRegistration"))
        result._dynamicRegistration = obj.value("dynamicRegistration").toBool();
    if (obj.contains("linkSupport"))
        result._linkSupport = obj.value("linkSupport").toBool();
    return result;
}

QJsonObject toJson(const DefinitionClientCapabilities &data)
{
    QJsonObject obj;
    if (data._dynamicRegistration.has_value())
        obj.insert("dynamicRegistration", *data._dynamicRegistration);
    if (data._linkSupport.has_value())
        obj.insert("linkSupport", *data._linkSupport);
    return obj;
}

template<>
Utils::Result<ClientDiagnosticsTagOptions> fromJson<ClientDiagnosticsTagOptions>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for ClientDiagnosticsTagOptions");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("valueSet"))
        return Utils::ResultError("Missing required field: valueSet");
    ClientDiagnosticsTagOptions result;
    if (obj.contains("valueSet") && obj["valueSet"].isArray()) {
        const QJsonArray arr = obj["valueSet"].toArray();
        for (const QJsonValue &v : arr) {
            result._valueSet.append(v.toInt());
        }
    }
    return result;
}

QJsonObject toJson(const ClientDiagnosticsTagOptions &data)
{
    QJsonObject obj;
    QJsonArray arr_valueSet;
    for (const auto &v : data._valueSet) arr_valueSet.append(v);
    obj.insert("valueSet", arr_valueSet);
    return obj;
}

template<>
Utils::Result<DiagnosticClientCapabilities> fromJson<DiagnosticClientCapabilities>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for DiagnosticClientCapabilities");
    const QJsonObject obj = val.toObject();
    DiagnosticClientCapabilities result;
    if (obj.contains("relatedInformation"))
        result._relatedInformation = obj.value("relatedInformation").toBool();
    if (obj.contains("tagSupport") && obj["tagSupport"].isObject()) {
        const auto res0 = fromJson<ClientDiagnosticsTagOptions>("tagSupport", obj["tagSupport"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._tagSupport = *res0;
    }
    if (obj.contains("codeDescriptionSupport"))
        result._codeDescriptionSupport = obj.value("codeDescriptionSupport").toBool();
    if (obj.contains("dataSupport"))
        result._dataSupport = obj.value("dataSupport").toBool();
    if (obj.contains("dynamicRegistration"))
        result._dynamicRegistration = obj.value("dynamicRegistration").toBool();
    if (obj.contains("relatedDocumentSupport"))
        result._relatedDocumentSupport = obj.value("relatedDocumentSupport").toBool();
    if (obj.contains("markupMessageSupport"))
        result._markupMessageSupport = obj.value("markupMessageSupport").toBool();
    return result;
}

QJsonObject toJson(const DiagnosticClientCapabilities &data)
{
    QJsonObject obj;
    if (data._relatedInformation.has_value())
        obj.insert("relatedInformation", *data._relatedInformation);
    if (data._tagSupport.has_value())
        obj.insert("tagSupport", toJson(*data._tagSupport));
    if (data._codeDescriptionSupport.has_value())
        obj.insert("codeDescriptionSupport", *data._codeDescriptionSupport);
    if (data._dataSupport.has_value())
        obj.insert("dataSupport", *data._dataSupport);
    if (data._dynamicRegistration.has_value())
        obj.insert("dynamicRegistration", *data._dynamicRegistration);
    if (data._relatedDocumentSupport.has_value())
        obj.insert("relatedDocumentSupport", *data._relatedDocumentSupport);
    if (data._markupMessageSupport.has_value())
        obj.insert("markupMessageSupport", *data._markupMessageSupport);
    return obj;
}

template<>
Utils::Result<DocumentColorClientCapabilities> fromJson<DocumentColorClientCapabilities>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for DocumentColorClientCapabilities");
    const QJsonObject obj = val.toObject();
    DocumentColorClientCapabilities result;
    if (obj.contains("dynamicRegistration"))
        result._dynamicRegistration = obj.value("dynamicRegistration").toBool();
    return result;
}

QJsonObject toJson(const DocumentColorClientCapabilities &data)
{
    QJsonObject obj;
    if (data._dynamicRegistration.has_value())
        obj.insert("dynamicRegistration", *data._dynamicRegistration);
    return obj;
}

template<>
Utils::Result<DocumentFormattingClientCapabilities> fromJson<DocumentFormattingClientCapabilities>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for DocumentFormattingClientCapabilities");
    const QJsonObject obj = val.toObject();
    DocumentFormattingClientCapabilities result;
    if (obj.contains("dynamicRegistration"))
        result._dynamicRegistration = obj.value("dynamicRegistration").toBool();
    return result;
}

QJsonObject toJson(const DocumentFormattingClientCapabilities &data)
{
    QJsonObject obj;
    if (data._dynamicRegistration.has_value())
        obj.insert("dynamicRegistration", *data._dynamicRegistration);
    return obj;
}

template<>
Utils::Result<DocumentHighlightClientCapabilities> fromJson<DocumentHighlightClientCapabilities>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for DocumentHighlightClientCapabilities");
    const QJsonObject obj = val.toObject();
    DocumentHighlightClientCapabilities result;
    if (obj.contains("dynamicRegistration"))
        result._dynamicRegistration = obj.value("dynamicRegistration").toBool();
    return result;
}

QJsonObject toJson(const DocumentHighlightClientCapabilities &data)
{
    QJsonObject obj;
    if (data._dynamicRegistration.has_value())
        obj.insert("dynamicRegistration", *data._dynamicRegistration);
    return obj;
}

template<>
Utils::Result<DocumentLinkClientCapabilities> fromJson<DocumentLinkClientCapabilities>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for DocumentLinkClientCapabilities");
    const QJsonObject obj = val.toObject();
    DocumentLinkClientCapabilities result;
    if (obj.contains("dynamicRegistration"))
        result._dynamicRegistration = obj.value("dynamicRegistration").toBool();
    if (obj.contains("tooltipSupport"))
        result._tooltipSupport = obj.value("tooltipSupport").toBool();
    return result;
}

QJsonObject toJson(const DocumentLinkClientCapabilities &data)
{
    QJsonObject obj;
    if (data._dynamicRegistration.has_value())
        obj.insert("dynamicRegistration", *data._dynamicRegistration);
    if (data._tooltipSupport.has_value())
        obj.insert("tooltipSupport", *data._tooltipSupport);
    return obj;
}

template<>
Utils::Result<DocumentOnTypeFormattingClientCapabilities> fromJson<DocumentOnTypeFormattingClientCapabilities>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for DocumentOnTypeFormattingClientCapabilities");
    const QJsonObject obj = val.toObject();
    DocumentOnTypeFormattingClientCapabilities result;
    if (obj.contains("dynamicRegistration"))
        result._dynamicRegistration = obj.value("dynamicRegistration").toBool();
    return result;
}

QJsonObject toJson(const DocumentOnTypeFormattingClientCapabilities &data)
{
    QJsonObject obj;
    if (data._dynamicRegistration.has_value())
        obj.insert("dynamicRegistration", *data._dynamicRegistration);
    return obj;
}

template<>
Utils::Result<DocumentRangeFormattingClientCapabilities> fromJson<DocumentRangeFormattingClientCapabilities>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for DocumentRangeFormattingClientCapabilities");
    const QJsonObject obj = val.toObject();
    DocumentRangeFormattingClientCapabilities result;
    if (obj.contains("dynamicRegistration"))
        result._dynamicRegistration = obj.value("dynamicRegistration").toBool();
    if (obj.contains("rangesSupport"))
        result._rangesSupport = obj.value("rangesSupport").toBool();
    return result;
}

QJsonObject toJson(const DocumentRangeFormattingClientCapabilities &data)
{
    QJsonObject obj;
    if (data._dynamicRegistration.has_value())
        obj.insert("dynamicRegistration", *data._dynamicRegistration);
    if (data._rangesSupport.has_value())
        obj.insert("rangesSupport", *data._rangesSupport);
    return obj;
}

template<>
Utils::Result<ClientSymbolKindOptions> fromJson<ClientSymbolKindOptions>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for ClientSymbolKindOptions");
    const QJsonObject obj = val.toObject();
    ClientSymbolKindOptions result;
    if (obj.contains("valueSet") && obj["valueSet"].isArray()) {
        const QJsonArray arr = obj["valueSet"].toArray();
        QList<int> list_valueSet;
        for (const QJsonValue &v : arr) {
            list_valueSet.append(v.toInt());
        }
        result._valueSet = list_valueSet;
    }
    return result;
}

QJsonObject toJson(const ClientSymbolKindOptions &data)
{
    QJsonObject obj;
    if (data._valueSet.has_value()) {
        QJsonArray arr_valueSet;
        for (const auto &v : *data._valueSet) arr_valueSet.append(v);
        obj.insert("valueSet", arr_valueSet);
    }
    return obj;
}

template<>
Utils::Result<ClientSymbolTagOptions> fromJson<ClientSymbolTagOptions>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for ClientSymbolTagOptions");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("valueSet"))
        return Utils::ResultError("Missing required field: valueSet");
    ClientSymbolTagOptions result;
    if (obj.contains("valueSet") && obj["valueSet"].isArray()) {
        const QJsonArray arr = obj["valueSet"].toArray();
        for (const QJsonValue &v : arr) {
            result._valueSet.append(v.toInt());
        }
    }
    return result;
}

QJsonObject toJson(const ClientSymbolTagOptions &data)
{
    QJsonObject obj;
    QJsonArray arr_valueSet;
    for (const auto &v : data._valueSet) arr_valueSet.append(v);
    obj.insert("valueSet", arr_valueSet);
    return obj;
}

template<>
Utils::Result<DocumentSymbolClientCapabilities> fromJson<DocumentSymbolClientCapabilities>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for DocumentSymbolClientCapabilities");
    const QJsonObject obj = val.toObject();
    DocumentSymbolClientCapabilities result;
    if (obj.contains("dynamicRegistration"))
        result._dynamicRegistration = obj.value("dynamicRegistration").toBool();
    if (obj.contains("symbolKind") && obj["symbolKind"].isObject()) {
        const auto res0 = fromJson<ClientSymbolKindOptions>("symbolKind", obj["symbolKind"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._symbolKind = *res0;
    }
    if (obj.contains("hierarchicalDocumentSymbolSupport"))
        result._hierarchicalDocumentSymbolSupport = obj.value("hierarchicalDocumentSymbolSupport").toBool();
    if (obj.contains("tagSupport") && obj["tagSupport"].isObject()) {
        const auto res1 = fromJson<ClientSymbolTagOptions>("tagSupport", obj["tagSupport"]);
        if (!res1)
            return Utils::ResultError(res1.error());
        result._tagSupport = *res1;
    }
    if (obj.contains("labelSupport"))
        result._labelSupport = obj.value("labelSupport").toBool();
    return result;
}

QJsonObject toJson(const DocumentSymbolClientCapabilities &data)
{
    QJsonObject obj;
    if (data._dynamicRegistration.has_value())
        obj.insert("dynamicRegistration", *data._dynamicRegistration);
    if (data._symbolKind.has_value())
        obj.insert("symbolKind", toJson(*data._symbolKind));
    if (data._hierarchicalDocumentSymbolSupport.has_value())
        obj.insert("hierarchicalDocumentSymbolSupport", *data._hierarchicalDocumentSymbolSupport);
    if (data._tagSupport.has_value())
        obj.insert("tagSupport", toJson(*data._tagSupport));
    if (data._labelSupport.has_value())
        obj.insert("labelSupport", *data._labelSupport);
    return obj;
}

template<>
Utils::Result<ClientFoldingRangeKindOptions> fromJson<ClientFoldingRangeKindOptions>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for ClientFoldingRangeKindOptions");
    const QJsonObject obj = val.toObject();
    ClientFoldingRangeKindOptions result;
    if (obj.contains("valueSet") && obj["valueSet"].isArray()) {
        const QJsonArray arr = obj["valueSet"].toArray();
        QList<FoldingRangeKind> list_valueSet;
        for (const QJsonValue &v : arr) {
            const auto res0 = fromJson<FoldingRangeKind>("valueSet", v);
            if (!res0)
                return Utils::ResultError(res0.error());
            list_valueSet.append(*res0);
        }
        result._valueSet = list_valueSet;
    }
    return result;
}

QJsonObject toJson(const ClientFoldingRangeKindOptions &data)
{
    QJsonObject obj;
    if (data._valueSet.has_value()) {
        QJsonArray arr_valueSet;
        for (const auto &v : *data._valueSet) arr_valueSet.append(v);
        obj.insert("valueSet", arr_valueSet);
    }
    return obj;
}

template<>
Utils::Result<ClientFoldingRangeOptions> fromJson<ClientFoldingRangeOptions>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for ClientFoldingRangeOptions");
    const QJsonObject obj = val.toObject();
    ClientFoldingRangeOptions result;
    if (obj.contains("collapsedText"))
        result._collapsedText = obj.value("collapsedText").toBool();
    return result;
}

QJsonObject toJson(const ClientFoldingRangeOptions &data)
{
    QJsonObject obj;
    if (data._collapsedText.has_value())
        obj.insert("collapsedText", *data._collapsedText);
    return obj;
}

template<>
Utils::Result<FoldingRangeClientCapabilities> fromJson<FoldingRangeClientCapabilities>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for FoldingRangeClientCapabilities");
    const QJsonObject obj = val.toObject();
    FoldingRangeClientCapabilities result;
    if (obj.contains("dynamicRegistration"))
        result._dynamicRegistration = obj.value("dynamicRegistration").toBool();
    if (obj.contains("rangeLimit"))
        result._rangeLimit = obj.value("rangeLimit").toInt();
    if (obj.contains("lineFoldingOnly"))
        result._lineFoldingOnly = obj.value("lineFoldingOnly").toBool();
    if (obj.contains("foldingRangeKind") && obj["foldingRangeKind"].isObject()) {
        const auto res0 = fromJson<ClientFoldingRangeKindOptions>("foldingRangeKind", obj["foldingRangeKind"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._foldingRangeKind = *res0;
    }
    if (obj.contains("foldingRange") && obj["foldingRange"].isObject()) {
        const auto res1 = fromJson<ClientFoldingRangeOptions>("foldingRange", obj["foldingRange"]);
        if (!res1)
            return Utils::ResultError(res1.error());
        result._foldingRange = *res1;
    }
    return result;
}

QJsonObject toJson(const FoldingRangeClientCapabilities &data)
{
    QJsonObject obj;
    if (data._dynamicRegistration.has_value())
        obj.insert("dynamicRegistration", *data._dynamicRegistration);
    if (data._rangeLimit.has_value())
        obj.insert("rangeLimit", *data._rangeLimit);
    if (data._lineFoldingOnly.has_value())
        obj.insert("lineFoldingOnly", *data._lineFoldingOnly);
    if (data._foldingRangeKind.has_value())
        obj.insert("foldingRangeKind", toJson(*data._foldingRangeKind));
    if (data._foldingRange.has_value())
        obj.insert("foldingRange", toJson(*data._foldingRange));
    return obj;
}

template<>
Utils::Result<HoverClientCapabilities> fromJson<HoverClientCapabilities>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for HoverClientCapabilities");
    const QJsonObject obj = val.toObject();
    HoverClientCapabilities result;
    if (obj.contains("dynamicRegistration"))
        result._dynamicRegistration = obj.value("dynamicRegistration").toBool();
    if (obj.contains("contentFormat") && obj["contentFormat"].isArray()) {
        const QJsonArray arr = obj["contentFormat"].toArray();
        QList<MarkupKind> list_contentFormat;
        for (const QJsonValue &v : arr) {
            const auto res0 = fromJson<MarkupKind>("contentFormat", v);
            if (!res0)
                return Utils::ResultError(res0.error());
            list_contentFormat.append(*res0);
        }
        result._contentFormat = list_contentFormat;
    }
    return result;
}

QJsonObject toJson(const HoverClientCapabilities &data)
{
    QJsonObject obj;
    if (data._dynamicRegistration.has_value())
        obj.insert("dynamicRegistration", *data._dynamicRegistration);
    if (data._contentFormat.has_value()) {
        QJsonArray arr_contentFormat;
        for (const auto &v : *data._contentFormat) arr_contentFormat.append(toJsonValue(v));
        obj.insert("contentFormat", arr_contentFormat);
    }
    return obj;
}

template<>
Utils::Result<ImplementationClientCapabilities> fromJson<ImplementationClientCapabilities>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for ImplementationClientCapabilities");
    const QJsonObject obj = val.toObject();
    ImplementationClientCapabilities result;
    if (obj.contains("dynamicRegistration"))
        result._dynamicRegistration = obj.value("dynamicRegistration").toBool();
    if (obj.contains("linkSupport"))
        result._linkSupport = obj.value("linkSupport").toBool();
    return result;
}

QJsonObject toJson(const ImplementationClientCapabilities &data)
{
    QJsonObject obj;
    if (data._dynamicRegistration.has_value())
        obj.insert("dynamicRegistration", *data._dynamicRegistration);
    if (data._linkSupport.has_value())
        obj.insert("linkSupport", *data._linkSupport);
    return obj;
}

template<>
Utils::Result<ClientInlayHintResolveOptions> fromJson<ClientInlayHintResolveOptions>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for ClientInlayHintResolveOptions");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("properties"))
        return Utils::ResultError("Missing required field: properties");
    ClientInlayHintResolveOptions result;
    if (obj.contains("properties") && obj["properties"].isArray()) {
        const QJsonArray arr = obj["properties"].toArray();
        for (const QJsonValue &v : arr) {
            result._properties.append(v.toString());
        }
    }
    return result;
}

QJsonObject toJson(const ClientInlayHintResolveOptions &data)
{
    QJsonObject obj;
    QJsonArray arr_properties;
    for (const auto &v : data._properties) arr_properties.append(v);
    obj.insert("properties", arr_properties);
    return obj;
}

template<>
Utils::Result<InlayHintClientCapabilities> fromJson<InlayHintClientCapabilities>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for InlayHintClientCapabilities");
    const QJsonObject obj = val.toObject();
    InlayHintClientCapabilities result;
    if (obj.contains("dynamicRegistration"))
        result._dynamicRegistration = obj.value("dynamicRegistration").toBool();
    if (obj.contains("resolveSupport") && obj["resolveSupport"].isObject()) {
        const auto res0 = fromJson<ClientInlayHintResolveOptions>("resolveSupport", obj["resolveSupport"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._resolveSupport = *res0;
    }
    return result;
}

QJsonObject toJson(const InlayHintClientCapabilities &data)
{
    QJsonObject obj;
    if (data._dynamicRegistration.has_value())
        obj.insert("dynamicRegistration", *data._dynamicRegistration);
    if (data._resolveSupport.has_value())
        obj.insert("resolveSupport", toJson(*data._resolveSupport));
    return obj;
}

template<>
Utils::Result<InlineCompletionClientCapabilities> fromJson<InlineCompletionClientCapabilities>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for InlineCompletionClientCapabilities");
    const QJsonObject obj = val.toObject();
    InlineCompletionClientCapabilities result;
    if (obj.contains("dynamicRegistration"))
        result._dynamicRegistration = obj.value("dynamicRegistration").toBool();
    return result;
}

QJsonObject toJson(const InlineCompletionClientCapabilities &data)
{
    QJsonObject obj;
    if (data._dynamicRegistration.has_value())
        obj.insert("dynamicRegistration", *data._dynamicRegistration);
    return obj;
}

template<>
Utils::Result<InlineValueClientCapabilities> fromJson<InlineValueClientCapabilities>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for InlineValueClientCapabilities");
    const QJsonObject obj = val.toObject();
    InlineValueClientCapabilities result;
    if (obj.contains("dynamicRegistration"))
        result._dynamicRegistration = obj.value("dynamicRegistration").toBool();
    return result;
}

QJsonObject toJson(const InlineValueClientCapabilities &data)
{
    QJsonObject obj;
    if (data._dynamicRegistration.has_value())
        obj.insert("dynamicRegistration", *data._dynamicRegistration);
    return obj;
}

template<>
Utils::Result<LinkedEditingRangeClientCapabilities> fromJson<LinkedEditingRangeClientCapabilities>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for LinkedEditingRangeClientCapabilities");
    const QJsonObject obj = val.toObject();
    LinkedEditingRangeClientCapabilities result;
    if (obj.contains("dynamicRegistration"))
        result._dynamicRegistration = obj.value("dynamicRegistration").toBool();
    return result;
}

QJsonObject toJson(const LinkedEditingRangeClientCapabilities &data)
{
    QJsonObject obj;
    if (data._dynamicRegistration.has_value())
        obj.insert("dynamicRegistration", *data._dynamicRegistration);
    return obj;
}

template<>
Utils::Result<MonikerClientCapabilities> fromJson<MonikerClientCapabilities>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for MonikerClientCapabilities");
    const QJsonObject obj = val.toObject();
    MonikerClientCapabilities result;
    if (obj.contains("dynamicRegistration"))
        result._dynamicRegistration = obj.value("dynamicRegistration").toBool();
    return result;
}

QJsonObject toJson(const MonikerClientCapabilities &data)
{
    QJsonObject obj;
    if (data._dynamicRegistration.has_value())
        obj.insert("dynamicRegistration", *data._dynamicRegistration);
    return obj;
}

template<>
Utils::Result<PublishDiagnosticsClientCapabilities> fromJson<PublishDiagnosticsClientCapabilities>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for PublishDiagnosticsClientCapabilities");
    const QJsonObject obj = val.toObject();
    PublishDiagnosticsClientCapabilities result;
    if (obj.contains("relatedInformation"))
        result._relatedInformation = obj.value("relatedInformation").toBool();
    if (obj.contains("tagSupport") && obj["tagSupport"].isObject()) {
        const auto res0 = fromJson<ClientDiagnosticsTagOptions>("tagSupport", obj["tagSupport"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._tagSupport = *res0;
    }
    if (obj.contains("codeDescriptionSupport"))
        result._codeDescriptionSupport = obj.value("codeDescriptionSupport").toBool();
    if (obj.contains("dataSupport"))
        result._dataSupport = obj.value("dataSupport").toBool();
    if (obj.contains("versionSupport"))
        result._versionSupport = obj.value("versionSupport").toBool();
    return result;
}

QJsonObject toJson(const PublishDiagnosticsClientCapabilities &data)
{
    QJsonObject obj;
    if (data._relatedInformation.has_value())
        obj.insert("relatedInformation", *data._relatedInformation);
    if (data._tagSupport.has_value())
        obj.insert("tagSupport", toJson(*data._tagSupport));
    if (data._codeDescriptionSupport.has_value())
        obj.insert("codeDescriptionSupport", *data._codeDescriptionSupport);
    if (data._dataSupport.has_value())
        obj.insert("dataSupport", *data._dataSupport);
    if (data._versionSupport.has_value())
        obj.insert("versionSupport", *data._versionSupport);
    return obj;
}

template<>
Utils::Result<ReferenceClientCapabilities> fromJson<ReferenceClientCapabilities>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for ReferenceClientCapabilities");
    const QJsonObject obj = val.toObject();
    ReferenceClientCapabilities result;
    if (obj.contains("dynamicRegistration"))
        result._dynamicRegistration = obj.value("dynamicRegistration").toBool();
    return result;
}

QJsonObject toJson(const ReferenceClientCapabilities &data)
{
    QJsonObject obj;
    if (data._dynamicRegistration.has_value())
        obj.insert("dynamicRegistration", *data._dynamicRegistration);
    return obj;
}

template<>
Utils::Result<RenameClientCapabilities> fromJson<RenameClientCapabilities>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for RenameClientCapabilities");
    const QJsonObject obj = val.toObject();
    RenameClientCapabilities result;
    if (obj.contains("dynamicRegistration"))
        result._dynamicRegistration = obj.value("dynamicRegistration").toBool();
    if (obj.contains("prepareSupport"))
        result._prepareSupport = obj.value("prepareSupport").toBool();
    if (obj.contains("prepareSupportDefaultBehavior") && obj["prepareSupportDefaultBehavior"].isDouble())
        result._prepareSupportDefaultBehavior = obj["prepareSupportDefaultBehavior"].toInt();
    if (obj.contains("honorsChangeAnnotations"))
        result._honorsChangeAnnotations = obj.value("honorsChangeAnnotations").toBool();
    return result;
}

QJsonObject toJson(const RenameClientCapabilities &data)
{
    QJsonObject obj;
    if (data._dynamicRegistration.has_value())
        obj.insert("dynamicRegistration", *data._dynamicRegistration);
    if (data._prepareSupport.has_value())
        obj.insert("prepareSupport", *data._prepareSupport);
    if (data._prepareSupportDefaultBehavior.has_value())
        obj.insert("prepareSupportDefaultBehavior", *data._prepareSupportDefaultBehavior);
    if (data._honorsChangeAnnotations.has_value())
        obj.insert("honorsChangeAnnotations", *data._honorsChangeAnnotations);
    return obj;
}

template<>
Utils::Result<SelectionRangeClientCapabilities> fromJson<SelectionRangeClientCapabilities>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for SelectionRangeClientCapabilities");
    const QJsonObject obj = val.toObject();
    SelectionRangeClientCapabilities result;
    if (obj.contains("dynamicRegistration"))
        result._dynamicRegistration = obj.value("dynamicRegistration").toBool();
    return result;
}

QJsonObject toJson(const SelectionRangeClientCapabilities &data)
{
    QJsonObject obj;
    if (data._dynamicRegistration.has_value())
        obj.insert("dynamicRegistration", *data._dynamicRegistration);
    return obj;
}

template<>
Utils::Result<ClientSemanticTokensRequestFullDelta> fromJson<ClientSemanticTokensRequestFullDelta>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for ClientSemanticTokensRequestFullDelta");
    const QJsonObject obj = val.toObject();
    ClientSemanticTokensRequestFullDelta result;
    if (obj.contains("delta"))
        result._delta = obj.value("delta").toBool();
    return result;
}

QJsonObject toJson(const ClientSemanticTokensRequestFullDelta &data)
{
    QJsonObject obj;
    if (data._delta.has_value())
        obj.insert("delta", *data._delta);
    return obj;
}

template<>
Utils::Result<ClientSemanticTokensRequestOptionsFull> fromJson<ClientSemanticTokensRequestOptionsFull>(const QJsonValue &val)
{
    if (val.isBool())
        return ClientSemanticTokensRequestOptionsFull(val.toBool());
    {
        auto result = fromJson<ClientSemanticTokensRequestFullDelta>(val);
        if (result) return ClientSemanticTokensRequestOptionsFull(*result);
    }
    return Utils::ResultError("Invalid ClientSemanticTokensRequestOptionsFull");
}

QJsonValue toJsonValue(const ClientSemanticTokensRequestOptionsFull &val)
{
    return std::visit([](const auto &v) -> QJsonValue {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, ClientSemanticTokensRequestFullDelta>) {
            return toJson(v);
        } else
        {
            return QVariant::fromValue(v).toJsonValue();
        }
    }, val);
}

template<>
Utils::Result<ClientSemanticTokensRequestOptions> fromJson<ClientSemanticTokensRequestOptions>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for ClientSemanticTokensRequestOptions");
    const QJsonObject obj = val.toObject();
    ClientSemanticTokensRequestOptions result;
    if (obj.contains("range")) {
        const auto res0 = fromJson<SemanticTokensRegistrationOptionsRange>("range", obj["range"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._range = *res0;
    }
    if (obj.contains("full")) {
        const auto res1 = fromJson<ClientSemanticTokensRequestOptionsFull>("full", obj["full"]);
        if (!res1)
            return Utils::ResultError(res1.error());
        result._full = *res1;
    }
    return result;
}

QJsonObject toJson(const ClientSemanticTokensRequestOptions &data)
{
    QJsonObject obj;
    if (data._range.has_value())
        obj.insert("range", toJsonValue(*data._range));
    if (data._full.has_value())
        obj.insert("full", toJsonValue(*data._full));
    return obj;
}

QString toString(TokenFormat v)
{
    switch(v) {
        case TokenFormat::relative: return "relative";
    }
    return {};
}

template<>
Utils::Result<TokenFormat> fromJson<TokenFormat>(const QJsonValue &val)
{
    if (!val.isString())
        return Utils::ResultError("Expected JSON string for TokenFormat");
    const QString str = val.toString();
    if (str == "relative") return TokenFormat::relative;
    return Utils::ResultError("Invalid TokenFormat value: " + str);
}

QJsonValue toJsonValue(const TokenFormat &v)
{
    return toString(v);
}

template<>
Utils::Result<SemanticTokensClientCapabilities> fromJson<SemanticTokensClientCapabilities>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for SemanticTokensClientCapabilities");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("requests"))
        return Utils::ResultError("Missing required field: requests");
    if (!obj.contains("tokenTypes"))
        return Utils::ResultError("Missing required field: tokenTypes");
    if (!obj.contains("tokenModifiers"))
        return Utils::ResultError("Missing required field: tokenModifiers");
    if (!obj.contains("formats"))
        return Utils::ResultError("Missing required field: formats");
    SemanticTokensClientCapabilities result;
    if (obj.contains("dynamicRegistration"))
        result._dynamicRegistration = obj.value("dynamicRegistration").toBool();
    if (obj.contains("requests") && obj["requests"].isObject()) {
        const auto res0 = fromJson<ClientSemanticTokensRequestOptions>("requests", obj["requests"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._requests = *res0;
    }
    if (obj.contains("tokenTypes") && obj["tokenTypes"].isArray()) {
        const QJsonArray arr = obj["tokenTypes"].toArray();
        for (const QJsonValue &v : arr) {
            result._tokenTypes.append(v.toString());
        }
    }
    if (obj.contains("tokenModifiers") && obj["tokenModifiers"].isArray()) {
        const QJsonArray arr = obj["tokenModifiers"].toArray();
        for (const QJsonValue &v : arr) {
            result._tokenModifiers.append(v.toString());
        }
    }
    if (obj.contains("formats") && obj["formats"].isArray()) {
        const QJsonArray arr = obj["formats"].toArray();
        for (const QJsonValue &v : arr) {
            const auto res1 = fromJson<TokenFormat>("formats", v);
            if (!res1)
                return Utils::ResultError(res1.error());
            result._formats.append(*res1);
        }
    }
    if (obj.contains("overlappingTokenSupport"))
        result._overlappingTokenSupport = obj.value("overlappingTokenSupport").toBool();
    if (obj.contains("multilineTokenSupport"))
        result._multilineTokenSupport = obj.value("multilineTokenSupport").toBool();
    if (obj.contains("serverCancelSupport"))
        result._serverCancelSupport = obj.value("serverCancelSupport").toBool();
    if (obj.contains("augmentsSyntaxTokens"))
        result._augmentsSyntaxTokens = obj.value("augmentsSyntaxTokens").toBool();
    return result;
}

QJsonObject toJson(const SemanticTokensClientCapabilities &data)
{
    QJsonObject obj{{"requests", toJson(data._requests)}};
    if (data._dynamicRegistration.has_value())
        obj.insert("dynamicRegistration", *data._dynamicRegistration);
    QJsonArray arr_tokenTypes;
    for (const auto &v : data._tokenTypes) arr_tokenTypes.append(v);
    obj.insert("tokenTypes", arr_tokenTypes);
    QJsonArray arr_tokenModifiers;
    for (const auto &v : data._tokenModifiers) arr_tokenModifiers.append(v);
    obj.insert("tokenModifiers", arr_tokenModifiers);
    QJsonArray arr_formats;
    for (const auto &v : data._formats) arr_formats.append(toJsonValue(v));
    obj.insert("formats", arr_formats);
    if (data._overlappingTokenSupport.has_value())
        obj.insert("overlappingTokenSupport", *data._overlappingTokenSupport);
    if (data._multilineTokenSupport.has_value())
        obj.insert("multilineTokenSupport", *data._multilineTokenSupport);
    if (data._serverCancelSupport.has_value())
        obj.insert("serverCancelSupport", *data._serverCancelSupport);
    if (data._augmentsSyntaxTokens.has_value())
        obj.insert("augmentsSyntaxTokens", *data._augmentsSyntaxTokens);
    return obj;
}

template<>
Utils::Result<ClientSignatureParameterInformationOptions> fromJson<ClientSignatureParameterInformationOptions>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for ClientSignatureParameterInformationOptions");
    const QJsonObject obj = val.toObject();
    ClientSignatureParameterInformationOptions result;
    if (obj.contains("labelOffsetSupport"))
        result._labelOffsetSupport = obj.value("labelOffsetSupport").toBool();
    return result;
}

QJsonObject toJson(const ClientSignatureParameterInformationOptions &data)
{
    QJsonObject obj;
    if (data._labelOffsetSupport.has_value())
        obj.insert("labelOffsetSupport", *data._labelOffsetSupport);
    return obj;
}

template<>
Utils::Result<ClientSignatureInformationOptions> fromJson<ClientSignatureInformationOptions>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for ClientSignatureInformationOptions");
    const QJsonObject obj = val.toObject();
    ClientSignatureInformationOptions result;
    if (obj.contains("documentationFormat") && obj["documentationFormat"].isArray()) {
        const QJsonArray arr = obj["documentationFormat"].toArray();
        QList<MarkupKind> list_documentationFormat;
        for (const QJsonValue &v : arr) {
            const auto res0 = fromJson<MarkupKind>("documentationFormat", v);
            if (!res0)
                return Utils::ResultError(res0.error());
            list_documentationFormat.append(*res0);
        }
        result._documentationFormat = list_documentationFormat;
    }
    if (obj.contains("parameterInformation") && obj["parameterInformation"].isObject()) {
        const auto res1 = fromJson<ClientSignatureParameterInformationOptions>("parameterInformation", obj["parameterInformation"]);
        if (!res1)
            return Utils::ResultError(res1.error());
        result._parameterInformation = *res1;
    }
    if (obj.contains("activeParameterSupport"))
        result._activeParameterSupport = obj.value("activeParameterSupport").toBool();
    if (obj.contains("noActiveParameterSupport"))
        result._noActiveParameterSupport = obj.value("noActiveParameterSupport").toBool();
    return result;
}

QJsonObject toJson(const ClientSignatureInformationOptions &data)
{
    QJsonObject obj;
    if (data._documentationFormat.has_value()) {
        QJsonArray arr_documentationFormat;
        for (const auto &v : *data._documentationFormat) arr_documentationFormat.append(toJsonValue(v));
        obj.insert("documentationFormat", arr_documentationFormat);
    }
    if (data._parameterInformation.has_value())
        obj.insert("parameterInformation", toJson(*data._parameterInformation));
    if (data._activeParameterSupport.has_value())
        obj.insert("activeParameterSupport", *data._activeParameterSupport);
    if (data._noActiveParameterSupport.has_value())
        obj.insert("noActiveParameterSupport", *data._noActiveParameterSupport);
    return obj;
}

template<>
Utils::Result<SignatureHelpClientCapabilities> fromJson<SignatureHelpClientCapabilities>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for SignatureHelpClientCapabilities");
    const QJsonObject obj = val.toObject();
    SignatureHelpClientCapabilities result;
    if (obj.contains("dynamicRegistration"))
        result._dynamicRegistration = obj.value("dynamicRegistration").toBool();
    if (obj.contains("signatureInformation") && obj["signatureInformation"].isObject()) {
        const auto res0 = fromJson<ClientSignatureInformationOptions>("signatureInformation", obj["signatureInformation"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._signatureInformation = *res0;
    }
    if (obj.contains("contextSupport"))
        result._contextSupport = obj.value("contextSupport").toBool();
    return result;
}

QJsonObject toJson(const SignatureHelpClientCapabilities &data)
{
    QJsonObject obj;
    if (data._dynamicRegistration.has_value())
        obj.insert("dynamicRegistration", *data._dynamicRegistration);
    if (data._signatureInformation.has_value())
        obj.insert("signatureInformation", toJson(*data._signatureInformation));
    if (data._contextSupport.has_value())
        obj.insert("contextSupport", *data._contextSupport);
    return obj;
}

template<>
Utils::Result<TextDocumentFilterClientCapabilities> fromJson<TextDocumentFilterClientCapabilities>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for TextDocumentFilterClientCapabilities");
    const QJsonObject obj = val.toObject();
    TextDocumentFilterClientCapabilities result;
    if (obj.contains("relativePatternSupport"))
        result._relativePatternSupport = obj.value("relativePatternSupport").toBool();
    return result;
}

QJsonObject toJson(const TextDocumentFilterClientCapabilities &data)
{
    QJsonObject obj;
    if (data._relativePatternSupport.has_value())
        obj.insert("relativePatternSupport", *data._relativePatternSupport);
    return obj;
}

template<>
Utils::Result<TextDocumentSyncClientCapabilities> fromJson<TextDocumentSyncClientCapabilities>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for TextDocumentSyncClientCapabilities");
    const QJsonObject obj = val.toObject();
    TextDocumentSyncClientCapabilities result;
    if (obj.contains("dynamicRegistration"))
        result._dynamicRegistration = obj.value("dynamicRegistration").toBool();
    if (obj.contains("willSave"))
        result._willSave = obj.value("willSave").toBool();
    if (obj.contains("willSaveWaitUntil"))
        result._willSaveWaitUntil = obj.value("willSaveWaitUntil").toBool();
    if (obj.contains("didSave"))
        result._didSave = obj.value("didSave").toBool();
    return result;
}

QJsonObject toJson(const TextDocumentSyncClientCapabilities &data)
{
    QJsonObject obj;
    if (data._dynamicRegistration.has_value())
        obj.insert("dynamicRegistration", *data._dynamicRegistration);
    if (data._willSave.has_value())
        obj.insert("willSave", *data._willSave);
    if (data._willSaveWaitUntil.has_value())
        obj.insert("willSaveWaitUntil", *data._willSaveWaitUntil);
    if (data._didSave.has_value())
        obj.insert("didSave", *data._didSave);
    return obj;
}

template<>
Utils::Result<TypeDefinitionClientCapabilities> fromJson<TypeDefinitionClientCapabilities>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for TypeDefinitionClientCapabilities");
    const QJsonObject obj = val.toObject();
    TypeDefinitionClientCapabilities result;
    if (obj.contains("dynamicRegistration"))
        result._dynamicRegistration = obj.value("dynamicRegistration").toBool();
    if (obj.contains("linkSupport"))
        result._linkSupport = obj.value("linkSupport").toBool();
    return result;
}

QJsonObject toJson(const TypeDefinitionClientCapabilities &data)
{
    QJsonObject obj;
    if (data._dynamicRegistration.has_value())
        obj.insert("dynamicRegistration", *data._dynamicRegistration);
    if (data._linkSupport.has_value())
        obj.insert("linkSupport", *data._linkSupport);
    return obj;
}

template<>
Utils::Result<TypeHierarchyClientCapabilities> fromJson<TypeHierarchyClientCapabilities>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for TypeHierarchyClientCapabilities");
    const QJsonObject obj = val.toObject();
    TypeHierarchyClientCapabilities result;
    if (obj.contains("dynamicRegistration"))
        result._dynamicRegistration = obj.value("dynamicRegistration").toBool();
    return result;
}

QJsonObject toJson(const TypeHierarchyClientCapabilities &data)
{
    QJsonObject obj;
    if (data._dynamicRegistration.has_value())
        obj.insert("dynamicRegistration", *data._dynamicRegistration);
    return obj;
}

template<>
Utils::Result<TextDocumentClientCapabilities> fromJson<TextDocumentClientCapabilities>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for TextDocumentClientCapabilities");
    const QJsonObject obj = val.toObject();
    TextDocumentClientCapabilities result;
    if (obj.contains("synchronization") && obj["synchronization"].isObject()) {
        const auto res0 = fromJson<TextDocumentSyncClientCapabilities>("synchronization", obj["synchronization"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._synchronization = *res0;
    }
    if (obj.contains("filters") && obj["filters"].isObject()) {
        const auto res1 = fromJson<TextDocumentFilterClientCapabilities>("filters", obj["filters"]);
        if (!res1)
            return Utils::ResultError(res1.error());
        result._filters = *res1;
    }
    if (obj.contains("completion") && obj["completion"].isObject()) {
        const auto res2 = fromJson<CompletionClientCapabilities>("completion", obj["completion"]);
        if (!res2)
            return Utils::ResultError(res2.error());
        result._completion = *res2;
    }
    if (obj.contains("hover") && obj["hover"].isObject()) {
        const auto res3 = fromJson<HoverClientCapabilities>("hover", obj["hover"]);
        if (!res3)
            return Utils::ResultError(res3.error());
        result._hover = *res3;
    }
    if (obj.contains("signatureHelp") && obj["signatureHelp"].isObject()) {
        const auto res4 = fromJson<SignatureHelpClientCapabilities>("signatureHelp", obj["signatureHelp"]);
        if (!res4)
            return Utils::ResultError(res4.error());
        result._signatureHelp = *res4;
    }
    if (obj.contains("declaration") && obj["declaration"].isObject()) {
        const auto res5 = fromJson<DeclarationClientCapabilities>("declaration", obj["declaration"]);
        if (!res5)
            return Utils::ResultError(res5.error());
        result._declaration = *res5;
    }
    if (obj.contains("definition") && obj["definition"].isObject()) {
        const auto res6 = fromJson<DefinitionClientCapabilities>("definition", obj["definition"]);
        if (!res6)
            return Utils::ResultError(res6.error());
        result._definition = *res6;
    }
    if (obj.contains("typeDefinition") && obj["typeDefinition"].isObject()) {
        const auto res7 = fromJson<TypeDefinitionClientCapabilities>("typeDefinition", obj["typeDefinition"]);
        if (!res7)
            return Utils::ResultError(res7.error());
        result._typeDefinition = *res7;
    }
    if (obj.contains("implementation") && obj["implementation"].isObject()) {
        const auto res8 = fromJson<ImplementationClientCapabilities>("implementation", obj["implementation"]);
        if (!res8)
            return Utils::ResultError(res8.error());
        result._implementation = *res8;
    }
    if (obj.contains("references") && obj["references"].isObject()) {
        const auto res9 = fromJson<ReferenceClientCapabilities>("references", obj["references"]);
        if (!res9)
            return Utils::ResultError(res9.error());
        result._references = *res9;
    }
    if (obj.contains("documentHighlight") && obj["documentHighlight"].isObject()) {
        const auto res10 = fromJson<DocumentHighlightClientCapabilities>("documentHighlight", obj["documentHighlight"]);
        if (!res10)
            return Utils::ResultError(res10.error());
        result._documentHighlight = *res10;
    }
    if (obj.contains("documentSymbol") && obj["documentSymbol"].isObject()) {
        const auto res11 = fromJson<DocumentSymbolClientCapabilities>("documentSymbol", obj["documentSymbol"]);
        if (!res11)
            return Utils::ResultError(res11.error());
        result._documentSymbol = *res11;
    }
    if (obj.contains("codeAction") && obj["codeAction"].isObject()) {
        const auto res12 = fromJson<CodeActionClientCapabilities>("codeAction", obj["codeAction"]);
        if (!res12)
            return Utils::ResultError(res12.error());
        result._codeAction = *res12;
    }
    if (obj.contains("codeLens") && obj["codeLens"].isObject()) {
        const auto res13 = fromJson<CodeLensClientCapabilities>("codeLens", obj["codeLens"]);
        if (!res13)
            return Utils::ResultError(res13.error());
        result._codeLens = *res13;
    }
    if (obj.contains("documentLink") && obj["documentLink"].isObject()) {
        const auto res14 = fromJson<DocumentLinkClientCapabilities>("documentLink", obj["documentLink"]);
        if (!res14)
            return Utils::ResultError(res14.error());
        result._documentLink = *res14;
    }
    if (obj.contains("colorProvider") && obj["colorProvider"].isObject()) {
        const auto res15 = fromJson<DocumentColorClientCapabilities>("colorProvider", obj["colorProvider"]);
        if (!res15)
            return Utils::ResultError(res15.error());
        result._colorProvider = *res15;
    }
    if (obj.contains("formatting") && obj["formatting"].isObject()) {
        const auto res16 = fromJson<DocumentFormattingClientCapabilities>("formatting", obj["formatting"]);
        if (!res16)
            return Utils::ResultError(res16.error());
        result._formatting = *res16;
    }
    if (obj.contains("rangeFormatting") && obj["rangeFormatting"].isObject()) {
        const auto res17 = fromJson<DocumentRangeFormattingClientCapabilities>("rangeFormatting", obj["rangeFormatting"]);
        if (!res17)
            return Utils::ResultError(res17.error());
        result._rangeFormatting = *res17;
    }
    if (obj.contains("onTypeFormatting") && obj["onTypeFormatting"].isObject()) {
        const auto res18 = fromJson<DocumentOnTypeFormattingClientCapabilities>("onTypeFormatting", obj["onTypeFormatting"]);
        if (!res18)
            return Utils::ResultError(res18.error());
        result._onTypeFormatting = *res18;
    }
    if (obj.contains("rename") && obj["rename"].isObject()) {
        const auto res19 = fromJson<RenameClientCapabilities>("rename", obj["rename"]);
        if (!res19)
            return Utils::ResultError(res19.error());
        result._rename = *res19;
    }
    if (obj.contains("foldingRange") && obj["foldingRange"].isObject()) {
        const auto res20 = fromJson<FoldingRangeClientCapabilities>("foldingRange", obj["foldingRange"]);
        if (!res20)
            return Utils::ResultError(res20.error());
        result._foldingRange = *res20;
    }
    if (obj.contains("selectionRange") && obj["selectionRange"].isObject()) {
        const auto res21 = fromJson<SelectionRangeClientCapabilities>("selectionRange", obj["selectionRange"]);
        if (!res21)
            return Utils::ResultError(res21.error());
        result._selectionRange = *res21;
    }
    if (obj.contains("publishDiagnostics") && obj["publishDiagnostics"].isObject()) {
        const auto res22 = fromJson<PublishDiagnosticsClientCapabilities>("publishDiagnostics", obj["publishDiagnostics"]);
        if (!res22)
            return Utils::ResultError(res22.error());
        result._publishDiagnostics = *res22;
    }
    if (obj.contains("callHierarchy") && obj["callHierarchy"].isObject()) {
        const auto res23 = fromJson<CallHierarchyClientCapabilities>("callHierarchy", obj["callHierarchy"]);
        if (!res23)
            return Utils::ResultError(res23.error());
        result._callHierarchy = *res23;
    }
    if (obj.contains("semanticTokens") && obj["semanticTokens"].isObject()) {
        const auto res24 = fromJson<SemanticTokensClientCapabilities>("semanticTokens", obj["semanticTokens"]);
        if (!res24)
            return Utils::ResultError(res24.error());
        result._semanticTokens = *res24;
    }
    if (obj.contains("linkedEditingRange") && obj["linkedEditingRange"].isObject()) {
        const auto res25 = fromJson<LinkedEditingRangeClientCapabilities>("linkedEditingRange", obj["linkedEditingRange"]);
        if (!res25)
            return Utils::ResultError(res25.error());
        result._linkedEditingRange = *res25;
    }
    if (obj.contains("moniker") && obj["moniker"].isObject()) {
        const auto res26 = fromJson<MonikerClientCapabilities>("moniker", obj["moniker"]);
        if (!res26)
            return Utils::ResultError(res26.error());
        result._moniker = *res26;
    }
    if (obj.contains("typeHierarchy") && obj["typeHierarchy"].isObject()) {
        const auto res27 = fromJson<TypeHierarchyClientCapabilities>("typeHierarchy", obj["typeHierarchy"]);
        if (!res27)
            return Utils::ResultError(res27.error());
        result._typeHierarchy = *res27;
    }
    if (obj.contains("inlineValue") && obj["inlineValue"].isObject()) {
        const auto res28 = fromJson<InlineValueClientCapabilities>("inlineValue", obj["inlineValue"]);
        if (!res28)
            return Utils::ResultError(res28.error());
        result._inlineValue = *res28;
    }
    if (obj.contains("inlayHint") && obj["inlayHint"].isObject()) {
        const auto res29 = fromJson<InlayHintClientCapabilities>("inlayHint", obj["inlayHint"]);
        if (!res29)
            return Utils::ResultError(res29.error());
        result._inlayHint = *res29;
    }
    if (obj.contains("diagnostic") && obj["diagnostic"].isObject()) {
        const auto res30 = fromJson<DiagnosticClientCapabilities>("diagnostic", obj["diagnostic"]);
        if (!res30)
            return Utils::ResultError(res30.error());
        result._diagnostic = *res30;
    }
    if (obj.contains("inlineCompletion") && obj["inlineCompletion"].isObject()) {
        const auto res31 = fromJson<InlineCompletionClientCapabilities>("inlineCompletion", obj["inlineCompletion"]);
        if (!res31)
            return Utils::ResultError(res31.error());
        result._inlineCompletion = *res31;
    }
    return result;
}

QJsonObject toJson(const TextDocumentClientCapabilities &data)
{
    QJsonObject obj;
    if (data._synchronization.has_value())
        obj.insert("synchronization", toJson(*data._synchronization));
    if (data._filters.has_value())
        obj.insert("filters", toJson(*data._filters));
    if (data._completion.has_value())
        obj.insert("completion", toJson(*data._completion));
    if (data._hover.has_value())
        obj.insert("hover", toJson(*data._hover));
    if (data._signatureHelp.has_value())
        obj.insert("signatureHelp", toJson(*data._signatureHelp));
    if (data._declaration.has_value())
        obj.insert("declaration", toJson(*data._declaration));
    if (data._definition.has_value())
        obj.insert("definition", toJson(*data._definition));
    if (data._typeDefinition.has_value())
        obj.insert("typeDefinition", toJson(*data._typeDefinition));
    if (data._implementation.has_value())
        obj.insert("implementation", toJson(*data._implementation));
    if (data._references.has_value())
        obj.insert("references", toJson(*data._references));
    if (data._documentHighlight.has_value())
        obj.insert("documentHighlight", toJson(*data._documentHighlight));
    if (data._documentSymbol.has_value())
        obj.insert("documentSymbol", toJson(*data._documentSymbol));
    if (data._codeAction.has_value())
        obj.insert("codeAction", toJson(*data._codeAction));
    if (data._codeLens.has_value())
        obj.insert("codeLens", toJson(*data._codeLens));
    if (data._documentLink.has_value())
        obj.insert("documentLink", toJson(*data._documentLink));
    if (data._colorProvider.has_value())
        obj.insert("colorProvider", toJson(*data._colorProvider));
    if (data._formatting.has_value())
        obj.insert("formatting", toJson(*data._formatting));
    if (data._rangeFormatting.has_value())
        obj.insert("rangeFormatting", toJson(*data._rangeFormatting));
    if (data._onTypeFormatting.has_value())
        obj.insert("onTypeFormatting", toJson(*data._onTypeFormatting));
    if (data._rename.has_value())
        obj.insert("rename", toJson(*data._rename));
    if (data._foldingRange.has_value())
        obj.insert("foldingRange", toJson(*data._foldingRange));
    if (data._selectionRange.has_value())
        obj.insert("selectionRange", toJson(*data._selectionRange));
    if (data._publishDiagnostics.has_value())
        obj.insert("publishDiagnostics", toJson(*data._publishDiagnostics));
    if (data._callHierarchy.has_value())
        obj.insert("callHierarchy", toJson(*data._callHierarchy));
    if (data._semanticTokens.has_value())
        obj.insert("semanticTokens", toJson(*data._semanticTokens));
    if (data._linkedEditingRange.has_value())
        obj.insert("linkedEditingRange", toJson(*data._linkedEditingRange));
    if (data._moniker.has_value())
        obj.insert("moniker", toJson(*data._moniker));
    if (data._typeHierarchy.has_value())
        obj.insert("typeHierarchy", toJson(*data._typeHierarchy));
    if (data._inlineValue.has_value())
        obj.insert("inlineValue", toJson(*data._inlineValue));
    if (data._inlayHint.has_value())
        obj.insert("inlayHint", toJson(*data._inlayHint));
    if (data._diagnostic.has_value())
        obj.insert("diagnostic", toJson(*data._diagnostic));
    if (data._inlineCompletion.has_value())
        obj.insert("inlineCompletion", toJson(*data._inlineCompletion));
    return obj;
}

template<>
Utils::Result<ShowDocumentClientCapabilities> fromJson<ShowDocumentClientCapabilities>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for ShowDocumentClientCapabilities");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("support"))
        return Utils::ResultError("Missing required field: support");
    ShowDocumentClientCapabilities result;
    result._support = obj.value("support").toBool();
    return result;
}

QJsonObject toJson(const ShowDocumentClientCapabilities &data)
{
    QJsonObject obj{{"support", data._support}};
    return obj;
}

template<>
Utils::Result<ClientShowMessageActionItemOptions> fromJson<ClientShowMessageActionItemOptions>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for ClientShowMessageActionItemOptions");
    const QJsonObject obj = val.toObject();
    ClientShowMessageActionItemOptions result;
    if (obj.contains("additionalPropertiesSupport"))
        result._additionalPropertiesSupport = obj.value("additionalPropertiesSupport").toBool();
    return result;
}

QJsonObject toJson(const ClientShowMessageActionItemOptions &data)
{
    QJsonObject obj;
    if (data._additionalPropertiesSupport.has_value())
        obj.insert("additionalPropertiesSupport", *data._additionalPropertiesSupport);
    return obj;
}

template<>
Utils::Result<ShowMessageRequestClientCapabilities> fromJson<ShowMessageRequestClientCapabilities>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for ShowMessageRequestClientCapabilities");
    const QJsonObject obj = val.toObject();
    ShowMessageRequestClientCapabilities result;
    if (obj.contains("messageActionItem") && obj["messageActionItem"].isObject()) {
        const auto res0 = fromJson<ClientShowMessageActionItemOptions>("messageActionItem", obj["messageActionItem"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._messageActionItem = *res0;
    }
    return result;
}

QJsonObject toJson(const ShowMessageRequestClientCapabilities &data)
{
    QJsonObject obj;
    if (data._messageActionItem.has_value())
        obj.insert("messageActionItem", toJson(*data._messageActionItem));
    return obj;
}

template<>
Utils::Result<WindowClientCapabilities> fromJson<WindowClientCapabilities>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for WindowClientCapabilities");
    const QJsonObject obj = val.toObject();
    WindowClientCapabilities result;
    if (obj.contains("workDoneProgress"))
        result._workDoneProgress = obj.value("workDoneProgress").toBool();
    if (obj.contains("showMessage") && obj["showMessage"].isObject()) {
        const auto res0 = fromJson<ShowMessageRequestClientCapabilities>("showMessage", obj["showMessage"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._showMessage = *res0;
    }
    if (obj.contains("showDocument") && obj["showDocument"].isObject()) {
        const auto res1 = fromJson<ShowDocumentClientCapabilities>("showDocument", obj["showDocument"]);
        if (!res1)
            return Utils::ResultError(res1.error());
        result._showDocument = *res1;
    }
    return result;
}

QJsonObject toJson(const WindowClientCapabilities &data)
{
    QJsonObject obj;
    if (data._workDoneProgress.has_value())
        obj.insert("workDoneProgress", *data._workDoneProgress);
    if (data._showMessage.has_value())
        obj.insert("showMessage", toJson(*data._showMessage));
    if (data._showDocument.has_value())
        obj.insert("showDocument", toJson(*data._showDocument));
    return obj;
}

template<>
Utils::Result<CodeLensWorkspaceClientCapabilities> fromJson<CodeLensWorkspaceClientCapabilities>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for CodeLensWorkspaceClientCapabilities");
    const QJsonObject obj = val.toObject();
    CodeLensWorkspaceClientCapabilities result;
    if (obj.contains("refreshSupport"))
        result._refreshSupport = obj.value("refreshSupport").toBool();
    return result;
}

QJsonObject toJson(const CodeLensWorkspaceClientCapabilities &data)
{
    QJsonObject obj;
    if (data._refreshSupport.has_value())
        obj.insert("refreshSupport", *data._refreshSupport);
    return obj;
}

template<>
Utils::Result<DiagnosticWorkspaceClientCapabilities> fromJson<DiagnosticWorkspaceClientCapabilities>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for DiagnosticWorkspaceClientCapabilities");
    const QJsonObject obj = val.toObject();
    DiagnosticWorkspaceClientCapabilities result;
    if (obj.contains("refreshSupport"))
        result._refreshSupport = obj.value("refreshSupport").toBool();
    return result;
}

QJsonObject toJson(const DiagnosticWorkspaceClientCapabilities &data)
{
    QJsonObject obj;
    if (data._refreshSupport.has_value())
        obj.insert("refreshSupport", *data._refreshSupport);
    return obj;
}

template<>
Utils::Result<DidChangeConfigurationClientCapabilities> fromJson<DidChangeConfigurationClientCapabilities>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for DidChangeConfigurationClientCapabilities");
    const QJsonObject obj = val.toObject();
    DidChangeConfigurationClientCapabilities result;
    if (obj.contains("dynamicRegistration"))
        result._dynamicRegistration = obj.value("dynamicRegistration").toBool();
    return result;
}

QJsonObject toJson(const DidChangeConfigurationClientCapabilities &data)
{
    QJsonObject obj;
    if (data._dynamicRegistration.has_value())
        obj.insert("dynamicRegistration", *data._dynamicRegistration);
    return obj;
}

template<>
Utils::Result<DidChangeWatchedFilesClientCapabilities> fromJson<DidChangeWatchedFilesClientCapabilities>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for DidChangeWatchedFilesClientCapabilities");
    const QJsonObject obj = val.toObject();
    DidChangeWatchedFilesClientCapabilities result;
    if (obj.contains("dynamicRegistration"))
        result._dynamicRegistration = obj.value("dynamicRegistration").toBool();
    if (obj.contains("relativePatternSupport"))
        result._relativePatternSupport = obj.value("relativePatternSupport").toBool();
    return result;
}

QJsonObject toJson(const DidChangeWatchedFilesClientCapabilities &data)
{
    QJsonObject obj;
    if (data._dynamicRegistration.has_value())
        obj.insert("dynamicRegistration", *data._dynamicRegistration);
    if (data._relativePatternSupport.has_value())
        obj.insert("relativePatternSupport", *data._relativePatternSupport);
    return obj;
}

template<>
Utils::Result<ExecuteCommandClientCapabilities> fromJson<ExecuteCommandClientCapabilities>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for ExecuteCommandClientCapabilities");
    const QJsonObject obj = val.toObject();
    ExecuteCommandClientCapabilities result;
    if (obj.contains("dynamicRegistration"))
        result._dynamicRegistration = obj.value("dynamicRegistration").toBool();
    return result;
}

QJsonObject toJson(const ExecuteCommandClientCapabilities &data)
{
    QJsonObject obj;
    if (data._dynamicRegistration.has_value())
        obj.insert("dynamicRegistration", *data._dynamicRegistration);
    return obj;
}

template<>
Utils::Result<FileOperationClientCapabilities> fromJson<FileOperationClientCapabilities>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for FileOperationClientCapabilities");
    const QJsonObject obj = val.toObject();
    FileOperationClientCapabilities result;
    if (obj.contains("dynamicRegistration"))
        result._dynamicRegistration = obj.value("dynamicRegistration").toBool();
    if (obj.contains("didCreate"))
        result._didCreate = obj.value("didCreate").toBool();
    if (obj.contains("willCreate"))
        result._willCreate = obj.value("willCreate").toBool();
    if (obj.contains("didRename"))
        result._didRename = obj.value("didRename").toBool();
    if (obj.contains("willRename"))
        result._willRename = obj.value("willRename").toBool();
    if (obj.contains("didDelete"))
        result._didDelete = obj.value("didDelete").toBool();
    if (obj.contains("willDelete"))
        result._willDelete = obj.value("willDelete").toBool();
    return result;
}

QJsonObject toJson(const FileOperationClientCapabilities &data)
{
    QJsonObject obj;
    if (data._dynamicRegistration.has_value())
        obj.insert("dynamicRegistration", *data._dynamicRegistration);
    if (data._didCreate.has_value())
        obj.insert("didCreate", *data._didCreate);
    if (data._willCreate.has_value())
        obj.insert("willCreate", *data._willCreate);
    if (data._didRename.has_value())
        obj.insert("didRename", *data._didRename);
    if (data._willRename.has_value())
        obj.insert("willRename", *data._willRename);
    if (data._didDelete.has_value())
        obj.insert("didDelete", *data._didDelete);
    if (data._willDelete.has_value())
        obj.insert("willDelete", *data._willDelete);
    return obj;
}

template<>
Utils::Result<FoldingRangeWorkspaceClientCapabilities> fromJson<FoldingRangeWorkspaceClientCapabilities>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for FoldingRangeWorkspaceClientCapabilities");
    const QJsonObject obj = val.toObject();
    FoldingRangeWorkspaceClientCapabilities result;
    if (obj.contains("refreshSupport"))
        result._refreshSupport = obj.value("refreshSupport").toBool();
    return result;
}

QJsonObject toJson(const FoldingRangeWorkspaceClientCapabilities &data)
{
    QJsonObject obj;
    if (data._refreshSupport.has_value())
        obj.insert("refreshSupport", *data._refreshSupport);
    return obj;
}

template<>
Utils::Result<InlayHintWorkspaceClientCapabilities> fromJson<InlayHintWorkspaceClientCapabilities>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for InlayHintWorkspaceClientCapabilities");
    const QJsonObject obj = val.toObject();
    InlayHintWorkspaceClientCapabilities result;
    if (obj.contains("refreshSupport"))
        result._refreshSupport = obj.value("refreshSupport").toBool();
    return result;
}

QJsonObject toJson(const InlayHintWorkspaceClientCapabilities &data)
{
    QJsonObject obj;
    if (data._refreshSupport.has_value())
        obj.insert("refreshSupport", *data._refreshSupport);
    return obj;
}

template<>
Utils::Result<InlineValueWorkspaceClientCapabilities> fromJson<InlineValueWorkspaceClientCapabilities>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for InlineValueWorkspaceClientCapabilities");
    const QJsonObject obj = val.toObject();
    InlineValueWorkspaceClientCapabilities result;
    if (obj.contains("refreshSupport"))
        result._refreshSupport = obj.value("refreshSupport").toBool();
    return result;
}

QJsonObject toJson(const InlineValueWorkspaceClientCapabilities &data)
{
    QJsonObject obj;
    if (data._refreshSupport.has_value())
        obj.insert("refreshSupport", *data._refreshSupport);
    return obj;
}

template<>
Utils::Result<SemanticTokensWorkspaceClientCapabilities> fromJson<SemanticTokensWorkspaceClientCapabilities>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for SemanticTokensWorkspaceClientCapabilities");
    const QJsonObject obj = val.toObject();
    SemanticTokensWorkspaceClientCapabilities result;
    if (obj.contains("refreshSupport"))
        result._refreshSupport = obj.value("refreshSupport").toBool();
    return result;
}

QJsonObject toJson(const SemanticTokensWorkspaceClientCapabilities &data)
{
    QJsonObject obj;
    if (data._refreshSupport.has_value())
        obj.insert("refreshSupport", *data._refreshSupport);
    return obj;
}

template<>
Utils::Result<TextDocumentContentClientCapabilities> fromJson<TextDocumentContentClientCapabilities>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for TextDocumentContentClientCapabilities");
    const QJsonObject obj = val.toObject();
    TextDocumentContentClientCapabilities result;
    if (obj.contains("dynamicRegistration"))
        result._dynamicRegistration = obj.value("dynamicRegistration").toBool();
    return result;
}

QJsonObject toJson(const TextDocumentContentClientCapabilities &data)
{
    QJsonObject obj;
    if (data._dynamicRegistration.has_value())
        obj.insert("dynamicRegistration", *data._dynamicRegistration);
    return obj;
}

template<>
Utils::Result<ChangeAnnotationsSupportOptions> fromJson<ChangeAnnotationsSupportOptions>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for ChangeAnnotationsSupportOptions");
    const QJsonObject obj = val.toObject();
    ChangeAnnotationsSupportOptions result;
    if (obj.contains("groupsOnLabel"))
        result._groupsOnLabel = obj.value("groupsOnLabel").toBool();
    return result;
}

QJsonObject toJson(const ChangeAnnotationsSupportOptions &data)
{
    QJsonObject obj;
    if (data._groupsOnLabel.has_value())
        obj.insert("groupsOnLabel", *data._groupsOnLabel);
    return obj;
}

QString toString(FailureHandlingKind v)
{
    switch(v) {
        case FailureHandlingKind::abort: return "abort";
        case FailureHandlingKind::transactional: return "transactional";
        case FailureHandlingKind::textOnlyTransactional: return "textOnlyTransactional";
        case FailureHandlingKind::undo: return "undo";
    }
    return {};
}

template<>
Utils::Result<FailureHandlingKind> fromJson<FailureHandlingKind>(const QJsonValue &val)
{
    if (!val.isString())
        return Utils::ResultError("Expected JSON string for FailureHandlingKind");
    const QString str = val.toString();
    if (str == "abort") return FailureHandlingKind::abort;
    if (str == "transactional") return FailureHandlingKind::transactional;
    if (str == "textOnlyTransactional") return FailureHandlingKind::textOnlyTransactional;
    if (str == "undo") return FailureHandlingKind::undo;
    return Utils::ResultError("Invalid FailureHandlingKind value: " + str);
}

QJsonValue toJsonValue(const FailureHandlingKind &v)
{
    return toString(v);
}

QString toString(ResourceOperationKind v)
{
    switch(v) {
        case ResourceOperationKind::create: return "create";
        case ResourceOperationKind::rename: return "rename";
        case ResourceOperationKind::delete_: return "delete";
    }
    return {};
}

template<>
Utils::Result<ResourceOperationKind> fromJson<ResourceOperationKind>(const QJsonValue &val)
{
    if (!val.isString())
        return Utils::ResultError("Expected JSON string for ResourceOperationKind");
    const QString str = val.toString();
    if (str == "create") return ResourceOperationKind::create;
    if (str == "rename") return ResourceOperationKind::rename;
    if (str == "delete") return ResourceOperationKind::delete_;
    return Utils::ResultError("Invalid ResourceOperationKind value: " + str);
}

QJsonValue toJsonValue(const ResourceOperationKind &v)
{
    return toString(v);
}

template<>
Utils::Result<WorkspaceEditClientCapabilities> fromJson<WorkspaceEditClientCapabilities>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for WorkspaceEditClientCapabilities");
    const QJsonObject obj = val.toObject();
    WorkspaceEditClientCapabilities result;
    if (obj.contains("documentChanges"))
        result._documentChanges = obj.value("documentChanges").toBool();
    if (obj.contains("resourceOperations") && obj["resourceOperations"].isArray()) {
        const QJsonArray arr = obj["resourceOperations"].toArray();
        QList<ResourceOperationKind> list_resourceOperations;
        for (const QJsonValue &v : arr) {
            const auto res0 = fromJson<ResourceOperationKind>("resourceOperations", v);
            if (!res0)
                return Utils::ResultError(res0.error());
            list_resourceOperations.append(*res0);
        }
        result._resourceOperations = list_resourceOperations;
    }
    if (obj.contains("failureHandling")) {
        const auto res1 = fromJson<FailureHandlingKind>("failureHandling", obj["failureHandling"]);
        if (!res1)
            return Utils::ResultError(res1.error());
        result._failureHandling = *res1;
    }
    if (obj.contains("normalizesLineEndings"))
        result._normalizesLineEndings = obj.value("normalizesLineEndings").toBool();
    if (obj.contains("changeAnnotationSupport") && obj["changeAnnotationSupport"].isObject()) {
        const auto res2 = fromJson<ChangeAnnotationsSupportOptions>("changeAnnotationSupport", obj["changeAnnotationSupport"]);
        if (!res2)
            return Utils::ResultError(res2.error());
        result._changeAnnotationSupport = *res2;
    }
    if (obj.contains("metadataSupport"))
        result._metadataSupport = obj.value("metadataSupport").toBool();
    if (obj.contains("snippetEditSupport"))
        result._snippetEditSupport = obj.value("snippetEditSupport").toBool();
    return result;
}

QJsonObject toJson(const WorkspaceEditClientCapabilities &data)
{
    QJsonObject obj;
    if (data._documentChanges.has_value())
        obj.insert("documentChanges", *data._documentChanges);
    if (data._resourceOperations.has_value()) {
        QJsonArray arr_resourceOperations;
        for (const auto &v : *data._resourceOperations) arr_resourceOperations.append(toJsonValue(v));
        obj.insert("resourceOperations", arr_resourceOperations);
    }
    if (data._failureHandling.has_value())
        obj.insert("failureHandling", toJsonValue(*data._failureHandling));
    if (data._normalizesLineEndings.has_value())
        obj.insert("normalizesLineEndings", *data._normalizesLineEndings);
    if (data._changeAnnotationSupport.has_value())
        obj.insert("changeAnnotationSupport", toJson(*data._changeAnnotationSupport));
    if (data._metadataSupport.has_value())
        obj.insert("metadataSupport", *data._metadataSupport);
    if (data._snippetEditSupport.has_value())
        obj.insert("snippetEditSupport", *data._snippetEditSupport);
    return obj;
}

template<>
Utils::Result<ClientSymbolResolveOptions> fromJson<ClientSymbolResolveOptions>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for ClientSymbolResolveOptions");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("properties"))
        return Utils::ResultError("Missing required field: properties");
    ClientSymbolResolveOptions result;
    if (obj.contains("properties") && obj["properties"].isArray()) {
        const QJsonArray arr = obj["properties"].toArray();
        for (const QJsonValue &v : arr) {
            result._properties.append(v.toString());
        }
    }
    return result;
}

QJsonObject toJson(const ClientSymbolResolveOptions &data)
{
    QJsonObject obj;
    QJsonArray arr_properties;
    for (const auto &v : data._properties) arr_properties.append(v);
    obj.insert("properties", arr_properties);
    return obj;
}

template<>
Utils::Result<WorkspaceSymbolClientCapabilities> fromJson<WorkspaceSymbolClientCapabilities>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for WorkspaceSymbolClientCapabilities");
    const QJsonObject obj = val.toObject();
    WorkspaceSymbolClientCapabilities result;
    if (obj.contains("dynamicRegistration"))
        result._dynamicRegistration = obj.value("dynamicRegistration").toBool();
    if (obj.contains("symbolKind") && obj["symbolKind"].isObject()) {
        const auto res0 = fromJson<ClientSymbolKindOptions>("symbolKind", obj["symbolKind"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._symbolKind = *res0;
    }
    if (obj.contains("tagSupport") && obj["tagSupport"].isObject()) {
        const auto res1 = fromJson<ClientSymbolTagOptions>("tagSupport", obj["tagSupport"]);
        if (!res1)
            return Utils::ResultError(res1.error());
        result._tagSupport = *res1;
    }
    if (obj.contains("resolveSupport") && obj["resolveSupport"].isObject()) {
        const auto res2 = fromJson<ClientSymbolResolveOptions>("resolveSupport", obj["resolveSupport"]);
        if (!res2)
            return Utils::ResultError(res2.error());
        result._resolveSupport = *res2;
    }
    return result;
}

QJsonObject toJson(const WorkspaceSymbolClientCapabilities &data)
{
    QJsonObject obj;
    if (data._dynamicRegistration.has_value())
        obj.insert("dynamicRegistration", *data._dynamicRegistration);
    if (data._symbolKind.has_value())
        obj.insert("symbolKind", toJson(*data._symbolKind));
    if (data._tagSupport.has_value())
        obj.insert("tagSupport", toJson(*data._tagSupport));
    if (data._resolveSupport.has_value())
        obj.insert("resolveSupport", toJson(*data._resolveSupport));
    return obj;
}

template<>
Utils::Result<WorkspaceClientCapabilities> fromJson<WorkspaceClientCapabilities>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for WorkspaceClientCapabilities");
    const QJsonObject obj = val.toObject();
    WorkspaceClientCapabilities result;
    if (obj.contains("applyEdit"))
        result._applyEdit = obj.value("applyEdit").toBool();
    if (obj.contains("workspaceEdit") && obj["workspaceEdit"].isObject()) {
        const auto res0 = fromJson<WorkspaceEditClientCapabilities>("workspaceEdit", obj["workspaceEdit"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._workspaceEdit = *res0;
    }
    if (obj.contains("didChangeConfiguration") && obj["didChangeConfiguration"].isObject()) {
        const auto res1 = fromJson<DidChangeConfigurationClientCapabilities>("didChangeConfiguration", obj["didChangeConfiguration"]);
        if (!res1)
            return Utils::ResultError(res1.error());
        result._didChangeConfiguration = *res1;
    }
    if (obj.contains("didChangeWatchedFiles") && obj["didChangeWatchedFiles"].isObject()) {
        const auto res2 = fromJson<DidChangeWatchedFilesClientCapabilities>("didChangeWatchedFiles", obj["didChangeWatchedFiles"]);
        if (!res2)
            return Utils::ResultError(res2.error());
        result._didChangeWatchedFiles = *res2;
    }
    if (obj.contains("symbol") && obj["symbol"].isObject()) {
        const auto res3 = fromJson<WorkspaceSymbolClientCapabilities>("symbol", obj["symbol"]);
        if (!res3)
            return Utils::ResultError(res3.error());
        result._symbol = *res3;
    }
    if (obj.contains("executeCommand") && obj["executeCommand"].isObject()) {
        const auto res4 = fromJson<ExecuteCommandClientCapabilities>("executeCommand", obj["executeCommand"]);
        if (!res4)
            return Utils::ResultError(res4.error());
        result._executeCommand = *res4;
    }
    if (obj.contains("workspaceFolders"))
        result._workspaceFolders = obj.value("workspaceFolders").toBool();
    if (obj.contains("configuration"))
        result._configuration = obj.value("configuration").toBool();
    if (obj.contains("semanticTokens") && obj["semanticTokens"].isObject()) {
        const auto res5 = fromJson<SemanticTokensWorkspaceClientCapabilities>("semanticTokens", obj["semanticTokens"]);
        if (!res5)
            return Utils::ResultError(res5.error());
        result._semanticTokens = *res5;
    }
    if (obj.contains("codeLens") && obj["codeLens"].isObject()) {
        const auto res6 = fromJson<CodeLensWorkspaceClientCapabilities>("codeLens", obj["codeLens"]);
        if (!res6)
            return Utils::ResultError(res6.error());
        result._codeLens = *res6;
    }
    if (obj.contains("fileOperations") && obj["fileOperations"].isObject()) {
        const auto res7 = fromJson<FileOperationClientCapabilities>("fileOperations", obj["fileOperations"]);
        if (!res7)
            return Utils::ResultError(res7.error());
        result._fileOperations = *res7;
    }
    if (obj.contains("inlineValue") && obj["inlineValue"].isObject()) {
        const auto res8 = fromJson<InlineValueWorkspaceClientCapabilities>("inlineValue", obj["inlineValue"]);
        if (!res8)
            return Utils::ResultError(res8.error());
        result._inlineValue = *res8;
    }
    if (obj.contains("inlayHint") && obj["inlayHint"].isObject()) {
        const auto res9 = fromJson<InlayHintWorkspaceClientCapabilities>("inlayHint", obj["inlayHint"]);
        if (!res9)
            return Utils::ResultError(res9.error());
        result._inlayHint = *res9;
    }
    if (obj.contains("diagnostics") && obj["diagnostics"].isObject()) {
        const auto res10 = fromJson<DiagnosticWorkspaceClientCapabilities>("diagnostics", obj["diagnostics"]);
        if (!res10)
            return Utils::ResultError(res10.error());
        result._diagnostics = *res10;
    }
    if (obj.contains("foldingRange") && obj["foldingRange"].isObject()) {
        const auto res11 = fromJson<FoldingRangeWorkspaceClientCapabilities>("foldingRange", obj["foldingRange"]);
        if (!res11)
            return Utils::ResultError(res11.error());
        result._foldingRange = *res11;
    }
    if (obj.contains("textDocumentContent") && obj["textDocumentContent"].isObject()) {
        const auto res12 = fromJson<TextDocumentContentClientCapabilities>("textDocumentContent", obj["textDocumentContent"]);
        if (!res12)
            return Utils::ResultError(res12.error());
        result._textDocumentContent = *res12;
    }
    return result;
}

QJsonObject toJson(const WorkspaceClientCapabilities &data)
{
    QJsonObject obj;
    if (data._applyEdit.has_value())
        obj.insert("applyEdit", *data._applyEdit);
    if (data._workspaceEdit.has_value())
        obj.insert("workspaceEdit", toJson(*data._workspaceEdit));
    if (data._didChangeConfiguration.has_value())
        obj.insert("didChangeConfiguration", toJson(*data._didChangeConfiguration));
    if (data._didChangeWatchedFiles.has_value())
        obj.insert("didChangeWatchedFiles", toJson(*data._didChangeWatchedFiles));
    if (data._symbol.has_value())
        obj.insert("symbol", toJson(*data._symbol));
    if (data._executeCommand.has_value())
        obj.insert("executeCommand", toJson(*data._executeCommand));
    if (data._workspaceFolders.has_value())
        obj.insert("workspaceFolders", *data._workspaceFolders);
    if (data._configuration.has_value())
        obj.insert("configuration", *data._configuration);
    if (data._semanticTokens.has_value())
        obj.insert("semanticTokens", toJson(*data._semanticTokens));
    if (data._codeLens.has_value())
        obj.insert("codeLens", toJson(*data._codeLens));
    if (data._fileOperations.has_value())
        obj.insert("fileOperations", toJson(*data._fileOperations));
    if (data._inlineValue.has_value())
        obj.insert("inlineValue", toJson(*data._inlineValue));
    if (data._inlayHint.has_value())
        obj.insert("inlayHint", toJson(*data._inlayHint));
    if (data._diagnostics.has_value())
        obj.insert("diagnostics", toJson(*data._diagnostics));
    if (data._foldingRange.has_value())
        obj.insert("foldingRange", toJson(*data._foldingRange));
    if (data._textDocumentContent.has_value())
        obj.insert("textDocumentContent", toJson(*data._textDocumentContent));
    return obj;
}

template<>
Utils::Result<ClientCapabilities> fromJson<ClientCapabilities>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for ClientCapabilities");
    const QJsonObject obj = val.toObject();
    ClientCapabilities result;
    if (obj.contains("workspace") && obj["workspace"].isObject()) {
        const auto res0 = fromJson<WorkspaceClientCapabilities>("workspace", obj["workspace"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._workspace = *res0;
    }
    if (obj.contains("textDocument") && obj["textDocument"].isObject()) {
        const auto res1 = fromJson<TextDocumentClientCapabilities>("textDocument", obj["textDocument"]);
        if (!res1)
            return Utils::ResultError(res1.error());
        result._textDocument = *res1;
    }
    if (obj.contains("notebookDocument") && obj["notebookDocument"].isObject()) {
        const auto res2 = fromJson<NotebookDocumentClientCapabilities>("notebookDocument", obj["notebookDocument"]);
        if (!res2)
            return Utils::ResultError(res2.error());
        result._notebookDocument = *res2;
    }
    if (obj.contains("window") && obj["window"].isObject()) {
        const auto res3 = fromJson<WindowClientCapabilities>("window", obj["window"]);
        if (!res3)
            return Utils::ResultError(res3.error());
        result._window = *res3;
    }
    if (obj.contains("general") && obj["general"].isObject()) {
        const auto res4 = fromJson<GeneralClientCapabilities>("general", obj["general"]);
        if (!res4)
            return Utils::ResultError(res4.error());
        result._general = *res4;
    }
    if (obj.contains("experimental"))
        result._experimental = obj.value("experimental");
    return result;
}

QJsonObject toJson(const ClientCapabilities &data)
{
    QJsonObject obj;
    if (data._workspace.has_value())
        obj.insert("workspace", toJson(*data._workspace));
    if (data._textDocument.has_value())
        obj.insert("textDocument", toJson(*data._textDocument));
    if (data._notebookDocument.has_value())
        obj.insert("notebookDocument", toJson(*data._notebookDocument));
    if (data._window.has_value())
        obj.insert("window", toJson(*data._window));
    if (data._general.has_value())
        obj.insert("general", toJson(*data._general));
    if (data._experimental.has_value())
        obj.insert("experimental", *data._experimental);
    return obj;
}

template<>
Utils::Result<ClientInfo> fromJson<ClientInfo>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for ClientInfo");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("name"))
        return Utils::ResultError("Missing required field: name");
    ClientInfo result;
    result._name = obj.value("name").toString();
    if (obj.contains("version"))
        result._version = obj.value("version").toString();
    return result;
}

QJsonObject toJson(const ClientInfo &data)
{
    QJsonObject obj{{"name", data._name}};
    if (data._version.has_value())
        obj.insert("version", *data._version);
    return obj;
}

QString toString(TraceValue v)
{
    switch(v) {
        case TraceValue::off: return "off";
        case TraceValue::messages: return "messages";
        case TraceValue::verbose: return "verbose";
    }
    return {};
}

template<>
Utils::Result<TraceValue> fromJson<TraceValue>(const QJsonValue &val)
{
    if (!val.isString())
        return Utils::ResultError("Expected JSON string for TraceValue");
    const QString str = val.toString();
    if (str == "off") return TraceValue::off;
    if (str == "messages") return TraceValue::messages;
    if (str == "verbose") return TraceValue::verbose;
    return Utils::ResultError("Invalid TraceValue value: " + str);
}

QJsonValue toJsonValue(const TraceValue &v)
{
    return toString(v);
}

template<>
Utils::Result<InitializeParamsWorkspaceFolders> fromJson<InitializeParamsWorkspaceFolders>(const QJsonValue &val)
{
    if (val.isNull())
        return InitializeParamsWorkspaceFolders(std::monostate{});
    if (val.isArray()) {
        QList<WorkspaceFolder> list;
        for (const QJsonValue &v : val.toArray()) {
            const auto res0 = fromJson<WorkspaceFolder>(v);
            if (!res0)
                return Utils::ResultError(res0.error());
            list.append(*res0);
        }
        return InitializeParamsWorkspaceFolders(std::move(list));
    }
    return Utils::ResultError("Invalid InitializeParamsWorkspaceFolders");
}

QJsonValue toJsonValue(const InitializeParamsWorkspaceFolders &val)
{
    return std::visit([](const auto &v) -> QJsonValue {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, QList<WorkspaceFolder>>) {
            QJsonArray arr;
            for (const auto &elem : v)
                arr.append(toJson(elem));
            return arr;
        } else
        if constexpr (std::is_same_v<T, std::monostate>) {
            return QJsonValue(QJsonValue::Null);
        } else
        {
            return QVariant::fromValue(v).toJsonValue();
        }
    }, val);
}

template<>
Utils::Result<InitializeParams> fromJson<InitializeParams>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for InitializeParams");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("processId"))
        return Utils::ResultError("Missing required field: processId");
    if (!obj.contains("rootUri"))
        return Utils::ResultError("Missing required field: rootUri");
    if (!obj.contains("capabilities"))
        return Utils::ResultError("Missing required field: capabilities");
    InitializeParams result;
    if (obj.contains("workDoneToken")) {
        const auto res0 = fromJson<ProgressToken>("workDoneToken", obj["workDoneToken"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._workDoneToken = *res0;
    }
    if (!obj["processId"].isNull()) {
        result._processId = obj.value("processId").toInt();
    }
    if (obj.contains("clientInfo") && obj["clientInfo"].isObject()) {
        const auto res1 = fromJson<ClientInfo>("clientInfo", obj["clientInfo"]);
        if (!res1)
            return Utils::ResultError(res1.error());
        result._clientInfo = *res1;
    }
    if (obj.contains("locale"))
        result._locale = obj.value("locale").toString();
    if (obj.contains("rootPath"))
        if (!obj["rootPath"].isNull()) {
            result._rootPath = obj.value("rootPath").toString();
        }
    if (!obj["rootUri"].isNull()) {
        result._rootUri = obj.value("rootUri").toString();
    }
    if (obj.contains("capabilities") && obj["capabilities"].isObject()) {
        const auto res2 = fromJson<ClientCapabilities>("capabilities", obj["capabilities"]);
        if (!res2)
            return Utils::ResultError(res2.error());
        result._capabilities = *res2;
    }
    if (obj.contains("initializationOptions"))
        result._initializationOptions = obj.value("initializationOptions");
    if (obj.contains("trace")) {
        const auto res3 = fromJson<TraceValue>("trace", obj["trace"]);
        if (!res3)
            return Utils::ResultError(res3.error());
        result._trace = *res3;
    }
    if (obj.contains("workspaceFolders")) {
        const auto res4 = fromJson<InitializeParamsWorkspaceFolders>("workspaceFolders", obj["workspaceFolders"]);
        if (!res4)
            return Utils::ResultError(res4.error());
        result._workspaceFolders = *res4;
    }
    return result;
}

QJsonObject toJson(const InitializeParams &data)
{
    QJsonObject obj{{"capabilities", toJson(data._capabilities)}};
    if (data._workDoneToken.has_value())
        obj.insert("workDoneToken", toJsonValue(*data._workDoneToken));
    if (data._processId.has_value())
        obj.insert("processId", *data._processId);
    else
        obj.insert("processId", QJsonValue::Null);
    if (data._clientInfo.has_value())
        obj.insert("clientInfo", toJson(*data._clientInfo));
    if (data._locale.has_value())
        obj.insert("locale", *data._locale);
    if (data._rootPath.has_value())
        obj.insert("rootPath", *data._rootPath);
    if (data._rootUri.has_value())
        obj.insert("rootUri", *data._rootUri);
    else
        obj.insert("rootUri", QJsonValue::Null);
    if (data._initializationOptions.has_value())
        obj.insert("initializationOptions", *data._initializationOptions);
    if (data._trace.has_value())
        obj.insert("trace", toJsonValue(*data._trace));
    if (data._workspaceFolders.has_value())
        obj.insert("workspaceFolders", toJsonValue(*data._workspaceFolders));
    return obj;
}

template<>
Utils::Result<CallHierarchyOptions> fromJson<CallHierarchyOptions>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for CallHierarchyOptions");
    const QJsonObject obj = val.toObject();
    CallHierarchyOptions result;
    if (obj.contains("workDoneProgress"))
        result._workDoneProgress = obj.value("workDoneProgress").toBool();
    return result;
}

QJsonObject toJson(const CallHierarchyOptions &data)
{
    QJsonObject obj;
    if (data._workDoneProgress.has_value())
        obj.insert("workDoneProgress", *data._workDoneProgress);
    return obj;
}

template<>
Utils::Result<CodeActionKindDocumentation> fromJson<CodeActionKindDocumentation>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for CodeActionKindDocumentation");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("kind"))
        return Utils::ResultError("Missing required field: kind");
    if (!obj.contains("command"))
        return Utils::ResultError("Missing required field: command");
    CodeActionKindDocumentation result;
    if (obj.contains("kind") && obj["kind"].isString()) {
        const auto res0 = fromJson<CodeActionKind>("kind", obj["kind"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._kind = *res0;
    }
    if (obj.contains("command") && obj["command"].isObject()) {
        const auto res1 = fromJson<Command>("command", obj["command"]);
        if (!res1)
            return Utils::ResultError(res1.error());
        result._command = *res1;
    }
    return result;
}

QJsonObject toJson(const CodeActionKindDocumentation &data)
{
    QJsonObject obj{
        {"kind", data._kind},
        {"command", toJson(data._command)}
    };
    return obj;
}

template<>
Utils::Result<CodeActionOptions> fromJson<CodeActionOptions>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for CodeActionOptions");
    const QJsonObject obj = val.toObject();
    CodeActionOptions result;
    if (obj.contains("workDoneProgress"))
        result._workDoneProgress = obj.value("workDoneProgress").toBool();
    if (obj.contains("codeActionKinds") && obj["codeActionKinds"].isArray()) {
        const QJsonArray arr = obj["codeActionKinds"].toArray();
        QList<CodeActionKind> list_codeActionKinds;
        for (const QJsonValue &v : arr) {
            const auto res0 = fromJson<CodeActionKind>("codeActionKinds", v);
            if (!res0)
                return Utils::ResultError(res0.error());
            list_codeActionKinds.append(*res0);
        }
        result._codeActionKinds = list_codeActionKinds;
    }
    if (obj.contains("documentation") && obj["documentation"].isArray()) {
        const QJsonArray arr = obj["documentation"].toArray();
        QList<CodeActionKindDocumentation> list_documentation;
        for (const QJsonValue &v : arr) {
            const auto res1 = fromJson<CodeActionKindDocumentation>("documentation", v);
            if (!res1)
                return Utils::ResultError(res1.error());
            list_documentation.append(*res1);
        }
        result._documentation = list_documentation;
    }
    if (obj.contains("resolveProvider"))
        result._resolveProvider = obj.value("resolveProvider").toBool();
    return result;
}

QJsonObject toJson(const CodeActionOptions &data)
{
    QJsonObject obj;
    if (data._workDoneProgress.has_value())
        obj.insert("workDoneProgress", *data._workDoneProgress);
    if (data._codeActionKinds.has_value()) {
        QJsonArray arr_codeActionKinds;
        for (const auto &v : *data._codeActionKinds) arr_codeActionKinds.append(v);
        obj.insert("codeActionKinds", arr_codeActionKinds);
    }
    if (data._documentation.has_value()) {
        QJsonArray arr_documentation;
        for (const auto &v : *data._documentation) arr_documentation.append(toJson(v));
        obj.insert("documentation", arr_documentation);
    }
    if (data._resolveProvider.has_value())
        obj.insert("resolveProvider", *data._resolveProvider);
    return obj;
}

template<>
Utils::Result<CodeLensOptions> fromJson<CodeLensOptions>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for CodeLensOptions");
    const QJsonObject obj = val.toObject();
    CodeLensOptions result;
    if (obj.contains("workDoneProgress"))
        result._workDoneProgress = obj.value("workDoneProgress").toBool();
    if (obj.contains("resolveProvider"))
        result._resolveProvider = obj.value("resolveProvider").toBool();
    return result;
}

QJsonObject toJson(const CodeLensOptions &data)
{
    QJsonObject obj;
    if (data._workDoneProgress.has_value())
        obj.insert("workDoneProgress", *data._workDoneProgress);
    if (data._resolveProvider.has_value())
        obj.insert("resolveProvider", *data._resolveProvider);
    return obj;
}

template<>
Utils::Result<ServerCompletionItemOptions> fromJson<ServerCompletionItemOptions>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for ServerCompletionItemOptions");
    const QJsonObject obj = val.toObject();
    ServerCompletionItemOptions result;
    if (obj.contains("labelDetailsSupport"))
        result._labelDetailsSupport = obj.value("labelDetailsSupport").toBool();
    return result;
}

QJsonObject toJson(const ServerCompletionItemOptions &data)
{
    QJsonObject obj;
    if (data._labelDetailsSupport.has_value())
        obj.insert("labelDetailsSupport", *data._labelDetailsSupport);
    return obj;
}

template<>
Utils::Result<CompletionOptions> fromJson<CompletionOptions>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for CompletionOptions");
    const QJsonObject obj = val.toObject();
    CompletionOptions result;
    if (obj.contains("workDoneProgress"))
        result._workDoneProgress = obj.value("workDoneProgress").toBool();
    if (obj.contains("triggerCharacters") && obj["triggerCharacters"].isArray()) {
        const QJsonArray arr = obj["triggerCharacters"].toArray();
        QStringList list_triggerCharacters;
        for (const QJsonValue &v : arr) {
            list_triggerCharacters.append(v.toString());
        }
        result._triggerCharacters = list_triggerCharacters;
    }
    if (obj.contains("allCommitCharacters") && obj["allCommitCharacters"].isArray()) {
        const QJsonArray arr = obj["allCommitCharacters"].toArray();
        QStringList list_allCommitCharacters;
        for (const QJsonValue &v : arr) {
            list_allCommitCharacters.append(v.toString());
        }
        result._allCommitCharacters = list_allCommitCharacters;
    }
    if (obj.contains("resolveProvider"))
        result._resolveProvider = obj.value("resolveProvider").toBool();
    if (obj.contains("completionItem") && obj["completionItem"].isObject()) {
        const auto res0 = fromJson<ServerCompletionItemOptions>("completionItem", obj["completionItem"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._completionItem = *res0;
    }
    return result;
}

QJsonObject toJson(const CompletionOptions &data)
{
    QJsonObject obj;
    if (data._workDoneProgress.has_value())
        obj.insert("workDoneProgress", *data._workDoneProgress);
    if (data._triggerCharacters.has_value()) {
        QJsonArray arr_triggerCharacters;
        for (const auto &v : *data._triggerCharacters) arr_triggerCharacters.append(v);
        obj.insert("triggerCharacters", arr_triggerCharacters);
    }
    if (data._allCommitCharacters.has_value()) {
        QJsonArray arr_allCommitCharacters;
        for (const auto &v : *data._allCommitCharacters) arr_allCommitCharacters.append(v);
        obj.insert("allCommitCharacters", arr_allCommitCharacters);
    }
    if (data._resolveProvider.has_value())
        obj.insert("resolveProvider", *data._resolveProvider);
    if (data._completionItem.has_value())
        obj.insert("completionItem", toJson(*data._completionItem));
    return obj;
}

template<>
Utils::Result<DeclarationOptions> fromJson<DeclarationOptions>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for DeclarationOptions");
    const QJsonObject obj = val.toObject();
    DeclarationOptions result;
    if (obj.contains("workDoneProgress"))
        result._workDoneProgress = obj.value("workDoneProgress").toBool();
    return result;
}

QJsonObject toJson(const DeclarationOptions &data)
{
    QJsonObject obj;
    if (data._workDoneProgress.has_value())
        obj.insert("workDoneProgress", *data._workDoneProgress);
    return obj;
}

template<>
Utils::Result<DefinitionOptions> fromJson<DefinitionOptions>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for DefinitionOptions");
    const QJsonObject obj = val.toObject();
    DefinitionOptions result;
    if (obj.contains("workDoneProgress"))
        result._workDoneProgress = obj.value("workDoneProgress").toBool();
    return result;
}

QJsonObject toJson(const DefinitionOptions &data)
{
    QJsonObject obj;
    if (data._workDoneProgress.has_value())
        obj.insert("workDoneProgress", *data._workDoneProgress);
    return obj;
}

template<>
Utils::Result<DiagnosticOptions> fromJson<DiagnosticOptions>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for DiagnosticOptions");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("interFileDependencies"))
        return Utils::ResultError("Missing required field: interFileDependencies");
    if (!obj.contains("workspaceDiagnostics"))
        return Utils::ResultError("Missing required field: workspaceDiagnostics");
    DiagnosticOptions result;
    if (obj.contains("workDoneProgress"))
        result._workDoneProgress = obj.value("workDoneProgress").toBool();
    if (obj.contains("identifier"))
        result._identifier = obj.value("identifier").toString();
    result._interFileDependencies = obj.value("interFileDependencies").toBool();
    result._workspaceDiagnostics = obj.value("workspaceDiagnostics").toBool();
    return result;
}

QJsonObject toJson(const DiagnosticOptions &data)
{
    QJsonObject obj{
        {"interFileDependencies", data._interFileDependencies},
        {"workspaceDiagnostics", data._workspaceDiagnostics}
    };
    if (data._workDoneProgress.has_value())
        obj.insert("workDoneProgress", *data._workDoneProgress);
    if (data._identifier.has_value())
        obj.insert("identifier", *data._identifier);
    return obj;
}

template<>
Utils::Result<DocumentColorOptions> fromJson<DocumentColorOptions>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for DocumentColorOptions");
    const QJsonObject obj = val.toObject();
    DocumentColorOptions result;
    if (obj.contains("workDoneProgress"))
        result._workDoneProgress = obj.value("workDoneProgress").toBool();
    return result;
}

QJsonObject toJson(const DocumentColorOptions &data)
{
    QJsonObject obj;
    if (data._workDoneProgress.has_value())
        obj.insert("workDoneProgress", *data._workDoneProgress);
    return obj;
}

template<>
Utils::Result<DocumentFormattingOptions> fromJson<DocumentFormattingOptions>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for DocumentFormattingOptions");
    const QJsonObject obj = val.toObject();
    DocumentFormattingOptions result;
    if (obj.contains("workDoneProgress"))
        result._workDoneProgress = obj.value("workDoneProgress").toBool();
    return result;
}

QJsonObject toJson(const DocumentFormattingOptions &data)
{
    QJsonObject obj;
    if (data._workDoneProgress.has_value())
        obj.insert("workDoneProgress", *data._workDoneProgress);
    return obj;
}

template<>
Utils::Result<DocumentHighlightOptions> fromJson<DocumentHighlightOptions>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for DocumentHighlightOptions");
    const QJsonObject obj = val.toObject();
    DocumentHighlightOptions result;
    if (obj.contains("workDoneProgress"))
        result._workDoneProgress = obj.value("workDoneProgress").toBool();
    return result;
}

QJsonObject toJson(const DocumentHighlightOptions &data)
{
    QJsonObject obj;
    if (data._workDoneProgress.has_value())
        obj.insert("workDoneProgress", *data._workDoneProgress);
    return obj;
}

template<>
Utils::Result<DocumentLinkOptions> fromJson<DocumentLinkOptions>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for DocumentLinkOptions");
    const QJsonObject obj = val.toObject();
    DocumentLinkOptions result;
    if (obj.contains("workDoneProgress"))
        result._workDoneProgress = obj.value("workDoneProgress").toBool();
    if (obj.contains("resolveProvider"))
        result._resolveProvider = obj.value("resolveProvider").toBool();
    return result;
}

QJsonObject toJson(const DocumentLinkOptions &data)
{
    QJsonObject obj;
    if (data._workDoneProgress.has_value())
        obj.insert("workDoneProgress", *data._workDoneProgress);
    if (data._resolveProvider.has_value())
        obj.insert("resolveProvider", *data._resolveProvider);
    return obj;
}

template<>
Utils::Result<DocumentOnTypeFormattingOptions> fromJson<DocumentOnTypeFormattingOptions>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for DocumentOnTypeFormattingOptions");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("firstTriggerCharacter"))
        return Utils::ResultError("Missing required field: firstTriggerCharacter");
    DocumentOnTypeFormattingOptions result;
    result._firstTriggerCharacter = obj.value("firstTriggerCharacter").toString();
    if (obj.contains("moreTriggerCharacter") && obj["moreTriggerCharacter"].isArray()) {
        const QJsonArray arr = obj["moreTriggerCharacter"].toArray();
        QStringList list_moreTriggerCharacter;
        for (const QJsonValue &v : arr) {
            list_moreTriggerCharacter.append(v.toString());
        }
        result._moreTriggerCharacter = list_moreTriggerCharacter;
    }
    return result;
}

QJsonObject toJson(const DocumentOnTypeFormattingOptions &data)
{
    QJsonObject obj{{"firstTriggerCharacter", data._firstTriggerCharacter}};
    if (data._moreTriggerCharacter.has_value()) {
        QJsonArray arr_moreTriggerCharacter;
        for (const auto &v : *data._moreTriggerCharacter) arr_moreTriggerCharacter.append(v);
        obj.insert("moreTriggerCharacter", arr_moreTriggerCharacter);
    }
    return obj;
}

template<>
Utils::Result<DocumentRangeFormattingOptions> fromJson<DocumentRangeFormattingOptions>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for DocumentRangeFormattingOptions");
    const QJsonObject obj = val.toObject();
    DocumentRangeFormattingOptions result;
    if (obj.contains("workDoneProgress"))
        result._workDoneProgress = obj.value("workDoneProgress").toBool();
    if (obj.contains("rangesSupport"))
        result._rangesSupport = obj.value("rangesSupport").toBool();
    return result;
}

QJsonObject toJson(const DocumentRangeFormattingOptions &data)
{
    QJsonObject obj;
    if (data._workDoneProgress.has_value())
        obj.insert("workDoneProgress", *data._workDoneProgress);
    if (data._rangesSupport.has_value())
        obj.insert("rangesSupport", *data._rangesSupport);
    return obj;
}

template<>
Utils::Result<DocumentSymbolOptions> fromJson<DocumentSymbolOptions>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for DocumentSymbolOptions");
    const QJsonObject obj = val.toObject();
    DocumentSymbolOptions result;
    if (obj.contains("workDoneProgress"))
        result._workDoneProgress = obj.value("workDoneProgress").toBool();
    if (obj.contains("label"))
        result._label = obj.value("label").toString();
    return result;
}

QJsonObject toJson(const DocumentSymbolOptions &data)
{
    QJsonObject obj;
    if (data._workDoneProgress.has_value())
        obj.insert("workDoneProgress", *data._workDoneProgress);
    if (data._label.has_value())
        obj.insert("label", *data._label);
    return obj;
}

template<>
Utils::Result<ExecuteCommandOptions> fromJson<ExecuteCommandOptions>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for ExecuteCommandOptions");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("commands"))
        return Utils::ResultError("Missing required field: commands");
    ExecuteCommandOptions result;
    if (obj.contains("workDoneProgress"))
        result._workDoneProgress = obj.value("workDoneProgress").toBool();
    if (obj.contains("commands") && obj["commands"].isArray()) {
        const QJsonArray arr = obj["commands"].toArray();
        for (const QJsonValue &v : arr) {
            result._commands.append(v.toString());
        }
    }
    return result;
}

QJsonObject toJson(const ExecuteCommandOptions &data)
{
    QJsonObject obj;
    if (data._workDoneProgress.has_value())
        obj.insert("workDoneProgress", *data._workDoneProgress);
    QJsonArray arr_commands;
    for (const auto &v : data._commands) arr_commands.append(v);
    obj.insert("commands", arr_commands);
    return obj;
}

template<>
Utils::Result<FoldingRangeOptions> fromJson<FoldingRangeOptions>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for FoldingRangeOptions");
    const QJsonObject obj = val.toObject();
    FoldingRangeOptions result;
    if (obj.contains("workDoneProgress"))
        result._workDoneProgress = obj.value("workDoneProgress").toBool();
    return result;
}

QJsonObject toJson(const FoldingRangeOptions &data)
{
    QJsonObject obj;
    if (data._workDoneProgress.has_value())
        obj.insert("workDoneProgress", *data._workDoneProgress);
    return obj;
}

template<>
Utils::Result<HoverOptions> fromJson<HoverOptions>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for HoverOptions");
    const QJsonObject obj = val.toObject();
    HoverOptions result;
    if (obj.contains("workDoneProgress"))
        result._workDoneProgress = obj.value("workDoneProgress").toBool();
    return result;
}

QJsonObject toJson(const HoverOptions &data)
{
    QJsonObject obj;
    if (data._workDoneProgress.has_value())
        obj.insert("workDoneProgress", *data._workDoneProgress);
    return obj;
}

template<>
Utils::Result<ImplementationOptions> fromJson<ImplementationOptions>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for ImplementationOptions");
    const QJsonObject obj = val.toObject();
    ImplementationOptions result;
    if (obj.contains("workDoneProgress"))
        result._workDoneProgress = obj.value("workDoneProgress").toBool();
    return result;
}

QJsonObject toJson(const ImplementationOptions &data)
{
    QJsonObject obj;
    if (data._workDoneProgress.has_value())
        obj.insert("workDoneProgress", *data._workDoneProgress);
    return obj;
}

template<>
Utils::Result<InlayHintOptions> fromJson<InlayHintOptions>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for InlayHintOptions");
    const QJsonObject obj = val.toObject();
    InlayHintOptions result;
    if (obj.contains("workDoneProgress"))
        result._workDoneProgress = obj.value("workDoneProgress").toBool();
    if (obj.contains("resolveProvider"))
        result._resolveProvider = obj.value("resolveProvider").toBool();
    return result;
}

QJsonObject toJson(const InlayHintOptions &data)
{
    QJsonObject obj;
    if (data._workDoneProgress.has_value())
        obj.insert("workDoneProgress", *data._workDoneProgress);
    if (data._resolveProvider.has_value())
        obj.insert("resolveProvider", *data._resolveProvider);
    return obj;
}

template<>
Utils::Result<InlineCompletionOptions> fromJson<InlineCompletionOptions>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for InlineCompletionOptions");
    const QJsonObject obj = val.toObject();
    InlineCompletionOptions result;
    if (obj.contains("workDoneProgress"))
        result._workDoneProgress = obj.value("workDoneProgress").toBool();
    return result;
}

QJsonObject toJson(const InlineCompletionOptions &data)
{
    QJsonObject obj;
    if (data._workDoneProgress.has_value())
        obj.insert("workDoneProgress", *data._workDoneProgress);
    return obj;
}

template<>
Utils::Result<InlineValueOptions> fromJson<InlineValueOptions>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for InlineValueOptions");
    const QJsonObject obj = val.toObject();
    InlineValueOptions result;
    if (obj.contains("workDoneProgress"))
        result._workDoneProgress = obj.value("workDoneProgress").toBool();
    return result;
}

QJsonObject toJson(const InlineValueOptions &data)
{
    QJsonObject obj;
    if (data._workDoneProgress.has_value())
        obj.insert("workDoneProgress", *data._workDoneProgress);
    return obj;
}

template<>
Utils::Result<LinkedEditingRangeOptions> fromJson<LinkedEditingRangeOptions>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for LinkedEditingRangeOptions");
    const QJsonObject obj = val.toObject();
    LinkedEditingRangeOptions result;
    if (obj.contains("workDoneProgress"))
        result._workDoneProgress = obj.value("workDoneProgress").toBool();
    return result;
}

QJsonObject toJson(const LinkedEditingRangeOptions &data)
{
    QJsonObject obj;
    if (data._workDoneProgress.has_value())
        obj.insert("workDoneProgress", *data._workDoneProgress);
    return obj;
}

template<>
Utils::Result<MonikerOptions> fromJson<MonikerOptions>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for MonikerOptions");
    const QJsonObject obj = val.toObject();
    MonikerOptions result;
    if (obj.contains("workDoneProgress"))
        result._workDoneProgress = obj.value("workDoneProgress").toBool();
    return result;
}

QJsonObject toJson(const MonikerOptions &data)
{
    QJsonObject obj;
    if (data._workDoneProgress.has_value())
        obj.insert("workDoneProgress", *data._workDoneProgress);
    return obj;
}

template<>
Utils::Result<NotebookDocumentSyncOptions> fromJson<NotebookDocumentSyncOptions>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for NotebookDocumentSyncOptions");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("notebookSelector"))
        return Utils::ResultError("Missing required field: notebookSelector");
    NotebookDocumentSyncOptions result;
    if (obj.contains("notebookSelector") && obj["notebookSelector"].isArray()) {
        const QJsonArray arr = obj["notebookSelector"].toArray();
        for (const QJsonValue &v : arr) {
            const auto res0 = fromJson<NotebookDocumentSyncRegistrationOptionsNotebookSelectorItem>("notebookSelector", v);
            if (!res0)
                return Utils::ResultError(res0.error());
            result._notebookSelector.append(*res0);
        }
    }
    if (obj.contains("save"))
        result._save = obj.value("save").toBool();
    return result;
}

QJsonObject toJson(const NotebookDocumentSyncOptions &data)
{
    QJsonObject obj;
    QJsonArray arr_notebookSelector;
    for (const auto &v : data._notebookSelector) arr_notebookSelector.append(toJsonValue(v));
    obj.insert("notebookSelector", arr_notebookSelector);
    if (data._save.has_value())
        obj.insert("save", *data._save);
    return obj;
}

template<>
Utils::Result<ReferenceOptions> fromJson<ReferenceOptions>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for ReferenceOptions");
    const QJsonObject obj = val.toObject();
    ReferenceOptions result;
    if (obj.contains("workDoneProgress"))
        result._workDoneProgress = obj.value("workDoneProgress").toBool();
    return result;
}

QJsonObject toJson(const ReferenceOptions &data)
{
    QJsonObject obj;
    if (data._workDoneProgress.has_value())
        obj.insert("workDoneProgress", *data._workDoneProgress);
    return obj;
}

template<>
Utils::Result<RenameOptions> fromJson<RenameOptions>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for RenameOptions");
    const QJsonObject obj = val.toObject();
    RenameOptions result;
    if (obj.contains("workDoneProgress"))
        result._workDoneProgress = obj.value("workDoneProgress").toBool();
    if (obj.contains("prepareProvider"))
        result._prepareProvider = obj.value("prepareProvider").toBool();
    return result;
}

QJsonObject toJson(const RenameOptions &data)
{
    QJsonObject obj;
    if (data._workDoneProgress.has_value())
        obj.insert("workDoneProgress", *data._workDoneProgress);
    if (data._prepareProvider.has_value())
        obj.insert("prepareProvider", *data._prepareProvider);
    return obj;
}

template<>
Utils::Result<SelectionRangeOptions> fromJson<SelectionRangeOptions>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for SelectionRangeOptions");
    const QJsonObject obj = val.toObject();
    SelectionRangeOptions result;
    if (obj.contains("workDoneProgress"))
        result._workDoneProgress = obj.value("workDoneProgress").toBool();
    return result;
}

QJsonObject toJson(const SelectionRangeOptions &data)
{
    QJsonObject obj;
    if (data._workDoneProgress.has_value())
        obj.insert("workDoneProgress", *data._workDoneProgress);
    return obj;
}

template<>
Utils::Result<SemanticTokensOptions> fromJson<SemanticTokensOptions>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for SemanticTokensOptions");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("legend"))
        return Utils::ResultError("Missing required field: legend");
    SemanticTokensOptions result;
    if (obj.contains("workDoneProgress"))
        result._workDoneProgress = obj.value("workDoneProgress").toBool();
    if (obj.contains("legend") && obj["legend"].isObject()) {
        const auto res0 = fromJson<SemanticTokensLegend>("legend", obj["legend"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._legend = *res0;
    }
    if (obj.contains("range")) {
        const auto res1 = fromJson<SemanticTokensRegistrationOptionsRange>("range", obj["range"]);
        if (!res1)
            return Utils::ResultError(res1.error());
        result._range = *res1;
    }
    if (obj.contains("full")) {
        const auto res2 = fromJson<SemanticTokensRegistrationOptionsFull>("full", obj["full"]);
        if (!res2)
            return Utils::ResultError(res2.error());
        result._full = *res2;
    }
    return result;
}

QJsonObject toJson(const SemanticTokensOptions &data)
{
    QJsonObject obj{{"legend", toJson(data._legend)}};
    if (data._workDoneProgress.has_value())
        obj.insert("workDoneProgress", *data._workDoneProgress);
    if (data._range.has_value())
        obj.insert("range", toJsonValue(*data._range));
    if (data._full.has_value())
        obj.insert("full", toJsonValue(*data._full));
    return obj;
}

template<>
Utils::Result<SignatureHelpOptions> fromJson<SignatureHelpOptions>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for SignatureHelpOptions");
    const QJsonObject obj = val.toObject();
    SignatureHelpOptions result;
    if (obj.contains("workDoneProgress"))
        result._workDoneProgress = obj.value("workDoneProgress").toBool();
    if (obj.contains("triggerCharacters") && obj["triggerCharacters"].isArray()) {
        const QJsonArray arr = obj["triggerCharacters"].toArray();
        QStringList list_triggerCharacters;
        for (const QJsonValue &v : arr) {
            list_triggerCharacters.append(v.toString());
        }
        result._triggerCharacters = list_triggerCharacters;
    }
    if (obj.contains("retriggerCharacters") && obj["retriggerCharacters"].isArray()) {
        const QJsonArray arr = obj["retriggerCharacters"].toArray();
        QStringList list_retriggerCharacters;
        for (const QJsonValue &v : arr) {
            list_retriggerCharacters.append(v.toString());
        }
        result._retriggerCharacters = list_retriggerCharacters;
    }
    return result;
}

QJsonObject toJson(const SignatureHelpOptions &data)
{
    QJsonObject obj;
    if (data._workDoneProgress.has_value())
        obj.insert("workDoneProgress", *data._workDoneProgress);
    if (data._triggerCharacters.has_value()) {
        QJsonArray arr_triggerCharacters;
        for (const auto &v : *data._triggerCharacters) arr_triggerCharacters.append(v);
        obj.insert("triggerCharacters", arr_triggerCharacters);
    }
    if (data._retriggerCharacters.has_value()) {
        QJsonArray arr_retriggerCharacters;
        for (const auto &v : *data._retriggerCharacters) arr_retriggerCharacters.append(v);
        obj.insert("retriggerCharacters", arr_retriggerCharacters);
    }
    return obj;
}

template<>
Utils::Result<SaveOptions> fromJson<SaveOptions>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for SaveOptions");
    const QJsonObject obj = val.toObject();
    SaveOptions result;
    if (obj.contains("includeText"))
        result._includeText = obj.value("includeText").toBool();
    return result;
}

QJsonObject toJson(const SaveOptions &data)
{
    QJsonObject obj;
    if (data._includeText.has_value())
        obj.insert("includeText", *data._includeText);
    return obj;
}

template<>
Utils::Result<TextDocumentSyncOptionsSave> fromJson<TextDocumentSyncOptionsSave>(const QJsonValue &val)
{
    if (val.isBool())
        return TextDocumentSyncOptionsSave(val.toBool());
    {
        auto result = fromJson<SaveOptions>(val);
        if (result) return TextDocumentSyncOptionsSave(*result);
    }
    return Utils::ResultError("Invalid TextDocumentSyncOptionsSave");
}

QJsonValue toJsonValue(const TextDocumentSyncOptionsSave &val)
{
    return std::visit([](const auto &v) -> QJsonValue {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, SaveOptions>) {
            return toJson(v);
        } else
        {
            return QVariant::fromValue(v).toJsonValue();
        }
    }, val);
}

template<>
Utils::Result<TextDocumentSyncOptions> fromJson<TextDocumentSyncOptions>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for TextDocumentSyncOptions");
    const QJsonObject obj = val.toObject();
    TextDocumentSyncOptions result;
    if (obj.contains("openClose"))
        result._openClose = obj.value("openClose").toBool();
    if (obj.contains("change") && obj["change"].isDouble())
        result._change = obj["change"].toInt();
    if (obj.contains("willSave"))
        result._willSave = obj.value("willSave").toBool();
    if (obj.contains("willSaveWaitUntil"))
        result._willSaveWaitUntil = obj.value("willSaveWaitUntil").toBool();
    if (obj.contains("save")) {
        const auto res0 = fromJson<TextDocumentSyncOptionsSave>("save", obj["save"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._save = *res0;
    }
    return result;
}

QJsonObject toJson(const TextDocumentSyncOptions &data)
{
    QJsonObject obj;
    if (data._openClose.has_value())
        obj.insert("openClose", *data._openClose);
    if (data._change.has_value())
        obj.insert("change", *data._change);
    if (data._willSave.has_value())
        obj.insert("willSave", *data._willSave);
    if (data._willSaveWaitUntil.has_value())
        obj.insert("willSaveWaitUntil", *data._willSaveWaitUntil);
    if (data._save.has_value())
        obj.insert("save", toJsonValue(*data._save));
    return obj;
}

template<>
Utils::Result<TypeDefinitionOptions> fromJson<TypeDefinitionOptions>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for TypeDefinitionOptions");
    const QJsonObject obj = val.toObject();
    TypeDefinitionOptions result;
    if (obj.contains("workDoneProgress"))
        result._workDoneProgress = obj.value("workDoneProgress").toBool();
    return result;
}

QJsonObject toJson(const TypeDefinitionOptions &data)
{
    QJsonObject obj;
    if (data._workDoneProgress.has_value())
        obj.insert("workDoneProgress", *data._workDoneProgress);
    return obj;
}

template<>
Utils::Result<TypeHierarchyOptions> fromJson<TypeHierarchyOptions>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for TypeHierarchyOptions");
    const QJsonObject obj = val.toObject();
    TypeHierarchyOptions result;
    if (obj.contains("workDoneProgress"))
        result._workDoneProgress = obj.value("workDoneProgress").toBool();
    return result;
}

QJsonObject toJson(const TypeHierarchyOptions &data)
{
    QJsonObject obj;
    if (data._workDoneProgress.has_value())
        obj.insert("workDoneProgress", *data._workDoneProgress);
    return obj;
}

template<>
Utils::Result<FileOperationOptions> fromJson<FileOperationOptions>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for FileOperationOptions");
    const QJsonObject obj = val.toObject();
    FileOperationOptions result;
    if (obj.contains("didCreate") && obj["didCreate"].isObject()) {
        const auto res0 = fromJson<FileOperationRegistrationOptions>("didCreate", obj["didCreate"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._didCreate = *res0;
    }
    if (obj.contains("willCreate") && obj["willCreate"].isObject()) {
        const auto res1 = fromJson<FileOperationRegistrationOptions>("willCreate", obj["willCreate"]);
        if (!res1)
            return Utils::ResultError(res1.error());
        result._willCreate = *res1;
    }
    if (obj.contains("didRename") && obj["didRename"].isObject()) {
        const auto res2 = fromJson<FileOperationRegistrationOptions>("didRename", obj["didRename"]);
        if (!res2)
            return Utils::ResultError(res2.error());
        result._didRename = *res2;
    }
    if (obj.contains("willRename") && obj["willRename"].isObject()) {
        const auto res3 = fromJson<FileOperationRegistrationOptions>("willRename", obj["willRename"]);
        if (!res3)
            return Utils::ResultError(res3.error());
        result._willRename = *res3;
    }
    if (obj.contains("didDelete") && obj["didDelete"].isObject()) {
        const auto res4 = fromJson<FileOperationRegistrationOptions>("didDelete", obj["didDelete"]);
        if (!res4)
            return Utils::ResultError(res4.error());
        result._didDelete = *res4;
    }
    if (obj.contains("willDelete") && obj["willDelete"].isObject()) {
        const auto res5 = fromJson<FileOperationRegistrationOptions>("willDelete", obj["willDelete"]);
        if (!res5)
            return Utils::ResultError(res5.error());
        result._willDelete = *res5;
    }
    return result;
}

QJsonObject toJson(const FileOperationOptions &data)
{
    QJsonObject obj;
    if (data._didCreate.has_value())
        obj.insert("didCreate", toJson(*data._didCreate));
    if (data._willCreate.has_value())
        obj.insert("willCreate", toJson(*data._willCreate));
    if (data._didRename.has_value())
        obj.insert("didRename", toJson(*data._didRename));
    if (data._willRename.has_value())
        obj.insert("willRename", toJson(*data._willRename));
    if (data._didDelete.has_value())
        obj.insert("didDelete", toJson(*data._didDelete));
    if (data._willDelete.has_value())
        obj.insert("willDelete", toJson(*data._willDelete));
    return obj;
}

template<>
Utils::Result<TextDocumentContentOptions> fromJson<TextDocumentContentOptions>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for TextDocumentContentOptions");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("schemes"))
        return Utils::ResultError("Missing required field: schemes");
    TextDocumentContentOptions result;
    if (obj.contains("schemes") && obj["schemes"].isArray()) {
        const QJsonArray arr = obj["schemes"].toArray();
        for (const QJsonValue &v : arr) {
            result._schemes.append(v.toString());
        }
    }
    return result;
}

QJsonObject toJson(const TextDocumentContentOptions &data)
{
    QJsonObject obj;
    QJsonArray arr_schemes;
    for (const auto &v : data._schemes) arr_schemes.append(v);
    obj.insert("schemes", arr_schemes);
    return obj;
}

template<>
Utils::Result<WorkspaceFoldersServerCapabilitiesChangeNotifications> fromJson<WorkspaceFoldersServerCapabilitiesChangeNotifications>(const QJsonValue &val)
{
    if (val.isString())
        return WorkspaceFoldersServerCapabilitiesChangeNotifications(val.toString());
    if (val.isBool())
        return WorkspaceFoldersServerCapabilitiesChangeNotifications(val.toBool());
    return Utils::ResultError("Invalid WorkspaceFoldersServerCapabilitiesChangeNotifications");
}

QJsonValue toJsonValue(const WorkspaceFoldersServerCapabilitiesChangeNotifications &val)
{
    return std::visit([](const auto &v) -> QJsonValue {
        using T = std::decay_t<decltype(v)>;
        {
            return QVariant::fromValue(v).toJsonValue();
        }
    }, val);
}

template<>
Utils::Result<WorkspaceFoldersServerCapabilities> fromJson<WorkspaceFoldersServerCapabilities>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for WorkspaceFoldersServerCapabilities");
    const QJsonObject obj = val.toObject();
    WorkspaceFoldersServerCapabilities result;
    if (obj.contains("supported"))
        result._supported = obj.value("supported").toBool();
    if (obj.contains("changeNotifications")) {
        const auto res0 = fromJson<WorkspaceFoldersServerCapabilitiesChangeNotifications>("changeNotifications", obj["changeNotifications"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._changeNotifications = *res0;
    }
    return result;
}

QJsonObject toJson(const WorkspaceFoldersServerCapabilities &data)
{
    QJsonObject obj;
    if (data._supported.has_value())
        obj.insert("supported", *data._supported);
    if (data._changeNotifications.has_value())
        obj.insert("changeNotifications", toJsonValue(*data._changeNotifications));
    return obj;
}

template<>
Utils::Result<WorkspaceOptionsTextDocumentContent> fromJson<WorkspaceOptionsTextDocumentContent>(const QJsonValue &val)
{
    {
        auto result = fromJson<TextDocumentContentOptions>(val);
        if (result) return WorkspaceOptionsTextDocumentContent(*result);
    }
    {
        auto result = fromJson<TextDocumentContentRegistrationOptions>(val);
        if (result) return WorkspaceOptionsTextDocumentContent(*result);
    }
    return Utils::ResultError("Invalid WorkspaceOptionsTextDocumentContent");
}

QJsonValue toJsonValue(const WorkspaceOptionsTextDocumentContent &val)
{
    return std::visit([](const auto &v) -> QJsonValue {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, QJsonObject>) {
            return v;
        } else {
            return toJson(v);
        }
    }, val);
}

template<>
Utils::Result<WorkspaceOptions> fromJson<WorkspaceOptions>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for WorkspaceOptions");
    const QJsonObject obj = val.toObject();
    WorkspaceOptions result;
    if (obj.contains("workspaceFolders") && obj["workspaceFolders"].isObject()) {
        const auto res0 = fromJson<WorkspaceFoldersServerCapabilities>("workspaceFolders", obj["workspaceFolders"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._workspaceFolders = *res0;
    }
    if (obj.contains("fileOperations") && obj["fileOperations"].isObject()) {
        const auto res1 = fromJson<FileOperationOptions>("fileOperations", obj["fileOperations"]);
        if (!res1)
            return Utils::ResultError(res1.error());
        result._fileOperations = *res1;
    }
    if (obj.contains("textDocumentContent")) {
        const auto res2 = fromJson<WorkspaceOptionsTextDocumentContent>("textDocumentContent", obj["textDocumentContent"]);
        if (!res2)
            return Utils::ResultError(res2.error());
        result._textDocumentContent = *res2;
    }
    return result;
}

QJsonObject toJson(const WorkspaceOptions &data)
{
    QJsonObject obj;
    if (data._workspaceFolders.has_value())
        obj.insert("workspaceFolders", toJson(*data._workspaceFolders));
    if (data._fileOperations.has_value())
        obj.insert("fileOperations", toJson(*data._fileOperations));
    if (data._textDocumentContent.has_value())
        obj.insert("textDocumentContent", toJsonValue(*data._textDocumentContent));
    return obj;
}

template<>
Utils::Result<WorkspaceSymbolOptions> fromJson<WorkspaceSymbolOptions>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for WorkspaceSymbolOptions");
    const QJsonObject obj = val.toObject();
    WorkspaceSymbolOptions result;
    if (obj.contains("workDoneProgress"))
        result._workDoneProgress = obj.value("workDoneProgress").toBool();
    if (obj.contains("resolveProvider"))
        result._resolveProvider = obj.value("resolveProvider").toBool();
    return result;
}

QJsonObject toJson(const WorkspaceSymbolOptions &data)
{
    QJsonObject obj;
    if (data._workDoneProgress.has_value())
        obj.insert("workDoneProgress", *data._workDoneProgress);
    if (data._resolveProvider.has_value())
        obj.insert("resolveProvider", *data._resolveProvider);
    return obj;
}

template<>
Utils::Result<ServerCapabilitiesTextDocumentSync> fromJson<ServerCapabilitiesTextDocumentSync>(const QJsonValue &val)
{
    if (val.isDouble())
        return ServerCapabilitiesTextDocumentSync(val.toInt());
    {
        auto result = fromJson<TextDocumentSyncOptions>(val);
        if (result) return ServerCapabilitiesTextDocumentSync(*result);
    }
    return Utils::ResultError("Invalid ServerCapabilitiesTextDocumentSync");
}

QJsonValue toJsonValue(const ServerCapabilitiesTextDocumentSync &val)
{
    return std::visit([](const auto &v) -> QJsonValue {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, TextDocumentSyncOptions>) {
            return toJson(v);
        } else
        {
            return QVariant::fromValue(v).toJsonValue();
        }
    }, val);
}

template<>
Utils::Result<ServerCapabilitiesNotebookDocumentSync> fromJson<ServerCapabilitiesNotebookDocumentSync>(const QJsonValue &val)
{
    {
        auto result = fromJson<NotebookDocumentSyncOptions>(val);
        if (result) return ServerCapabilitiesNotebookDocumentSync(*result);
    }
    {
        auto result = fromJson<NotebookDocumentSyncRegistrationOptions>(val);
        if (result) return ServerCapabilitiesNotebookDocumentSync(*result);
    }
    return Utils::ResultError("Invalid ServerCapabilitiesNotebookDocumentSync");
}

QJsonValue toJsonValue(const ServerCapabilitiesNotebookDocumentSync &val)
{
    return std::visit([](const auto &v) -> QJsonValue {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, QJsonObject>) {
            return v;
        } else {
            return toJson(v);
        }
    }, val);
}

template<>
Utils::Result<ServerCapabilitiesHoverProvider> fromJson<ServerCapabilitiesHoverProvider>(const QJsonValue &val)
{
    if (val.isBool())
        return ServerCapabilitiesHoverProvider(val.toBool());
    {
        auto result = fromJson<HoverOptions>(val);
        if (result) return ServerCapabilitiesHoverProvider(*result);
    }
    return Utils::ResultError("Invalid ServerCapabilitiesHoverProvider");
}

QJsonValue toJsonValue(const ServerCapabilitiesHoverProvider &val)
{
    return std::visit([](const auto &v) -> QJsonValue {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, HoverOptions>) {
            return toJson(v);
        } else
        {
            return QVariant::fromValue(v).toJsonValue();
        }
    }, val);
}

template<>
Utils::Result<ServerCapabilitiesDeclarationProvider> fromJson<ServerCapabilitiesDeclarationProvider>(const QJsonValue &val)
{
    if (val.isBool())
        return ServerCapabilitiesDeclarationProvider(val.toBool());
    if (!val.isObject())
        return Utils::ResultError("Invalid ServerCapabilitiesDeclarationProvider: expected object or array");
    const QJsonObject obj = val.toObject();
    if (obj.contains("documentSelector")) {
        const auto res0 = fromJson<DeclarationRegistrationOptions>(val);
        if (!res0)
            return Utils::ResultError(res0.error());
        return ServerCapabilitiesDeclarationProvider(*res0);
    }
    {
        auto result = fromJson<DeclarationOptions>(val);
        if (result) return ServerCapabilitiesDeclarationProvider(*result);
    }
    return Utils::ResultError("Invalid ServerCapabilitiesDeclarationProvider");
}

QJsonValue toJsonValue(const ServerCapabilitiesDeclarationProvider &val)
{
    return std::visit([](const auto &v) -> QJsonValue {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, DeclarationOptions>) {
            return toJson(v);
        } else
        if constexpr (std::is_same_v<T, DeclarationRegistrationOptions>) {
            return toJson(v);
        } else
        {
            return QVariant::fromValue(v).toJsonValue();
        }
    }, val);
}

template<>
Utils::Result<ServerCapabilitiesDefinitionProvider> fromJson<ServerCapabilitiesDefinitionProvider>(const QJsonValue &val)
{
    if (val.isBool())
        return ServerCapabilitiesDefinitionProvider(val.toBool());
    {
        auto result = fromJson<DefinitionOptions>(val);
        if (result) return ServerCapabilitiesDefinitionProvider(*result);
    }
    return Utils::ResultError("Invalid ServerCapabilitiesDefinitionProvider");
}

QJsonValue toJsonValue(const ServerCapabilitiesDefinitionProvider &val)
{
    return std::visit([](const auto &v) -> QJsonValue {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, DefinitionOptions>) {
            return toJson(v);
        } else
        {
            return QVariant::fromValue(v).toJsonValue();
        }
    }, val);
}

template<>
Utils::Result<ServerCapabilitiesTypeDefinitionProvider> fromJson<ServerCapabilitiesTypeDefinitionProvider>(const QJsonValue &val)
{
    if (val.isBool())
        return ServerCapabilitiesTypeDefinitionProvider(val.toBool());
    if (!val.isObject())
        return Utils::ResultError("Invalid ServerCapabilitiesTypeDefinitionProvider: expected object or array");
    const QJsonObject obj = val.toObject();
    if (obj.contains("documentSelector")) {
        const auto res0 = fromJson<TypeDefinitionRegistrationOptions>(val);
        if (!res0)
            return Utils::ResultError(res0.error());
        return ServerCapabilitiesTypeDefinitionProvider(*res0);
    }
    {
        auto result = fromJson<TypeDefinitionOptions>(val);
        if (result) return ServerCapabilitiesTypeDefinitionProvider(*result);
    }
    return Utils::ResultError("Invalid ServerCapabilitiesTypeDefinitionProvider");
}

QJsonValue toJsonValue(const ServerCapabilitiesTypeDefinitionProvider &val)
{
    return std::visit([](const auto &v) -> QJsonValue {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, TypeDefinitionOptions>) {
            return toJson(v);
        } else
        if constexpr (std::is_same_v<T, TypeDefinitionRegistrationOptions>) {
            return toJson(v);
        } else
        {
            return QVariant::fromValue(v).toJsonValue();
        }
    }, val);
}

template<>
Utils::Result<ServerCapabilitiesImplementationProvider> fromJson<ServerCapabilitiesImplementationProvider>(const QJsonValue &val)
{
    if (val.isBool())
        return ServerCapabilitiesImplementationProvider(val.toBool());
    if (!val.isObject())
        return Utils::ResultError("Invalid ServerCapabilitiesImplementationProvider: expected object or array");
    const QJsonObject obj = val.toObject();
    if (obj.contains("documentSelector")) {
        const auto res0 = fromJson<ImplementationRegistrationOptions>(val);
        if (!res0)
            return Utils::ResultError(res0.error());
        return ServerCapabilitiesImplementationProvider(*res0);
    }
    {
        auto result = fromJson<ImplementationOptions>(val);
        if (result) return ServerCapabilitiesImplementationProvider(*result);
    }
    return Utils::ResultError("Invalid ServerCapabilitiesImplementationProvider");
}

QJsonValue toJsonValue(const ServerCapabilitiesImplementationProvider &val)
{
    return std::visit([](const auto &v) -> QJsonValue {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, ImplementationOptions>) {
            return toJson(v);
        } else
        if constexpr (std::is_same_v<T, ImplementationRegistrationOptions>) {
            return toJson(v);
        } else
        {
            return QVariant::fromValue(v).toJsonValue();
        }
    }, val);
}

template<>
Utils::Result<ServerCapabilitiesReferencesProvider> fromJson<ServerCapabilitiesReferencesProvider>(const QJsonValue &val)
{
    if (val.isBool())
        return ServerCapabilitiesReferencesProvider(val.toBool());
    {
        auto result = fromJson<ReferenceOptions>(val);
        if (result) return ServerCapabilitiesReferencesProvider(*result);
    }
    return Utils::ResultError("Invalid ServerCapabilitiesReferencesProvider");
}

QJsonValue toJsonValue(const ServerCapabilitiesReferencesProvider &val)
{
    return std::visit([](const auto &v) -> QJsonValue {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, ReferenceOptions>) {
            return toJson(v);
        } else
        {
            return QVariant::fromValue(v).toJsonValue();
        }
    }, val);
}

template<>
Utils::Result<ServerCapabilitiesDocumentHighlightProvider> fromJson<ServerCapabilitiesDocumentHighlightProvider>(const QJsonValue &val)
{
    if (val.isBool())
        return ServerCapabilitiesDocumentHighlightProvider(val.toBool());
    {
        auto result = fromJson<DocumentHighlightOptions>(val);
        if (result) return ServerCapabilitiesDocumentHighlightProvider(*result);
    }
    return Utils::ResultError("Invalid ServerCapabilitiesDocumentHighlightProvider");
}

QJsonValue toJsonValue(const ServerCapabilitiesDocumentHighlightProvider &val)
{
    return std::visit([](const auto &v) -> QJsonValue {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, DocumentHighlightOptions>) {
            return toJson(v);
        } else
        {
            return QVariant::fromValue(v).toJsonValue();
        }
    }, val);
}

template<>
Utils::Result<ServerCapabilitiesDocumentSymbolProvider> fromJson<ServerCapabilitiesDocumentSymbolProvider>(const QJsonValue &val)
{
    if (val.isBool())
        return ServerCapabilitiesDocumentSymbolProvider(val.toBool());
    {
        auto result = fromJson<DocumentSymbolOptions>(val);
        if (result) return ServerCapabilitiesDocumentSymbolProvider(*result);
    }
    return Utils::ResultError("Invalid ServerCapabilitiesDocumentSymbolProvider");
}

QJsonValue toJsonValue(const ServerCapabilitiesDocumentSymbolProvider &val)
{
    return std::visit([](const auto &v) -> QJsonValue {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, DocumentSymbolOptions>) {
            return toJson(v);
        } else
        {
            return QVariant::fromValue(v).toJsonValue();
        }
    }, val);
}

template<>
Utils::Result<ServerCapabilitiesCodeActionProvider> fromJson<ServerCapabilitiesCodeActionProvider>(const QJsonValue &val)
{
    if (val.isBool())
        return ServerCapabilitiesCodeActionProvider(val.toBool());
    {
        auto result = fromJson<CodeActionOptions>(val);
        if (result) return ServerCapabilitiesCodeActionProvider(*result);
    }
    return Utils::ResultError("Invalid ServerCapabilitiesCodeActionProvider");
}

QJsonValue toJsonValue(const ServerCapabilitiesCodeActionProvider &val)
{
    return std::visit([](const auto &v) -> QJsonValue {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, CodeActionOptions>) {
            return toJson(v);
        } else
        {
            return QVariant::fromValue(v).toJsonValue();
        }
    }, val);
}

template<>
Utils::Result<ServerCapabilitiesColorProvider> fromJson<ServerCapabilitiesColorProvider>(const QJsonValue &val)
{
    if (val.isBool())
        return ServerCapabilitiesColorProvider(val.toBool());
    if (!val.isObject())
        return Utils::ResultError("Invalid ServerCapabilitiesColorProvider: expected object or array");
    const QJsonObject obj = val.toObject();
    if (obj.contains("documentSelector")) {
        const auto res0 = fromJson<DocumentColorRegistrationOptions>(val);
        if (!res0)
            return Utils::ResultError(res0.error());
        return ServerCapabilitiesColorProvider(*res0);
    }
    {
        auto result = fromJson<DocumentColorOptions>(val);
        if (result) return ServerCapabilitiesColorProvider(*result);
    }
    return Utils::ResultError("Invalid ServerCapabilitiesColorProvider");
}

QJsonValue toJsonValue(const ServerCapabilitiesColorProvider &val)
{
    return std::visit([](const auto &v) -> QJsonValue {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, DocumentColorOptions>) {
            return toJson(v);
        } else
        if constexpr (std::is_same_v<T, DocumentColorRegistrationOptions>) {
            return toJson(v);
        } else
        {
            return QVariant::fromValue(v).toJsonValue();
        }
    }, val);
}

template<>
Utils::Result<ServerCapabilitiesWorkspaceSymbolProvider> fromJson<ServerCapabilitiesWorkspaceSymbolProvider>(const QJsonValue &val)
{
    if (val.isBool())
        return ServerCapabilitiesWorkspaceSymbolProvider(val.toBool());
    {
        auto result = fromJson<WorkspaceSymbolOptions>(val);
        if (result) return ServerCapabilitiesWorkspaceSymbolProvider(*result);
    }
    return Utils::ResultError("Invalid ServerCapabilitiesWorkspaceSymbolProvider");
}

QJsonValue toJsonValue(const ServerCapabilitiesWorkspaceSymbolProvider &val)
{
    return std::visit([](const auto &v) -> QJsonValue {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, WorkspaceSymbolOptions>) {
            return toJson(v);
        } else
        {
            return QVariant::fromValue(v).toJsonValue();
        }
    }, val);
}

template<>
Utils::Result<ServerCapabilitiesDocumentFormattingProvider> fromJson<ServerCapabilitiesDocumentFormattingProvider>(const QJsonValue &val)
{
    if (val.isBool())
        return ServerCapabilitiesDocumentFormattingProvider(val.toBool());
    {
        auto result = fromJson<DocumentFormattingOptions>(val);
        if (result) return ServerCapabilitiesDocumentFormattingProvider(*result);
    }
    return Utils::ResultError("Invalid ServerCapabilitiesDocumentFormattingProvider");
}

QJsonValue toJsonValue(const ServerCapabilitiesDocumentFormattingProvider &val)
{
    return std::visit([](const auto &v) -> QJsonValue {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, DocumentFormattingOptions>) {
            return toJson(v);
        } else
        {
            return QVariant::fromValue(v).toJsonValue();
        }
    }, val);
}

template<>
Utils::Result<ServerCapabilitiesDocumentRangeFormattingProvider> fromJson<ServerCapabilitiesDocumentRangeFormattingProvider>(const QJsonValue &val)
{
    if (val.isBool())
        return ServerCapabilitiesDocumentRangeFormattingProvider(val.toBool());
    {
        auto result = fromJson<DocumentRangeFormattingOptions>(val);
        if (result) return ServerCapabilitiesDocumentRangeFormattingProvider(*result);
    }
    return Utils::ResultError("Invalid ServerCapabilitiesDocumentRangeFormattingProvider");
}

QJsonValue toJsonValue(const ServerCapabilitiesDocumentRangeFormattingProvider &val)
{
    return std::visit([](const auto &v) -> QJsonValue {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, DocumentRangeFormattingOptions>) {
            return toJson(v);
        } else
        {
            return QVariant::fromValue(v).toJsonValue();
        }
    }, val);
}

template<>
Utils::Result<ServerCapabilitiesRenameProvider> fromJson<ServerCapabilitiesRenameProvider>(const QJsonValue &val)
{
    if (val.isBool())
        return ServerCapabilitiesRenameProvider(val.toBool());
    {
        auto result = fromJson<RenameOptions>(val);
        if (result) return ServerCapabilitiesRenameProvider(*result);
    }
    return Utils::ResultError("Invalid ServerCapabilitiesRenameProvider");
}

QJsonValue toJsonValue(const ServerCapabilitiesRenameProvider &val)
{
    return std::visit([](const auto &v) -> QJsonValue {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, RenameOptions>) {
            return toJson(v);
        } else
        {
            return QVariant::fromValue(v).toJsonValue();
        }
    }, val);
}

template<>
Utils::Result<ServerCapabilitiesFoldingRangeProvider> fromJson<ServerCapabilitiesFoldingRangeProvider>(const QJsonValue &val)
{
    if (val.isBool())
        return ServerCapabilitiesFoldingRangeProvider(val.toBool());
    if (!val.isObject())
        return Utils::ResultError("Invalid ServerCapabilitiesFoldingRangeProvider: expected object or array");
    const QJsonObject obj = val.toObject();
    if (obj.contains("documentSelector")) {
        const auto res0 = fromJson<FoldingRangeRegistrationOptions>(val);
        if (!res0)
            return Utils::ResultError(res0.error());
        return ServerCapabilitiesFoldingRangeProvider(*res0);
    }
    {
        auto result = fromJson<FoldingRangeOptions>(val);
        if (result) return ServerCapabilitiesFoldingRangeProvider(*result);
    }
    return Utils::ResultError("Invalid ServerCapabilitiesFoldingRangeProvider");
}

QJsonValue toJsonValue(const ServerCapabilitiesFoldingRangeProvider &val)
{
    return std::visit([](const auto &v) -> QJsonValue {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, FoldingRangeOptions>) {
            return toJson(v);
        } else
        if constexpr (std::is_same_v<T, FoldingRangeRegistrationOptions>) {
            return toJson(v);
        } else
        {
            return QVariant::fromValue(v).toJsonValue();
        }
    }, val);
}

template<>
Utils::Result<ServerCapabilitiesSelectionRangeProvider> fromJson<ServerCapabilitiesSelectionRangeProvider>(const QJsonValue &val)
{
    if (val.isBool())
        return ServerCapabilitiesSelectionRangeProvider(val.toBool());
    if (!val.isObject())
        return Utils::ResultError("Invalid ServerCapabilitiesSelectionRangeProvider: expected object or array");
    const QJsonObject obj = val.toObject();
    if (obj.contains("documentSelector")) {
        const auto res0 = fromJson<SelectionRangeRegistrationOptions>(val);
        if (!res0)
            return Utils::ResultError(res0.error());
        return ServerCapabilitiesSelectionRangeProvider(*res0);
    }
    {
        auto result = fromJson<SelectionRangeOptions>(val);
        if (result) return ServerCapabilitiesSelectionRangeProvider(*result);
    }
    return Utils::ResultError("Invalid ServerCapabilitiesSelectionRangeProvider");
}

QJsonValue toJsonValue(const ServerCapabilitiesSelectionRangeProvider &val)
{
    return std::visit([](const auto &v) -> QJsonValue {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, SelectionRangeOptions>) {
            return toJson(v);
        } else
        if constexpr (std::is_same_v<T, SelectionRangeRegistrationOptions>) {
            return toJson(v);
        } else
        {
            return QVariant::fromValue(v).toJsonValue();
        }
    }, val);
}

template<>
Utils::Result<ServerCapabilitiesCallHierarchyProvider> fromJson<ServerCapabilitiesCallHierarchyProvider>(const QJsonValue &val)
{
    if (val.isBool())
        return ServerCapabilitiesCallHierarchyProvider(val.toBool());
    if (!val.isObject())
        return Utils::ResultError("Invalid ServerCapabilitiesCallHierarchyProvider: expected object or array");
    const QJsonObject obj = val.toObject();
    if (obj.contains("documentSelector")) {
        const auto res0 = fromJson<CallHierarchyRegistrationOptions>(val);
        if (!res0)
            return Utils::ResultError(res0.error());
        return ServerCapabilitiesCallHierarchyProvider(*res0);
    }
    {
        auto result = fromJson<CallHierarchyOptions>(val);
        if (result) return ServerCapabilitiesCallHierarchyProvider(*result);
    }
    return Utils::ResultError("Invalid ServerCapabilitiesCallHierarchyProvider");
}

QJsonValue toJsonValue(const ServerCapabilitiesCallHierarchyProvider &val)
{
    return std::visit([](const auto &v) -> QJsonValue {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, CallHierarchyOptions>) {
            return toJson(v);
        } else
        if constexpr (std::is_same_v<T, CallHierarchyRegistrationOptions>) {
            return toJson(v);
        } else
        {
            return QVariant::fromValue(v).toJsonValue();
        }
    }, val);
}

template<>
Utils::Result<ServerCapabilitiesLinkedEditingRangeProvider> fromJson<ServerCapabilitiesLinkedEditingRangeProvider>(const QJsonValue &val)
{
    if (val.isBool())
        return ServerCapabilitiesLinkedEditingRangeProvider(val.toBool());
    if (!val.isObject())
        return Utils::ResultError("Invalid ServerCapabilitiesLinkedEditingRangeProvider: expected object or array");
    const QJsonObject obj = val.toObject();
    if (obj.contains("documentSelector")) {
        const auto res0 = fromJson<LinkedEditingRangeRegistrationOptions>(val);
        if (!res0)
            return Utils::ResultError(res0.error());
        return ServerCapabilitiesLinkedEditingRangeProvider(*res0);
    }
    {
        auto result = fromJson<LinkedEditingRangeOptions>(val);
        if (result) return ServerCapabilitiesLinkedEditingRangeProvider(*result);
    }
    return Utils::ResultError("Invalid ServerCapabilitiesLinkedEditingRangeProvider");
}

QJsonValue toJsonValue(const ServerCapabilitiesLinkedEditingRangeProvider &val)
{
    return std::visit([](const auto &v) -> QJsonValue {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, LinkedEditingRangeOptions>) {
            return toJson(v);
        } else
        if constexpr (std::is_same_v<T, LinkedEditingRangeRegistrationOptions>) {
            return toJson(v);
        } else
        {
            return QVariant::fromValue(v).toJsonValue();
        }
    }, val);
}

template<>
Utils::Result<ServerCapabilitiesSemanticTokensProvider> fromJson<ServerCapabilitiesSemanticTokensProvider>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Invalid ServerCapabilitiesSemanticTokensProvider: expected object or array");
    const QJsonObject obj = val.toObject();
    if (obj.contains("documentSelector")) {
        const auto res0 = fromJson<SemanticTokensRegistrationOptions>(val);
        if (!res0)
            return Utils::ResultError(res0.error());
        return ServerCapabilitiesSemanticTokensProvider(*res0);
    }
    {
        auto result = fromJson<SemanticTokensOptions>(val);
        if (result) return ServerCapabilitiesSemanticTokensProvider(*result);
    }
    return Utils::ResultError("Invalid ServerCapabilitiesSemanticTokensProvider");
}

QJsonValue toJsonValue(const ServerCapabilitiesSemanticTokensProvider &val)
{
    return std::visit([](const auto &v) -> QJsonValue {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, QJsonObject>) {
            return v;
        } else {
            return toJson(v);
        }
    }, val);
}

template<>
Utils::Result<ServerCapabilitiesMonikerProvider> fromJson<ServerCapabilitiesMonikerProvider>(const QJsonValue &val)
{
    if (val.isBool())
        return ServerCapabilitiesMonikerProvider(val.toBool());
    if (!val.isObject())
        return Utils::ResultError("Invalid ServerCapabilitiesMonikerProvider: expected object or array");
    const QJsonObject obj = val.toObject();
    if (obj.contains("documentSelector")) {
        const auto res0 = fromJson<MonikerRegistrationOptions>(val);
        if (!res0)
            return Utils::ResultError(res0.error());
        return ServerCapabilitiesMonikerProvider(*res0);
    }
    {
        auto result = fromJson<MonikerOptions>(val);
        if (result) return ServerCapabilitiesMonikerProvider(*result);
    }
    return Utils::ResultError("Invalid ServerCapabilitiesMonikerProvider");
}

QJsonValue toJsonValue(const ServerCapabilitiesMonikerProvider &val)
{
    return std::visit([](const auto &v) -> QJsonValue {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, MonikerOptions>) {
            return toJson(v);
        } else
        if constexpr (std::is_same_v<T, MonikerRegistrationOptions>) {
            return toJson(v);
        } else
        {
            return QVariant::fromValue(v).toJsonValue();
        }
    }, val);
}

template<>
Utils::Result<ServerCapabilitiesTypeHierarchyProvider> fromJson<ServerCapabilitiesTypeHierarchyProvider>(const QJsonValue &val)
{
    if (val.isBool())
        return ServerCapabilitiesTypeHierarchyProvider(val.toBool());
    if (!val.isObject())
        return Utils::ResultError("Invalid ServerCapabilitiesTypeHierarchyProvider: expected object or array");
    const QJsonObject obj = val.toObject();
    if (obj.contains("documentSelector")) {
        const auto res0 = fromJson<TypeHierarchyRegistrationOptions>(val);
        if (!res0)
            return Utils::ResultError(res0.error());
        return ServerCapabilitiesTypeHierarchyProvider(*res0);
    }
    {
        auto result = fromJson<TypeHierarchyOptions>(val);
        if (result) return ServerCapabilitiesTypeHierarchyProvider(*result);
    }
    return Utils::ResultError("Invalid ServerCapabilitiesTypeHierarchyProvider");
}

QJsonValue toJsonValue(const ServerCapabilitiesTypeHierarchyProvider &val)
{
    return std::visit([](const auto &v) -> QJsonValue {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, TypeHierarchyOptions>) {
            return toJson(v);
        } else
        if constexpr (std::is_same_v<T, TypeHierarchyRegistrationOptions>) {
            return toJson(v);
        } else
        {
            return QVariant::fromValue(v).toJsonValue();
        }
    }, val);
}

template<>
Utils::Result<ServerCapabilitiesInlineValueProvider> fromJson<ServerCapabilitiesInlineValueProvider>(const QJsonValue &val)
{
    if (val.isBool())
        return ServerCapabilitiesInlineValueProvider(val.toBool());
    if (!val.isObject())
        return Utils::ResultError("Invalid ServerCapabilitiesInlineValueProvider: expected object or array");
    const QJsonObject obj = val.toObject();
    if (obj.contains("documentSelector")) {
        const auto res0 = fromJson<InlineValueRegistrationOptions>(val);
        if (!res0)
            return Utils::ResultError(res0.error());
        return ServerCapabilitiesInlineValueProvider(*res0);
    }
    {
        auto result = fromJson<InlineValueOptions>(val);
        if (result) return ServerCapabilitiesInlineValueProvider(*result);
    }
    return Utils::ResultError("Invalid ServerCapabilitiesInlineValueProvider");
}

QJsonValue toJsonValue(const ServerCapabilitiesInlineValueProvider &val)
{
    return std::visit([](const auto &v) -> QJsonValue {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, InlineValueOptions>) {
            return toJson(v);
        } else
        if constexpr (std::is_same_v<T, InlineValueRegistrationOptions>) {
            return toJson(v);
        } else
        {
            return QVariant::fromValue(v).toJsonValue();
        }
    }, val);
}

template<>
Utils::Result<ServerCapabilitiesInlayHintProvider> fromJson<ServerCapabilitiesInlayHintProvider>(const QJsonValue &val)
{
    if (val.isBool())
        return ServerCapabilitiesInlayHintProvider(val.toBool());
    if (!val.isObject())
        return Utils::ResultError("Invalid ServerCapabilitiesInlayHintProvider: expected object or array");
    const QJsonObject obj = val.toObject();
    if (obj.contains("documentSelector")) {
        const auto res0 = fromJson<InlayHintRegistrationOptions>(val);
        if (!res0)
            return Utils::ResultError(res0.error());
        return ServerCapabilitiesInlayHintProvider(*res0);
    }
    {
        auto result = fromJson<InlayHintOptions>(val);
        if (result) return ServerCapabilitiesInlayHintProvider(*result);
    }
    return Utils::ResultError("Invalid ServerCapabilitiesInlayHintProvider");
}

QJsonValue toJsonValue(const ServerCapabilitiesInlayHintProvider &val)
{
    return std::visit([](const auto &v) -> QJsonValue {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, InlayHintOptions>) {
            return toJson(v);
        } else
        if constexpr (std::is_same_v<T, InlayHintRegistrationOptions>) {
            return toJson(v);
        } else
        {
            return QVariant::fromValue(v).toJsonValue();
        }
    }, val);
}

template<>
Utils::Result<ServerCapabilitiesDiagnosticProvider> fromJson<ServerCapabilitiesDiagnosticProvider>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Invalid ServerCapabilitiesDiagnosticProvider: expected object or array");
    const QJsonObject obj = val.toObject();
    if (obj.contains("documentSelector")) {
        const auto res0 = fromJson<DiagnosticRegistrationOptions>(val);
        if (!res0)
            return Utils::ResultError(res0.error());
        return ServerCapabilitiesDiagnosticProvider(*res0);
    }
    {
        auto result = fromJson<DiagnosticOptions>(val);
        if (result) return ServerCapabilitiesDiagnosticProvider(*result);
    }
    return Utils::ResultError("Invalid ServerCapabilitiesDiagnosticProvider");
}

QJsonValue toJsonValue(const ServerCapabilitiesDiagnosticProvider &val)
{
    return std::visit([](const auto &v) -> QJsonValue {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, QJsonObject>) {
            return v;
        } else {
            return toJson(v);
        }
    }, val);
}

template<>
Utils::Result<ServerCapabilitiesInlineCompletionProvider> fromJson<ServerCapabilitiesInlineCompletionProvider>(const QJsonValue &val)
{
    if (val.isBool())
        return ServerCapabilitiesInlineCompletionProvider(val.toBool());
    {
        auto result = fromJson<InlineCompletionOptions>(val);
        if (result) return ServerCapabilitiesInlineCompletionProvider(*result);
    }
    return Utils::ResultError("Invalid ServerCapabilitiesInlineCompletionProvider");
}

QJsonValue toJsonValue(const ServerCapabilitiesInlineCompletionProvider &val)
{
    return std::visit([](const auto &v) -> QJsonValue {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, InlineCompletionOptions>) {
            return toJson(v);
        } else
        {
            return QVariant::fromValue(v).toJsonValue();
        }
    }, val);
}

template<>
Utils::Result<ServerCapabilities> fromJson<ServerCapabilities>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for ServerCapabilities");
    const QJsonObject obj = val.toObject();
    ServerCapabilities result;
    if (obj.contains("positionEncoding") && obj["positionEncoding"].isString()) {
        const auto res0 = fromJson<PositionEncodingKind>("positionEncoding", obj["positionEncoding"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._positionEncoding = *res0;
    }
    if (obj.contains("textDocumentSync")) {
        const auto res1 = fromJson<ServerCapabilitiesTextDocumentSync>("textDocumentSync", obj["textDocumentSync"]);
        if (!res1)
            return Utils::ResultError(res1.error());
        result._textDocumentSync = *res1;
    }
    if (obj.contains("notebookDocumentSync")) {
        const auto res2 = fromJson<ServerCapabilitiesNotebookDocumentSync>("notebookDocumentSync", obj["notebookDocumentSync"]);
        if (!res2)
            return Utils::ResultError(res2.error());
        result._notebookDocumentSync = *res2;
    }
    if (obj.contains("completionProvider") && obj["completionProvider"].isObject()) {
        const auto res3 = fromJson<CompletionOptions>("completionProvider", obj["completionProvider"]);
        if (!res3)
            return Utils::ResultError(res3.error());
        result._completionProvider = *res3;
    }
    if (obj.contains("hoverProvider")) {
        const auto res4 = fromJson<ServerCapabilitiesHoverProvider>("hoverProvider", obj["hoverProvider"]);
        if (!res4)
            return Utils::ResultError(res4.error());
        result._hoverProvider = *res4;
    }
    if (obj.contains("signatureHelpProvider") && obj["signatureHelpProvider"].isObject()) {
        const auto res5 = fromJson<SignatureHelpOptions>("signatureHelpProvider", obj["signatureHelpProvider"]);
        if (!res5)
            return Utils::ResultError(res5.error());
        result._signatureHelpProvider = *res5;
    }
    if (obj.contains("declarationProvider")) {
        const auto res6 = fromJson<ServerCapabilitiesDeclarationProvider>("declarationProvider", obj["declarationProvider"]);
        if (!res6)
            return Utils::ResultError(res6.error());
        result._declarationProvider = *res6;
    }
    if (obj.contains("definitionProvider")) {
        const auto res7 = fromJson<ServerCapabilitiesDefinitionProvider>("definitionProvider", obj["definitionProvider"]);
        if (!res7)
            return Utils::ResultError(res7.error());
        result._definitionProvider = *res7;
    }
    if (obj.contains("typeDefinitionProvider")) {
        const auto res8 = fromJson<ServerCapabilitiesTypeDefinitionProvider>("typeDefinitionProvider", obj["typeDefinitionProvider"]);
        if (!res8)
            return Utils::ResultError(res8.error());
        result._typeDefinitionProvider = *res8;
    }
    if (obj.contains("implementationProvider")) {
        const auto res9 = fromJson<ServerCapabilitiesImplementationProvider>("implementationProvider", obj["implementationProvider"]);
        if (!res9)
            return Utils::ResultError(res9.error());
        result._implementationProvider = *res9;
    }
    if (obj.contains("referencesProvider")) {
        const auto res10 = fromJson<ServerCapabilitiesReferencesProvider>("referencesProvider", obj["referencesProvider"]);
        if (!res10)
            return Utils::ResultError(res10.error());
        result._referencesProvider = *res10;
    }
    if (obj.contains("documentHighlightProvider")) {
        const auto res11 = fromJson<ServerCapabilitiesDocumentHighlightProvider>("documentHighlightProvider", obj["documentHighlightProvider"]);
        if (!res11)
            return Utils::ResultError(res11.error());
        result._documentHighlightProvider = *res11;
    }
    if (obj.contains("documentSymbolProvider")) {
        const auto res12 = fromJson<ServerCapabilitiesDocumentSymbolProvider>("documentSymbolProvider", obj["documentSymbolProvider"]);
        if (!res12)
            return Utils::ResultError(res12.error());
        result._documentSymbolProvider = *res12;
    }
    if (obj.contains("codeActionProvider")) {
        const auto res13 = fromJson<ServerCapabilitiesCodeActionProvider>("codeActionProvider", obj["codeActionProvider"]);
        if (!res13)
            return Utils::ResultError(res13.error());
        result._codeActionProvider = *res13;
    }
    if (obj.contains("codeLensProvider") && obj["codeLensProvider"].isObject()) {
        const auto res14 = fromJson<CodeLensOptions>("codeLensProvider", obj["codeLensProvider"]);
        if (!res14)
            return Utils::ResultError(res14.error());
        result._codeLensProvider = *res14;
    }
    if (obj.contains("documentLinkProvider") && obj["documentLinkProvider"].isObject()) {
        const auto res15 = fromJson<DocumentLinkOptions>("documentLinkProvider", obj["documentLinkProvider"]);
        if (!res15)
            return Utils::ResultError(res15.error());
        result._documentLinkProvider = *res15;
    }
    if (obj.contains("colorProvider")) {
        const auto res16 = fromJson<ServerCapabilitiesColorProvider>("colorProvider", obj["colorProvider"]);
        if (!res16)
            return Utils::ResultError(res16.error());
        result._colorProvider = *res16;
    }
    if (obj.contains("workspaceSymbolProvider")) {
        const auto res17 = fromJson<ServerCapabilitiesWorkspaceSymbolProvider>("workspaceSymbolProvider", obj["workspaceSymbolProvider"]);
        if (!res17)
            return Utils::ResultError(res17.error());
        result._workspaceSymbolProvider = *res17;
    }
    if (obj.contains("documentFormattingProvider")) {
        const auto res18 = fromJson<ServerCapabilitiesDocumentFormattingProvider>("documentFormattingProvider", obj["documentFormattingProvider"]);
        if (!res18)
            return Utils::ResultError(res18.error());
        result._documentFormattingProvider = *res18;
    }
    if (obj.contains("documentRangeFormattingProvider")) {
        const auto res19 = fromJson<ServerCapabilitiesDocumentRangeFormattingProvider>("documentRangeFormattingProvider", obj["documentRangeFormattingProvider"]);
        if (!res19)
            return Utils::ResultError(res19.error());
        result._documentRangeFormattingProvider = *res19;
    }
    if (obj.contains("documentOnTypeFormattingProvider") && obj["documentOnTypeFormattingProvider"].isObject()) {
        const auto res20 = fromJson<DocumentOnTypeFormattingOptions>("documentOnTypeFormattingProvider", obj["documentOnTypeFormattingProvider"]);
        if (!res20)
            return Utils::ResultError(res20.error());
        result._documentOnTypeFormattingProvider = *res20;
    }
    if (obj.contains("renameProvider")) {
        const auto res21 = fromJson<ServerCapabilitiesRenameProvider>("renameProvider", obj["renameProvider"]);
        if (!res21)
            return Utils::ResultError(res21.error());
        result._renameProvider = *res21;
    }
    if (obj.contains("foldingRangeProvider")) {
        const auto res22 = fromJson<ServerCapabilitiesFoldingRangeProvider>("foldingRangeProvider", obj["foldingRangeProvider"]);
        if (!res22)
            return Utils::ResultError(res22.error());
        result._foldingRangeProvider = *res22;
    }
    if (obj.contains("selectionRangeProvider")) {
        const auto res23 = fromJson<ServerCapabilitiesSelectionRangeProvider>("selectionRangeProvider", obj["selectionRangeProvider"]);
        if (!res23)
            return Utils::ResultError(res23.error());
        result._selectionRangeProvider = *res23;
    }
    if (obj.contains("executeCommandProvider") && obj["executeCommandProvider"].isObject()) {
        const auto res24 = fromJson<ExecuteCommandOptions>("executeCommandProvider", obj["executeCommandProvider"]);
        if (!res24)
            return Utils::ResultError(res24.error());
        result._executeCommandProvider = *res24;
    }
    if (obj.contains("callHierarchyProvider")) {
        const auto res25 = fromJson<ServerCapabilitiesCallHierarchyProvider>("callHierarchyProvider", obj["callHierarchyProvider"]);
        if (!res25)
            return Utils::ResultError(res25.error());
        result._callHierarchyProvider = *res25;
    }
    if (obj.contains("linkedEditingRangeProvider")) {
        const auto res26 = fromJson<ServerCapabilitiesLinkedEditingRangeProvider>("linkedEditingRangeProvider", obj["linkedEditingRangeProvider"]);
        if (!res26)
            return Utils::ResultError(res26.error());
        result._linkedEditingRangeProvider = *res26;
    }
    if (obj.contains("semanticTokensProvider")) {
        const auto res27 = fromJson<ServerCapabilitiesSemanticTokensProvider>("semanticTokensProvider", obj["semanticTokensProvider"]);
        if (!res27)
            return Utils::ResultError(res27.error());
        result._semanticTokensProvider = *res27;
    }
    if (obj.contains("monikerProvider")) {
        const auto res28 = fromJson<ServerCapabilitiesMonikerProvider>("monikerProvider", obj["monikerProvider"]);
        if (!res28)
            return Utils::ResultError(res28.error());
        result._monikerProvider = *res28;
    }
    if (obj.contains("typeHierarchyProvider")) {
        const auto res29 = fromJson<ServerCapabilitiesTypeHierarchyProvider>("typeHierarchyProvider", obj["typeHierarchyProvider"]);
        if (!res29)
            return Utils::ResultError(res29.error());
        result._typeHierarchyProvider = *res29;
    }
    if (obj.contains("inlineValueProvider")) {
        const auto res30 = fromJson<ServerCapabilitiesInlineValueProvider>("inlineValueProvider", obj["inlineValueProvider"]);
        if (!res30)
            return Utils::ResultError(res30.error());
        result._inlineValueProvider = *res30;
    }
    if (obj.contains("inlayHintProvider")) {
        const auto res31 = fromJson<ServerCapabilitiesInlayHintProvider>("inlayHintProvider", obj["inlayHintProvider"]);
        if (!res31)
            return Utils::ResultError(res31.error());
        result._inlayHintProvider = *res31;
    }
    if (obj.contains("diagnosticProvider")) {
        const auto res32 = fromJson<ServerCapabilitiesDiagnosticProvider>("diagnosticProvider", obj["diagnosticProvider"]);
        if (!res32)
            return Utils::ResultError(res32.error());
        result._diagnosticProvider = *res32;
    }
    if (obj.contains("inlineCompletionProvider")) {
        const auto res33 = fromJson<ServerCapabilitiesInlineCompletionProvider>("inlineCompletionProvider", obj["inlineCompletionProvider"]);
        if (!res33)
            return Utils::ResultError(res33.error());
        result._inlineCompletionProvider = *res33;
    }
    if (obj.contains("workspace") && obj["workspace"].isObject()) {
        const auto res34 = fromJson<WorkspaceOptions>("workspace", obj["workspace"]);
        if (!res34)
            return Utils::ResultError(res34.error());
        result._workspace = *res34;
    }
    if (obj.contains("experimental"))
        result._experimental = obj.value("experimental");
    return result;
}

QJsonObject toJson(const ServerCapabilities &data)
{
    QJsonObject obj;
    if (data._positionEncoding.has_value())
        obj.insert("positionEncoding", *data._positionEncoding);
    if (data._textDocumentSync.has_value())
        obj.insert("textDocumentSync", toJsonValue(*data._textDocumentSync));
    if (data._notebookDocumentSync.has_value())
        obj.insert("notebookDocumentSync", toJsonValue(*data._notebookDocumentSync));
    if (data._completionProvider.has_value())
        obj.insert("completionProvider", toJson(*data._completionProvider));
    if (data._hoverProvider.has_value())
        obj.insert("hoverProvider", toJsonValue(*data._hoverProvider));
    if (data._signatureHelpProvider.has_value())
        obj.insert("signatureHelpProvider", toJson(*data._signatureHelpProvider));
    if (data._declarationProvider.has_value())
        obj.insert("declarationProvider", toJsonValue(*data._declarationProvider));
    if (data._definitionProvider.has_value())
        obj.insert("definitionProvider", toJsonValue(*data._definitionProvider));
    if (data._typeDefinitionProvider.has_value())
        obj.insert("typeDefinitionProvider", toJsonValue(*data._typeDefinitionProvider));
    if (data._implementationProvider.has_value())
        obj.insert("implementationProvider", toJsonValue(*data._implementationProvider));
    if (data._referencesProvider.has_value())
        obj.insert("referencesProvider", toJsonValue(*data._referencesProvider));
    if (data._documentHighlightProvider.has_value())
        obj.insert("documentHighlightProvider", toJsonValue(*data._documentHighlightProvider));
    if (data._documentSymbolProvider.has_value())
        obj.insert("documentSymbolProvider", toJsonValue(*data._documentSymbolProvider));
    if (data._codeActionProvider.has_value())
        obj.insert("codeActionProvider", toJsonValue(*data._codeActionProvider));
    if (data._codeLensProvider.has_value())
        obj.insert("codeLensProvider", toJson(*data._codeLensProvider));
    if (data._documentLinkProvider.has_value())
        obj.insert("documentLinkProvider", toJson(*data._documentLinkProvider));
    if (data._colorProvider.has_value())
        obj.insert("colorProvider", toJsonValue(*data._colorProvider));
    if (data._workspaceSymbolProvider.has_value())
        obj.insert("workspaceSymbolProvider", toJsonValue(*data._workspaceSymbolProvider));
    if (data._documentFormattingProvider.has_value())
        obj.insert("documentFormattingProvider", toJsonValue(*data._documentFormattingProvider));
    if (data._documentRangeFormattingProvider.has_value())
        obj.insert("documentRangeFormattingProvider", toJsonValue(*data._documentRangeFormattingProvider));
    if (data._documentOnTypeFormattingProvider.has_value())
        obj.insert("documentOnTypeFormattingProvider", toJson(*data._documentOnTypeFormattingProvider));
    if (data._renameProvider.has_value())
        obj.insert("renameProvider", toJsonValue(*data._renameProvider));
    if (data._foldingRangeProvider.has_value())
        obj.insert("foldingRangeProvider", toJsonValue(*data._foldingRangeProvider));
    if (data._selectionRangeProvider.has_value())
        obj.insert("selectionRangeProvider", toJsonValue(*data._selectionRangeProvider));
    if (data._executeCommandProvider.has_value())
        obj.insert("executeCommandProvider", toJson(*data._executeCommandProvider));
    if (data._callHierarchyProvider.has_value())
        obj.insert("callHierarchyProvider", toJsonValue(*data._callHierarchyProvider));
    if (data._linkedEditingRangeProvider.has_value())
        obj.insert("linkedEditingRangeProvider", toJsonValue(*data._linkedEditingRangeProvider));
    if (data._semanticTokensProvider.has_value())
        obj.insert("semanticTokensProvider", toJsonValue(*data._semanticTokensProvider));
    if (data._monikerProvider.has_value())
        obj.insert("monikerProvider", toJsonValue(*data._monikerProvider));
    if (data._typeHierarchyProvider.has_value())
        obj.insert("typeHierarchyProvider", toJsonValue(*data._typeHierarchyProvider));
    if (data._inlineValueProvider.has_value())
        obj.insert("inlineValueProvider", toJsonValue(*data._inlineValueProvider));
    if (data._inlayHintProvider.has_value())
        obj.insert("inlayHintProvider", toJsonValue(*data._inlayHintProvider));
    if (data._diagnosticProvider.has_value())
        obj.insert("diagnosticProvider", toJsonValue(*data._diagnosticProvider));
    if (data._inlineCompletionProvider.has_value())
        obj.insert("inlineCompletionProvider", toJsonValue(*data._inlineCompletionProvider));
    if (data._workspace.has_value())
        obj.insert("workspace", toJson(*data._workspace));
    if (data._experimental.has_value())
        obj.insert("experimental", *data._experimental);
    return obj;
}

template<>
Utils::Result<ServerInfo> fromJson<ServerInfo>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for ServerInfo");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("name"))
        return Utils::ResultError("Missing required field: name");
    ServerInfo result;
    result._name = obj.value("name").toString();
    if (obj.contains("version"))
        result._version = obj.value("version").toString();
    return result;
}

QJsonObject toJson(const ServerInfo &data)
{
    QJsonObject obj{{"name", data._name}};
    if (data._version.has_value())
        obj.insert("version", *data._version);
    return obj;
}

template<>
Utils::Result<InitializeResult> fromJson<InitializeResult>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for InitializeResult");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("capabilities"))
        return Utils::ResultError("Missing required field: capabilities");
    InitializeResult result;
    if (obj.contains("capabilities") && obj["capabilities"].isObject()) {
        const auto res0 = fromJson<ServerCapabilities>("capabilities", obj["capabilities"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._capabilities = *res0;
    }
    if (obj.contains("serverInfo") && obj["serverInfo"].isObject()) {
        const auto res1 = fromJson<ServerInfo>("serverInfo", obj["serverInfo"]);
        if (!res1)
            return Utils::ResultError(res1.error());
        result._serverInfo = *res1;
    }
    return result;
}

QJsonObject toJson(const InitializeResult &data)
{
    QJsonObject obj{{"capabilities", toJson(data._capabilities)}};
    if (data._serverInfo.has_value())
        obj.insert("serverInfo", toJson(*data._serverInfo));
    return obj;
}

template<>
Utils::Result<InitializeError> fromJson<InitializeError>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for InitializeError");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("retry"))
        return Utils::ResultError("Missing required field: retry");
    InitializeError result;
    result._retry = obj.value("retry").toBool();
    return result;
}

QJsonObject toJson(const InitializeError &data)
{
    QJsonObject obj{{"retry", data._retry}};
    return obj;
}

template<>
Utils::Result<InitializedParams> fromJson<InitializedParams>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for InitializedParams");
    const QJsonObject obj = val.toObject();
    InitializedParams result;
    return result;
}

QJsonObject toJson(const InitializedParams &data)
{
    Q_UNUSED(data)
    QJsonObject obj;
    return obj;
}

template<>
Utils::Result<DidChangeConfigurationParams> fromJson<DidChangeConfigurationParams>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for DidChangeConfigurationParams");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("settings"))
        return Utils::ResultError("Missing required field: settings");
    DidChangeConfigurationParams result;
    result._settings = obj.value("settings");
    return result;
}

QJsonObject toJson(const DidChangeConfigurationParams &data)
{
    QJsonObject obj{{"settings", data._settings}};
    return obj;
}

template<>
Utils::Result<DidChangeConfigurationRegistrationOptions> fromJson<DidChangeConfigurationRegistrationOptions>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for DidChangeConfigurationRegistrationOptions");
    const QJsonObject obj = val.toObject();
    DidChangeConfigurationRegistrationOptions result;
    if (obj.contains("section"))
        result._section = obj.value("section").toString();
    return result;
}

QJsonObject toJson(const DidChangeConfigurationRegistrationOptions &data)
{
    QJsonObject obj;
    if (data._section.has_value())
        obj.insert("section", *data._section);
    return obj;
}

template<>
Utils::Result<ShowMessageParams> fromJson<ShowMessageParams>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for ShowMessageParams");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("type"))
        return Utils::ResultError("Missing required field: type");
    if (!obj.contains("message"))
        return Utils::ResultError("Missing required field: message");
    ShowMessageParams result;
    result._type = obj.value("type").toInt();
    result._message = obj.value("message").toString();
    return result;
}

QJsonObject toJson(const ShowMessageParams &data)
{
    QJsonObject obj{
        {"type", data._type},
        {"message", data._message}
    };
    return obj;
}

template<>
Utils::Result<MessageActionItem> fromJson<MessageActionItem>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for MessageActionItem");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("title"))
        return Utils::ResultError("Missing required field: title");
    MessageActionItem result;
    result._title = obj.value("title").toString();
    return result;
}

QJsonObject toJson(const MessageActionItem &data)
{
    QJsonObject obj{{"title", data._title}};
    return obj;
}

template<>
Utils::Result<ShowMessageRequestParams> fromJson<ShowMessageRequestParams>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for ShowMessageRequestParams");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("type"))
        return Utils::ResultError("Missing required field: type");
    if (!obj.contains("message"))
        return Utils::ResultError("Missing required field: message");
    ShowMessageRequestParams result;
    result._type = obj.value("type").toInt();
    result._message = obj.value("message").toString();
    if (obj.contains("actions") && obj["actions"].isArray()) {
        const QJsonArray arr = obj["actions"].toArray();
        QList<MessageActionItem> list_actions;
        for (const QJsonValue &v : arr) {
            const auto res0 = fromJson<MessageActionItem>("actions", v);
            if (!res0)
                return Utils::ResultError(res0.error());
            list_actions.append(*res0);
        }
        result._actions = list_actions;
    }
    return result;
}

QJsonObject toJson(const ShowMessageRequestParams &data)
{
    QJsonObject obj{
        {"type", data._type},
        {"message", data._message}
    };
    if (data._actions.has_value()) {
        QJsonArray arr_actions;
        for (const auto &v : *data._actions) arr_actions.append(toJson(v));
        obj.insert("actions", arr_actions);
    }
    return obj;
}

template<>
Utils::Result<LogMessageParams> fromJson<LogMessageParams>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for LogMessageParams");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("type"))
        return Utils::ResultError("Missing required field: type");
    if (!obj.contains("message"))
        return Utils::ResultError("Missing required field: message");
    LogMessageParams result;
    result._type = obj.value("type").toInt();
    result._message = obj.value("message").toString();
    return result;
}

QJsonObject toJson(const LogMessageParams &data)
{
    QJsonObject obj{
        {"type", data._type},
        {"message", data._message}
    };
    return obj;
}

template<>
Utils::Result<DidOpenTextDocumentParams> fromJson<DidOpenTextDocumentParams>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for DidOpenTextDocumentParams");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("textDocument"))
        return Utils::ResultError("Missing required field: textDocument");
    DidOpenTextDocumentParams result;
    if (obj.contains("textDocument") && obj["textDocument"].isObject()) {
        const auto res0 = fromJson<TextDocumentItem>("textDocument", obj["textDocument"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._textDocument = *res0;
    }
    return result;
}

QJsonObject toJson(const DidOpenTextDocumentParams &data)
{
    QJsonObject obj{{"textDocument", toJson(data._textDocument)}};
    return obj;
}

template<>
Utils::Result<TextDocumentRegistrationOptions> fromJson<TextDocumentRegistrationOptions>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for TextDocumentRegistrationOptions");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("documentSelector"))
        return Utils::ResultError("Missing required field: documentSelector");
    TextDocumentRegistrationOptions result;
    if (obj.contains("documentSelector") && !obj["documentSelector"].isNull()) {
        const auto res0 = fromJson<DocumentSelector>("documentSelector", obj["documentSelector"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._documentSelector = *res0;
    }
    return result;
}

QJsonObject toJson(const TextDocumentRegistrationOptions &data)
{
    QJsonObject obj;
    if (data._documentSelector.has_value())
        obj.insert("documentSelector", toJson(*data._documentSelector));
    else
        obj.insert("documentSelector", QJsonValue::Null);
    return obj;
}

template<>
Utils::Result<DidChangeTextDocumentParams> fromJson<DidChangeTextDocumentParams>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for DidChangeTextDocumentParams");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("textDocument"))
        return Utils::ResultError("Missing required field: textDocument");
    if (!obj.contains("contentChanges"))
        return Utils::ResultError("Missing required field: contentChanges");
    DidChangeTextDocumentParams result;
    if (obj.contains("textDocument") && obj["textDocument"].isObject()) {
        const auto res0 = fromJson<VersionedTextDocumentIdentifier>("textDocument", obj["textDocument"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._textDocument = *res0;
    }
    if (obj.contains("contentChanges") && obj["contentChanges"].isArray()) {
        const QJsonArray arr = obj["contentChanges"].toArray();
        for (const QJsonValue &v : arr) {
            const auto res1 = fromJson<TextDocumentContentChangeEvent>("contentChanges", v);
            if (!res1)
                return Utils::ResultError(res1.error());
            result._contentChanges.append(*res1);
        }
    }
    return result;
}

QJsonObject toJson(const DidChangeTextDocumentParams &data)
{
    QJsonObject obj{{"textDocument", toJson(data._textDocument)}};
    QJsonArray arr_contentChanges;
    for (const auto &v : data._contentChanges) arr_contentChanges.append(toJsonValue(v));
    obj.insert("contentChanges", arr_contentChanges);
    return obj;
}

template<>
Utils::Result<TextDocumentChangeRegistrationOptions> fromJson<TextDocumentChangeRegistrationOptions>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for TextDocumentChangeRegistrationOptions");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("documentSelector"))
        return Utils::ResultError("Missing required field: documentSelector");
    if (!obj.contains("syncKind"))
        return Utils::ResultError("Missing required field: syncKind");
    TextDocumentChangeRegistrationOptions result;
    if (obj.contains("documentSelector") && !obj["documentSelector"].isNull()) {
        const auto res0 = fromJson<DocumentSelector>("documentSelector", obj["documentSelector"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._documentSelector = *res0;
    }
    result._syncKind = obj.value("syncKind").toInt();
    return result;
}

QJsonObject toJson(const TextDocumentChangeRegistrationOptions &data)
{
    QJsonObject obj{{"syncKind", data._syncKind}};
    if (data._documentSelector.has_value())
        obj.insert("documentSelector", toJson(*data._documentSelector));
    else
        obj.insert("documentSelector", QJsonValue::Null);
    return obj;
}

template<>
Utils::Result<DidCloseTextDocumentParams> fromJson<DidCloseTextDocumentParams>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for DidCloseTextDocumentParams");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("textDocument"))
        return Utils::ResultError("Missing required field: textDocument");
    DidCloseTextDocumentParams result;
    if (obj.contains("textDocument") && obj["textDocument"].isObject()) {
        const auto res0 = fromJson<TextDocumentIdentifier>("textDocument", obj["textDocument"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._textDocument = *res0;
    }
    return result;
}

QJsonObject toJson(const DidCloseTextDocumentParams &data)
{
    QJsonObject obj{{"textDocument", toJson(data._textDocument)}};
    return obj;
}

template<>
Utils::Result<DidSaveTextDocumentParams> fromJson<DidSaveTextDocumentParams>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for DidSaveTextDocumentParams");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("textDocument"))
        return Utils::ResultError("Missing required field: textDocument");
    DidSaveTextDocumentParams result;
    if (obj.contains("textDocument") && obj["textDocument"].isObject()) {
        const auto res0 = fromJson<TextDocumentIdentifier>("textDocument", obj["textDocument"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._textDocument = *res0;
    }
    if (obj.contains("text"))
        result._text = obj.value("text").toString();
    return result;
}

QJsonObject toJson(const DidSaveTextDocumentParams &data)
{
    QJsonObject obj{{"textDocument", toJson(data._textDocument)}};
    if (data._text.has_value())
        obj.insert("text", *data._text);
    return obj;
}

template<>
Utils::Result<TextDocumentSaveRegistrationOptions> fromJson<TextDocumentSaveRegistrationOptions>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for TextDocumentSaveRegistrationOptions");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("documentSelector"))
        return Utils::ResultError("Missing required field: documentSelector");
    TextDocumentSaveRegistrationOptions result;
    if (obj.contains("documentSelector") && !obj["documentSelector"].isNull()) {
        const auto res0 = fromJson<DocumentSelector>("documentSelector", obj["documentSelector"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._documentSelector = *res0;
    }
    if (obj.contains("includeText"))
        result._includeText = obj.value("includeText").toBool();
    return result;
}

QJsonObject toJson(const TextDocumentSaveRegistrationOptions &data)
{
    QJsonObject obj;
    if (data._documentSelector.has_value())
        obj.insert("documentSelector", toJson(*data._documentSelector));
    else
        obj.insert("documentSelector", QJsonValue::Null);
    if (data._includeText.has_value())
        obj.insert("includeText", *data._includeText);
    return obj;
}

template<>
Utils::Result<WillSaveTextDocumentParams> fromJson<WillSaveTextDocumentParams>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for WillSaveTextDocumentParams");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("textDocument"))
        return Utils::ResultError("Missing required field: textDocument");
    if (!obj.contains("reason"))
        return Utils::ResultError("Missing required field: reason");
    WillSaveTextDocumentParams result;
    if (obj.contains("textDocument") && obj["textDocument"].isObject()) {
        const auto res0 = fromJson<TextDocumentIdentifier>("textDocument", obj["textDocument"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._textDocument = *res0;
    }
    result._reason = obj.value("reason").toInt();
    return result;
}

QJsonObject toJson(const WillSaveTextDocumentParams &data)
{
    QJsonObject obj{
        {"textDocument", toJson(data._textDocument)},
        {"reason", data._reason}
    };
    return obj;
}

template<>
Utils::Result<FileEvent> fromJson<FileEvent>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for FileEvent");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("uri"))
        return Utils::ResultError("Missing required field: uri");
    if (!obj.contains("type"))
        return Utils::ResultError("Missing required field: type");
    FileEvent result;
    result._uri = obj.value("uri").toString();
    result._type = obj.value("type").toInt();
    return result;
}

QJsonObject toJson(const FileEvent &data)
{
    QJsonObject obj{
        {"uri", data._uri},
        {"type", data._type}
    };
    return obj;
}

template<>
Utils::Result<DidChangeWatchedFilesParams> fromJson<DidChangeWatchedFilesParams>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for DidChangeWatchedFilesParams");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("changes"))
        return Utils::ResultError("Missing required field: changes");
    DidChangeWatchedFilesParams result;
    if (obj.contains("changes") && obj["changes"].isArray()) {
        const QJsonArray arr = obj["changes"].toArray();
        for (const QJsonValue &v : arr) {
            const auto res0 = fromJson<FileEvent>("changes", v);
            if (!res0)
                return Utils::ResultError(res0.error());
            result._changes.append(*res0);
        }
    }
    return result;
}

QJsonObject toJson(const DidChangeWatchedFilesParams &data)
{
    QJsonObject obj;
    QJsonArray arr_changes;
    for (const auto &v : data._changes) arr_changes.append(toJson(v));
    obj.insert("changes", arr_changes);
    return obj;
}

template<>
Utils::Result<FileSystemWatcher> fromJson<FileSystemWatcher>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for FileSystemWatcher");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("globPattern"))
        return Utils::ResultError("Missing required field: globPattern");
    FileSystemWatcher result;
    if (obj.contains("globPattern")) {
        const auto res0 = fromJson<GlobPattern>("globPattern", obj["globPattern"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._globPattern = *res0;
    }
    if (obj.contains("kind") && obj["kind"].isDouble()) {
        const auto res1 = fromJson<WatchKind>("kind", obj["kind"]);
        if (!res1)
            return Utils::ResultError(res1.error());
        result._kind = *res1;
    }
    return result;
}

QJsonObject toJson(const FileSystemWatcher &data)
{
    QJsonObject obj{{"globPattern", toJsonValue(data._globPattern)}};
    if (data._kind.has_value())
        obj.insert("kind", *data._kind);
    return obj;
}

template<>
Utils::Result<DidChangeWatchedFilesRegistrationOptions> fromJson<DidChangeWatchedFilesRegistrationOptions>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for DidChangeWatchedFilesRegistrationOptions");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("watchers"))
        return Utils::ResultError("Missing required field: watchers");
    DidChangeWatchedFilesRegistrationOptions result;
    if (obj.contains("watchers") && obj["watchers"].isArray()) {
        const QJsonArray arr = obj["watchers"].toArray();
        for (const QJsonValue &v : arr) {
            const auto res0 = fromJson<FileSystemWatcher>("watchers", v);
            if (!res0)
                return Utils::ResultError(res0.error());
            result._watchers.append(*res0);
        }
    }
    return result;
}

QJsonObject toJson(const DidChangeWatchedFilesRegistrationOptions &data)
{
    QJsonObject obj;
    QJsonArray arr_watchers;
    for (const auto &v : data._watchers) arr_watchers.append(toJson(v));
    obj.insert("watchers", arr_watchers);
    return obj;
}

template<>
Utils::Result<PublishDiagnosticsParams> fromJson<PublishDiagnosticsParams>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for PublishDiagnosticsParams");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("uri"))
        return Utils::ResultError("Missing required field: uri");
    if (!obj.contains("diagnostics"))
        return Utils::ResultError("Missing required field: diagnostics");
    PublishDiagnosticsParams result;
    result._uri = obj.value("uri").toString();
    if (obj.contains("version"))
        result._version = obj.value("version").toInt();
    if (obj.contains("diagnostics") && obj["diagnostics"].isArray()) {
        const QJsonArray arr = obj["diagnostics"].toArray();
        for (const QJsonValue &v : arr) {
            const auto res0 = fromJson<Diagnostic>("diagnostics", v);
            if (!res0)
                return Utils::ResultError(res0.error());
            result._diagnostics.append(*res0);
        }
    }
    return result;
}

QJsonObject toJson(const PublishDiagnosticsParams &data)
{
    QJsonObject obj{{"uri", data._uri}};
    if (data._version.has_value())
        obj.insert("version", *data._version);
    QJsonArray arr_diagnostics;
    for (const auto &v : data._diagnostics) arr_diagnostics.append(toJson(v));
    obj.insert("diagnostics", arr_diagnostics);
    return obj;
}

template<>
Utils::Result<CompletionContext> fromJson<CompletionContext>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for CompletionContext");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("triggerKind"))
        return Utils::ResultError("Missing required field: triggerKind");
    CompletionContext result;
    result._triggerKind = obj.value("triggerKind").toInt();
    if (obj.contains("triggerCharacter"))
        result._triggerCharacter = obj.value("triggerCharacter").toString();
    return result;
}

QJsonObject toJson(const CompletionContext &data)
{
    QJsonObject obj{{"triggerKind", data._triggerKind}};
    if (data._triggerCharacter.has_value())
        obj.insert("triggerCharacter", *data._triggerCharacter);
    return obj;
}

template<>
Utils::Result<CompletionParams> fromJson<CompletionParams>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for CompletionParams");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("textDocument"))
        return Utils::ResultError("Missing required field: textDocument");
    if (!obj.contains("position"))
        return Utils::ResultError("Missing required field: position");
    CompletionParams result;
    if (obj.contains("textDocument") && obj["textDocument"].isObject()) {
        const auto res0 = fromJson<TextDocumentIdentifier>("textDocument", obj["textDocument"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._textDocument = *res0;
    }
    if (obj.contains("position") && obj["position"].isObject()) {
        const auto res1 = fromJson<Position>("position", obj["position"]);
        if (!res1)
            return Utils::ResultError(res1.error());
        result._position = *res1;
    }
    if (obj.contains("workDoneToken")) {
        const auto res2 = fromJson<ProgressToken>("workDoneToken", obj["workDoneToken"]);
        if (!res2)
            return Utils::ResultError(res2.error());
        result._workDoneToken = *res2;
    }
    if (obj.contains("partialResultToken")) {
        const auto res3 = fromJson<ProgressToken>("partialResultToken", obj["partialResultToken"]);
        if (!res3)
            return Utils::ResultError(res3.error());
        result._partialResultToken = *res3;
    }
    if (obj.contains("context") && obj["context"].isObject()) {
        const auto res4 = fromJson<CompletionContext>("context", obj["context"]);
        if (!res4)
            return Utils::ResultError(res4.error());
        result._context = *res4;
    }
    return result;
}

QJsonObject toJson(const CompletionParams &data)
{
    QJsonObject obj{
        {"textDocument", toJson(data._textDocument)},
        {"position", toJson(data._position)}
    };
    if (data._workDoneToken.has_value())
        obj.insert("workDoneToken", toJsonValue(*data._workDoneToken));
    if (data._partialResultToken.has_value())
        obj.insert("partialResultToken", toJsonValue(*data._partialResultToken));
    if (data._context.has_value())
        obj.insert("context", toJson(*data._context));
    return obj;
}

template<>
Utils::Result<CompletionItemLabelDetails> fromJson<CompletionItemLabelDetails>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for CompletionItemLabelDetails");
    const QJsonObject obj = val.toObject();
    CompletionItemLabelDetails result;
    if (obj.contains("detail"))
        result._detail = obj.value("detail").toString();
    if (obj.contains("description"))
        result._description = obj.value("description").toString();
    return result;
}

QJsonObject toJson(const CompletionItemLabelDetails &data)
{
    QJsonObject obj;
    if (data._detail.has_value())
        obj.insert("detail", *data._detail);
    if (data._description.has_value())
        obj.insert("description", *data._description);
    return obj;
}

template<>
Utils::Result<InsertReplaceEdit> fromJson<InsertReplaceEdit>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for InsertReplaceEdit");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("newText"))
        return Utils::ResultError("Missing required field: newText");
    if (!obj.contains("insert"))
        return Utils::ResultError("Missing required field: insert");
    if (!obj.contains("replace"))
        return Utils::ResultError("Missing required field: replace");
    InsertReplaceEdit result;
    result._newText = obj.value("newText").toString();
    if (obj.contains("insert") && obj["insert"].isObject()) {
        const auto res0 = fromJson<Range>("insert", obj["insert"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._insert = *res0;
    }
    if (obj.contains("replace") && obj["replace"].isObject()) {
        const auto res1 = fromJson<Range>("replace", obj["replace"]);
        if (!res1)
            return Utils::ResultError(res1.error());
        result._replace = *res1;
    }
    return result;
}

QJsonObject toJson(const InsertReplaceEdit &data)
{
    QJsonObject obj{
        {"newText", data._newText},
        {"insert", toJson(data._insert)},
        {"replace", toJson(data._replace)}
    };
    return obj;
}

template<>
Utils::Result<CompletionItemTextEdit> fromJson<CompletionItemTextEdit>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Invalid CompletionItemTextEdit: expected object or array");
    const QJsonObject obj = val.toObject();
    if (obj.contains("range")) {
        const auto res0 = fromJson<TextEdit>(val);
        if (!res0)
            return Utils::ResultError(res0.error());
        return CompletionItemTextEdit(*res0);
    }
    if (obj.contains("insert")) {
        const auto res1 = fromJson<InsertReplaceEdit>(val);
        if (!res1)
            return Utils::ResultError(res1.error());
        return CompletionItemTextEdit(*res1);
    }
    return Utils::ResultError("Invalid CompletionItemTextEdit");
}

QJsonValue toJsonValue(const CompletionItemTextEdit &val)
{
    return std::visit([](const auto &v) -> QJsonValue {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, QJsonObject>) {
            return v;
        } else {
            return toJson(v);
        }
    }, val);
}

template<>
Utils::Result<CompletionItem> fromJson<CompletionItem>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for CompletionItem");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("label"))
        return Utils::ResultError("Missing required field: label");
    CompletionItem result;
    result._label = obj.value("label").toString();
    if (obj.contains("labelDetails") && obj["labelDetails"].isObject()) {
        const auto res0 = fromJson<CompletionItemLabelDetails>("labelDetails", obj["labelDetails"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._labelDetails = *res0;
    }
    if (obj.contains("kind") && obj["kind"].isDouble())
        result._kind = obj["kind"].toInt();
    if (obj.contains("tags") && obj["tags"].isArray()) {
        const QJsonArray arr = obj["tags"].toArray();
        QList<int> list_tags;
        for (const QJsonValue &v : arr) {
            list_tags.append(v.toInt());
        }
        result._tags = list_tags;
    }
    if (obj.contains("detail"))
        result._detail = obj.value("detail").toString();
    if (obj.contains("documentation")) {
        const auto res1 = fromJson<InlayHintLabelPartTooltip>("documentation", obj["documentation"]);
        if (!res1)
            return Utils::ResultError(res1.error());
        result._documentation = *res1;
    }
    if (obj.contains("deprecated"))
        result._deprecated = obj.value("deprecated").toBool();
    if (obj.contains("preselect"))
        result._preselect = obj.value("preselect").toBool();
    if (obj.contains("sortText"))
        result._sortText = obj.value("sortText").toString();
    if (obj.contains("filterText"))
        result._filterText = obj.value("filterText").toString();
    if (obj.contains("insertText"))
        result._insertText = obj.value("insertText").toString();
    if (obj.contains("insertTextFormat") && obj["insertTextFormat"].isDouble())
        result._insertTextFormat = obj["insertTextFormat"].toInt();
    if (obj.contains("insertTextMode") && obj["insertTextMode"].isDouble())
        result._insertTextMode = obj["insertTextMode"].toInt();
    if (obj.contains("textEdit")) {
        const auto res2 = fromJson<CompletionItemTextEdit>("textEdit", obj["textEdit"]);
        if (!res2)
            return Utils::ResultError(res2.error());
        result._textEdit = *res2;
    }
    if (obj.contains("textEditText"))
        result._textEditText = obj.value("textEditText").toString();
    if (obj.contains("additionalTextEdits") && obj["additionalTextEdits"].isArray()) {
        const QJsonArray arr = obj["additionalTextEdits"].toArray();
        QList<TextEdit> list_additionalTextEdits;
        for (const QJsonValue &v : arr) {
            const auto res3 = fromJson<TextEdit>("additionalTextEdits", v);
            if (!res3)
                return Utils::ResultError(res3.error());
            list_additionalTextEdits.append(*res3);
        }
        result._additionalTextEdits = list_additionalTextEdits;
    }
    if (obj.contains("commitCharacters") && obj["commitCharacters"].isArray()) {
        const QJsonArray arr = obj["commitCharacters"].toArray();
        QStringList list_commitCharacters;
        for (const QJsonValue &v : arr) {
            list_commitCharacters.append(v.toString());
        }
        result._commitCharacters = list_commitCharacters;
    }
    if (obj.contains("command") && obj["command"].isObject()) {
        const auto res4 = fromJson<Command>("command", obj["command"]);
        if (!res4)
            return Utils::ResultError(res4.error());
        result._command = *res4;
    }
    if (obj.contains("data"))
        result._data = obj.value("data");
    return result;
}

QJsonObject toJson(const CompletionItem &data)
{
    QJsonObject obj{{"label", data._label}};
    if (data._labelDetails.has_value())
        obj.insert("labelDetails", toJson(*data._labelDetails));
    if (data._kind.has_value())
        obj.insert("kind", *data._kind);
    if (data._tags.has_value()) {
        QJsonArray arr_tags;
        for (const auto &v : *data._tags) arr_tags.append(v);
        obj.insert("tags", arr_tags);
    }
    if (data._detail.has_value())
        obj.insert("detail", *data._detail);
    if (data._documentation.has_value())
        obj.insert("documentation", toJsonValue(*data._documentation));
    if (data._deprecated.has_value())
        obj.insert("deprecated", *data._deprecated);
    if (data._preselect.has_value())
        obj.insert("preselect", *data._preselect);
    if (data._sortText.has_value())
        obj.insert("sortText", *data._sortText);
    if (data._filterText.has_value())
        obj.insert("filterText", *data._filterText);
    if (data._insertText.has_value())
        obj.insert("insertText", *data._insertText);
    if (data._insertTextFormat.has_value())
        obj.insert("insertTextFormat", *data._insertTextFormat);
    if (data._insertTextMode.has_value())
        obj.insert("insertTextMode", *data._insertTextMode);
    if (data._textEdit.has_value())
        obj.insert("textEdit", toJsonValue(*data._textEdit));
    if (data._textEditText.has_value())
        obj.insert("textEditText", *data._textEditText);
    if (data._additionalTextEdits.has_value()) {
        QJsonArray arr_additionalTextEdits;
        for (const auto &v : *data._additionalTextEdits) arr_additionalTextEdits.append(toJson(v));
        obj.insert("additionalTextEdits", arr_additionalTextEdits);
    }
    if (data._commitCharacters.has_value()) {
        QJsonArray arr_commitCharacters;
        for (const auto &v : *data._commitCharacters) arr_commitCharacters.append(v);
        obj.insert("commitCharacters", arr_commitCharacters);
    }
    if (data._command.has_value())
        obj.insert("command", toJson(*data._command));
    if (data._data.has_value())
        obj.insert("data", *data._data);
    return obj;
}

template<>
Utils::Result<CompletionItemApplyKinds> fromJson<CompletionItemApplyKinds>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for CompletionItemApplyKinds");
    const QJsonObject obj = val.toObject();
    CompletionItemApplyKinds result;
    if (obj.contains("commitCharacters") && obj["commitCharacters"].isDouble())
        result._commitCharacters = obj["commitCharacters"].toInt();
    if (obj.contains("data") && obj["data"].isDouble())
        result._data = obj["data"].toInt();
    return result;
}

QJsonObject toJson(const CompletionItemApplyKinds &data)
{
    QJsonObject obj;
    if (data._commitCharacters.has_value())
        obj.insert("commitCharacters", *data._commitCharacters);
    if (data._data.has_value())
        obj.insert("data", *data._data);
    return obj;
}

template<>
Utils::Result<EditRangeWithInsertReplace> fromJson<EditRangeWithInsertReplace>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for EditRangeWithInsertReplace");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("insert"))
        return Utils::ResultError("Missing required field: insert");
    if (!obj.contains("replace"))
        return Utils::ResultError("Missing required field: replace");
    EditRangeWithInsertReplace result;
    if (obj.contains("insert") && obj["insert"].isObject()) {
        const auto res0 = fromJson<Range>("insert", obj["insert"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._insert = *res0;
    }
    if (obj.contains("replace") && obj["replace"].isObject()) {
        const auto res1 = fromJson<Range>("replace", obj["replace"]);
        if (!res1)
            return Utils::ResultError(res1.error());
        result._replace = *res1;
    }
    return result;
}

QJsonObject toJson(const EditRangeWithInsertReplace &data)
{
    QJsonObject obj{
        {"insert", toJson(data._insert)},
        {"replace", toJson(data._replace)}
    };
    return obj;
}

template<>
Utils::Result<CompletionItemDefaultsEditRange> fromJson<CompletionItemDefaultsEditRange>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Invalid CompletionItemDefaultsEditRange: expected object or array");
    const QJsonObject obj = val.toObject();
    if (obj.contains("end")) {
        const auto res0 = fromJson<Range>(val);
        if (!res0)
            return Utils::ResultError(res0.error());
        return CompletionItemDefaultsEditRange(*res0);
    }
    if (obj.contains("insert")) {
        const auto res1 = fromJson<EditRangeWithInsertReplace>(val);
        if (!res1)
            return Utils::ResultError(res1.error());
        return CompletionItemDefaultsEditRange(*res1);
    }
    return Utils::ResultError("Invalid CompletionItemDefaultsEditRange");
}

QJsonValue toJsonValue(const CompletionItemDefaultsEditRange &val)
{
    return std::visit([](const auto &v) -> QJsonValue {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, QJsonObject>) {
            return v;
        } else {
            return toJson(v);
        }
    }, val);
}

template<>
Utils::Result<CompletionItemDefaults> fromJson<CompletionItemDefaults>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for CompletionItemDefaults");
    const QJsonObject obj = val.toObject();
    CompletionItemDefaults result;
    if (obj.contains("commitCharacters") && obj["commitCharacters"].isArray()) {
        const QJsonArray arr = obj["commitCharacters"].toArray();
        QStringList list_commitCharacters;
        for (const QJsonValue &v : arr) {
            list_commitCharacters.append(v.toString());
        }
        result._commitCharacters = list_commitCharacters;
    }
    if (obj.contains("editRange")) {
        const auto res0 = fromJson<CompletionItemDefaultsEditRange>("editRange", obj["editRange"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._editRange = *res0;
    }
    if (obj.contains("insertTextFormat") && obj["insertTextFormat"].isDouble())
        result._insertTextFormat = obj["insertTextFormat"].toInt();
    if (obj.contains("insertTextMode") && obj["insertTextMode"].isDouble())
        result._insertTextMode = obj["insertTextMode"].toInt();
    if (obj.contains("data"))
        result._data = obj.value("data");
    return result;
}

QJsonObject toJson(const CompletionItemDefaults &data)
{
    QJsonObject obj;
    if (data._commitCharacters.has_value()) {
        QJsonArray arr_commitCharacters;
        for (const auto &v : *data._commitCharacters) arr_commitCharacters.append(v);
        obj.insert("commitCharacters", arr_commitCharacters);
    }
    if (data._editRange.has_value())
        obj.insert("editRange", toJsonValue(*data._editRange));
    if (data._insertTextFormat.has_value())
        obj.insert("insertTextFormat", *data._insertTextFormat);
    if (data._insertTextMode.has_value())
        obj.insert("insertTextMode", *data._insertTextMode);
    if (data._data.has_value())
        obj.insert("data", *data._data);
    return obj;
}

template<>
Utils::Result<CompletionList> fromJson<CompletionList>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for CompletionList");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("isIncomplete"))
        return Utils::ResultError("Missing required field: isIncomplete");
    if (!obj.contains("items"))
        return Utils::ResultError("Missing required field: items");
    CompletionList result;
    result._isIncomplete = obj.value("isIncomplete").toBool();
    if (obj.contains("itemDefaults") && obj["itemDefaults"].isObject()) {
        const auto res0 = fromJson<CompletionItemDefaults>("itemDefaults", obj["itemDefaults"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._itemDefaults = *res0;
    }
    if (obj.contains("applyKind") && obj["applyKind"].isObject()) {
        const auto res1 = fromJson<CompletionItemApplyKinds>("applyKind", obj["applyKind"]);
        if (!res1)
            return Utils::ResultError(res1.error());
        result._applyKind = *res1;
    }
    if (obj.contains("items") && obj["items"].isArray()) {
        const QJsonArray arr = obj["items"].toArray();
        for (const QJsonValue &v : arr) {
            const auto res2 = fromJson<CompletionItem>("items", v);
            if (!res2)
                return Utils::ResultError(res2.error());
            result._items.append(*res2);
        }
    }
    return result;
}

QJsonObject toJson(const CompletionList &data)
{
    QJsonObject obj{{"isIncomplete", data._isIncomplete}};
    if (data._itemDefaults.has_value())
        obj.insert("itemDefaults", toJson(*data._itemDefaults));
    if (data._applyKind.has_value())
        obj.insert("applyKind", toJson(*data._applyKind));
    QJsonArray arr_items;
    for (const auto &v : data._items) arr_items.append(toJson(v));
    obj.insert("items", arr_items);
    return obj;
}

template<>
Utils::Result<CompletionRegistrationOptions> fromJson<CompletionRegistrationOptions>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for CompletionRegistrationOptions");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("documentSelector"))
        return Utils::ResultError("Missing required field: documentSelector");
    CompletionRegistrationOptions result;
    if (obj.contains("documentSelector") && !obj["documentSelector"].isNull()) {
        const auto res0 = fromJson<DocumentSelector>("documentSelector", obj["documentSelector"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._documentSelector = *res0;
    }
    if (obj.contains("workDoneProgress"))
        result._workDoneProgress = obj.value("workDoneProgress").toBool();
    if (obj.contains("triggerCharacters") && obj["triggerCharacters"].isArray()) {
        const QJsonArray arr = obj["triggerCharacters"].toArray();
        QStringList list_triggerCharacters;
        for (const QJsonValue &v : arr) {
            list_triggerCharacters.append(v.toString());
        }
        result._triggerCharacters = list_triggerCharacters;
    }
    if (obj.contains("allCommitCharacters") && obj["allCommitCharacters"].isArray()) {
        const QJsonArray arr = obj["allCommitCharacters"].toArray();
        QStringList list_allCommitCharacters;
        for (const QJsonValue &v : arr) {
            list_allCommitCharacters.append(v.toString());
        }
        result._allCommitCharacters = list_allCommitCharacters;
    }
    if (obj.contains("resolveProvider"))
        result._resolveProvider = obj.value("resolveProvider").toBool();
    if (obj.contains("completionItem") && obj["completionItem"].isObject()) {
        const auto res1 = fromJson<ServerCompletionItemOptions>("completionItem", obj["completionItem"]);
        if (!res1)
            return Utils::ResultError(res1.error());
        result._completionItem = *res1;
    }
    return result;
}

QJsonObject toJson(const CompletionRegistrationOptions &data)
{
    QJsonObject obj;
    if (data._documentSelector.has_value())
        obj.insert("documentSelector", toJson(*data._documentSelector));
    else
        obj.insert("documentSelector", QJsonValue::Null);
    if (data._workDoneProgress.has_value())
        obj.insert("workDoneProgress", *data._workDoneProgress);
    if (data._triggerCharacters.has_value()) {
        QJsonArray arr_triggerCharacters;
        for (const auto &v : *data._triggerCharacters) arr_triggerCharacters.append(v);
        obj.insert("triggerCharacters", arr_triggerCharacters);
    }
    if (data._allCommitCharacters.has_value()) {
        QJsonArray arr_allCommitCharacters;
        for (const auto &v : *data._allCommitCharacters) arr_allCommitCharacters.append(v);
        obj.insert("allCommitCharacters", arr_allCommitCharacters);
    }
    if (data._resolveProvider.has_value())
        obj.insert("resolveProvider", *data._resolveProvider);
    if (data._completionItem.has_value())
        obj.insert("completionItem", toJson(*data._completionItem));
    return obj;
}

template<>
Utils::Result<HoverParams> fromJson<HoverParams>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for HoverParams");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("textDocument"))
        return Utils::ResultError("Missing required field: textDocument");
    if (!obj.contains("position"))
        return Utils::ResultError("Missing required field: position");
    HoverParams result;
    if (obj.contains("textDocument") && obj["textDocument"].isObject()) {
        const auto res0 = fromJson<TextDocumentIdentifier>("textDocument", obj["textDocument"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._textDocument = *res0;
    }
    if (obj.contains("position") && obj["position"].isObject()) {
        const auto res1 = fromJson<Position>("position", obj["position"]);
        if (!res1)
            return Utils::ResultError(res1.error());
        result._position = *res1;
    }
    if (obj.contains("workDoneToken")) {
        const auto res2 = fromJson<ProgressToken>("workDoneToken", obj["workDoneToken"]);
        if (!res2)
            return Utils::ResultError(res2.error());
        result._workDoneToken = *res2;
    }
    return result;
}

QJsonObject toJson(const HoverParams &data)
{
    QJsonObject obj{
        {"textDocument", toJson(data._textDocument)},
        {"position", toJson(data._position)}
    };
    if (data._workDoneToken.has_value())
        obj.insert("workDoneToken", toJsonValue(*data._workDoneToken));
    return obj;
}

template<>
Utils::Result<MarkedStringWithLanguage> fromJson<MarkedStringWithLanguage>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for MarkedStringWithLanguage");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("language"))
        return Utils::ResultError("Missing required field: language");
    if (!obj.contains("value"))
        return Utils::ResultError("Missing required field: value");
    MarkedStringWithLanguage result;
    result._language = obj.value("language").toString();
    result._value = obj.value("value").toString();
    return result;
}

QJsonObject toJson(const MarkedStringWithLanguage &data)
{
    QJsonObject obj{
        {"language", data._language},
        {"value", data._value}
    };
    return obj;
}

template<>
Utils::Result<MarkedString> fromJson<MarkedString>(const QJsonValue &val)
{
    if (val.isString())
        return MarkedString(val.toString());
    if (!val.isObject())
        return Utils::ResultError("Invalid MarkedString: expected object");
    const QJsonObject obj = val.toObject();
    if (obj.contains("language")) {
        const auto res0 = fromJson<MarkedStringWithLanguage>(val);
        if (!res0)
            return Utils::ResultError(res0.error());
        return MarkedString(*res0);
    }
    return Utils::ResultError("Invalid MarkedString");
}

QJsonValue toJsonValue(const MarkedString &val)
{
    return std::visit([](const auto &v) -> QJsonValue {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, MarkedStringWithLanguage>) {
            return toJson(v);
        } else
        {
            return QVariant::fromValue(v).toJsonValue();
        }
    }, val);
}

template<>
Utils::Result<HoverContents> fromJson<HoverContents>(const QJsonValue &val)
{
    if (val.isArray()) {
        QList<MarkedString> list;
        for (const QJsonValue &v : val.toArray()) {
            const auto res0 = fromJson<MarkedString>(v);
            if (!res0)
                return Utils::ResultError(res0.error());
            list.append(*res0);
        }
        return HoverContents(std::move(list));
    }
    if (!val.isObject())
        return Utils::ResultError("Invalid HoverContents: expected object or array");
    const QJsonObject obj = val.toObject();
    if (obj.contains("kind")) {
        const auto res1 = fromJson<MarkupContent>(val);
        if (!res1)
            return Utils::ResultError(res1.error());
        return HoverContents(*res1);
    }
    {
        auto result = fromJson<MarkedString>(val);
        if (result) return HoverContents(*result);
    }
    return Utils::ResultError("Invalid HoverContents");
}

QJsonValue toJsonValue(const HoverContents &val)
{
    return std::visit([](const auto &v) -> QJsonValue {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, MarkupContent>) {
            return toJson(v);
        } else
        if constexpr (std::is_same_v<T, MarkedString>) {
            return toJsonValue(v);
        } else
        if constexpr (std::is_same_v<T, QList<MarkedString>>) {
            QJsonArray arr;
            for (const auto &elem : v)
                arr.append(toJsonValue(elem));
            return arr;
        } else
        {
            return QVariant::fromValue(v).toJsonValue();
        }
    }, val);
}

template<>
Utils::Result<Hover> fromJson<Hover>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for Hover");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("contents"))
        return Utils::ResultError("Missing required field: contents");
    Hover result;
    if (obj.contains("contents")) {
        const auto res0 = fromJson<HoverContents>("contents", obj["contents"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._contents = *res0;
    }
    if (obj.contains("range") && obj["range"].isObject()) {
        const auto res1 = fromJson<Range>("range", obj["range"]);
        if (!res1)
            return Utils::ResultError(res1.error());
        result._range = *res1;
    }
    return result;
}

QJsonObject toJson(const Hover &data)
{
    QJsonObject obj{{"contents", toJsonValue(data._contents)}};
    if (data._range.has_value())
        obj.insert("range", toJson(*data._range));
    return obj;
}

template<>
Utils::Result<HoverRegistrationOptions> fromJson<HoverRegistrationOptions>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for HoverRegistrationOptions");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("documentSelector"))
        return Utils::ResultError("Missing required field: documentSelector");
    HoverRegistrationOptions result;
    if (obj.contains("documentSelector") && !obj["documentSelector"].isNull()) {
        const auto res0 = fromJson<DocumentSelector>("documentSelector", obj["documentSelector"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._documentSelector = *res0;
    }
    if (obj.contains("workDoneProgress"))
        result._workDoneProgress = obj.value("workDoneProgress").toBool();
    return result;
}

QJsonObject toJson(const HoverRegistrationOptions &data)
{
    QJsonObject obj;
    if (data._documentSelector.has_value())
        obj.insert("documentSelector", toJson(*data._documentSelector));
    else
        obj.insert("documentSelector", QJsonValue::Null);
    if (data._workDoneProgress.has_value())
        obj.insert("workDoneProgress", *data._workDoneProgress);
    return obj;
}

template<>
Utils::Result<ParameterInformation> fromJson<ParameterInformation>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for ParameterInformation");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("label"))
        return Utils::ResultError("Missing required field: label");
    ParameterInformation result;
    result._label = obj.value("label").toString();
    if (obj.contains("documentation")) {
        const auto res0 = fromJson<InlayHintLabelPartTooltip>("documentation", obj["documentation"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._documentation = *res0;
    }
    return result;
}

QJsonObject toJson(const ParameterInformation &data)
{
    QJsonObject obj{{"label", data._label}};
    if (data._documentation.has_value())
        obj.insert("documentation", toJsonValue(*data._documentation));
    return obj;
}

template<>
Utils::Result<SignatureInformation> fromJson<SignatureInformation>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for SignatureInformation");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("label"))
        return Utils::ResultError("Missing required field: label");
    SignatureInformation result;
    result._label = obj.value("label").toString();
    if (obj.contains("documentation")) {
        const auto res0 = fromJson<InlayHintLabelPartTooltip>("documentation", obj["documentation"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._documentation = *res0;
    }
    if (obj.contains("parameters") && obj["parameters"].isArray()) {
        const QJsonArray arr = obj["parameters"].toArray();
        QList<ParameterInformation> list_parameters;
        for (const QJsonValue &v : arr) {
            const auto res1 = fromJson<ParameterInformation>("parameters", v);
            if (!res1)
                return Utils::ResultError(res1.error());
            list_parameters.append(*res1);
        }
        result._parameters = list_parameters;
    }
    if (obj.contains("activeParameter"))
        if (!obj["activeParameter"].isNull()) {
            result._activeParameter = obj.value("activeParameter").toInt();
        }
    return result;
}

QJsonObject toJson(const SignatureInformation &data)
{
    QJsonObject obj{{"label", data._label}};
    if (data._documentation.has_value())
        obj.insert("documentation", toJsonValue(*data._documentation));
    if (data._parameters.has_value()) {
        QJsonArray arr_parameters;
        for (const auto &v : *data._parameters) arr_parameters.append(toJson(v));
        obj.insert("parameters", arr_parameters);
    }
    if (data._activeParameter.has_value())
        obj.insert("activeParameter", *data._activeParameter);
    return obj;
}

template<>
Utils::Result<SignatureHelp> fromJson<SignatureHelp>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for SignatureHelp");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("signatures"))
        return Utils::ResultError("Missing required field: signatures");
    SignatureHelp result;
    if (obj.contains("signatures") && obj["signatures"].isArray()) {
        const QJsonArray arr = obj["signatures"].toArray();
        for (const QJsonValue &v : arr) {
            const auto res0 = fromJson<SignatureInformation>("signatures", v);
            if (!res0)
                return Utils::ResultError(res0.error());
            result._signatures.append(*res0);
        }
    }
    if (obj.contains("activeSignature"))
        result._activeSignature = obj.value("activeSignature").toInt();
    if (obj.contains("activeParameter"))
        if (!obj["activeParameter"].isNull()) {
            result._activeParameter = obj.value("activeParameter").toInt();
        }
    return result;
}

QJsonObject toJson(const SignatureHelp &data)
{
    QJsonObject obj;
    QJsonArray arr_signatures;
    for (const auto &v : data._signatures) arr_signatures.append(toJson(v));
    obj.insert("signatures", arr_signatures);
    if (data._activeSignature.has_value())
        obj.insert("activeSignature", *data._activeSignature);
    if (data._activeParameter.has_value())
        obj.insert("activeParameter", *data._activeParameter);
    return obj;
}

template<>
Utils::Result<SignatureHelpContext> fromJson<SignatureHelpContext>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for SignatureHelpContext");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("triggerKind"))
        return Utils::ResultError("Missing required field: triggerKind");
    if (!obj.contains("isRetrigger"))
        return Utils::ResultError("Missing required field: isRetrigger");
    SignatureHelpContext result;
    result._triggerKind = obj.value("triggerKind").toInt();
    if (obj.contains("triggerCharacter"))
        result._triggerCharacter = obj.value("triggerCharacter").toString();
    result._isRetrigger = obj.value("isRetrigger").toBool();
    if (obj.contains("activeSignatureHelp") && obj["activeSignatureHelp"].isObject()) {
        const auto res0 = fromJson<SignatureHelp>("activeSignatureHelp", obj["activeSignatureHelp"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._activeSignatureHelp = *res0;
    }
    return result;
}

QJsonObject toJson(const SignatureHelpContext &data)
{
    QJsonObject obj{
        {"triggerKind", data._triggerKind},
        {"isRetrigger", data._isRetrigger}
    };
    if (data._triggerCharacter.has_value())
        obj.insert("triggerCharacter", *data._triggerCharacter);
    if (data._activeSignatureHelp.has_value())
        obj.insert("activeSignatureHelp", toJson(*data._activeSignatureHelp));
    return obj;
}

template<>
Utils::Result<SignatureHelpParams> fromJson<SignatureHelpParams>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for SignatureHelpParams");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("textDocument"))
        return Utils::ResultError("Missing required field: textDocument");
    if (!obj.contains("position"))
        return Utils::ResultError("Missing required field: position");
    SignatureHelpParams result;
    if (obj.contains("textDocument") && obj["textDocument"].isObject()) {
        const auto res0 = fromJson<TextDocumentIdentifier>("textDocument", obj["textDocument"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._textDocument = *res0;
    }
    if (obj.contains("position") && obj["position"].isObject()) {
        const auto res1 = fromJson<Position>("position", obj["position"]);
        if (!res1)
            return Utils::ResultError(res1.error());
        result._position = *res1;
    }
    if (obj.contains("workDoneToken")) {
        const auto res2 = fromJson<ProgressToken>("workDoneToken", obj["workDoneToken"]);
        if (!res2)
            return Utils::ResultError(res2.error());
        result._workDoneToken = *res2;
    }
    if (obj.contains("context") && obj["context"].isObject()) {
        const auto res3 = fromJson<SignatureHelpContext>("context", obj["context"]);
        if (!res3)
            return Utils::ResultError(res3.error());
        result._context = *res3;
    }
    return result;
}

QJsonObject toJson(const SignatureHelpParams &data)
{
    QJsonObject obj{
        {"textDocument", toJson(data._textDocument)},
        {"position", toJson(data._position)}
    };
    if (data._workDoneToken.has_value())
        obj.insert("workDoneToken", toJsonValue(*data._workDoneToken));
    if (data._context.has_value())
        obj.insert("context", toJson(*data._context));
    return obj;
}

template<>
Utils::Result<SignatureHelpRegistrationOptions> fromJson<SignatureHelpRegistrationOptions>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for SignatureHelpRegistrationOptions");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("documentSelector"))
        return Utils::ResultError("Missing required field: documentSelector");
    SignatureHelpRegistrationOptions result;
    if (obj.contains("documentSelector") && !obj["documentSelector"].isNull()) {
        const auto res0 = fromJson<DocumentSelector>("documentSelector", obj["documentSelector"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._documentSelector = *res0;
    }
    if (obj.contains("workDoneProgress"))
        result._workDoneProgress = obj.value("workDoneProgress").toBool();
    if (obj.contains("triggerCharacters") && obj["triggerCharacters"].isArray()) {
        const QJsonArray arr = obj["triggerCharacters"].toArray();
        QStringList list_triggerCharacters;
        for (const QJsonValue &v : arr) {
            list_triggerCharacters.append(v.toString());
        }
        result._triggerCharacters = list_triggerCharacters;
    }
    if (obj.contains("retriggerCharacters") && obj["retriggerCharacters"].isArray()) {
        const QJsonArray arr = obj["retriggerCharacters"].toArray();
        QStringList list_retriggerCharacters;
        for (const QJsonValue &v : arr) {
            list_retriggerCharacters.append(v.toString());
        }
        result._retriggerCharacters = list_retriggerCharacters;
    }
    return result;
}

QJsonObject toJson(const SignatureHelpRegistrationOptions &data)
{
    QJsonObject obj;
    if (data._documentSelector.has_value())
        obj.insert("documentSelector", toJson(*data._documentSelector));
    else
        obj.insert("documentSelector", QJsonValue::Null);
    if (data._workDoneProgress.has_value())
        obj.insert("workDoneProgress", *data._workDoneProgress);
    if (data._triggerCharacters.has_value()) {
        QJsonArray arr_triggerCharacters;
        for (const auto &v : *data._triggerCharacters) arr_triggerCharacters.append(v);
        obj.insert("triggerCharacters", arr_triggerCharacters);
    }
    if (data._retriggerCharacters.has_value()) {
        QJsonArray arr_retriggerCharacters;
        for (const auto &v : *data._retriggerCharacters) arr_retriggerCharacters.append(v);
        obj.insert("retriggerCharacters", arr_retriggerCharacters);
    }
    return obj;
}

template<>
Utils::Result<DefinitionParams> fromJson<DefinitionParams>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for DefinitionParams");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("textDocument"))
        return Utils::ResultError("Missing required field: textDocument");
    if (!obj.contains("position"))
        return Utils::ResultError("Missing required field: position");
    DefinitionParams result;
    if (obj.contains("textDocument") && obj["textDocument"].isObject()) {
        const auto res0 = fromJson<TextDocumentIdentifier>("textDocument", obj["textDocument"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._textDocument = *res0;
    }
    if (obj.contains("position") && obj["position"].isObject()) {
        const auto res1 = fromJson<Position>("position", obj["position"]);
        if (!res1)
            return Utils::ResultError(res1.error());
        result._position = *res1;
    }
    if (obj.contains("workDoneToken")) {
        const auto res2 = fromJson<ProgressToken>("workDoneToken", obj["workDoneToken"]);
        if (!res2)
            return Utils::ResultError(res2.error());
        result._workDoneToken = *res2;
    }
    if (obj.contains("partialResultToken")) {
        const auto res3 = fromJson<ProgressToken>("partialResultToken", obj["partialResultToken"]);
        if (!res3)
            return Utils::ResultError(res3.error());
        result._partialResultToken = *res3;
    }
    return result;
}

QJsonObject toJson(const DefinitionParams &data)
{
    QJsonObject obj{
        {"textDocument", toJson(data._textDocument)},
        {"position", toJson(data._position)}
    };
    if (data._workDoneToken.has_value())
        obj.insert("workDoneToken", toJsonValue(*data._workDoneToken));
    if (data._partialResultToken.has_value())
        obj.insert("partialResultToken", toJsonValue(*data._partialResultToken));
    return obj;
}

template<>
Utils::Result<DefinitionRegistrationOptions> fromJson<DefinitionRegistrationOptions>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for DefinitionRegistrationOptions");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("documentSelector"))
        return Utils::ResultError("Missing required field: documentSelector");
    DefinitionRegistrationOptions result;
    if (obj.contains("documentSelector") && !obj["documentSelector"].isNull()) {
        const auto res0 = fromJson<DocumentSelector>("documentSelector", obj["documentSelector"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._documentSelector = *res0;
    }
    if (obj.contains("workDoneProgress"))
        result._workDoneProgress = obj.value("workDoneProgress").toBool();
    return result;
}

QJsonObject toJson(const DefinitionRegistrationOptions &data)
{
    QJsonObject obj;
    if (data._documentSelector.has_value())
        obj.insert("documentSelector", toJson(*data._documentSelector));
    else
        obj.insert("documentSelector", QJsonValue::Null);
    if (data._workDoneProgress.has_value())
        obj.insert("workDoneProgress", *data._workDoneProgress);
    return obj;
}

template<>
Utils::Result<ReferenceContext> fromJson<ReferenceContext>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for ReferenceContext");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("includeDeclaration"))
        return Utils::ResultError("Missing required field: includeDeclaration");
    ReferenceContext result;
    result._includeDeclaration = obj.value("includeDeclaration").toBool();
    return result;
}

QJsonObject toJson(const ReferenceContext &data)
{
    QJsonObject obj{{"includeDeclaration", data._includeDeclaration}};
    return obj;
}

template<>
Utils::Result<ReferenceParams> fromJson<ReferenceParams>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for ReferenceParams");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("textDocument"))
        return Utils::ResultError("Missing required field: textDocument");
    if (!obj.contains("position"))
        return Utils::ResultError("Missing required field: position");
    if (!obj.contains("context"))
        return Utils::ResultError("Missing required field: context");
    ReferenceParams result;
    if (obj.contains("textDocument") && obj["textDocument"].isObject()) {
        const auto res0 = fromJson<TextDocumentIdentifier>("textDocument", obj["textDocument"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._textDocument = *res0;
    }
    if (obj.contains("position") && obj["position"].isObject()) {
        const auto res1 = fromJson<Position>("position", obj["position"]);
        if (!res1)
            return Utils::ResultError(res1.error());
        result._position = *res1;
    }
    if (obj.contains("workDoneToken")) {
        const auto res2 = fromJson<ProgressToken>("workDoneToken", obj["workDoneToken"]);
        if (!res2)
            return Utils::ResultError(res2.error());
        result._workDoneToken = *res2;
    }
    if (obj.contains("partialResultToken")) {
        const auto res3 = fromJson<ProgressToken>("partialResultToken", obj["partialResultToken"]);
        if (!res3)
            return Utils::ResultError(res3.error());
        result._partialResultToken = *res3;
    }
    if (obj.contains("context") && obj["context"].isObject()) {
        const auto res4 = fromJson<ReferenceContext>("context", obj["context"]);
        if (!res4)
            return Utils::ResultError(res4.error());
        result._context = *res4;
    }
    return result;
}

QJsonObject toJson(const ReferenceParams &data)
{
    QJsonObject obj{
        {"textDocument", toJson(data._textDocument)},
        {"position", toJson(data._position)},
        {"context", toJson(data._context)}
    };
    if (data._workDoneToken.has_value())
        obj.insert("workDoneToken", toJsonValue(*data._workDoneToken));
    if (data._partialResultToken.has_value())
        obj.insert("partialResultToken", toJsonValue(*data._partialResultToken));
    return obj;
}

template<>
Utils::Result<ReferenceRegistrationOptions> fromJson<ReferenceRegistrationOptions>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for ReferenceRegistrationOptions");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("documentSelector"))
        return Utils::ResultError("Missing required field: documentSelector");
    ReferenceRegistrationOptions result;
    if (obj.contains("documentSelector") && !obj["documentSelector"].isNull()) {
        const auto res0 = fromJson<DocumentSelector>("documentSelector", obj["documentSelector"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._documentSelector = *res0;
    }
    if (obj.contains("workDoneProgress"))
        result._workDoneProgress = obj.value("workDoneProgress").toBool();
    return result;
}

QJsonObject toJson(const ReferenceRegistrationOptions &data)
{
    QJsonObject obj;
    if (data._documentSelector.has_value())
        obj.insert("documentSelector", toJson(*data._documentSelector));
    else
        obj.insert("documentSelector", QJsonValue::Null);
    if (data._workDoneProgress.has_value())
        obj.insert("workDoneProgress", *data._workDoneProgress);
    return obj;
}

template<>
Utils::Result<DocumentHighlightParams> fromJson<DocumentHighlightParams>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for DocumentHighlightParams");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("textDocument"))
        return Utils::ResultError("Missing required field: textDocument");
    if (!obj.contains("position"))
        return Utils::ResultError("Missing required field: position");
    DocumentHighlightParams result;
    if (obj.contains("textDocument") && obj["textDocument"].isObject()) {
        const auto res0 = fromJson<TextDocumentIdentifier>("textDocument", obj["textDocument"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._textDocument = *res0;
    }
    if (obj.contains("position") && obj["position"].isObject()) {
        const auto res1 = fromJson<Position>("position", obj["position"]);
        if (!res1)
            return Utils::ResultError(res1.error());
        result._position = *res1;
    }
    if (obj.contains("workDoneToken")) {
        const auto res2 = fromJson<ProgressToken>("workDoneToken", obj["workDoneToken"]);
        if (!res2)
            return Utils::ResultError(res2.error());
        result._workDoneToken = *res2;
    }
    if (obj.contains("partialResultToken")) {
        const auto res3 = fromJson<ProgressToken>("partialResultToken", obj["partialResultToken"]);
        if (!res3)
            return Utils::ResultError(res3.error());
        result._partialResultToken = *res3;
    }
    return result;
}

QJsonObject toJson(const DocumentHighlightParams &data)
{
    QJsonObject obj{
        {"textDocument", toJson(data._textDocument)},
        {"position", toJson(data._position)}
    };
    if (data._workDoneToken.has_value())
        obj.insert("workDoneToken", toJsonValue(*data._workDoneToken));
    if (data._partialResultToken.has_value())
        obj.insert("partialResultToken", toJsonValue(*data._partialResultToken));
    return obj;
}

template<>
Utils::Result<DocumentHighlight> fromJson<DocumentHighlight>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for DocumentHighlight");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("range"))
        return Utils::ResultError("Missing required field: range");
    DocumentHighlight result;
    if (obj.contains("range") && obj["range"].isObject()) {
        const auto res0 = fromJson<Range>("range", obj["range"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._range = *res0;
    }
    if (obj.contains("kind") && obj["kind"].isDouble())
        result._kind = obj["kind"].toInt();
    return result;
}

QJsonObject toJson(const DocumentHighlight &data)
{
    QJsonObject obj{{"range", toJson(data._range)}};
    if (data._kind.has_value())
        obj.insert("kind", *data._kind);
    return obj;
}

template<>
Utils::Result<DocumentHighlightRegistrationOptions> fromJson<DocumentHighlightRegistrationOptions>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for DocumentHighlightRegistrationOptions");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("documentSelector"))
        return Utils::ResultError("Missing required field: documentSelector");
    DocumentHighlightRegistrationOptions result;
    if (obj.contains("documentSelector") && !obj["documentSelector"].isNull()) {
        const auto res0 = fromJson<DocumentSelector>("documentSelector", obj["documentSelector"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._documentSelector = *res0;
    }
    if (obj.contains("workDoneProgress"))
        result._workDoneProgress = obj.value("workDoneProgress").toBool();
    return result;
}

QJsonObject toJson(const DocumentHighlightRegistrationOptions &data)
{
    QJsonObject obj;
    if (data._documentSelector.has_value())
        obj.insert("documentSelector", toJson(*data._documentSelector));
    else
        obj.insert("documentSelector", QJsonValue::Null);
    if (data._workDoneProgress.has_value())
        obj.insert("workDoneProgress", *data._workDoneProgress);
    return obj;
}

template<>
Utils::Result<DocumentSymbolParams> fromJson<DocumentSymbolParams>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for DocumentSymbolParams");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("textDocument"))
        return Utils::ResultError("Missing required field: textDocument");
    DocumentSymbolParams result;
    if (obj.contains("workDoneToken")) {
        const auto res0 = fromJson<ProgressToken>("workDoneToken", obj["workDoneToken"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._workDoneToken = *res0;
    }
    if (obj.contains("partialResultToken")) {
        const auto res1 = fromJson<ProgressToken>("partialResultToken", obj["partialResultToken"]);
        if (!res1)
            return Utils::ResultError(res1.error());
        result._partialResultToken = *res1;
    }
    if (obj.contains("textDocument") && obj["textDocument"].isObject()) {
        const auto res2 = fromJson<TextDocumentIdentifier>("textDocument", obj["textDocument"]);
        if (!res2)
            return Utils::ResultError(res2.error());
        result._textDocument = *res2;
    }
    return result;
}

QJsonObject toJson(const DocumentSymbolParams &data)
{
    QJsonObject obj{{"textDocument", toJson(data._textDocument)}};
    if (data._workDoneToken.has_value())
        obj.insert("workDoneToken", toJsonValue(*data._workDoneToken));
    if (data._partialResultToken.has_value())
        obj.insert("partialResultToken", toJsonValue(*data._partialResultToken));
    return obj;
}

template<>
Utils::Result<SymbolInformation> fromJson<SymbolInformation>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for SymbolInformation");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("name"))
        return Utils::ResultError("Missing required field: name");
    if (!obj.contains("kind"))
        return Utils::ResultError("Missing required field: kind");
    if (!obj.contains("location"))
        return Utils::ResultError("Missing required field: location");
    SymbolInformation result;
    result._name = obj.value("name").toString();
    result._kind = obj.value("kind").toInt();
    if (obj.contains("tags") && obj["tags"].isArray()) {
        const QJsonArray arr = obj["tags"].toArray();
        QList<int> list_tags;
        for (const QJsonValue &v : arr) {
            list_tags.append(v.toInt());
        }
        result._tags = list_tags;
    }
    if (obj.contains("containerName"))
        result._containerName = obj.value("containerName").toString();
    if (obj.contains("deprecated"))
        result._deprecated = obj.value("deprecated").toBool();
    if (obj.contains("location") && obj["location"].isObject()) {
        const auto res0 = fromJson<Location>("location", obj["location"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._location = *res0;
    }
    return result;
}

QJsonObject toJson(const SymbolInformation &data)
{
    QJsonObject obj{
        {"name", data._name},
        {"kind", data._kind},
        {"location", toJson(data._location)}
    };
    if (data._tags.has_value()) {
        QJsonArray arr_tags;
        for (const auto &v : *data._tags) arr_tags.append(v);
        obj.insert("tags", arr_tags);
    }
    if (data._containerName.has_value())
        obj.insert("containerName", *data._containerName);
    if (data._deprecated.has_value())
        obj.insert("deprecated", *data._deprecated);
    return obj;
}

bool DocumentSymbol::operator==(const DocumentSymbol &other) const = default;

template<>
Utils::Result<DocumentSymbol> fromJson<DocumentSymbol>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for DocumentSymbol");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("name"))
        return Utils::ResultError("Missing required field: name");
    if (!obj.contains("kind"))
        return Utils::ResultError("Missing required field: kind");
    if (!obj.contains("range"))
        return Utils::ResultError("Missing required field: range");
    if (!obj.contains("selectionRange"))
        return Utils::ResultError("Missing required field: selectionRange");
    DocumentSymbol result;
    result._name = obj.value("name").toString();
    if (obj.contains("detail"))
        result._detail = obj.value("detail").toString();
    result._kind = obj.value("kind").toInt();
    if (obj.contains("tags") && obj["tags"].isArray()) {
        const QJsonArray arr = obj["tags"].toArray();
        QList<int> list_tags;
        for (const QJsonValue &v : arr) {
            list_tags.append(v.toInt());
        }
        result._tags = list_tags;
    }
    if (obj.contains("deprecated"))
        result._deprecated = obj.value("deprecated").toBool();
    if (obj.contains("range") && obj["range"].isObject()) {
        const auto res0 = fromJson<Range>("range", obj["range"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._range = *res0;
    }
    if (obj.contains("selectionRange") && obj["selectionRange"].isObject()) {
        const auto res1 = fromJson<Range>("selectionRange", obj["selectionRange"]);
        if (!res1)
            return Utils::ResultError(res1.error());
        result._selectionRange = *res1;
    }
    if (obj.contains("children") && obj["children"].isArray()) {
        const QJsonArray arr = obj["children"].toArray();
        QList<DocumentSymbol> list_children;
        for (const QJsonValue &v : arr) {
            const auto res2 = fromJson<DocumentSymbol>("children", v);
            if (!res2)
                return Utils::ResultError(res2.error());
            list_children.append(*res2);
        }
        result._children = list_children;
    }
    return result;
}

QJsonObject toJson(const DocumentSymbol &data)
{
    QJsonObject obj{
        {"name", data._name},
        {"kind", data._kind},
        {"range", toJson(data._range)},
        {"selectionRange", toJson(data._selectionRange)}
    };
    if (data._detail.has_value())
        obj.insert("detail", *data._detail);
    if (data._tags.has_value()) {
        QJsonArray arr_tags;
        for (const auto &v : *data._tags) arr_tags.append(v);
        obj.insert("tags", arr_tags);
    }
    if (data._deprecated.has_value())
        obj.insert("deprecated", *data._deprecated);
    if (data._children.has_value()) {
        QJsonArray arr_children;
        for (const auto &v : *data._children) arr_children.append(toJson(v));
        obj.insert("children", arr_children);
    }
    return obj;
}

template<>
Utils::Result<DocumentSymbolRegistrationOptions> fromJson<DocumentSymbolRegistrationOptions>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for DocumentSymbolRegistrationOptions");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("documentSelector"))
        return Utils::ResultError("Missing required field: documentSelector");
    DocumentSymbolRegistrationOptions result;
    if (obj.contains("documentSelector") && !obj["documentSelector"].isNull()) {
        const auto res0 = fromJson<DocumentSelector>("documentSelector", obj["documentSelector"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._documentSelector = *res0;
    }
    if (obj.contains("workDoneProgress"))
        result._workDoneProgress = obj.value("workDoneProgress").toBool();
    if (obj.contains("label"))
        result._label = obj.value("label").toString();
    return result;
}

QJsonObject toJson(const DocumentSymbolRegistrationOptions &data)
{
    QJsonObject obj;
    if (data._documentSelector.has_value())
        obj.insert("documentSelector", toJson(*data._documentSelector));
    else
        obj.insert("documentSelector", QJsonValue::Null);
    if (data._workDoneProgress.has_value())
        obj.insert("workDoneProgress", *data._workDoneProgress);
    if (data._label.has_value())
        obj.insert("label", *data._label);
    return obj;
}

template<>
Utils::Result<CodeActionContext> fromJson<CodeActionContext>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for CodeActionContext");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("diagnostics"))
        return Utils::ResultError("Missing required field: diagnostics");
    CodeActionContext result;
    if (obj.contains("diagnostics") && obj["diagnostics"].isArray()) {
        const QJsonArray arr = obj["diagnostics"].toArray();
        for (const QJsonValue &v : arr) {
            const auto res0 = fromJson<Diagnostic>("diagnostics", v);
            if (!res0)
                return Utils::ResultError(res0.error());
            result._diagnostics.append(*res0);
        }
    }
    if (obj.contains("only") && obj["only"].isArray()) {
        const QJsonArray arr = obj["only"].toArray();
        QList<CodeActionKind> list_only;
        for (const QJsonValue &v : arr) {
            const auto res1 = fromJson<CodeActionKind>("only", v);
            if (!res1)
                return Utils::ResultError(res1.error());
            list_only.append(*res1);
        }
        result._only = list_only;
    }
    if (obj.contains("triggerKind") && obj["triggerKind"].isDouble())
        result._triggerKind = obj["triggerKind"].toInt();
    return result;
}

QJsonObject toJson(const CodeActionContext &data)
{
    QJsonObject obj;
    QJsonArray arr_diagnostics;
    for (const auto &v : data._diagnostics) arr_diagnostics.append(toJson(v));
    obj.insert("diagnostics", arr_diagnostics);
    if (data._only.has_value()) {
        QJsonArray arr_only;
        for (const auto &v : *data._only) arr_only.append(v);
        obj.insert("only", arr_only);
    }
    if (data._triggerKind.has_value())
        obj.insert("triggerKind", *data._triggerKind);
    return obj;
}

template<>
Utils::Result<CodeActionParams> fromJson<CodeActionParams>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for CodeActionParams");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("textDocument"))
        return Utils::ResultError("Missing required field: textDocument");
    if (!obj.contains("range"))
        return Utils::ResultError("Missing required field: range");
    if (!obj.contains("context"))
        return Utils::ResultError("Missing required field: context");
    CodeActionParams result;
    if (obj.contains("workDoneToken")) {
        const auto res0 = fromJson<ProgressToken>("workDoneToken", obj["workDoneToken"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._workDoneToken = *res0;
    }
    if (obj.contains("partialResultToken")) {
        const auto res1 = fromJson<ProgressToken>("partialResultToken", obj["partialResultToken"]);
        if (!res1)
            return Utils::ResultError(res1.error());
        result._partialResultToken = *res1;
    }
    if (obj.contains("textDocument") && obj["textDocument"].isObject()) {
        const auto res2 = fromJson<TextDocumentIdentifier>("textDocument", obj["textDocument"]);
        if (!res2)
            return Utils::ResultError(res2.error());
        result._textDocument = *res2;
    }
    if (obj.contains("range") && obj["range"].isObject()) {
        const auto res3 = fromJson<Range>("range", obj["range"]);
        if (!res3)
            return Utils::ResultError(res3.error());
        result._range = *res3;
    }
    if (obj.contains("context") && obj["context"].isObject()) {
        const auto res4 = fromJson<CodeActionContext>("context", obj["context"]);
        if (!res4)
            return Utils::ResultError(res4.error());
        result._context = *res4;
    }
    return result;
}

QJsonObject toJson(const CodeActionParams &data)
{
    QJsonObject obj{
        {"textDocument", toJson(data._textDocument)},
        {"range", toJson(data._range)},
        {"context", toJson(data._context)}
    };
    if (data._workDoneToken.has_value())
        obj.insert("workDoneToken", toJsonValue(*data._workDoneToken));
    if (data._partialResultToken.has_value())
        obj.insert("partialResultToken", toJsonValue(*data._partialResultToken));
    return obj;
}

template<>
Utils::Result<CodeActionDisabled> fromJson<CodeActionDisabled>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for CodeActionDisabled");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("reason"))
        return Utils::ResultError("Missing required field: reason");
    CodeActionDisabled result;
    result._reason = obj.value("reason").toString();
    return result;
}

QJsonObject toJson(const CodeActionDisabled &data)
{
    QJsonObject obj{{"reason", data._reason}};
    return obj;
}

template<>
Utils::Result<CodeAction> fromJson<CodeAction>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for CodeAction");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("title"))
        return Utils::ResultError("Missing required field: title");
    CodeAction result;
    result._title = obj.value("title").toString();
    if (obj.contains("kind") && obj["kind"].isString()) {
        const auto res0 = fromJson<CodeActionKind>("kind", obj["kind"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._kind = *res0;
    }
    if (obj.contains("diagnostics") && obj["diagnostics"].isArray()) {
        const QJsonArray arr = obj["diagnostics"].toArray();
        QList<Diagnostic> list_diagnostics;
        for (const QJsonValue &v : arr) {
            const auto res1 = fromJson<Diagnostic>("diagnostics", v);
            if (!res1)
                return Utils::ResultError(res1.error());
            list_diagnostics.append(*res1);
        }
        result._diagnostics = list_diagnostics;
    }
    if (obj.contains("isPreferred"))
        result._isPreferred = obj.value("isPreferred").toBool();
    if (obj.contains("disabled") && obj["disabled"].isObject()) {
        const auto res2 = fromJson<CodeActionDisabled>("disabled", obj["disabled"]);
        if (!res2)
            return Utils::ResultError(res2.error());
        result._disabled = *res2;
    }
    if (obj.contains("edit") && obj["edit"].isObject()) {
        const auto res3 = fromJson<WorkspaceEdit>("edit", obj["edit"]);
        if (!res3)
            return Utils::ResultError(res3.error());
        result._edit = *res3;
    }
    if (obj.contains("command") && obj["command"].isObject()) {
        const auto res4 = fromJson<Command>("command", obj["command"]);
        if (!res4)
            return Utils::ResultError(res4.error());
        result._command = *res4;
    }
    if (obj.contains("data"))
        result._data = obj.value("data");
    if (obj.contains("tags") && obj["tags"].isArray()) {
        const QJsonArray arr = obj["tags"].toArray();
        QList<int> list_tags;
        for (const QJsonValue &v : arr) {
            list_tags.append(v.toInt());
        }
        result._tags = list_tags;
    }
    return result;
}

QJsonObject toJson(const CodeAction &data)
{
    QJsonObject obj{{"title", data._title}};
    if (data._kind.has_value())
        obj.insert("kind", *data._kind);
    if (data._diagnostics.has_value()) {
        QJsonArray arr_diagnostics;
        for (const auto &v : *data._diagnostics) arr_diagnostics.append(toJson(v));
        obj.insert("diagnostics", arr_diagnostics);
    }
    if (data._isPreferred.has_value())
        obj.insert("isPreferred", *data._isPreferred);
    if (data._disabled.has_value())
        obj.insert("disabled", toJson(*data._disabled));
    if (data._edit.has_value())
        obj.insert("edit", toJson(*data._edit));
    if (data._command.has_value())
        obj.insert("command", toJson(*data._command));
    if (data._data.has_value())
        obj.insert("data", *data._data);
    if (data._tags.has_value()) {
        QJsonArray arr_tags;
        for (const auto &v : *data._tags) arr_tags.append(v);
        obj.insert("tags", arr_tags);
    }
    return obj;
}

template<>
Utils::Result<CodeActionRegistrationOptions> fromJson<CodeActionRegistrationOptions>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for CodeActionRegistrationOptions");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("documentSelector"))
        return Utils::ResultError("Missing required field: documentSelector");
    CodeActionRegistrationOptions result;
    if (obj.contains("documentSelector") && !obj["documentSelector"].isNull()) {
        const auto res0 = fromJson<DocumentSelector>("documentSelector", obj["documentSelector"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._documentSelector = *res0;
    }
    if (obj.contains("workDoneProgress"))
        result._workDoneProgress = obj.value("workDoneProgress").toBool();
    if (obj.contains("codeActionKinds") && obj["codeActionKinds"].isArray()) {
        const QJsonArray arr = obj["codeActionKinds"].toArray();
        QList<CodeActionKind> list_codeActionKinds;
        for (const QJsonValue &v : arr) {
            const auto res1 = fromJson<CodeActionKind>("codeActionKinds", v);
            if (!res1)
                return Utils::ResultError(res1.error());
            list_codeActionKinds.append(*res1);
        }
        result._codeActionKinds = list_codeActionKinds;
    }
    if (obj.contains("documentation") && obj["documentation"].isArray()) {
        const QJsonArray arr = obj["documentation"].toArray();
        QList<CodeActionKindDocumentation> list_documentation;
        for (const QJsonValue &v : arr) {
            const auto res2 = fromJson<CodeActionKindDocumentation>("documentation", v);
            if (!res2)
                return Utils::ResultError(res2.error());
            list_documentation.append(*res2);
        }
        result._documentation = list_documentation;
    }
    if (obj.contains("resolveProvider"))
        result._resolveProvider = obj.value("resolveProvider").toBool();
    return result;
}

QJsonObject toJson(const CodeActionRegistrationOptions &data)
{
    QJsonObject obj;
    if (data._documentSelector.has_value())
        obj.insert("documentSelector", toJson(*data._documentSelector));
    else
        obj.insert("documentSelector", QJsonValue::Null);
    if (data._workDoneProgress.has_value())
        obj.insert("workDoneProgress", *data._workDoneProgress);
    if (data._codeActionKinds.has_value()) {
        QJsonArray arr_codeActionKinds;
        for (const auto &v : *data._codeActionKinds) arr_codeActionKinds.append(v);
        obj.insert("codeActionKinds", arr_codeActionKinds);
    }
    if (data._documentation.has_value()) {
        QJsonArray arr_documentation;
        for (const auto &v : *data._documentation) arr_documentation.append(toJson(v));
        obj.insert("documentation", arr_documentation);
    }
    if (data._resolveProvider.has_value())
        obj.insert("resolveProvider", *data._resolveProvider);
    return obj;
}

template<>
Utils::Result<WorkspaceSymbolParams> fromJson<WorkspaceSymbolParams>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for WorkspaceSymbolParams");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("query"))
        return Utils::ResultError("Missing required field: query");
    WorkspaceSymbolParams result;
    if (obj.contains("workDoneToken")) {
        const auto res0 = fromJson<ProgressToken>("workDoneToken", obj["workDoneToken"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._workDoneToken = *res0;
    }
    if (obj.contains("partialResultToken")) {
        const auto res1 = fromJson<ProgressToken>("partialResultToken", obj["partialResultToken"]);
        if (!res1)
            return Utils::ResultError(res1.error());
        result._partialResultToken = *res1;
    }
    result._query = obj.value("query").toString();
    return result;
}

QJsonObject toJson(const WorkspaceSymbolParams &data)
{
    QJsonObject obj{{"query", data._query}};
    if (data._workDoneToken.has_value())
        obj.insert("workDoneToken", toJsonValue(*data._workDoneToken));
    if (data._partialResultToken.has_value())
        obj.insert("partialResultToken", toJsonValue(*data._partialResultToken));
    return obj;
}

template<>
Utils::Result<LocationUriOnly> fromJson<LocationUriOnly>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for LocationUriOnly");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("uri"))
        return Utils::ResultError("Missing required field: uri");
    LocationUriOnly result;
    result._uri = obj.value("uri").toString();
    return result;
}

QJsonObject toJson(const LocationUriOnly &data)
{
    QJsonObject obj{{"uri", data._uri}};
    return obj;
}

template<>
Utils::Result<WorkspaceSymbolLocation> fromJson<WorkspaceSymbolLocation>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Invalid WorkspaceSymbolLocation: expected object or array");
    const QJsonObject obj = val.toObject();
    if (obj.contains("range")) {
        const auto res0 = fromJson<Location>(val);
        if (!res0)
            return Utils::ResultError(res0.error());
        return WorkspaceSymbolLocation(*res0);
    }
    {
        auto result = fromJson<LocationUriOnly>(val);
        if (result) return WorkspaceSymbolLocation(*result);
    }
    return Utils::ResultError("Invalid WorkspaceSymbolLocation");
}

QJsonValue toJsonValue(const WorkspaceSymbolLocation &val)
{
    return std::visit([](const auto &v) -> QJsonValue {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, QJsonObject>) {
            return v;
        } else {
            return toJson(v);
        }
    }, val);
}

template<>
Utils::Result<WorkspaceSymbol> fromJson<WorkspaceSymbol>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for WorkspaceSymbol");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("name"))
        return Utils::ResultError("Missing required field: name");
    if (!obj.contains("kind"))
        return Utils::ResultError("Missing required field: kind");
    if (!obj.contains("location"))
        return Utils::ResultError("Missing required field: location");
    WorkspaceSymbol result;
    result._name = obj.value("name").toString();
    result._kind = obj.value("kind").toInt();
    if (obj.contains("tags") && obj["tags"].isArray()) {
        const QJsonArray arr = obj["tags"].toArray();
        QList<int> list_tags;
        for (const QJsonValue &v : arr) {
            list_tags.append(v.toInt());
        }
        result._tags = list_tags;
    }
    if (obj.contains("containerName"))
        result._containerName = obj.value("containerName").toString();
    if (obj.contains("location")) {
        const auto res0 = fromJson<WorkspaceSymbolLocation>("location", obj["location"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._location = *res0;
    }
    if (obj.contains("data"))
        result._data = obj.value("data");
    return result;
}

QJsonObject toJson(const WorkspaceSymbol &data)
{
    QJsonObject obj{
        {"name", data._name},
        {"kind", data._kind},
        {"location", toJsonValue(data._location)}
    };
    if (data._tags.has_value()) {
        QJsonArray arr_tags;
        for (const auto &v : *data._tags) arr_tags.append(v);
        obj.insert("tags", arr_tags);
    }
    if (data._containerName.has_value())
        obj.insert("containerName", *data._containerName);
    if (data._data.has_value())
        obj.insert("data", *data._data);
    return obj;
}

template<>
Utils::Result<WorkspaceSymbolRegistrationOptions> fromJson<WorkspaceSymbolRegistrationOptions>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for WorkspaceSymbolRegistrationOptions");
    const QJsonObject obj = val.toObject();
    WorkspaceSymbolRegistrationOptions result;
    if (obj.contains("workDoneProgress"))
        result._workDoneProgress = obj.value("workDoneProgress").toBool();
    if (obj.contains("resolveProvider"))
        result._resolveProvider = obj.value("resolveProvider").toBool();
    return result;
}

QJsonObject toJson(const WorkspaceSymbolRegistrationOptions &data)
{
    QJsonObject obj;
    if (data._workDoneProgress.has_value())
        obj.insert("workDoneProgress", *data._workDoneProgress);
    if (data._resolveProvider.has_value())
        obj.insert("resolveProvider", *data._resolveProvider);
    return obj;
}

template<>
Utils::Result<CodeLensParams> fromJson<CodeLensParams>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for CodeLensParams");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("textDocument"))
        return Utils::ResultError("Missing required field: textDocument");
    CodeLensParams result;
    if (obj.contains("workDoneToken")) {
        const auto res0 = fromJson<ProgressToken>("workDoneToken", obj["workDoneToken"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._workDoneToken = *res0;
    }
    if (obj.contains("partialResultToken")) {
        const auto res1 = fromJson<ProgressToken>("partialResultToken", obj["partialResultToken"]);
        if (!res1)
            return Utils::ResultError(res1.error());
        result._partialResultToken = *res1;
    }
    if (obj.contains("textDocument") && obj["textDocument"].isObject()) {
        const auto res2 = fromJson<TextDocumentIdentifier>("textDocument", obj["textDocument"]);
        if (!res2)
            return Utils::ResultError(res2.error());
        result._textDocument = *res2;
    }
    return result;
}

QJsonObject toJson(const CodeLensParams &data)
{
    QJsonObject obj{{"textDocument", toJson(data._textDocument)}};
    if (data._workDoneToken.has_value())
        obj.insert("workDoneToken", toJsonValue(*data._workDoneToken));
    if (data._partialResultToken.has_value())
        obj.insert("partialResultToken", toJsonValue(*data._partialResultToken));
    return obj;
}

template<>
Utils::Result<CodeLens> fromJson<CodeLens>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for CodeLens");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("range"))
        return Utils::ResultError("Missing required field: range");
    CodeLens result;
    if (obj.contains("range") && obj["range"].isObject()) {
        const auto res0 = fromJson<Range>("range", obj["range"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._range = *res0;
    }
    if (obj.contains("command") && obj["command"].isObject()) {
        const auto res1 = fromJson<Command>("command", obj["command"]);
        if (!res1)
            return Utils::ResultError(res1.error());
        result._command = *res1;
    }
    if (obj.contains("data"))
        result._data = obj.value("data");
    return result;
}

QJsonObject toJson(const CodeLens &data)
{
    QJsonObject obj{{"range", toJson(data._range)}};
    if (data._command.has_value())
        obj.insert("command", toJson(*data._command));
    if (data._data.has_value())
        obj.insert("data", *data._data);
    return obj;
}

template<>
Utils::Result<CodeLensRegistrationOptions> fromJson<CodeLensRegistrationOptions>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for CodeLensRegistrationOptions");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("documentSelector"))
        return Utils::ResultError("Missing required field: documentSelector");
    CodeLensRegistrationOptions result;
    if (obj.contains("documentSelector") && !obj["documentSelector"].isNull()) {
        const auto res0 = fromJson<DocumentSelector>("documentSelector", obj["documentSelector"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._documentSelector = *res0;
    }
    if (obj.contains("workDoneProgress"))
        result._workDoneProgress = obj.value("workDoneProgress").toBool();
    if (obj.contains("resolveProvider"))
        result._resolveProvider = obj.value("resolveProvider").toBool();
    return result;
}

QJsonObject toJson(const CodeLensRegistrationOptions &data)
{
    QJsonObject obj;
    if (data._documentSelector.has_value())
        obj.insert("documentSelector", toJson(*data._documentSelector));
    else
        obj.insert("documentSelector", QJsonValue::Null);
    if (data._workDoneProgress.has_value())
        obj.insert("workDoneProgress", *data._workDoneProgress);
    if (data._resolveProvider.has_value())
        obj.insert("resolveProvider", *data._resolveProvider);
    return obj;
}

template<>
Utils::Result<DocumentLinkParams> fromJson<DocumentLinkParams>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for DocumentLinkParams");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("textDocument"))
        return Utils::ResultError("Missing required field: textDocument");
    DocumentLinkParams result;
    if (obj.contains("workDoneToken")) {
        const auto res0 = fromJson<ProgressToken>("workDoneToken", obj["workDoneToken"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._workDoneToken = *res0;
    }
    if (obj.contains("partialResultToken")) {
        const auto res1 = fromJson<ProgressToken>("partialResultToken", obj["partialResultToken"]);
        if (!res1)
            return Utils::ResultError(res1.error());
        result._partialResultToken = *res1;
    }
    if (obj.contains("textDocument") && obj["textDocument"].isObject()) {
        const auto res2 = fromJson<TextDocumentIdentifier>("textDocument", obj["textDocument"]);
        if (!res2)
            return Utils::ResultError(res2.error());
        result._textDocument = *res2;
    }
    return result;
}

QJsonObject toJson(const DocumentLinkParams &data)
{
    QJsonObject obj{{"textDocument", toJson(data._textDocument)}};
    if (data._workDoneToken.has_value())
        obj.insert("workDoneToken", toJsonValue(*data._workDoneToken));
    if (data._partialResultToken.has_value())
        obj.insert("partialResultToken", toJsonValue(*data._partialResultToken));
    return obj;
}

template<>
Utils::Result<DocumentLink> fromJson<DocumentLink>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for DocumentLink");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("range"))
        return Utils::ResultError("Missing required field: range");
    DocumentLink result;
    if (obj.contains("range") && obj["range"].isObject()) {
        const auto res0 = fromJson<Range>("range", obj["range"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._range = *res0;
    }
    if (obj.contains("target"))
        result._target = obj.value("target").toString();
    if (obj.contains("tooltip"))
        result._tooltip = obj.value("tooltip").toString();
    if (obj.contains("data"))
        result._data = obj.value("data");
    return result;
}

QJsonObject toJson(const DocumentLink &data)
{
    QJsonObject obj{{"range", toJson(data._range)}};
    if (data._target.has_value())
        obj.insert("target", *data._target);
    if (data._tooltip.has_value())
        obj.insert("tooltip", *data._tooltip);
    if (data._data.has_value())
        obj.insert("data", *data._data);
    return obj;
}

template<>
Utils::Result<DocumentLinkRegistrationOptions> fromJson<DocumentLinkRegistrationOptions>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for DocumentLinkRegistrationOptions");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("documentSelector"))
        return Utils::ResultError("Missing required field: documentSelector");
    DocumentLinkRegistrationOptions result;
    if (obj.contains("documentSelector") && !obj["documentSelector"].isNull()) {
        const auto res0 = fromJson<DocumentSelector>("documentSelector", obj["documentSelector"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._documentSelector = *res0;
    }
    if (obj.contains("workDoneProgress"))
        result._workDoneProgress = obj.value("workDoneProgress").toBool();
    if (obj.contains("resolveProvider"))
        result._resolveProvider = obj.value("resolveProvider").toBool();
    return result;
}

QJsonObject toJson(const DocumentLinkRegistrationOptions &data)
{
    QJsonObject obj;
    if (data._documentSelector.has_value())
        obj.insert("documentSelector", toJson(*data._documentSelector));
    else
        obj.insert("documentSelector", QJsonValue::Null);
    if (data._workDoneProgress.has_value())
        obj.insert("workDoneProgress", *data._workDoneProgress);
    if (data._resolveProvider.has_value())
        obj.insert("resolveProvider", *data._resolveProvider);
    return obj;
}

template<>
Utils::Result<FormattingOptions> fromJson<FormattingOptions>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for FormattingOptions");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("tabSize"))
        return Utils::ResultError("Missing required field: tabSize");
    if (!obj.contains("insertSpaces"))
        return Utils::ResultError("Missing required field: insertSpaces");
    FormattingOptions result;
    result._tabSize = obj.value("tabSize").toInt();
    result._insertSpaces = obj.value("insertSpaces").toBool();
    if (obj.contains("trimTrailingWhitespace"))
        result._trimTrailingWhitespace = obj.value("trimTrailingWhitespace").toBool();
    if (obj.contains("insertFinalNewline"))
        result._insertFinalNewline = obj.value("insertFinalNewline").toBool();
    if (obj.contains("trimFinalNewlines"))
        result._trimFinalNewlines = obj.value("trimFinalNewlines").toBool();
    return result;
}

QJsonObject toJson(const FormattingOptions &data)
{
    QJsonObject obj{
        {"tabSize", data._tabSize},
        {"insertSpaces", data._insertSpaces}
    };
    if (data._trimTrailingWhitespace.has_value())
        obj.insert("trimTrailingWhitespace", *data._trimTrailingWhitespace);
    if (data._insertFinalNewline.has_value())
        obj.insert("insertFinalNewline", *data._insertFinalNewline);
    if (data._trimFinalNewlines.has_value())
        obj.insert("trimFinalNewlines", *data._trimFinalNewlines);
    return obj;
}

template<>
Utils::Result<DocumentFormattingParams> fromJson<DocumentFormattingParams>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for DocumentFormattingParams");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("textDocument"))
        return Utils::ResultError("Missing required field: textDocument");
    if (!obj.contains("options"))
        return Utils::ResultError("Missing required field: options");
    DocumentFormattingParams result;
    if (obj.contains("workDoneToken")) {
        const auto res0 = fromJson<ProgressToken>("workDoneToken", obj["workDoneToken"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._workDoneToken = *res0;
    }
    if (obj.contains("textDocument") && obj["textDocument"].isObject()) {
        const auto res1 = fromJson<TextDocumentIdentifier>("textDocument", obj["textDocument"]);
        if (!res1)
            return Utils::ResultError(res1.error());
        result._textDocument = *res1;
    }
    if (obj.contains("options") && obj["options"].isObject()) {
        const auto res2 = fromJson<FormattingOptions>("options", obj["options"]);
        if (!res2)
            return Utils::ResultError(res2.error());
        result._options = *res2;
    }
    return result;
}

QJsonObject toJson(const DocumentFormattingParams &data)
{
    QJsonObject obj{
        {"textDocument", toJson(data._textDocument)},
        {"options", toJson(data._options)}
    };
    if (data._workDoneToken.has_value())
        obj.insert("workDoneToken", toJsonValue(*data._workDoneToken));
    return obj;
}

template<>
Utils::Result<DocumentFormattingRegistrationOptions> fromJson<DocumentFormattingRegistrationOptions>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for DocumentFormattingRegistrationOptions");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("documentSelector"))
        return Utils::ResultError("Missing required field: documentSelector");
    DocumentFormattingRegistrationOptions result;
    if (obj.contains("documentSelector") && !obj["documentSelector"].isNull()) {
        const auto res0 = fromJson<DocumentSelector>("documentSelector", obj["documentSelector"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._documentSelector = *res0;
    }
    if (obj.contains("workDoneProgress"))
        result._workDoneProgress = obj.value("workDoneProgress").toBool();
    return result;
}

QJsonObject toJson(const DocumentFormattingRegistrationOptions &data)
{
    QJsonObject obj;
    if (data._documentSelector.has_value())
        obj.insert("documentSelector", toJson(*data._documentSelector));
    else
        obj.insert("documentSelector", QJsonValue::Null);
    if (data._workDoneProgress.has_value())
        obj.insert("workDoneProgress", *data._workDoneProgress);
    return obj;
}

template<>
Utils::Result<DocumentRangeFormattingParams> fromJson<DocumentRangeFormattingParams>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for DocumentRangeFormattingParams");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("textDocument"))
        return Utils::ResultError("Missing required field: textDocument");
    if (!obj.contains("range"))
        return Utils::ResultError("Missing required field: range");
    if (!obj.contains("options"))
        return Utils::ResultError("Missing required field: options");
    DocumentRangeFormattingParams result;
    if (obj.contains("workDoneToken")) {
        const auto res0 = fromJson<ProgressToken>("workDoneToken", obj["workDoneToken"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._workDoneToken = *res0;
    }
    if (obj.contains("textDocument") && obj["textDocument"].isObject()) {
        const auto res1 = fromJson<TextDocumentIdentifier>("textDocument", obj["textDocument"]);
        if (!res1)
            return Utils::ResultError(res1.error());
        result._textDocument = *res1;
    }
    if (obj.contains("range") && obj["range"].isObject()) {
        const auto res2 = fromJson<Range>("range", obj["range"]);
        if (!res2)
            return Utils::ResultError(res2.error());
        result._range = *res2;
    }
    if (obj.contains("options") && obj["options"].isObject()) {
        const auto res3 = fromJson<FormattingOptions>("options", obj["options"]);
        if (!res3)
            return Utils::ResultError(res3.error());
        result._options = *res3;
    }
    return result;
}

QJsonObject toJson(const DocumentRangeFormattingParams &data)
{
    QJsonObject obj{
        {"textDocument", toJson(data._textDocument)},
        {"range", toJson(data._range)},
        {"options", toJson(data._options)}
    };
    if (data._workDoneToken.has_value())
        obj.insert("workDoneToken", toJsonValue(*data._workDoneToken));
    return obj;
}

template<>
Utils::Result<DocumentRangeFormattingRegistrationOptions> fromJson<DocumentRangeFormattingRegistrationOptions>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for DocumentRangeFormattingRegistrationOptions");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("documentSelector"))
        return Utils::ResultError("Missing required field: documentSelector");
    DocumentRangeFormattingRegistrationOptions result;
    if (obj.contains("documentSelector") && !obj["documentSelector"].isNull()) {
        const auto res0 = fromJson<DocumentSelector>("documentSelector", obj["documentSelector"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._documentSelector = *res0;
    }
    if (obj.contains("workDoneProgress"))
        result._workDoneProgress = obj.value("workDoneProgress").toBool();
    if (obj.contains("rangesSupport"))
        result._rangesSupport = obj.value("rangesSupport").toBool();
    return result;
}

QJsonObject toJson(const DocumentRangeFormattingRegistrationOptions &data)
{
    QJsonObject obj;
    if (data._documentSelector.has_value())
        obj.insert("documentSelector", toJson(*data._documentSelector));
    else
        obj.insert("documentSelector", QJsonValue::Null);
    if (data._workDoneProgress.has_value())
        obj.insert("workDoneProgress", *data._workDoneProgress);
    if (data._rangesSupport.has_value())
        obj.insert("rangesSupport", *data._rangesSupport);
    return obj;
}

template<>
Utils::Result<DocumentRangesFormattingParams> fromJson<DocumentRangesFormattingParams>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for DocumentRangesFormattingParams");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("textDocument"))
        return Utils::ResultError("Missing required field: textDocument");
    if (!obj.contains("ranges"))
        return Utils::ResultError("Missing required field: ranges");
    if (!obj.contains("options"))
        return Utils::ResultError("Missing required field: options");
    DocumentRangesFormattingParams result;
    if (obj.contains("workDoneToken")) {
        const auto res0 = fromJson<ProgressToken>("workDoneToken", obj["workDoneToken"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._workDoneToken = *res0;
    }
    if (obj.contains("textDocument") && obj["textDocument"].isObject()) {
        const auto res1 = fromJson<TextDocumentIdentifier>("textDocument", obj["textDocument"]);
        if (!res1)
            return Utils::ResultError(res1.error());
        result._textDocument = *res1;
    }
    if (obj.contains("ranges") && obj["ranges"].isArray()) {
        const QJsonArray arr = obj["ranges"].toArray();
        for (const QJsonValue &v : arr) {
            const auto res2 = fromJson<Range>("ranges", v);
            if (!res2)
                return Utils::ResultError(res2.error());
            result._ranges.append(*res2);
        }
    }
    if (obj.contains("options") && obj["options"].isObject()) {
        const auto res3 = fromJson<FormattingOptions>("options", obj["options"]);
        if (!res3)
            return Utils::ResultError(res3.error());
        result._options = *res3;
    }
    return result;
}

QJsonObject toJson(const DocumentRangesFormattingParams &data)
{
    QJsonObject obj{
        {"textDocument", toJson(data._textDocument)},
        {"options", toJson(data._options)}
    };
    if (data._workDoneToken.has_value())
        obj.insert("workDoneToken", toJsonValue(*data._workDoneToken));
    QJsonArray arr_ranges;
    for (const auto &v : data._ranges) arr_ranges.append(toJson(v));
    obj.insert("ranges", arr_ranges);
    return obj;
}

template<>
Utils::Result<DocumentOnTypeFormattingParams> fromJson<DocumentOnTypeFormattingParams>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for DocumentOnTypeFormattingParams");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("textDocument"))
        return Utils::ResultError("Missing required field: textDocument");
    if (!obj.contains("position"))
        return Utils::ResultError("Missing required field: position");
    if (!obj.contains("ch"))
        return Utils::ResultError("Missing required field: ch");
    if (!obj.contains("options"))
        return Utils::ResultError("Missing required field: options");
    DocumentOnTypeFormattingParams result;
    if (obj.contains("textDocument") && obj["textDocument"].isObject()) {
        const auto res0 = fromJson<TextDocumentIdentifier>("textDocument", obj["textDocument"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._textDocument = *res0;
    }
    if (obj.contains("position") && obj["position"].isObject()) {
        const auto res1 = fromJson<Position>("position", obj["position"]);
        if (!res1)
            return Utils::ResultError(res1.error());
        result._position = *res1;
    }
    result._ch = obj.value("ch").toString();
    if (obj.contains("options") && obj["options"].isObject()) {
        const auto res2 = fromJson<FormattingOptions>("options", obj["options"]);
        if (!res2)
            return Utils::ResultError(res2.error());
        result._options = *res2;
    }
    return result;
}

QJsonObject toJson(const DocumentOnTypeFormattingParams &data)
{
    QJsonObject obj{
        {"textDocument", toJson(data._textDocument)},
        {"position", toJson(data._position)},
        {"ch", data._ch},
        {"options", toJson(data._options)}
    };
    return obj;
}

template<>
Utils::Result<DocumentOnTypeFormattingRegistrationOptions> fromJson<DocumentOnTypeFormattingRegistrationOptions>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for DocumentOnTypeFormattingRegistrationOptions");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("documentSelector"))
        return Utils::ResultError("Missing required field: documentSelector");
    if (!obj.contains("firstTriggerCharacter"))
        return Utils::ResultError("Missing required field: firstTriggerCharacter");
    DocumentOnTypeFormattingRegistrationOptions result;
    if (obj.contains("documentSelector") && !obj["documentSelector"].isNull()) {
        const auto res0 = fromJson<DocumentSelector>("documentSelector", obj["documentSelector"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._documentSelector = *res0;
    }
    result._firstTriggerCharacter = obj.value("firstTriggerCharacter").toString();
    if (obj.contains("moreTriggerCharacter") && obj["moreTriggerCharacter"].isArray()) {
        const QJsonArray arr = obj["moreTriggerCharacter"].toArray();
        QStringList list_moreTriggerCharacter;
        for (const QJsonValue &v : arr) {
            list_moreTriggerCharacter.append(v.toString());
        }
        result._moreTriggerCharacter = list_moreTriggerCharacter;
    }
    return result;
}

QJsonObject toJson(const DocumentOnTypeFormattingRegistrationOptions &data)
{
    QJsonObject obj{{"firstTriggerCharacter", data._firstTriggerCharacter}};
    if (data._documentSelector.has_value())
        obj.insert("documentSelector", toJson(*data._documentSelector));
    else
        obj.insert("documentSelector", QJsonValue::Null);
    if (data._moreTriggerCharacter.has_value()) {
        QJsonArray arr_moreTriggerCharacter;
        for (const auto &v : *data._moreTriggerCharacter) arr_moreTriggerCharacter.append(v);
        obj.insert("moreTriggerCharacter", arr_moreTriggerCharacter);
    }
    return obj;
}

template<>
Utils::Result<RenameParams> fromJson<RenameParams>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for RenameParams");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("textDocument"))
        return Utils::ResultError("Missing required field: textDocument");
    if (!obj.contains("position"))
        return Utils::ResultError("Missing required field: position");
    if (!obj.contains("newName"))
        return Utils::ResultError("Missing required field: newName");
    RenameParams result;
    if (obj.contains("textDocument") && obj["textDocument"].isObject()) {
        const auto res0 = fromJson<TextDocumentIdentifier>("textDocument", obj["textDocument"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._textDocument = *res0;
    }
    if (obj.contains("position") && obj["position"].isObject()) {
        const auto res1 = fromJson<Position>("position", obj["position"]);
        if (!res1)
            return Utils::ResultError(res1.error());
        result._position = *res1;
    }
    if (obj.contains("workDoneToken")) {
        const auto res2 = fromJson<ProgressToken>("workDoneToken", obj["workDoneToken"]);
        if (!res2)
            return Utils::ResultError(res2.error());
        result._workDoneToken = *res2;
    }
    result._newName = obj.value("newName").toString();
    return result;
}

QJsonObject toJson(const RenameParams &data)
{
    QJsonObject obj{
        {"textDocument", toJson(data._textDocument)},
        {"position", toJson(data._position)},
        {"newName", data._newName}
    };
    if (data._workDoneToken.has_value())
        obj.insert("workDoneToken", toJsonValue(*data._workDoneToken));
    return obj;
}

template<>
Utils::Result<RenameRegistrationOptions> fromJson<RenameRegistrationOptions>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for RenameRegistrationOptions");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("documentSelector"))
        return Utils::ResultError("Missing required field: documentSelector");
    RenameRegistrationOptions result;
    if (obj.contains("documentSelector") && !obj["documentSelector"].isNull()) {
        const auto res0 = fromJson<DocumentSelector>("documentSelector", obj["documentSelector"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._documentSelector = *res0;
    }
    if (obj.contains("workDoneProgress"))
        result._workDoneProgress = obj.value("workDoneProgress").toBool();
    if (obj.contains("prepareProvider"))
        result._prepareProvider = obj.value("prepareProvider").toBool();
    return result;
}

QJsonObject toJson(const RenameRegistrationOptions &data)
{
    QJsonObject obj;
    if (data._documentSelector.has_value())
        obj.insert("documentSelector", toJson(*data._documentSelector));
    else
        obj.insert("documentSelector", QJsonValue::Null);
    if (data._workDoneProgress.has_value())
        obj.insert("workDoneProgress", *data._workDoneProgress);
    if (data._prepareProvider.has_value())
        obj.insert("prepareProvider", *data._prepareProvider);
    return obj;
}

template<>
Utils::Result<PrepareRenameParams> fromJson<PrepareRenameParams>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for PrepareRenameParams");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("textDocument"))
        return Utils::ResultError("Missing required field: textDocument");
    if (!obj.contains("position"))
        return Utils::ResultError("Missing required field: position");
    PrepareRenameParams result;
    if (obj.contains("textDocument") && obj["textDocument"].isObject()) {
        const auto res0 = fromJson<TextDocumentIdentifier>("textDocument", obj["textDocument"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._textDocument = *res0;
    }
    if (obj.contains("position") && obj["position"].isObject()) {
        const auto res1 = fromJson<Position>("position", obj["position"]);
        if (!res1)
            return Utils::ResultError(res1.error());
        result._position = *res1;
    }
    if (obj.contains("workDoneToken")) {
        const auto res2 = fromJson<ProgressToken>("workDoneToken", obj["workDoneToken"]);
        if (!res2)
            return Utils::ResultError(res2.error());
        result._workDoneToken = *res2;
    }
    return result;
}

QJsonObject toJson(const PrepareRenameParams &data)
{
    QJsonObject obj{
        {"textDocument", toJson(data._textDocument)},
        {"position", toJson(data._position)}
    };
    if (data._workDoneToken.has_value())
        obj.insert("workDoneToken", toJsonValue(*data._workDoneToken));
    return obj;
}

template<>
Utils::Result<ExecuteCommandParams> fromJson<ExecuteCommandParams>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for ExecuteCommandParams");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("command"))
        return Utils::ResultError("Missing required field: command");
    ExecuteCommandParams result;
    if (obj.contains("workDoneToken")) {
        const auto res0 = fromJson<ProgressToken>("workDoneToken", obj["workDoneToken"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._workDoneToken = *res0;
    }
    result._command = obj.value("command").toString();
    if (obj.contains("arguments") && obj["arguments"].isArray()) {
        const QJsonArray arr = obj["arguments"].toArray();
        QList<QJsonValue> list_arguments;
        for (const QJsonValue &v : arr) {
            list_arguments.append(v);
        }
        result._arguments = list_arguments;
    }
    return result;
}

QJsonObject toJson(const ExecuteCommandParams &data)
{
    QJsonObject obj{{"command", data._command}};
    if (data._workDoneToken.has_value())
        obj.insert("workDoneToken", toJsonValue(*data._workDoneToken));
    if (data._arguments.has_value()) {
        QJsonArray arr_arguments;
        for (const auto &v : *data._arguments) arr_arguments.append(v);
        obj.insert("arguments", arr_arguments);
    }
    return obj;
}

template<>
Utils::Result<ExecuteCommandRegistrationOptions> fromJson<ExecuteCommandRegistrationOptions>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for ExecuteCommandRegistrationOptions");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("commands"))
        return Utils::ResultError("Missing required field: commands");
    ExecuteCommandRegistrationOptions result;
    if (obj.contains("workDoneProgress"))
        result._workDoneProgress = obj.value("workDoneProgress").toBool();
    if (obj.contains("commands") && obj["commands"].isArray()) {
        const QJsonArray arr = obj["commands"].toArray();
        for (const QJsonValue &v : arr) {
            result._commands.append(v.toString());
        }
    }
    return result;
}

QJsonObject toJson(const ExecuteCommandRegistrationOptions &data)
{
    QJsonObject obj;
    if (data._workDoneProgress.has_value())
        obj.insert("workDoneProgress", *data._workDoneProgress);
    QJsonArray arr_commands;
    for (const auto &v : data._commands) arr_commands.append(v);
    obj.insert("commands", arr_commands);
    return obj;
}

template<>
Utils::Result<WorkspaceEditMetadata> fromJson<WorkspaceEditMetadata>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for WorkspaceEditMetadata");
    const QJsonObject obj = val.toObject();
    WorkspaceEditMetadata result;
    if (obj.contains("isRefactoring"))
        result._isRefactoring = obj.value("isRefactoring").toBool();
    return result;
}

QJsonObject toJson(const WorkspaceEditMetadata &data)
{
    QJsonObject obj;
    if (data._isRefactoring.has_value())
        obj.insert("isRefactoring", *data._isRefactoring);
    return obj;
}

template<>
Utils::Result<ApplyWorkspaceEditParams> fromJson<ApplyWorkspaceEditParams>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for ApplyWorkspaceEditParams");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("edit"))
        return Utils::ResultError("Missing required field: edit");
    ApplyWorkspaceEditParams result;
    if (obj.contains("label"))
        result._label = obj.value("label").toString();
    if (obj.contains("edit") && obj["edit"].isObject()) {
        const auto res0 = fromJson<WorkspaceEdit>("edit", obj["edit"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._edit = *res0;
    }
    if (obj.contains("metadata") && obj["metadata"].isObject()) {
        const auto res1 = fromJson<WorkspaceEditMetadata>("metadata", obj["metadata"]);
        if (!res1)
            return Utils::ResultError(res1.error());
        result._metadata = *res1;
    }
    return result;
}

QJsonObject toJson(const ApplyWorkspaceEditParams &data)
{
    QJsonObject obj{{"edit", toJson(data._edit)}};
    if (data._label.has_value())
        obj.insert("label", *data._label);
    if (data._metadata.has_value())
        obj.insert("metadata", toJson(*data._metadata));
    return obj;
}

template<>
Utils::Result<ApplyWorkspaceEditResult> fromJson<ApplyWorkspaceEditResult>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for ApplyWorkspaceEditResult");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("applied"))
        return Utils::ResultError("Missing required field: applied");
    ApplyWorkspaceEditResult result;
    result._applied = obj.value("applied").toBool();
    if (obj.contains("failureReason"))
        result._failureReason = obj.value("failureReason").toString();
    if (obj.contains("failedChange"))
        result._failedChange = obj.value("failedChange").toInt();
    return result;
}

QJsonObject toJson(const ApplyWorkspaceEditResult &data)
{
    QJsonObject obj{{"applied", data._applied}};
    if (data._failureReason.has_value())
        obj.insert("failureReason", *data._failureReason);
    if (data._failedChange.has_value())
        obj.insert("failedChange", *data._failedChange);
    return obj;
}

template<>
Utils::Result<WorkDoneProgressBegin> fromJson<WorkDoneProgressBegin>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for WorkDoneProgressBegin");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("kind"))
        return Utils::ResultError("Missing required field: kind");
    if (!obj.contains("title"))
        return Utils::ResultError("Missing required field: title");
    WorkDoneProgressBegin result;
    if (obj.value("kind").toString() != "begin")
        return Utils::ResultError("Field 'kind' must be 'begin', got: " + obj.value("kind").toString());
    result._title = obj.value("title").toString();
    if (obj.contains("cancellable"))
        result._cancellable = obj.value("cancellable").toBool();
    if (obj.contains("message"))
        result._message = obj.value("message").toString();
    if (obj.contains("percentage"))
        result._percentage = obj.value("percentage").toInt();
    return result;
}

QJsonObject toJson(const WorkDoneProgressBegin &data)
{
    QJsonObject obj{
        {"kind", QString("begin")},
        {"title", data._title}
    };
    if (data._cancellable.has_value())
        obj.insert("cancellable", *data._cancellable);
    if (data._message.has_value())
        obj.insert("message", *data._message);
    if (data._percentage.has_value())
        obj.insert("percentage", *data._percentage);
    return obj;
}

template<>
Utils::Result<WorkDoneProgressReport> fromJson<WorkDoneProgressReport>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for WorkDoneProgressReport");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("kind"))
        return Utils::ResultError("Missing required field: kind");
    WorkDoneProgressReport result;
    if (obj.value("kind").toString() != "report")
        return Utils::ResultError("Field 'kind' must be 'report', got: " + obj.value("kind").toString());
    if (obj.contains("cancellable"))
        result._cancellable = obj.value("cancellable").toBool();
    if (obj.contains("message"))
        result._message = obj.value("message").toString();
    if (obj.contains("percentage"))
        result._percentage = obj.value("percentage").toInt();
    return result;
}

QJsonObject toJson(const WorkDoneProgressReport &data)
{
    QJsonObject obj{{"kind", QString("report")}};
    if (data._cancellable.has_value())
        obj.insert("cancellable", *data._cancellable);
    if (data._message.has_value())
        obj.insert("message", *data._message);
    if (data._percentage.has_value())
        obj.insert("percentage", *data._percentage);
    return obj;
}

template<>
Utils::Result<WorkDoneProgressEnd> fromJson<WorkDoneProgressEnd>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for WorkDoneProgressEnd");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("kind"))
        return Utils::ResultError("Missing required field: kind");
    WorkDoneProgressEnd result;
    if (obj.value("kind").toString() != "end")
        return Utils::ResultError("Field 'kind' must be 'end', got: " + obj.value("kind").toString());
    if (obj.contains("message"))
        result._message = obj.value("message").toString();
    return result;
}

QJsonObject toJson(const WorkDoneProgressEnd &data)
{
    QJsonObject obj{{"kind", QString("end")}};
    if (data._message.has_value())
        obj.insert("message", *data._message);
    return obj;
}

template<>
Utils::Result<SetTraceParams> fromJson<SetTraceParams>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for SetTraceParams");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("value"))
        return Utils::ResultError("Missing required field: value");
    SetTraceParams result;
    const auto res0 = fromJson<TraceValue>("value", obj["value"]);
    if (!res0)
        return Utils::ResultError(res0.error());
    result._value = *res0;
    return result;
}

QJsonObject toJson(const SetTraceParams &data)
{
    QJsonObject obj{{"value", toJsonValue(data._value)}};
    return obj;
}

template<>
Utils::Result<LogTraceParams> fromJson<LogTraceParams>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for LogTraceParams");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("message"))
        return Utils::ResultError("Missing required field: message");
    LogTraceParams result;
    result._message = obj.value("message").toString();
    if (obj.contains("verbose"))
        result._verbose = obj.value("verbose").toString();
    return result;
}

QJsonObject toJson(const LogTraceParams &data)
{
    QJsonObject obj{{"message", data._message}};
    if (data._verbose.has_value())
        obj.insert("verbose", *data._verbose);
    return obj;
}

template<>
Utils::Result<CancelParams> fromJson<CancelParams>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for CancelParams");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("id"))
        return Utils::ResultError("Missing required field: id");
    CancelParams result;
    if (obj.contains("id")) {
        const auto res0 = fromJson<ProgressToken>("id", obj["id"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._id = *res0;
    }
    return result;
}

QJsonObject toJson(const CancelParams &data)
{
    QJsonObject obj{{"id", toJsonValue(data._id)}};
    return obj;
}

template<>
Utils::Result<ProgressParams> fromJson<ProgressParams>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for ProgressParams");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("token"))
        return Utils::ResultError("Missing required field: token");
    if (!obj.contains("value"))
        return Utils::ResultError("Missing required field: value");
    ProgressParams result;
    if (obj.contains("token")) {
        const auto res0 = fromJson<ProgressToken>("token", obj["token"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._token = *res0;
    }
    result._value = obj.value("value");
    return result;
}

QJsonObject toJson(const ProgressParams &data)
{
    QJsonObject obj{
        {"token", toJsonValue(data._token)},
        {"value", data._value}
    };
    return obj;
}

template<>
Utils::Result<TextDocumentPositionParams> fromJson<TextDocumentPositionParams>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for TextDocumentPositionParams");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("textDocument"))
        return Utils::ResultError("Missing required field: textDocument");
    if (!obj.contains("position"))
        return Utils::ResultError("Missing required field: position");
    TextDocumentPositionParams result;
    if (obj.contains("textDocument") && obj["textDocument"].isObject()) {
        const auto res0 = fromJson<TextDocumentIdentifier>("textDocument", obj["textDocument"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._textDocument = *res0;
    }
    if (obj.contains("position") && obj["position"].isObject()) {
        const auto res1 = fromJson<Position>("position", obj["position"]);
        if (!res1)
            return Utils::ResultError(res1.error());
        result._position = *res1;
    }
    return result;
}

QJsonObject toJson(const TextDocumentPositionParams &data)
{
    QJsonObject obj{
        {"textDocument", toJson(data._textDocument)},
        {"position", toJson(data._position)}
    };
    return obj;
}

template<>
Utils::Result<WorkDoneProgressParams> fromJson<WorkDoneProgressParams>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for WorkDoneProgressParams");
    const QJsonObject obj = val.toObject();
    WorkDoneProgressParams result;
    if (obj.contains("workDoneToken")) {
        const auto res0 = fromJson<ProgressToken>("workDoneToken", obj["workDoneToken"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._workDoneToken = *res0;
    }
    return result;
}

QJsonObject toJson(const WorkDoneProgressParams &data)
{
    QJsonObject obj;
    if (data._workDoneToken.has_value())
        obj.insert("workDoneToken", toJsonValue(*data._workDoneToken));
    return obj;
}

template<>
Utils::Result<PartialResultParams> fromJson<PartialResultParams>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for PartialResultParams");
    const QJsonObject obj = val.toObject();
    PartialResultParams result;
    if (obj.contains("partialResultToken")) {
        const auto res0 = fromJson<ProgressToken>("partialResultToken", obj["partialResultToken"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._partialResultToken = *res0;
    }
    return result;
}

QJsonObject toJson(const PartialResultParams &data)
{
    QJsonObject obj;
    if (data._partialResultToken.has_value())
        obj.insert("partialResultToken", toJsonValue(*data._partialResultToken));
    return obj;
}

template<>
Utils::Result<LocationLink> fromJson<LocationLink>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for LocationLink");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("targetUri"))
        return Utils::ResultError("Missing required field: targetUri");
    if (!obj.contains("targetRange"))
        return Utils::ResultError("Missing required field: targetRange");
    if (!obj.contains("targetSelectionRange"))
        return Utils::ResultError("Missing required field: targetSelectionRange");
    LocationLink result;
    if (obj.contains("originSelectionRange") && obj["originSelectionRange"].isObject()) {
        const auto res0 = fromJson<Range>("originSelectionRange", obj["originSelectionRange"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._originSelectionRange = *res0;
    }
    result._targetUri = obj.value("targetUri").toString();
    if (obj.contains("targetRange") && obj["targetRange"].isObject()) {
        const auto res1 = fromJson<Range>("targetRange", obj["targetRange"]);
        if (!res1)
            return Utils::ResultError(res1.error());
        result._targetRange = *res1;
    }
    if (obj.contains("targetSelectionRange") && obj["targetSelectionRange"].isObject()) {
        const auto res2 = fromJson<Range>("targetSelectionRange", obj["targetSelectionRange"]);
        if (!res2)
            return Utils::ResultError(res2.error());
        result._targetSelectionRange = *res2;
    }
    return result;
}

QJsonObject toJson(const LocationLink &data)
{
    QJsonObject obj{
        {"targetUri", data._targetUri},
        {"targetRange", toJson(data._targetRange)},
        {"targetSelectionRange", toJson(data._targetSelectionRange)}
    };
    if (data._originSelectionRange.has_value())
        obj.insert("originSelectionRange", toJson(*data._originSelectionRange));
    return obj;
}

template<>
Utils::Result<StaticRegistrationOptions> fromJson<StaticRegistrationOptions>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for StaticRegistrationOptions");
    const QJsonObject obj = val.toObject();
    StaticRegistrationOptions result;
    if (obj.contains("id"))
        result._id = obj.value("id").toString();
    return result;
}

QJsonObject toJson(const StaticRegistrationOptions &data)
{
    QJsonObject obj;
    if (data._id.has_value())
        obj.insert("id", *data._id);
    return obj;
}

template<>
Utils::Result<InlineValueText> fromJson<InlineValueText>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for InlineValueText");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("range"))
        return Utils::ResultError("Missing required field: range");
    if (!obj.contains("text"))
        return Utils::ResultError("Missing required field: text");
    InlineValueText result;
    if (obj.contains("range") && obj["range"].isObject()) {
        const auto res0 = fromJson<Range>("range", obj["range"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._range = *res0;
    }
    result._text = obj.value("text").toString();
    return result;
}

QJsonObject toJson(const InlineValueText &data)
{
    QJsonObject obj{
        {"range", toJson(data._range)},
        {"text", data._text}
    };
    return obj;
}

template<>
Utils::Result<InlineValueVariableLookup> fromJson<InlineValueVariableLookup>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for InlineValueVariableLookup");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("range"))
        return Utils::ResultError("Missing required field: range");
    if (!obj.contains("caseSensitiveLookup"))
        return Utils::ResultError("Missing required field: caseSensitiveLookup");
    InlineValueVariableLookup result;
    if (obj.contains("range") && obj["range"].isObject()) {
        const auto res0 = fromJson<Range>("range", obj["range"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._range = *res0;
    }
    if (obj.contains("variableName"))
        result._variableName = obj.value("variableName").toString();
    result._caseSensitiveLookup = obj.value("caseSensitiveLookup").toBool();
    return result;
}

QJsonObject toJson(const InlineValueVariableLookup &data)
{
    QJsonObject obj{
        {"range", toJson(data._range)},
        {"caseSensitiveLookup", data._caseSensitiveLookup}
    };
    if (data._variableName.has_value())
        obj.insert("variableName", *data._variableName);
    return obj;
}

template<>
Utils::Result<InlineValueEvaluatableExpression> fromJson<InlineValueEvaluatableExpression>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for InlineValueEvaluatableExpression");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("range"))
        return Utils::ResultError("Missing required field: range");
    InlineValueEvaluatableExpression result;
    if (obj.contains("range") && obj["range"].isObject()) {
        const auto res0 = fromJson<Range>("range", obj["range"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._range = *res0;
    }
    if (obj.contains("expression"))
        result._expression = obj.value("expression").toString();
    return result;
}

QJsonObject toJson(const InlineValueEvaluatableExpression &data)
{
    QJsonObject obj{{"range", toJson(data._range)}};
    if (data._expression.has_value())
        obj.insert("expression", *data._expression);
    return obj;
}

template<>
Utils::Result<FullDocumentDiagnosticReport> fromJson<FullDocumentDiagnosticReport>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for FullDocumentDiagnosticReport");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("kind"))
        return Utils::ResultError("Missing required field: kind");
    if (!obj.contains("items"))
        return Utils::ResultError("Missing required field: items");
    FullDocumentDiagnosticReport result;
    if (obj.value("kind").toString() != "full")
        return Utils::ResultError("Field 'kind' must be 'full', got: " + obj.value("kind").toString());
    if (obj.contains("resultId"))
        result._resultId = obj.value("resultId").toString();
    if (obj.contains("items") && obj["items"].isArray()) {
        const QJsonArray arr = obj["items"].toArray();
        for (const QJsonValue &v : arr) {
            const auto res0 = fromJson<Diagnostic>("items", v);
            if (!res0)
                return Utils::ResultError(res0.error());
            result._items.append(*res0);
        }
    }
    return result;
}

QJsonObject toJson(const FullDocumentDiagnosticReport &data)
{
    QJsonObject obj{{"kind", QString("full")}};
    if (data._resultId.has_value())
        obj.insert("resultId", *data._resultId);
    QJsonArray arr_items;
    for (const auto &v : data._items) arr_items.append(toJson(v));
    obj.insert("items", arr_items);
    return obj;
}

template<>
Utils::Result<UnchangedDocumentDiagnosticReport> fromJson<UnchangedDocumentDiagnosticReport>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for UnchangedDocumentDiagnosticReport");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("kind"))
        return Utils::ResultError("Missing required field: kind");
    if (!obj.contains("resultId"))
        return Utils::ResultError("Missing required field: resultId");
    UnchangedDocumentDiagnosticReport result;
    if (obj.value("kind").toString() != "unchanged")
        return Utils::ResultError("Field 'kind' must be 'unchanged', got: " + obj.value("kind").toString());
    result._resultId = obj.value("resultId").toString();
    return result;
}

QJsonObject toJson(const UnchangedDocumentDiagnosticReport &data)
{
    QJsonObject obj{
        {"kind", QString("unchanged")},
        {"resultId", data._resultId}
    };
    return obj;
}

template<>
Utils::Result<RelatedFullDocumentDiagnosticReportRelatedDocumentsValue> fromJson<RelatedFullDocumentDiagnosticReportRelatedDocumentsValue>(const QJsonValue &val)
{
    if (val.isObject()) {
        auto result = fromJson<FullDocumentDiagnosticReport>(val);
        if (result) return RelatedFullDocumentDiagnosticReportRelatedDocumentsValue(*result);
    }
    if (val.isObject()) {
        auto result = fromJson<UnchangedDocumentDiagnosticReport>(val);
        if (result) return RelatedFullDocumentDiagnosticReportRelatedDocumentsValue(*result);
    }
    return Utils::ResultError("Invalid RelatedFullDocumentDiagnosticReportRelatedDocumentsValue");
}

QJsonValue toJsonValue(const RelatedFullDocumentDiagnosticReportRelatedDocumentsValue &val)
{
    return std::visit([](const auto &v) -> QJsonValue {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, FullDocumentDiagnosticReport>) return toJson(v);
        if constexpr (std::is_same_v<T, UnchangedDocumentDiagnosticReport>) return toJson(v);
        return QJsonValue{};
    }, val);
}

template<>
Utils::Result<RelatedFullDocumentDiagnosticReport> fromJson<RelatedFullDocumentDiagnosticReport>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for RelatedFullDocumentDiagnosticReport");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("kind"))
        return Utils::ResultError("Missing required field: kind");
    if (!obj.contains("items"))
        return Utils::ResultError("Missing required field: items");
    RelatedFullDocumentDiagnosticReport result;
    if (obj.value("kind").toString() != "full")
        return Utils::ResultError("Field 'kind' must be 'full', got: " + obj.value("kind").toString());
    if (obj.contains("resultId"))
        result._resultId = obj.value("resultId").toString();
    if (obj.contains("items") && obj["items"].isArray()) {
        const QJsonArray arr = obj["items"].toArray();
        for (const QJsonValue &v : arr) {
            const auto res0 = fromJson<Diagnostic>("items", v);
            if (!res0)
                return Utils::ResultError(res0.error());
            result._items.append(*res0);
        }
    }
    if (obj.contains("relatedDocuments") && obj["relatedDocuments"].isObject()) {
        const QJsonObject mapObj_relatedDocuments = obj["relatedDocuments"].toObject();
        QMap<QString, RelatedFullDocumentDiagnosticReportRelatedDocumentsValue> map_relatedDocuments;
        for (auto it = mapObj_relatedDocuments.constBegin(); it != mapObj_relatedDocuments.constEnd(); ++it) {
            const auto res1 = fromJson<RelatedFullDocumentDiagnosticReportRelatedDocumentsValue>("relatedDocuments", it.value());
            if (!res1)
                return Utils::ResultError(res1.error());
            map_relatedDocuments.insert(it.key(), *res1);
        }
        result._relatedDocuments = map_relatedDocuments;
    }
    return result;
}

QJsonObject toJson(const RelatedFullDocumentDiagnosticReport &data)
{
    QJsonObject obj{{"kind", QString("full")}};
    if (data._resultId.has_value())
        obj.insert("resultId", *data._resultId);
    QJsonArray arr_items;
    for (const auto &v : data._items) arr_items.append(toJson(v));
    obj.insert("items", arr_items);
    if (data._relatedDocuments.has_value()) {
        QJsonObject map_relatedDocuments;
        for (auto it = data._relatedDocuments->constBegin(); it != data._relatedDocuments->constEnd(); ++it)
            map_relatedDocuments.insert(it.key(), toJsonValue(it.value()));
        obj.insert("relatedDocuments", map_relatedDocuments);
    }
    return obj;
}

template<>
Utils::Result<RelatedUnchangedDocumentDiagnosticReport> fromJson<RelatedUnchangedDocumentDiagnosticReport>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for RelatedUnchangedDocumentDiagnosticReport");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("kind"))
        return Utils::ResultError("Missing required field: kind");
    if (!obj.contains("resultId"))
        return Utils::ResultError("Missing required field: resultId");
    RelatedUnchangedDocumentDiagnosticReport result;
    if (obj.value("kind").toString() != "unchanged")
        return Utils::ResultError("Field 'kind' must be 'unchanged', got: " + obj.value("kind").toString());
    result._resultId = obj.value("resultId").toString();
    if (obj.contains("relatedDocuments") && obj["relatedDocuments"].isObject()) {
        const QJsonObject mapObj_relatedDocuments = obj["relatedDocuments"].toObject();
        QMap<QString, RelatedFullDocumentDiagnosticReportRelatedDocumentsValue> map_relatedDocuments;
        for (auto it = mapObj_relatedDocuments.constBegin(); it != mapObj_relatedDocuments.constEnd(); ++it) {
            const auto res0 = fromJson<RelatedFullDocumentDiagnosticReportRelatedDocumentsValue>("relatedDocuments", it.value());
            if (!res0)
                return Utils::ResultError(res0.error());
            map_relatedDocuments.insert(it.key(), *res0);
        }
        result._relatedDocuments = map_relatedDocuments;
    }
    return result;
}

QJsonObject toJson(const RelatedUnchangedDocumentDiagnosticReport &data)
{
    QJsonObject obj{
        {"kind", QString("unchanged")},
        {"resultId", data._resultId}
    };
    if (data._relatedDocuments.has_value()) {
        QJsonObject map_relatedDocuments;
        for (auto it = data._relatedDocuments->constBegin(); it != data._relatedDocuments->constEnd(); ++it)
            map_relatedDocuments.insert(it.key(), toJsonValue(it.value()));
        obj.insert("relatedDocuments", map_relatedDocuments);
    }
    return obj;
}

template<>
Utils::Result<DocumentDiagnosticReportPartialResult> fromJson<DocumentDiagnosticReportPartialResult>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for DocumentDiagnosticReportPartialResult");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("relatedDocuments"))
        return Utils::ResultError("Missing required field: relatedDocuments");
    DocumentDiagnosticReportPartialResult result;
    if (obj.contains("relatedDocuments") && obj["relatedDocuments"].isObject()) {
        const QJsonObject mapObj_relatedDocuments = obj["relatedDocuments"].toObject();
        QMap<QString, RelatedFullDocumentDiagnosticReportRelatedDocumentsValue> map_relatedDocuments;
        for (auto it = mapObj_relatedDocuments.constBegin(); it != mapObj_relatedDocuments.constEnd(); ++it) {
            const auto res0 = fromJson<RelatedFullDocumentDiagnosticReportRelatedDocumentsValue>("relatedDocuments", it.value());
            if (!res0)
                return Utils::ResultError(res0.error());
            map_relatedDocuments.insert(it.key(), *res0);
        }
        result._relatedDocuments = map_relatedDocuments;
    }
    return result;
}

QJsonObject toJson(const DocumentDiagnosticReportPartialResult &data)
{
    QJsonObject obj;
    QJsonObject map_relatedDocuments;
    for (auto it = data._relatedDocuments.constBegin(); it != data._relatedDocuments.constEnd(); ++it)
        map_relatedDocuments.insert(it.key(), toJsonValue(it.value()));
    obj.insert("relatedDocuments", map_relatedDocuments);
    return obj;
}

template<>
Utils::Result<_InitializeParams> fromJson<_InitializeParams>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for _InitializeParams");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("processId"))
        return Utils::ResultError("Missing required field: processId");
    if (!obj.contains("rootUri"))
        return Utils::ResultError("Missing required field: rootUri");
    if (!obj.contains("capabilities"))
        return Utils::ResultError("Missing required field: capabilities");
    _InitializeParams result;
    if (obj.contains("workDoneToken")) {
        const auto res0 = fromJson<ProgressToken>("workDoneToken", obj["workDoneToken"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._workDoneToken = *res0;
    }
    if (!obj["processId"].isNull()) {
        result._processId = obj.value("processId").toInt();
    }
    if (obj.contains("clientInfo") && obj["clientInfo"].isObject()) {
        const auto res1 = fromJson<ClientInfo>("clientInfo", obj["clientInfo"]);
        if (!res1)
            return Utils::ResultError(res1.error());
        result._clientInfo = *res1;
    }
    if (obj.contains("locale"))
        result._locale = obj.value("locale").toString();
    if (obj.contains("rootPath"))
        if (!obj["rootPath"].isNull()) {
            result._rootPath = obj.value("rootPath").toString();
        }
    if (!obj["rootUri"].isNull()) {
        result._rootUri = obj.value("rootUri").toString();
    }
    if (obj.contains("capabilities") && obj["capabilities"].isObject()) {
        const auto res2 = fromJson<ClientCapabilities>("capabilities", obj["capabilities"]);
        if (!res2)
            return Utils::ResultError(res2.error());
        result._capabilities = *res2;
    }
    if (obj.contains("initializationOptions"))
        result._initializationOptions = obj.value("initializationOptions");
    if (obj.contains("trace")) {
        const auto res3 = fromJson<TraceValue>("trace", obj["trace"]);
        if (!res3)
            return Utils::ResultError(res3.error());
        result._trace = *res3;
    }
    return result;
}

QJsonObject toJson(const _InitializeParams &data)
{
    QJsonObject obj{{"capabilities", toJson(data._capabilities)}};
    if (data._workDoneToken.has_value())
        obj.insert("workDoneToken", toJsonValue(*data._workDoneToken));
    if (data._processId.has_value())
        obj.insert("processId", *data._processId);
    else
        obj.insert("processId", QJsonValue::Null);
    if (data._clientInfo.has_value())
        obj.insert("clientInfo", toJson(*data._clientInfo));
    if (data._locale.has_value())
        obj.insert("locale", *data._locale);
    if (data._rootPath.has_value())
        obj.insert("rootPath", *data._rootPath);
    if (data._rootUri.has_value())
        obj.insert("rootUri", *data._rootUri);
    else
        obj.insert("rootUri", QJsonValue::Null);
    if (data._initializationOptions.has_value())
        obj.insert("initializationOptions", *data._initializationOptions);
    if (data._trace.has_value())
        obj.insert("trace", toJsonValue(*data._trace));
    return obj;
}

template<>
Utils::Result<WorkspaceFoldersInitializeParams> fromJson<WorkspaceFoldersInitializeParams>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for WorkspaceFoldersInitializeParams");
    const QJsonObject obj = val.toObject();
    WorkspaceFoldersInitializeParams result;
    if (obj.contains("workspaceFolders")) {
        const auto res0 = fromJson<InitializeParamsWorkspaceFolders>("workspaceFolders", obj["workspaceFolders"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._workspaceFolders = *res0;
    }
    return result;
}

QJsonObject toJson(const WorkspaceFoldersInitializeParams &data)
{
    QJsonObject obj;
    if (data._workspaceFolders.has_value())
        obj.insert("workspaceFolders", toJsonValue(*data._workspaceFolders));
    return obj;
}

template<>
Utils::Result<BaseSymbolInformation> fromJson<BaseSymbolInformation>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for BaseSymbolInformation");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("name"))
        return Utils::ResultError("Missing required field: name");
    if (!obj.contains("kind"))
        return Utils::ResultError("Missing required field: kind");
    BaseSymbolInformation result;
    result._name = obj.value("name").toString();
    result._kind = obj.value("kind").toInt();
    if (obj.contains("tags") && obj["tags"].isArray()) {
        const QJsonArray arr = obj["tags"].toArray();
        QList<int> list_tags;
        for (const QJsonValue &v : arr) {
            list_tags.append(v.toInt());
        }
        result._tags = list_tags;
    }
    if (obj.contains("containerName"))
        result._containerName = obj.value("containerName").toString();
    return result;
}

QJsonObject toJson(const BaseSymbolInformation &data)
{
    QJsonObject obj{
        {"name", data._name},
        {"kind", data._kind}
    };
    if (data._tags.has_value()) {
        QJsonArray arr_tags;
        for (const auto &v : *data._tags) arr_tags.append(v);
        obj.insert("tags", arr_tags);
    }
    if (data._containerName.has_value())
        obj.insert("containerName", *data._containerName);
    return obj;
}

template<>
Utils::Result<PrepareRenamePlaceholder> fromJson<PrepareRenamePlaceholder>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for PrepareRenamePlaceholder");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("range"))
        return Utils::ResultError("Missing required field: range");
    if (!obj.contains("placeholder"))
        return Utils::ResultError("Missing required field: placeholder");
    PrepareRenamePlaceholder result;
    if (obj.contains("range") && obj["range"].isObject()) {
        const auto res0 = fromJson<Range>("range", obj["range"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._range = *res0;
    }
    result._placeholder = obj.value("placeholder").toString();
    return result;
}

QJsonObject toJson(const PrepareRenamePlaceholder &data)
{
    QJsonObject obj{
        {"range", toJson(data._range)},
        {"placeholder", data._placeholder}
    };
    return obj;
}

template<>
Utils::Result<PrepareRenameDefaultBehavior> fromJson<PrepareRenameDefaultBehavior>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for PrepareRenameDefaultBehavior");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("defaultBehavior"))
        return Utils::ResultError("Missing required field: defaultBehavior");
    PrepareRenameDefaultBehavior result;
    result._defaultBehavior = obj.value("defaultBehavior").toBool();
    return result;
}

QJsonObject toJson(const PrepareRenameDefaultBehavior &data)
{
    QJsonObject obj{{"defaultBehavior", data._defaultBehavior}};
    return obj;
}

template<>
Utils::Result<WorkDoneProgressOptions> fromJson<WorkDoneProgressOptions>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for WorkDoneProgressOptions");
    const QJsonObject obj = val.toObject();
    WorkDoneProgressOptions result;
    if (obj.contains("workDoneProgress"))
        result._workDoneProgress = obj.value("workDoneProgress").toBool();
    return result;
}

QJsonObject toJson(const WorkDoneProgressOptions &data)
{
    QJsonObject obj;
    if (data._workDoneProgress.has_value())
        obj.insert("workDoneProgress", *data._workDoneProgress);
    return obj;
}

template<>
Utils::Result<ResourceOperation> fromJson<ResourceOperation>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for ResourceOperation");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("kind"))
        return Utils::ResultError("Missing required field: kind");
    ResourceOperation result;
    result._kind = obj.value("kind").toString();
    if (obj.contains("annotationId") && obj["annotationId"].isString()) {
        const auto res0 = fromJson<ChangeAnnotationIdentifier>("annotationId", obj["annotationId"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._annotationId = *res0;
    }
    return result;
}

QJsonObject toJson(const ResourceOperation &data)
{
    QJsonObject obj{{"kind", data._kind}};
    if (data._annotationId.has_value())
        obj.insert("annotationId", *data._annotationId);
    return obj;
}

template<>
Utils::Result<DiagnosticsCapabilities> fromJson<DiagnosticsCapabilities>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for DiagnosticsCapabilities");
    const QJsonObject obj = val.toObject();
    DiagnosticsCapabilities result;
    if (obj.contains("relatedInformation"))
        result._relatedInformation = obj.value("relatedInformation").toBool();
    if (obj.contains("tagSupport") && obj["tagSupport"].isObject()) {
        const auto res0 = fromJson<ClientDiagnosticsTagOptions>("tagSupport", obj["tagSupport"]);
        if (!res0)
            return Utils::ResultError(res0.error());
        result._tagSupport = *res0;
    }
    if (obj.contains("codeDescriptionSupport"))
        result._codeDescriptionSupport = obj.value("codeDescriptionSupport").toBool();
    if (obj.contains("dataSupport"))
        result._dataSupport = obj.value("dataSupport").toBool();
    return result;
}

QJsonObject toJson(const DiagnosticsCapabilities &data)
{
    QJsonObject obj;
    if (data._relatedInformation.has_value())
        obj.insert("relatedInformation", *data._relatedInformation);
    if (data._tagSupport.has_value())
        obj.insert("tagSupport", toJson(*data._tagSupport));
    if (data._codeDescriptionSupport.has_value())
        obj.insert("codeDescriptionSupport", *data._codeDescriptionSupport);
    if (data._dataSupport.has_value())
        obj.insert("dataSupport", *data._dataSupport);
    return obj;
}

QString toString(DocumentDiagnosticReportKind v)
{
    switch(v) {
        case DocumentDiagnosticReportKind::full: return "full";
        case DocumentDiagnosticReportKind::unchanged: return "unchanged";
    }
    return {};
}

template<>
Utils::Result<DocumentDiagnosticReportKind> fromJson<DocumentDiagnosticReportKind>(const QJsonValue &val)
{
    if (!val.isString())
        return Utils::ResultError("Expected JSON string for DocumentDiagnosticReportKind");
    const QString str = val.toString();
    if (str == "full") return DocumentDiagnosticReportKind::full;
    if (str == "unchanged") return DocumentDiagnosticReportKind::unchanged;
    return Utils::ResultError("Invalid DocumentDiagnosticReportKind value: " + str);
}

QJsonValue toJsonValue(const DocumentDiagnosticReportKind &v)
{
    return toString(v);
}

template<>
Utils::Result<Definition> fromJson<Definition>(const QJsonValue &val)
{
    if (val.isArray()) {
        bool ok = true;
        QList<Location> list;
        for (const auto &elem : val.toArray()) {
            auto r = fromJson<Location>(elem);
            if (!r) { ok = false; break; }
            list.append(*r);
        }
        if (ok) return Definition(std::move(list));
    }
    if (!val.isObject())
        return Utils::ResultError("Invalid Definition: expected object");
    const QJsonObject obj = val.toObject();
    if (obj.contains("range")) {
        const auto res0 = fromJson<Location>(val);
        if (!res0)
            return Utils::ResultError(res0.error());
        return Definition(*res0);
    }
    return Utils::ResultError("Invalid Definition");
}

QJsonValue toJsonValue(const Definition &val)
{
    return std::visit([](const auto &v) -> QJsonValue {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, Location>) {
            return toJson(v);
        } else
        if constexpr (std::is_same_v<T, QList<Location>>) {
            QJsonArray arr;
            for (const auto &elem : v)
                arr.append(toJson(elem));
            return arr;
        } else
        {
            return QVariant::fromValue(v).toJsonValue();
        }
    }, val);
}

template<>
Utils::Result<InlineValue> fromJson<InlineValue>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Invalid InlineValue: expected object");
    const QJsonObject obj = val.toObject();
    if (obj.contains("text")) {
        const auto res0 = fromJson<InlineValueText>(val);
        if (!res0)
            return Utils::ResultError(res0.error());
        return InlineValue(*res0);
    }
    if (obj.contains("caseSensitiveLookup")) {
        const auto res1 = fromJson<InlineValueVariableLookup>(val);
        if (!res1)
            return Utils::ResultError(res1.error());
        return InlineValue(*res1);
    }
    {
        auto result = fromJson<InlineValueEvaluatableExpression>(val);
        if (result) return InlineValue(*result);
    }
    return Utils::ResultError("Invalid InlineValue");
}

Range range(const InlineValue &val)
{
    return std::visit([](const auto &v) -> Range { return v._range; }, val);
}

QJsonObject toJson(const InlineValue &val)
{
    return std::visit([](const auto &v) -> QJsonObject {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, QJsonObject>) {
            return v;
        } else {
            return toJson(v);
        }
    }, val);
}

QJsonValue toJsonValue(const InlineValue &val)
{
    return toJson(val);
}

template<>
Utils::Result<DocumentDiagnosticReport> fromJson<DocumentDiagnosticReport>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Invalid DocumentDiagnosticReport: expected object");
    const QString dispatchValue = val.toObject().value("kind").toString();
    if (dispatchValue == "full") {
        const auto res0 = fromJson<RelatedFullDocumentDiagnosticReport>(val);
        if (!res0)
            return Utils::ResultError(res0.error());
        return DocumentDiagnosticReport(*res0);
    }
    else if (dispatchValue == "unchanged") {
        const auto res1 = fromJson<RelatedUnchangedDocumentDiagnosticReport>(val);
        if (!res1)
            return Utils::ResultError(res1.error());
        return DocumentDiagnosticReport(*res1);
    }
    return Utils::ResultError("Invalid DocumentDiagnosticReport: unknown kind \"" + dispatchValue + "\"");
}

QJsonObject toJson(const DocumentDiagnosticReport &val)
{
    return std::visit([](const auto &v) -> QJsonObject {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, QJsonObject>) {
            return v;
        } else {
            return toJson(v);
        }
    }, val);
}

QJsonValue toJsonValue(const DocumentDiagnosticReport &val)
{
    return toJson(val);
}

QString dispatchValue(const DocumentDiagnosticReport &val)
{
    return std::visit([](const auto &v) -> QString {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, RelatedFullDocumentDiagnosticReport>) return "full";
        else if constexpr (std::is_same_v<T, RelatedUnchangedDocumentDiagnosticReport>) return "unchanged";
        return {};
    }, val);
}

template<>
Utils::Result<DocumentDiagnosticReportProgress> fromJson<DocumentDiagnosticReportProgress>(const QJsonValue &val)
{
    if (!val.isObject()) {
        {
            auto result = fromJson<DocumentDiagnosticReport>(val);
            if (result) return DocumentDiagnosticReportProgress(*result);
        }
        return Utils::ResultError("Invalid DocumentDiagnosticReportProgress: expected object");
    }
    const QJsonObject obj = val.toObject();
    if (obj.contains("relatedDocuments")) {
        const auto res0 = fromJson<DocumentDiagnosticReportPartialResult>(val);
        if (!res0)
            return Utils::ResultError(res0.error());
        return DocumentDiagnosticReportProgress(*res0);
    }
    {
        auto result = fromJson<DocumentDiagnosticReport>(val);
        if (result) return DocumentDiagnosticReportProgress(*result);
    }
    return Utils::ResultError("Invalid DocumentDiagnosticReportProgress");
}

QJsonObject toJson(const DocumentDiagnosticReportProgress &val)
{
    return std::visit([](const auto &v) -> QJsonObject {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, QJsonObject>) {
            return v;
        } else {
            return toJson(v);
        }
    }, val);
}

QJsonValue toJsonValue(const DocumentDiagnosticReportProgress &val)
{
    return toJson(val);
}

template<>
Utils::Result<PrepareRenameResult> fromJson<PrepareRenameResult>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Invalid PrepareRenameResult: expected object");
    const QJsonObject obj = val.toObject();
    if (obj.contains("end")) {
        const auto res0 = fromJson<Range>(val);
        if (!res0)
            return Utils::ResultError(res0.error());
        return PrepareRenameResult(*res0);
    }
    if (obj.contains("placeholder")) {
        const auto res1 = fromJson<PrepareRenamePlaceholder>(val);
        if (!res1)
            return Utils::ResultError(res1.error());
        return PrepareRenameResult(*res1);
    }
    if (obj.contains("defaultBehavior")) {
        const auto res2 = fromJson<PrepareRenameDefaultBehavior>(val);
        if (!res2)
            return Utils::ResultError(res2.error());
        return PrepareRenameResult(*res2);
    }
    return Utils::ResultError("Invalid PrepareRenameResult");
}

QJsonObject toJson(const PrepareRenameResult &val)
{
    return std::visit([](const auto &v) -> QJsonObject {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, QJsonObject>) {
            return v;
        } else {
            return toJson(v);
        }
    }, val);
}

QJsonValue toJsonValue(const PrepareRenameResult &val)
{
    return toJson(val);
}

template<>
Utils::Result<ImplementationRequestResult> fromJson<ImplementationRequestResult>(const QJsonValue &val)
{
    if (val.isArray()) {
        bool ok = true;
        QList<DefinitionLink> list;
        for (const auto &elem : val.toArray()) {
            auto r = fromJson<DefinitionLink>(elem);
            if (!r) { ok = false; break; }
            list.append(*r);
        }
        if (ok) return ImplementationRequestResult(std::move(list));
    }
    if (val.isNull())
        return ImplementationRequestResult(std::monostate{});
    if (!val.isObject()) {
        {
            auto result = fromJson<Definition>(val);
            if (result) return ImplementationRequestResult(*result);
        }
    }
    if (val.isObject()) {
        auto result = fromJson<Definition>(val);
        if (result) return ImplementationRequestResult(*result);
    }
    return Utils::ResultError("Invalid ImplementationRequestResult");
}

QJsonValue toJsonValue(const ImplementationRequestResult &val)
{
    return std::visit([](const auto &v) -> QJsonValue {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, Definition>) {
            return toJsonValue(v);
        } else
        if constexpr (std::is_same_v<T, QList<DefinitionLink>>) {
            QJsonArray arr;
            for (const auto &elem : v)
                arr.append(toJson(elem));
            return arr;
        } else
        if constexpr (std::is_same_v<T, std::monostate>) {
            return QJsonValue(QJsonValue::Null);
        } else
        {
            return QVariant::fromValue(v).toJsonValue();
        }
    }, val);
}

template<>
Utils::Result<ImplementationRequestPartialResult> fromJson<ImplementationRequestPartialResult>(const QJsonValue &val)
{
    if (val.isArray()) {
        bool ok = true;
        QList<Location> list;
        for (const auto &elem : val.toArray()) {
            auto r = fromJson<Location>(elem);
            if (!r) { ok = false; break; }
            list.append(*r);
        }
        if (ok) return ImplementationRequestPartialResult(std::move(list));
    }
    if (val.isArray()) {
        bool ok = true;
        QList<DefinitionLink> list;
        for (const auto &elem : val.toArray()) {
            auto r = fromJson<DefinitionLink>(elem);
            if (!r) { ok = false; break; }
            list.append(*r);
        }
        if (ok) return ImplementationRequestPartialResult(std::move(list));
    }
    return Utils::ResultError("Invalid ImplementationRequestPartialResult");
}

QJsonValue toJsonValue(const ImplementationRequestPartialResult &val)
{
    return std::visit([](const auto &v) -> QJsonValue {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, QList<Location>>) {
            QJsonArray arr;
            for (const auto &elem : v)
                arr.append(toJson(elem));
            return arr;
        } else
        if constexpr (std::is_same_v<T, QList<DefinitionLink>>) {
            QJsonArray arr;
            for (const auto &elem : v)
                arr.append(toJson(elem));
            return arr;
        } else
        {
            return QVariant::fromValue(v).toJsonValue();
        }
    }, val);
}

template<>
Utils::Result<DocumentColorRequestResult> fromJson<DocumentColorRequestResult>(const QJsonValue &val)
{
    if (!val.isArray())
        return Utils::ResultError("Expected JSON array for DocumentColorRequestResult");
    DocumentColorRequestResult result;
    for (const QJsonValue &v : val.toArray()) {
        const auto res0 = fromJson<ColorInformation>(v);
        if (!res0)
            return Utils::ResultError(res0.error());
        result.append(*res0);
    }
    return result;
}

QJsonArray toJson(const DocumentColorRequestResult &data)
{
    QJsonArray arr;
    for (const auto &v : data) arr.append(toJson(v));
    return arr;
}

template<>
Utils::Result<ColorPresentationRequestResult> fromJson<ColorPresentationRequestResult>(const QJsonValue &val)
{
    if (!val.isArray())
        return Utils::ResultError("Expected JSON array for ColorPresentationRequestResult");
    ColorPresentationRequestResult result;
    for (const QJsonValue &v : val.toArray()) {
        const auto res0 = fromJson<ColorPresentation>(v);
        if (!res0)
            return Utils::ResultError(res0.error());
        result.append(*res0);
    }
    return result;
}

QJsonArray toJson(const ColorPresentationRequestResult &data)
{
    QJsonArray arr;
    for (const auto &v : data) arr.append(toJson(v));
    return arr;
}

template<>
Utils::Result<FoldingRangeRequestResult> fromJson<FoldingRangeRequestResult>(const QJsonValue &val)
{
    if (val.isArray()) {
        bool ok = true;
        QList<FoldingRange> list;
        for (const auto &elem : val.toArray()) {
            auto r = fromJson<FoldingRange>(elem);
            if (!r) { ok = false; break; }
            list.append(*r);
        }
        if (ok) return FoldingRangeRequestResult(std::move(list));
    }
    if (val.isNull())
        return FoldingRangeRequestResult(std::monostate{});
    return Utils::ResultError("Invalid FoldingRangeRequestResult");
}

QJsonValue toJsonValue(const FoldingRangeRequestResult &val)
{
    return std::visit([](const auto &v) -> QJsonValue {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, std::monostate>) {
            return QJsonValue(QJsonValue::Null);
        } else
        if constexpr (std::is_same_v<T, QList<FoldingRange>>) {
            QJsonArray arr;
            for (const auto &elem : v)
                arr.append(toJson(elem));
            return arr;
        } else
        {
            return QVariant::fromValue(v).toJsonValue();
        }
    }, val);
}

template<>
Utils::Result<FoldingRangeRequestPartialResult> fromJson<FoldingRangeRequestPartialResult>(const QJsonValue &val)
{
    if (!val.isArray())
        return Utils::ResultError("Expected JSON array for FoldingRangeRequestPartialResult");
    FoldingRangeRequestPartialResult result;
    for (const QJsonValue &v : val.toArray()) {
        const auto res0 = fromJson<FoldingRange>(v);
        if (!res0)
            return Utils::ResultError(res0.error());
        result.append(*res0);
    }
    return result;
}

QJsonArray toJson(const FoldingRangeRequestPartialResult &data)
{
    QJsonArray arr;
    for (const auto &v : data) arr.append(toJson(v));
    return arr;
}

template<>
Utils::Result<SelectionRangeRequestResult> fromJson<SelectionRangeRequestResult>(const QJsonValue &val)
{
    if (val.isArray()) {
        bool ok = true;
        QList<SelectionRange> list;
        for (const auto &elem : val.toArray()) {
            auto r = fromJson<SelectionRange>(elem);
            if (!r) { ok = false; break; }
            list.append(*r);
        }
        if (ok) return SelectionRangeRequestResult(std::move(list));
    }
    if (val.isNull())
        return SelectionRangeRequestResult(std::monostate{});
    return Utils::ResultError("Invalid SelectionRangeRequestResult");
}

QJsonValue toJsonValue(const SelectionRangeRequestResult &val)
{
    return std::visit([](const auto &v) -> QJsonValue {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, std::monostate>) {
            return QJsonValue(QJsonValue::Null);
        } else
        if constexpr (std::is_same_v<T, QList<SelectionRange>>) {
            QJsonArray arr;
            for (const auto &elem : v)
                arr.append(toJson(elem));
            return arr;
        } else
        {
            return QVariant::fromValue(v).toJsonValue();
        }
    }, val);
}

template<>
Utils::Result<SelectionRangeRequestPartialResult> fromJson<SelectionRangeRequestPartialResult>(const QJsonValue &val)
{
    if (!val.isArray())
        return Utils::ResultError("Expected JSON array for SelectionRangeRequestPartialResult");
    SelectionRangeRequestPartialResult result;
    for (const QJsonValue &v : val.toArray()) {
        const auto res0 = fromJson<SelectionRange>(v);
        if (!res0)
            return Utils::ResultError(res0.error());
        result.append(*res0);
    }
    return result;
}

QJsonArray toJson(const SelectionRangeRequestPartialResult &data)
{
    QJsonArray arr;
    for (const auto &v : data) arr.append(toJson(v));
    return arr;
}

template<>
Utils::Result<CallHierarchyPrepareRequestResult> fromJson<CallHierarchyPrepareRequestResult>(const QJsonValue &val)
{
    if (val.isArray()) {
        bool ok = true;
        QList<CallHierarchyItem> list;
        for (const auto &elem : val.toArray()) {
            auto r = fromJson<CallHierarchyItem>(elem);
            if (!r) { ok = false; break; }
            list.append(*r);
        }
        if (ok) return CallHierarchyPrepareRequestResult(std::move(list));
    }
    if (val.isNull())
        return CallHierarchyPrepareRequestResult(std::monostate{});
    return Utils::ResultError("Invalid CallHierarchyPrepareRequestResult");
}

QJsonValue toJsonValue(const CallHierarchyPrepareRequestResult &val)
{
    return std::visit([](const auto &v) -> QJsonValue {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, std::monostate>) {
            return QJsonValue(QJsonValue::Null);
        } else
        if constexpr (std::is_same_v<T, QList<CallHierarchyItem>>) {
            QJsonArray arr;
            for (const auto &elem : v)
                arr.append(toJson(elem));
            return arr;
        } else
        {
            return QVariant::fromValue(v).toJsonValue();
        }
    }, val);
}

template<>
Utils::Result<CallHierarchyIncomingCallsRequestResult> fromJson<CallHierarchyIncomingCallsRequestResult>(const QJsonValue &val)
{
    if (val.isArray()) {
        bool ok = true;
        QList<CallHierarchyIncomingCall> list;
        for (const auto &elem : val.toArray()) {
            auto r = fromJson<CallHierarchyIncomingCall>(elem);
            if (!r) { ok = false; break; }
            list.append(*r);
        }
        if (ok) return CallHierarchyIncomingCallsRequestResult(std::move(list));
    }
    if (val.isNull())
        return CallHierarchyIncomingCallsRequestResult(std::monostate{});
    return Utils::ResultError("Invalid CallHierarchyIncomingCallsRequestResult");
}

QJsonValue toJsonValue(const CallHierarchyIncomingCallsRequestResult &val)
{
    return std::visit([](const auto &v) -> QJsonValue {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, std::monostate>) {
            return QJsonValue(QJsonValue::Null);
        } else
        if constexpr (std::is_same_v<T, QList<CallHierarchyIncomingCall>>) {
            QJsonArray arr;
            for (const auto &elem : v)
                arr.append(toJson(elem));
            return arr;
        } else
        {
            return QVariant::fromValue(v).toJsonValue();
        }
    }, val);
}

template<>
Utils::Result<CallHierarchyIncomingCallsRequestPartialResult> fromJson<CallHierarchyIncomingCallsRequestPartialResult>(const QJsonValue &val)
{
    if (!val.isArray())
        return Utils::ResultError("Expected JSON array for CallHierarchyIncomingCallsRequestPartialResult");
    CallHierarchyIncomingCallsRequestPartialResult result;
    for (const QJsonValue &v : val.toArray()) {
        const auto res0 = fromJson<CallHierarchyIncomingCall>(v);
        if (!res0)
            return Utils::ResultError(res0.error());
        result.append(*res0);
    }
    return result;
}

QJsonArray toJson(const CallHierarchyIncomingCallsRequestPartialResult &data)
{
    QJsonArray arr;
    for (const auto &v : data) arr.append(toJson(v));
    return arr;
}

template<>
Utils::Result<CallHierarchyOutgoingCallsRequestResult> fromJson<CallHierarchyOutgoingCallsRequestResult>(const QJsonValue &val)
{
    if (val.isArray()) {
        bool ok = true;
        QList<CallHierarchyOutgoingCall> list;
        for (const auto &elem : val.toArray()) {
            auto r = fromJson<CallHierarchyOutgoingCall>(elem);
            if (!r) { ok = false; break; }
            list.append(*r);
        }
        if (ok) return CallHierarchyOutgoingCallsRequestResult(std::move(list));
    }
    if (val.isNull())
        return CallHierarchyOutgoingCallsRequestResult(std::monostate{});
    return Utils::ResultError("Invalid CallHierarchyOutgoingCallsRequestResult");
}

QJsonValue toJsonValue(const CallHierarchyOutgoingCallsRequestResult &val)
{
    return std::visit([](const auto &v) -> QJsonValue {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, std::monostate>) {
            return QJsonValue(QJsonValue::Null);
        } else
        if constexpr (std::is_same_v<T, QList<CallHierarchyOutgoingCall>>) {
            QJsonArray arr;
            for (const auto &elem : v)
                arr.append(toJson(elem));
            return arr;
        } else
        {
            return QVariant::fromValue(v).toJsonValue();
        }
    }, val);
}

template<>
Utils::Result<CallHierarchyOutgoingCallsRequestPartialResult> fromJson<CallHierarchyOutgoingCallsRequestPartialResult>(const QJsonValue &val)
{
    if (!val.isArray())
        return Utils::ResultError("Expected JSON array for CallHierarchyOutgoingCallsRequestPartialResult");
    CallHierarchyOutgoingCallsRequestPartialResult result;
    for (const QJsonValue &v : val.toArray()) {
        const auto res0 = fromJson<CallHierarchyOutgoingCall>(v);
        if (!res0)
            return Utils::ResultError(res0.error());
        result.append(*res0);
    }
    return result;
}

QJsonArray toJson(const CallHierarchyOutgoingCallsRequestPartialResult &data)
{
    QJsonArray arr;
    for (const auto &v : data) arr.append(toJson(v));
    return arr;
}

template<>
Utils::Result<SemanticTokensRequestResult> fromJson<SemanticTokensRequestResult>(const QJsonValue &val)
{
    if (val.isNull())
        return SemanticTokensRequestResult(std::monostate{});
    if (!val.isObject())
        return Utils::ResultError("Invalid SemanticTokensRequestResult: expected object");
    const QJsonObject obj = val.toObject();
    if (obj.contains("data")) {
        const auto res0 = fromJson<SemanticTokens>(val);
        if (!res0)
            return Utils::ResultError(res0.error());
        return SemanticTokensRequestResult(*res0);
    }
    return Utils::ResultError("Invalid SemanticTokensRequestResult");
}

QJsonValue toJsonValue(const SemanticTokensRequestResult &val)
{
    return std::visit([](const auto &v) -> QJsonValue {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, SemanticTokens>) {
            return toJson(v);
        } else
        if constexpr (std::is_same_v<T, std::monostate>) {
            return QJsonValue(QJsonValue::Null);
        } else
        {
            return QVariant::fromValue(v).toJsonValue();
        }
    }, val);
}

template<>
Utils::Result<SemanticTokensDeltaRequestResult> fromJson<SemanticTokensDeltaRequestResult>(const QJsonValue &val)
{
    if (val.isNull())
        return SemanticTokensDeltaRequestResult(std::monostate{});
    if (!val.isObject())
        return Utils::ResultError("Invalid SemanticTokensDeltaRequestResult: expected object");
    const QJsonObject obj = val.toObject();
    if (obj.contains("data")) {
        const auto res0 = fromJson<SemanticTokens>(val);
        if (!res0)
            return Utils::ResultError(res0.error());
        return SemanticTokensDeltaRequestResult(*res0);
    }
    if (obj.contains("edits")) {
        const auto res1 = fromJson<SemanticTokensDelta>(val);
        if (!res1)
            return Utils::ResultError(res1.error());
        return SemanticTokensDeltaRequestResult(*res1);
    }
    return Utils::ResultError("Invalid SemanticTokensDeltaRequestResult");
}

QJsonValue toJsonValue(const SemanticTokensDeltaRequestResult &val)
{
    return std::visit([](const auto &v) -> QJsonValue {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, SemanticTokens>) {
            return toJson(v);
        } else
        if constexpr (std::is_same_v<T, SemanticTokensDelta>) {
            return toJson(v);
        } else
        if constexpr (std::is_same_v<T, std::monostate>) {
            return QJsonValue(QJsonValue::Null);
        } else
        {
            return QVariant::fromValue(v).toJsonValue();
        }
    }, val);
}

template<>
Utils::Result<SemanticTokensDeltaRequestPartialResult> fromJson<SemanticTokensDeltaRequestPartialResult>(const QJsonValue &val)
{
    if (!val.isObject())
        return Utils::ResultError("Invalid SemanticTokensDeltaRequestPartialResult: expected object");
    const QJsonObject obj = val.toObject();
    if (obj.contains("data")) {
        const auto res0 = fromJson<SemanticTokensPartialResult>(val);
        if (!res0)
            return Utils::ResultError(res0.error());
        return SemanticTokensDeltaRequestPartialResult(*res0);
    }
    if (obj.contains("edits")) {
        const auto res1 = fromJson<SemanticTokensDeltaPartialResult>(val);
        if (!res1)
            return Utils::ResultError(res1.error());
        return SemanticTokensDeltaRequestPartialResult(*res1);
    }
    return Utils::ResultError("Invalid SemanticTokensDeltaRequestPartialResult");
}

QJsonObject toJson(const SemanticTokensDeltaRequestPartialResult &val)
{
    return std::visit([](const auto &v) -> QJsonObject {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, QJsonObject>) {
            return v;
        } else {
            return toJson(v);
        }
    }, val);
}

QJsonValue toJsonValue(const SemanticTokensDeltaRequestPartialResult &val)
{
    return toJson(val);
}

template<>
Utils::Result<LinkedEditingRangeRequestResult> fromJson<LinkedEditingRangeRequestResult>(const QJsonValue &val)
{
    if (val.isNull())
        return LinkedEditingRangeRequestResult(std::monostate{});
    if (!val.isObject())
        return Utils::ResultError("Invalid LinkedEditingRangeRequestResult: expected object");
    const QJsonObject obj = val.toObject();
    if (obj.contains("ranges")) {
        const auto res0 = fromJson<LinkedEditingRanges>(val);
        if (!res0)
            return Utils::ResultError(res0.error());
        return LinkedEditingRangeRequestResult(*res0);
    }
    return Utils::ResultError("Invalid LinkedEditingRangeRequestResult");
}

QJsonValue toJsonValue(const LinkedEditingRangeRequestResult &val)
{
    return std::visit([](const auto &v) -> QJsonValue {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, LinkedEditingRanges>) {
            return toJson(v);
        } else
        if constexpr (std::is_same_v<T, std::monostate>) {
            return QJsonValue(QJsonValue::Null);
        } else
        {
            return QVariant::fromValue(v).toJsonValue();
        }
    }, val);
}

template<>
Utils::Result<WillCreateFilesRequestResult> fromJson<WillCreateFilesRequestResult>(const QJsonValue &val)
{
    if (val.isNull())
        return WillCreateFilesRequestResult(std::monostate{});
    if (val.isObject()) {
        auto result = fromJson<WorkspaceEdit>(val);
        if (result) return WillCreateFilesRequestResult(*result);
    }
    return Utils::ResultError("Invalid WillCreateFilesRequestResult");
}

QJsonValue toJsonValue(const WillCreateFilesRequestResult &val)
{
    return std::visit([](const auto &v) -> QJsonValue {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, WorkspaceEdit>) {
            return toJson(v);
        } else
        if constexpr (std::is_same_v<T, std::monostate>) {
            return QJsonValue(QJsonValue::Null);
        } else
        {
            return QVariant::fromValue(v).toJsonValue();
        }
    }, val);
}

template<>
Utils::Result<MonikerRequestResult> fromJson<MonikerRequestResult>(const QJsonValue &val)
{
    if (val.isArray()) {
        bool ok = true;
        QList<Moniker> list;
        for (const auto &elem : val.toArray()) {
            auto r = fromJson<Moniker>(elem);
            if (!r) { ok = false; break; }
            list.append(*r);
        }
        if (ok) return MonikerRequestResult(std::move(list));
    }
    if (val.isNull())
        return MonikerRequestResult(std::monostate{});
    return Utils::ResultError("Invalid MonikerRequestResult");
}

QJsonValue toJsonValue(const MonikerRequestResult &val)
{
    return std::visit([](const auto &v) -> QJsonValue {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, std::monostate>) {
            return QJsonValue(QJsonValue::Null);
        } else
        if constexpr (std::is_same_v<T, QList<Moniker>>) {
            QJsonArray arr;
            for (const auto &elem : v)
                arr.append(toJson(elem));
            return arr;
        } else
        {
            return QVariant::fromValue(v).toJsonValue();
        }
    }, val);
}

template<>
Utils::Result<MonikerRequestPartialResult> fromJson<MonikerRequestPartialResult>(const QJsonValue &val)
{
    if (!val.isArray())
        return Utils::ResultError("Expected JSON array for MonikerRequestPartialResult");
    MonikerRequestPartialResult result;
    for (const QJsonValue &v : val.toArray()) {
        const auto res0 = fromJson<Moniker>(v);
        if (!res0)
            return Utils::ResultError(res0.error());
        result.append(*res0);
    }
    return result;
}

QJsonArray toJson(const MonikerRequestPartialResult &data)
{
    QJsonArray arr;
    for (const auto &v : data) arr.append(toJson(v));
    return arr;
}

template<>
Utils::Result<TypeHierarchyPrepareRequestResult> fromJson<TypeHierarchyPrepareRequestResult>(const QJsonValue &val)
{
    if (val.isArray()) {
        bool ok = true;
        QList<TypeHierarchyItem> list;
        for (const auto &elem : val.toArray()) {
            auto r = fromJson<TypeHierarchyItem>(elem);
            if (!r) { ok = false; break; }
            list.append(*r);
        }
        if (ok) return TypeHierarchyPrepareRequestResult(std::move(list));
    }
    if (val.isNull())
        return TypeHierarchyPrepareRequestResult(std::monostate{});
    return Utils::ResultError("Invalid TypeHierarchyPrepareRequestResult");
}

QJsonValue toJsonValue(const TypeHierarchyPrepareRequestResult &val)
{
    return std::visit([](const auto &v) -> QJsonValue {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, std::monostate>) {
            return QJsonValue(QJsonValue::Null);
        } else
        if constexpr (std::is_same_v<T, QList<TypeHierarchyItem>>) {
            QJsonArray arr;
            for (const auto &elem : v)
                arr.append(toJson(elem));
            return arr;
        } else
        {
            return QVariant::fromValue(v).toJsonValue();
        }
    }, val);
}

template<>
Utils::Result<TypeHierarchySupertypesRequestPartialResult> fromJson<TypeHierarchySupertypesRequestPartialResult>(const QJsonValue &val)
{
    if (!val.isArray())
        return Utils::ResultError("Expected JSON array for TypeHierarchySupertypesRequestPartialResult");
    TypeHierarchySupertypesRequestPartialResult result;
    for (const QJsonValue &v : val.toArray()) {
        const auto res0 = fromJson<TypeHierarchyItem>(v);
        if (!res0)
            return Utils::ResultError(res0.error());
        result.append(*res0);
    }
    return result;
}

QJsonArray toJson(const TypeHierarchySupertypesRequestPartialResult &data)
{
    QJsonArray arr;
    for (const auto &v : data) arr.append(toJson(v));
    return arr;
}

template<>
Utils::Result<InlineValueRequestResult> fromJson<InlineValueRequestResult>(const QJsonValue &val)
{
    if (val.isArray()) {
        bool ok = true;
        QList<InlineValue> list;
        for (const auto &elem : val.toArray()) {
            auto r = fromJson<InlineValue>(elem);
            if (!r) { ok = false; break; }
            list.append(*r);
        }
        if (ok) return InlineValueRequestResult(std::move(list));
    }
    if (val.isNull())
        return InlineValueRequestResult(std::monostate{});
    return Utils::ResultError("Invalid InlineValueRequestResult");
}

QJsonValue toJsonValue(const InlineValueRequestResult &val)
{
    return std::visit([](const auto &v) -> QJsonValue {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, std::monostate>) {
            return QJsonValue(QJsonValue::Null);
        } else
        if constexpr (std::is_same_v<T, QList<InlineValue>>) {
            QJsonArray arr;
            for (const auto &elem : v)
                arr.append(toJsonValue(elem));
            return arr;
        } else
        {
            return QVariant::fromValue(v).toJsonValue();
        }
    }, val);
}

template<>
Utils::Result<InlineValueRequestPartialResult> fromJson<InlineValueRequestPartialResult>(const QJsonValue &val)
{
    if (!val.isArray())
        return Utils::ResultError("Expected JSON array for InlineValueRequestPartialResult");
    InlineValueRequestPartialResult result;
    for (const QJsonValue &v : val.toArray()) {
        const auto res0 = fromJson<InlineValue>(v);
        if (!res0)
            return Utils::ResultError(res0.error());
        result.append(*res0);
    }
    return result;
}

QJsonArray toJson(const InlineValueRequestPartialResult &data)
{
    QJsonArray arr;
    for (const auto &v : data) arr.append(toJsonValue(v));
    return arr;
}

template<>
Utils::Result<InlayHintRequestResult> fromJson<InlayHintRequestResult>(const QJsonValue &val)
{
    if (val.isArray()) {
        bool ok = true;
        QList<InlayHint> list;
        for (const auto &elem : val.toArray()) {
            auto r = fromJson<InlayHint>(elem);
            if (!r) { ok = false; break; }
            list.append(*r);
        }
        if (ok) return InlayHintRequestResult(std::move(list));
    }
    if (val.isNull())
        return InlayHintRequestResult(std::monostate{});
    return Utils::ResultError("Invalid InlayHintRequestResult");
}

QJsonValue toJsonValue(const InlayHintRequestResult &val)
{
    return std::visit([](const auto &v) -> QJsonValue {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, std::monostate>) {
            return QJsonValue(QJsonValue::Null);
        } else
        if constexpr (std::is_same_v<T, QList<InlayHint>>) {
            QJsonArray arr;
            for (const auto &elem : v)
                arr.append(toJson(elem));
            return arr;
        } else
        {
            return QVariant::fromValue(v).toJsonValue();
        }
    }, val);
}

template<>
Utils::Result<InlayHintRequestPartialResult> fromJson<InlayHintRequestPartialResult>(const QJsonValue &val)
{
    if (!val.isArray())
        return Utils::ResultError("Expected JSON array for InlayHintRequestPartialResult");
    InlayHintRequestPartialResult result;
    for (const QJsonValue &v : val.toArray()) {
        const auto res0 = fromJson<InlayHint>(v);
        if (!res0)
            return Utils::ResultError(res0.error());
        result.append(*res0);
    }
    return result;
}

QJsonArray toJson(const InlayHintRequestPartialResult &data)
{
    QJsonArray arr;
    for (const auto &v : data) arr.append(toJson(v));
    return arr;
}

template<>
Utils::Result<InlineCompletionRequestResult> fromJson<InlineCompletionRequestResult>(const QJsonValue &val)
{
    if (val.isArray()) {
        bool ok = true;
        QList<InlineCompletionItem> list;
        for (const auto &elem : val.toArray()) {
            auto r = fromJson<InlineCompletionItem>(elem);
            if (!r) { ok = false; break; }
            list.append(*r);
        }
        if (ok) return InlineCompletionRequestResult(std::move(list));
    }
    if (val.isNull())
        return InlineCompletionRequestResult(std::monostate{});
    if (!val.isObject())
        return Utils::ResultError("Invalid InlineCompletionRequestResult: expected object");
    const QJsonObject obj = val.toObject();
    if (obj.contains("items")) {
        const auto res0 = fromJson<InlineCompletionList>(val);
        if (!res0)
            return Utils::ResultError(res0.error());
        return InlineCompletionRequestResult(*res0);
    }
    return Utils::ResultError("Invalid InlineCompletionRequestResult");
}

QJsonValue toJsonValue(const InlineCompletionRequestResult &val)
{
    return std::visit([](const auto &v) -> QJsonValue {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, InlineCompletionList>) {
            return toJson(v);
        } else
        if constexpr (std::is_same_v<T, QList<InlineCompletionItem>>) {
            QJsonArray arr;
            for (const auto &elem : v)
                arr.append(toJson(elem));
            return arr;
        } else
        if constexpr (std::is_same_v<T, std::monostate>) {
            return QJsonValue(QJsonValue::Null);
        } else
        {
            return QVariant::fromValue(v).toJsonValue();
        }
    }, val);
}

template<>
Utils::Result<InlineCompletionRequestPartialResult> fromJson<InlineCompletionRequestPartialResult>(const QJsonValue &val)
{
    if (!val.isArray())
        return Utils::ResultError("Expected JSON array for InlineCompletionRequestPartialResult");
    InlineCompletionRequestPartialResult result;
    for (const QJsonValue &v : val.toArray()) {
        const auto res0 = fromJson<InlineCompletionItem>(v);
        if (!res0)
            return Utils::ResultError(res0.error());
        result.append(*res0);
    }
    return result;
}

QJsonArray toJson(const InlineCompletionRequestPartialResult &data)
{
    QJsonArray arr;
    for (const auto &v : data) arr.append(toJson(v));
    return arr;
}

template<>
Utils::Result<ShowMessageRequestResult> fromJson<ShowMessageRequestResult>(const QJsonValue &val)
{
    if (val.isNull())
        return ShowMessageRequestResult(std::monostate{});
    if (!val.isObject())
        return Utils::ResultError("Invalid ShowMessageRequestResult: expected object");
    const QJsonObject obj = val.toObject();
    if (obj.contains("title")) {
        const auto res0 = fromJson<MessageActionItem>(val);
        if (!res0)
            return Utils::ResultError(res0.error());
        return ShowMessageRequestResult(*res0);
    }
    return Utils::ResultError("Invalid ShowMessageRequestResult");
}

QJsonValue toJsonValue(const ShowMessageRequestResult &val)
{
    return std::visit([](const auto &v) -> QJsonValue {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, MessageActionItem>) {
            return toJson(v);
        } else
        if constexpr (std::is_same_v<T, std::monostate>) {
            return QJsonValue(QJsonValue::Null);
        } else
        {
            return QVariant::fromValue(v).toJsonValue();
        }
    }, val);
}

template<>
Utils::Result<WillSaveTextDocumentWaitUntilRequestResult> fromJson<WillSaveTextDocumentWaitUntilRequestResult>(const QJsonValue &val)
{
    if (val.isArray()) {
        bool ok = true;
        QList<TextEdit> list;
        for (const auto &elem : val.toArray()) {
            auto r = fromJson<TextEdit>(elem);
            if (!r) { ok = false; break; }
            list.append(*r);
        }
        if (ok) return WillSaveTextDocumentWaitUntilRequestResult(std::move(list));
    }
    if (val.isNull())
        return WillSaveTextDocumentWaitUntilRequestResult(std::monostate{});
    return Utils::ResultError("Invalid WillSaveTextDocumentWaitUntilRequestResult");
}

QJsonValue toJsonValue(const WillSaveTextDocumentWaitUntilRequestResult &val)
{
    return std::visit([](const auto &v) -> QJsonValue {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, std::monostate>) {
            return QJsonValue(QJsonValue::Null);
        } else
        if constexpr (std::is_same_v<T, QList<TextEdit>>) {
            QJsonArray arr;
            for (const auto &elem : v)
                arr.append(toJson(elem));
            return arr;
        } else
        {
            return QVariant::fromValue(v).toJsonValue();
        }
    }, val);
}

template<>
Utils::Result<CompletionRequestResult> fromJson<CompletionRequestResult>(const QJsonValue &val)
{
    if (val.isArray()) {
        bool ok = true;
        QList<CompletionItem> list;
        for (const auto &elem : val.toArray()) {
            auto r = fromJson<CompletionItem>(elem);
            if (!r) { ok = false; break; }
            list.append(*r);
        }
        if (ok) return CompletionRequestResult(std::move(list));
    }
    if (val.isNull())
        return CompletionRequestResult(std::monostate{});
    if (!val.isObject())
        return Utils::ResultError("Invalid CompletionRequestResult: expected object");
    const QJsonObject obj = val.toObject();
    if (obj.contains("isIncomplete")) {
        const auto res0 = fromJson<CompletionList>(val);
        if (!res0)
            return Utils::ResultError(res0.error());
        return CompletionRequestResult(*res0);
    }
    return Utils::ResultError("Invalid CompletionRequestResult");
}

QJsonValue toJsonValue(const CompletionRequestResult &val)
{
    return std::visit([](const auto &v) -> QJsonValue {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, QList<CompletionItem>>) {
            QJsonArray arr;
            for (const auto &elem : v)
                arr.append(toJson(elem));
            return arr;
        } else
        if constexpr (std::is_same_v<T, CompletionList>) {
            return toJson(v);
        } else
        if constexpr (std::is_same_v<T, std::monostate>) {
            return QJsonValue(QJsonValue::Null);
        } else
        {
            return QVariant::fromValue(v).toJsonValue();
        }
    }, val);
}

template<>
Utils::Result<CompletionRequestPartialResult> fromJson<CompletionRequestPartialResult>(const QJsonValue &val)
{
    if (!val.isArray())
        return Utils::ResultError("Expected JSON array for CompletionRequestPartialResult");
    CompletionRequestPartialResult result;
    for (const QJsonValue &v : val.toArray()) {
        const auto res0 = fromJson<CompletionItem>(v);
        if (!res0)
            return Utils::ResultError(res0.error());
        result.append(*res0);
    }
    return result;
}

QJsonArray toJson(const CompletionRequestPartialResult &data)
{
    QJsonArray arr;
    for (const auto &v : data) arr.append(toJson(v));
    return arr;
}

template<>
Utils::Result<HoverRequestResult> fromJson<HoverRequestResult>(const QJsonValue &val)
{
    if (val.isNull())
        return HoverRequestResult(std::monostate{});
    if (!val.isObject())
        return Utils::ResultError("Invalid HoverRequestResult: expected object");
    const QJsonObject obj = val.toObject();
    if (obj.contains("contents")) {
        const auto res0 = fromJson<Hover>(val);
        if (!res0)
            return Utils::ResultError(res0.error());
        return HoverRequestResult(*res0);
    }
    return Utils::ResultError("Invalid HoverRequestResult");
}

QJsonValue toJsonValue(const HoverRequestResult &val)
{
    return std::visit([](const auto &v) -> QJsonValue {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, Hover>) {
            return toJson(v);
        } else
        if constexpr (std::is_same_v<T, std::monostate>) {
            return QJsonValue(QJsonValue::Null);
        } else
        {
            return QVariant::fromValue(v).toJsonValue();
        }
    }, val);
}

template<>
Utils::Result<SignatureHelpRequestResult> fromJson<SignatureHelpRequestResult>(const QJsonValue &val)
{
    if (val.isNull())
        return SignatureHelpRequestResult(std::monostate{});
    if (!val.isObject())
        return Utils::ResultError("Invalid SignatureHelpRequestResult: expected object");
    const QJsonObject obj = val.toObject();
    if (obj.contains("signatures")) {
        const auto res0 = fromJson<SignatureHelp>(val);
        if (!res0)
            return Utils::ResultError(res0.error());
        return SignatureHelpRequestResult(*res0);
    }
    return Utils::ResultError("Invalid SignatureHelpRequestResult");
}

QJsonValue toJsonValue(const SignatureHelpRequestResult &val)
{
    return std::visit([](const auto &v) -> QJsonValue {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, SignatureHelp>) {
            return toJson(v);
        } else
        if constexpr (std::is_same_v<T, std::monostate>) {
            return QJsonValue(QJsonValue::Null);
        } else
        {
            return QVariant::fromValue(v).toJsonValue();
        }
    }, val);
}

template<>
Utils::Result<ReferencesRequestResult> fromJson<ReferencesRequestResult>(const QJsonValue &val)
{
    if (val.isArray()) {
        bool ok = true;
        QList<Location> list;
        for (const auto &elem : val.toArray()) {
            auto r = fromJson<Location>(elem);
            if (!r) { ok = false; break; }
            list.append(*r);
        }
        if (ok) return ReferencesRequestResult(std::move(list));
    }
    if (val.isNull())
        return ReferencesRequestResult(std::monostate{});
    return Utils::ResultError("Invalid ReferencesRequestResult");
}

QJsonValue toJsonValue(const ReferencesRequestResult &val)
{
    return std::visit([](const auto &v) -> QJsonValue {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, std::monostate>) {
            return QJsonValue(QJsonValue::Null);
        } else
        if constexpr (std::is_same_v<T, QList<Location>>) {
            QJsonArray arr;
            for (const auto &elem : v)
                arr.append(toJson(elem));
            return arr;
        } else
        {
            return QVariant::fromValue(v).toJsonValue();
        }
    }, val);
}

template<>
Utils::Result<ReferencesRequestPartialResult> fromJson<ReferencesRequestPartialResult>(const QJsonValue &val)
{
    if (!val.isArray())
        return Utils::ResultError("Expected JSON array for ReferencesRequestPartialResult");
    ReferencesRequestPartialResult result;
    for (const QJsonValue &v : val.toArray()) {
        const auto res0 = fromJson<Location>(v);
        if (!res0)
            return Utils::ResultError(res0.error());
        result.append(*res0);
    }
    return result;
}

QJsonArray toJson(const ReferencesRequestPartialResult &data)
{
    QJsonArray arr;
    for (const auto &v : data) arr.append(toJson(v));
    return arr;
}

template<>
Utils::Result<DocumentHighlightRequestResult> fromJson<DocumentHighlightRequestResult>(const QJsonValue &val)
{
    if (val.isArray()) {
        bool ok = true;
        QList<DocumentHighlight> list;
        for (const auto &elem : val.toArray()) {
            auto r = fromJson<DocumentHighlight>(elem);
            if (!r) { ok = false; break; }
            list.append(*r);
        }
        if (ok) return DocumentHighlightRequestResult(std::move(list));
    }
    if (val.isNull())
        return DocumentHighlightRequestResult(std::monostate{});
    return Utils::ResultError("Invalid DocumentHighlightRequestResult");
}

QJsonValue toJsonValue(const DocumentHighlightRequestResult &val)
{
    return std::visit([](const auto &v) -> QJsonValue {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, std::monostate>) {
            return QJsonValue(QJsonValue::Null);
        } else
        if constexpr (std::is_same_v<T, QList<DocumentHighlight>>) {
            QJsonArray arr;
            for (const auto &elem : v)
                arr.append(toJson(elem));
            return arr;
        } else
        {
            return QVariant::fromValue(v).toJsonValue();
        }
    }, val);
}

template<>
Utils::Result<DocumentHighlightRequestPartialResult> fromJson<DocumentHighlightRequestPartialResult>(const QJsonValue &val)
{
    if (!val.isArray())
        return Utils::ResultError("Expected JSON array for DocumentHighlightRequestPartialResult");
    DocumentHighlightRequestPartialResult result;
    for (const QJsonValue &v : val.toArray()) {
        const auto res0 = fromJson<DocumentHighlight>(v);
        if (!res0)
            return Utils::ResultError(res0.error());
        result.append(*res0);
    }
    return result;
}

QJsonArray toJson(const DocumentHighlightRequestPartialResult &data)
{
    QJsonArray arr;
    for (const auto &v : data) arr.append(toJson(v));
    return arr;
}

template<>
Utils::Result<DocumentSymbolRequestResult> fromJson<DocumentSymbolRequestResult>(const QJsonValue &val)
{
    if (val.isArray()) {
        bool ok = true;
        QList<SymbolInformation> list;
        for (const auto &elem : val.toArray()) {
            auto r = fromJson<SymbolInformation>(elem);
            if (!r) { ok = false; break; }
            list.append(*r);
        }
        if (ok) return DocumentSymbolRequestResult(std::move(list));
    }
    if (val.isArray()) {
        bool ok = true;
        QList<DocumentSymbol> list;
        for (const auto &elem : val.toArray()) {
            auto r = fromJson<DocumentSymbol>(elem);
            if (!r) { ok = false; break; }
            list.append(*r);
        }
        if (ok) return DocumentSymbolRequestResult(std::move(list));
    }
    if (val.isNull())
        return DocumentSymbolRequestResult(std::monostate{});
    return Utils::ResultError("Invalid DocumentSymbolRequestResult");
}

QJsonValue toJsonValue(const DocumentSymbolRequestResult &val)
{
    return std::visit([](const auto &v) -> QJsonValue {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, std::monostate>) {
            return QJsonValue(QJsonValue::Null);
        } else
        if constexpr (std::is_same_v<T, QList<SymbolInformation>>) {
            QJsonArray arr;
            for (const auto &elem : v)
                arr.append(toJson(elem));
            return arr;
        } else
        if constexpr (std::is_same_v<T, QList<DocumentSymbol>>) {
            QJsonArray arr;
            for (const auto &elem : v)
                arr.append(toJson(elem));
            return arr;
        } else
        {
            return QVariant::fromValue(v).toJsonValue();
        }
    }, val);
}

template<>
Utils::Result<DocumentSymbolRequestPartialResult> fromJson<DocumentSymbolRequestPartialResult>(const QJsonValue &val)
{
    if (val.isArray()) {
        bool ok = true;
        QList<SymbolInformation> list;
        for (const auto &elem : val.toArray()) {
            auto r = fromJson<SymbolInformation>(elem);
            if (!r) { ok = false; break; }
            list.append(*r);
        }
        if (ok) return DocumentSymbolRequestPartialResult(std::move(list));
    }
    if (val.isArray()) {
        bool ok = true;
        QList<DocumentSymbol> list;
        for (const auto &elem : val.toArray()) {
            auto r = fromJson<DocumentSymbol>(elem);
            if (!r) { ok = false; break; }
            list.append(*r);
        }
        if (ok) return DocumentSymbolRequestPartialResult(std::move(list));
    }
    return Utils::ResultError("Invalid DocumentSymbolRequestPartialResult");
}

QJsonValue toJsonValue(const DocumentSymbolRequestPartialResult &val)
{
    return std::visit([](const auto &v) -> QJsonValue {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, QList<SymbolInformation>>) {
            QJsonArray arr;
            for (const auto &elem : v)
                arr.append(toJson(elem));
            return arr;
        } else
        if constexpr (std::is_same_v<T, QList<DocumentSymbol>>) {
            QJsonArray arr;
            for (const auto &elem : v)
                arr.append(toJson(elem));
            return arr;
        } else
        {
            return QVariant::fromValue(v).toJsonValue();
        }
    }, val);
}

template<>
Utils::Result<CommandOrCodeAction> fromJson<CommandOrCodeAction>(const QJsonValue &val)
{
    {
        auto result = fromJson<Command>(val);
        if (result) return CommandOrCodeAction(*result);
    }
    {
        auto result = fromJson<CodeAction>(val);
        if (result) return CommandOrCodeAction(*result);
    }
    return Utils::ResultError("Invalid CommandOrCodeAction");
}

QJsonValue toJsonValue(const CommandOrCodeAction &val)
{
    return std::visit([](const auto &v) -> QJsonValue {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, QJsonObject>) {
            return v;
        } else {
            return toJson(v);
        }
    }, val);
}

template<>
Utils::Result<CodeActionRequestResult> fromJson<CodeActionRequestResult>(const QJsonValue &val)
{
    if (val.isArray()) {
        bool ok = true;
        QList<CommandOrCodeAction> list;
        for (const auto &elem : val.toArray()) {
            auto r = fromJson<CommandOrCodeAction>(elem);
            if (!r) { ok = false; break; }
            list.append(*r);
        }
        if (ok) return CodeActionRequestResult(std::move(list));
    }
    if (val.isNull())
        return CodeActionRequestResult(std::monostate{});
    return Utils::ResultError("Invalid CodeActionRequestResult");
}

QJsonValue toJsonValue(const CodeActionRequestResult &val)
{
    return std::visit([](const auto &v) -> QJsonValue {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, std::monostate>) {
            return QJsonValue(QJsonValue::Null);
        } else
        if constexpr (std::is_same_v<T, QList<CommandOrCodeAction>>) {
            QJsonArray arr;
            for (const auto &elem : v)
                arr.append(toJsonValue(elem));
            return arr;
        } else
        {
            return QVariant::fromValue(v).toJsonValue();
        }
    }, val);
}

template<>
Utils::Result<WorkspaceSymbolRequestResult> fromJson<WorkspaceSymbolRequestResult>(const QJsonValue &val)
{
    if (val.isArray()) {
        bool ok = true;
        QList<SymbolInformation> list;
        for (const auto &elem : val.toArray()) {
            auto r = fromJson<SymbolInformation>(elem);
            if (!r) { ok = false; break; }
            list.append(*r);
        }
        if (ok) return WorkspaceSymbolRequestResult(std::move(list));
    }
    if (val.isArray()) {
        bool ok = true;
        QList<WorkspaceSymbol> list;
        for (const auto &elem : val.toArray()) {
            auto r = fromJson<WorkspaceSymbol>(elem);
            if (!r) { ok = false; break; }
            list.append(*r);
        }
        if (ok) return WorkspaceSymbolRequestResult(std::move(list));
    }
    if (val.isNull())
        return WorkspaceSymbolRequestResult(std::monostate{});
    return Utils::ResultError("Invalid WorkspaceSymbolRequestResult");
}

QJsonValue toJsonValue(const WorkspaceSymbolRequestResult &val)
{
    return std::visit([](const auto &v) -> QJsonValue {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, std::monostate>) {
            return QJsonValue(QJsonValue::Null);
        } else
        if constexpr (std::is_same_v<T, QList<SymbolInformation>>) {
            QJsonArray arr;
            for (const auto &elem : v)
                arr.append(toJson(elem));
            return arr;
        } else
        if constexpr (std::is_same_v<T, QList<WorkspaceSymbol>>) {
            QJsonArray arr;
            for (const auto &elem : v)
                arr.append(toJson(elem));
            return arr;
        } else
        {
            return QVariant::fromValue(v).toJsonValue();
        }
    }, val);
}

template<>
Utils::Result<WorkspaceSymbolRequestPartialResult> fromJson<WorkspaceSymbolRequestPartialResult>(const QJsonValue &val)
{
    if (val.isArray()) {
        bool ok = true;
        QList<SymbolInformation> list;
        for (const auto &elem : val.toArray()) {
            auto r = fromJson<SymbolInformation>(elem);
            if (!r) { ok = false; break; }
            list.append(*r);
        }
        if (ok) return WorkspaceSymbolRequestPartialResult(std::move(list));
    }
    if (val.isArray()) {
        bool ok = true;
        QList<WorkspaceSymbol> list;
        for (const auto &elem : val.toArray()) {
            auto r = fromJson<WorkspaceSymbol>(elem);
            if (!r) { ok = false; break; }
            list.append(*r);
        }
        if (ok) return WorkspaceSymbolRequestPartialResult(std::move(list));
    }
    return Utils::ResultError("Invalid WorkspaceSymbolRequestPartialResult");
}

QJsonValue toJsonValue(const WorkspaceSymbolRequestPartialResult &val)
{
    return std::visit([](const auto &v) -> QJsonValue {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, QList<SymbolInformation>>) {
            QJsonArray arr;
            for (const auto &elem : v)
                arr.append(toJson(elem));
            return arr;
        } else
        if constexpr (std::is_same_v<T, QList<WorkspaceSymbol>>) {
            QJsonArray arr;
            for (const auto &elem : v)
                arr.append(toJson(elem));
            return arr;
        } else
        {
            return QVariant::fromValue(v).toJsonValue();
        }
    }, val);
}

template<>
Utils::Result<CodeLensRequestResult> fromJson<CodeLensRequestResult>(const QJsonValue &val)
{
    if (val.isArray()) {
        bool ok = true;
        QList<CodeLens> list;
        for (const auto &elem : val.toArray()) {
            auto r = fromJson<CodeLens>(elem);
            if (!r) { ok = false; break; }
            list.append(*r);
        }
        if (ok) return CodeLensRequestResult(std::move(list));
    }
    if (val.isNull())
        return CodeLensRequestResult(std::monostate{});
    return Utils::ResultError("Invalid CodeLensRequestResult");
}

QJsonValue toJsonValue(const CodeLensRequestResult &val)
{
    return std::visit([](const auto &v) -> QJsonValue {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, std::monostate>) {
            return QJsonValue(QJsonValue::Null);
        } else
        if constexpr (std::is_same_v<T, QList<CodeLens>>) {
            QJsonArray arr;
            for (const auto &elem : v)
                arr.append(toJson(elem));
            return arr;
        } else
        {
            return QVariant::fromValue(v).toJsonValue();
        }
    }, val);
}

template<>
Utils::Result<CodeLensRequestPartialResult> fromJson<CodeLensRequestPartialResult>(const QJsonValue &val)
{
    if (!val.isArray())
        return Utils::ResultError("Expected JSON array for CodeLensRequestPartialResult");
    CodeLensRequestPartialResult result;
    for (const QJsonValue &v : val.toArray()) {
        const auto res0 = fromJson<CodeLens>(v);
        if (!res0)
            return Utils::ResultError(res0.error());
        result.append(*res0);
    }
    return result;
}

QJsonArray toJson(const CodeLensRequestPartialResult &data)
{
    QJsonArray arr;
    for (const auto &v : data) arr.append(toJson(v));
    return arr;
}

template<>
Utils::Result<DocumentLinkRequestResult> fromJson<DocumentLinkRequestResult>(const QJsonValue &val)
{
    if (val.isArray()) {
        bool ok = true;
        QList<DocumentLink> list;
        for (const auto &elem : val.toArray()) {
            auto r = fromJson<DocumentLink>(elem);
            if (!r) { ok = false; break; }
            list.append(*r);
        }
        if (ok) return DocumentLinkRequestResult(std::move(list));
    }
    if (val.isNull())
        return DocumentLinkRequestResult(std::monostate{});
    return Utils::ResultError("Invalid DocumentLinkRequestResult");
}

QJsonValue toJsonValue(const DocumentLinkRequestResult &val)
{
    return std::visit([](const auto &v) -> QJsonValue {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, std::monostate>) {
            return QJsonValue(QJsonValue::Null);
        } else
        if constexpr (std::is_same_v<T, QList<DocumentLink>>) {
            QJsonArray arr;
            for (const auto &elem : v)
                arr.append(toJson(elem));
            return arr;
        } else
        {
            return QVariant::fromValue(v).toJsonValue();
        }
    }, val);
}

template<>
Utils::Result<DocumentLinkRequestPartialResult> fromJson<DocumentLinkRequestPartialResult>(const QJsonValue &val)
{
    if (!val.isArray())
        return Utils::ResultError("Expected JSON array for DocumentLinkRequestPartialResult");
    DocumentLinkRequestPartialResult result;
    for (const QJsonValue &v : val.toArray()) {
        const auto res0 = fromJson<DocumentLink>(v);
        if (!res0)
            return Utils::ResultError(res0.error());
        result.append(*res0);
    }
    return result;
}

QJsonArray toJson(const DocumentLinkRequestPartialResult &data)
{
    QJsonArray arr;
    for (const auto &v : data) arr.append(toJson(v));
    return arr;
}

template<>
Utils::Result<PrepareRenameRequestResult> fromJson<PrepareRenameRequestResult>(const QJsonValue &val)
{
    if (val.isNull())
        return PrepareRenameRequestResult(std::monostate{});
    if (!val.isObject()) {
        {
            auto result = fromJson<PrepareRenameResult>(val);
            if (result) return PrepareRenameRequestResult(*result);
        }
    }
    if (val.isObject()) {
        auto result = fromJson<PrepareRenameResult>(val);
        if (result) return PrepareRenameRequestResult(*result);
    }
    return Utils::ResultError("Invalid PrepareRenameRequestResult");
}

QJsonValue toJsonValue(const PrepareRenameRequestResult &val)
{
    return std::visit([](const auto &v) -> QJsonValue {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, PrepareRenameResult>) {
            return toJsonValue(v);
        } else
        if constexpr (std::is_same_v<T, std::monostate>) {
            return QJsonValue(QJsonValue::Null);
        } else
        {
            return QVariant::fromValue(v).toJsonValue();
        }
    }, val);
}

template<>
Utils::Result<ExecuteCommandRequestResult> fromJson<ExecuteCommandRequestResult>(const QJsonValue &val)
{
    if (val.isNull())
        return ExecuteCommandRequestResult(std::monostate{});
    return Utils::ResultError("Invalid ExecuteCommandRequestResult");
}

QJsonValue toJsonValue(const ExecuteCommandRequestResult &val)
{
    return std::visit([](const auto &v) -> QJsonValue {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, std::monostate>) {
            return QJsonValue(QJsonValue::Null);
        } else
        {
            return QVariant::fromValue(v).toJsonValue();
        }
    }, val);
}

} // namespace LanguageServerProtocol
