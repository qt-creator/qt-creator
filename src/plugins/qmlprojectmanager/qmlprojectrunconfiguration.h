/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**************************************************************************/

#ifndef QMLPROJECTRUNCONFIGURATION_H
#define QMLPROJECTRUNCONFIGURATION_H

#include "qmlprojectmanager_global.h"

#include <projectexplorer/runconfiguration.h>

#include <QWeakPointer>

QT_FORWARD_DECLARE_CLASS(QStringListModel)

namespace Core {
    class IEditor;
}

namespace Utils {
    class Environment;
    class EnvironmentItem;
}

namespace QtSupport { class BaseQtVersion; }

namespace QmlProjectManager {

namespace Internal {
    class QmlProjectRunConfigurationFactory;
    class QmlProjectRunConfigurationWidget;
}

class QMLPROJECTMANAGER_EXPORT QmlProjectRunConfiguration : public ProjectExplorer::RunConfiguration
{
    Q_OBJECT
    friend class Internal::QmlProjectRunConfigurationFactory;
    friend class Internal::QmlProjectRunConfigurationWidget;

public:
    QmlProjectRunConfiguration(ProjectExplorer::Target *parent, Core::Id id);
    virtual ~QmlProjectRunConfiguration();

    QString viewerPath() const;
    QString observerPath() const;
    QString viewerArguments() const;
    QString workingDirectory() const;
    QtSupport::BaseQtVersion *qtVersion() const;

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
    bool isEnabled() const;
    QString disabledReason() const;
    virtual QWidget *createConfigurationWidget();
    Utils::OutputFormatter *createOutputFormatter() const;
    QVariantMap toMap() const;

    ProjectExplorer::Abi abi() const;

public slots:
    void changeCurrentFile(Core::IEditor*);

private slots:
    void updateEnabled();

protected:
    QmlProjectRunConfiguration(ProjectExplorer::Target *parent,
                               QmlProjectRunConfiguration *source);
    virtual bool fromMap(const QVariantMap &map);
    void setEnabled(bool value);

private:
    void ctor();
    static bool isValidVersion(QtSupport::BaseQtVersion *version);
    
    static QString canonicalCapsPath(const QString &filePath);

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

    QWeakPointer<Internal::QmlProjectRunConfigurationWidget> m_configurationWidget;

    bool m_isEnabled;

    QList<Utils::EnvironmentItem> m_userEnvironmentChanges;
};

} // namespace QmlProjectManager

#endif // QMLPROJECTRUNCONFIGURATION_H
