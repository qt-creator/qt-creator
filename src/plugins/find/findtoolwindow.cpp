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

#include "findtoolwindow.h"
#include "findplugin.h"

#include <coreplugin/icore.h>

#include <QtGui/QMainWindow>

using namespace Find;
using namespace Find::Internal;

FindToolWindow::FindToolWindow(FindPlugin *plugin)
    : QDialog(Core::ICore::instance()->mainWindow()),
    m_plugin(plugin),
    m_findCompleter(new QCompleter(this))
{
    m_ui.setupUi(this);
    connect(m_ui.closeButton, SIGNAL(clicked()), this, SLOT(reject()));
    connect(m_ui.searchButton, SIGNAL(clicked()), this, SLOT(accept()));
    connect(m_ui.matchCase, SIGNAL(toggled(bool)), m_plugin, SLOT(setCaseSensitive(bool)));
    connect(m_ui.wholeWords, SIGNAL(toggled(bool)), m_plugin, SLOT(setWholeWord(bool)));
    connect(m_ui.filterList, SIGNAL(currentIndexChanged(int)), this, SLOT(setCurrentFilter(int)));
    connect(this, SIGNAL(accepted()), this, SLOT(search()));
    m_findCompleter->setModel(m_plugin->findCompletionModel());
    m_ui.searchTerm->setCompleter(m_findCompleter);
    QVBoxLayout *layout = new QVBoxLayout;
    layout->setMargin(0);
    layout->setSpacing(0);
    m_ui.configWidget->setLayout(layout);
}

FindToolWindow::~FindToolWindow()
{
    qDeleteAll(m_configWidgets);
}

void FindToolWindow::setFindFilters(const QList<IFindFilter *> &filters)
{
    qDeleteAll(m_configWidgets);
    m_configWidgets.clear();
    m_filters = filters;
    m_ui.filterList->clear();
    QStringList names;
    foreach (IFindFilter *filter, filters) {
        names << filter->name();
        m_configWidgets.append(filter->createConfigWidget());
    }
    m_ui.filterList->addItems(names);
}

void FindToolWindow::setFindText(const QString &text)
{
    m_ui.searchTerm->setText(text);
}

void FindToolWindow::open(IFindFilter *filter)
{
    int index = m_filters.indexOf(filter);
    if (index >= 0) {
        m_ui.filterList->setCurrentIndex(index);
    }
    m_ui.matchCase->setChecked(m_plugin->findFlags() & QTextDocument::FindCaseSensitively);
    m_ui.wholeWords->setChecked(m_plugin->findFlags() & QTextDocument::FindWholeWords);
    m_ui.searchTerm->setFocus();
    m_ui.searchTerm->selectAll();
    exec();
}

void FindToolWindow::setCurrentFilter(int index)
{
    for (int i = 0; i < m_configWidgets.size(); ++i) {
        QWidget *configWidget = m_configWidgets.at(i);
        if (!configWidget)
            continue;
        if (i == index) {
            m_ui.configWidget->layout()->addWidget(configWidget);
            bool enabled = m_filters.at(i)->isEnabled();
            m_ui.matchCase->setEnabled(enabled);
            m_ui.wholeWords->setEnabled(enabled);
            m_ui.searchTerm->setEnabled(enabled);
            m_ui.searchButton->setEnabled(enabled);
            configWidget->setEnabled(enabled);
        } else {
            configWidget->setParent(0);
        }
    }
}

void FindToolWindow::search()
{
    m_plugin->updateFindCompletion(m_ui.searchTerm->text());
    int index = m_ui.filterList->currentIndex();
    QString term = m_ui.searchTerm->text();
    if (term.isEmpty() || index < 0)
        return;
    IFindFilter *filter = m_filters.at(index);
    filter->findAll(term, m_plugin->findFlags());
}

void FindToolWindow::writeSettings()
{
    QSettings *settings = Core::ICore::instance()->settings();
    settings->beginGroup("Find");
    foreach (IFindFilter *filter, m_filters)
        filter->writeSettings(settings);
    settings->endGroup();
}

void FindToolWindow::readSettings()
{
    QSettings *settings = Core::ICore::instance()->settings();
    settings->beginGroup("Find");
    foreach (IFindFilter *filter, m_filters)
        filter->readSettings(settings);
    settings->endGroup();
}
