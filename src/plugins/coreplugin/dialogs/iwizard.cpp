#include "iwizard.h"

/*!
    \class Core::IWizard
    \mainclass

    \brief The class IWizard is the base class for all wizards
    (for example shown in \gui {File | New}).

    The wizard interface is a very thin abstraction for the \gui{New...} wizards.
    Basically it defines what to show to the user in the wizard selection dialogs,
    and a hook that is called if the user selects the wizard.

    Wizards can then perform any operations they like, including showing dialogs and
    creating files. Often it is not necessary to create your own wizard from scratch,
    instead use one of the predefined wizards and adapt it to your needs.

    To make your wizard known to the system, add your IWizard instance to the
    plugin manager's object pool in your plugin's initialize method:
    \code
        bool MyPlugin::initialize(const QStringList &arguments, QString *errorString)
        {
            // ... do setup
            addAutoReleasedObject(new MyWizard);
            // ... do more setup
        }
    \endcode
    \sa Core::BaseFileWizard
    \sa Core::StandardFileWizard
*/

/*!
    \enum Core::IWizard::Kind
    Used to specify what kind of objects the wizard creates. This information is used
    to show e.g. only wizards that create projects when selecting a \gui{New Project}
    menu item.
    \value FileWizard
        The wizard creates one or more files.
    \value ClassWizard
        The wizard creates a new class (e.g. source+header files).
    \value ProjectWizard
        The wizard creates a new project.
*/

/*!
    \fn IWizard::IWizard(QObject *parent)
    \internal
*/

/*!
    \fn IWizard::~IWizard()
    \internal
*/

/*!
    \fn Kind IWizard::kind() const
    Returns what kind of objects are created by the wizard.
    \sa Kind
*/

/*!
    \fn QIcon IWizard::icon() const
    Returns an icon to show in the wizard selection dialog.
*/

/*!
    \fn QString IWizard::description() const
    Returns a translated description to show when this wizard is selected
    in the dialog.
*/

/*!
    \fn QString IWizard::name() const
    Returns the translated name of the wizard, how it should appear in the
    dialog.
*/

/*!
    \fn QString IWizard::category() const
    Returns a category ID to add the wizard to.
*/

/*!
    \fn QString IWizard::trCategory() const
    Returns the translated string of the category, how it should appear
    in the dialog.
*/

/*!
    \fn QStringList IWizard::runWizard(const QString &path, QWidget *parent)
    This method is executed when the wizard has been selected by the user
    for execution. Any dialogs the wizard opens should use the given \a parent.
    The \a path argument is a suggestion for the location where files should be
    created. The wizard should fill this in it's path selection elements as a
    default path.
    Returns a list of files (absolute paths) that have been created, if any.
*/
