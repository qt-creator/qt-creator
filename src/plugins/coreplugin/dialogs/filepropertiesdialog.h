// Copyright (C) 2018 Andre Hartmann <aha_1980@gmx.de>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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
