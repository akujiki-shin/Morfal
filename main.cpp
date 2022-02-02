#include "mainwindow.h"

#include <QApplication>
#include <QQuickWindow>
#include <QGuiApplication>
#include <QQmlApplicationEngine>

#include <QTranslator>
#include <QXmlStreamReader>

#include "utils.h"

#include "minimalscopeprofiler.h"

QString GetLocaleFromSettings()
{
    QString xmlPath;
    if (Utils::TryFindDirPath("settings", xmlPath))
    {
        xmlPath += "/Application.xml";

        QFile file(xmlPath);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text))
        {
            QXmlStreamReader reader(&file);
            while (reader.readNext() && !reader.isEndDocument())
            {
                if (!reader.isEndElement())
                {
                    const QString& elementName = reader.name().toString();
                    if (elementName == "localization")
                    {
                        return reader.attributes().value("Name").toString();
                    }
                }
            }
        }
    }

    return "";
}

void LoadLocalization(QApplication& application, QTranslator& translator, const QString& locale)
{
    profile();

    QString localizationPath;
    if (Utils::TryFindDirPath("settings", localizationPath))
    {
        localizationPath += "/localization";
        QProcess process;

        QString localeFileName = "morfal_" + locale;

        QStringList args;
        args.append(localeFileName + ".ts");

        process.setWorkingDirectory(localizationPath);
        process.start("lrelease", args);

        if (process.waitForStarted() && process.waitForFinished())
        {
            if (translator.load(localizationPath + "/" + localeFileName))
            {
                application.installTranslator(&translator);
            }
        }
        else if (translator.load(localizationPath + "/" + localeFileName))
        {
            application.installTranslator(&translator);
        }
    }
}

int main(int argc, char *argv[])
{
    QCoreApplication::setAttribute(Qt::AA_UseDesktopOpenGL);
    QQuickWindow::setGraphicsApi(QSGRendererInterface::OpenGL);

    QApplication application(argc, argv);

    QTranslator translator;
    const QString& locale = GetLocaleFromSettings();
    LoadLocalization(application, translator, locale);

    QString stylesheetPath;
    if (Utils::TryGetFirstFilePathIn("settings", ".qss", stylesheetPath))
    {
        QFile file(stylesheetPath);
        file.open(QFile::ReadOnly);
        QString styleSheet = QLatin1String(file.readAll());

        application.setStyleSheet(styleSheet);
    }

    MainWindow w;
    w.setWindowTitle("Morfal - Virtual Pen and Paper");
    w.show();

    //QQmlApplicationEngine engine;
    //engine.load(QUrl(QStringLiteral("qrc:/main.qml")));

    return application.exec();
}
