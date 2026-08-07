#include "app_paths.h"
#include "main_window.h"

#include <QApplication>
#include <QDir>
#include <QFileInfo>
#include <QIcon>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("snake_game");
    app.setOrganizationName("local");

    AppPaths::ensureDataDirectories();

    MainWindow window;
    window.show();

    return app.exec();
}
