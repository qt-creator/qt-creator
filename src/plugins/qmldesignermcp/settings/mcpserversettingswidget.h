// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include "mcpserverconfigstore.h"

#include <utils/uniqueobjectptr.h>

#include <QGroupBox>

QT_BEGIN_NAMESPACE
class QComboBox;
class QLabel;
class QLineEdit;
class QStackedWidget;
QT_END_NAMESPACE

namespace QmlDesigner {

class StringListWidget;

/**
 * @brief Settings widget for a single MCP server entry.
 *
 * Displayed as a checkable QGroupBox (enable/disable the server).
 * The transport selector switches between a Stdio panel (command +
 * args + workingDir) and an HTTP panel (URL + bearer token).
 */
class McpServerSettingsWidget : public QGroupBox
{
    Q_OBJECT

public:
    explicit McpServerSettingsWidget(const QString &serverName, QWidget *parent = nullptr);
    ~McpServerSettingsWidget();

    // Reload the widget fields from the persisted store.
    void load();

    // Persist the current field values.  Returns true if anything changed.
    bool save();

    QString serverName() const { return m_store.serverName(); }

signals:
    // Emitted when the user clicks the Remove button.
    void removeRequested(const QString &serverName);

private:
    void setupUi();
    void updateTransportPanelVisibility();

    McpServerConfigStore m_store;

    // Transport selector
    Utils::UniqueObjectPtr<QComboBox> m_transportCombo;

    // Stdio panel
    Utils::UniqueObjectPtr<QLineEdit> m_commandLineEdit;
    Utils::UniqueObjectPtr<QLineEdit> m_workingDirLineEdit;
    Utils::UniqueObjectPtr<StringListWidget> m_argsWidget;

    // HTTP panel
    Utils::UniqueObjectPtr<QLineEdit> m_urlLineEdit;
    Utils::UniqueObjectPtr<QLineEdit> m_bearerTokenLineEdit;

    // Shared panels (stacked)
    Utils::UniqueObjectPtr<QStackedWidget> m_transportStack;
};

} // namespace QmlDesigner
