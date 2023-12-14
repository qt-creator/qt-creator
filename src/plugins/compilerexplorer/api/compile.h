// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "compiler.h"
#include "config.h"

#include "../compilerexploreraspects.h"

#include <QList>
#include <QString>

#include <QFuture>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>

namespace CompilerExplorer::Api {

struct ExecuteParameters
{
    ExecuteParameters &args(QStringList args)
    {
        obj["args"] = QJsonArray::fromStringList(args);
        return *this;
    }
    ExecuteParameters &stdIn(const QString &stdIn)
    {
        obj["stdin"] = stdIn;
        return *this;
    }
    QJsonObject obj;
};

struct CompileParameters
{
    CompileParameters() = delete;
    CompileParameters(const QString &cId)
        : compilerId(cId)
    {
        obj["compiler"] = cId;
    }
    CompileParameters(const Compiler &compiler)
        : compilerId(compiler.id)
    {
        obj["compiler"] = compiler.id;
    }

    QByteArray toByteArray() const { return QJsonDocument(obj).toJson(QJsonDocument::Compact); }

    struct Options
    {
        Options &userArguments(const QString &userArguments)
        {
            obj["userArguments"] = userArguments;
            return *this;
        }

        struct CompilerOptions
        {
            bool skipAsm{false};
            bool executorRequest{false};
        };

        Options &compilerOptions(CompilerOptions compilerOptions)
        {
            QJsonObject co;
            co["skipAsm"] = compilerOptions.skipAsm;
            co["executorRequest"] = compilerOptions.executorRequest;

            obj["compilerOptions"] = co;
            return *this;
        }

        Options &executeParameters(ExecuteParameters executeParameters)
        {
            obj["executeParameters"] = executeParameters.obj;
            return *this;
        }

        struct Filters
        {
            bool binary{false};
            bool binaryObject{false};
            bool commentOnly{true};
            bool demangle{true};
            bool directives{true};
            bool execute{false};
            bool intel{true};
            bool labels{true};
            bool libraryCode{false};
            bool trim{false};
            bool debugCalls{false};
        };

        Options &filters(Filters filters)
        {
            QJsonObject filter;
            filter["binary"] = filters.binary;
            filter["binaryObject"] = filters.binaryObject;
            filter["commentOnly"] = filters.commentOnly;
            filter["demangle"] = filters.demangle;
            filter["directives"] = filters.directives;
            filter["execute"] = filters.execute;
            filter["intel"] = filters.intel;
            filter["labels"] = filters.labels;
            filter["libraryCode"] = filters.libraryCode;
            filter["trim"] = filters.trim;
            filter["debugCalls"] = filters.debugCalls;

            obj["filters"] = filter;
            return *this;
        }

        struct Libraries
        {
            QJsonArray array;
            Libraries &add(const QString id, const QString version)
            {
                QJsonObject obj;
                obj["id"] = id;
                obj["version"] = version;
                array.append(obj);
                return *this;
            }
        };

        Options &libraries(const Libraries &libraries)
        {
            obj["libraries"] = libraries.array;
            return *this;
        }

        Options &libraries(const QMap<QString, QString> &libraries)
        {
            Libraries result;
            for (const auto &key : libraries.keys()) {
                result.add(key, libraries[key]);
            }
            obj["libraries"] = result.array;
            return *this;
        }

        QJsonObject obj;
    };

    CompileParameters &source(const QString &source)
    {
        obj["source"] = source;
        return *this;
    }
    CompileParameters &language(const QString &languageId)
    {
        obj["lang"] = languageId;
        return *this;
    }
    CompileParameters &options(Options options)
    {
        obj["options"] = options.obj;
        return *this;
    }

    QJsonObject obj;
    QString compilerId;
};

struct CompilerResult
{
    struct Line
    {
        QString text;
        struct Tag
        {
            int column;
            QString file;
            int line;
            int severity;
            QString text;
            static Tag fromJson(const QJsonObject &obj)
            {
                Tag tag;
                tag.column = obj["column"].toInt();
                tag.file = obj["file"].toString();
                tag.line = obj["line"].toInt();
                tag.severity = obj["severity"].toInt();
                tag.text = obj["text"].toString();
                return tag;
            }
        };
        std::optional<Tag> tag;

        static Line fromJson(const QJsonObject &obj)
        {
            Line line;
            line.text = obj["text"].toString();
            if (obj.contains("tag")) {
                line.tag = Tag::fromJson(obj["tag"].toObject());
            }
            return line;
        }
    };

    int code;
    bool timedOut;
    bool truncated;
    QList<Line> stdErr;
    QList<Line> stdOut;

    static CompilerResult fromJson(const QJsonObject &object)
    {
        CompilerResult result;

        result.timedOut = object["timedOut"].toBool();
        result.truncated = object["truncated"].toBool();
        result.code = object["code"].toInt();
        for (const auto &line : object["stderr"].toArray())
            result.stdErr.append(Line::fromJson(line.toObject()));
        for (const auto &line : object["stdout"].toArray())
            result.stdOut.append(Line::fromJson(line.toObject()));

        return result;
    }
};

struct CompileResult : CompilerResult
{
    struct AssemblyLine
    {
        // A part of the asm that is a (hyper)link to a label (the name references labelDefinitions)
        struct Label
        {
            QString name;
            struct Range
            {
                int startCol;
                int endCol;
            } range;

            static Label fromJson(const QJsonObject &object)
            {
                Label label;
                label.name = object["name"].toString();
                label.range.startCol = object["range"]["startCol"].toInt();
                label.range.endCol = object["range"]["endCol"].toInt();
                return label;
            }

            bool operator==(const Label &other) const
            {
                return name == other.name && range.startCol == other.range.startCol
                       && range.endCol == other.range.endCol;
            }
            bool operator!=(const Label &other) const { return !(*this == other); }
        };
        QList<Label> labels;
        // The part of the source that generated this asm
        struct Source
        {
            std::optional<int> column;
            QString file;
            int line;
            bool operator==(const Source &other) const
            {
                return column == other.column && file == other.file && line == other.line;
            }
            bool operator!=(const Source &other) const { return !(*this == other); }
        };

        std::optional<Source> source;

        QString text;
        QStringList opcodes;

        static AssemblyLine fromJson(const QJsonObject &object)
        {
            AssemblyLine line;
            line.text = object["text"].toString();
            auto opcodes = object["opcodes"].toArray();
            for (const auto &opcode : opcodes)
                line.opcodes.append(opcode.toString());

            auto itSource = object.find("source");
            if (itSource != object.end() && !itSource->isNull()) {
                std::optional<int> columnOpt;
                auto source = itSource->toObject();

                auto itColumn = source.find("column");
                if (itColumn != source.end() && !itColumn->isNull())
                    columnOpt = itColumn->toInt();

                line.source = Source{
                    columnOpt,
                    source["file"].toString(),
                    source["line"].toInt(),
                };
            }

            for (const auto &label : object["labels"].toArray()) {
                line.labels.append(Label::fromJson(label.toObject()));
            }
            return line;
        }

        bool operator==(const AssemblyLine &other) const
        {
            return source == other.source && text == other.text && opcodes == other.opcodes;
        }
        bool operator!=(const AssemblyLine &other) const { return !(*this == other); }
    };

    struct ExecResult
    {
        int code;
        bool didExecute;
        bool timedOut;
        bool truncated;
        QStringList stdOutLines;
        QStringList stdErrLines;

        CompilerResult buildResult;

        static ExecResult fromJson(const QJsonObject &object)
        {
            ExecResult result;
            result.code = object["code"].toInt();
            result.didExecute = object["didExecute"].toBool();
            result.timedOut = object["timedOut"].toBool();
            result.truncated = object["truncated"].toBool();
            for (const auto &line : object["stdout"].toArray())
                result.stdOutLines.append(line.toObject()["text"].toString());
            for (const auto &line : object["stderr"].toArray())
                result.stdErrLines.append(line.toObject()["text"].toString());
            result.buildResult = CompilerResult::fromJson(object["buildResult"].toObject());
            return result;
        }
    };

    QMap<QString, int> labelDefinitions;
    QList<AssemblyLine> assemblyLines;

    std::optional<ExecResult> execResult;

    static CompileResult fromJson(const QJsonObject &object)
    {
        CompilerResult compilerResult = CompilerResult::fromJson(object);

        CompileResult result;
        result.code = compilerResult.code;
        result.timedOut = compilerResult.timedOut;
        result.truncated = compilerResult.truncated;
        result.stdOut = compilerResult.stdOut;
        result.stdErr = compilerResult.stdErr;

        if (object.contains("labelDefinitions")) {
            QJsonObject labelDefinitions = object["labelDefinitions"].toObject();
            for (const QString &key : labelDefinitions.keys())
                result.labelDefinitions[key] = labelDefinitions[key].toInt();
        }

        if (object.contains("asm")) {
            for (const auto &asmLine : object["asm"].toArray())
                result.assemblyLines.append(AssemblyLine::fromJson(asmLine.toObject()));
        }

        if (object.contains("execResult"))
            result.execResult = ExecResult::fromJson(object["execResult"].toObject());

        return result;
    }
};

QFuture<CompileResult> compile(const Config &config, const CompileParameters &parameters);

} // namespace CompilerExplorer::Api
