// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "utils_global.h"

#include "namevalueitem.h"

#include <QDialog>

#include <functional>
#include <memory>
#include <optional>

namespace Utils {

namespace Internal { class NameValueItemsWidget; }

class QTCREATOR_UTILS_EXPORT NameValuesDialog : public QDialog
{
public:
    void setNameValueItems(const NameValueItems &items);
    NameValueItems nameValueItems() const;

    void setPlaceholderText(const QString &text);

    using Polisher = std::function<void(QWidget *)>;
    static std::optional<NameValueItems> getNameValueItems(QWidget *parent = nullptr,
                                                             const NameValueItems &initial = {},
                                                             const QString &placeholderText = {},
                                                             Polisher polish = {},
                                                             const QString &windowTitle = {},
                                                             const QString &helpText = {});
protected:
    explicit NameValuesDialog(const QString &windowTitle,
                              const QString &helpText,
                              QWidget *parent = {});

private:
    Internal::NameValueItemsWidget *m_editor;
};

} // namespace Utils
