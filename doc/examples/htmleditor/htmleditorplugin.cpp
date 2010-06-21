#include "htmleditorplugin.h"
#include "htmleditorfactory.h"
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/mimedatabase.h>
#include <coreplugin/icore.h>
#include <QtPlugin>
#include <QString>
#include <QMessageBox>
#include <QFontDialog>

HTMLEditorPlugin::HTMLEditorPlugin()
{
    // Do nothing
}

HTMLEditorPlugin::~HTMLEditorPlugin()
{
    // Do notning
}

void HTMLEditorPlugin::extensionsInitialized()
{
    // Do nothing
}

bool HTMLEditorPlugin::initialize(const QStringList& args, QString *errMsg)
{
    Q_UNUSED(args);

    Core::ActionManager* am = Core::ICore::instance()->actionManager();
    Core::ActionContainer* ac = am->actionContainer(Core::Constants::M_EDIT);
    QAction* Font = ac->menu()->addAction("Font");

    // Create a command for "Font".
    Core::Command* cmd = am->registerAction(new QAction(this),"HTMLEditorPlugin.Font",QList<int>() << 0);
    cmd->action()->setText("Font");

    Core::ICore* core = Core::ICore::instance();
    Core::MimeDatabase* mdb = core->mimeDatabase();

    if(!mdb->addMimeTypes("text-html-mimetype.xml", errMsg))
        return false;

    QMessageBox::information(0, "Msg", *errMsg);

    addAutoReleasedObject(new HTMLEditorFactory(this));
    return true;
}

void HTMLEditorPlugin::shutdown()
{
    // Do nothing
}

Q_EXPORT_PLUGIN(HTMLEditorPlugin)
