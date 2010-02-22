#include <QtCore/QCoreApplication>
#include <QtCore/QDebug>
#include <QtCore/QMetaObject>
#include <QtCore/QMetaProperty>
#include <QtDeclarative/QmlMetaType>

 void messageOutput(QtMsgType type, const char *msg)
 {
     switch (type) {
     case QtDebugMsg:
         fprintf(stdout, "%s\n", msg);
         break;
     case QtWarningMsg:
         fprintf(stderr, "Warning: %s\n", msg);
         break;
     case QtCriticalMsg:
         fprintf(stderr, "Critical: %s\n", msg);
         break;
     case QtFatalMsg:
         fprintf(stderr, "Fatal: %s\n", msg);
         abort();
     }
 }
void dumpProperty(QMetaProperty qProperty)
{
    qDebug() << "\t\t" << qProperty.name()  << "\t\t"
                       << qProperty.typeName()
                       << "readable=" << qProperty.isReadable()
                       << "writable=" << qProperty.isWritable()
                       << "resettable=" << qProperty.isResettable();
}

void dumpType(QmlType *type)
{
    qDebug() << "Type:" << type->qmlTypeName() << type->majorVersion() << type->minorVersion() << "(C++: " << type->typeName() << ")";

    const QMetaObject *qMetaObject = type->metaObject();
    if (!qMetaObject) {
        qDebug() << "\tNo metaObject! Skipping ...";
        return;
    }

    qDebug() << "\tProperties (default property :" << QmlMetaType::defaultProperty(qMetaObject).typeName() << ")";
    for (int i = 0; i < qMetaObject->propertyCount(); ++i) {
        dumpProperty(qMetaObject->property(i));
    }

    qDebug() << "\tSuper Classes:";
    if (qMetaObject->superClass()) {
        if (QmlType *qmlParentType = QmlMetaType::qmlType(qMetaObject->superClass())) {
            if (!qmlParentType->qmlTypeName().isEmpty()) {
                qDebug() << "\t\t" << qmlParentType->qmlTypeName();
            } else {
                qDebug() << "\t\t" << "(no Qml Type name)" << qmlParentType->typeName();
            }
        } else {
            const QString parentClass = qMetaObject->superClass()->className();
            qDebug() << "\t\t" << "(no Qml Type)" << parentClass;
        }
    } else {
        qDebug() << "\t\t" << "no superclass";
    }
}

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);

    qInstallMsgHandler(messageOutput);

    qDebug() << "Registered qml meta types:\n\n";
    foreach (const QByteArray &typeName, QmlMetaType::qmlTypeNames()) {
        if (typeName.isEmpty()) {
            qDebug() << "Empty type name!!! Skipping ...";
            continue;
        }
        QmlType *qmlType = QmlMetaType::qmlType(typeName, 4, 6);
        Q_ASSERT(qmlType);
        dumpType(qmlType);
    }
    return 0;
}
