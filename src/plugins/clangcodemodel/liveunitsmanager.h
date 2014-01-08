/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef LIVEUNITSMANAGER_H
#define LIVEUNITSMANAGER_H

#include "unit.h"

#include <coreplugin/editormanager/ieditor.h>

#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QHash>

namespace ClangCodeModel {
namespace Internal {

class LiveUnitsManager : public QObject
{
    Q_OBJECT

public:
    LiveUnitsManager();
    ~LiveUnitsManager();
    static LiveUnitsManager *instance()
    { return m_instance; }

    void requestTracking(const QString &fileName);
    bool isTracking(const QString &fileName) const
    { return m_units.contains(fileName); }

    void cancelTrackingRequest(const QString &fileName);

    void updateUnit(const QString &fileName, const Unit &unit);
    Unit unit(const QString &fileName);

public slots:
    void editorOpened(Core::IEditor *editor);
    void editorAboutToClose(Core::IEditor *editor);

signals:
    void unitAvailable(const ClangCodeModel::Internal::Unit &unit);

private:
    static LiveUnitsManager *m_instance;
    QHash<QString, Unit> m_units;
};

} // Internal
} // ClangCodeModel

#endif // LIVEUNITSMANAGER_H
