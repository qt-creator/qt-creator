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

#ifndef DESIGNEREXTERNALEDITOR_H
#define DESIGNEREXTERNALEDITOR_H

#include <coreplugin/editormanager/iexternaleditor.h>

#include <QtCore/QStringList>
#include <QtCore/QMap>

QT_BEGIN_NAMESPACE
class QProcess;
class QTcpSocket;
class QSignalMapper;
QT_END_NAMESPACE

namespace Qt4ProjectManager {

class QtVersion;

namespace Internal {

/* Convience parametrizable base class for Qt editors/binaries
 * Provides convenience methods that
 * try to retrieve the binary of the editor from the Qt version
 * of the project the file belongs to, falling back to path search
 * if none is found. On Mac, the "open" mechanism can be optionally be used. */

class ExternalQtEditor : public Core::IExternalEditor
{
    Q_OBJECT
public:
    virtual QStringList mimeTypes() const;
    virtual QString id() const;
    virtual QString displayName() const;

protected:
    // Method pointer for a QtVersion method return a string (command)
    typedef QString (QtVersion::*QtVersionCommandAccessor)() const;

    // Data required to launch the editor
    struct EditorLaunchData {
        QString binary;
        QStringList arguments;
        QString workingDirectory;
    };

    explicit ExternalQtEditor(const QString &id,
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
    const QString m_id;
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
    QSignalMapper *m_terminationMapper;
};

} // namespace Internal
} // namespace Qt4ProjectManager

#endif // DESIGNEREXTERNALEDITOR_H
