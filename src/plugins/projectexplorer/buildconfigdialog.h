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

#ifndef BUILDCONFIGDIALOG_H
#define BUILDCONFIGDIALOG_H

#include <QtGui/QDialog>

QT_BEGIN_NAMESPACE
class QAction;
class QComboBox;
QT_END_NAMESPACE

namespace ProjectExplorer {
class Project;
class BuildConfiguration;

namespace Internal {

class BuildConfigDialog : public QDialog
{
    Q_OBJECT
public:
    enum DialogResult {
        ChangeBuild = 10,
        Cancel = 11,
        Continue = 12
    };
    explicit BuildConfigDialog(Project *project, QWidget *parent = 0);

    BuildConfiguration *selectedBuildConfiguration() const;

private slots:
    void buttonClicked();

private:
    Project *m_project;
    QPushButton *m_changeBuildConfiguration;
    QPushButton *m_cancel;
    QPushButton *m_justContinue;
    QComboBox *m_configCombo;
};

} // namespace Internal
} // namespace ProjectExplorer

#endif // BUILDCONFIGDIALOG_H
