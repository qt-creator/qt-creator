/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
**
** Non-Open Source Usage
**
** Licensees may use this file in accordance with the Qt Beta Version
** License Agreement, Agreement version 2.2 provided with the Software or,
** alternatively, in accordance with the terms contained in a written
** agreement between you and Nokia.
**
** GNU General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU General
** Public License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the packaging
** of this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
**
** http://www.fsf.org/licensing/licenses/info/GPLv2.html and
** http://www.gnu.org/copyleft/gpl.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt GPL Exception
** version 1.3, included in the file GPL_EXCEPTION.txt in this package.
**
***************************************************************************/

#include "cpasterplugin.h"

#include "ui_pasteselect.h"

#include "splitter.h"
#include "view.h"

#include <coreplugin/icore.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/filemanager.h>
#include <coreplugin/messagemanager.h>
#include <coreplugin/uniqueidmanager.h>
#include <coreplugin/actionmanager/actionmanagerinterface.h>
#include <coreplugin/editormanager/editormanager.h>
#include <texteditor/itexteditor.h>
#include <coreplugin/messageoutputwindow.h>

#include <QtCore/qplugin.h>
#include <QtCore/QDebug>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QClipboard>
#include <QtGui/QMenu>
#include <QtGui/QMessageBox>
#include <QtGui/QMainWindow>
#include <QtGui/QListWidget>

using namespace CodePaster;
using namespace Core;
using namespace TextEditor;

Core::ICore *gCoreInstance = NULL;

CodepasterPlugin::CodepasterPlugin()
    : m_settingsPage(0), m_fetcher(0), m_poster(0)
{
}

CodepasterPlugin::~CodepasterPlugin()
{
    if (m_settingsPage) {
        removeObject(m_settingsPage);
        delete m_settingsPage;
        m_settingsPage = 0;
    }
}

bool CodepasterPlugin::initialize(const QStringList &arguments, QString *error_message)
{
    Q_UNUSED(arguments);
    Q_UNUSED(error_message);

    gCoreInstance = ExtensionSystem::PluginManager::instance()->getObject<Core::ICore>();

    // Create the globalcontext list to register actions accordingly
    QList<int> globalcontext;
    globalcontext << gCoreInstance->uniqueIDManager()->
        uniqueIdentifier(Core::Constants::C_GLOBAL);

    // Create the settings Page
    m_settingsPage = new SettingsPage();
    addObject(m_settingsPage);

    //register actions
    Core::ActionManagerInterface *actionManager = gCoreInstance->actionManager();

    Core::IActionContainer *toolsContainer =
        actionManager->actionContainer(Core::Constants::M_TOOLS);

    Core::IActionContainer *cpContainer =
        actionManager->createMenu(QLatin1String("CodePaster"));
    cpContainer->menu()->setTitle(tr("&CodePaster"));
    toolsContainer->addMenu(cpContainer);

    Core::ICommand *command;

    m_postAction = new QAction(tr("Paste snippet..."), this);
    command = actionManager->registerAction(m_postAction, "CodePaster.post", globalcontext);
    command->setDefaultKeySequence(QKeySequence(tr("Alt+C,Alt+P")));
    connect(m_postAction, SIGNAL(triggered()), this, SLOT(post()));
    cpContainer->addAction(command);

    m_fetchAction = new QAction(tr("Fetch snippet..."), this);
    command = actionManager->registerAction(m_fetchAction, "CodePaster.fetch", globalcontext);
    command->setDefaultKeySequence(QKeySequence(tr("Alt+C,Alt+F")));
    connect(m_fetchAction, SIGNAL(triggered()), this, SLOT(fetch()));
    cpContainer->addAction(command);

    return true;
}

void CodepasterPlugin::extensionsInitialized()
{
    m_projectExplorer = ExtensionSystem::PluginManager::instance()
        ->getObject<ProjectExplorer::ProjectExplorerPlugin>();
}

void CodepasterPlugin::post()
{
    if (m_poster)
        delete m_poster;
    IEditor* editor = gCoreInstance->editorManager()->currentEditor();
    ITextEditor* textEditor = qobject_cast<ITextEditor*>(editor);
    if (!textEditor)
        return;

    QString data = textEditor->selectedText();
    if (!data.isEmpty()) {
        QChar *uc = data.data();
        QChar *e = uc + data.size();

        for (; uc != e; ++uc) {
            switch (uc->unicode()) {
            case 0xfdd0: // QTextBeginningOfFrame
            case 0xfdd1: // QTextEndOfFrame
            case QChar::ParagraphSeparator:
            case QChar::LineSeparator:
                *uc = QLatin1Char('\n');
                break;
            case QChar::Nbsp:
                *uc = QLatin1Char(' ');
                break;
            default:
                ;
            }
        }
    } else
        data = textEditor->contents();

    FileDataList lst = splitDiffToFiles(data.toLatin1());
    QString username = m_settingsPage->username();
    QString description;
    QString comment;

    View view(0);
    if (!view.show(username, description, comment, lst))
        return; // User canceled post

    username = view.getUser();
    description = view.getDescription();
    comment = view.getComment();
    data = view.getContent();

    // Submit to codepaster
    m_poster = new CustomPoster(m_settingsPage->serverUrl().toString());

    // Copied from cpaster. Otherwise lineendings will screw up
    if (!data.contains("\r\n")) {
        if (data.contains('\n'))
            data.replace('\n', "\r\n");
        else if (data.contains('\r'))
            data.replace('\r', "\r\n");
    }
    m_poster->post(description, comment, data, username);
}

void CodepasterPlugin::fetch()
{
    if (m_fetcher)
        delete m_fetcher;
    m_fetcher = new CustomFetcher(m_settingsPage->serverUrl().toString());

    QDialog dialog;
    Ui_PasteSelectDialog ui;
    ui.setupUi(&dialog);

    ui.listWidget->addItems(QStringList() << "Waiting for items");
    ui.listWidget->setSelectionMode(QAbstractItemView::ExtendedSelection);
    ui.listWidget->setFrameStyle(QFrame::NoFrame);
    m_fetcher->list(ui.listWidget);

    int result = dialog.exec();
    if (!result)
        return;
    bool ok;
    QStringList list = ui.pasteEdit->text().split(QLatin1Char(' '));
    int pasteID = !list.isEmpty() ? list.first().toInt(&ok) : -1;
    if (!ok || pasteID <= 0)
        return;

    delete m_fetcher;
    m_fetcher = new CustomFetcher(m_settingsPage->serverUrl().toString());
    m_fetcher->fetch(pasteID);
}

CustomFetcher::CustomFetcher(const QString &host)
        : Fetcher(host)
        , m_host(host)
        , m_listWidget(0)
        , m_id(-1)
        , m_customError(false)
{
    // cpaster calls QCoreApplication::exit which we want to avoid here
    disconnect(this, SIGNAL(requestFinished(int,bool))
              ,this, SLOT(gotRequestFinished(int,bool)));
    connect(this, SIGNAL(requestFinished(int,bool))
                    , SLOT(customRequestFinished(int,bool)));
}

void CustomFetcher::customRequestFinished(int, bool error)
{
    m_customError = error;
    if (m_customError || hadError()) {
        QMessageBox::warning(0, QLatin1String("CodePaster Error")
                             , QLatin1String("Could not fetch code")
                             , QMessageBox::Ok);
        return;
    }

    QByteArray data = body();
    if (!m_listWidget) {
        QString title = QString::fromLatin1("Code Paster: %1").arg(m_id);
        gCoreInstance->editorManager()->newFile(Core::Constants::K_DEFAULT_TEXT_EDITOR
                                                , &title, data);
    } else {
        m_listWidget->clear();
        QStringList lines = QString(data).split(QLatin1Char('\n'));
        m_listWidget->addItems(lines);
        m_listWidget = 0;
    }
}

int CustomFetcher::fetch(int pasteID)
{
    m_id = pasteID;
    return Fetcher::fetch(pasteID);
}

void CustomFetcher::list(QListWidget* list)
{
    m_listWidget = list;
    QString url = QLatin1String("http://");
    url += m_host;
    url += QLatin1String("/?command=browse&format=raw");
    Fetcher::fetch(url);
}

CustomPoster::CustomPoster(const QString &host, bool copyToClipboard, bool displayOutput)
    : Poster(host), m_copy(copyToClipboard), m_output(displayOutput)
{
    // cpaster calls QCoreApplication::exit which we want to avoid here
    disconnect(this, SIGNAL(requestFinished(int,bool)),
              this, SLOT(gotRequestFinished(int,bool)));
    connect(this, SIGNAL(requestFinished(int,bool)),
                  SLOT(customRequestFinished(int,bool)));
}

void CustomPoster::customRequestFinished(int, bool error)
{
    if (!error) {
        if (m_copy)
            QApplication::clipboard()->setText(pastedUrl());
        gCoreInstance->messageManager()->printToOutputPane(pastedUrl(), m_output);
    } else
        QMessageBox::warning(0, "Code Paster Error", "Some error occured while posting", QMessageBox::Ok);
#if 0 // Figure out how to access
    Core::Internal::MessageOutputWindow* messageWindow =
            ExtensionSystem::PluginManager::instance()->getObject<Core::Internal::MessageOutputWindow>();
    if (!messageWindow)
        qDebug() << "Pasted at:" << pastedUrl();

    messageWindow->append(pastedUrl());
    messageWindow->setFocus();
#endif
}

Q_EXPORT_PLUGIN(CodepasterPlugin)
