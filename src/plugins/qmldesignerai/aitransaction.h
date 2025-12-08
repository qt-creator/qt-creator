// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QStringList>

namespace QmlDesigner {

class AiResponse;

class AiTransaction
{
public:
    explicit AiTransaction();
    explicit AiTransaction(const AiResponse &response);

    void commit();
    void rollback();

    bool applied() const { return m_applied; }
    bool isValid() const { return m_isValid; }
    bool producesQmlError() const { return m_producesQmlError; };

private:
    struct Context
    {
        QString qml;
        QStringList selectedItems;
    };

private: // variables
    Context m_before;
    Context m_after;
    bool m_applied = false;
    bool m_isValid = false;
    bool m_producesQmlError = false;
    bool m_affectsIds = false;
    bool m_affectsQml = false;
};

} // namespace QmlDesigner
