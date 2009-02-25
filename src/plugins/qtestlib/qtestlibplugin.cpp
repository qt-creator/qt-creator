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

#include "qtestlibplugin.h"

#include <coreplugin/icore.h>
#include <texteditor/itexteditor.h>

#include <QtCore/QDebug>
#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QTemporaryFile>
#include <QtCore/QtPlugin>
#include <QtGui/QAction>
#include <QtGui/QComboBox>
#include <QtGui/QHeaderView>
#include <QtGui/QIcon>
#include <QtGui/QKeySequence>
#include <QtGui/QLabel>
#include <QtGui/QSplitter>
#include <QtGui/QStandardItemModel>
#include <QtGui/QTextEdit>
#include <QtGui/QTreeView>
#include <QtGui/QVBoxLayout>
#include <QtXml/QDomDocument>

using namespace QTestLib::Internal;

static QString incidentString(QTestFunction::IncidentType type)
{
    static QMap<QTestFunction::IncidentType, QString> strings;
    if (strings.empty()) {
        strings.insert(QTestFunction::Pass,  QObject::tr("Pass"));
        strings.insert(QTestFunction::XFail, QObject::tr("Expected Failure"));
        strings.insert(QTestFunction::Fail,  QObject::tr("Failure"));
        strings.insert(QTestFunction::XPass, QObject::tr("Expected Pass"));
    }
    return strings.value(type, QString());
}

static QString messageString(QTestFunction::MessageType type)
{
    static QMap<QTestFunction::MessageType,  QString> strings;
    if (strings.empty()) {
        strings.insert(QTestFunction::Warning,  QObject::tr("Warning"));
        strings.insert(QTestFunction::QWarning, QObject::tr("Qt Warning"));
        strings.insert(QTestFunction::QDebug,   QObject::tr("Qt Debug"));
        strings.insert(QTestFunction::QSystem,  QObject::tr("Critical"));
        strings.insert(QTestFunction::QFatal,   QObject::tr("Fatal"));
        strings.insert(QTestFunction::Skip,     QObject::tr("Skipped"));
        strings.insert(QTestFunction::Info,     QObject::tr("Info"));
    }
    return strings.value(type, QString());
}

static QTestFunction::IncidentType stringToIncident(const QString &str)
{
    if (str == QLatin1String("pass"))
        return QTestFunction::Pass;
    else if (str == QLatin1String("fail"))
        return QTestFunction::Fail;
    else if (str == QLatin1String("xfail"))
        return QTestFunction::XFail;
    else if (str == QLatin1String("xpass"))
        return QTestFunction::XPass;
    return QTestFunction::Fail; // ...
}

static QTestFunction::MessageType stringToMessageType(const QString &str)
{
    if (str == QLatin1String("warn"))
        return QTestFunction::Warning;
    else if (str == QLatin1String("system"))
        return QTestFunction::QSystem;
    else if (str == QLatin1String("qdebug"))
        return QTestFunction::QDebug;
    else if (str == QLatin1String("qwarn"))
        return QTestFunction::QWarning;
    else if (str == QLatin1String("qfatal"))
        return QTestFunction::QFatal;
    else if (str == QLatin1String("skip"))
        return QTestFunction::Skip;
    else if (str == QLatin1String("info"))
        return QTestFunction::Info;
    return QTestFunction::QSystem; // ...
}

// -----------------------------------
QTestLibPlugin::QTestLibPlugin()
    : m_projectExplorer(0), m_outputPane(0)
{
}

QTestLibPlugin::~QTestLibPlugin()
{
    if (m_outputPane)
        ExtensionSystem::PluginManager::instance()->removeObject(m_outputPane);
}

bool QTestLibPlugin::init(const QStringList &arguments, QString *errorMessage)
{
    Q_UNUSED(arguments);
    Q_UNUSED(errorMessage);
    m_projectExplorer = ProjectExplorer::ProjectExplorerPlugin::instance();
    connect(m_projectExplorer, SIGNAL(aboutToExecuteProject(ProjectExplorer::Project *)),
            this, SLOT(projectRunHook(ProjectExplorer::Project *)));

    m_outputPane = new QTestOutputPane(this);
    ExtensionSystem::PluginManager::instance()->addObject(m_outputPane);

    return true;
}

void QTestLibPlugin::extensionsInitialized()
{
}

void QTestLibPlugin::projectRunHook(ProjectExplorer::Project *proj)
{
    return; //NBS TODO QTestlibplugin
    if (!proj)
        return;

    m_projectDirectory = QString();
    //NBS proj->setExtraApplicationRunArguments(QStringList());
    //NBS proj->setCustomApplicationOutputHandler(0);

    const QVariant config; //NBS  = proj->projectProperty(ProjectExplorer::Constants::P_CONFIGVAR);
    if (!config.toStringList().contains(QLatin1String("qtestlib")))
        return;

    {
        QTemporaryFile tempFile;
        tempFile.setAutoRemove(false);
        tempFile.open();
        m_outputFile = tempFile.fileName();
    }

    //NBS proj->setCustomApplicationOutputHandler(this);
    //NBS proj->setExtraApplicationRunArguments(QStringList() << QLatin1String("-xml") << QLatin1String("-o") << m_outputFile);
//    const QString proFile = proj->fileName();
//    const QFileInfo fi(proFile);
//    if (QFile::exists(fi.absolutePath()))
//        m_projectDirectory = fi.absolutePath();
}

void QTestLibPlugin::clear()
{
    m_projectExplorer->applicationOutputWindow()->clear();
}

void QTestLibPlugin::appendOutput(const QString &out)
{
    m_projectExplorer->applicationOutputWindow()->appendOutput(out);
}

void QTestLibPlugin::processExited(int exitCode)
{
    m_projectExplorer->applicationOutputWindow()->processExited(exitCode);

    QFile f(m_outputFile);
    if (!f.open(QIODevice::ReadOnly))
        return;

    QDomDocument doc;
    if (!doc.setContent(&f))
        return;

    f.close();
    f.remove();

    m_outputPane->clearContents();

    const QString testFunctionTag = QLatin1String("TestFunction");
    const QString nameAttr = QLatin1String("name");
    const QString typeAttr = QLatin1String("type");
    const QString incidentTag = QLatin1String("Incident");
    const QString fileAttr = QLatin1String("file");
    const QString lineAttr = QLatin1String("line");
    const QString messageTag = QLatin1String("Message");
    const QString descriptionItem = QLatin1String("Description");

    for (QDomElement testElement = doc.documentElement().firstChildElement();
         !testElement.isNull(); testElement = testElement.nextSiblingElement()) {

         if (testElement.tagName() != testFunctionTag)
             continue;

         QTestFunction *function = new QTestFunction(testElement.attribute(nameAttr));

         for (QDomElement e = testElement.firstChildElement();
              !e.isNull(); e = e.nextSiblingElement()) {

             const QString type = e.attribute(typeAttr);

              if (e.tagName() == incidentTag) {
                 QString file = e.attribute(fileAttr);

                 if (!file.isEmpty()
                     && QFileInfo(file).isRelative()
                     && !m_projectDirectory.isEmpty()) {

                     QFileInfo fi(m_projectDirectory, file);
                     if (fi.exists())
                         file = fi.absoluteFilePath();
                 }

                 const QString line = e.attribute(lineAttr);
                 const QString details = e.text();

                 QTestFunction::IncidentType itype = stringToIncident(type);
                 function->addIncident(itype, file, line, details);
             } else if (e.tagName() ==  messageTag ) {
                 QTestFunction::MessageType msgType = stringToMessageType(type);
                 function->addMessage(msgType, e.namedItem(descriptionItem).toElement().text());
             }
         }

         m_outputPane->addFunction(function);
     }

     m_outputPane->show();
}

// -------- QTestFunction
void QTestFunction::addIncident(IncidentType type,
                                const QString &file,
                                const QString &line,
                                const QString &details)
{
    QStandardItem *status = new QStandardItem(incidentString(type));
    status->setData(QVariant::fromValue(type));

    switch (type) {
        case QTestFunction::Pass: status->setForeground(Qt::green); break;
        case QTestFunction::Fail: status->setForeground(Qt::red); break;
        case QTestFunction::XFail: status->setForeground(Qt::darkMagenta); break;
        case QTestFunction::XPass: status->setForeground(Qt::darkGreen); break;
    }

    QStandardItem *location = new QStandardItem;
    if (!file.isEmpty()) {
        location->setText(file + QLatin1Char(':') + line);
        location->setForeground(Qt::red);

        QTestLocation loc;
        loc.file = file;
        loc.line = line;
        location->setData(QVariant::fromValue(loc));
    }

    appendRow(QList<QStandardItem *>() << status << location);

    if (!details.isEmpty()) {
        status->setColumnCount(2);
        status->appendRow(QList<QStandardItem *>() << new QStandardItem() << new QStandardItem(details));
    }
}

void QTestFunction::addMessage(MessageType type, const QString &text)
{
    QStandardItem *status = new QStandardItem(messageString(type));
    status->setData(QVariant::fromValue(type));
    QStandardItem *msg = new QStandardItem(text);
    appendRow(QList<QStandardItem *>() << status << msg);
}

bool QTestFunction::indexHasIncidents(const QModelIndex &function, IncidentType type)
{
    if (!function.isValid())
        return false;
    const QAbstractItemModel *m = function.model();
    if (!m->hasChildren(function))
        return false;

    const int rows = m->rowCount(function);
    for (int row = 0; row < rows; ++row) {
        const QModelIndex child = m->index(row, 0, function);

        QVariant tag = child.data(Qt::UserRole + 1);
        if (tag.type() != QVariant::UserType
            || tag.userType() != qMetaTypeId<QTestFunction::IncidentType>())
            continue;

        if (tag.value<QTestFunction::IncidentType>() == type)
            return true;
    }

    return false;
}

// -------------- QTestOutputPane

QTestOutputPane::QTestOutputPane(QTestLibPlugin *plugin)
  : QObject(plugin),
    m_plugin(plugin),
    m_widget(0),
    m_model(new QStandardItemModel(this))
{
    clearContents();
}

void QTestOutputPane::addFunction(QTestFunction *function)
{
    m_model->appendRow(function);
}

QWidget *QTestOutputPane::outputWidget(QWidget *parent)
{
    if (!m_widget)
        m_widget = new QTestOutputWidget(m_model, m_plugin->coreInterface(), parent);
    return m_widget;
}

QString QTestOutputPane::name() const
{
    return tr("Test Results");
}

void QTestOutputPane::clearContents()
{
    m_model->clear();
    m_model->setColumnCount(2);
    m_model->setHorizontalHeaderLabels(QStringList() << tr("Result") << tr("Message"));
}

void QTestOutputPane::visibilityChanged(bool visible)
{
    Q_UNUSED(visible)
}

void QTestOutputPane::show()
{
    if (m_widget)
        m_widget->expand();
    emit showPage();
}

// --------  QTestOutputFilter
bool QTestOutputFilter::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    if (sourceParent.isValid()) {
        return true;
    }

    QModelIndex idx = sourceModel()->index(sourceRow, 0);
    if (QTestFunction::indexHasIncidents(idx, m_filter))
        return true;
    else
        return false;
}

// ------- QTestOutputWidget


QTestOutputWidget::QTestOutputWidget(QStandardItemModel *model, QWidget *parent)
  : QWidget(parent),
    m_model(model),
    m_resultsView(new QTreeView(this)),
    m_filterCombo(new QComboBox(this)),
    m_filterModel(new QTestOutputFilter(this))
{
    m_resultsView->setModel(model);
    m_resultsView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_resultsView->header()->setStretchLastSection(true);
    connect(m_resultsView, SIGNAL(activated(const QModelIndex &)),
            this, SLOT(gotoLocation(QModelIndex)));

    m_filterCombo->addItem(tr("All Incidents"));
    m_filterCombo->addItem(incidentString(QTestFunction::Fail), QVariant::fromValue(QTestFunction::Fail));
    m_filterCombo->addItem(incidentString(QTestFunction::Pass), QVariant::fromValue(QTestFunction::Pass));
    m_filterCombo->addItem(incidentString(QTestFunction::XFail), QVariant::fromValue(QTestFunction::XFail));
    m_filterCombo->addItem(incidentString(QTestFunction::XPass), QVariant::fromValue(QTestFunction::XPass));
    connect(m_filterCombo, SIGNAL(activated(int)),
            this, SLOT(activateComboFilter(int)));

    QHBoxLayout *filterLayout = new QHBoxLayout;
    filterLayout->addWidget(new QLabel(tr("Show Only:"), this));
    filterLayout->addWidget(m_filterCombo);
    filterLayout->addStretch();

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->addLayout(filterLayout);
    layout->addWidget(m_resultsView);

    m_filterModel->setDynamicSortFilter(true);
    m_filterModel->setSourceModel(m_model);
}

void QTestOutputWidget::expand()
{
    /*
    const QAbstractItemModel *m = m_resultsView->model();
    for (int r = 0, count = m->rowCount(); r < count; ++r) {
        m_resultsView->expand(m->index(r, 0));
    }
    */
    m_resultsView->expandAll();
    m_resultsView->header()->resizeSections(QHeaderView::ResizeToContents);
}

void QTestOutputWidget::activateComboFilter(int index)
{
    QVariant tag = m_filterCombo->itemData(index);
    if (!tag.isValid()) {
        if (m_resultsView->model() != m_model)
            m_resultsView->setModel(m_model);
    } else {

        QTestFunction::IncidentType incident = tag.value<QTestFunction::IncidentType>();
        m_filterModel->setIncidentFilter(incident);

        if (m_resultsView->model() != m_filterModel)
            m_resultsView->setModel(m_filterModel);
    }
    expand();
}

void QTestOutputWidget::gotoLocation(QModelIndex index)
{
    if (!index.isValid())
        return;

    if (m_resultsView->model() == m_filterModel)
        index = m_filterModel->mapToSource(index);

    if (!index.isValid())
        return;

    const QAbstractItemModel *m = index.model();

    QModelIndex parent = index.parent();
    if (!parent.isValid())
        return;

    QModelIndex functionIndex = parent;
    QModelIndex failureIndex = index;

    QModelIndex grandParent = parent.parent();
    if (grandParent.isValid()) {
        functionIndex = grandParent;
        failureIndex = parent;
    }

    if (!functionIndex.isValid())
        return;

    QModelIndex locationIndex = m->index(failureIndex.row(), 1, functionIndex);
    if (!locationIndex.isValid())
        return;

    QVariant tag = locationIndex.data(Qt::UserRole + 1);
    if (tag.type() != QVariant::UserType
        || tag.userType() != qMetaTypeId<QTestLocation>())
        return;

    QTestLocation loc = tag.value<QTestLocation>();

    Core::ICore::instance()->editorManager()->openEditor(loc.file);
    Core::EditorInterface *edtIface = Core::ICore::instance()->editorManager()->currentEditor();
    if (!edtIface)
        return;
    TextEditor::ITextEditor *editor =
        qobject_cast<TextEditor::ITextEditor*>(edtIface->qObject());
    if (!editor)
        return;

    editor->gotoLine(loc.line.toInt());
}

Q_EXPORT_PLUGIN(QTestLibPlugin)
