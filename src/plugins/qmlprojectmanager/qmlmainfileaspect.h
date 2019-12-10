/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "qmlprojectmanager_global.h"

#include <projectexplorer/runconfigurationaspects.h>

#include <QStandardItemModel>

namespace Core {
class IEditor;
}

QT_BEGIN_NAMESPACE
class QComboBox;
QT_END_NAMESPACE

namespace QmlProjectManager {

class QmlProject;
class QmlBuildSystem;

class QMLPROJECTMANAGER_EXPORT QmlMainFileAspect : public ProjectExplorer::ProjectConfigurationAspect
{
    Q_OBJECT
public:
    explicit QmlMainFileAspect(ProjectExplorer::Target *target);
    ~QmlMainFileAspect() override;

    enum MainScriptSource {
        FileInEditor,
        FileInProjectFile,
        FileInSettings
    };

    void addToLayout(ProjectExplorer::LayoutBuilder &builder) final;
    void toMap(QVariantMap &map) const final;
    void fromMap(const QVariantMap &map) final;

    void updateFileComboBox();
    MainScriptSource mainScriptSource() const;
    void setMainScript(int index);

    void setScriptSource(MainScriptSource source, const QString &settingsPath = QString());

    QString mainScript() const;
    QString currentFile() const;
    void changeCurrentFile(Core::IEditor *editor = nullptr);
    bool isQmlFilePresent();
    QmlBuildSystem *qmlBuildSystem() const;

public:

    ProjectExplorer::Target *m_target = nullptr;
    QPointer<QComboBox> m_fileListCombo;
    QStandardItemModel m_fileListModel;
    QString m_scriptFile;
    // absolute path to current file (if being used)
    QString m_currentFileFilename;
    // absolute path to selected main script (if being used)
    QString m_mainScriptFilename;
};

} // namespace QmlProjectManager
