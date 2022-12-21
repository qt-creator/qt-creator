// Copyright (C) 2020 Denis Shienkov <denis.shienkov@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "uvtargetdeviceselection.h"

#include <utils/detailsbutton.h>
#include <utils/detailswidget.h>
#include <utils/fileutils.h>

#include <QDialog>

QT_BEGIN_NAMESPACE
class QLineEdit;
class QPlainTextEdit;
QT_END_NAMESPACE

namespace Utils { class PathChooser; }

namespace BareMetal::Internal::Uv {

class DeviceSelectionModel;
class DeviceSelectionView;
class DeviceSelectionMemoryView;
class DeviceSelectionAlgorithmView;

// DeviceSelector

class DeviceSelector final : public Utils::DetailsWidget
{
    Q_OBJECT

public:
    explicit DeviceSelector(QWidget *parent = nullptr);

    void setToolsIniFile(const Utils::FilePath &toolsIniFile);
    Utils::FilePath toolsIniFile() const;

    void setSelection(const DeviceSelection &selection);
    DeviceSelection selection() const;

signals:
    void selectionChanged();

private:
    Utils::FilePath m_toolsIniFile;
    DeviceSelection m_selection;
};

// DeviceSelectorToolPanel

class DeviceSelectorToolPanel final : public Utils::FadingPanel
{
    Q_OBJECT

public:
    explicit DeviceSelectorToolPanel(QWidget *parent = nullptr);

signals:
    void clicked();

private:
    void fadeTo(qreal value) final;
    void setOpacity(qreal value) final;
};

// DeviceSelectorDetailsPanel

class DeviceSelectorDetailsPanel final : public QWidget
{
    Q_OBJECT

public:
    explicit DeviceSelectorDetailsPanel(DeviceSelection &selection, QWidget *parent = nullptr);
    void refresh();

signals:
    void selectionChanged();

private:
    DeviceSelection &m_selection;
    QLineEdit *m_vendorEdit = nullptr;
    QLineEdit *m_packageEdit = nullptr;
    QPlainTextEdit *m_descEdit = nullptr;
    DeviceSelectionMemoryView *m_memoryView = nullptr;
    DeviceSelectionAlgorithmView *m_algorithmView = nullptr;
    Utils::PathChooser *m_peripheralDescriptionFileChooser = nullptr;
};

// DeviceSelectionDialog

class DeviceSelectionDialog final : public QDialog
{
    Q_OBJECT

public:
    explicit DeviceSelectionDialog(const Utils::FilePath &toolsIniFile, QWidget *parent = nullptr);
    DeviceSelection selection() const;

private:
    void setSelection(const DeviceSelection &selection);

    DeviceSelection m_selection;
    DeviceSelectionModel *m_model = nullptr;
    DeviceSelectionView *m_view = nullptr;
};

} // BareMetal::Internal::Uv
