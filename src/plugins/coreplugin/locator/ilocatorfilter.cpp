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

#include "ilocatorfilter.h"

#include <coreplugin/coreconstants.h>
#include <utils/camelhumpmatcher.h>

#include <QBoxLayout>
#include <QCheckBox>
#include <QCoreApplication>
#include <QDialog>
#include <QDialogButtonBox>
#include <QLabel>
#include <QLineEdit>
#include <QRegularExpression>

using namespace Core;

/*!
    \class Core::ILocatorFilter
    \inmodule Qt Creator

    \brief The ILocatorFilter class adds a locator filter.

    The filter is added to \uicontrol Tools > \uicontrol Locate.
*/

/*!
    Constructs a locator filter with \a parent. Call from subclasses.
*/
ILocatorFilter::ILocatorFilter(QObject *parent):
    QObject(parent)
{
}

/*!
    Specifies a shortcut string that can be used to explicitly choose this
    filter in the locator input field by preceding the search term with the
    shortcut string and a whitespace.

    The default value is an empty string.

    \sa setShortcutString()
*/
QString ILocatorFilter::shortcutString() const
{
    return m_shortcut;
}

/*!
    Performs actions that need to be done in the main thread before actually
    running the search for \a entry.

    Called on the main thread before matchesFor() is called in a separate
    thread.

    The default implementation does nothing.

    \sa matchesFor()
*/
void ILocatorFilter::prepareSearch(const QString &entry)
{
    Q_UNUSED(entry)
}

/*!
    Sets the \a shortcut string that can be used to explicitly choose this
    filter in the locator input field. Call from the constructor of subclasses
    to set the default setting.

    \sa shortcutString()
*/
void ILocatorFilter::setShortcutString(const QString &shortcut)
{
    m_shortcut = shortcut;
}

/*!
    Returns data that can be used to restore the settings for this filter
    (for example at startup).
    By default, adds the base settings (shortcut string, included by default)
    with a data stream.

    \sa restoreState()
*/
QByteArray ILocatorFilter::saveState() const
{
    QByteArray value;
    QDataStream out(&value, QIODevice::WriteOnly);
    out << shortcutString();
    out << isIncludedByDefault();
    return value;
}

/*!
    Restores the \a state of the filter from data previously created by
    saveState().

    \sa saveState()
*/
void ILocatorFilter::restoreState(const QByteArray &state)
{
    QString shortcut;
    bool defaultFilter;

    QDataStream in(state);
    in >> shortcut;
    in >> defaultFilter;

    setShortcutString(shortcut);
    setIncludedByDefault(defaultFilter);
}

/*!
    Opens a dialog for the \a parent widget that allows the user to configure
    various aspects of the filter. Called when the user requests to configure
    the filter.

    Set \a needsRefresh to \c true, if a refresh() should be done after
    closing the dialog. Return \c false if the user canceled the dialog.

    The default implementation allows changing the shortcut and whether the
    filter is included by default.

    \sa refresh()
*/
bool ILocatorFilter::openConfigDialog(QWidget *parent, bool &needsRefresh)
{
    Q_UNUSED(needsRefresh)

    QDialog dialog(parent, Qt::WindowTitleHint | Qt::WindowSystemMenuHint);
    dialog.setWindowTitle(msgConfigureDialogTitle());

    QVBoxLayout *vlayout = new QVBoxLayout(&dialog);
    QHBoxLayout *hlayout = new QHBoxLayout;
    QLineEdit *shortcutEdit = new QLineEdit(shortcutString());
    QCheckBox *includeByDefault = new QCheckBox(msgIncludeByDefault());
    includeByDefault->setToolTip(msgIncludeByDefaultToolTip());
    includeByDefault->setChecked(isIncludedByDefault());

    auto prefixLabel = new QLabel(msgPrefixLabel());
    prefixLabel->setToolTip(msgPrefixToolTip());
    hlayout->addWidget(prefixLabel);
    hlayout->addWidget(shortcutEdit);
    hlayout->addWidget(includeByDefault);

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok |
                                                       QDialogButtonBox::Cancel);
    connect(buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    vlayout->addLayout(hlayout);
    vlayout->addStretch();
    vlayout->addWidget(buttonBox);

    if (dialog.exec() == QDialog::Accepted) {
        setShortcutString(shortcutEdit->text().trimmed());
        setIncludedByDefault(includeByDefault->isChecked());
        return true;
    }

    return false;
}

/*!
    Returns whether a case sensitive or case insensitive search should be
    performed for the search term \a str.
*/
Qt::CaseSensitivity ILocatorFilter::caseSensitivity(const QString &str)
{
    return str == str.toLower() ? Qt::CaseInsensitive : Qt::CaseSensitive;
}

/*!
    Returns whether the search term \a str contains wildcard characters.
    Can be used for choosing an optimal matching strategy.
*/
bool ILocatorFilter::containsWildcard(const QString &str)
{
    return str.contains(QLatin1Char('*')) || str.contains(QLatin1Char('?'));
}

/*!
 * \brief Returns a simple regular expression to search for \a text.
 *
 * \a text may contain the simple '?' and '*' wildcards known from the shell.
 * '?' matches exactly one character, '*' matches a number of characters
 * (including none).
 *
 * The regular expression contains capture groups to allow highlighting
 * matched characters after a match.
 */
static QRegularExpression createWildcardRegExp(const QString &text)
{
    QString pattern = '(' + text + ')';
    pattern.replace('?', ").(");
    pattern.replace('*', ").*(");
    pattern.remove("()");
    return QRegularExpression(pattern, QRegularExpression::CaseInsensitiveOption);
}

QRegularExpression ILocatorFilter::createRegExp(const QString &text)
{
    return containsWildcard(text) ? createWildcardRegExp(text)
                                  : CamelHumpMatcher::createCamelHumpRegExp(text);
}

LocatorFilterEntry::HighlightInfo ILocatorFilter::highlightInfo(
        const QRegularExpressionMatch &match, LocatorFilterEntry::HighlightInfo::DataType dataType)
{
    const CamelHumpMatcher::HighlightingPositions positions =
            CamelHumpMatcher::highlightingPositions(match);

    return LocatorFilterEntry::HighlightInfo(positions.starts, positions.lengths, dataType);
}

/*!
    Specifies a title for configuration dialogs.
*/
QString ILocatorFilter::msgConfigureDialogTitle()
{
    return tr("Filter Configuration");
}

/*!
    Specifies a label for the prefix input field in configuration dialogs.
*/
QString ILocatorFilter::msgPrefixLabel()
{
    return tr("Prefix:");
}

/*!
    Specifies a tooltip for the  prefix input field in configuration dialogs.
*/
QString ILocatorFilter::msgPrefixToolTip()
{
    return tr("Type the prefix followed by a space and search term to restrict search to the filter.");
}

/*!
    Specifies a label for the include by default input field in configuration
    dialogs.
*/
QString ILocatorFilter::msgIncludeByDefault()
{
    return tr("Include by default");
}

/*!
    Specifies a tooltip for the include by default input field in configuration
    dialogs.
*/
QString ILocatorFilter::msgIncludeByDefaultToolTip()
{
    return tr("Include the filter when not using a prefix for searches.");
}

/*!
    Returns whether a configuration dialog is available for this filter.

    The default is \c true.

    \sa setConfigurable()
*/
bool ILocatorFilter::isConfigurable() const
{
    return m_isConfigurable;
}

/*!
    Returns whether using the shortcut string is required to use this filter.
    The default is \c false.

    \sa shortcutString()
    \sa setIncludedByDefault()
*/
bool ILocatorFilter::isIncludedByDefault() const
{
    return m_includedByDefault;
}

/*!
    Sets whether using the shortcut string is required to use this filter.
    Call from the constructor of subclasses to change the default.

    \sa isIncludedByDefault()
*/
void ILocatorFilter::setIncludedByDefault(bool includedByDefault)
{
    m_includedByDefault = includedByDefault;
}

/*!
    Returns whether the filter should be hidden in the
    \uicontrol {Locator filters} filter, menus, and locator settings.

    The default is \c false.

    \sa setHidden()
*/
bool ILocatorFilter::isHidden() const
{
    return m_hidden;
}

/*!
    Hides the filter in the \uicontrol {Locator filters} filter,
    menus, and locator settings. Call in the constructor of subclasses.
*/
void ILocatorFilter::setHidden(bool hidden)
{
    m_hidden = hidden;
}

/*!
    Returns whether the filter is currently available. Disabled filters are
    neither visible in menus nor included in searches, even when the search is
    prefixed with their shortcut string.

    The default is \c true.

    \sa setEnabled()
*/
bool ILocatorFilter::isEnabled() const
{
    return m_enabled;
}

/*!
    Returns the filter's unique ID.

    \sa setId()
*/
Id ILocatorFilter::id() const
{
    return m_id;
}

Id ILocatorFilter::actionId() const
{
    return m_id.withPrefix("Locator.");
}

/*!
    Returns the filter's translated display name.

    \sa setDisplayName()
*/
QString ILocatorFilter::displayName() const
{
    return m_displayName;
}

/*!
    Returns the priority that is used for ordering the results when multiple
    filters are used.

    The default is ILocatorFilter::Medium.

    \sa setPriority()
*/
ILocatorFilter::Priority ILocatorFilter::priority() const
{
    return m_priority;
}

/*!
    Sets whether the filter is currently available.

    \sa isEnabled()
*/
void ILocatorFilter::setEnabled(bool enabled)
{
    m_enabled = enabled;
}

/*!
    Sets the filter's unique ID.
    Subclasses must set the ID in their constructor.

    \sa id()
*/
void ILocatorFilter::setId(Id id)
{
    m_id = id;
}

/*!
    Sets the priority of results of this filter in the result list.

    \sa priority()
*/
void ILocatorFilter::setPriority(Priority priority)
{
    m_priority = priority;
}

/*!
    Sets the translated display name of this filter.

    Subclasses must set the display name in their constructor.

    \sa displayName()
*/
void ILocatorFilter::setDisplayName(const QString &displayString)
{
    m_displayName = displayString;
}

/*!
    Sets whether the filter provides a configuration dialog.
    Most filters should at least provide the default dialog.

    \sa isConfigurable()
*/
void ILocatorFilter::setConfigurable(bool configurable)
{
    m_isConfigurable = configurable;
}

/*!
    \fn QList<LocatorFilterEntry> ILocatorFilter::matchesFor(QFutureInterface<LocatorFilterEntry> &future, const QString &entry)

    Returns the list of results of this filter for the search term \a entry.
    This is run in a separate thread, but is guaranteed to only run in a single
    thread at any given time. Quickly running preparations can be done in the
    GUI thread in prepareSearch().

    Implementations should do a case sensitive or case insensitive search
    depending on caseSensitivity(). If \a future is \c canceled, the search
    should be aborted.

    \sa prepareSearch()
    \sa caseSensitivity()
    \sa containsWildcard()
*/

/*!
    \fn void ILocatorFilter::accept(LocatorFilterEntry selection, QString *newText, int *selectionStart, int *selectionLength) const

    Called with the entry specified by \a selection when the user activates it
    in the result list.
    Implementations can return a new search term \a newText, which has \a selectionLength characters
    starting from \a selectionStart preselected, and the cursor set to the end of the selection.
*/

/*!
    \fn void ILocatorFilter::refresh(QFutureInterface<void> &future)

    Refreshes cached data asynchronously.

    If \a future is \c canceled, the refresh should be aborted.
*/

/*!
    \enum ILocatorFilter::Priority

    This enum value holds the priority that is used for ordering the results
    when multiple filters are used.

    \value  Highest
            The results for this filter are placed above the results for filters
            that have other priorities.
    \value  High
    \value  Medium
            The default value.
    \value  Low
            The results for this filter are placed below the results for filters
            that have other priorities.
*/
