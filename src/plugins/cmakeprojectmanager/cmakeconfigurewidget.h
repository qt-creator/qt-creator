/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
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
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#ifndef CMAKECONFIGUREWIDGET_H
#define CMAKECONFIGUREWIDGET_H

#include "ui_cmakeconfigurewidget.h"
#include <QtGui/QWidget>
#include <QtGui/QDialog>

namespace CMakeProjectManager {
namespace Internal {

class CMakeManager;

class CMakeConfigureWidget : public QWidget
{
    Q_OBJECT
public:
    CMakeConfigureWidget(QWidget *parent, CMakeManager *manager, const QString &sourceDirectory);
    Ui::CMakeConfigureWidget m_ui;

    QString buildDirectory();
    QStringList arguments();
    bool configureSucceded();

private slots:
    void runCMake();
private:
    bool m_configureSucceded;
    CMakeManager *m_cmakeManager;
    QString m_sourceDirectory;
};

class CMakeConfigureDialog : public QDialog
{
public:
    CMakeConfigureDialog(QWidget *parent, CMakeManager *manager, const QString &sourceDirectory);

    QString buildDirectory();
    QStringList arguments();
    bool configureSucceded();

private:
    CMakeConfigureWidget *m_cmakeConfigureWidget;
};

}
}

#endif // CMAKECONFIGUREWIDGET_H
