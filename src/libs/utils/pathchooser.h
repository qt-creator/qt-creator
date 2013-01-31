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

#ifndef PATHCHOOSER_H
#define PATHCHOOSER_H

#include "utils_global.h"

#include "fileutils.h"
#include "fancylineedit.h"

#include <QWidget>

QT_BEGIN_NAMESPACE
class QAbstractButton;
class QLineEdit;
QT_END_NAMESPACE


namespace Utils {

class Environment;
class PathChooserPrivate;

class QTCREATOR_UTILS_EXPORT PathChooser : public QWidget
{
    Q_OBJECT
    Q_ENUMS(Kind)
    Q_PROPERTY(QString path READ path WRITE setPath DESIGNABLE true)
    Q_PROPERTY(QString promptDialogTitle READ promptDialogTitle WRITE setPromptDialogTitle DESIGNABLE true)
    Q_PROPERTY(Kind expectedKind READ expectedKind WRITE setExpectedKind DESIGNABLE true)
    Q_PROPERTY(QString baseDirectory READ baseDirectory WRITE setBaseDirectory DESIGNABLE true)
    Q_PROPERTY(QStringList commandVersionArguments READ commandVersionArguments WRITE setCommandVersionArguments)
    Q_PROPERTY(bool readOnly READ isReadOnly WRITE setReadOnly DESIGNABLE true)
    // Designer does not know this type, so force designable to false:
    Q_PROPERTY(Utils::FileName fileName READ fileName WRITE setFileName DESIGNABLE false)
    Q_PROPERTY(Utils::FileName baseFileName READ baseFileName WRITE setBaseFileName DESIGNABLE false)

public:
    static const char * const browseButtonLabel;

    explicit PathChooser(QWidget *parent = 0);
    virtual ~PathChooser();

    enum Kind {
        ExistingDirectory,
        Directory, // A directory, doesn't need to exist
        File,
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
    Utils::FileName fileName() const;

    QString baseDirectory() const;
    void setBaseDirectory(const QString &directory);
    Utils::FileName baseFileName() const;
    void setBaseFileName(const Utils::FileName &base);

    void setEnvironment(const Utils::Environment &env);

    /** Returns the suggested label title when used in a form layout. */
    static QString label();

    virtual bool validatePath(const QString &path, QString *errorMessage = 0);

    /** Return the home directory, which needs some fixing under Windows. */
    static QString homePath();

    void addButton(const QString &text, QObject *receiver, const char *slotFunc);
    void insertButton(int index, const QString &text, QObject *receiver, const char *slotFunc);
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

    bool isReadOnly() const;
    void setReadOnly(bool b);

    void triggerChanged();
private:
    // Returns overridden title or the one from <title>
    QString makeDialogTitle(const QString &title);

signals:
    void validChanged();
    void validChanged(bool validState);
    void changed(const QString &text);
    void editingFinished();
    void beforeBrowsing();
    void browsingFinished();
    void returnPressed();

public slots:
    void setPath(const QString &);
    void setFileName(const Utils::FileName &);

private slots:
    void slotBrowse();

private:
    PathChooserPrivate *d;
};

} // namespace Utils


#endif // PATHCHOOSER_H
