/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef QMLPROJECTRUNCONFIGURATION_H
#define QMLPROJECTRUNCONFIGURATION_H

#include "qmlprojectmanager_global.h"

#include <projectexplorer/localapplicationrunconfiguration.h>

#include <QPointer>

QT_FORWARD_DECLARE_CLASS(QStringListModel)

namespace Core { class IEditor; }

namespace QtSupport { class BaseQtVersion; }

namespace QmlProjectManager {
class QmlProject;

namespace Internal {
    class QmlProjectRunConfigurationFactory;
    class QmlProjectRunConfigurationWidget;
}

class QMLPROJECTMANAGER_EXPORT QmlProjectRunConfiguration : public ProjectExplorer::LocalApplicationRunConfiguration
{
    Q_OBJECT
    friend class Internal::QmlProjectRunConfigurationFactory;
    friend class Internal::QmlProjectRunConfigurationWidget;
    friend class QmlProject; // to call updateEnabled()

public:
    QmlProjectRunConfiguration(ProjectExplorer::Target *parent, Core::Id id);

    QString executable() const;
    ProjectExplorer::ApplicationLauncher::Mode runMode() const;
    QString commandLineArguments() const;

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

    // RunConfiguration
    bool isEnabled() const;
    QString disabledReason() const;
    virtual QWidget *createConfigurationWidget();
    Utils::OutputFormatter *createOutputFormatter() const;
    QVariantMap toMap() const;

    ProjectExplorer::Abi abi() const;
signals:
    void scriptSourceChanged();

private slots:
    void changeCurrentFile(Core::IEditor* = 0);
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

    // absolute path to current file (if being used)
    QString m_currentFileFilename;
    // absolute path to selected main script (if being used)
    QString m_mainScriptFilename;

    QString m_scriptFile;
    QString m_qmlViewerArgs;

    bool m_isEnabled;
};

} // namespace QmlProjectManager

#endif // QMLPROJECTRUNCONFIGURATION_H
