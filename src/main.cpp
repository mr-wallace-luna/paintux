#include <QApplication>
#include <QTranslator>
#include <QSettings>
#include <QStyleFactory>
#include <QSvgRenderer>
#include <QDir>
#include <QFileInfo>
#include <QDebug>

#include "ui/mainwindow.h"


static QTranslator *appTranslator = nullptr;

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    app.setStyle(QStyleFactory::create("Fusion"));

    QSvgRenderer svgRenderer(QByteArray(
        "<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 1 1\">"
        "<rect width=\"1\" height=\"1\" fill=\"none\"/></svg>"));

    // ============================================================
    // Localizar directorio de assets
    // ============================================================
    const QString appDir = QCoreApplication::applicationDirPath();

    QStringList assetCandidates;
    assetCandidates
        << QDir::currentPath() + "/assets"
        << appDir + "/assets"
        << appDir + "/../assets"
        << appDir + "/../../assets"
        << QStringLiteral("/usr/share/paint-ux/assets")      // ruta REAL del CMakeLists
        << QStringLiteral("/usr/local/share/paint-ux/assets")
        << QStringLiteral("/usr/share/paintux/assets");      // compatibilidad nombre viejo

    QString assetPath;
    for (const QString &cand : assetCandidates) {
        if (QDir(cand).exists()) {
            assetPath = QDir(cand).absolutePath();
            break;
        }
    }

    if (!assetPath.isEmpty()) {
        // Poner el directorio PADRE de assets como CWD
        QDir::setCurrent(QFileInfo(assetPath).absolutePath());
        qDebug() << "[Paint-UX] Assets encontrados en:" << assetPath;
    } else {
        qWarning() << "[Paint-UX] No se encontro el directorio de assets.";
    }

    // ============================================================
    // Idioma
    // ============================================================
    QSettings settings("Paintux", "PaintuxStudio");
    QString lang = settings.value("language", "es").toString();

    appTranslator = new QTranslator(&app);

    QStringList searchPaths;
    searchPaths << appDir + "/translations"
                << appDir + "/../translations"
                << appDir + "/../../translations"
                << QDir::currentPath() + "/translations"
                << QStringLiteral("/usr/share/paint-ux/translations")
                << QStringLiteral("/usr/local/share/paint-ux/translations")
                << QStringLiteral("/usr/share/paintux/translations");

    bool loaded = false;
    for (const QString &path : searchPaths) {
        if (appTranslator->load("paintux_" + lang, path)) {
            app.installTranslator(appTranslator);
            loaded = true;
            qDebug() << "[Paint-UX] Traduccion cargada:" << path;
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