#include "qmlprojectitem.h"
#include "filefilteritems.h"
#include <qdebug.h>

namespace QmlProjectManager {

class QmlProjectItemPrivate : public QObject {
    Q_OBJECT

public:
    QString sourceDirectory;

    QList<QmlFileFilterItem*> qmlFileFilters() const;

    // content property
    QmlConcreteList<QmlProjectContentItem*> content;
};

QList<QmlFileFilterItem*> QmlProjectItemPrivate::qmlFileFilters() const
{
    QList<QmlFileFilterItem*> qmlFilters;
    for (int i = 0; i < content.size(); ++i) {
        QmlProjectContentItem *contentElement = content.at(i);
        QmlFileFilterItem *qmlFileFilter = qobject_cast<QmlFileFilterItem*>(contentElement);
        if (qmlFileFilter) {
            qmlFilters << qmlFileFilter;
        }
    }
    return qmlFilters;
}

QmlProjectItem::QmlProjectItem(QObject *parent) :
        QObject(parent),
        d_ptr(new QmlProjectItemPrivate)
{
//    Q_D(QmlProjectItem);
//
//    QmlFileFilter *defaultQmlFilter = new QmlFileFilter(this);
//    d->content.append(defaultQmlFilter);
}

QmlProjectItem::~QmlProjectItem()
{
    delete d_ptr;
}

QmlList<QmlProjectContentItem*> *QmlProjectItem::content()
{
    Q_D(QmlProjectItem);
    return &d->content;
}

QString QmlProjectItem::sourceDirectory() const
{
    const Q_D(QmlProjectItem);
    return d->sourceDirectory;
}

// kind of initialization
void QmlProjectItem::setSourceDirectory(const QString &directoryPath)
{
    Q_D(QmlProjectItem);

    if (d->sourceDirectory == directoryPath)
        return;

    d->sourceDirectory = directoryPath;

    for (int i = 0; i < d->content.size(); ++i) {
        QmlProjectContentItem *contentElement = d->content.at(i);
        FileFilterBaseItem *fileFilter = qobject_cast<FileFilterBaseItem*>(contentElement);
        if (fileFilter) {
            fileFilter->setDefaultDirectory(directoryPath);
            connect(fileFilter, SIGNAL(filesChanged()), this, SIGNAL(qmlFilesChanged()));
        }
    }

    emit sourceDirectoryChanged();
}

/* Returns list of absolute paths */
QStringList QmlProjectItem::qmlFiles() const
{
    const Q_D(QmlProjectItem);
    QStringList qmlFiles;

    for (int i = 0; i < d->content.size(); ++i) {
        QmlProjectContentItem *contentElement = d->content.at(i);
        QmlFileFilterItem *qmlFileFilter = qobject_cast<QmlFileFilterItem*>(contentElement);
        if (qmlFileFilter) {
            foreach (const QString &file, qmlFileFilter->files()) {
                if (!qmlFiles.contains(file))
                    qmlFiles << file;
            }
        }
    }
    return qmlFiles;
}

} // namespace QmlProjectManager

QML_DEFINE_NOCREATE_TYPE(QmlProjectManager::QmlProjectContentItem)
QML_DEFINE_TYPE(QmlProject,1,0,Project,QmlProjectManager::QmlProjectItem)

#include "qmlprojectitem.moc"
