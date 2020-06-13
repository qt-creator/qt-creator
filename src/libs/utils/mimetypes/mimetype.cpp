/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "mimetype.h"

#include "mimetype_p.h"
#include "mimedatabase_p.h"
#include "mimeprovider_p.h"

#include "mimeglobpattern_p.h"

#include <QtCore/QDebug>
#include <QtCore/QLocale>

#include <memory>

using namespace Utils;
using namespace Utils::Internal;

static QString suffixFromPattern(const QString &pattern)
{
    // Not a simple suffix if it looks like: README or *. or *.* or *.JP*G or *.JP?
    if (pattern.startsWith(QLatin1String("*.")) &&
            pattern.length() > 2 &&
            pattern.indexOf(QLatin1Char('*'), 2) < 0 && pattern.indexOf(QLatin1Char('?'), 2) < 0) {
        return pattern.mid(2);
    }
    return QString();
}

MimeTypePrivate::MimeTypePrivate()
    : loaded(false)
{}

MimeTypePrivate::MimeTypePrivate(const MimeType &other)
        : name(other.d->name),
        localeComments(other.d->localeComments),
        genericIconName(other.d->genericIconName),
        iconName(other.d->iconName),
        globPatterns(other.d->globPatterns),
        loaded(other.d->loaded)
{}

void MimeTypePrivate::clear()
{
    name.clear();
    localeComments.clear();
    genericIconName.clear();
    iconName.clear();
    globPatterns.clear();
    loaded = false;
}

void MimeTypePrivate::addGlobPattern(const QString &pattern)
{
    globPatterns.append(pattern);
}

/*!
    \class MimeType
    \inmodule QtCreator
    \ingroup shared
    \brief The MimeType class describes types of file or data, represented by a MIME type string.

    \since 5.0

    For instance, a file named \c readme.txt has the MIME type \c text/plain.
    The MIME type can be determined from the file name, or from the file
    contents, or from both. MIME type determination can also be done on
    buffers of data not coming from files.

    Determining the MIME type of a file can be useful to make sure your
    application supports it. It is also useful in file-manager-like applications
    or widgets, in order to display an appropriate \l {MimeType::iconName}{icon} for the file, or even
    the descriptive \l {MimeType::comment()}{comment} in detailed views.

    To check if a file has the expected MIME type, you should use inherits()
    rather than a simple string comparison based on the name(). This is because
    MIME types can inherit from each other: for instance a C source file is
    a specific type of plain text file, so \c text/x-csrc inherits \c text/plain.
 */

/*!
    Constructs this MimeType object initialized with default property values that indicate an invalid MIME type.
 */
MimeType::MimeType() :
        d(new MimeTypePrivate())
{
}

/*!
    Constructs this MimeType object as a copy of \a other.
 */
MimeType::MimeType(const MimeType &other) = default;

/*!
    Assigns the data of \a other to this MimeType object, and returns a reference to this object.
 */
MimeType &MimeType::operator=(const MimeType &other)
{
    if (d != other.d)
        d = other.d;
    return *this;
}

/*!
    \fn MimeType::MimeType(const Internal::MimeTypePrivate &dd)
    \internal
 */
MimeType::MimeType(const MimeTypePrivate &dd) :
        d(new MimeTypePrivate(dd))
{
}

/*!
    Destroys the MimeType object, and releases the d pointer.
 */
MimeType::~MimeType() = default;

/*!
    Returns \c true if \a other equals this MimeType object, otherwise returns \c false.
    The name is the unique identifier for a MIME type, so two  MIME types with
    the same name are equal.
 */
bool MimeType::operator==(const MimeType &other) const
{
    return d == other.d || d->name == other.d->name;
}

/*!
    \fn bool MimeType::operator!=(const MimeType &other) const;
    Returns \c true if \a other does not equal this MimeType object, otherwise returns \c false.
 */

/*!
    \fn inline uint Utils::qHash(const MimeType &mime)
    \internal
*/

/*!
    Returns \c true if the MimeType object contains valid data, otherwise returns \c false.
    A valid MIME type has a non-empty name().
    The invalid MIME type is the default-constructed MimeType.
 */
bool MimeType::isValid() const
{
    return !d->name.isEmpty();
}

/*!
    Returns \c true if this MIME type is the default MIME type which
    applies to all files: \c application/octet-stream.
 */
bool MimeType::isDefault() const
{
    return d->name == MimeDatabasePrivate::instance()->defaultMimeType();
}

/*!
    Returns the name of the MIME type.
 */
QString MimeType::name() const
{
    return d->name;
}

/*!
    Returns the description of the MIME type to be displayed on user interfaces.

    The system language (QLocale::system().name()) is used to select the appropriate translation.
 */
QString MimeType::comment() const
{
    MimeDatabasePrivate::instance()->provider()->loadMimeTypePrivate(*d);

    QStringList languageList;
    languageList << QLocale::system().name();
    languageList << QLocale::system().uiLanguages();
    Q_FOREACH (const QString &language, languageList) {
        const QString lang = language == QLatin1String("C") ? QLatin1String("en_US") : language;
        const QString comm = d->localeComments.value(lang);
        if (!comm.isEmpty())
            return comm;
        const int pos = lang.indexOf(QLatin1Char('_'));
        if (pos != -1) {
            // "pt_BR" not found? try just "pt"
            const QString shortLang = lang.left(pos);
            const QString commShort = d->localeComments.value(shortLang);
            if (!commShort.isEmpty())
                return commShort;
        }
    }

    // Use the mimetype name as fallback
    return d->name;
}

/*!
    Returns the file name of a generic icon that represents the MIME type.

    This should be used if the icon returned by iconName() cannot be found on
    the system. It is used for categories of similar types (like spreadsheets
    or archives) that can use a common icon.
    The freedesktop.org Icon Naming Specification lists a set of such icon names.

    The icon name can be given to QIcon::fromTheme() in order to load the icon.
 */
QString MimeType::genericIconName() const
{
    MimeDatabasePrivate::instance()->provider()->loadGenericIcon(*d);
    if (d->genericIconName.isEmpty()) {
        // From the spec:
        // If the generic icon name is empty (not specified by the mimetype definition)
        // then the mimetype is used to generate the generic icon by using the top-level
        // media type (e.g.  "video" in "video/ogg") and appending "-x-generic"
        // (i.e. "video-x-generic" in the previous example).
        QString group = name();
        const int slashindex = group.indexOf(QLatin1Char('/'));
        if (slashindex != -1)
            group = group.left(slashindex);
        return group + QLatin1String("-x-generic");
    }
    return d->genericIconName;
}

/*!
    Returns the file name of an icon image that represents the MIME type.

    The icon name can be given to QIcon::fromTheme() in order to load the icon.
 */
QString MimeType::iconName() const
{
    MimeDatabasePrivate::instance()->provider()->loadIcon(*d);
    if (d->iconName.isEmpty()) {
        // Make default icon name from the mimetype name
        d->iconName = name();
        const int slashindex = d->iconName.indexOf(QLatin1Char('/'));
        if (slashindex != -1)
            d->iconName[slashindex] = QLatin1Char('-');
    }
    return d->iconName;
}

/*!
    Returns the list of glob matching patterns.
 */
QStringList MimeType::globPatterns() const
{
    MimeDatabasePrivate::instance()->provider()->loadMimeTypePrivate(*d);
    return d->globPatterns;
}

/*!
    A type is a subclass of another type if any instance of the first type is
    also an instance of the second. For example, all \c image/svg+xml files are
    also \c text/xml, \c text/plain and \c application/octet-stream files.

    Subclassing is about the format, rather than the category of the data.
    For example, there is no \e {generic spreadsheet} class that all
    spreadsheets inherit from.

    Conversely, the parent  MIME type of \c image/svg+xml is \c text/xml.

    A  MIME type can have multiple parents. For instance, \c application/x-perl
    has two parents: \c application/x-executable and \c text/plain. This makes
    it possible to both execute perl scripts, and to open them in text editors.
*/
QStringList MimeType::parentMimeTypes() const
{
    return MimeDatabasePrivate::instance()->provider()->parents(d->name);
}

static void collectParentMimeTypes(const QString &mime, QStringList &allParents)
{
    const QStringList parents = MimeDatabasePrivate::instance()->provider()->parents(mime);
    for (const QString &parent : parents) {
        // I would use QSet, but since order matters I better not
        if (!allParents.contains(parent))
            allParents.append(parent);
    }
    // We want a breadth-first search, so that the least-specific parent (octet-stream) is last
    // This means iterating twice, unfortunately.
    for (const QString &parent : parents)
        collectParentMimeTypes(parent, allParents);
}

/*!
    Returns all the parent MIME types of this type, direct and indirect.
    This includes grandparents, and so on.

    For instance, for \c image/svg+xml the list would be:
    \c application/xml, \c text/plain, \c application/octet-stream.

    \note The \c application/octet-stream type is the ultimate parent for all types
    of files (but not directories).
*/
QStringList MimeType::allAncestors() const
{
    QStringList allParents;
    collectParentMimeTypes(d->name, allParents);
    return allParents;
}

/*!
    Returns the list of aliases of this MIME type.

    For instance, for \c text/csv, the returned list would be:
    \c text/x-csv, \c text/x-comma-separated-values.

    \note All MimeType instances refer to proper  MIME types,
    never to aliases directly.

    The order of the aliases in the list is undefined.
*/
QStringList MimeType::aliases() const
{
    return MimeDatabasePrivate::instance()->provider()->listAliases(d->name);
}

/*!
    Returns the known suffixes for the MIME type.
    No leading dot is included, so for instance this would return
    \c {"jpg", "jpeg"} for \c image/jpeg.
 */
QStringList MimeType::suffixes() const
{
    MimeDatabasePrivate::instance()->provider()->loadMimeTypePrivate(*d);

    QStringList result;
    for (const QString &pattern : d->globPatterns) {
        const QString suffix = suffixFromPattern(pattern);
        if (!suffix.isEmpty())
            result.append(suffix);
    }

    return result;
}

/*!
    Returns the preferred suffix for the MIME type.
    No leading dot is included, so for instance this would return \c "pdf" for
    \c application/pdf. The return value can be empty, for MIME types which do
    not have any suffixes associated.
 */
QString MimeType::preferredSuffix() const
{
    const QStringList suffixList = suffixes();
    return suffixList.isEmpty() ? QString() : suffixList.at(0);
}

/*!
    Returns a filter string usable for a file dialog.
*/
QString MimeType::filterString() const
{
    MimeDatabasePrivate::instance()->provider()->loadMimeTypePrivate(*d);
    QString filter;

    if (!d->globPatterns.empty()) {
        filter += comment() + QLatin1String(" (");
        for (int i = 0; i < d->globPatterns.size(); ++i) {
            if (i != 0)
                filter += QLatin1Char(' ');
            filter += d->globPatterns.at(i);
        }
        filter +=  QLatin1Char(')');
    }

    return filter;
}

/*!
    Returns \c true if the name or alias of the MIME type matches
    \a nameOrAlias.
*/
bool MimeType::matchesName(const QString &nameOrAlias) const
{
    if (d->name == nameOrAlias)
        return true;
    return MimeDatabasePrivate::instance()->provider()->resolveAlias(nameOrAlias) == d->name;
}

/*!
    Sets the preferred filename suffix for the MIME type to \a suffix.
*/
void MimeType::setPreferredSuffix(const QString &suffix)
{
    MimeDatabasePrivate::instance()->provider()->loadMimeTypePrivate(*d);

    auto it = std::find_if(d->globPatterns.begin(), d->globPatterns.end(),
                           [suffix](const QString &pattern) {
                               return suffixFromPattern(pattern) == suffix;
                           });
    if (it != d->globPatterns.end())
        d->globPatterns.erase(it);
    d->globPatterns.prepend(QLatin1String("*.") + suffix);
}

/*!
    Returns \c true if this MIME type is \a mimeTypeName or inherits it,
    or if \a mimeTypeName is an alias for this mimetype.

    \sa parentMimeTypes()
 */
bool MimeType::inherits(const QString &mimeTypeName) const
{
    if (d->name == mimeTypeName)
        return true;
    return MimeDatabasePrivate::instance()->inherits(d->name, mimeTypeName);
}

#ifndef QT_NO_DEBUG_STREAM
QDebug operator<<(QDebug debug, const MimeType &mime)
{
    QDebugStateSaver saver(debug);
    if (!mime.isValid()) {
        debug.nospace() << "MimeType(invalid)";
    } else {
        debug.nospace() << "MimeType(" << mime.name() << ")";
    }
    return debug;
}
#endif
