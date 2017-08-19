#include <QJSEngine>
#include <QNetworkReply>

#include "UpdaterDialog.h"
#include "ui_UpdaterDialog.h"

#if defined(Q_OS_OSX)
    static QString updaterFileName = "SpriteSheetPacker-Installer.dmg";
#elif defined(Q_OS_WIN)
    static QString updaterFileName = "SpriteSheetPacker-Installer.exe";
#elif defined(Q_OS_LINUX)
    static QString updaterFileName = "SpriteSheetPacker-Installer.AppImage";
#endif


UpdaterDialog::UpdaterDialog(const QString& lastVersion, const QString& changelog, QWidget *parent):
    QDialog(parent),
    ui(new Ui::UpdaterDialog),
    _lastVersion(lastVersion)
{
    ui->setupUi(this);

    QJSEngine engine;
    QFile scriptFile("://res/markdown/micromarkdown.js");
    scriptFile.open(QIODevice::ReadOnly);
    QTextStream stream(&scriptFile);
    QString fileContent = stream.readAll();
    scriptFile.close();

    QJSValue result = engine.evaluate(fileContent);
    if (result.isError()) {
        QString errorString = "Uncaught exception at line " + result.property("lineNumber").toString() + " : " + result.toString();
        qDebug() << errorString;
    } else {
        if (engine.globalObject().hasOwnProperty("convertMD")) {
            QJSValueList args;
            args << QJSValue(changelog);

            // run convertMD
            QJSValue convertMD = engine.globalObject().property("convertMD");
            result = convertMD.call(args);

            if (result.isError()) {
                QString errorString = "Uncaught exception at line " + result.property("lineNumber").toString() + " : " + result.toString();
                qDebug() << errorString;
            } else {
                ui->textBrowser->setHtml(result.toString());
            }
        }
    }


    //setWindowTitle(QString("Current version is %1 update to %2").arg(QCoreApplication::applicationVersion()).arg(_lastVersion));
    ui->installPushButton->setVisible(false);
}

UpdaterDialog::~UpdaterDialog()
{
    delete ui;
}

void UpdaterDialog::on_downloadPushButton_clicked() {
    ui->downloadPushButton->setEnabled(false);

    connect(&_networkManager, &QNetworkAccessManager::finished, [this](QNetworkReply* reply) {
        QUrl url = reply->url();
        if (reply->error()) {
            qDebug() << QString("Download of [%1] failed: %2").arg(url.toEncoded().constData()).arg(qPrintable(reply->errorString()));
            ui->downloadPushButton->setEnabled(true);
        } else {
            qDebug() << QString("Download of [%1] succeeded").arg(url.toEncoded().constData());

            QUrl redirected_url = reply->attribute(QNetworkRequest::RedirectionTargetAttribute).toUrl();
            if (!redirected_url.isEmpty()) {
                qDebug() << redirected_url;

                QNetworkReply* redirectedReply = _networkManager.get(QNetworkRequest(redirected_url));
                connect(redirectedReply, &QNetworkReply::downloadProgress,[this](qint64 bytesReceived, qint64 bytesTotal){
                    qDebug() << bytesReceived << ":" << bytesTotal;
                    ui->progressBar->setValue(bytesReceived / 1024);
                    ui->progressBar->setMaximum(bytesTotal / 1024);
                    ui->downloadProgressLabel->setText(QString("%1Kb of %2Kb").arg(bytesReceived / 1024).arg(bytesTotal / 1024));
                });
            } else {
                _updateFilePath = QDir::tempPath() + QDir::separator() + updaterFileName;
                qDebug() << _updateFilePath;

                QFile file(_updateFilePath);
                if (!file.open(QIODevice::WriteOnly)) {
                    qDebug() << QString("Could not open %1 for writing: %2\n").arg(_updateFilePath).arg(file.errorString());
                    ui->downloadPushButton->setEnabled(true);
                } else {
                    file.write(reply->readAll());
                    file.close();
                    ui->downloadPushButton->setDefault(false);
                    ui->downloadPushButton->setVisible(false);
                    ui->installPushButton->setVisible(true);
                    ui->installPushButton->setDefault(true);
                }
            }
        }
        reply->deleteLater();
    });

    QUrl url = QUrl::fromEncoded(QString("https://github.com/amakaseev/sprite-sheet-packer/releases/download/%1/%2")
                                 .arg(_lastVersion)
                                 .arg(updaterFileName)
                                 .toUtf8());
    QNetworkReply* reply = _networkManager.get(QNetworkRequest(url));

    connect(reply, &QNetworkReply::downloadProgress,[this](qint64 bytesReceived, qint64 bytesTotal){
        qDebug() << bytesReceived << ":" << bytesTotal;
        ui->progressBar->setValue(bytesReceived / 1024);
        ui->progressBar->setMaximum(bytesTotal / 1024);
        ui->downloadProgressLabel->setText(QString("%1Kb of %2Kb").arg(bytesReceived / 1024).arg(bytesTotal / 1024));
    });
}

void UpdaterDialog::on_installPushButton_clicked() {
    qDebug() << "Install:" << _updateFilePath;
    QProcess *process = new QProcess(this);
#if defined(Q_OS_OSX)
    process->start(QString("hdiutil mount ") + _updateFilePath);
    process->waitForFinished();
    qDebug() << process->readAllStandardOutput();
    qApp->quit();
#elif defined(Q_OS_WIN)
    process->start(_updateFilePath);
#endif
}
