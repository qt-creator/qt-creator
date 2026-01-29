/*
    SPDX-FileCopyrightText: 2018 Eike Hein <hein@kde.org>
    SPDX-FileCopyrightText: 2021 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: MIT
*/

#ifndef KQUICKSYNTAXHIGHLIGHTER_H
#define KQUICKSYNTAXHIGHLIGHTER_H

#include <KSyntaxHighlighting/Definition>
#include <KSyntaxHighlighting/Theme>

#include <QObject>
#include <QQmlEngine>
#include <QVariant>

namespace KSyntaxHighlighting
{
class Repository;
class SyntaxHighlighter;
}

/*!
 * \qmltype SyntaxHighlighter
 * \inqmlmodule org.kde.syntaxhighlighting
 * \brief Add syntax highlighting to QML text fields.
 *
 * Syntax highlighting may be added to a text field by specifying a \l textEdit id and a \l definition.
 *
 * The list of available definitions can be seen on the \l {https://kate-editor.org/syntax/} {Kate website}.
 *
 * You can learn more about syntax highlighting definitions on the \l {https://invent.kde.org/frameworks/syntax-highlighting} {syntax-highlighting repository}.
 *
 * A custom \l repository containing syntax highlighting definitions may be set. See \l KSyntaxHighlighting::Repository for details.
 *
 * A custom \l theme for the highlighting definitions may be set. The list of available themes can be seen on the
 * \l {https://invent.kde.org/frameworks/syntax-highlighting/-/tree/master/data/themes} {repository's data/themes/ folder}.
 * The name to be used should match the name specified in the JSON metadata, and is case sensitive. For example:
 *
 * \code
 * {
 *   "metadata": {
 *     "name": "GitHub Dark"
 *   }
 * }
 * \endcode
 *
 * Basic usage:
 *
 * \qml
 * import org.kde.syntaxhighlighting
 *
 * // ...
 *
 * SyntaxHighlighter {
 *     textEdit: sourceTextArea
 *     definition: "INI Files"
 *     theme: Repository.theme("GitHub Dark")
 * }
 * \endqml
 *
 * For more complex uses, the syntax definition might not be fixed but depend on input data (for example,
 * derived from its mimetype or file name), or a user selection. In the C++ API this is enabled
 * by the \l KSyntaxHighlighting::Repository class, which is available in QML as a singleton object.
 *
 * The following example shows how to use this for a simple syntax selection combobox:
 *
 * \qml
 * ComboBox {
 *    model: Repository.definitions
 *    displayText: currentValue.translatedName + " (" + currentValue.translatedSection + ")"
 *    textRole: "translatedName"
 *    onCurrentIndexChanged: myHighlighter.definition = currentValue
 * }
 * \endqml
 *
 * Handling color themes is also possible, similarly to syntax definitions. Themes can be listed,
 * their properties can be accessed and they can be set by theme object or name on the highlighter.
 * Like in the C++ API it's also possible to just ask for the light or dark default theme.
 *
 * More examples can be seen in the \l {https://invent.kde.org/frameworks/syntax-highlighting/-/tree/master/examples} {repository's examples/ folder}.
 */
class KQuickSyntaxHighlighter : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_NAMED_ELEMENT(SyntaxHighlighter)

    /*!
     * \qmlproperty QtObject SyntaxHighlighter::textEdit
     * \brief The id of the text edit component that will have syntax highlighting.
     */
    Q_PROPERTY(QObject *textEdit READ textEdit WRITE setTextEdit NOTIFY textEditChanged)

    /*!
     * \qmlproperty variant SyntaxHighlighter::definition
     * \brief The syntax highlighting definition to be used.
     * \sa {https://kate-editor.org/syntax/} {Available Syntax Highlighting language names}
     */
    Q_PROPERTY(QVariant definition READ definition WRITE setDefinition NOTIFY definitionChanged)

    /*!
     * \qmlproperty variant SyntaxHighlighter::theme
     * \brief The theme to be used for the syntax highlighting.
     *
     * The theme name to be used should match the name specified in the JSON metadata, and is case sensitive.
     *
     * The Repository singleton exposes the API from \l KSyntaxHighlighting::Repository
     * that is necessary to manage themes in your application:
     *
     * \list
     * \li Repository.themes
     * \li Repository.theme
     * \li Repository.defaultTheme()
     * \li Repository.DefaultTheme
     * \endlist
     */
    Q_PROPERTY(QVariant theme READ theme WRITE setTheme NOTIFY themeChanged)

    /*!
     * \qmlproperty Repository SyntaxHighlighter::repository
     * \brief The repository from where syntax highlighting definitions will be pulled.
     *
     * Defaults to the default repository provided by the KSyntaxHighlighting framework.
     *
     * The selected repository can be accessed via the Repository singleton,
     * which exposes the API from \l KSyntaxHighlighting::Repository.
     *
     * The most commonly accessed API is:
     *
     * \list
     * \li Repository.definitions
     * \li Repository.definition
     * \li Repository.definitionForName()
     * \li Repository.definitionForFileName()
     * \li Repository.themes
     * \li Repository.theme
     * \li Repository.defaultTheme()
     * \li Repository.DefaultTheme
     * \endlist
     * \sa {https://invent.kde.org/frameworks/syntax-highlighting/-/blob/master/examples/qml/example.qml} {QML Example}
     */
    Q_PROPERTY(KSyntaxHighlighting::Repository *repository READ repository WRITE setRepository NOTIFY repositoryChanged)

public:
    explicit KQuickSyntaxHighlighter(QObject *parent = nullptr);
    ~KQuickSyntaxHighlighter() override;

    QObject *textEdit() const;
    void setTextEdit(QObject *textEdit);

    QVariant definition() const;
    void setDefinition(const QVariant &definition);

    QVariant theme() const;
    void setTheme(const QVariant &theme);

    KSyntaxHighlighting::Repository *repository() const;
    void setRepository(KSyntaxHighlighting::Repository *repository);

Q_SIGNALS:
    void textEditChanged() const;
    void definitionChanged() const;
    void themeChanged();
    void repositoryChanged();

private:
    KSyntaxHighlighting::Repository *unwrappedRepository() const;

    QObject *m_textEdit;
    KSyntaxHighlighting::Definition m_definition;
    KSyntaxHighlighting::Theme m_theme;
    KSyntaxHighlighting::Repository *m_repository = nullptr;
    KSyntaxHighlighting::SyntaxHighlighter *m_highlighter = nullptr;
};

#endif // KQUICKSYNTAXHIGHLIGHTER_H
