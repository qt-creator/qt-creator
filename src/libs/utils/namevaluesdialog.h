// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "namevalueitem.h"
#include "utils_global.h"

#include <QDialog>

#include <functional>
#include <optional>

namespace Utils {

namespace Internal { class TextEditHelper; }

class QTCREATOR_UTILS_EXPORT NameValueItemsWidget : public QWidget
{
    Q_OBJECT
public:
    explicit NameValueItemsWidget(QWidget *parent = nullptr);

    void setEnvironmentItems(const EnvironmentItems &items);
    EnvironmentItems environmentItems() const;

    void setPlaceholderText(const QString &text);

    enum class Selection { Name, Value };
    bool editVariable(const QString &name, Selection selection);

    void forceUpdateCheck();

signals:
    void userChangedItems(const EnvironmentItems &items);

private:
    Internal::TextEditHelper *m_editor;
    EnvironmentItems m_originalItems;
};

class QTCREATOR_UTILS_EXPORT NameValuesDialog : public QDialog
{
public:
    void setNameValueItems(const EnvironmentItems &items);
    EnvironmentItems nameValueItems() const;

    void setPlaceholderText(const QString &text);

    using Polisher = std::function<void(QWidget *)>;
    static std::optional<EnvironmentItems> getNameValueItems(QWidget *parent = nullptr,
                                                             const EnvironmentItems &initial = {},
                                                             const QString &placeholderText = {},
                                                             Polisher polish = {},
                                                             const QString &windowTitle = {});
protected:
    explicit NameValuesDialog(const QString &windowTitle,
                              QWidget *parent = {});

private:
    NameValueItemsWidget *m_editor;
};

} // namespace Utils
