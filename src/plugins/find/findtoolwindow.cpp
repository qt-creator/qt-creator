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

#include "findtoolwindow.h"
#include "findplugin.h"

#include <coreplugin/icore.h>

#include <QtCore/QSettings>
#include <QtGui/QMainWindow>
#include <QtGui/QStringListModel>
#include <QtGui/QCompleter>
#include <QtGui/QKeyEvent>

using namespace Find;
using namespace Find::Internal;

FindToolWindow::FindToolWindow(FindPlugin *plugin)
    : QDialog(Core::ICore::instance()->mainWindow()),
    m_plugin(plugin),
    m_findCompleter(new QCompleter(this)),
    m_currentFilter(0),
    m_configWidget(0)
{
    m_ui.setupUi(this);
    connect(m_ui.closeButton, SIGNAL(clicked()), this, SLOT(reject()));
    connect(m_ui.searchButton, SIGNAL(clicked()), this, SLOT(search()));
    connect(m_ui.replaceButton, SIGNAL(clicked()), this, SLOT(replace()));
    connect(m_ui.cancelButton, SIGNAL(clicked()), this, SLOT(cancelSearch()));
    connect(m_ui.matchCase, SIGNAL(toggled(bool)), m_plugin, SLOT(setCaseSensitive(bool)));
    connect(m_ui.wholeWords, SIGNAL(toggled(bool)), m_plugin, SLOT(setWholeWord(bool)));
    connect(m_ui.regExp, SIGNAL(toggled(bool)), m_plugin, SLOT(setRegularExpression(bool)));
    connect(m_ui.filterList, SIGNAL(activated(int)), this, SLOT(setCurrentFilter(int)));
    connect(m_ui.searchTerm, SIGNAL(textChanged(QString)), this, SLOT(updateButtonStates()));

    m_findCompleter->setModel(m_plugin->findCompletionModel());
    m_ui.searchTerm->setCompleter(m_findCompleter);
    m_ui.searchTerm->installEventFilter(this);
    QVBoxLayout *layout = new QVBoxLayout;
    layout->setMargin(0);
    layout->setSpacing(0);
    m_ui.configWidget->setLayout(layout);
    updateButtonStates();
}

FindToolWindow::~FindToolWindow()
{
    qDeleteAll(m_configWidgets);
}

bool FindToolWindow::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == m_ui.searchTerm && event->type() == QEvent::KeyPress) {
        QKeyEvent *ke = static_cast<QKeyEvent *>(event);
        if (ke->key() == Qt::Key_Down) {
            m_findCompleter->complete();
        }
    }
    return QDialog::eventFilter(obj, event);
}

void FindToolWindow::updateButtonStates()
{
    bool filterEnabled = m_currentFilter && m_currentFilter->isEnabled();
    bool enabled = !m_ui.searchTerm->text().isEmpty() && filterEnabled;
    m_ui.searchButton->setEnabled(enabled);
    m_ui.replaceButton->setEnabled(m_currentFilter
                                   && m_currentFilter->isReplaceSupported() && enabled);
    if (m_configWidget)
        m_configWidget->setEnabled(filterEnabled);

    m_ui.matchCase->setEnabled(filterEnabled
                               && (m_currentFilter->supportedFindFlags() & Find::FindCaseSensitively));
    m_ui.wholeWords->setEnabled(filterEnabled
                                && (m_currentFilter->supportedFindFlags() & Find::FindWholeWords));
    m_ui.regExp->setEnabled(filterEnabled
                            && (m_currentFilter->supportedFindFlags() & Find::FindRegularExpression));
    m_ui.searchTerm->setEnabled(filterEnabled);
    m_ui.cancelButton->setEnabled(m_currentFilter && m_currentFilter->canCancel());
}

void FindToolWindow::setFindFilters(const QList<IFindFilter *> &filters)
{
    qDeleteAll(m_configWidgets);
    m_configWidgets.clear();
    m_filters = filters;
    m_ui.filterList->clear();
    QStringList names;
    foreach (IFindFilter *filter, filters) {
        names << filter->displayName();
        m_configWidgets.append(filter->createConfigWidget());
    }
    m_ui.filterList->addItems(names);
    if (m_filters.size() > 0)
        setCurrentFilter(0);
}

void FindToolWindow::setFindText(const QString &text)
{
    m_ui.searchTerm->setText(text);
}

void FindToolWindow::open(IFindFilter *filter)
{
    if (!filter)
        filter = m_currentFilter;
    int index = m_filters.indexOf(filter);
    if (index >= 0) {
        setCurrentFilter(index);
    }
    m_ui.matchCase->setChecked(m_plugin->hasFindFlag(Find::FindCaseSensitively));
    m_ui.wholeWords->setChecked(m_plugin->hasFindFlag(Find::FindWholeWords));
    m_ui.regExp->setChecked(m_plugin->hasFindFlag(Find::FindRegularExpression));

    m_ui.searchTerm->setFocus();
    m_ui.searchTerm->selectAll();
    exec();
}

void FindToolWindow::setCurrentFilter(int index)
{
    m_ui.filterList->setCurrentIndex(index);
    for (int i = 0; i < m_configWidgets.size(); ++i) {
        QWidget *configWidget = m_configWidgets.at(i);
        if (i == index) {
            m_configWidget = configWidget;
            if (m_currentFilter)
                disconnect(m_currentFilter, SIGNAL(changed()), this, SLOT(updateButtonStates()));
            m_currentFilter = m_filters.at(i);
            connect(m_currentFilter, SIGNAL(changed()), this, SLOT(updateButtonStates()));
            updateButtonStates();
            if (m_configWidget) {
                m_ui.configWidget->layout()->addWidget(m_configWidget);
            }
        } else {
            if (configWidget)
                configWidget->setParent(0);
        }
    }
}

void FindToolWindow::acceptAndGetParameters(QString *term, IFindFilter **filter)
{
    if (filter)
        *filter = 0;
    accept();
    m_plugin->updateFindCompletion(m_ui.searchTerm->text());
    int index = m_ui.filterList->currentIndex();
    QString searchTerm = m_ui.searchTerm->text();
    if (term)
        *term = searchTerm;
    if (searchTerm.isEmpty() || index < 0)
        return;
    if (filter)
        *filter = m_filters.at(index);
}

void FindToolWindow::search()
{
    QString term;
    IFindFilter *filter;
    acceptAndGetParameters(&term, &filter);
    if (filter)
        filter->findAll(term, m_plugin->findFlags());
}

void FindToolWindow::replace()
{
    QString term;
    IFindFilter *filter;
    acceptAndGetParameters(&term, &filter);
    filter->replaceAll(term, m_plugin->findFlags());
}

void FindToolWindow::cancelSearch()
{
    if (m_currentFilter)
        m_currentFilter->cancel();
}

void FindToolWindow::writeSettings()
{
    QSettings *settings = Core::ICore::instance()->settings();
    settings->beginGroup("Find");
    settings->setValue("CurrentFilter", m_currentFilter ? m_currentFilter->id() : 0);
    foreach (IFindFilter *filter, m_filters)
        filter->writeSettings(settings);
    settings->endGroup();
}

void FindToolWindow::readSettings()
{
    QSettings *settings = Core::ICore::instance()->settings();
    settings->beginGroup("Find");
    const QString currentFilter = settings->value("CurrentFilter").toString();
    for (int i = 0; i < m_filters.size(); ++i) {
        IFindFilter *filter = m_filters.at(i);
        filter->readSettings(settings);
        if (filter->id() == currentFilter) {
            setCurrentFilter(i);
        }
    }
    settings->endGroup();
}
