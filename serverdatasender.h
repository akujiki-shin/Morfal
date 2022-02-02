#pragma once

#include <QObject>

#include <QAbstractSocket>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class QTcpSocket;
class QNetworkAccessManager;
class QString;

class ServerDataSender : public QObject
{
private:
    Q_OBJECT

    using super = QObject;

public:
    explicit ServerDataSender(Ui::MainWindow* mainWindowUI, QObject *parent = nullptr);
    ~ServerDataSender();

private slots:
    void OnServerAddressEditingFinished();

    void SocketReadyRead();
    void SocketConnected();
    void SocketClosed();
    void SocketStateChanged(QAbstractSocket::SocketState);

    void OnSendButtonClicked();

private:
    void ConnectToServer();
    void SendToServer(const QString& text);

private:
    Ui::MainWindow* ui { nullptr };
    QTcpSocket* m_Socket { nullptr };
    QNetworkAccessManager* m_NetworkAccessManager { nullptr };
};
