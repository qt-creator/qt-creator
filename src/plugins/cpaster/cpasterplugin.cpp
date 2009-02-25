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

#include "cpasterplugin.h"

#include "ui_pasteselect.h"

#include "splitter.h"
#include "view.h"

#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/filemanager.h>
#include <coreplugin/icore.h>
#include <coreplugin/messagemanager.h>
#include <coreplugin/messageoutputwindow.h>
#include <coreplugin/uniqueidmanager.h>
#include <extensionsystem/pluginmanager.h>
#include <texteditor/itexteditor.h>

#include <QtCore/QtPlugin>
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

    // Create the globalcontext list to register actions accordingly
    QList<int> globalcontext;
    globalcontext << UniqueIDManager::instance()->uniqueIdentifier(Core::Constants::C_GLOBAL);

    // Create the settings Page
    m_settingsPage = new SettingsPage();
    addObject(m_settingsPage);

    //register actions
    Core::ActionManager *actionManager = ICore::instance()->actionManager();

    Core::ActionContainer *toolsContainer =
        actionManager->actionContainer(Core::Constants::M_TOOLS);

    Core::ActionContainer *cpContainer =
        actionManager->createMenu(QLatin1String("CodePaster"));
    cpContainer->menu()->setTitle(tr("&CodePaster"));
    toolsContainer->addMenu(cpContainer);

    Core::Command *command;

    m_postAction = new QAction(tr("Paste Snippet..."), this);
    command = actionManager->registerAction(m_postAction, "CodePaster.Post", globalcontext);
    command->setDefaultKeySequence(QKeySequence(tr("Alt+C,Alt+P")));
    connect(m_postAction, SIGNAL(triggered()), this, SLOT(post()));
    cpContainer->addAction(command);

    m_fetchAction = new QAction(tr("Fetch Snippet..."), this);
    command = actionManager->registerAction(m_fetchAction, "CodePaster.Fetch", globalcontext);
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

QString CodepasterPlugin::serverUrl() const
{
    QString url = m_settingsPage->serverUrl().toString();
    if (url.startsWith("http://"))
        url = url.mid(7);
    if (url.endsWith('/'))
        url.chop(1);
    return url;
}

void CodepasterPlugin::post()
{
    // FIXME: The whole m_poster thing is de facto a simple function call.
    if (m_poster) {
        delete m_poster;
        m_poster = 0; 
    }
    IEditor* editor = EditorManager::instance()->currentEditor();
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

    m_poster = new CustomPoster(serverUrl(),
                                m_settingsPage->copyToClipBoard(),
                                m_settingsPage->displayOutput());

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
    if (m_fetcher) {
        delete m_fetcher;
        m_fetcher = 0;
    }
    m_fetcher = new CustomFetcher(serverUrl());

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
    m_fetcher = new CustomFetcher(serverUrl());
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
        EditorManager::instance()->newFile(Core::Constants::K_DEFAULT_TEXT_EDITOR, &title, data);
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
        ICore::instance()->messageManager()->printToOutputPane(pastedUrl(), m_output);
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
