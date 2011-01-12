/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
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
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef COREPLUGIN_H
#define COREPLUGIN_H

#include <extensionsystem/iplugin.h>

namespace Core {
class DesignMode;
namespace Internal {

class EditMode;
class MainWindow;

class CorePlugin : public ExtensionSystem::IPlugin
{
    Q_OBJECT

public:
    CorePlugin();
    ~CorePlugin();

    virtual bool initialize(const QStringList &arguments, QString *errorMessage = 0);
    virtual void extensionsInitialized();
    virtual ShutdownFlag aboutToShutdown();
    virtual void remoteCommand(const QStringList & /* options */, const QStringList &args);

public slots:
    void fileOpenRequest(const QString&);

private:
    void parseArguments(const QStringList & arguments);

    MainWindow *m_mainWindow;
    EditMode *m_editMode;
    DesignMode *m_designMode;
};

} // namespace Internal
} // namespace Core

#endif // COREPLUGIN_H
