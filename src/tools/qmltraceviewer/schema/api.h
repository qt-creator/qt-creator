/*
 This file is auto-generated. Do not edit manually.
 Generated with:

 /usr/bin/python3 \
  scripts/generate_cpp_from_schema.py \
  src/tools/qmltraceviewer/schema/qmltraceviewerapi.json.schema src/tools/qmltraceviewer/schema/api.h --namespace QmlTraceViewer::Api::Schema
*/
#pragma once

#include <utils/result.h>
#include <utils/co_result.h>

#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <QMap>
#include <QSet>
#include <QString>
#include <QVariant>

#include <variant>

namespace QmlTraceViewer::Api::Schema {

template<typename T> Utils::Result<T> fromJson(const QJsonValue &val) = delete;

/** A uniquely identifying ID for a request in JSON-RPC. */
using RequestId = std::variant<QString, int>;

template<>
inline Utils::Result<RequestId> fromJson<RequestId>(const QJsonValue &val) {
    if (val.isString()) {
        return RequestId(val.toString());
    }
    if (val.isDouble()) {
        return RequestId(val.toInt());
    }
    return Utils::ResultError("Invalid RequestId");
}

inline QJsonValue toJsonValue(const RequestId &val) {
    return std::visit([](const auto &v) -> QJsonValue {
        {
            return QVariant::fromValue(v).toJsonValue();
        }
    }, val);
}
/**
 * Predefined error codes for common JSON-RPC and ACP-specific errors.
 *
 * These codes follow the JSON-RPC 2.0 specification for standard errors
 * and use the reserved range (-32000 to -32099) for protocol-specific errors.
 */
namespace ErrorCode {
    constexpr int Parse_error = -32700;
    constexpr int Invalid_request = -32600;
    constexpr int Method_not_found = -32601;
    constexpr int Invalid_params = -32602;
    constexpr int Internal_error = -32603;
    constexpr int Authentication_required = -32000;
    constexpr int Resource_not_found = -32002;
} // namespace ErrorCode
struct Error {
    int _code;  //!< The error type that occurred.
    std::optional<QString> _data;  //!< Additional information about the error. The value of this member is defined by the sender (e.g. detailed error information, nested errors etc.).
    QString _message;  //!< A short description of the error. The message SHOULD be limited to a concise single sentence.

    Error& code(int v) { _code = v; return *this; }
    Error& data(const std::optional<QString> & v) { _data = v; return *this; }
    Error& message(const QString & v) { _message = v; return *this; }

    const int& code() const { return _code; }
    const std::optional<QString>& data() const { return _data; }
    const QString& message() const { return _message; }
};

template<>
inline Utils::Result<Error> fromJson<Error>(const QJsonValue &val) {
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for Error");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("code"))
        return Utils::ResultError("Missing required field: code");
    if (!obj.contains("message"))
        return Utils::ResultError("Missing required field: message");
    Error result;
    result._code = obj.value("code").toInt();
    if (obj.contains("data"))
        result._data = obj.value("data").toString();
    result._message = obj.value("message").toString();
    return result;
}

inline QJsonObject toJson(const Error &data) {
    QJsonObject obj{
        {"code", data._code},
        {"message", data._message}
    };
    if (data._data.has_value())
        obj.insert("data", *data._data);
    return obj;
}

/** A response to a request that indicates an error occurred. */
struct ErrorResponse {
    Error _error;
    RequestId _id;

    ErrorResponse& error(const Error & v) { _error = v; return *this; }
    ErrorResponse& id(const RequestId & v) { _id = v; return *this; }

    const Error& error() const { return _error; }
    const RequestId& id() const { return _id; }
};

template<>
inline Utils::Result<ErrorResponse> fromJson<ErrorResponse>(const QJsonValue &val) {
    if (!val.isObject())
        co_return Utils::ResultError("Expected JSON object for ErrorResponse");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("error"))
        co_return Utils::ResultError("Missing required field: error");
    if (!obj.contains("jsonrpc"))
        co_return Utils::ResultError("Missing required field: jsonrpc");
    if (!obj.contains("id"))
        co_return Utils::ResultError("Missing required field: id");
    ErrorResponse result;
    if (obj.contains("error") && obj["error"].isObject())
        result._error = co_await fromJson<Error>(obj["error"]);
    if (obj.contains("id"))
        result._id = co_await fromJson<RequestId>(obj["id"]);
    if (obj.value("jsonrpc").toString() != "2.0")
        co_return Utils::ResultError("Field 'jsonrpc' must be '2.0', got: " + obj.value("jsonrpc").toString());
    co_return result;
}

inline QJsonObject toJson(const ErrorResponse &data) {
    QJsonObject obj{
        {"error", toJson(data._error)},
        {"id", toJsonValue(data._id)},
        {"jsonrpc", QString("2.0")}
    };
    return obj;
}

/** After receiving an openTraceFile request, the application sends this response. */
struct OpenTraceFileResult {
    bool _result;  //!< Indicates whether the trace file was successfully opened.
    RequestId _id;

    OpenTraceFileResult& result(bool v) { _result = v; return *this; }
    OpenTraceFileResult& id(const RequestId & v) { _id = v; return *this; }

    const bool& result() const { return _result; }
    const RequestId& id() const { return _id; }
};

template<>
inline Utils::Result<OpenTraceFileResult> fromJson<OpenTraceFileResult>(const QJsonValue &val) {
    if (!val.isObject())
        co_return Utils::ResultError("Expected JSON object for OpenTraceFileResult");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("id"))
        co_return Utils::ResultError("Missing required field: id");
    if (!obj.contains("jsonrpc"))
        co_return Utils::ResultError("Missing required field: jsonrpc");
    if (!obj.contains("result"))
        co_return Utils::ResultError("Missing required field: result");
    OpenTraceFileResult result;
    result._result = obj.value("result").toBool();
    if (obj.contains("id"))
        result._id = co_await fromJson<RequestId>(obj["id"]);
    if (obj.value("jsonrpc").toString() != "2.0")
        co_return Utils::ResultError("Field 'jsonrpc' must be '2.0', got: " + obj.value("jsonrpc").toString());
    co_return result;
}

inline QJsonObject toJson(const OpenTraceFileResult &data) {
    QJsonObject obj{
        {"result", data._result},
        {"id", toJsonValue(data._id)},
        {"jsonrpc", QString("2.0")}
    };
    return obj;
}

/** The result of a successful JSON-RPC request. */
using ApplicationResult = std::variant<OpenTraceFileResult, ErrorResponse>;

template<>
inline Utils::Result<ApplicationResult> fromJson<ApplicationResult>(const QJsonValue &val) {
    if (!val.isObject())
        co_return Utils::ResultError("Invalid ApplicationResult: expected object");
    const QJsonObject obj = val.toObject();
    if (obj.contains("result"))
        co_return ApplicationResult(co_await fromJson<OpenTraceFileResult>(val));
    if (obj.contains("error"))
        co_return ApplicationResult(co_await fromJson<ErrorResponse>(val));
    co_return Utils::ResultError("Invalid ApplicationResult");
}

/** Returns the 'id' field from the active variant. */
inline RequestId id(const ApplicationResult &val) {
    return std::visit([](const auto &v) -> RequestId { return v._id; }, val);
}

inline QJsonObject toJson(const ApplicationResult &val) {
    return std::visit([](const auto &v) -> QJsonObject {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, QJsonObject>) {
            return v;
        } else {
            return toJson(v);
        }
    }, val);
}

inline QJsonValue toJsonValue(const ApplicationResult &val) {
    return toJson(val);
}
struct TraceDiscardedNotification {

};

template<>
inline Utils::Result<TraceDiscardedNotification> fromJson<TraceDiscardedNotification>(const QJsonValue &val) {
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for TraceDiscardedNotification");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("jsonrpc"))
        return Utils::ResultError("Missing required field: jsonrpc");
    if (!obj.contains("method"))
        return Utils::ResultError("Missing required field: method");
    TraceDiscardedNotification result;
    if (obj.value("jsonrpc").toString() != "2.0")
        return Utils::ResultError("Field 'jsonrpc' must be '2.0', got: " + obj.value("jsonrpc").toString());
    if (obj.value("method").toString() != "traceDiscarded")
        return Utils::ResultError("Field 'method' must be 'traceDiscarded', got: " + obj.value("method").toString());
    return result;
}

inline QJsonObject toJson(const TraceDiscardedNotification &data) {
    Q_UNUSED(data)
    QJsonObject obj{
        {"jsonrpc", QString("2.0")},
        {"method", QString("traceDiscarded")}
    };
    return obj;
}

struct TraceEventSelectedNotification {
    struct Params {
        QString _sourceFilePath;  //!< The file path of the source file to show.
        int _lineNumber;  //!< The line number in the source file to show (1-based index).
        int _columnNumber;  //!< The column number in the source file to show (1-based index).

        Params& sourceFilePath(const QString & v) { _sourceFilePath = v; return *this; }
        Params& lineNumber(int v) { _lineNumber = v; return *this; }
        Params& columnNumber(int v) { _columnNumber = v; return *this; }

        const QString& sourceFilePath() const { return _sourceFilePath; }
        const int& lineNumber() const { return _lineNumber; }
        const int& columnNumber() const { return _columnNumber; }
    };

    Params _params;

    TraceEventSelectedNotification& params(const Params & v) { _params = v; return *this; }

    const Params& params() const { return _params; }
};

template<>
inline Utils::Result<TraceEventSelectedNotification::Params> fromJson<TraceEventSelectedNotification::Params>(const QJsonValue &val) {
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for Params");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("sourceFilePath"))
        return Utils::ResultError("Missing required field: sourceFilePath");
    if (!obj.contains("lineNumber"))
        return Utils::ResultError("Missing required field: lineNumber");
    if (!obj.contains("columnNumber"))
        return Utils::ResultError("Missing required field: columnNumber");
    TraceEventSelectedNotification::Params result;
    result._sourceFilePath = obj.value("sourceFilePath").toString();
    result._lineNumber = obj.value("lineNumber").toInt();
    result._columnNumber = obj.value("columnNumber").toInt();
    return result;
}

inline QJsonObject toJson(const TraceEventSelectedNotification::Params &data) {
    QJsonObject obj{
        {"sourceFilePath", data._sourceFilePath},
        {"lineNumber", data._lineNumber},
        {"columnNumber", data._columnNumber}
    };
    return obj;
}

template<>
inline Utils::Result<TraceEventSelectedNotification> fromJson<TraceEventSelectedNotification>(const QJsonValue &val) {
    if (!val.isObject())
        co_return Utils::ResultError("Expected JSON object for TraceEventSelectedNotification");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("jsonrpc"))
        co_return Utils::ResultError("Missing required field: jsonrpc");
    if (!obj.contains("method"))
        co_return Utils::ResultError("Missing required field: method");
    if (!obj.contains("params"))
        co_return Utils::ResultError("Missing required field: params");
    TraceEventSelectedNotification result;
    if (obj.value("jsonrpc").toString() != "2.0")
        co_return Utils::ResultError("Field 'jsonrpc' must be '2.0', got: " + obj.value("jsonrpc").toString());
    if (obj.value("method").toString() != "traceEventSelected")
        co_return Utils::ResultError("Field 'method' must be 'traceEventSelected', got: " + obj.value("method").toString());
    if (obj.contains("params") && obj["params"].isObject())
        result._params = co_await fromJson<TraceEventSelectedNotification::Params>(obj["params"]);
    co_return result;
}

inline QJsonObject toJson(const TraceEventSelectedNotification &data) {
    QJsonObject obj{
        {"jsonrpc", QString("2.0")},
        {"method", QString("traceEventSelected")},
        {"params", toJson(data._params)}
    };
    return obj;
}

struct TraceFileLoadingFinishedNotification {
    struct Params {
        QString _traceFilePath;  //!< The file path of the trace file.
        bool _successful;  //!< The loading of the file was successful.
        std::optional<QString> _errorMessage;  //!< Error message in case of failure.

        Params& traceFilePath(const QString & v) { _traceFilePath = v; return *this; }
        Params& successful(bool v) { _successful = v; return *this; }
        Params& errorMessage(const std::optional<QString> & v) { _errorMessage = v; return *this; }

        const QString& traceFilePath() const { return _traceFilePath; }
        const bool& successful() const { return _successful; }
        const std::optional<QString>& errorMessage() const { return _errorMessage; }
    };

    Params _params;

    TraceFileLoadingFinishedNotification& params(const Params & v) { _params = v; return *this; }

    const Params& params() const { return _params; }
};

template<>
inline Utils::Result<TraceFileLoadingFinishedNotification::Params> fromJson<TraceFileLoadingFinishedNotification::Params>(const QJsonValue &val) {
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for Params");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("traceFilePath"))
        return Utils::ResultError("Missing required field: traceFilePath");
    if (!obj.contains("successful"))
        return Utils::ResultError("Missing required field: successful");
    TraceFileLoadingFinishedNotification::Params result;
    result._traceFilePath = obj.value("traceFilePath").toString();
    result._successful = obj.value("successful").toBool();
    if (obj.contains("errorMessage"))
        result._errorMessage = obj.value("errorMessage").toString();
    return result;
}

inline QJsonObject toJson(const TraceFileLoadingFinishedNotification::Params &data) {
    QJsonObject obj{
        {"traceFilePath", data._traceFilePath},
        {"successful", data._successful}
    };
    if (data._errorMessage.has_value())
        obj.insert("errorMessage", *data._errorMessage);
    return obj;
}

template<>
inline Utils::Result<TraceFileLoadingFinishedNotification> fromJson<TraceFileLoadingFinishedNotification>(const QJsonValue &val) {
    if (!val.isObject())
        co_return Utils::ResultError("Expected JSON object for TraceFileLoadingFinishedNotification");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("jsonrpc"))
        co_return Utils::ResultError("Missing required field: jsonrpc");
    if (!obj.contains("method"))
        co_return Utils::ResultError("Missing required field: method");
    if (!obj.contains("params"))
        co_return Utils::ResultError("Missing required field: params");
    TraceFileLoadingFinishedNotification result;
    if (obj.value("jsonrpc").toString() != "2.0")
        co_return Utils::ResultError("Field 'jsonrpc' must be '2.0', got: " + obj.value("jsonrpc").toString());
    if (obj.value("method").toString() != "traceFileLoadingFinished")
        co_return Utils::ResultError("Field 'method' must be 'traceFileLoadingFinished', got: " + obj.value("method").toString());
    if (obj.contains("params") && obj["params"].isObject())
        result._params = co_await fromJson<TraceFileLoadingFinishedNotification::Params>(obj["params"]);
    co_return result;
}

inline QJsonObject toJson(const TraceFileLoadingFinishedNotification &data) {
    QJsonObject obj{
        {"jsonrpc", QString("2.0")},
        {"method", QString("traceFileLoadingFinished")},
        {"params", toJson(data._params)}
    };
    return obj;
}

struct TraceFileLoadingStartedNotification {
    struct Params {
        QString _traceFilePath;  //!< The file path of the trace file.

        Params& traceFilePath(const QString & v) { _traceFilePath = v; return *this; }

        const QString& traceFilePath() const { return _traceFilePath; }
    };

    Params _params;

    TraceFileLoadingStartedNotification& params(const Params & v) { _params = v; return *this; }

    const Params& params() const { return _params; }
};

template<>
inline Utils::Result<TraceFileLoadingStartedNotification::Params> fromJson<TraceFileLoadingStartedNotification::Params>(const QJsonValue &val) {
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for Params");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("traceFilePath"))
        return Utils::ResultError("Missing required field: traceFilePath");
    TraceFileLoadingStartedNotification::Params result;
    result._traceFilePath = obj.value("traceFilePath").toString();
    return result;
}

inline QJsonObject toJson(const TraceFileLoadingStartedNotification::Params &data) {
    QJsonObject obj{{"traceFilePath", data._traceFilePath}};
    return obj;
}

template<>
inline Utils::Result<TraceFileLoadingStartedNotification> fromJson<TraceFileLoadingStartedNotification>(const QJsonValue &val) {
    if (!val.isObject())
        co_return Utils::ResultError("Expected JSON object for TraceFileLoadingStartedNotification");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("jsonrpc"))
        co_return Utils::ResultError("Missing required field: jsonrpc");
    if (!obj.contains("method"))
        co_return Utils::ResultError("Missing required field: method");
    if (!obj.contains("params"))
        co_return Utils::ResultError("Missing required field: params");
    TraceFileLoadingStartedNotification result;
    if (obj.value("jsonrpc").toString() != "2.0")
        co_return Utils::ResultError("Field 'jsonrpc' must be '2.0', got: " + obj.value("jsonrpc").toString());
    if (obj.value("method").toString() != "traceFileLoadingStarted")
        co_return Utils::ResultError("Field 'method' must be 'traceFileLoadingStarted', got: " + obj.value("method").toString());
    if (obj.contains("params") && obj["params"].isObject())
        result._params = co_await fromJson<TraceFileLoadingStartedNotification::Params>(obj["params"]);
    co_return result;
}

inline QJsonObject toJson(const TraceFileLoadingStartedNotification &data) {
    QJsonObject obj{
        {"jsonrpc", QString("2.0")},
        {"method", QString("traceFileLoadingStarted")},
        {"params", toJson(data._params)}
    };
    return obj;
}

/** A JSON-RPC notification sent from the application to the client. */
using ApplicationNotification = std::variant<TraceFileLoadingStartedNotification, TraceFileLoadingFinishedNotification, TraceEventSelectedNotification, TraceDiscardedNotification>;

template<>
inline Utils::Result<ApplicationNotification> fromJson<ApplicationNotification>(const QJsonValue &val) {
    if (!val.isObject())
        co_return Utils::ResultError("Invalid ApplicationNotification: expected object");
    const QString dispatchValue = val.toObject().value("method").toString();
    if (dispatchValue == "traceFileLoadingStarted")
        co_return ApplicationNotification(co_await fromJson<TraceFileLoadingStartedNotification>(val));
    else if (dispatchValue == "traceFileLoadingFinished")
        co_return ApplicationNotification(co_await fromJson<TraceFileLoadingFinishedNotification>(val));
    else if (dispatchValue == "traceEventSelected")
        co_return ApplicationNotification(co_await fromJson<TraceEventSelectedNotification>(val));
    else if (dispatchValue == "traceDiscarded")
        co_return ApplicationNotification(co_await fromJson<TraceDiscardedNotification>(val));
    co_return Utils::ResultError("Invalid ApplicationNotification: unknown method \"" + dispatchValue + "\"");
}

inline QJsonObject toJson(const ApplicationNotification &val) {
    return std::visit([](const auto &v) -> QJsonObject {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, QJsonObject>) {
            return v;
        } else {
            return toJson(v);
        }
    }, val);
}

inline QJsonValue toJsonValue(const ApplicationNotification &val) {
    return toJson(val);
}

/** Returns the 'method' dispatch field value for the active variant. */
inline QString dispatchValue(const ApplicationNotification &val) {
    return std::visit([](const auto &v) -> QString {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, TraceFileLoadingStartedNotification>) return "traceFileLoadingStarted";
        else if constexpr (std::is_same_v<T, TraceFileLoadingFinishedNotification>) return "traceFileLoadingFinished";
        else if constexpr (std::is_same_v<T, TraceEventSelectedNotification>) return "traceEventSelected";
        else if constexpr (std::is_same_v<T, TraceDiscardedNotification>) return "traceDiscarded";
        return {};
    }, val);
}
/** Request to exit the application. */
struct ExitRequest {
    RequestId _id;

    ExitRequest& id(const RequestId & v) { _id = v; return *this; }

    const RequestId& id() const { return _id; }
};

template<>
inline Utils::Result<ExitRequest> fromJson<ExitRequest>(const QJsonValue &val) {
    if (!val.isObject())
        co_return Utils::ResultError("Expected JSON object for ExitRequest");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("id"))
        co_return Utils::ResultError("Missing required field: id");
    if (!obj.contains("jsonrpc"))
        co_return Utils::ResultError("Missing required field: jsonrpc");
    if (!obj.contains("method"))
        co_return Utils::ResultError("Missing required field: method");
    ExitRequest result;
    if (obj.contains("id"))
        result._id = co_await fromJson<RequestId>(obj["id"]);
    if (obj.value("jsonrpc").toString() != "2.0")
        co_return Utils::ResultError("Field 'jsonrpc' must be '2.0', got: " + obj.value("jsonrpc").toString());
    if (obj.value("method").toString() != "exit")
        co_return Utils::ResultError("Field 'method' must be 'exit', got: " + obj.value("method").toString());
    co_return result;
}

inline QJsonObject toJson(const ExitRequest &data) {
    QJsonObject obj{
        {"id", toJsonValue(data._id)},
        {"jsonrpc", QString("2.0")},
        {"method", QString("exit")}
    };
    return obj;
}

/** Request to open a trace file. */
struct OpenTraceFileRequest {
    struct Params {
        QString _traceFilePath;

        Params& traceFilePath(const QString & v) { _traceFilePath = v; return *this; }

        const QString& traceFilePath() const { return _traceFilePath; }
    };

    std::optional<RequestId> _id;
    std::optional<Params> _params;

    OpenTraceFileRequest& id(const std::optional<RequestId> & v) { _id = v; return *this; }
    OpenTraceFileRequest& params(const std::optional<Params> & v) { _params = v; return *this; }

    const std::optional<RequestId>& id() const { return _id; }
    const std::optional<Params>& params() const { return _params; }
};

template<>
inline Utils::Result<OpenTraceFileRequest::Params> fromJson<OpenTraceFileRequest::Params>(const QJsonValue &val) {
    if (!val.isObject())
        return Utils::ResultError("Expected JSON object for Params");
    const QJsonObject obj = val.toObject();
    if (!obj.contains("method"))
        return Utils::ResultError("Missing required field: method");
    if (!obj.contains("traceFilePath"))
        return Utils::ResultError("Missing required field: traceFilePath");
    OpenTraceFileRequest::Params result;
    result._traceFilePath = obj.value("traceFilePath").toString();
    return result;
}

inline QJsonObject toJson(const OpenTraceFileRequest::Params &data) {
    QJsonObject obj{{"traceFilePath", data._traceFilePath}};
    return obj;
}

template<>
inline Utils::Result<OpenTraceFileRequest> fromJson<OpenTraceFileRequest>(const QJsonValue &val) {
    if (!val.isObject())
        co_return Utils::ResultError("Expected JSON object for OpenTraceFileRequest");
    const QJsonObject obj = val.toObject();
    OpenTraceFileRequest result;
    if (obj.contains("id"))
        result._id = co_await fromJson<RequestId>(obj["id"]);
    if (obj.value("jsonrpc").toString() != "2.0")
        co_return Utils::ResultError("Field 'jsonrpc' must be '2.0', got: " + obj.value("jsonrpc").toString());
    if (obj.value("method").toString() != "openTraceFile")
        co_return Utils::ResultError("Field 'method' must be 'openTraceFile', got: " + obj.value("method").toString());
    if (obj.contains("params") && obj["params"].isObject())
        result._params = co_await fromJson<OpenTraceFileRequest::Params>(obj["params"]);
    co_return result;
}

inline QJsonObject toJson(const OpenTraceFileRequest &data) {
    QJsonObject obj{
        {"jsonrpc", QString("2.0")},
        {"method", QString("openTraceFile")}
    };
    if (data._id.has_value())
        obj.insert("id", toJsonValue(*data._id));
    if (data._params.has_value())
        obj.insert("params", toJson(*data._params));
    return obj;
}

/** A JSON-RPC request sent from the client to the application. */
using ClientRequest = std::variant<OpenTraceFileRequest, ExitRequest>;

template<>
inline Utils::Result<ClientRequest> fromJson<ClientRequest>(const QJsonValue &val) {
    if (!val.isObject())
        co_return Utils::ResultError("Invalid ClientRequest: expected object");
    const QString dispatchValue = val.toObject().value("method").toString();
    if (dispatchValue == "openTraceFile")
        co_return ClientRequest(co_await fromJson<OpenTraceFileRequest>(val));
    else if (dispatchValue == "exit")
        co_return ClientRequest(co_await fromJson<ExitRequest>(val));
    co_return Utils::ResultError("Invalid ClientRequest: unknown method \"" + dispatchValue + "\"");
}

inline QJsonObject toJson(const ClientRequest &val) {
    return std::visit([](const auto &v) -> QJsonObject {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, QJsonObject>) {
            return v;
        } else {
            return toJson(v);
        }
    }, val);
}

inline QJsonValue toJsonValue(const ClientRequest &val) {
    return toJson(val);
}

/** Returns the 'method' dispatch field value for the active variant. */
inline QString dispatchValue(const ClientRequest &val) {
    return std::visit([](const auto &v) -> QString {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, OpenTraceFileRequest>) return "openTraceFile";
        else if constexpr (std::is_same_v<T, ExitRequest>) return "exit";
        return {};
    }, val);
}

} // namespace QmlTraceViewer::Api::Schema
