// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "environment.h"
#include "utils_global.h"

#include <QDialog>

#include <functional>
#include <optional>

QT_FORWARD_DECLARE_CLASS(QCheckBox)

namespace Utils {
namespace Internal { class TextEditHelper; }
class FilePath;
class PathChooser;

class QTCREATOR_UTILS_EXPORT NameValueItemsWidget : public QWidget
{
    Q_OBJECT
public:
    explicit NameValueItemsWidget(QWidget *parent = nullptr);

    void setEnvironmentChanges(const EnvironmentChanges &userEnv);
    EnvironmentChanges envChanges() const;

    void setPlaceholderText(const QString &text);
    void setBrowseHint(const FilePath &hint);

    enum class Selection { Name, Value };
    bool editVariable(const QString &name, Selection selection);

    void forceUpdateCheck();
    void setupDirtyHooks();

signals:
    void userChangedItems(const EnvironmentChanges &envChanges);

private:
    Internal::TextEditHelper *m_editor;
    QCheckBox * const m_fileCheckBox;
    PathChooser * const m_scriptChooser;
    QCheckBox * const m_scriptCheckBox;
    EnvironmentChanges m_originalEnvChanges;
    bool m_interactive = true;
};

class QTCREATOR_UTILS_EXPORT NameValuesDialog : public QDialog
{
public:
    void setEnvChanges(const EnvironmentChanges &envFromUser);
    EnvironmentChanges envChanges() const;

    void setPlaceholderText(const QString &text);

    void setBrowseHint(const FilePath &hint) { m_editor->setBrowseHint(hint); }

    using Polisher = std::function<void(QWidget *)>;
    static std::optional<EnvironmentChanges> getNameValueItems(
        QWidget *parent = nullptr,
        const EnvironmentChanges &initial = {},
        const QString &placeholderText = {},
        Polisher polish = {},
        const QString &windowTitle = {},
        const FilePath &browseHint = {});

protected:
    explicit NameValuesDialog(const QString &windowTitle,
                              QWidget *parent = {});

private:
    NameValueItemsWidget *m_editor;
};

QTCREATOR_UTILS_EXPORT std::optional<EnvironmentChanges> runEnvironmentItemsDialog(
    QWidget *parent = nullptr,
    const EnvironmentChanges &initial = {},
    const QString &placeholderText = {},
    NameValuesDialog::Polisher polish = {},
    const QString &dialogTitle = {},
    const FilePath &browseHint = {});

} // namespace Utils
