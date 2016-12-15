#ifndef UPDATERDIALOG_H
#define UPDATERDIALOG_H

#include <QtWidgets>
#include <QWebEngineView>
#include <QWebEnginePage>

class PreviewPage : public QWebEnginePage
{
    Q_OBJECT
public:
    explicit PreviewPage(QObject *parent = nullptr) : QWebEnginePage(parent) {}

protected:
    bool acceptNavigationRequest(const QUrl &url, NavigationType type, bool isMainFrame);
};

class Content: public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString text MEMBER _text)
public:
    explicit Content(QObject *parent = nullptr) : QObject(parent) {}

    void setText(const QString &text) { _text = text; }

private:
    QString _text;
};

namespace Ui { class UpdaterDialog; }

class UpdaterDialog : public QDialog
{
    Q_OBJECT

public:
    explicit UpdaterDialog(const QString& changelog, QWidget *parent = 0);
    ~UpdaterDialog();

private slots:
    void on_downloadPushButton_clicked();

private:
    Ui::UpdaterDialog*      ui;
    QNetworkAccessManager   _networkManager;
    Content                 _content;
};

#endif // UPDATERDIALOG_H
