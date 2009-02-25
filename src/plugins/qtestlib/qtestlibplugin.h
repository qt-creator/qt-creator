/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
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
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#ifndef QTESTLIBPLUGIN_H
#define QTESTLIBPLUGIN_H

#include <coreplugin/ioutputpane.h>
#include <extensionsystem/pluginmanager.h>
#include <projectexplorer/projectexplorer.h>

#include <QtGui/QPixmap>
#include <QtGui/QSortFilterProxyModel>
#include <QtGui/QStandardItem>
#include <QtGui/QWidget>

QT_BEGIN_NAMESPACE
class QComboBox;
class QStandardItemModel;
class QTextEdit;
class QTreeView;
QT_END_NAMESPACE

namespace QTestLib {
namespace Internal {

class QTestLibPlugin;
class QTestOutputWidget;

struct QTestLocation
{
    QString file;
    QString line;
};

class QTestFunction : public QStandardItem
{
public:
    enum IncidentType {
        Pass,
        XFail,
        Fail,
        XPass
    };

    enum MessageType {
        Warning,
        QWarning,
        QDebug,
        QSystem,
        QFatal,
        Skip,
        Info
    };

    inline QTestFunction(const QString &name)
        : QStandardItem(name) {
        setColumnCount(2);
        // ### hardcoding colors sucks...
        setForeground(Qt::darkBlue);
    }

    void addIncident(IncidentType type,
                     const QString &file = QString(),
                     const QString &line = QString(),
                     const QString &details = QString());

    void addMessage(MessageType type, const QString &text);

    static bool indexHasIncidents(const QModelIndex &function, IncidentType type);
};

class QTestOutputPane : public Core::IOutputPane
{
    Q_OBJECT

public:
    QTestOutputPane(QTestLibPlugin *plugin);

    void addFunction(QTestFunction *function);

    virtual QWidget *outputWidget(QWidget *parent);
    QList<QWidget*> toolBarWidgets(void) const { return QList<QWidget *>(); }
    virtual QString name() const;

    virtual void clearContents();
    virtual void visibilityChanged(bool visible);

    void show();

    // FIXME:
    virtual int priorityInStatusBar() const { return 0;}
    virtual void setFocus() {}
    virtual bool hasFocus() { return false;}
    virtual bool canFocus() { return false;}

signals:
    void showPage();

private:
    QTestLibPlugin *m_plugin;
    QTestOutputWidget *m_widget;
    QStandardItemModel *m_model;
};

class QTestOutputFilter : public QSortFilterProxyModel
{
public:
    inline QTestOutputFilter(QObject *parent)
        : QSortFilterProxyModel(parent), m_filter(QTestFunction::Fail)
    {}

    inline void setIncidentFilter(QTestFunction::IncidentType incident) {
        m_filter = incident;
        filterChanged();
    }

protected:
    virtual bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const;

private:
    QTestFunction::IncidentType m_filter;
};

class QTestOutputWidget : public QWidget
{
    Q_OBJECT

public:
    QTestOutputWidget(QStandardItemModel *model, QWidget *parent);

    void expand();

private Q_SLOTS:
    void activateComboFilter(int index);
    void gotoLocation(QModelIndex index);

private:
    QStandardItemModel *m_model;
    QTreeView *m_resultsView;
    QComboBox *m_filterCombo;
    QTestOutputFilter *m_filterModel;
};

class QTestLibPlugin : public QObject
{
    Q_OBJECT

public:
    QTestLibPlugin();
    virtual ~QTestLibPlugin();

    bool init(const QStringList &args, QString *error_message);
    void extensionsInitialized();

    // IApplicationOutput
    virtual void clear();
    virtual void appendOutput(const QString &out);
    virtual void processExited(int exitCode);

private slots:
    void projectRunHook(ProjectExplorer::Project *project);

private:
    ProjectExplorer::ProjectExplorerPlugin *m_projectExplorer;
    QString m_outputFile;
    QString m_projectDirectory;
    QTestOutputPane *m_outputPane;
};

} // namespace Internal
} // namespace QTestLibPlugin

Q_DECLARE_METATYPE(QTestLib::Internal::QTestLocation)
Q_DECLARE_METATYPE(QTestLib::Internal::QTestFunction::IncidentType)
Q_DECLARE_METATYPE(QTestLib::Internal::QTestFunction::MessageType)

#endif // QTESTLIBPLUGIN_H
