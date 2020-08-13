/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
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

#include "qmlpreview_global.h"

#include <utils/fileutils.h>
#include <utils/outputformat.h>

#include <QWidget>

QT_BEGIN_NAMESPACE
class QRadioButton;
class QPushButton;
class QHBoxLayout;
QT_END_NAMESPACE

namespace Core {
class IEditor;
class OutputWindow;
}
namespace ProjectExplorer {
class Project;
class RunControl;
}

namespace QmlPreview {

class QMLPREVIEW_EXPORT QmlDebugTranslationWidget : public QWidget
{
    Q_OBJECT
public:
    explicit QmlDebugTranslationWidget(QWidget *parent = nullptr);
    ~QmlDebugTranslationWidget() override;

    void setCurrentFile(const Utils::FilePath &filepath);
    void setFiles(const Utils::FilePaths &filePathes);
    void updateStartupProjectTranslations();
private:
    void updateCurrentEditor(const Core::IEditor *editor);
    void updateCurrentTranslations(ProjectExplorer::Project *project);
    void runTest();
    void appendMessage(const QString &message, Utils::OutputFormat format);
    void clear();
    void loadLogFile();
    void saveLogToFile();
    QString currentDir() const;
    void setCurrentDir(const QString &path);

    QString singleFileButtonText(const QString &filePath);
    QString runButtonText(bool isRunning = false);

    QStringList m_testLanguages;
    QString m_lastUsedLanguageBeforeTest;
    bool m_elideWarning = false;

    Core::OutputWindow *m_runOutputWindow = nullptr;

    QRadioButton *m_singleFileButton = nullptr;
    QRadioButton *m_multipleFileButton = nullptr;
    QPushButton *m_runTestButton = nullptr;

    Utils::FilePath m_currentFilePath;
    Utils::FilePaths m_selectedFilePaths;
    ProjectExplorer::RunControl *m_currentRunControl = nullptr;

    QString m_lastDir;

    QHBoxLayout *m_selectLanguageLayout;
};

} // namespace QmlPreview
