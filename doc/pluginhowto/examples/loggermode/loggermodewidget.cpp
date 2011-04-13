/***************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (info@qt.nokia.com)
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

#include "loggermodewidget.h"
#include<QTableWidget>
#include <QFileDialog>
#include<QLabel>
#include<QGroupBox>
#include<QComboBox>
#include<QPushButton>
#include<QLineEdit>
#include<QTime>
#include<QTimer>
#include<QTextEdit>
#include<QCalendarWidget>
#include<QComboBox>
#include<QGridLayout>
#include<QMessageBox>
#include<QApplication>
#include<QTextStream>
#include<QCloseEvent>

struct LoggerModeWidgetData
{
    QLabel *progressLabel;
    QLabel *hoursWorkedLabel;
    QLabel *dateLabel;
    QLabel *descriptionLabel;
    QCalendarWidget *calendar;
    QComboBox *progressComboBox;
    QLineEdit *hoursWorkedLineEdit;
    QPushButton *startTimerButton;
    QPushButton *stopTimerButton;
    QPushButton *saveButton;
    QTimer *timer;
    QTextEdit *textEdit;
    QString projectName;
    int totalTime;
};

LoggerModeWidget::LoggerModeWidget(const QString projectName, QWidget* parent)
    :QWidget(parent)
{
    d = new LoggerModeWidgetData;
    d->projectName = projectName;
    d->totalTime = 0;
    /*
        // Catch hold of the plugin-manager
        ExtensionSystem::PluginManager* pm = ExtensionSystem::PluginManager::instance();

        // Look for the ProjectExplorerPlugin object
        ProjectExplorer::ProjectExplorerPlugin* projectExplorerPlugin
        = pm->getObject<ProjectExplorer::ProjectExplorerPlugin>();

        // Fetch a list of all open projects
        QList<ProjectExplorer::Project*> projects =projectExplorerPlugin->session()->projects();
        Q_FOREACH(ProjectExplorer::Project* project, projects)
             d->projectExplorerCombo->addItem( project->name());
    */
    QStringList percentList;
    percentList <<"10%" <<"20%" <<"30%" <<"40%" <<"50%"
                <<"60%" <<"70%" <<"80%" <<"90%" <<"100%" ;
    d->progressLabel = new QLabel("Progress:");
    d->hoursWorkedLabel = new QLabel("Hours Worked:");
    d->dateLabel = new QLabel("Date:");
    d->descriptionLabel = new QLabel("Description :");
    d->hoursWorkedLineEdit = new QLineEdit;
    d->hoursWorkedLineEdit->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    d->progressComboBox = new QComboBox;
    d->progressComboBox->addItems(percentList);
    d->progressComboBox->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    d->startTimerButton = new QPushButton(tr("Start Timer"));
    d->startTimerButton->setFixedWidth(80);
    d->stopTimerButton = new QPushButton(tr("Pause Timer"));
    d->stopTimerButton->setFixedWidth(80);
    d->stopTimerButton->setCheckable(true);
    d->textEdit = new QTextEdit(this);
    d->textEdit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    d->calendar = new QCalendarWidget;
    d->saveButton = new QPushButton(tr("Save To File"));
    d->saveButton->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);

    QGroupBox *timeLoggerBox = new QGroupBox(tr("Time Logger"));

    QGridLayout *gLayout = new QGridLayout;
    gLayout->addWidget(d->dateLabel, 0, 0, 1, 1);
    gLayout->addWidget(d->calendar, 1, 0, 1, 3);
    gLayout->addWidget(d->progressLabel, 2, 0, 1, 1);
    gLayout->addWidget(d->progressComboBox, 2, 1, 1, 1);
    gLayout->addWidget(d->hoursWorkedLabel, 3, 0, 1, 1);
    gLayout->addWidget(d->hoursWorkedLineEdit, 3, 1, 1, 1);
    gLayout->addWidget(d->startTimerButton, 4, 1, 1, 1);
    gLayout->addWidget(d->stopTimerButton, 4, 2, 1, 1);
    timeLoggerBox->setLayout(gLayout);

    d->timer = new QTimer(this);
   //d->time.setHMS(0,0,0);

    // connection of SIGNALS and SLOTS

    connect(d->timer, SIGNAL(timeout()), this, SLOT(updateTime()));
    connect(d->startTimerButton,SIGNAL(clicked()),this,SLOT(startTimeLog()));
    connect(d->stopTimerButton,SIGNAL(clicked()),this,SLOT(endTimeLog()));
    connect(d->saveButton, SIGNAL(clicked()), this, SLOT(saveToFile()));


    QVBoxLayout *vLayout = new QVBoxLayout;
    vLayout->addWidget(d->descriptionLabel);
    vLayout->addWidget(d->textEdit);

    QHBoxLayout * hLayout = new QHBoxLayout;
    hLayout->addWidget(timeLoggerBox);
    hLayout->addLayout(vLayout);

    QHBoxLayout *bLayout = new QHBoxLayout;
    bLayout->addStretch(1);
    bLayout->addWidget(d->saveButton);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->addLayout(hLayout);
    mainLayout->addLayout(bLayout);
    mainLayout->addStretch(1);

}

LoggerModeWidget::~LoggerModeWidget()
{
    delete d;
}

bool LoggerModeWidget::saveToFile()
{
    QString fileName = QFileDialog::getSaveFileName(this);
    if (fileName.isEmpty())
        return false;

    QFile file(fileName);
    if (!file.open(QFile::WriteOnly | QFile::Text)) {
        QMessageBox::critical(this, tr("Application"),
                             tr("Unable to open file %1 for writing :\n%2.")
                             .arg(fileName)
                             .arg(file.errorString()));
        return false;
    }

    QTextStream out(&file);

#ifndef QT_NO_CURSOR
    QApplication::setOverrideCursor(Qt::WaitCursor);
#endif
    out << "Project name : " << d->projectName << "\n";
    out << "Date         : " << d->calendar->selectedDate().toString() << "\n";
    out << "Progress     : " << d->progressComboBox->currentText() << "\n";
    out << "Duration     : " << d->hoursWorkedLineEdit->text() << "\n\n";
    out << "Description  : " << d->textEdit->toPlainText();
#ifndef QT_NO_CURSOR
    QApplication::restoreOverrideCursor();
#endif

    return true;
}

void LoggerModeWidget::startTimeLog()
{
    d->totalTime = 0;
    d->timer->start(1000);
}

void LoggerModeWidget::endTimeLog()
{
    if(d->stopTimerButton->isChecked())
    {
        d->stopTimerButton->setText("Continue Timer");
        d->timer->stop();
    }
    else
    {
        d->stopTimerButton->setText("Pause Timer");
        d->timer->start(1000);
    }
}

void LoggerModeWidget::updateTime()
{
    d->totalTime++;
    QTime time(0,0,0);
    time = time.addSecs(d->totalTime);
    d->hoursWorkedLineEdit->setText(time.toString());
}
/*
void LoggerModeWidget::setProjectName(QString name)
{
    d->projectName = name;
}
*/
