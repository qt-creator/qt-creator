/**************************************************************************
**
** Copyright (c) 2014 Bojan Petrovic
** Copyright (c) 2014 Radovan Zivkovic
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
#ifndef VCPROJECTMANAGER_INTERNAL_MENUHANDLER_H
#define VCPROJECTMANAGER_INTERNAL_MENUHANDLER_H

#include <QObject>

class QAction;

namespace VcProjectManager {
namespace Internal {

class MenuHandler : public QObject
{
    Q_OBJECT

    friend class VcProjectManagerPlugin;

public:
    static MenuHandler *instance();
    ~MenuHandler();

private:
    MenuHandler();
    void initialize();
    void initialize2005();

private slots:
    void onShowProjectSettings();
    void onAddFolder();
    void onAddFilter();
    void onRemoveFilter();
    void onRemoveFolder();
    void onShowFileSettings();

private:
    static MenuHandler *m_instance;
    QAction *m_projectProperties;
    QAction *m_addFilter;
    QAction *m_removeFilter;
    QAction *m_fileProperties;

    QAction *m_projectProperties2005;
    QAction *m_addFolder2005;
    QAction *m_addFilter2005;
    QAction *m_removeFolder2005;
    QAction *m_removeFilter2005;
    QAction *m_fileProperties2005;
};

} // namespace Internal
} // namespace VcProjectManager

#endif // VCPROJECTMANAGER_INTERNAL_MENUHANDLER_H
