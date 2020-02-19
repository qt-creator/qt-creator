/****************************************************************************
**
** Copyright (C) 2020 Denis Shienkov <denis.shienkov@gmail.com>
** Contact: https://www.qt.io/licensing/
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

#include "uvtargetdriverselection.h"

#include <utils/detailsbutton.h>
#include <utils/detailswidget.h>
#include <utils/fileutils.h>

#include <QDialog>

QT_BEGIN_NAMESPACE
class QLineEdit;
QT_END_NAMESPACE

namespace BareMetal {
namespace Internal {
namespace Uv {

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

} // namespace Uv
} // namespace Internal
} // namespace BareMetal
