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

namespace BareMetal {
namespace Internal {
namespace Uv {

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

} // namespace Uv
} // namespace Internal
} // namespace BareMetal
