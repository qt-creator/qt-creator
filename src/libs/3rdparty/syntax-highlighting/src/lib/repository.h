/*
    SPDX-FileCopyrightText: 2016 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: MIT
*/

#ifndef KSYNTAXHIGHLIGHTING_REPOSITORY_H
#define KSYNTAXHIGHLIGHTING_REPOSITORY_H

#include "ksyntaxhighlighting_export.h"

#include <memory>
#include <qglobal.h>
#include <qvector.h>

QT_BEGIN_NAMESPACE
class QString;
class QPalette;
QT_END_NAMESPACE

/**
 * @namespace KSyntaxHighlighting
 *
 * Syntax highlighting engine for Kate syntax definitions.
 * In order to access the syntax highlighting Definition files, use the
 * class Repository.
 *
 * @see Repository
 */
namespace KSyntaxHighlighting
{
class Definition;
class RepositoryPrivate;
class Theme;

/**
 * @brief Syntax highlighting repository.
 *
 * @section repo_intro Introduction
 *
 * The Repository gives access to all syntax Definitions available on the
 * system, typically described in *.xml files. The Definition files are read
 * from the resource that is compiled into the executable, and from the file
 * system. If a Definition exists in the resource and on the file system,
 * then the one from the file system is chosen.
 *
 * @section repo_access Definitions and Themes
 *
 * Typically, only one instance of the Repository is needed. This single
 * instance can be thought of as a singleton you keep alive throughout the
 * lifetime of your application. Then, either call definitionForName() with the
 * given language name (e.g. "QML" or "Java"), or definitionForFileName() to
 * obtain a Definition based on the filename/mimetype of the file. The
 * function definitions() returns a list of all available syntax Definition%s.
 *
 * In addition to Definitions, the Repository also provides a list of Themes.
 * A Theme is defined by a set of default text style colors as well as editor
 * colors. These colors together provide all required colros for drawing all
 * primitives of a text editor. All available Theme%s can be queried through
 * themes(), and a Theme with a specific name is obtained through theme().
 * Additionally, defaultTheme() provides a way to obtain a default theme for
 * either a light or a black color theme.
 *
 * @section repo_search_paths Search Paths
 *
 * All highlighting Definition and Theme files are compiled into the shared
 * KSyntaxHighlighting library by using the Qt resource system. Loading
 * additional files from disk is supported as well.
 *
 * Loading syntax Definition files works as follows:
 *
 * -# First, all syntax highlighting files are loaded that are located in
 *    QStandardPaths::locateAll(QStandardPaths::GenericDataLocation, QStringLiteral("org.kde.syntax-highlighting/syntax"), QStandardPaths::LocateDirectory);
 *    Under Unix, this uses $XDG_DATA_HOME and $XDG_DATA_DIRS, which could
 *    map to $HOME/.local5/share/org.kde.syntax-highlighting/syntax and
 *    /usr/share/org.kde.syntax-highlighting/syntax.
 *
 * -# Next, for backwards compatibility with Kate, all syntax highlighting
 *    files are loaded that are located in
 *    QStandardPaths::locateAll(QStandardPaths::GenericDataLocation, QStringLiteral("katepart5/syntax"), QStandardPaths::LocateDirectory);
 *    Again, under Unix, this uses $XDG_DATA_HOME and $XDG_DATA_DIRS, which
 *    could map to $HOME/.local5/share/katepart5/syntax and
 *    /usr/share/katepart5/syntax.
 *
 * -# Then, all files compiled into the library through resources are loaded.
 *    The internal resource path is ":/org.kde.syntax-highlighting/syntax".
 *    This path should never be touched by other applications.
 *
 * -# Finally, the search path can be extended by calling addCustomSearchPath().
 *    A custom search path can either be a path on disk or again a path to
 *    a Qt resource.
 *
 * Similarly, loading Theme files works as follows:
 *
 * -# First, all Theme files are loaded that are located in
 *    QStandardPaths::locateAll(QStandardPaths::GenericDataLocation, QStringLiteral("org.kde.syntax-highlighting/themes"), QStandardPaths::LocateDirectory);
 *    Under Unix, this uses $XDG_DATA_HOME and $XDG_DATA_DIRS, which could
 *    map to $HOME/.local5/share/org.kde.syntax-highlighting/themes and
 *    /usr/share/org.kde.syntax-highlighting/themes.
 *
 * -# Then, all files compiled into the library through resources are loaded.
 *    The internal resource path is ":/org.kde.syntax-highlighting/themes".
 *    This path should never be touched by other applications.
 *
 * -# Finally, all Theme%s located in the paths added addCustomSearchPath()
 *    are loaded.
 *
 * @note Whenever a Definition or a Theme exists twice, the variant with
 *       higher version is used.
 *
 * @note The QStandardPaths lookup can be disabled by compiling the framework with the -DNO_STANDARD_PATHS define.
 *
 * @see Definition, Theme, AbstractHighlighter
 * @since 5.28
 */
class KSYNTAXHIGHLIGHTING_EXPORT Repository
{
public:
    /**
     * Create a new syntax definition repository.
     * This will read the meta data information of all available syntax
     * definition, which is a moderately expensive operation, it's therefore
     * recommended to keep a single instance of Repository around as long
     * as you need highlighting in your application.
     */
    Repository();
    ~Repository();

    /**
     * Returns the Definition named @p defName.
     *
     * If no Definition is found, Definition::isValid() of the returned instance
     * returns false.
     *
     * @note This uses case sensitive, untranslated names. For instance,
     *       the javascript.xml definition file sets its name to @e JavaScript.
     *       Therefore, only the string "JavaScript" will return a valid
     *       Definition file.
     */
    Definition definitionForName(const QString &defName) const;

    /**
     * Returns the best matching Definition for the file named @p fileName.
     * The match is performed based on the \e extensions and @e mimetype of
     * the definition files. If multiple matches are found, the one with the
     * highest priority is returned.
     *
     * If no match is found, Definition::isValid() of the returned instance
     * returns false.
     */
    Definition definitionForFileName(const QString &fileName) const;

    /**
     * Returns all Definition%s for the file named @p fileName sorted by priority.
     * The match is performed based on the \e extensions and @e mimetype of
     * the definition files.
     *
     * @since 5.56
     */
    QVector<Definition> definitionsForFileName(const QString &fileName) const;

    /**
     * Returns the best matching Definition to the type named @p mimeType
     *
     * If no match is found, Definition::isValid() of the returned instance
     * returns false.
     *
     * @since 5.50
     */
    Definition definitionForMimeType(const QString &mimeType) const;

    /**
     * Returns all Definition%s to the type named @p mimeType sorted by priority
     *
     * @since 5.56
     */
    QVector<Definition> definitionsForMimeType(const QString &mimeType) const;

    /**
     * Returns all available Definition%s.
     * Definition%ss are ordered by translated section and translated names,
     * for consistent displaying.
     */
    QVector<Definition> definitions() const;

    /**
     * Returns all available color themes.
     * The returned list should never be empty.
     */
    QVector<Theme> themes() const;

    /**
     * Returns the theme called @p themeName.
     * If the requested theme cannot be found, the retunred Theme is invalid,
     * see Theme::isValid().
     */
    Theme theme(const QString &themeName) const;

    /**
     * Built-in default theme types.
     * @see defaultTheme()
     */
    enum DefaultTheme {
        //! Theme with a light background color.
        LightTheme,
        //! Theme with a dark background color.
        DarkTheme
    };

    /**
     * Returns a default theme instance of the given type.
     * The returned Theme is guaranteed to be a valid theme.
     * @since 5.79
     */
    Theme defaultTheme(DefaultTheme t = LightTheme) const;

    /**
     * Returns a default theme instance of the given type.
     * The returned Theme is guaranteed to be a valid theme.
     *
     * KF6: remove in favor of const variant
     */
    Theme defaultTheme(DefaultTheme t = LightTheme);

    /**
     * Returns the best matching theme for the given palette
     * @since 5.79
     **/
    Theme themeForPalette(const QPalette &palette) const;

    /**
     * Returns the best matching theme for the given palette
     * @since 5.77
     *
     * KF6: remove in favor of const variant
     **/
    Theme themeForPalette(const QPalette &palette);

    /**
     * Reloads the repository.
     * This is a moderately expensive operations and should thus only be
     * triggered when the installed syntax definition files changed.
     */
    void reload();

    /**
     * Add a custom search path to the repository.
     * This path will be searched in addition to the usual locations for syntax
     * and theme definition files. Both locations on disk as well as Qt
     * resource paths are supported.
     *
     * @note Internally, the two sub-folders @p path/syntax as well as
     *       @p path/themes are searched for additional Definition%s and
     *       Theme%s. Do not append @e syntax or @e themes to @p path
     *       yourself.
     *
     * @note Calling this triggers a reload() of the repository.
     *
     * @since 5.39
     */
    void addCustomSearchPath(const QString &path);

    /**
     * Returns the list of custom search paths added to the repository.
     * By default, this list is empty.
     *
     * @see addCustomSearchPath()
     * @since 5.39
     */
    QVector<QString> customSearchPaths() const;

private:
    Q_DISABLE_COPY(Repository)
    friend class RepositoryPrivate;
    std::unique_ptr<RepositoryPrivate> d;
};

}

#endif // KSYNTAXHIGHLIGHTING_REPOSITORY_H
