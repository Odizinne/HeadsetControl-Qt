#include "headsetcontrolqt.h"
#include "headsetcontrolchecker.h"
#include <QApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QProcess>
#include <QDebug>
#include <QTranslator>
#include <QSettings>

int main(int argc, char *argv[])
{
    qputenv("QT_QUICK_CONTROLS_MATERIAL_VARIANT", "Dense");
    QApplication a(argc, argv);
#ifdef _WIN32
    a.setStyle("fusion");
#endif
    a.setQuitOnLastWindowClosed(false);
    a.setOrganizationName("Odizinne");
    a.setApplicationName("HeadsetControlQt");
    QApplication::setWindowIcon(QIcon(":/icons/icon.png"));
#ifdef __linux__
    QProcess process;
    process.start("which", QStringList() << "headsetcontrol");
    process.waitForFinished();
    bool headsetControlInstalled = (process.exitCode() == 0);
    if (!headsetControlInstalled) {
        qDebug() << "HeadsetControl is missing on your system. Please install it.";
        return 1;
    }
    HeadsetControlQt w;
    return a.exec();
#else
    HeadsetControlChecker checker;
    bool headsetControlInstalled = checker.isInstalled();
    QQmlApplicationEngine* promptEngine = nullptr;
    bool startMainApp = false;
    if (!headsetControlInstalled) {
        QSettings settings("Odizinne", "HeadsetControlQt");
        int languageIndex = settings.value("language", 0).toInt();

        QTranslator* translator = new QTranslator();
        QString languageCode;
        if (languageIndex == 0) {
            QLocale systemLocale;
            languageCode = systemLocale.name().left(2);
        } else {
            switch (languageIndex) {
            case 1:
                languageCode = "en";
                break;
            case 2:
                languageCode = "fr";
                break;
            case 3:
                languageCode = "de";
                break;
            case 4:
                languageCode = "es";
                break;
            case 5:
                languageCode = "it";
                break;
            case 6:
                languageCode = "hu";
                break;
            case 7:
                languageCode = "tr";
                break;
            default:
                languageCode = "en";
                break;
            }
        }

        QString translationFile = QString(":/i18n/HeadsetControl-Qt_%1.qm").arg(languageCode);
        if (translator->load(translationFile)) {
            a.installTranslator(translator);
        }

        promptEngine = new QQmlApplicationEngine();
        promptEngine->setInitialProperties({{"checker", QVariant::fromValue(&checker)}});
        promptEngine->loadFromModule("Odizinne.HeadsetControlQt", "HeadsetcontrolPrompt");
        const QList<QObject*> rootObjects = promptEngine->rootObjects();
        QObject* rootObject = rootObjects.first();
        QObject::connect(&checker, &HeadsetControlChecker::downloadCompleted, &a, [&]() {
            startMainApp = true;
            a.quit();
        });
        QObject::connect(rootObject, SIGNAL(installCancelled()), &a, SLOT(quit()));
        a.exec();

        delete promptEngine;
        if (translator) {
            delete translator;
        }

        if (startMainApp) {
            HeadsetControlQt w;
            return a.exec();
        } else {
            return 0;
        }
    } else {
        HeadsetControlQt w;
        return a.exec();
    }
#endif
}
