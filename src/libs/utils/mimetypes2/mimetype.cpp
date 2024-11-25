// Copyright (C) 2016 The Qt Company Ltd.
// Copyright (C) 2015 Klaralvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author David Faure <david.faure@kdab.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "mimetype.h"

#include "mimetype_p.h"
#include "mimedatabase_p.h"

#include <QtCore/QDebug>
#include <QtCore/QLocale>
#include <QtCore/QHashFunctions>

#include <memory>

using namespace Qt::StringLiterals;

namespace Utils {

QString MimeType::suffixFromPattern(const QString &pattern)
{
    // Not a simple suffix if it looks like: README or *. or *.* or *.JP*G or *.JP?
    if (pattern.startsWith("*."_L1) &&
        pattern.size() > 2 &&
        pattern.indexOf(u'*', 2) < 0 && pattern.indexOf(u'?', 2) < 0) {
        return pattern.mid(2);
    }
    return {};
}

/*!
    \class MimeType
    \inmodule QtCore
    \ingroup shared
    \brief The MimeType class describes types of file or data, represented by a MIME type string.

    \since 5.0

    For instance a file named "readme.txt" has the MIME type "text/plain".
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
    a specific type of plain text file, so text/x-csrc inherits text/plain.

    \sa MimeDatabase, {MIME Type Browser}
 */

/*!
    \fn MimeType &MimeType::operator=(MimeType &&other)

    Move-assigns \a other to this MimeType instance.

    \since 5.2
*/

/*!
    \fn MimeType::MimeType();
    Constructs this MimeType object initialized with default property values that indicate an invalid MIME type.
 */
MimeType::MimeType() :
        d(new MimeTypePrivate())
{
}

/*!
    \fn MimeType::MimeType(const MimeType &other);
    Constructs this MimeType object as a copy of \a other.
 */
MimeType::MimeType(const MimeType &other) :
        d(other.d)
{
}

/*!
    \fn MimeType &MimeType::operator=(const MimeType &other);
    Assigns the data of \a other to this MimeType object, and returns a reference to this object.
 */
MimeType &MimeType::operator=(const MimeType &other)
{
    if (d != other.d)
        d = other.d;
    return *this;
}

/*!
    \fn MimeType::MimeType(const MimeTypePrivate &dd);
    Assigns the data of the MimeTypePrivate \a dd to this MimeType object, and returns a reference to this object.
    \internal
 */
MimeType::MimeType(const MimeTypePrivate &dd) :
        d(new MimeTypePrivate(dd))
{
}

/*!
    \fn void MimeType::swap(MimeType &other);
    Swaps MimeType \a other with this MimeType object.

    This operation is very fast and never fails.

    The swap() method helps with the implementation of assignment
    operators in an exception-safe way. For more information consult
    \l {http://en.wikibooks.org/wiki/More_C++_Idioms/Copy-and-swap}
    {More C++ Idioms - Copy-and-swap}.
 */

/*!
    \fn MimeType::~MimeType();
    Destroys the MimeType object, and releases the d pointer.
 */
MimeType::~MimeType()
{
}

/*!
    \fn bool MimeType::operator==(const MimeType &other) const;
    Returns \c true if \a other equals this MimeType object, otherwise returns \c false.
    The name is the unique identifier for a mimetype, so two mimetypes with
    the same name, are equal.
 */
bool MimeType::operator==(const MimeType &other) const
{
    return d == other.d || d->name == other.d->name;
}

/*!
    \since 5.6
    \relates MimeType

    Returns the hash value for \a key, using
    \a seed to seed the calculation.
 */
size_t qHash(const MimeType &key, size_t seed) noexcept
{
    return qHash(key.d->name, seed);
}

/*!
    \fn bool MimeType::operator!=(const MimeType &other) const;
    Returns \c true if \a other does not equal this MimeType object, otherwise returns \c false.
 */

/*!
    \property MimeType::valid
    \brief \c true if the MimeType object contains valid data, \c false otherwise

    A valid MIME type has a non-empty name().
    The invalid MIME type is the default-constructed MimeType.

    While this property was introduced in 5.10, the
    corresponding accessor method has always been there.
 */
bool MimeType::isValid() const
{
    return !d->name.isEmpty();
}

/*!
    \property MimeType::isDefault
    \brief \c true if this MIME type is the default MIME type which
    applies to all files: application/octet-stream.

    While this property was introduced in 5.10, the
    corresponding accessor method has always been there.
 */
bool MimeType::isDefault() const
{
    return d->name == MimeDatabasePrivate::instance()->defaultMimeType();
}

/*!
    \property MimeType::name
    \brief the name of the MIME type

    While this property was introduced in 5.10, the
    corresponding accessor method has always been there.
 */
QString MimeType::name() const
{
    return d->name;
}

/*!
    \property MimeType::comment
    \brief the description of the MIME type to be displayed on user interfaces

    The default language (QLocale().name()) is used to select the appropriate translation.

    While this property was introduced in 5.10, the
    corresponding accessor method has always been there.
 */
QString MimeType::comment() const
{
    const auto localeComments = MimeDatabasePrivate::instance()->localeComments(d->name);

    QStringList languageList = QLocale().uiLanguages();
    qsizetype defaultIndex = languageList.indexOf(u"en-US"_s);

    // Include the default locale as fall-back.
    if (defaultIndex >= 0) {
        // en_US is generally the default, and may be omitted from the
        // overtly-named locales in the MIME type's data (QTBUG-105007).
        ++defaultIndex; // Skip over en-US.
        // That's typically followed by en-Latn-US and en (in that order):
        if (defaultIndex < languageList.size() && languageList.at(defaultIndex) == u"en-Latn-US")
            ++defaultIndex;
        if (defaultIndex < languageList.size() && languageList.at(defaultIndex) == u"en")
            ++defaultIndex;
    } else {
        // Absent en-US, just append it:
        defaultIndex = languageList.size();
    }
    languageList.insert(defaultIndex, u"default"_s);

    for (const QString &language : std::as_const(languageList)) {
            // uiLanguages() uses '-' as separator, MIME database uses '_'
        const QString lang
            = language == "C"_L1 ? u"en_US"_s : QString(language).replace(u'-', u'_');
        QString comm = localeComments.value(lang);
        if (!comm.isEmpty())
            return comm;
        const qsizetype cut = lang.indexOf(u'_');
        // If "de_CH" is missing, check for "de" (and similar):
        if (cut != -1) {
            comm = localeComments.value(lang.left(cut));
            if (!comm.isEmpty())
                return comm;
        }
    }

    // Use the mimetype name as fallback
    return d->name;
}

/*!
    \property MimeType::genericIconName
    \brief the file name of a generic icon that represents the MIME type

    This should be used if the icon returned by iconName() cannot be found on
    the system. It is used for categories of similar types (like spreadsheets
    or archives) that can use a common icon.
    The freedesktop.org Icon Naming Specification lists a set of such icon names.

    The icon name can be given to QIcon::fromTheme() in order to load the icon.

    While this property was introduced in 5.10, the
    corresponding accessor method has always been there.
 */
QString MimeType::genericIconName() const
{
    QString genericIconName = MimeDatabasePrivate::instance()->genericIcon(d->name);
    if (genericIconName.isEmpty()) {
        // From the spec:
        // If the generic icon name is empty (not specified by the mimetype definition)
        // then the mimetype is used to generate the generic icon by using the top-level
        // media type (e.g.  "video" in "video/ogg") and appending "-x-generic"
        // (i.e. "video-x-generic" in the previous example).
        const QString group = name();
        QStringView groupRef(group);
        const qsizetype slashindex = groupRef.indexOf(u'/');
        if (slashindex != -1)
            groupRef = groupRef.left(slashindex);
        return groupRef + "-x-generic"_L1;
    }
    return genericIconName;
}

static QString make_default_icon_name_from_mimetype_name(QString iconName)
{
    const qsizetype slashindex = iconName.indexOf(u'/');
    if (slashindex != -1)
        iconName[slashindex] = u'-';
    return iconName;
}

/*!
    \property MimeType::iconName
    \brief the file name of an icon image that represents the MIME type

    The icon name can be given to QIcon::fromTheme() in order to load the icon.

    While this property was introduced in 5.10, the
    corresponding accessor method has always been there.
 */
QString MimeType::iconName() const
{
    QString iconName = MimeDatabasePrivate::instance()->icon(d->name);
    if (iconName.isEmpty()) {
        return make_default_icon_name_from_mimetype_name(name());
    }
    return iconName;
}

/*!
    \property MimeType::globPatterns
    \brief the list of glob matching patterns

    While this property was introduced in 5.10, the
    corresponding accessor method has always been there.
 */
QStringList MimeType::globPatterns() const
{
    return MimeDatabasePrivate::instance()->globPatterns(d->name);
}

/*!
    \property MimeType::parentMimeTypes
    \brief the names of parent MIME types

    A type is a subclass of another type if any instance of the first type is
    also an instance of the second. For example, all image/svg+xml files are also
    text/xml, text/plain and application/octet-stream files. Subclassing is about
    the format, rather than the category of the data (for example, there is no
    'generic spreadsheet' class that all spreadsheets inherit from).
    Conversely, the parent mimetype of image/svg+xml is text/xml.

    A mimetype can have multiple parents. For instance application/x-perl
    has two parents: application/x-executable and text/plain. This makes
    it possible to both execute perl scripts, and to open them in text editors.

    While this property was introduced in 5.10, the
    corresponding accessor method has always been there.
*/
QStringList MimeType::parentMimeTypes() const
{
    return MimeDatabasePrivate::instance()->mimeParents(d->name);
}

static void collectParentMimeTypes(const QString &mime, QStringList &allParents)
{
    const QStringList parents = MimeDatabasePrivate::instance()->mimeParents(mime);
    QStringList newParents;
    for (const QString &parent : parents) {
        // I would use QSet, but since order matters I better not
        if (!allParents.contains(parent)) {
            allParents.append(parent);
            newParents.append(parent);
        }
    }
    // We want a breadth-first search, so that the least-specific parent (octet-stream) is last
    // This means iterating twice, unfortunately.
    for (const QString &parent : newParents)
        collectParentMimeTypes(parent, allParents);
}

/*!
    \property MimeType::allAncestors
    \brief the names of direct and indirect parent MIME types

    Return all the parent mimetypes of this mimetype, direct and indirect.
    This includes the parent(s) of its parent(s), etc.

    For instance, for image/svg+xml the list would be:
    application/xml, text/plain, application/octet-stream.

    Note that application/octet-stream is the ultimate parent for all types
    of files (but not directories).

    While this property was introduced in 5.10, the
    corresponding accessor method has always been there.
*/
QStringList MimeType::allAncestors() const
{
    QStringList allParents;
    collectParentMimeTypes(d->name, allParents);
    return allParents;
}

/*!
    \property MimeType::aliases
    \brief the list of aliases of this mimetype

    For instance, for text/csv, the returned list would be:
    text/x-csv, text/x-comma-separated-values.

    Note that all MimeType instances refer to proper mimetypes,
    never to aliases directly.

    The order of the aliases in the list is undefined.

    While this property was introduced in 5.10, the
    corresponding accessor method has always been there.
*/
QStringList MimeType::aliases() const
{
    return MimeDatabasePrivate::instance()->listAliases(d->name);
}

/*!
    \property MimeType::suffixes
    \brief the known suffixes for the MIME type

    No leading dot is included, so for instance this would return "jpg", "jpeg" for image/jpeg.

    While this property was introduced in 5.10, the
    corresponding accessor method has always been there.
 */
QStringList MimeType::suffixes() const
{
    const QStringList patterns = globPatterns();

    QStringList result;
    result.reserve(patterns.size());
    for (const QString &pattern : patterns) {
        const QString suffix = suffixFromPattern(pattern);
        if (!suffix.isEmpty())
            result.append(suffix);
    }

    return result;
}

/*!
    \property MimeType::preferredSuffix
    \brief the preferred suffix for the MIME type

    No leading dot is included, so for instance this would return "pdf" for application/pdf.
    The return value can be empty, for mime types which do not have any suffixes associated.

    While this property was introduced in 5.10, the
    corresponding accessor method has always been there.
 */
QString MimeType::preferredSuffix() const
{
    if (isDefault()) // workaround for unwanted *.bin suffix for octet-stream, https://bugs.freedesktop.org/show_bug.cgi?id=101667, fixed upstream in 1.10
        return QString();
    const QStringList suffixList = suffixes();
    return suffixList.isEmpty() ? QString() : suffixList.at(0);
}

/*!
    \property MimeType::filterString
    \brief a filter string usable for a file dialog

    While this property was introduced in 5.10, the
    corresponding accessor method has always been there.
*/
QString MimeType::filterString() const
{
    const QStringList patterns = globPatterns();
    QString filter;

    if (!patterns.isEmpty()) {
        filter = comment() + " ("_L1 + patterns.join(u' ') + u')';
    }

    return filter;
}

/*!
    \fn bool MimeType::inherits(const QString &mimeTypeName) const;
    Returns \c true if this mimetype is \a mimeTypeName,
    or inherits \a mimeTypeName (see parentMimeTypes()),
    or \a mimeTypeName is an alias for this mimetype.

    This method has been made invokable from QML since 5.10.
 */
bool MimeType::inherits(const QString &mimeTypeName) const
{
    if (d->name == mimeTypeName)
        return true;
    return MimeDatabasePrivate::instance()->mimeInherits(d->name, mimeTypeName);
}

/*!
    Returns \c true if the name or alias of the MIME type matches
    \a nameOrAlias.
*/
bool MimeType::matchesName(const QString &nameOrAlias) const
{
    if (d->name == nameOrAlias)
        return true;
    auto dbp = MimeDatabasePrivate::instance();
    QMutexLocker locker(&dbp->mutex);
    return MimeDatabasePrivate::instance()->resolveAlias(nameOrAlias) == d->name;
}

/*!
    Sets the preferred filename suffix for the MIME type to \a suffix.
*/
void MimeType::setPreferredSuffix(const QString &suffix)
{
    MimeDatabasePrivate::instance()->setPreferredSuffix(d->name, suffix);
}

QDebug operator<<(QDebug debug, const Utils::MimeType &mime)
{
    QDebugStateSaver saver(debug);
    if (!mime.isValid()) {
        debug.nospace() << "MimeType(invalid)";
    } else {
        debug.nospace() << "MimeType(" << mime.name() << ")";
    }
    return debug;
}

} // namespace Utils


#include "moc_mimetype.cpp"
