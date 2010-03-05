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
