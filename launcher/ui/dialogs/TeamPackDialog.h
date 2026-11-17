#pragma once

#include <QDialog>
#include <QFile>
#include <QPointer>
#include <QTemporaryDir>
#include <QUrl>
#include <QVector>

class QCloseEvent;
class QComboBox;
class QLabel;
class QLineEdit;
class QNetworkReply;
class QProgressBar;
class QPushButton;

class TeamPackDialog final : public QDialog {
    Q_OBJECT
   public:
    explicit TeamPackDialog(QWidget* parent = nullptr);
    void reject() override;

   protected:
    void closeEvent(QCloseEvent* event) override;

   private:
    struct Pack {
        QString name;
        QString version;
        QString downloadUrl;
        QString sha256;
        qint64 size = 0;
    };

    QUrl serverBaseUrl() const;
    void refreshPacks();
    void installSelectedPack();
    void downloadFinished();
    void extractPack(const QString& folderName);
    void setBusy(bool busy);
    void cancelNetworkRequest();
    void showError(const QString& message);


    QLineEdit* m_serverUrl = nullptr;
    QComboBox* m_packSelector = nullptr;
    QLabel* m_status = nullptr;
    QProgressBar* m_progress = nullptr;
    QPushButton* m_refreshButton = nullptr;
    QPushButton* m_installButton = nullptr;
    QPushButton* m_closeButton = nullptr;
    bool m_installTaskRunning = false;
    QPointer<QNetworkReply> m_reply;
    QFile m_downloadFile;
    QTemporaryDir m_tempDir;
    QVector<Pack> m_packs;
    QString m_archivePath;


};




