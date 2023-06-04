// Morfal is a Virtual Pen and Paper (VPP): a Game Master's toolbox designed for those who play Role Playing Games with pen and paper only,
// as opposed to Virtual Table Tops (VTT) which are made to handle tactical gameplay (tokens, square/hexagons, area of effect, etc...).
// Copyright (C) 2022  akujiki
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

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

    return application.exec();
}
