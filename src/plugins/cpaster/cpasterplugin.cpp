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
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "cpasterplugin.h"

#include "ui_pasteselect.h"

#include "splitter.h"
#include "pasteview.h"
#include "codepasterprotocol.h"
#include "pastebindotcomprotocol.h"
#include "pastebindotcaprotocol.h"

#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/filemanager.h>
#include <coreplugin/icore.h>
#include <coreplugin/messagemanager.h>
#include <coreplugin/messageoutputwindow.h>
#include <coreplugin/uniqueidmanager.h>
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
    : m_settingsPage(0)
{
}

CodepasterPlugin::~CodepasterPlugin()
{
    qDeleteAll(m_protocols);
}

bool CodepasterPlugin::initialize(const QStringList &arguments, QString *error_message)
{
    Q_UNUSED(arguments)
    Q_UNUSED(error_message)

    // Create the globalcontext list to register actions accordingly
    QList<int> globalcontext;
    globalcontext << UniqueIDManager::instance()->uniqueIdentifier(Core::Constants::C_GLOBAL);

    // Create the settings Page
    m_settingsPage = new SettingsPage();
    addAutoReleasedObject(m_settingsPage);

    // Create the protocols and append them to the Settings
    Protocol *protos[] =  { new CodePasterProtocol(),
                            new PasteBinDotComProtocol(),
                            new PasteBinDotCaProtocol(),
                            0};
    for(int i=0; protos[i] != 0; ++i) {
        connect(protos[i], SIGNAL(pasteDone(QString)), this, SLOT(finishPost(QString)));
        connect(protos[i], SIGNAL(fetchDone(QString,QString,bool)),
                this, SLOT(finishFetch(QString,QString,bool)));
        m_settingsPage->addProtocol(protos[i]->name());
        if (protos[i]->hasSettings())
            addAutoReleasedObject(protos[i]->settingsPage());
        m_protocols.append(protos[i]);
    }

    //register actions
    Core::ActionManager *actionManager = ICore::instance()->actionManager();

    Core::ActionContainer *toolsContainer =
        actionManager->actionContainer(Core::Constants::M_TOOLS);

    Core::ActionContainer *cpContainer =
        actionManager->createMenu(QLatin1String("CodePaster"));
    cpContainer->menu()->setTitle(tr("&Code Pasting"));
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
}

void CodepasterPlugin::post()
{
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
    QString protocolName;

    PasteView view(0);
    foreach (Protocol *p, m_protocols) {
        view.addProtocol(p->name(), p->name() == m_settingsPage->defaultProtocol());
    }

    if (!view.show(username, description, comment, lst))
        return; // User canceled post

    username = view.user();
    description = view.description();
    comment = view.comment();
    data = view.content();
    protocolName = view.protocol();

    // Copied from cpaster. Otherwise lineendings will screw up
    if (!data.contains("\r\n")) {
        if (data.contains('\n'))
            data.replace('\n', "\r\n");
        else if (data.contains('\r'))
            data.replace('\r', "\r\n");
    }

    foreach(Protocol *protocol, m_protocols) {
        if (protocol->name() == protocolName) {
            protocol->paste(data, username, comment, description);
            break;
        }
    }

}

void CodepasterPlugin::fetch()
{
    QDialog dialog(ICore::instance()->mainWindow());
    Ui_PasteSelectDialog ui;
    ui.setupUi(&dialog);
    foreach(const Protocol *protocol, m_protocols)
        ui.protocolBox->addItem(protocol->name());
    ui.protocolBox->setCurrentIndex(ui.protocolBox->findText(m_settingsPage->defaultProtocol()));

    ui.listWidget->addItems(QStringList() << tr("This protocol supports no listing"));
    ui.listWidget->setSelectionMode(QAbstractItemView::ExtendedSelection);
#ifndef Q_WS_MACX
    ui.listWidget->setFrameStyle(QFrame::NoFrame);
#endif // Q_WS_MACX
    QFont listFont = ui.listWidget->font();
    listFont.setFamily("Courier");
    listFont.setStyleHint(QFont::TypeWriter);
    ui.listWidget->setFont(listFont);
    // ### TODO2: when we change the protocol, we need to relist
    foreach(Protocol *protocol, m_protocols) {
        if (protocol->name() == ui.protocolBox->currentText() && protocol->canList()) {
            ui.listWidget->clear();
            ui.listWidget->addItems(QStringList() << tr("Waiting for items"));
            protocol->list(ui.listWidget);
            break;
        }
    }

    int result = dialog.exec();
    if (!result)
        return;
    QStringList list = ui.pasteEdit->text().split(QLatin1Char(' '));
    if (list.isEmpty())
        return;
    QString pasteID = list.first();

    // Get Protocol
    foreach(Protocol *protocol, m_protocols) {
        if (protocol->name() == ui.protocolBox->currentText()) {
            protocol->fetch(pasteID);
            break;
        }
    }
}

void CodepasterPlugin::finishPost(const QString &link)
{
    if (m_settingsPage->copyToClipBoard())
        QApplication::clipboard()->setText(link);
    ICore::instance()->messageManager()->printToOutputPane(link,
                                                           m_settingsPage->displayOutput());
}

void CodepasterPlugin::finishFetch(const QString &titleDescription,
                                   const QString &content,
                                   bool error)
{
    QString title = titleDescription;
    if (error) {
        ICore::instance()->messageManager()->printToOutputPane(content, true);
    } else {
        EditorManager* manager = EditorManager::instance();
        IEditor* editor = manager->openEditorWithContents(Core::Constants::K_DEFAULT_TEXT_EDITOR_ID, &title, content);
        manager->activateEditor(editor);
    }
}

Q_EXPORT_PLUGIN(CodepasterPlugin)
