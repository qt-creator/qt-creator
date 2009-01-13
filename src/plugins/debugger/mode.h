/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
**
** Non-Open Source Usage
**
** Licensees may use this file in accordance with the Qt Beta Version
** License Agreement, Agreement version 2.2 provided with the Software or,
** alternatively, in accordance with the terms contained in a written
** agreement between you and Nokia.
**
** GNU General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU General
** Public License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the packaging
** of this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
**
** http://www.fsf.org/licensing/licenses/info/GPLv2.html and
** http://www.gnu.org/copyleft/gpl.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt GPL Exception
** version 1.3, included in the file GPL_EXCEPTION.txt in this package.
**
***************************************************************************/

#ifndef DEBUGGER_DEBUGMODE_H
#define DEBUGGER_DEBUGMODE_H

#include <coreplugin/basemode.h>

#include <QtCore/QList>
#include <QtCore/QPointer>

QT_BEGIN_NAMESPACE
class QAction;
class QDockWidget;
class QMainWindow;
class QSettings;
class QSplitter;
class QToolBar;
class QWidget;
QT_END_NAMESPACE

namespace Debugger {
namespace Internal {

class DebuggerManager;

class DebugMode : public Core::BaseMode
{
    Q_OBJECT

public:
    DebugMode(DebuggerManager *manager, QObject *parent = 0);
    ~DebugMode();

    // IMode
    void activated();
    void shutdown();
    static QSettings *settings();

private slots:
    void focusCurrentEditor(Core::IMode *mode);

private:
    QToolBar *createToolBar();
    void writeSettings() const;
    void readSettings();

    QPointer<DebuggerManager> m_manager;
};

} // namespace Internal
} // namespace Debugger

#endif // DEBUGGER_DEBUGMODE_H
