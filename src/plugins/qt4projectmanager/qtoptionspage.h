/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://www.qtsoftware.com/contact.
**
**************************************************************************/
#ifndef QTOPTIONSPAGE_H
#define QTOPTIONSPAGE_H

#include <coreplugin/dialogs/ioptionspage.h>

#include <QtGui/QWidget>
#include <QtGui/QPixmap>
#include <QtGui/QIcon>

QT_BEGIN_NAMESPACE
class QTreeWidgetItem;
QT_END_NAMESPACE

namespace Qt4ProjectManager {

class QtVersion;

namespace Internal {
namespace Ui {
class QtVersionManager;
}

// A task suitable to be run by QtConcurrent to build the helpers.
// It may outlive the settings page if someone quickly cancels it,
// so, it maintains a copy of the QtVersion and emits finished() by name.
class DebuggingHelperBuildTask : public QObject {
    Q_OBJECT
public:
    explicit DebuggingHelperBuildTask(const QtVersion &version);
    virtual ~DebuggingHelperBuildTask();

    void run();

signals:
    void finished(const QString &versionName, const QString &output);

private:
    QtVersion *m_version;
};

class QtOptionsPageWidget : public QWidget
{
    Q_OBJECT
public:
    QtOptionsPageWidget(QWidget *parent, QList<QtVersion *> versions, QtVersion *defaultVersion);
    ~QtOptionsPageWidget();
    QList<QtVersion *> versions() const;
    int defaultVersion() const;
    void finish();

private:
    const QPixmap m_debuggingHelperOkPixmap;
    const QPixmap m_debuggingHelperErrorPixmap;
    const QIcon m_debuggingHelperOkIcon;
    const QIcon m_debuggingHelperErrorIcon;

    void showEnvironmentPage(QTreeWidgetItem * item);
    void fixQtVersionName(int index);
    int indexForTreeItem(const QTreeWidgetItem *item) const;
    QTreeWidgetItem *treeItemForIndex(int index) const;
    QtVersion *currentVersion() const;
    void updateDebuggingHelperStateLabel(const QtVersion *version = 0);

    Internal::Ui::QtVersionManager *m_ui;
    QList<QtVersion *> m_versions;
    int m_defaultVersion;
    QString m_specifyNameString;
    QString m_specifyPathString;

private slots:
    void versionChanged(QTreeWidgetItem *item, QTreeWidgetItem *old);
    void addQtDir();
    void removeQtDir();
    void updateState();
    void makeMingwVisible(bool visible);
    void makeMSVCVisible(bool visible);
    void makeMWCVisible(bool visible);
    void onQtBrowsed();
    void onMingwBrowsed();
    void defaultChanged(int index);
    void updateCurrentQtName();
    void updateCurrentQtPath();
    void updateCurrentMingwDirectory();
#ifdef QTCREATOR_WITH_S60
    void updateCurrentMwcDirectory();
#endif
    void msvcVersionChanged();
    void buildDebuggingHelper();
    void showDebuggingBuildLog();
    void debuggingHelperBuildFinished(const QString &name, const QString &output);
};

class QtOptionsPage : public Core::IOptionsPage
{
    Q_OBJECT
public:
    QtOptionsPage();
    ~QtOptionsPage();
    QString id() const;
    QString trName() const;
    QString category() const;
    QString trCategory() const;
    QWidget *createPage(QWidget *parent);
    void apply();
    void finish() { }
private:
    QtOptionsPageWidget *m_widget;    
};

} //namespace Internal
} //namespace Qt4ProjectManager


#endif // QTOPTIONSPAGE_H
