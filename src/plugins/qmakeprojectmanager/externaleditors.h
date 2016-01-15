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

#ifndef EXTERNALEDITORS_H
#define EXTERNALEDITORS_H

#include <coreplugin/editormanager/iexternaleditor.h>
#include <coreplugin/id.h>

#include <QStringList>
#include <QMap>

QT_BEGIN_NAMESPACE
class QProcess;
class QTcpSocket;
class QSignalMapper;
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
    virtual QStringList mimeTypes() const;
    virtual Core::Id id() const;
    virtual QString displayName() const;

protected:
    // Member function pointer for a QtVersion function return a string (command)
    typedef QString (QtSupport::BaseQtVersion::*QtVersionCommandAccessor)() const;

    // Data required to launch the editor
    struct EditorLaunchData {
        QString binary;
        QStringList arguments;
        QString workingDirectory;
    };

    explicit ExternalQtEditor(Core::Id id,
                              const QString &displayName,
                              const QString &mimetype,
                              QObject *parent = 0);

    // Try to retrieve the binary of the editor from the Qt version,
    // prepare arguments accordingly (Mac "open" if desired)
    bool getEditorLaunchData(const QString &fileName,
                             QtVersionCommandAccessor commandAccessor,
                             const QString &fallbackBinary,
                             const QStringList &additionalArguments,
                             bool useMacOpenCommand,
                             EditorLaunchData *data,
                             QString *errorMessage) const;

    // Create and start a detached GUI process executing in the background.
    // Set the project environment if there is one.
    bool startEditorProcess(const EditorLaunchData &data, QString *errorMessage);

private:
    const QStringList m_mimeTypes;
    const Core::Id m_id;
    const QString m_displayName;
};

// Qt Linguist
class LinguistExternalEditor : public ExternalQtEditor
{
    Q_OBJECT
public:
    explicit LinguistExternalEditor(QObject *parent = 0);
    virtual bool startEditor(const QString &fileName, QString *errorMessage);
};

// Qt Designer on Mac: Make use of the Mac's 'open' mechanism to
// ensure files are opened in the same (per version) instance.

class MacDesignerExternalEditor : public ExternalQtEditor
{
    Q_OBJECT
public:
    explicit MacDesignerExternalEditor(QObject *parent = 0);
    virtual bool startEditor(const QString &fileName, QString *errorMessage);
};

/* Qt Designer on the remaining platforms: Uses Designer's own
 * Tcp-based communication mechanism to ensure all files are opened
 * in one instance (per version). */

class DesignerExternalEditor : public ExternalQtEditor
{
    Q_OBJECT
public:
    explicit DesignerExternalEditor(QObject *parent = 0);

    virtual bool startEditor(const QString &fileName, QString *errorMessage);

private slots:
    void processTerminated(const QString &binary);

private:
    // A per-binary entry containing the socket
    typedef QMap<QString, QTcpSocket*> ProcessCache;

    ProcessCache m_processCache;
    QSignalMapper *m_terminationMapper = nullptr;
};

} // namespace Internal
} // namespace QmakeProjectManager

#endif // EXTERNALEDITORS_H
