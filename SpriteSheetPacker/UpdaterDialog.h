#ifndef UPDATERDIALOG_H
#define UPDATERDIALOG_H

#include <QtWidgets>
#include <QNetworkAccessManager>

namespace Ui { class UpdaterDialog; }

class UpdaterDialog : public QDialog
{
    Q_OBJECT

public:
    explicit UpdaterDialog(const QString& lastVersion, const QString& changelog, QWidget *parent = 0);
    ~UpdaterDialog();

private slots:
    void on_downloadPushButton_clicked();
    void on_installPushButton_clicked();

private:
    Ui::UpdaterDialog*      ui;
    QNetworkAccessManager   _networkManager;
    QString                 _lastVersion;
    QString                 _updateFilePath;
};

#endif // UPDATERDIALOG_H
