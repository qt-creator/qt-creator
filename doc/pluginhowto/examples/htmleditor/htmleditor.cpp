#include "htmlfile.h"
#include "htmleditor.h"
#include "htmleditorwidget.h"

#include "coreplugin/uniqueidmanager.h"

struct HTMLEditorData
{
    HTMLEditorData() : editorWidget(0), file(0) { }

    HTMLEditorWidget* editorWidget;
    QString displayName;
    HTMLFile* file;
    QList<int> context;
};

namespace HTMLEditorConstants
{
    const char* const C_HTMLEDITOR_MIMETYPE = "text/html";
    const char* const C_HTMLEDITOR = "HTML Editor";
}

HTMLEditor::HTMLEditor(HTMLEditorWidget* editorWidget)
    :Core::IEditor(editorWidget)
{
    d = new HTMLEditorData;
    d->editorWidget = editorWidget;
    d->file = new HTMLFile(this, editorWidget);

    Core::UniqueIDManager* uidm = Core::UniqueIDManager::instance();
    d->context << uidm->uniqueIdentifier(HTMLEditorConstants::C_HTMLEDITOR);

    connect(d->editorWidget, SIGNAL(contentModified()),
            d->file, SLOT(modified()));
    connect(d->editorWidget, SIGNAL(titleChanged(QString)),
            this, SLOT(slotTitleChanged(QString)));
    connect(d->editorWidget, SIGNAL(contentModified()),
            this, SIGNAL(changed()));
}

HTMLEditor::~HTMLEditor()
{
    delete d;
}

QWidget* HTMLEditor::widget()
{
    return d->editorWidget;
}

QList<int> HTMLEditor::context() const
{
    return d->context;
}

Core::IFile* HTMLEditor::file()
{
    return d->file;
}

bool HTMLEditor::createNew(const QString& contents)
{
    Q_UNUSED(contents);

    d->editorWidget->setContent(QByteArray());
    d->file->setFilename(QString());

    return true;
}

bool HTMLEditor::open(const QString &fileName)
{
    return d->file->open(fileName);
}

const char* HTMLEditor::kind() const
{
    return HTMLEditorConstants::C_HTMLEDITOR;
}

QString HTMLEditor::displayName() const
{
    return d->displayName;
}

void HTMLEditor::setDisplayName(const QString& title)
{
    if(d->displayName == title)
        return;

    d->displayName = title;
    emit changed();
}

bool HTMLEditor::duplicateSupported() const
{
    return false;
}
Core::IEditor* HTMLEditor::duplicate(QWidget* parent)
{
    Q_UNUSED(parent);

    return 0;
}

QByteArray HTMLEditor::saveState() const
{
    return QByteArray();
}

bool HTMLEditor::restoreState(const QByteArray& state)
{
    Q_UNUSED(state);

    return false;
}

QToolBar* HTMLEditor::toolBar()
{
    return 0;
}

bool HTMLEditor::isTemporary() const
{
    return false;
}



