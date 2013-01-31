/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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

#ifndef IPROJECTPROPERTIES_H
#define IPROJECTPROPERTIES_H

#include "projectexplorer_export.h"

#include <QObject>
#include <QIcon>
#include <QWidget>

namespace ProjectExplorer {
class Project;
class Target;

namespace Constants {
    const int PANEL_LEFT_MARGIN = 70;
}

class PROJECTEXPLORER_EXPORT PropertiesPanel
{
    Q_DISABLE_COPY(PropertiesPanel)

public:
    PropertiesPanel() {}
    ~PropertiesPanel() { delete m_widget; }

    QString displayName() const { return m_displayName; }
    QIcon icon() const { return m_icon; }
    QWidget *widget() const { return m_widget; }

    void setDisplayName(const QString &name) { m_displayName = name; }
    void setIcon(const QIcon &icon) { m_icon = icon; }
    void setWidget(QWidget *widget) { m_widget = widget; }

private:
    QString m_displayName;
    QWidget *m_widget;
    QIcon m_icon;
};

class PROJECTEXPLORER_EXPORT IPanelFactory : public QObject
{
    Q_OBJECT
public:
    virtual QString id() const = 0;
    virtual QString displayName() const = 0;
    virtual int priority() const = 0;
    static bool prioritySort(IPanelFactory *a, IPanelFactory *b)
    { return (a->priority() == b->priority() && a->id() < b->id())
                || a->priority() < b->priority(); }
};

class PROJECTEXPLORER_EXPORT IProjectPanelFactory : public IPanelFactory
{
    Q_OBJECT
public:
    virtual bool supports(Project *project) = 0;
    virtual PropertiesPanel *createPanel(Project *project) = 0;
};

class PROJECTEXPLORER_EXPORT ITargetPanelFactory : public IPanelFactory
{
    Q_OBJECT
public:
    virtual bool supports(Target *target) = 0;
    virtual PropertiesPanel *createPanel(Target *target) = 0;
};

} // namespace ProjectExplorer

#endif // IPROJECTPROPERTIES_H
