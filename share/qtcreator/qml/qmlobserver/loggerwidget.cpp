/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the tools applications of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
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
**
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <qglobal.h>
#include <QDebug>
#include <QSettings>
#include <QActionGroup>
#include <QMenu>
#include <QPlainTextEdit>
#ifdef Q_WS_MAEMO_5
#  include <QScrollArea>
#  include <QVBoxLayout>
#  include "texteditautoresizer_maemo5.h"
#endif

#include "loggerwidget.h"

QT_BEGIN_NAMESPACE

LoggerWidget::LoggerWidget(QWidget *parent) :
    QMainWindow(parent),
    m_visibilityOrigin(SettingsOrigin)
{
    setAttribute(Qt::WA_QuitOnClose, false);
    setWindowTitle(tr("Warnings"));

    m_plainTextEdit = new QPlainTextEdit();

#ifdef Q_WS_MAEMO_5
    new TextEditAutoResizer(m_plainTextEdit);
    setAttribute(Qt::WA_Maemo5StackedWindow);
    QScrollArea *area = new QScrollArea();
    area->setWidget(m_plainTextEdit);
    area->setWidgetResizable(true);
    setCentralWidget(area);
#else
    setCentralWidget(m_plainTextEdit);
#endif
    readSettings();
    setupPreferencesMenu();
}

void LoggerWidget::append(const QString &msg)
{
    m_plainTextEdit->appendPlainText(msg);

    if (!isVisible() && (defaultVisibility() == AutoShowWarnings))
        setVisible(true);
}

LoggerWidget::Visibility LoggerWidget::defaultVisibility() const
{
    return m_visibility;
}

void LoggerWidget::setDefaultVisibility(LoggerWidget::Visibility visibility)
{
    if (m_visibility == visibility)
        return;

    m_visibility = visibility;
    m_visibilityOrigin = CommandLineOrigin;

    m_preferencesMenu->setEnabled(m_visibilityOrigin == SettingsOrigin);
}

QMenu *LoggerWidget::preferencesMenu()
{
    return m_preferencesMenu;
}

QAction *LoggerWidget::showAction()
{
    return m_showWidgetAction;
}

void LoggerWidget::readSettings()
{
    QSettings settings;
    QString warningsPreferences = settings.value("warnings", "hide").toString();
    if (warningsPreferences == "show") {
        m_visibility = ShowWarnings;
    } else if (warningsPreferences == "hide") {
        m_visibility = HideWarnings;
    } else {
        m_visibility = AutoShowWarnings;
    }
}

void LoggerWidget::saveSettings()
{
    if (m_visibilityOrigin != SettingsOrigin)
        return;

    QString value = "autoShow";
    if (defaultVisibility() == ShowWarnings) {
        value = "show";
    } else if (defaultVisibility() == HideWarnings) {
        value = "hide";
    }

    QSettings settings;
    settings.setValue("warnings", value);
}

void LoggerWidget::warningsPreferenceChanged(QAction *action)
{
    Visibility newSetting = static_cast<Visibility>(action->data().toInt());
    m_visibility = newSetting;
    saveSettings();
}

void LoggerWidget::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    emit opened();
}

void LoggerWidget::closeEvent(QCloseEvent *event)
{
    QWidget::closeEvent(event);
    emit closed();
}

void LoggerWidget::setupPreferencesMenu()
{
    m_preferencesMenu = new QMenu(tr("Warnings"));
    QActionGroup *warnings = new QActionGroup(m_preferencesMenu);
    warnings->setExclusive(true);

    connect(warnings, SIGNAL(triggered(QAction*)), this, SLOT(warningsPreferenceChanged(QAction*)));

    QAction *showWarningsPreference = new QAction(tr("Show by default"), m_preferencesMenu);
    showWarningsPreference->setCheckable(true);
    showWarningsPreference->setData(LoggerWidget::ShowWarnings);
    warnings->addAction(showWarningsPreference);
    m_preferencesMenu->addAction(showWarningsPreference);

    QAction *hideWarningsPreference = new QAction(tr("Hide by default"), m_preferencesMenu);
    hideWarningsPreference->setCheckable(true);
    hideWarningsPreference->setData(LoggerWidget::HideWarnings);
    warnings->addAction(hideWarningsPreference);
    m_preferencesMenu->addAction(hideWarningsPreference);

    QAction *autoWarningsPreference = new QAction(tr("Show for first warning"), m_preferencesMenu);
    autoWarningsPreference->setCheckable(true);
    autoWarningsPreference->setData(LoggerWidget::AutoShowWarnings);
    warnings->addAction(autoWarningsPreference);
    m_preferencesMenu->addAction(autoWarningsPreference);

    switch (defaultVisibility()) {
    case LoggerWidget::ShowWarnings:
        showWarningsPreference->setChecked(true);
        break;
    case LoggerWidget::HideWarnings:
        hideWarningsPreference->setChecked(true);
        break;
    default:
        autoWarningsPreference->setChecked(true);
    }
}

QT_END_NAMESPACE
