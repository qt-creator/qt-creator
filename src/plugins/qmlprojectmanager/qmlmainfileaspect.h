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
    explicit QmlMainFileAspect(ProjectExplorer::Target *target);
    ~QmlMainFileAspect() override;

    enum MainScriptSource {
        FileInEditor,
        FileInProjectFile,
        FileInSettings
    };

    struct Data : BaseAspect::Data
    {
        QString mainScript;
        QString currentFile;
    };

    void addToLayout(Utils::LayoutBuilder &builder) final;
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
