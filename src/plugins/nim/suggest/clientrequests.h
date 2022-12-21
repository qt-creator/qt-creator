// Copyright (C) Filippo Cucchetto <filippocucchetto@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QObject>

namespace Nim {
namespace Suggest {

class Line
{
    Q_GADGET
public:
    enum class LineType : int {
        sug,
        con,
        def,
        use,
        dus,
        chk,
        mod,
        highlight,
        outline,
        known
    };

    Q_ENUM(LineType)

    enum class SymbolKind : int {
        skUnknown,
        skConditional,
        skDynLib,
        skParam,
        skGenericParam,
        skTemp,
        skModule,
        skType,
        skVar,
        skLet,
        skConst,
        skResult,
        skProc,
        skFunc,
        skMethod,
        skIterator,
        skConverter,
        skMacro,
        skTemplate,
        skField,
        skEnumField,
        skForVar,
        skLabel,
        skStub,
        skPackage,
        skAlias
    };

    Q_ENUM(SymbolKind)

    static bool fromString(LineType &type, const std::string &str);
    static bool fromString(SymbolKind &type, const std::string &str);

    friend QDebug operator<<(QDebug debug, const Line &c);

    LineType line_type;
    SymbolKind symbol_kind;
    QString abs_path;
    QString symbol_type;
    std::vector<QString> data;
    int row;
    int column;
    QString doc;
};

class NimSuggestClientRequest : public QObject
{
    Q_OBJECT

public:
    NimSuggestClientRequest(quint64 id);

    quint64 id() const
    {
        return m_id;
    }

    std::vector<Line> &lines()
    {
        return m_lines;
    }

    const std::vector<Line> &lines() const
    {
        return m_lines;
    }

signals:
    void finished();

private:
    friend class NimSuggestClient;

    void setFinished(std::vector<Line> lines)
    {
        m_lines = std::move(lines);
        emit finished();
    }

    const quint64 m_id;
    std::vector<Line> m_lines;
};

} // namespace Suggest
} // namespace Nim
