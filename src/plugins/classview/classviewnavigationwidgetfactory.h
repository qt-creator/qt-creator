/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Denis Mingulov.
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

#ifndef CLASSVIEWNAVIGATIONWIDGETFACTORY_H
#define CLASSVIEWNAVIGATIONWIDGETFACTORY_H

#include <coreplugin/inavigationwidgetfactory.h>

#include <QtCore/QScopedPointer>

namespace ClassView {
namespace Internal {

/*!
   \class NavigationWidgetFactory
   \brief INavigationWidgetFactory implementation for Class View

   INavigationWidgetFactory implementation for Class View. Singleton instance.
   Supports \a setState publc slot to add/remove factory to \a ExtensionSystem::PluginManager.
   Also supports some additional signals, \a widgetIsCreated and \a stateChanged.
 */

class NavigationWidgetFactory : public Core::INavigationWidgetFactory
{
    Q_OBJECT
    Q_DISABLE_COPY(NavigationWidgetFactory)

public:
    //! destructor
    virtual ~NavigationWidgetFactory();

    //! get an instance
    static NavigationWidgetFactory *instance();

    // Core::INavigationWidgetFactory
    //! \implements Core::INavigationWidgetFactory::displayName
    QString displayName() const;

    //! \implements Core::INavigationWidgetFactory::priority
    int priority() const;

    //! \implements Core::INavigationWidgetFactory::id
    QString id() const;

    //! \implements Core::INavigationWidgetFactory::activationSequence
    QKeySequence activationSequence() const;

    //! \implements Core::INavigationWidgetFactory::createWidget
    Core::NavigationView createWidget();

    //! \implements Core::INavigationWidgetFactory::saveSettings
    void saveSettings(int position, QWidget *widget);

    //! \implements Core::INavigationWidgetFactory::restoreSettings
    void restoreSettings(int position, QWidget *widget);

    // own functionality

signals:
    /*!
       \brief Signal which informs that the widget factory creates a widget.
     */
    void widgetIsCreated();

private:
    //! Constructor
    NavigationWidgetFactory();

    /*!
       \brief Get a settings prefix for the specified position
       \param position Position
       \return Settings prefix
     */
    QString settingsPrefix(int position) const;

private:
    //! private class data pointer
    QScopedPointer<struct NavigationWidgetFactoryPrivate> d_ptr;
};

} // namespace Internal
} // namespace ClassView

#endif // CLASSVIEWNAVIGATIONWIDGETFACTORY_H
