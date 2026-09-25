#include <QApplication>
#include <QTranslator>
#include <QSettings>
#include <QStyleFactory>
#include <QSvgRenderer>
#include <QDir>
#include <QDebug>

#include "ui/mainwindow.h"


static QTranslator *appTranslator = nullptr;

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    app.setStyle(QStyleFactory::create("Fusion"));

    
    QSvgRenderer svgRenderer(QByteArray(
        "<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 1 1\">"
        "<rect width=\"1\" height=\"1\" fill=\"none\"/></svg>"));

    
    if (QDir("/usr/share/paintux/assets").exists()) {
        QDir::setCurrent("/usr/share/paintux");
    } else if (QDir(QCoreApplication::applicationDirPath() + "/assets").exists()) {
        QDir::setCurrent(QCoreApplication::applicationDirPath());
    } else if (QDir(QCoreApplication::applicationDirPath() + "/../assets").exists()) {
        QDir::setCurrent(QCoreApplication::applicationDirPath() + "/..");
    }

    
    QSettings settings("Paintux", "PaintuxStudio");
    QString lang = settings.value("language", "es").toString();

    appTranslator = new QTranslator(&app);

    QStringList searchPaths;
    searchPaths << QApplication::applicationDirPath() + "/translations"
                << QApplication::applicationDirPath()
                << QDir::currentPath() + "/translations"
                << QDir::currentPath()
                << "/usr/share/paintux/translations";

    bool loaded = false;
    
    for (const QString &path : searchPaths) {
        if (appTranslator->load("paintux_" + lang, path)) {
            app.installTranslator(appTranslator);
            loaded = true;
            break;
        }
    }
    if (!loaded) {
        delete appTranslator;
        appTranslator = nullptr;
    }

    
    mainwind window;
    window.show();

    return app.exec();
}

#include "main.moc"