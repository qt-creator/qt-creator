/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "findtoolwindow.h"
#include "ifindfilter.h"
#include "findplugin.h"

#include <coreplugin/icore.h>
#include <utils/qtcassert.h>
#include <utils/algorithm.h>

#include <QSettings>
#include <QStringListModel>
#include <QCompleter>
#include <QKeyEvent>
#include <QRegularExpression>
#include <QScrollArea>

using namespace Core;
using namespace Core::Internal;

static FindToolWindow *m_instance = 0;

static bool validateRegExp(Utils::FancyLineEdit *edit, QString *errorMessage)
{
    if (edit->text().isEmpty()) {
        if (errorMessage)
            *errorMessage = FindToolWindow::tr("Empty search term");
        return false;
    }
    if (Find::hasFindFlag(FindRegularExpression)) {
        QRegularExpression regexp(edit->text());
        bool regexpValid = regexp.isValid();
        if (!regexpValid && errorMessage)
            *errorMessage = regexp.errorString();
        return regexpValid;
    }
    return true;
}

FindToolWindow::FindToolWindow(QWidget *parent)
    : QWidget(parent),
    m_findCompleter(new QCompleter(this)),
    m_currentFilter(0),
    m_configWidget(0)
{
    m_instance = this;
    m_ui.setupUi(this);
    m_ui.searchTerm->setFiltering(true);
    m_ui.searchTerm->setPlaceholderText(QString());
    setFocusProxy(m_ui.searchTerm);

    connect(m_ui.searchButton, &QAbstractButton::clicked, this, &FindToolWindow::search);
    connect(m_ui.replaceButton, &QAbstractButton::clicked, this, &FindToolWindow::replace);
    connect(m_ui.matchCase, &QAbstractButton::toggled, Find::instance(), &Find::setCaseSensitive);
    connect(m_ui.wholeWords, &QAbstractButton::toggled, Find::instance(), &Find::setWholeWord);
    connect(m_ui.regExp, &QAbstractButton::toggled, Find::instance(), &Find::setRegularExpression);
    connect(m_ui.filterList, static_cast<void (QComboBox::*)(int)>(&QComboBox::activated),
            this, static_cast<void (FindToolWindow::*)(int)>(&FindToolWindow::setCurrentFilter));

    m_findCompleter->setModel(Find::findCompletionModel());
    m_ui.searchTerm->setSpecialCompleter(m_findCompleter);
    m_ui.searchTerm->installEventFilter(this);

    m_ui.searchTerm->setValidationFunction(validateRegExp);
    connect(Find::instance(), &Find::findFlagsChanged,
            m_ui.searchTerm, &Utils::FancyLineEdit::validate);
    connect(m_ui.searchTerm, &Utils::FancyLineEdit::validChanged,
            this, &FindToolWindow::updateButtonStates);

    QVBoxLayout *layout = new QVBoxLayout;
    layout->setMargin(0);
    layout->setSpacing(0);
    m_ui.configWidget->setLayout(layout);
    updateButtonStates();

    connect(Find::instance(), &Find::findFlagsChanged, this, &FindToolWindow::updateFindFlags);
}

FindToolWindow::~FindToolWindow()
{
    qDeleteAll(m_configWidgets);
}

FindToolWindow *FindToolWindow::instance()
{
    return m_instance;
}

bool FindToolWindow::event(QEvent *event)
{
    if (event->type() == QEvent::KeyPress) {
        QKeyEvent *ke = static_cast<QKeyEvent *>(event);
        if ((ke->key() == Qt::Key_Return || ke->key() == Qt::Key_Enter)
                && (ke->modifiers() == Qt::NoModifier || ke->modifiers() == Qt::KeypadModifier)) {
            ke->accept();
            if (m_ui.searchButton->isEnabled())
                search();
            return true;
        }
    }
    return QWidget::event(event);
}

bool FindToolWindow::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == m_ui.searchTerm && event->type() == QEvent::KeyPress) {
        QKeyEvent *ke = static_cast<QKeyEvent *>(event);
        if (ke->key() == Qt::Key_Down) {
            if (m_ui.searchTerm->text().isEmpty())
                m_findCompleter->setCompletionPrefix(QString());
            m_findCompleter->complete();
        }
    }
    return QWidget::eventFilter(obj, event);
}

void FindToolWindow::updateButtonStates()
{
    bool filterEnabled = m_currentFilter && m_currentFilter->isEnabled();
    bool enabled = m_ui.searchTerm->isValid() && filterEnabled && m_currentFilter->isValid();
    m_ui.searchButton->setEnabled(enabled);
    m_ui.replaceButton->setEnabled(m_currentFilter
                                   && m_currentFilter->isReplaceSupported() && enabled);
    if (m_configWidget)
        m_configWidget->setEnabled(filterEnabled);

    m_ui.matchCase->setEnabled(filterEnabled
                               && (m_currentFilter->supportedFindFlags() & FindCaseSensitively));
    m_ui.wholeWords->setEnabled(filterEnabled
                                && (m_currentFilter->supportedFindFlags() & FindWholeWords));
    m_ui.regExp->setEnabled(filterEnabled
                            && (m_currentFilter->supportedFindFlags() & FindRegularExpression));
    m_ui.searchTerm->setEnabled(filterEnabled);
}

void FindToolWindow::updateFindFlags()
{
    m_ui.matchCase->setChecked(Find::hasFindFlag(FindCaseSensitively));
    m_ui.wholeWords->setChecked(Find::hasFindFlag(FindWholeWords));
    m_ui.regExp->setChecked(Find::hasFindFlag(FindRegularExpression));
}


void FindToolWindow::setFindFilters(const QList<IFindFilter *> &filters)
{
    qDeleteAll(m_configWidgets);
    m_configWidgets.clear();
    foreach (IFindFilter *filter, m_filters)
        filter->disconnect(this);
    m_filters = filters;
    m_ui.filterList->clear();
    QStringList names;
    foreach (IFindFilter *filter, filters) {
        names << filter->displayName();
        m_configWidgets.append(filter->createConfigWidget());
        connect(filter, &IFindFilter::displayNameChanged,
                this, [this, filter]() { updateFindFilterName(filter); });
    }
    m_ui.filterList->addItems(names);
    if (m_filters.size() > 0)
        setCurrentFilter(0);
}

QList<IFindFilter *> FindToolWindow::findFilters() const
{
    return m_filters;
}

void FindToolWindow::updateFindFilterName(IFindFilter *filter)
{
    int index = m_filters.indexOf(filter);
    if (QTC_GUARD(index >= 0))
        m_ui.filterList->setItemText(index, filter->displayName());
}

void FindToolWindow::setFindText(const QString &text)
{
    m_ui.searchTerm->setText(text);
}

void FindToolWindow::setCurrentFilter(IFindFilter *filter)
{
    if (!filter)
        filter = m_currentFilter;
    int index = m_filters.indexOf(filter);
    if (index >= 0)
        setCurrentFilter(index);
    updateFindFlags();
    m_ui.searchTerm->setFocus();
    m_ui.searchTerm->selectAll();
}

void FindToolWindow::setCurrentFilter(int index)
{
    m_ui.filterList->setCurrentIndex(index);
    for (int i = 0; i < m_configWidgets.size(); ++i) {
        QWidget *configWidget = m_configWidgets.at(i);
        if (i == index) {
            m_configWidget = configWidget;
            if (m_currentFilter) {
                disconnect(m_currentFilter, &IFindFilter::enabledChanged,
                           this, &FindToolWindow::updateButtonStates);
                disconnect(m_currentFilter, &IFindFilter::validChanged,
                           this, &FindToolWindow::updateButtonStates);
            }
            m_currentFilter = m_filters.at(i);
            connect(m_currentFilter, &IFindFilter::enabledChanged,
                    this, &FindToolWindow::updateButtonStates);
            connect(m_currentFilter, &IFindFilter::validChanged,
                    this, &FindToolWindow::updateButtonStates);
            updateButtonStates();
            if (m_configWidget)
                m_ui.configWidget->layout()->addWidget(m_configWidget);
        } else {
            if (configWidget)
                configWidget->setParent(0);
        }
    }
    QWidget *w = m_ui.configWidget;
    while (w) {
        QScrollArea *sa = qobject_cast<QScrollArea *>(w);
        if (sa) {
            sa->updateGeometry();
            break;
        }
        w = w->parentWidget();
    }
    for (w = m_configWidget ? m_configWidget : m_ui.configWidget; w; w = w->parentWidget()) {
        if (w->layout())
            w->layout()->activate();
    }
}

void FindToolWindow::acceptAndGetParameters(QString *term, IFindFilter **filter)
{
    if (filter)
        *filter = 0;
    Find::updateFindCompletion(m_ui.searchTerm->text());
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
    IFindFilter *filter = 0;
    acceptAndGetParameters(&term, &filter);
    QTC_ASSERT(filter, return);
    filter->findAll(term, Find::findFlags());
}

void FindToolWindow::replace()
{
    QString term;
    IFindFilter *filter = 0;
    acceptAndGetParameters(&term, &filter);
    QTC_ASSERT(filter, return);
    filter->replaceAll(term, Find::findFlags());
}

void FindToolWindow::writeSettings()
{
    QSettings *settings = ICore::settings();
    settings->beginGroup(QLatin1String("Find"));
    settings->setValue(QLatin1String("CurrentFilter"), m_currentFilter ?  m_currentFilter->id() : QString());
    foreach (IFindFilter *filter, m_filters)
        filter->writeSettings(settings);
    settings->endGroup();
}

void FindToolWindow::readSettings()
{
    QSettings *settings = ICore::settings();
    settings->beginGroup(QLatin1String("Find"));
    const QString currentFilter = settings->value(QLatin1String("CurrentFilter")).toString();
    for (int i = 0; i < m_filters.size(); ++i) {
        IFindFilter *filter = m_filters.at(i);
        filter->readSettings(settings);
        if (filter->id() == currentFilter)
            setCurrentFilter(i);
    }
    settings->endGroup();
}
