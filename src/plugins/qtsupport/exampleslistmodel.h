#ifndef EXAMPLESLISTMODEL_H
#define EXAMPLESLISTMODEL_H

#include <QAbstractListModel>
#include <QStringList>
#include <QtGui/QSortFilterProxyModel>


QT_BEGIN_NAMESPACE
class QXmlStreamReader;
QT_END_NAMESPACE

namespace QtSupport {
namespace Internal {

enum ExampleRoles { Name=Qt::UserRole, ProjectPath, Description, ImageUrl,
                    DocUrl,  FilesToOpen, Tags, Difficulty, HasSourceCode, Type };

enum InstructionalType { Example=0, Demo, Tutorial };

struct ExampleItem {
    ExampleItem(): difficulty(0) {}
    InstructionalType type;
    QString name;
    QString projectPath;
    QString description;
    QString imageUrl;
    QString docUrl;
    QStringList filesToOpen;
    QStringList tags;
    int difficulty;
    bool hasSourceCode;
};

class ExamplesListModel : public QAbstractListModel {
    Q_OBJECT
public:
    explicit ExamplesListModel(QObject *parent);
    void addItems(const QList<ExampleItem> &items);

    virtual int rowCount(const QModelIndex &parent = QModelIndex()) const;
    virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;

    QStringList tags() const { return m_tags; }

public slots:
    void readNewsItems(const QString &examplesPath, const QString &demosPath, const QString &sourcePath);

signals:
    void tagsUpdated();

private:
    QList<ExampleItem> parseExamples(QXmlStreamReader* reader, const QString& projectsOffset);
    QList<ExampleItem> parseDemos(QXmlStreamReader* reader, const QString& projectsOffset);
    QList<ExampleItem> parseTutorials(QXmlStreamReader* reader, const QString& projectsOffset);
    void clear();
    QStringList exampleSources() const;
    QList<ExampleItem> exampleItems;
    QStringList m_tags;
};

class ExamplesListModelFilter : public QSortFilterProxyModel {
    Q_OBJECT
public:
    Q_PROPERTY(bool showTutorialsOnly READ showTutorialsOnly WRITE setShowTutorialsOnly NOTIFY showTutorialsOnlyChanged)
    Q_PROPERTY(QString filterTag READ filterTag WRITE setFilterTag NOTIFY filterTagChanged)

    explicit ExamplesListModelFilter(QObject *parent);

    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const;
    bool showTutorialsOnly() {return m_showTutorialsOnly;}

    QString filterTag() const
    {
        return m_filterTag;
    }

public slots:
    void setFilterTag(const QString& arg)
    {
        if (m_filterTag != arg) {
            m_filterTag = arg;
            emit filterTagChanged(arg);
        }
    }
    void updateFilter();

signals:
    void showTutorialsOnlyChanged();

    void filterTagChanged(const QString& arg);

private slots:
    void setShowTutorialsOnly(bool showTutorialsOnly);

private:
    bool m_showTutorialsOnly;
    QString m_filterTag;
};

} // namespace Internal
} // namespace QtSupport

#endif // EXAMPLESLISTMODEL_H


