/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008-2009 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef DEPENDENCIESDIALOG_H
#define DEPENDENCIESDIALOG_H

#include "ui_dependenciesdialog.h"

#include <QtGui/QDialog>

namespace ProjectExplorer {

class Project;
class SessionManager;

namespace Internal {

class DependencyModel;

// NBS kill DependenciesDialog?
class DependenciesDialog : public QDialog
{
    Q_OBJECT
public:
    typedef QList<ProjectExplorer::Project *> ProjectList;

    DependenciesDialog(QWidget *parent, SessionManager *sln);
    virtual ~DependenciesDialog();

public slots:
    virtual void accept();
    void reset();

private:
    Ui::DependenciesDialog m_ui;
    SessionManager *m_sln;
    ProjectList m_projectList;
    DependencyModel *m_model;
};

} // namespace Internal
} // namespace ProjectExplorer

#endif // DEPENDENCIESDIALOG_H
