// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "utils_global.h"

#include "fancylineedit.h"
#include "filepath.h"

#include <QWidget>

QT_BEGIN_NAMESPACE
class QAbstractButton;
class QLineEdit;
QT_END_NAMESPACE

namespace Utils {

class CommandLine;
class MacroExpander;
class Environment;
class PathChooserPrivate;

class QTCREATOR_UTILS_EXPORT PathChooser : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(QString path READ path WRITE setPath NOTIFY textChanged DESIGNABLE true)
    Q_PROPERTY(QString promptDialogTitle READ promptDialogTitle WRITE setPromptDialogTitle DESIGNABLE true)
    Q_PROPERTY(QString promptDialogFilter READ promptDialogFilter WRITE setPromptDialogFilter DESIGNABLE true)
    Q_PROPERTY(Kind expectedKind READ expectedKind WRITE setExpectedKind DESIGNABLE true)
    Q_PROPERTY(Utils::FilePath baseDirectory READ baseDirectory WRITE setBaseDirectory DESIGNABLE true)
    Q_PROPERTY(QStringList commandVersionArguments READ commandVersionArguments WRITE setCommandVersionArguments)
    Q_PROPERTY(bool readOnly READ isReadOnly WRITE setReadOnly DESIGNABLE true)
    // Designer does not know this type, so force designable to false:
    Q_PROPERTY(Utils::FilePath filePath READ filePath WRITE setFilePath DESIGNABLE false)

public:
    static QString browseButtonLabel();

    explicit PathChooser(QWidget *parent = nullptr);
    ~PathChooser() override;

    enum Kind {
        ExistingDirectory,
        Directory, // A directory, doesn't need to exist
        File,
        SaveFile,
        ExistingCommand, // A command that must exist at the time of selection
        Command, // A command that may or may not exist at the time of selection (e.g. result of a build)
        Any
    };
    Q_ENUM(Kind)

    // Default is <ExistingDirectory>
    void setExpectedKind(Kind expected);
    Kind expectedKind() const;

    void setPromptDialogTitle(const QString &title);
    QString promptDialogTitle() const;

    void setPromptDialogFilter(const QString &filter);
    QString promptDialogFilter() const;

    void setInitialBrowsePathBackup(const FilePath &path);

    bool isValid() const;
    QString errorMessage() const;

    FilePath filePath() const; // Close to what's in the line edit.
    FilePath absoluteFilePath() const; // Relative paths resolved wrt the specified base dir.

    FilePath rawFilePath() const; // The raw unexpanded input as FilePath.

    FilePath baseDirectory() const;
    void setBaseDirectory(const FilePath &base);

    void setEnvironment(const Environment &env);

    /** Returns the suggested label title when used in a form layout. */
    static QString label();

    FancyLineEdit::AsyncValidationFunction defaultValidationFunction() const;
    void setValidationFunction(const FancyLineEdit::ValidationFunction &fn);

    /** Return the home directory, which needs some fixing under Windows. */
    static FilePath homePath();

    void addButton(const QString &text, QObject *context, const std::function<void()> &callback);
    void insertButton(int index, QAbstractButton *button);
    void insertButton(int index, const QString &text, QObject *context, const std::function<void()> &callback);
    QAbstractButton *buttonAtIndex(int index) const;

    FancyLineEdit *lineEdit() const;

    // For PathChoosers of 'Command' type, this property specifies the arguments
    // required to obtain the tool version (commonly, '--version'). Setting them
    // causes the version to be displayed as a tooltip.
    QStringList commandVersionArguments() const;
    void setCommandVersionArguments(const QStringList &arguments);

    // Utility to run a tool and return its stdout.
    static QString toolVersion(const CommandLine &cmd);
    // Install a tooltip on lineedits used for binaries showing the version.
    static void installLineEditVersionToolTip(QLineEdit *le, const QStringList &arguments);

    // Enable a history completer with a history of entries.
    void setHistoryCompleter(const Key &historyKey, bool restoreLastItemFromHistory = false);

    // Sets a macro expander that is used when producing path and fileName.
    // By default, the global expander is used.
    // nullptr can be passed to disable macro expansion.
    void setMacroExpander(const MacroExpander *macroExpander);

    bool isReadOnly() const;
    void setReadOnly(bool b);

    void triggerChanged();

    // global handler for adding context menus to ALL pathchooser
    // used by the coreplugin to add "Open in Terminal" and "Open in Explorer" context menu actions
    using AboutToShowContextMenuHandler = std::function<void (PathChooser *, QMenu *)>;
    static void setAboutToShowContextMenuHandler(AboutToShowContextMenuHandler handler);

    void setOpenTerminalHandler(const std::function<void()> &openTerminal);
    std::function<void()> openTerminalHandler() const;

    // this sets the placeHolderText to defaultValue and enables to use this as
    // input value during validation if the real value is empty
    // setting an empty QString will disable this and clear the placeHolderText
    void setDefaultValue(const QString &defaultValue);
    void setPlaceholderText(const QString &placeholderText);
    void setToolTip(const QString &toolTip);

    void setAllowPathFromDevice(bool allow);
    bool allowPathFromDevice() const;

public slots:
    void setPath(const QString &);
    void setFilePath(const FilePath &);

signals:
    void validChanged(bool validState);
    void rawPathChanged();
    void textChanged(const QString &text); // Triggered from the line edit's textChanged()
    void editingFinished(); // Triggered from the line edit's editingFinished()
    void beforeBrowsing();
    void browsingFinished();
    void returnPressed();

private:
    // Deprecated, only used in property getter.
    // Use filePath().toString() or better suitable conversions.
    QString path() const { return filePath().toString(); }

    // Returns overridden title or the one from <title>
    QString makeDialogTitle(const QString &title);
    void slotBrowse(bool remote);
    void contextMenuRequested(const QPoint &pos);

    PathChooserPrivate *d = nullptr;
    static AboutToShowContextMenuHandler s_aboutToShowContextMenuHandler;
};

} // namespace Utils
