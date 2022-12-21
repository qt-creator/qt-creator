// Copyright (C) 2020 Denis Shienkov <denis.shienkov@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "uvtargetdriverselection.h"

#include <utils/detailsbutton.h>
#include <utils/detailswidget.h>
#include <utils/fileutils.h>

#include <QDialog>

QT_BEGIN_NAMESPACE
class QLineEdit;
QT_END_NAMESPACE

namespace BareMetal::Internal::Uv {

class DriverSelectionModel;
class DriverSelectionView;
class DriverSelectionCpuDllView;

// DriverSelector

class DriverSelector final : public Utils::DetailsWidget
{
    Q_OBJECT

public:
    explicit DriverSelector(const QStringList &supportedDrivers, QWidget *parent = nullptr);

    void setToolsIniFile(const Utils::FilePath &toolsIniFile);
    Utils::FilePath toolsIniFile() const;

    void setSelection(const DriverSelection &selection);
    DriverSelection selection() const;

signals:
    void selectionChanged();

private:
    Utils::FilePath m_toolsIniFile;
    DriverSelection m_selection;
};

// DriverSelectorToolPanel

class DriverSelectorToolPanel final : public Utils::FadingPanel
{
    Q_OBJECT

public:
    explicit DriverSelectorToolPanel(QWidget *parent = nullptr);

signals:
    void clicked();

private:
    void fadeTo(qreal value) final;
    void setOpacity(qreal value) final;
};

// DriverSelectorDetailsPanel

class DriverSelectorDetailsPanel final : public QWidget
{
    Q_OBJECT

public:
    explicit DriverSelectorDetailsPanel(DriverSelection &selection, QWidget *parent = nullptr);
    void refresh();

signals:
    void selectionChanged();

private:
    DriverSelection &m_selection;
    QLineEdit *m_dllEdit = nullptr;
    DriverSelectionCpuDllView *m_cpuDllView = nullptr;
};

// DriverSelectionDialog

class DriverSelectionDialog final : public QDialog
{
    Q_OBJECT

public:
    explicit DriverSelectionDialog(const Utils::FilePath &toolsIniFile,
                                   const QStringList &supportedDrivers,
                                   QWidget *parent = nullptr);
    DriverSelection selection() const;

private:
    void setSelection(const DriverSelection &selection);

    DriverSelection m_selection;
    DriverSelectionModel *m_model = nullptr;
    DriverSelectionView *m_view = nullptr;
};

} // BareMetal::Internal::Uv
