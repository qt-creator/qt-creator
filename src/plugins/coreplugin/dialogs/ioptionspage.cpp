/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "ioptionspage.h"

#include <QCheckBox>
#include <QGroupBox>
#include <QLabel>
#include <QPushButton>

/*!
  \class Core::IOptionsPage
  \mainclass
  \brief The IOptionsPage class is an interface for providing pages for the
  \gui Options dialog (called \gui Preferences on Mac OS).

  You need to subclass this interface and put an instance of your subclass
  into the plugin manager object pool (e.g. ExtensionSystem::PluginManager::addObject).
  Guidelines for implementing:
  \list
  \li \c id() is a unique identifier for referencing this page
  \li \c displayName() is the (translated) name for display
  \li \c category() is the unique id for the category that the page should be displayed in
  \li \c displayCategory() is the translated name of the category
  \li \c widget() is called to retrieve the widget to show in the
        \gui Options dialog. You should create a widget lazily here, and delete it again in the
        finish() method. This method can be called multiple times, so you should only create a new
        widget if the old one was deleted.
  \li \c apply() is called to store the settings. It should detect if any changes have been
         made and store those
  \li \c finish() is called directly before the \gui Options dialog closes. Here you should delete
         the widget that was created in widget() to free resources.
  \li \c matches() is used for the \gui Options dialog search filter. The default implementation
         takes the widget() and searches for all labels, buttons, checkboxes and group boxes,
         and matches on their texts/titles. You can implement your own matching algorithm, but
         usually the default implementation will work fine.
  \endlist
*/


Core::IOptionsPage::IOptionsPage(QObject *parent)
    : QObject(parent),
      m_keywordsInitialized(false)
{

}

Core::IOptionsPage::~IOptionsPage()
{
}

bool Core::IOptionsPage::matches(const QString &searchKeyWord) const
{
    if (!m_keywordsInitialized) {
        IOptionsPage *that = const_cast<IOptionsPage *>(this);
        QWidget *widget = that->widget();
        if (!widget)
            return false;
        // find common subwidgets
        foreach (const QLabel *label, widget->findChildren<QLabel *>())
            m_keywords << label->text();
        foreach (const QCheckBox *checkbox, widget->findChildren<QCheckBox *>())
            m_keywords << checkbox->text();
        foreach (const QPushButton *pushButton, widget->findChildren<QPushButton *>())
            m_keywords << pushButton->text();
        foreach (const QGroupBox *groupBox, widget->findChildren<QGroupBox *>())
            m_keywords << groupBox->title();

        // clean up accelerators
        QMutableStringListIterator it(m_keywords);
        while (it.hasNext())
            it.next().remove(QLatin1Char('&'));
        m_keywordsInitialized = true;
    }
    foreach (const QString &keyword, m_keywords)
        if (keyword.contains(searchKeyWord, Qt::CaseInsensitive))
            return true;
    return false;
}
