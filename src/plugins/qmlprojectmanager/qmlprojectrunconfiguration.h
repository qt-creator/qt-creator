/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#ifndef QMLPROJECTRUNCONFIGURATION_H
#define QMLPROJECTRUNCONFIGURATION_H

#include "qmlprojectmanager_global.h"
#include <projectexplorer/runconfiguration.h>
#include <QWeakPointer>
#include <QComboBox>
#include <QLabel>

QT_FORWARD_DECLARE_CLASS(QStringListModel);

namespace Core {
    class IEditor;
}

namespace Utils {
    class Environment;
    class EnvironmentItem;
}

namespace Qt4ProjectManager {
    class QtVersion;
}

namespace QmlProjectManager {

namespace Internal {
    class QmlProjectTarget;
    class QmlProjectRunConfigurationFactory;
    class QmlProjectRunConfigurationWidget;
}

class QMLPROJECTMANAGER_EXPORT QmlProjectRunConfiguration : public ProjectExplorer::RunConfiguration
{
    Q_OBJECT
    friend class Internal::QmlProjectRunConfigurationFactory;
    friend class Internal::QmlProjectRunConfigurationWidget;

    // used in qmldumptool.cpp
    Q_PROPERTY(int qtVersionId READ qtVersionId)

public:
    QmlProjectRunConfiguration(Internal::QmlProjectTarget *parent);
    virtual ~QmlProjectRunConfiguration();

    Internal::QmlProjectTarget *qmlTarget() const;

    QString viewerPath() const;
    QString observerPath() const;
    QString viewerArguments() const;
    QString workingDirectory() const;
    int qtVersionId() const;
    Qt4ProjectManager::QtVersion *qtVersion() const;

    enum MainScriptSource {
        FileInEditor,
        FileInProjectFile,
        FileInSettings
    };
    MainScriptSource mainScriptSource() const;
    void setScriptSource(MainScriptSource source, const QString &settingsPath = QString());

    QString mainScript() const;

    Utils::Environment environment() const;

    // RunConfiguration
    bool isEnabled(ProjectExplorer::BuildConfiguration *bc) const;
    virtual QWidget *createConfigurationWidget();
    ProjectExplorer::OutputFormatter *createOutputFormatter() const;
    QVariantMap toMap() const;

public slots:
    void changeCurrentFile(Core::IEditor*);

private slots:
    void updateEnabled();
    void updateQtVersions();

protected:
    QmlProjectRunConfiguration(Internal::QmlProjectTarget *parent,
                               QmlProjectRunConfiguration *source);
    virtual bool fromMap(const QVariantMap &map);
    void setEnabled(bool value);

private:
    void ctor();
    static bool isValidVersion(Qt4ProjectManager::QtVersion *version);
    void setQtVersionId(int id);

    Utils::Environment baseEnvironment() const;
    void setUserEnvironmentChanges(const QList<Utils::EnvironmentItem> &diff);
    QList<Utils::EnvironmentItem> userEnvironmentChanges() const;

    // absolute path to current file (if being used)
    QString m_currentFileFilename;
    // absolute path to selected main script (if being used)
    QString m_mainScriptFilename;

    int m_qtVersionId;
    QString m_scriptFile;
    QString m_qmlViewerArgs;

    Internal::QmlProjectTarget *m_projectTarget;
    QWeakPointer<Internal::QmlProjectRunConfigurationWidget> m_configurationWidget;

    bool m_usingCurrentFile;
    bool m_isEnabled;

    QList<Utils::EnvironmentItem> m_userEnvironmentChanges;
};

} // namespace QmlProjectManager

#endif // QMLPROJECTRUNCONFIGURATION_H
