/****************************************************************************
**
** Copyright (C) Filippo Cucchetto <filippocucchetto@gmail.com>
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

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

    LineType line_type;
    SymbolKind symbol_kind;
    QString abs_path;
    QString symbol_type;
    std::vector<QString> data;
    int row;
    int column;
    QString doc;
};

class BaseNimSuggestClientRequest : public QObject
{
    Q_OBJECT

public:
    BaseNimSuggestClientRequest(quint64 id);

    quint64 id() const;

signals:
    void finished();

private:
    const quint64 m_id;
};

class SugRequest : public BaseNimSuggestClientRequest
{
public:
    using BaseNimSuggestClientRequest::BaseNimSuggestClientRequest;

    std::vector<Line> &lines()
    {
        return m_lines;
    }

    const std::vector<Line> &lines() const
    {
        return m_lines;
    }

private:
    friend class NimSuggestClient;
    void setFinished(std::vector<Line> lines)
    {
        m_lines = std::move(lines);
        emit finished();
    }

    std::vector<Line> m_lines;
};

} // namespace Suggest
} // namespace Nim

QDebug operator<<(QDebug debug, const Nim::Suggest::Line &c);
