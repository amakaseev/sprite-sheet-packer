#include <QWebChannel>
#include <QNetworkReply>

#include "UpdaterDialog.h"
#include "ui_UpdaterDialog.h"

bool PreviewPage::acceptNavigationRequest(const QUrl &url,
                                          QWebEnginePage::NavigationType /*type*/,
                                          bool /*isMainFrame*/)
{
    // Only allow qrc:/index.html.
    if (url.scheme() == QString("qrc"))
        return true;
    QDesktopServices::openUrl(url);
    return false;
}

UpdaterDialog::UpdaterDialog(const QString& changelog, QWidget *parent):
    QDialog(parent),
    ui(new Ui::UpdaterDialog)
{
    ui->setupUi(this);

    setWindowTitle(windowTitle() + QString(" - current version is %1").arg(QCoreApplication::applicationVersion()));

    PreviewPage *page = new PreviewPage(this);
    ui->preview->setPage(page);

    QWebChannel *channel = new QWebChannel(this);
    channel->registerObject(QStringLiteral("content"), &_content);
    page->setWebChannel(channel);

    ui->preview->setUrl(QUrl("qrc:/res/markdown/index.html"));

    _content.setText(changelog);
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

            QString filePath = QDir::tempPath() + QDir::separator() + QFileInfo(url.path()).fileName();
            qDebug() << filePath;

            QFile file(filePath);
            if (!file.open(QIODevice::WriteOnly)) {
                qDebug() << QString("Could not open %1 for writing: %2\n").arg(filePath).arg(file.errorString());
            } else {
                file.write(reply->readAll());
                file.close();
            }
        }
        reply->deleteLater();
    });

    QUrl url = QUrl::fromEncoded("https://github-cloud.s3.amazonaws.com/releases/32418767/7a456396-bc9c-11e6-9cad-9ad4d5544727.dmg?X-Amz-Algorithm=AWS4-HMAC-SHA256&X-Amz-Credential=AKIAISTNZFOVBIJMK3TQ%2F20161215%2Fus-east-1%2Fs3%2Faws4_request&X-Amz-Date=20161215T173044Z&X-Amz-Expires=300&X-Amz-Signature=81cdd34ea54a53901fe1ae459a5d0d8fa3dd2de2910eb1cd74ac087ff45dc378&X-Amz-SignedHeaders=host&actor_id=0&response-content-disposition=attachment%3B%20filename%3DSpriteSheetPacker.dmg&response-content-type=application%2Foctet-stream");
    QNetworkReply* reply = _networkManager.get(QNetworkRequest(url));

    connect(reply, &QNetworkReply::downloadProgress,[this](qint64 bytesReceived, qint64 bytesTotal){
        qDebug() << bytesReceived << ":" << bytesTotal;
        ui->progressBar->setValue(bytesReceived / 1024);
        ui->progressBar->setMaximum(bytesTotal / 1024);
    });
}
