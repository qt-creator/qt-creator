#include "qmljslink.h"

#include "parser/qmljsast_p.h"
#include "qmljsdocument.h"
#include "qmljsbind.h"

#include <QtCore/QFileInfo>
#include <QtCore/QDir>
#include <QtCore/QDebug>

using namespace QmlJS;
using namespace QmlJS::Interpreter;
using namespace QmlJS::AST;

static QString componentName(const QString &fileName)
{
    QString componentName = fileName;
    int dotIndex = componentName.indexOf(QLatin1Char('.'));
    if (dotIndex != -1)
        componentName.truncate(dotIndex);
    componentName[0] = componentName[0].toUpper();
    return componentName;
}

void LinkImports::linkImports(Bind *bind, const QList<Bind *> &binds)
{
    // ### TODO: remove all properties from _typeEnv

    Document::Ptr doc = bind->_doc;
    if (! (doc->qmlProgram() && doc->qmlProgram()->imports))
        return;

    QFileInfo fileInfo(doc->fileName());
    const QString absolutePath = fileInfo.absolutePath();

    // implicit imports
    foreach (Bind *otherBind, binds) {
        if (otherBind == bind)
            continue;

        Document::Ptr otherDoc = otherBind->_doc;
        QFileInfo otherFileInfo(otherDoc->fileName());
        const QString otherAbsolutePath = otherFileInfo.absolutePath();

        if (otherAbsolutePath.size() < absolutePath.size()
            || otherAbsolutePath.left(absolutePath.size()) != absolutePath)
            continue;

        // ### TODO: implicit directory access not implemented
        if (otherAbsolutePath != absolutePath)
            continue;

        bind->_typeEnvironment->setProperty(componentName(otherFileInfo.fileName()), otherBind->_rootObjectValue);
    }

    // explicit imports, whether directories or files
    for (UiImportList *it = doc->qmlProgram()->imports; it; it = it->next) {
        if (! (it->import && it->import->fileName))
            continue;

        QString path = absolutePath;
        path += QLatin1Char('/');
        path += it->import->fileName->asString();
        path = QDir::cleanPath(path);

        foreach (Bind *otherBind, binds) {
            Document::Ptr otherDoc = otherBind->_doc;
            QFileInfo otherFileInfo(otherDoc->fileName());
            const QString otherAbsolutePath = otherFileInfo.absolutePath();

            if (path != otherDoc->fileName() && path != otherAbsolutePath)
                continue;

            bool directoryImport = (path == otherAbsolutePath);
            bool fileImport = (path == otherDoc->fileName());
            if (!directoryImport && !fileImport)
                continue;

            ObjectValue *importInto = bind->_typeEnvironment;
            if (directoryImport && it->import->importId) {
                // ### TODO: set importInto to a namespace object value
            }

            QString targetName;
            if (fileImport && it->import->importId) {
                targetName = it->import->importId->asString();
            } else {
                targetName = componentName(otherFileInfo.fileName());
            }

            importInto->setProperty(targetName, otherBind->_rootObjectValue);
        }
    }
}

static const ObjectValue *lookupType(ObjectValue *env, UiQualifiedId *id)
{
    const ObjectValue *objectValue = env;

    for (UiQualifiedId *iter = id; objectValue && iter; iter = iter->next) {
        if (! iter->name)
            return 0;

        const Value *value = objectValue->property(iter->name->asString());
        if (!value)
            return 0;

        objectValue = value->asObjectValue();
    }

    return objectValue;
}

void LinkImports::operator()(const QList<Bind *> &binds)
{
    foreach (Bind *bind, binds) {
        // Populate the _typeEnvironment with imports.
        linkImports(bind, binds);

        // Set the prototypes.
        {
            QHash<UiObjectDefinition *, ObjectValue *>::iterator it = bind->_qmlObjectDefinitions.begin();
            QHash<UiObjectDefinition *, ObjectValue *>::iterator end = bind->_qmlObjectDefinitions.end();
            for (; it != end; ++it) {
                UiObjectDefinition *key = it.key();
                ObjectValue *value = it.value();
                if (!key->qualifiedTypeNameId)
                    continue;

                value->setPrototype(lookupType(bind->_typeEnvironment, key->qualifiedTypeNameId));
            }
        }
        {
            QHash<UiObjectBinding *, ObjectValue *>::iterator it = bind->_qmlObjectBindings.begin();
            QHash<UiObjectBinding *, ObjectValue *>::iterator end = bind->_qmlObjectBindings.end();
            for (; it != end; ++it) {
                UiObjectBinding *key = it.key();
                ObjectValue *value = it.value();
                if (!key->qualifiedTypeNameId)
                    continue;

                value->setPrototype(lookupType(bind->_typeEnvironment, key->qualifiedTypeNameId));
            }
        }
    }
}

ObjectValue *Link::operator()(const QList<Bind *> &binds, Bind *currentBind, UiObjectMember *currentObject)
{
    ObjectValue *scopeObject;
    if (UiObjectDefinition *definition = cast<UiObjectDefinition *>(currentObject))
        scopeObject = currentBind->_qmlObjectDefinitions.value(definition);
    else if (UiObjectBinding *binding = cast<UiObjectBinding *>(currentObject))
        scopeObject = currentBind->_qmlObjectBindings.value(binding);
    else
        return 0;

    if (!scopeObject)
        return 0;

    // Build the scope chain.
    currentBind->_typeEnvironment->setScope(currentBind->_idEnvironment);
    currentBind->_idEnvironment->setScope(currentBind->_functionEnvironment);
    currentBind->_functionEnvironment->setScope(scopeObject);
    if (scopeObject != currentBind->_rootObjectValue) {
        scopeObject->setScope(currentBind->_rootObjectValue);
        currentBind->_rootObjectValue->setScope(currentBind->_interp->globalObject());
    } else {
        scopeObject->setScope(currentBind->_interp->globalObject());
    }
    // May want to link to instantiating components from here.

    return currentBind->_typeEnvironment;
}
