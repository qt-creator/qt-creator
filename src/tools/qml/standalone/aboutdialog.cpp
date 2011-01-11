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

#include "aboutdialog.h"
#include "integrationcore.h"
#include "pluginmanager.h"

#include <QtCore/QDebug>
#include <QtCore/QUrl>

#include <QtGui/QApplication>
#include <QtGui/QGridLayout>
#include <QtGui/QTextEdit>
#include <QtGui/QPushButton>


static QString aboutText = "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0//EN\" \"http://www.w3.org/TR/REC-html40/strict.dtd\">"
                           "<html><head><style type=\"text/css\">\np, li { white-space: pre-wrap; }\n</style></head><body style=\"font-family:'MS Shell Dlg 2'; font-size:8.25pt; font-weight:400; font-style:normal;\">"
                           "<p align=\"center\"><img src=\"logo\"/></p>"
                           "<h2 align=\"center\">Bauhaus</h2>"
                           "</body></html>";

AboutDialog::AboutDialog(QWidget* parent):
        QDialog(parent)
{
    setWindowFlags(windowFlags() | Qt::CustomizeWindowHint | Qt::WindowCloseButtonHint);
    setStyleSheet(QString("background-color: #FFFFFF;"));

    QGridLayout* dialogLayout = new QGridLayout;
    setLayout(dialogLayout);

    QTextEdit* textArea = new QTextEdit(this);
    textArea->setReadOnly(true);
    textArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    textArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    textArea->setFrameShape(QFrame::NoFrame);
    textArea->setLineWidth(0);
    QImage logoImage = QImage(QString(":/128xBauhaus_Logo.png"));
    textArea->document()->addResource(QTextDocument::ImageResource, QUrl("logo"), logoImage);
    textArea->setHtml(aboutText);
    dialogLayout->addWidget(textArea, 0, 0, 1, 3);

    QPushButton* aboutPluginsButton = new QPushButton("About Plug-ins...", this);
    dialogLayout->addWidget(aboutPluginsButton, 1, 0, 1, 1);
    connect(aboutPluginsButton, SIGNAL(clicked()), this, SLOT(doAboutPlugins()));

    QPushButton* aboutQtButton = new QPushButton("About Qt...", this);
    dialogLayout->addWidget(aboutQtButton, 1, 1, 1, 1);
    connect(aboutQtButton, SIGNAL(clicked()), qApp, SLOT(aboutQt()));

    QPushButton* closeButton = new QPushButton("Close", this);
    dialogLayout->addWidget(closeButton, 1, 2, 1, 1);
    connect(closeButton, SIGNAL(clicked()), this, SLOT(close()));
}

void AboutDialog::go(QWidget* parent)
{
    AboutDialog dialog(parent);
    dialog.setWindowTitle(tr("About Bauhaus", "AboutDialog"));
    dialog.exec();
}

void AboutDialog::doAboutPlugins()
{
    QmlDesigner::IntegrationCore *core = QmlDesigner::IntegrationCore::instance();
    QDialog* dialog = core->pluginManager()->createAboutPluginDialog(this);
    dialog->setWindowFlags(Qt::Sheet);
    dialog->exec();
}
