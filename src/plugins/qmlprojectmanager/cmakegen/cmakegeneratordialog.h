// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0


#ifndef CMAKEGENERATORDIALOG_H
#define CMAKEGENERATORDIALOG_H

#include "cmakegeneratordialogtreemodel.h"

#include <utils/fileutils.h>

#include <QDialog>
#include <QTextEdit>
#include <QTreeView>
#include <QLabel>

namespace QmlProjectManager {
namespace GenerateCmake {

class CmakeGeneratorDialog : public QDialog
{
    Q_OBJECT

public:
    CmakeGeneratorDialog(const Utils::FilePath &rootDir,
                         const Utils::FilePaths &files,
                         const Utils::FilePaths invalidFiles);
    Utils::FilePaths getFilePaths();

public slots:
    void refreshNotificationText();
    void advancedVisibilityChanged(bool visible);

private:
    QTreeView* createFileTree();
    QWidget* createDetailsWidget();
    QWidget* createButtons();

private:
    CMakeGeneratorDialogTreeModel *m_model;
    QTextEdit *m_notifications;
    QVariant m_warningIcon;
    Utils::FilePath m_rootDir;
    Utils::FilePaths m_files;
    Utils::FilePaths m_invalidFiles;
};

} //GenerateCmake
} //QmlProjectManager

#endif // CMAKEGENERATORDIALOG_H
