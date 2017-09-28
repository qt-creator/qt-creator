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

#include <coreplugin/editormanager/iexternaleditor.h>
#include <coreplugin/id.h>
#include <functional>

#include <QStringList>
#include <QMap>

QT_BEGIN_NAMESPACE
class QTcpSocket;
QT_END_NAMESPACE

namespace QtSupport { class BaseQtVersion; }

namespace QmakeProjectManager {
namespace Internal {

/* Convenience parametrizable base class for Qt editors/binaries
 * Provides convenience functions that
 * try to retrieve the binary of the editor from the Qt version
 * of the project the file belongs to, falling back to path search
 * if none is found. On Mac, the "open" mechanism can be optionally be used. */

class ExternalQtEditor : public Core::IExternalEditor
{
    Q_OBJECT

public:
    // Member function pointer for a QtVersion function return a string (command)
    using CommandForQtVersion = std::function<QString(const QtSupport::BaseQtVersion *)>;

    static ExternalQtEditor *createLinguistEditor();
    static ExternalQtEditor *createDesignerEditor();

    QStringList mimeTypes() const override;
    Core::Id id() const override;
    QString displayName() const override;

    bool startEditor(const QString &fileName, QString *errorMessage) override;

    // Data required to launch the editor
    struct LaunchData {
        QString binary;
        QStringList arguments;
        QString workingDirectory;
    };

protected:
    ExternalQtEditor(Core::Id id,
                     const QString &displayName,
                     const QString &mimetype,
                     const CommandForQtVersion &commandForQtVersion);

    // Try to retrieve the binary of the editor from the Qt version,
    // prepare arguments accordingly (Mac "open" if desired)
    bool getEditorLaunchData(const QString &fileName,
                             LaunchData *data,
                             QString *errorMessage) const;

    // Create and start a detached GUI process executing in the background.
    // Set the project environment if there is one.
    bool startEditorProcess(const LaunchData &data, QString *errorMessage);

private:
    const QStringList m_mimeTypes;
    const Core::Id m_id;
    const QString m_displayName;
    const CommandForQtVersion m_commandForQtVersion;
};

/* Qt Designer on the remaining platforms: Uses Designer's own
 * Tcp-based communication mechanism to ensure all files are opened
 * in one instance (per version). */

class DesignerExternalEditor : public ExternalQtEditor
{
    Q_OBJECT
public:
    DesignerExternalEditor();

    bool startEditor(const QString &fileName, QString *errorMessage) override;

private:
    void processTerminated(const QString &binary);

    // A per-binary entry containing the socket
    typedef QMap<QString, QTcpSocket*> ProcessCache;

    ProcessCache m_processCache;
};

} // namespace Internal
} // namespace QmakeProjectManager
