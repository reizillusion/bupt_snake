#pragma once

#include <QString>

namespace AppPaths {

QString projectRoot();
QString assetRoot();
QString dataRoot();
QString assetPath(const QString &relativePath);
QString dataPath(const QString &relativePath);
void ensureDataDirectories();

}
