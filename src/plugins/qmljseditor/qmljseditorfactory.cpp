/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
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

#include "qmljseditorfactory.h"
#include "qmljseditor.h"
#include "qmljseditoractionhandler.h"
#include "qmljseditorconstants.h"
#include "qmljseditorplugin.h"

#include <extensionsystem/pluginmanager.h>
#include <extensionsystem/pluginspec.h>

#include <coreplugin/icore.h>
#include <coreplugin/editormanager/editormanager.h>

#include <QtCore/QFileInfo>
#include <QtCore/QDebug>
#include <QtCore/QSettings>
#include <QtGui/QMessageBox>
#include <QtGui/QPushButton>
#include <QtGui/QMainWindow>

namespace {
    const char * const QMLDESIGNER_INFO_BAR = "QmlJSEditor.QmlDesignerInfoBar";
    const char * const KEY_QMLGROUP = "QML";
    const char * const KEY_NAGABOUTDESIGNER = "AskAboutVisualDesigner";

    bool isQmlDesignerExperimentallyDisabled()
    {
        ExtensionSystem::PluginManager *pm = ExtensionSystem::PluginManager::instance();
        foreach (const ExtensionSystem::PluginSpec *spec, pm->plugins()) {
            if (spec->name() == QLatin1String("QmlDesigner")) {
                if (spec->isExperimental() && !spec->isEnabled())
                    return true;
                return false;
            }
        }
        return false;
    }

    bool isNaggingAboutExperimentalDesignerEnabled()
    {
        if (!isQmlDesignerExperimentallyDisabled()) {
            return false;
        }
        QSettings *settings = Core::ICore::instance()->settings();
        settings->beginGroup(QLatin1String(KEY_QMLGROUP));
        bool nag = settings->value(QLatin1String(KEY_NAGABOUTDESIGNER), true).toBool();
        settings->endGroup();
        return nag;
    }
}

using namespace QmlJSEditor::Internal;
using namespace QmlJSEditor::Constants;

QmlJSEditorFactory::QmlJSEditorFactory(QObject *parent)
  : Core::IEditorFactory(parent)
{
    m_mimeTypes
            << QLatin1String(QmlJSEditor::Constants::QML_MIMETYPE)
            << QLatin1String(QmlJSEditor::Constants::JS_MIMETYPE)
            ;
}

QmlJSEditorFactory::~QmlJSEditorFactory()
{
}

QString QmlJSEditorFactory::id() const
{
    return QLatin1String(C_QMLJSEDITOR_ID);
}

QString QmlJSEditorFactory::displayName() const
{
    return tr(C_QMLJSEDITOR_DISPLAY_NAME);
}


Core::IFile *QmlJSEditorFactory::open(const QString &fileName)
{
    Core::IEditor *iface = Core::EditorManager::instance()->openEditor(fileName, id());
    if (!iface) {
        qWarning() << "QmlEditorFactory::open: openEditor failed for " << fileName;
        return 0;
    }
    return iface->file();
}

Core::IEditor *QmlJSEditorFactory::createEditor(QWidget *parent)
{
    static bool listenerInitialized = false;
    if (!listenerInitialized) {
        listenerInitialized = true;
        if (isNaggingAboutExperimentalDesignerEnabled()) {
            connect(Core::EditorManager::instance(), SIGNAL(currentEditorChanged(Core::IEditor*)),
                     SLOT(updateEditorInfoBar(Core::IEditor*)));
        }
    }
    QmlJSTextEditor *rc = new QmlJSTextEditor(parent);
    QmlJSEditorPlugin::instance()->initializeEditor(rc);
    return rc->editableInterface();
}

QStringList QmlJSEditorFactory::mimeTypes() const
{
    return m_mimeTypes;
}

void QmlJSEditorFactory::updateEditorInfoBar(Core::IEditor *editor)
{
    if (qobject_cast<QmlJSEditorEditable *>(editor)) {
        Core::EditorManager::instance()->showEditorInfoBar(QMLDESIGNER_INFO_BAR,
            tr("Do you want to enable the experimental Qt Quick Designer?"),
            tr("Enable Qt Quick Designer"), this,
            SLOT(activateQmlDesigner()),
            SLOT(neverAskAgainAboutQmlDesigner()));
    } else {
        Core::EditorManager::instance()->hideEditorInfoBar(QMLDESIGNER_INFO_BAR);
    }
}

void QmlJSEditorFactory::activateQmlDesigner()
{
    QString menu;
#ifdef Q_WS_MAC
    menu = tr("Qt Creator -> About Plugins...");
#else
    menu = tr("Help -> About Plugins...");
#endif
    QMessageBox message(Core::ICore::instance()->mainWindow());
    message.setWindowTitle(tr("Enable experimental Qt Quick Designer?"));
    message.setText(tr("Do you want to enable the experimental Qt Quick Designer? "
                       "After enabling it, you can access the visual design capabilities by switching to Design Mode. "
                       "This can affect the overall stability of Qt Creator. "
                       "To disable Qt Quick Designer again, visit the menu '%1' and disable 'QmlDesigner'.").arg(menu));
    message.setIcon(QMessageBox::Question);
    QPushButton *enable = message.addButton(tr("Enable Qt Quick Designer"), QMessageBox::AcceptRole);
    message.addButton(tr("Cancel"), QMessageBox::RejectRole);
    message.exec();
    if (message.clickedButton() == enable) {
        ExtensionSystem::PluginManager *pm = ExtensionSystem::PluginManager::instance();
        foreach (ExtensionSystem::PluginSpec *spec, pm->plugins()) {
            if (spec->name() == QLatin1String("QmlDesigner")) {
                spec->setEnabled(true);
                pm->writeSettings();
                QMessageBox::information(Core::ICore::instance()->mainWindow(), tr("Please restart Qt Creator"),
                                         tr("Please restart Qt Creator to make the change effective."));
                disconnect(Core::EditorManager::instance(), SIGNAL(currentEditorChanged(Core::IEditor*)),
                         this, SLOT(updateEditorInfoBar(Core::IEditor*)));
                Core::EditorManager::instance()->hideEditorInfoBar(QMLDESIGNER_INFO_BAR);
                return;
            }
        }
    }
}

void QmlJSEditorFactory::neverAskAgainAboutQmlDesigner()
{
    QSettings *settings = Core::ICore::instance()->settings();
    settings->beginGroup(QLatin1String(KEY_QMLGROUP));
    settings->setValue(QLatin1String(KEY_NAGABOUTDESIGNER), false);
    settings->endGroup();
    disconnect(Core::EditorManager::instance(), SIGNAL(currentEditorChanged(Core::IEditor*)),
             this, SLOT(updateEditorInfoBar(Core::IEditor*)));
}
