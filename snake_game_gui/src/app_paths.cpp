#include "app_paths.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>

namespace AppPaths {

static QString normalizePath(const QString &path)
{
    return QDir::fromNativeSeparators(QDir(path).absolutePath());
}

QString projectRoot()
{
    return normalizePath(QString::fromUtf8(PROJECT_ROOT));
}

static QString firstExistingRoot(const QStringList &candidates, const QString &childName)
{
    for (const QString &candidate : candidates) {
        const QString absolute = normalizePath(candidate);
        if (QFileInfo::exists(QDir(absolute).filePath(childName))) {
            return absolute;
        }
    }
    return normalizePath(candidates.isEmpty() ? QDir::currentPath() : candidates.first());
}

QString assetRoot()
{
    const QString appDir = QCoreApplication::applicationDirPath();
    const QStringList candidates = {
        QDir(appDir).filePath("assets"),
        QDir(appDir).filePath("../assets"),
        QDir(appDir).filePath("../../assets"),
        QDir(projectRoot()).filePath("assets")
    };
    return firstExistingRoot(candidates, "music");
}

QString dataRoot()
{
    const QString appDir = QCoreApplication::applicationDirPath();
    const QStringList candidates = {
        QDir(appDir).filePath("data"),
        QDir(appDir).filePath("../data"),
        QDir(appDir).filePath("../../data"),
        QDir(projectRoot()).filePath("data")
    };
    return firstExistingRoot(candidates, QString());
}

QString assetPath(const QString &relativePath)
{
    return QDir(assetRoot()).filePath(relativePath);
}

QString dataPath(const QString &relativePath)
{
    return QDir(dataRoot()).filePath(relativePath);
}

void ensureDataDirectories()
{
    QDir().mkpath(dataRoot());
}

}
