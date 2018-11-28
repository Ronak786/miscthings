#ifndef DIALOGWLANTEST_H
#define DIALOGWLANTEST_H

#include <QDialog>
#include <QString>
#include <QStringList>
#include <QKeyEvent>
#include <QHash>
#include <QListWidgetItem>
#include <device_test/passdialog.h>
#include <device_test/mylistwidget.h>
#include <QPlainTextEdit>

namespace Ui {
class DialogWlanTest;
}

class DialogWlanTest : public QDialog
{
    Q_OBJECT

public:
    explicit DialogWlanTest(QWidget *parent = 0);
    ~DialogWlanTest();
    void keyPressEvent(QKeyEvent *event);
    void setupnetwork(QString ssid, QString pass);
public slots:
    void handlePress(QListWidgetItem*);

private:
    Ui::DialogWlanTest *ui;
    QPlainTextEdit *_showedit;
    MyListWidget *_mlist;
    QHash<int, QString> hmap;
    QStringList m_SSID;
    QString     m_Password;
    int         m_SsidIndex;
    QString ssid;
    QString pass;
    const char *ifup = "/system/bin/ifconfig wlan0 up";
    const char *scan = "/system/bin/wpa_cli -i wlan0 -p /data/misc/wifi/sockets/ scan";
    const char *scanresult = "/system/bin/wpa_cli -i wlan0 -p /data/misc/wifi/sockets/ scan_results";
    const char *addnework = "/system/bin/wpa_cli -i wlan0 -p /data/misc/wifi/sockets/ add_network";
    char setnetwork[256] = "/data/bin/net.sh %s %s";
    const char *disablenetwork = "/system/bin/wpa_cli -i wlan0 -p /data/misc/wifi/sockets/ disable_network 0";

    // use together
    const char *enablenetwork = "/system/bin/wpa_cli -i wlan0 -p /data/misc/wifi/sockets/ enable_network 0";
    const char *dhcpcd = "/system/bin/dhcpcd wlan0";
    // use together


};

#endif // DIALOGWLANTEST_H
