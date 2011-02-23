/***************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the documentation of Qt Creator.
**
** You may use this file under the terms of the BSD license as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of Nokia Corporation and its Subsidiary(-ies) nor
**     the names of its contributors may be used to endorse or promote
**     products derived from this software without specific prior written
**     permission.
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
****************************************************************************/

#include "loggermode.h"
#include "loggermodewidget.h"

#include <extensionsystem/pluginmanager.h>
#include <coreplugin/icore.h>
#include <coreplugin/coreconstants.h>
#include <utils/styledbar.h>
#include <utils/iwelcomepage.h>
#include <coreplugin/uniqueidmanager.h>
#include <extensionsystem/pluginmanager.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/session.h>
#include <projectexplorer/project.h>

#include <QtCore/QSettings>

#include <QStackedWidget>
#include <QLabel>
#include <QComboBox>
#include <QVBoxLayout>
#include <QPushbutton>
#include <QMessageBox>



struct LoggerModeData
{
   QWidget *m_widget;
   QLabel *currentProjectsLabel;
   QLabel *addProjectLabel;
   QComboBox *currentProjectsCombobox;
   QComboBox *addProjectComboBox;
   QPushButton *addToProjectButton;
   QStackedWidget *stackedWidget;
};

LoggerMode::LoggerMode()
{
    d = new LoggerModeData;
    d->m_widget = new QWidget;

    d->currentProjectsLabel = new QLabel("Current projects :");
    d->currentProjectsLabel->setFixedWidth(90);
    d->currentProjectsCombobox = new QComboBox;
    d->currentProjectsCombobox->setSizePolicy(QSizePolicy::Preferred,
                                              QSizePolicy::Preferred);

    d->addProjectLabel = new QLabel("Add Project :");
    d->addProjectLabel->setAlignment(Qt::AlignRight);
    d->addProjectComboBox = new QComboBox;
    d->addProjectComboBox->setSizePolicy(QSizePolicy::Preferred,
                                         QSizePolicy::Preferred);
    d->addProjectComboBox->setEditable(true);

    d->addToProjectButton = new QPushButton(tr("Add Project"));
    d->addToProjectButton->setFixedWidth(80);


    QHBoxLayout *hLayout = new QHBoxLayout;
    hLayout->addWidget(d->currentProjectsLabel);
    hLayout->addWidget(d->currentProjectsCombobox);
    hLayout->addWidget(d->addProjectLabel);
    hLayout->addWidget(d->addProjectComboBox);
    hLayout->addWidget(d->addToProjectButton);



    d->stackedWidget = new QStackedWidget;

    QVBoxLayout* layout = new QVBoxLayout;
    layout->addLayout(hLayout);
    layout->addWidget(d->stackedWidget);

    d->m_widget->setLayout(layout);

    d->addProjectComboBox->addItem("Project 1");
    d->addProjectComboBox->addItem("Project 2");
    d->addProjectComboBox->addItem("Project 3");

    connect(d->addToProjectButton,SIGNAL(clicked()),
            this,SLOT(addItem()));

    connect(d->currentProjectsCombobox, SIGNAL(currentIndexChanged(int)),
            d->stackedWidget, SLOT(setCurrentIndex(int)));
}


LoggerMode::~LoggerMode()
{
    delete d->m_widget;
    delete d;
}

void LoggerMode::addItem()
{
    d->currentProjectsCombobox->addItem(d->addProjectComboBox->currentText());
    addNewStackWidgetPage(d->currentProjectsCombobox->itemText(0));
    d->addProjectComboBox->removeItem(d->addProjectComboBox->currentIndex());
}

QString LoggerMode::name() const
{
    return tr("LoggerMode");
}


QIcon LoggerMode::icon() const
{
    return QIcon(QLatin1String(":/core/images/qtcreator_logo_32.png"));
}


int LoggerMode::priority() const
{
    return 0;
}


QWidget* LoggerMode::widget()
{
    return d->m_widget;
}


const char* LoggerMode::uniqueModeName() const
{
    return "LoggerMode" ;
}

QList<int> LoggerMode::context() const
{
     return QList<int>();
}

void LoggerMode::addNewStackWidgetPage(const QString projectName)
{
    d->stackedWidget->addWidget(new LoggerModeWidget(projectName));
}

