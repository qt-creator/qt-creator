// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/fileutils.h>

#include <QWidget>
#include <QStringList>

class QCheckBox;
class QComboBox;
class QPushButton;
class QListView;

namespace TextEditor { class TextEditorWidget; }

namespace Android::Internal {

class PermissionsModel;

class PermissionsContainerWidget : public QWidget
{
    Q_OBJECT
public:
    explicit PermissionsContainerWidget(QWidget *parent = nullptr);
    bool initialize(TextEditor::TextEditorWidget *textEditorWidget);

signals:
    void permissionsModified();

private:
    void addPermission();
    void removePermission();
    void updateAddRemovePermissionButtons();
    void defaultPermissionOrFeatureCheckBoxClicked();

    TextEditor::TextEditorWidget *m_textEditorWidget = nullptr;
    Utils::FilePath m_manifestDirectory;

    QCheckBox *m_defaultPermissonsCheckBox = nullptr;
    QCheckBox *m_defaultFeaturesCheckBox = nullptr;
    QComboBox *m_permissionsComboBox = nullptr;
    QPushButton *m_addPermissionButton = nullptr;
    QPushButton *m_removePermissionButton = nullptr;
    QListView *m_permissionsListView = nullptr;
    PermissionsModel *m_permissionsModel = nullptr;
};

} // namespace Android::Internal
