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

#pragma once

#include "fancylineedit.h"
#include "fileutils.h"

#include <QWidget>

QT_BEGIN_NAMESPACE
class QAbstractButton;
class QLineEdit;
QT_END_NAMESPACE


namespace Utils {

class FancyLineEdit;
class Environment;
class PathChooserPrivate;

class QTCREATOR_UTILS_EXPORT PathChooser : public QWidget
{
    Q_OBJECT
    Q_ENUMS(Kind)
    Q_PROPERTY(QString path READ path WRITE setPath NOTIFY pathChanged DESIGNABLE true)
    Q_PROPERTY(QString promptDialogTitle READ promptDialogTitle WRITE setPromptDialogTitle DESIGNABLE true)
    Q_PROPERTY(QString promptDialogFilter READ promptDialogFilter WRITE setPromptDialogFilter DESIGNABLE true)
    Q_PROPERTY(Kind expectedKind READ expectedKind WRITE setExpectedKind DESIGNABLE true)
    Q_PROPERTY(QString baseDirectory READ baseDirectory WRITE setBaseDirectory DESIGNABLE true)
    Q_PROPERTY(QStringList commandVersionArguments READ commandVersionArguments WRITE setCommandVersionArguments)
    Q_PROPERTY(bool readOnly READ isReadOnly WRITE setReadOnly DESIGNABLE true)
    // Designer does not know this type, so force designable to false:
    Q_PROPERTY(Utils::FileName fileName READ fileName WRITE setFileName DESIGNABLE false)
    Q_PROPERTY(Utils::FileName baseFileName READ baseFileName WRITE setBaseFileName DESIGNABLE false)
    Q_PROPERTY(QColor errorColor READ errorColor WRITE setErrorColor DESIGNABLE true)
    Q_PROPERTY(QColor okColor READ okColor WRITE setOkColor DESIGNABLE true)

public:
    static QString browseButtonLabel();

    explicit PathChooser(QWidget *parent = nullptr);
    virtual ~PathChooser();

    enum Kind {
        ExistingDirectory,
        Directory, // A directory, doesn't need to exist
        File,
        SaveFile,
        ExistingCommand, // A command that must exist at the time of selection
        Command, // A command that may or may not exist at the time of selection (e.g. result of a build)
        Any
    };

    // Default is <Directory>
    void setExpectedKind(Kind expected);
    Kind expectedKind() const;

    void setPromptDialogTitle(const QString &title);
    QString promptDialogTitle() const;

    void setPromptDialogFilter(const QString &filter);
    QString promptDialogFilter() const;

    void setInitialBrowsePathBackup(const QString &path);

    bool isValid() const;
    QString errorMessage() const;

    QString path() const;
    QString rawPath() const; // The raw unexpanded input.
    FileName rawFileName() const; // The raw unexpanded input.
    FileName fileName() const;

    static QString expandedDirectory(const QString &input, const Utils::Environment &env,
                                     const QString &baseDir);

    QString baseDirectory() const;
    void setBaseDirectory(const QString &directory);

    FileName baseFileName() const;
    void setBaseFileName(const FileName &base);

    void setEnvironment(const Environment &env);

    /** Returns the suggested label title when used in a form layout. */
    static QString label();

    FancyLineEdit::ValidationFunction defaultValidationFunction() const;
    void setValidationFunction(const FancyLineEdit::ValidationFunction &fn);

    /** Return the home directory, which needs some fixing under Windows. */
    static QString homePath();

    void addButton(const QString &text, QObject *context, const std::function<void()> &callback);
    void insertButton(int index, const QString &text, QObject *context, const std::function<void()> &callback);
    QAbstractButton *buttonAtIndex(int index) const;

    FancyLineEdit *lineEdit() const;

    // For PathChoosers of 'Command' type, this property specifies the arguments
    // required to obtain the tool version (commonly, '--version'). Setting them
    // causes the version to be displayed as a tooltip.
    QStringList commandVersionArguments() const;
    void setCommandVersionArguments(const QStringList &arguments);

    // Utility to run a tool and return its stdout.
    static QString toolVersion(const QString &binary, const QStringList &arguments);
    // Install a tooltip on lineedits used for binaries showing the version.
    static void installLineEditVersionToolTip(QLineEdit *le, const QStringList &arguments);

    // Enable a history completer with a history of entries.
    void setHistoryCompleter(const QString &historyKey, bool restoreLastItemFromHistory = false);

    bool isReadOnly() const;
    void setReadOnly(bool b);

    void triggerChanged();

    // global handler for adding context menus to ALL pathchooser
    // used by the coreplugin to add "Open in Terminal" and "Open in Explorer" context menu actions
    using AboutToShowContextMenuHandler = std::function<void (Utils::PathChooser *, QMenu *)>;
    static void setAboutToShowContextMenuHandler(AboutToShowContextMenuHandler handler);

    QColor errorColor() const;
    QColor okColor() const;

private:
    bool validatePath(FancyLineEdit *edit, QString *errorMessage) const;
    // Returns overridden title or the one from <title>
    QString makeDialogTitle(const QString &title);
    void slotBrowse();
    void contextMenuRequested(const QPoint &pos);

signals:
    void validChanged(bool validState);
    void rawPathChanged(const QString &text);
    void pathChanged(const QString &path);
    void editingFinished();
    void beforeBrowsing();
    void browsingFinished();
    void returnPressed();

public slots:
    void setPath(const QString &);
    void setFileName(const Utils::FileName &);

    void setErrorColor(const QColor &errorColor);
    void setOkColor(const QColor &okColor);

private:
    PathChooserPrivate *d = nullptr;
    static AboutToShowContextMenuHandler s_aboutToShowContextMenuHandler;
};

} // namespace Utils
