/****************************************************************************
**
** Copyright (C) 2018 Andre Hartmann <aha_1980@gmx.de>
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

#include <utils/filepath.h>

#include <QDialog>

QT_BEGIN_NAMESPACE
class QLabel;
class QCheckBox;
QT_END_NAMESPACE

namespace Core {

class FilePropertiesDialog : public QDialog
{
    Q_OBJECT

public:
    explicit FilePropertiesDialog(const Utils::FilePath &filePath, QWidget *parent = nullptr);
    ~FilePropertiesDialog() override;

private:
    void refresh();
    void setPermission(QFile::Permissions newPermissions, bool set);
    void detectTextFileSettings();

private:
    QLabel *m_name;
    QLabel *m_path;
    QLabel *m_mimeType;
    QLabel *m_defaultEditor;
    QLabel *m_lineEndings;
    QLabel *m_indentation;
    QLabel *m_owner;
    QLabel *m_group;
    QLabel *m_size;
    QLabel *m_lastRead;
    QLabel *m_lastModified;
    QCheckBox *m_readable;
    QCheckBox *m_writable;
    QCheckBox *m_executable;
    QCheckBox *m_symLink;
    const Utils::FilePath m_filePath;
};

} // Core
