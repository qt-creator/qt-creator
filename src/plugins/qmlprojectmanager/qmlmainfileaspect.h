// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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

class QMLPROJECTMANAGER_EXPORT QmlMainFileAspect : public Utils::BaseAspect
{
    Q_OBJECT

public:
    explicit QmlMainFileAspect(Utils::AspectContainer *container = nullptr);
    ~QmlMainFileAspect() override;

    enum MainScriptSource {
        FileInEditor,
        FileInProjectFile,
        FileInSettings
    };

    struct Data : BaseAspect::Data
    {
        Utils::FilePath mainScript;
        Utils::FilePath currentFile;
    };

    void addToLayout(Layouting::LayoutItem &parent) final;
    void toMap(Utils::Store &map) const final;
    void fromMap(const Utils::Store &map) final;

    void updateFileComboBox();
    MainScriptSource mainScriptSource() const;
    void setMainScript(int index);

    void setTarget(ProjectExplorer::Target *target);
    void setScriptSource(MainScriptSource source, const QString &settingsPath = QString());

    Utils::FilePath mainScript() const;
    Utils::FilePath currentFile() const;
    void changeCurrentFile(Core::IEditor *editor = nullptr);
    bool isQmlFilePresent();
    QmlBuildSystem *qmlBuildSystem() const;

public:
    ProjectExplorer::Target *m_target = nullptr;
    QPointer<QComboBox> m_fileListCombo;
    QStandardItemModel m_fileListModel;
    QString m_scriptFile;
    // absolute path to current file (if being used)
    Utils::FilePath m_currentFileFilename;
    // absolute path to selected main script (if being used)
    Utils::FilePath m_mainScriptFilename;
};

} // QmlProjectManager
