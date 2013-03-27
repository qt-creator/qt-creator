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

#include "variablemanager.h"

#include <utils/stringutils.h>

#include <QFileInfo>
#include <QMap>
#include <QDebug>

static const char kFilePathPostfix[] = ":FilePath";
static const char kPathPostfix[] = ":Path";
static const char kFileNamePostfix[] = ":FileName";
static const char kFileBaseNamePostfix[] = ":FileBaseName";

namespace Core {

class VMMapExpander : public Utils::AbstractQtcMacroExpander
{
public:
    virtual bool resolveMacro(const QString &name, QString *ret)
    {
        bool found;
        *ret = Core::VariableManager::value(name.toUtf8(), &found);
        return found;
    }
};

class VariableManagerPrivate
{
public:
    QHash<QByteArray, QString> m_map;
    VMMapExpander m_macroExpander;
    QMap<QByteArray, QString> m_descriptions;
};

/*!
    \class Core::VariableManager
    \brief The VariableManager class manages \QC wide variables, that a user
    can enter into many string settings. The variables are replaced by an actual value when the string
    is used, similar to how environment variables are expanded by a shell.

    \section1 Variables

    Variable names can be basically any string without dollar sign and braces,
    though it is recommended to only use 7-bit ASCII without special characters and whitespace.

    If there are several variables that contain different aspects of the same object,
    it is convention to give them the same prefix, followed by a colon and a postfix
    that describes the aspect.
    Examples of this are \c{CurrentDocument:FilePath} and \c{CurrentDocument:Selection}.

    When the variable manager is requested to replace variables in a string, it looks for
    variable names enclosed in ${ and }, like ${CurrentDocument:FilePath}.

    \note The names of the variables are stored as QByteArray. They are typically
    7-bit-clean. In cases where this is not possible, UTF-8 encoding is
    assumed.

    \section1 Providing Variable Values

    Plugins can register variables together with a description through registerVariable(),
    and then need to connect to the variableUpdateRequested() signal to actually give
    the variable its value when requested. A typical setup is to

    \list 1
    \o Register the variables in ExtensionSystem::IPlugin::initialize():
       \code
       static const char kMyVariable[] = "MyVariable";

       bool MyPlugin::initialize(const QStringList &arguments, QString *errorString)
       {
       [...]
       VariableManager::registerVariable(kMyVariable, tr("The current value of whatever I want."));
       connect(VariableManager::instance(), SIGNAL(variableUpdateRequested(QByteArray)),
               this, SLOT(updateVariable(QByteArray)));
       [...]
       }
       \endcode

    \o Set the variable value when requested:
       \code
       void MyPlugin::updateVariable(const QByteArray &variable)
       {
           if (variable == kMyVariable) {
               QString value;
               // do whatever is necessary to retrieve the value
               [...]
               VariableManager::insert(variable, value);
           }
       }
       \endcode
    \endlist

    If there are conditions where your variable is not valid, you should call
    VariableManager::remove(kMyVariable) in updateVariable().

    For variables that refer to a file, you should use the convenience methods
    VariableManager::registerFileVariables(), VariableManager::fileVariableValue() and
    VariableManager::isFileVariable(). The methods take a variable prefix, like \c MyFileVariable,
    and automatically handle standardized postfixes like \c{:FilePath},
    \c{:Path} and \c{:FileBaseName}, resulting in the combined variables, such as
    \c{MyFileVariable:FilePath}.

    \section1 Providing and Expanding Parametrized Strings

    Though it is possible to just ask the variable manager for the value of some variable in your
    code, the preferred use case is to give the user the possibility to parametrize strings, for
    example for settings.

    (If you ever think about doing the former, think twice. It is much more efficient
    to just ask the plugin that provides the variable value directly, without going through
    string conversions, and through the variable manager which will do a large scale poll. To be
    more concrete, using the example from the Providing Variable Values section: instead of
    calling \c{VariableManager::value("MyVariable")}, it is much more efficient to just ask directly
    with \c{MyPlugin::variableValue()}.)

    \section2 User Interface

    If the string that you want to parametrize is settable by the user, through a QLineEdit or
    QTextEdit derived class, you should add a variable chooser to your UI, which allows adding
    variables to the string by browsing through a list. See Core::VariableChooser for more
    details.

    \section2 Expanding Strings

    Expanding variable values in strings is done by "macro expanders".
    Utils::AbstractMacroExpander is the base class for these, and the variable manager
    provides an implementation that expands \QC variables through
    VariableManager::macroExpander().

    There are several different ways to expand a string, covering the different use cases,
    listed here sorted by relevance:
    \list
    \o Using VariableManager::expandedString(). This is the most comfortable way to get a string
       with variable values expanded, but also the least flexible one. If this is sufficient for
       you, use it.
    \o Using the Utils::expandMacros() methods. These take a string and a macro expander (for which
       you would use the one provided by the variable manager). Mostly the same as
       VariableManager::expandedString(), but also has a variant that does the replacement inline
       instead of returning a new string.
    \o Using Utils::QtcProcess::expandMacros(). This expands the string while conforming to the
       quoting rules of the platform it is run on. Use this method with the variable manager's
       macro expander if your string will be passed as a command line parameter string to an
       external command.
    \o Writing your own macro expander that nests the variable manager's macro expander. And then
       doing one of the above. This allows you to expand additional "local" variables/macros,
       that do not come from the variable manager.
    \endlist

*/

/*!
 * \fn void VariableManager::variableUpdateRequested(const QByteArray &variable)
 * Signals that the value of the \a variable should be updated because someone requests its value.
 * Handlers of this signal should call insert() and return as fast as possible.
 */

static VariableManager *variableManagerInstance = 0;
static VariableManagerPrivate *d;

/*!
 * \internal
 */
VariableManager::VariableManager()
{
    d = new VariableManagerPrivate;
    variableManagerInstance = this;
}

/*!
 * \internal
 */
VariableManager::~VariableManager()
{
    variableManagerInstance = 0;
    delete d;
}

/*!
 * Used to set the \a value of a \a variable. Most of the time this is only done when
 * requested by VariableManager::variableUpdateRequested(). If the value of the variable
 * does not change, or changes very seldom, you can also keep the value up to date by calling
 * this method whenever the value changes.
 *
 * As long as insert() was never called for a variable, it will not have a value, not even
 * an empty string, meaning that the variable will not be expanded when expanding strings.
 *
 * \sa remove()
 */
void VariableManager::insert(const QByteArray &variable, const QString &value)
{
    d->m_map.insert(variable, value);
}

/*!
 * Removes any previous value for the given \a variable. This means that the variable
 * will not be expanded at all when expanding strings, not even to an empty string.
 *
 * Returns true if the variable value could be removed, false if the variable value
 * was not set when remove() was called.
 *
 * \sa insert()
 */
bool VariableManager::remove(const QByteArray &variable)
{
    return d->m_map.remove(variable) > 0;
}

/*!
 * Returns the value of the given \a variable. This will request an
 * update of the variable's value first, by sending the variableUpdateRequested() signal.
 * If \a found is given, it is set to true if the variable has a value at all, false if not.
 */
QString VariableManager::value(const QByteArray &variable, bool *found)
{
    variableManagerInstance->variableUpdateRequested(variable);
    if (found)
        *found = d->m_map.contains(variable);
    return d->m_map.value(variable);
}

/*!
 * Returns \a stringWithVariables with all variables replaced by their values.
 * See the VariableManager overview documentation for other ways to expand variables.
 *
 * \sa VariableManager
 * \sa macroExpander()
 */
QString VariableManager::expandedString(const QString &stringWithVariables)
{
    return Utils::expandMacros(stringWithVariables, macroExpander());
}

/*!
 * Returns a macro expander that is used to expand all variables from the variable manager
 * in a string.
 * See the VariableManager overview documentation for other ways to expand variables.
 *
 * \sa VariableManager
 * \sa expandedString()
 */
Utils::AbstractMacroExpander *VariableManager::macroExpander()
{
    return &d->m_macroExpander;
}

/*!
 * Returns the variable manager instance, for connecting to signals. All other methods are static
 * and should be called as class methods, not through the instance.
 */
VariableManager *VariableManager::instance()
{
    return variableManagerInstance;
}

/*!
 * Makes the given \a variable known to the variable manager, together with a localized
 * \a description. It is not strictly necessary to register variables, but highly recommended,
 * because this information is used and presented to the user by the VariableChooser.
 *
 * \sa registerFileVariables()
 */
void VariableManager::registerVariable(const QByteArray &variable, const QString &description)
{
    d->m_descriptions.insert(variable, description);
}

/*!
 * Convenience method to register several variables with the same \a prefix, that have a file
 * as a value. Takes the prefix and registers variables like \c{prefix:FilePath} and
 * \c{prefix:Path}, with descriptions that start with the given \a heading.
 * For example \c{registerFileVariables("CurrentDocument", tr("Current Document"))} registers
 * variables such as \c{CurrentDocument:FilePath} with description
 * "Current Document: Full path including file name."
 *
 * \sa isFileVariable()
 * \sa fileVariableValue()
 */
void VariableManager::registerFileVariables(const QByteArray &prefix, const QString &heading)
{
    registerVariable(prefix + kFilePathPostfix, tr("%1: Full path including file name.").arg(heading));
    registerVariable(prefix + kPathPostfix, tr("%1: Full path excluding file name.").arg(heading));
    registerVariable(prefix + kFileNamePostfix, tr("%1: File name without including path.").arg(heading));
    registerVariable(prefix + kFileBaseNamePostfix, tr("%1: File base name without path and suffix.").arg(heading));
}

/*!
 * Returns whether the \a variable is a file kind of variable with the given \a prefix. For example
 * \c{MyVariable:FilePath} is a file variable with prefix \c{MyVariable}.
 *
 * \sa registerFileVariables()
 * \sa fileVariableValue()
 */
bool VariableManager::isFileVariable(const QByteArray &variable, const QByteArray &prefix)
{
    return variable == prefix + kFilePathPostfix
            || variable == prefix + kPathPostfix
            || variable == prefix + kFileNamePostfix
            || variable == prefix + kFileBaseNamePostfix;
}

/*!
 * Checks if the \a variable is a variable of the file type with the given \a prefix, and returns
 * the value of the variable by extracting the wanted information from the given absolute
 * \a fileName.
 * Returns an empty string if the variable does not have the prefix, or does not have a
 * postfix that is used for file variables, or if the file name is empty.
 *
 * \sa registerFileVariables()
 * \sa isFileVariable()
 */
QString VariableManager::fileVariableValue(const QByteArray &variable, const QByteArray &prefix,
                                           const QString &fileName)
{
    return fileVariableValue(variable, prefix, QFileInfo(fileName));
}

/*!
 * Checks if the \a variable is a variable of the file type with the given \a prefix, and returns
 * the value of the variable by extracting the wanted information from the given
 * \a fileInfo.
 * Returns an empty string if the variable does not have the prefix, or does not have a
 * postfix that is used for file variables, or if the file name is empty.
 *
 * \sa registerFileVariables()
 * \sa isFileVariable()
 */
QString VariableManager::fileVariableValue(const QByteArray &variable, const QByteArray &prefix,
                                           const QFileInfo &fileInfo)
{
    if (variable == prefix + kFilePathPostfix)
        return fileInfo.filePath();
    else if (variable == prefix + kPathPostfix)
        return fileInfo.path();
    else if (variable == prefix + kFileNamePostfix)
        return fileInfo.fileName();
    else if (variable == prefix + kFileBaseNamePostfix)
        return fileInfo.baseName();
    return QString();
}

/*!
 * Returns all registered variable names.
 *
 * \sa registerVariable()
 * \sa registerFileVariables()
 */
QList<QByteArray> VariableManager::variables()
{
    return d->m_descriptions.keys();
}

/*!
 * Returns the description that was registered for the \a variable.
 */
QString VariableManager::variableDescription(const QByteArray &variable)
{
    return d->m_descriptions.value(variable);
}

} // namespace Core
