/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Copyright (C) 2016 Falko Arps
** Copyright (C) 2016 Sven Klein
** Copyright (C) 2016 Giuliano Schneider
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

#include "ioptionspage.h"

#include <QCheckBox>
#include <QGroupBox>
#include <QIcon>
#include <QLabel>
#include <QPushButton>

/*!
    \class Core::IOptionsPage
    \mainclass
    \inmodule Qt Creator
    \brief The IOptionsPage class is an interface for providing pages for the
    \gui Options dialog (called \gui Preferences on Mac OS).
*/

/*!

    \fn Id IOptionsPage::id() const

    Returns a unique identifier for referencing the options page.
*/

/*!
    \fn QString IOptionsPage::displayName() const

    Returns the translated display name of the options page.
*/

/*!
    \fn Id IOptionsPage::category() const

    Returns the unique id for the category that the options page should be displayed in. This id is
    used for sorting the list on the left side of the \gui Options dialog.
*/

/*!
    \fn QString IOptionsPage::displayCategory() const

    Returns the translated category name of the options page. This name is displayed in the list on
    the left side of the \gui Options dialog.
*/

/*!
    \fn QIcon IOptionsPage::categoryIcon() const

    Returns the category icon of the options page. This icon is displayed in the list on the left
    side of the \gui Options dialog.
*/

QIcon Core::IOptionsPage::categoryIcon() const
{
    return m_categoryIcon.icon();
}

/*!
    \fn QWidget *IOptionsPage::widget()

    Returns the widget to show in the \gui Options dialog. You should create a widget lazily here,
    and delete it again in the finish() method. This method can be called multiple times, so you
    should only create a new widget if the old one was deleted.
*/

/*!
    \fn void IOptionsPage::apply()

    This is called when selecting the \gui Apply button on the options page dialog. It should detect
    whether any changes were made and store those.
*/

/*!
    \fn void IOptionsPage::finish()

    Is called directly before the \gui Options dialog closes. Here you should delete the widget that
    was created in widget() to free resources.
*/

/*!
    \fn void IOptionsPage::setId(Id id)

    Sets the \a id of the options page.
*/

/*!
    \fn void IOptionsPage::setDisplayName(const QString &displayName)

    Sets \a displayName as the display name of the options page.
*/

/*!
    \fn void IOptionsPage::setCategory(Id category)

    Uses \a category to sort the options pages.
*/

/*!
    \fn void IOptionsPage::setDisplayCategory(const QString &displayCategory)

    Sets \a displayCategory as the display category of the options page.
*/

/*!
    \fn void IOptionsPage::setCategoryIcon(const QString &categoryIcon)

    Sets \a categoryIcon as the category icon of the options page.
*/


/*!
    Constructs an options page with the given \a parent.
*/
Core::IOptionsPage::IOptionsPage(QObject *parent)
    : QObject(parent),
      m_keywordsInitialized(false)
{

}

/*!
    Destroys the options page.
 */
Core::IOptionsPage::~IOptionsPage()
{
}

/*!
    Is used by the \gui Options dialog search filter to match \a searchKeyWord to this options
    page. This defaults to take the widget and then looks for all child labels, check boxes, push
    buttons, and group boxes. Should return \c true when a match is found.
*/
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

QIcon Core::IOptionsPageProvider::categoryIcon() const
{
    return QIcon(m_categoryIcon);
}
