#ifndef MERCURIALCLIENT_H
#define MERCURIALCLIENT_H

#include <QtCore/QObject>
#include <QtCore/QFileInfo>
#include <QtCore/QByteArray>
#include <QtCore/QPair>

namespace Core {
class ICore;
}

namespace VCSBase{
class VCSBaseEditor;
}

namespace Mercurial {
namespace Internal {

class MercurialJobRunner;

class MercurialClient : public QObject
{
    Q_OBJECT
public:
    MercurialClient();
    ~MercurialClient();
    bool add(const QString &fileName);
    bool remove(const QString &fileName);
    bool manifestSync(const QString &filename);
    QString branchQuerySync(const QFileInfo &repositoryRoot);
    void annotate(const QFileInfo &file);
    void diff(const QFileInfo &fileOrDir);
    void log(const QFileInfo &fileOrDir);
    void import(const QFileInfo &repositoryRoot, const QStringList &files);
    void pull(const QFileInfo &repositoryRoot, const QString &repository);
    void push(const QFileInfo &repositoryRoot, const QString &repository);
    void incoming(const QFileInfo &repositoryRoot, const QString &repository);
    void outgoing(const QFileInfo &repositoryRoot);
    void status(const QFileInfo &fileOrDir);
    void statusWithSignal(const QFileInfo &fileOrDir);
    void revert(const QFileInfo &fileOrDir, const QString &revision);
    void update(const QFileInfo &repositoryRoot, const QString &revision);
    void commit(const QFileInfo &repositoryRoot, const QStringList &files,
                const QString &commiterInfo, const QString &commitMessageFile);

    static QString findTopLevelForFile(const QFileInfo &file);

signals:
    void parsedStatus(const QList<QPair<QString, QString> > &statusList);

public slots:
    void view(const QString &source, const QString &id);
    void settingsChanged();

private slots:
    void statusParser(const QByteArray &data);

private:
    bool hgProcessSync(const QFileInfo &file, const QStringList &args,
                       QByteArray *output=0) const;

    MercurialJobRunner *jobManager;
    Core::ICore *core;

    VCSBase::VCSBaseEditor *createVCSEditor(const QString &kind, QString title,
                                            const QString &source, bool setSourceCodec,
                                            const char *registerDynamicProperty,
                                            const QString &dynamicPropertyValue) const;
};

} //namespace Internal
} //namespace Mercurial

#endif // MERCURIALCLIENT_H
