#include "TeamPackDialog.h"


#include "Application.h"
#include <InstanceList.h>
#include <tasks/Task.h>

#include <QComboBox>
#include <QCloseEvent>
#include <QDir>
#include <QFileInfo>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QProgressBar>
#include <QPushButton>
#include <QRegularExpression>
#include <QUuid>
#include <archive/ArchiveReader.h>
#include <archive/ExtractZipTask.h>
#include "ProgressDialog.h"
#include <memory>

#include <QVBoxLayout>
#include <QCryptographicHash>



namespace {
QString sha256File(const QString& path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        return {};
    }

    QCryptographicHash hash(QCryptographicHash::Sha256);
    while (!file.atEnd()) {
        const QByteArray chunk = file.read(1024 * 1024);
        if (chunk.isEmpty() && file.error() != QFileDevice::NoError) {
            return {};
        }
        hash.addData(chunk);
    }
    return QString::fromLatin1(hash.result().toHex());
}

bool isZipFile(const QString& path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }
    const auto header = file.read(4);
    return header.size() == 4 && header[0] == 'P' && header[1] == 'K';
}

QString archiveSubdirectory(const QString& path)
{
    MMCZip::ArchiveReader archive(path);
    if (!archive.collectFiles()) {
        return {};
    }

    const QStringList files = archive.getFiles();
    if (files.isEmpty()) {
        return {};
    }

    for (const QString& file : files) {
        const QString normalized = QDir::fromNativeSeparators(file);
        if (normalized == QStringLiteral("instance.cfg") || normalized.endsWith(QStringLiteral("/instance.cfg"))) {
            const qsizetype separator = normalized.lastIndexOf('/');
            if (separator >= 0) {
                return normalized.left(separator + 1);
            }
            return {};
        }
    }
    for (const QString& file : files) {
        const QString normalized = QDir::fromNativeSeparators(file);
        if (normalized == QStringLiteral("mmc-pack.json") || normalized.endsWith(QStringLiteral("/mmc-pack.json"))) {
            const qsizetype separator = normalized.lastIndexOf('/');
            if (separator >= 0) {
                return normalized.left(separator + 1);
            }
            return {};
        }
    }
    return {};
}

bool isValidInstanceFolderName(const QString& name)
{
    const QString value = name.trimmed();
    if (value.isEmpty() || value.size() > 80 || value == QStringLiteral(".") || value == QStringLiteral("..") ||
        value.endsWith(' ') || value.endsWith('.')) {
        return false;
    }
    for (const QChar character : value) {
        if (character.unicode() < 32 || QStringLiteral("<>:\"/\\|?*").contains(character)) {
            return false;
        }
    }

    const QString baseName = value.section('.', 0, 0).toUpper();
    static const QRegularExpression reservedName(QStringLiteral("^(CON|PRN|AUX|NUL|COM[1-9]|LPT[1-9])$"));
    return !reservedName.match(baseName).hasMatch();
}
}  // namespace

TeamPackDialog::TeamPackDialog(QWidget* parent) : QDialog(parent)
{
    setWindowTitle(tr("Team Mods"));
    setMinimumWidth(460);

    auto* layout = new QVBoxLayout(this);
    auto* intro = new QLabel(tr("Download the team's ZIP, choose a new instance folder name, and extract the files into it."), this);
    intro->setWordWrap(true);
    layout->addWidget(intro);

    auto* form = new QFormLayout();
    m_serverUrl = new QLineEdit(APPLICATION->settings()->get("TeamPackServerURL").toString(), this);
    m_serverUrl->setPlaceholderText(QStringLiteral("http://fusion.pikamc.vn:25832"));
    form->addRow(tr("Team server:"), m_serverUrl);
    layout->addLayout(form);

    auto* packRow = new QHBoxLayout();
    m_packSelector = new QComboBox(this);
    m_packSelector->setSizeAdjustPolicy(QComboBox::AdjustToContents);
    m_refreshButton = new QPushButton(tr("Refresh"), this);
    packRow->addWidget(m_packSelector, 1);
    packRow->addWidget(m_refreshButton);
    layout->addLayout(packRow);

    m_progress = new QProgressBar(this);
    m_progress->setRange(0, 100);
    m_progress->setValue(0);
    layout->addWidget(m_progress);

    m_status = new QLabel(this);
    m_status->setWordWrap(true);
    layout->addWidget(m_status);

    auto* buttons = new QHBoxLayout();
    buttons->addStretch();
    m_installButton = new QPushButton(tr("Download and extract"), this);
    m_closeButton = new QPushButton(tr("Close"), this);
    m_installButton->setDefault(true);
    buttons->addWidget(m_installButton);
    buttons->addWidget(m_closeButton);
    layout->addLayout(buttons);

    connect(m_refreshButton, &QPushButton::clicked, this, &TeamPackDialog::refreshPacks);
    connect(m_installButton, &QPushButton::clicked, this, &TeamPackDialog::installSelectedPack);
    connect(m_closeButton, &QPushButton::clicked, this, &TeamPackDialog::reject);
    connect(m_serverUrl, &QLineEdit::returnPressed, this, &TeamPackDialog::refreshPacks);
    connect(m_packSelector, &QComboBox::currentIndexChanged, this, [this](int index) {
        m_installButton->setEnabled(index >= 0 && index < m_packs.size() && !m_reply);
    });

    refreshPacks();
}

QUrl TeamPackDialog::serverBaseUrl() const
{
    QString text = m_serverUrl->text().trimmed();
    while (text.endsWith('/')) {
        text.chop(1);
    }
    QUrl url(text);
    if (!url.isValid() || url.host().isEmpty() || (url.scheme() != "http" && url.scheme() != "https")) {
        return {};
    }
    return url;
}

void TeamPackDialog::refreshPacks()
{
    if (m_reply) {
        return;
    }

    const QUrl base = serverBaseUrl();
    if (base.isEmpty()) {
        showError(tr("Enter a valid team server address using http:// or https://."));
        return;
    }

    APPLICATION->settings()->set("TeamPackServerURL", base.toString());
    m_packs.clear();
    m_packSelector->clear();
    m_status->setText(tr("Checking the team server…"));
    setBusy(true);
    m_refreshButton->setEnabled(false);
    m_installButton->setEnabled(false);
    QNetworkRequest request(base.resolved(QUrl(QStringLiteral("/api/packs"))));
    request.setTransferTimeout(30000);
    m_reply = APPLICATION->network()->get(request);
    connect(m_reply, &QNetworkReply::finished, this, [this] {
        const auto reply = m_reply;
        m_reply = nullptr;
        setBusy(false);
        if (!reply) {
            return;
        }

        const QByteArray body = reply->readAll();
        const auto networkError = reply->error();
        const QString errorText = reply->errorString();
        const int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        reply->deleteLater();
        if (networkError != QNetworkReply::NoError || statusCode < 200 || statusCode >= 300) {
            showError(tr("Could not read the team pack list: %1").arg(errorText));
            return;
        }

        QJsonParseError parseError;
        const auto document = QJsonDocument::fromJson(body, &parseError);
        if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
            showError(tr("The team server returned an invalid pack list."));
            return;
        }

        m_packs.clear();
        m_packSelector->clear();
        const auto packs = document.object().value("packs").toArray();
        for (const auto& value : packs) {
            const auto object = value.toObject();
            Pack pack;
            pack.name = object.value("name").toString();
            pack.version = object.value("version").toString();
            pack.downloadUrl = object.value("downloadUrl").toString();
            pack.sha256 = object.value("sha256").toString();
            pack.size = static_cast<qint64>(object.value("size").toDouble());
            if (pack.name.isEmpty() || pack.downloadUrl.isEmpty() || pack.sha256.size() != 64 || pack.size <= 0) {
                continue;
            }
            m_packs.append(pack);
            m_packSelector->addItem(QStringLiteral("%1  ·  %2").arg(pack.name, pack.version));
        }

        if (m_packs.isEmpty()) {
            m_status->setText(tr("The server is reachable, but no ZIP has been uploaded yet."));
            m_installButton->setEnabled(false);
        } else {
            m_status->setText(tr("ZIP ready: %1 (%2 bytes). It will be extracted into a new instance folder.")
                                  .arg(m_packs.first().name)
                                  .arg(m_packs.first().size));
            m_installButton->setEnabled(true);
        }
    });
}

void TeamPackDialog::installSelectedPack()
{
    if (m_packs.isEmpty() || m_packSelector->currentIndex() < 0) {
        return;
    }

    const auto result = QMessageBox::question(this, tr("Download team ZIP?"),
                                              tr("Download and extract this ZIP into a new instance folder. Existing instances will not be changed."),
                                              QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    if (result != QMessageBox::Yes) {
        return;
    }

    const Pack pack = m_packs.at(m_packSelector->currentIndex());
    const QUrl base = serverBaseUrl();
    if (base.isEmpty()) {
        showError(tr("Enter a valid team server address using http:// or https://."));
        return;
    }
    const QUrl downloadUrl = base.resolved(QUrl(pack.downloadUrl));
    if (downloadUrl.host() != base.host() || downloadUrl.scheme() != base.scheme() || downloadUrl.port() != base.port()) {
        showError(tr("The team server returned a download URL outside its own address."));
        return;
    }

    if (!m_tempDir.isValid()) {
        showError(tr("Could not create temporary storage for the team instance."));
        return;
    }
    m_archivePath = QDir(m_tempDir.path()).filePath(QStringLiteral("team-instance.zip"));
    m_downloadFile.setFileName(m_archivePath);
    if (!m_downloadFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        showError(tr("Could not create a temporary ZIP file: %1").arg(m_downloadFile.errorString()));
        return;
    }
    setBusy(true);
    m_progress->setRange(0, 0);
    m_status->setText(tr("Downloading full instance %1…").arg(pack.name));
    QNetworkRequest request(downloadUrl);
    request.setTransferTimeout(120000);
    m_reply = APPLICATION->network()->get(request);
    connect(m_reply, &QNetworkReply::readyRead, this, [this] {
        if (!m_reply || !m_downloadFile.isOpen()) {
            return;
        }
        const QByteArray bytes = m_reply->readAll();
        if (m_downloadFile.write(bytes) != bytes.size()) {
            m_reply->abort();
            m_status->setText(tr("Could not write the downloaded ZIP file."));
        }
    });
    connect(m_reply, &QNetworkReply::downloadProgress, this, [this](qint64 received, qint64 total) {
        if (total > 0) {
            m_progress->setRange(0, 100);
            m_progress->setValue(static_cast<int>((received * 100) / total));
        }
    });
    connect(m_reply, &QNetworkReply::finished, this, &TeamPackDialog::downloadFinished);
}
void TeamPackDialog::downloadFinished()
{
    const auto reply = m_reply;
    m_reply = nullptr;
    if (!reply) {
        return;
    }
    if (m_downloadFile.isOpen()) {
        const QByteArray remaining = reply->readAll();
        if (!remaining.isEmpty() && m_downloadFile.write(remaining) != remaining.size()) {
            reply->abort();
        }
        m_downloadFile.flush();
        m_downloadFile.close();
    }

    const auto networkError = reply->error();
    const QString errorText = reply->errorString();
    const int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    reply->deleteLater();
    if (networkError != QNetworkReply::NoError || statusCode < 200 || statusCode >= 300) {
        showError(tr("The team instance ZIP could not be downloaded: %1").arg(errorText));
        return;
    }

    const Pack pack = m_packs.at(m_packSelector->currentIndex());
    const QFileInfo archiveInfo(m_archivePath);
    if (archiveInfo.size() != pack.size || !isZipFile(m_archivePath)) {
        showError(tr("The downloaded file is incomplete or is not a ZIP archive."));
        return;
    }
    if (sha256File(m_archivePath).compare(pack.sha256, Qt::CaseInsensitive) != 0) {
        showError(tr("The downloaded instance ZIP does not match the server's SHA-256 checksum."));
        return;
    }
    QString folderName = pack.name.trimmed();
    while (true) {
        bool accepted = false;
        folderName = QInputDialog::getText(this, tr("Name the new instance folder"),
                                           tr("Enter a folder name under:\n%1").arg(APPLICATION->instances()->primaryDir()),
                                           QLineEdit::Normal, folderName, &accepted).trimmed();
        if (!accepted) {
            m_status->setText(tr("Download complete. Choose Download and extract to try again."));
            setBusy(false);
            return;
        }
        if (!isValidInstanceFolderName(folderName)) {
            QMessageBox::warning(this, tr("Invalid folder name"),
                                 tr("Use a name up to 80 characters. Do not use Windows reserved characters, names, or a trailing dot or space."));
            continue;
        }
        const QString targetPath = QDir(APPLICATION->instances()->primaryDir()).filePath(folderName);
        if (QFileInfo::exists(targetPath)) {
            QMessageBox::warning(this, tr("Folder already exists"), tr("The instance folder already exists:\n%1").arg(targetPath));
            continue;
        }
        break;
    }

    m_status->setText(tr("Download complete. Extracting into %1…").arg(folderName));
    extractPack(folderName);
}

void TeamPackDialog::extractPack(const QString& folderName)
{
    const QString instancesPath = APPLICATION->instances()->primaryDir();
    if (instancesPath.isEmpty()) {
        showError(tr("The launcher instance folder is unavailable."));
        return;
    }

    QDir instancesDir(instancesPath);
    const QString stagingName = QStringLiteral(".team-install-%1").arg(QUuid::createUuid().toString(QUuid::WithoutBraces));
    if (!instancesDir.mkpath(stagingName)) {
        showError(tr("Could not create a temporary folder under the launcher instances directory."));
        return;
    }
    const QString stagingPath = instancesDir.filePath(stagingName);
    std::unique_ptr<Task> task = std::make_unique<MMCZip::ExtractZipTask>(m_archivePath, QDir(stagingPath), archiveSubdirectory(m_archivePath));
    bool cancelled = false;
    QString failure;
    connect(task.get(), &Task::aborted, this, [&cancelled] { cancelled = true; });
    connect(task.get(), &Task::failed, this, [&failure](const QString& reason) { failure = reason; });

    m_installTaskRunning = true;
    m_closeButton->setEnabled(false);
    m_closeButton->setText(tr("Extracting…"));
    m_status->setText(tr("Extracting all ZIP files into %1…").arg(folderName));
    ProgressDialog progress(this);
    progress.showSkipButton();
    progress.execWithTask(task);
    m_installTaskRunning = false;
    setBusy(false);

    if (failure.isEmpty() && !cancelled) {
        const QString stagedConfig = QDir(stagingPath).filePath(QStringLiteral("instance.cfg"));
        const QString stagedPack = QDir(stagingPath).filePath(QStringLiteral("mmc-pack.json"));
        if (!QFileInfo::exists(stagedConfig) && QFileInfo::exists(stagedPack)) {
            QFile config(stagedConfig);
            if (config.open(QIODevice::WriteOnly | QIODevice::Text)) {
                const QByteArray content = QStringLiteral("InstanceType=OneSix\nname=%1\n").arg(folderName).toUtf8();
                if (config.write(content) != content.size()) {
                    failure = tr("Could not write instance.cfg for the extracted Prism pack.");
                }
            } else {
                failure = tr("Could not create instance.cfg for the extracted Prism pack: %1").arg(config.errorString());
            }
        }

        if (failure.isEmpty() && !instancesDir.rename(stagingName, folderName)) {
            failure = tr("Could not move the extracted files into the new instance folder.");
        }
    }

    if (failure.isEmpty() && !cancelled) {
        const QString instanceCfg = QDir(instancesDir.filePath(folderName)).filePath(QStringLiteral("instance.cfg"));
        if (QFileInfo::exists(instanceCfg)) {
            APPLICATION->instances()->loadList();
            APPLICATION->instances()->setInstanceGroup(folderName, QStringLiteral("Team"));
            m_status->setText(tr("ZIP extracted to %1 and the instance was added to the launcher.").arg(folderName));
        } else {
            m_status->setText(tr("ZIP extracted to %1. To show and run it as a Prism instance, the ZIP must include mmc-pack.json (and preferably instance.cfg).")
                                  .arg(folderName));
        }
        m_progress->setRange(0, 100);
        m_progress->setValue(100);
    } else if (!failure.isEmpty()) {
        QDir(stagingPath).removeRecursively();
        showError(tr("Could not extract the team ZIP: %1").arg(failure));
    } else if (cancelled) {
        QDir(stagingPath).removeRecursively();
        showError(tr("ZIP extraction was cancelled. The partial files were removed."));
    } else {
        QDir(stagingPath).removeRecursively();
        showError(tr("ZIP extraction did not complete."));
    }
}
void TeamPackDialog::setBusy(bool busy)
{
    m_serverUrl->setEnabled(!busy);
    m_packSelector->setEnabled(!busy);
    m_refreshButton->setEnabled(!busy);
    m_installButton->setEnabled(!busy && !m_packs.isEmpty());
    m_closeButton->setEnabled(!m_installTaskRunning);
    m_closeButton->setText(busy ? tr("Cancel") : tr("Close"));
}

void TeamPackDialog::cancelNetworkRequest()
{
    if (!m_reply) {
        return;
    }
    auto* reply = m_reply.data();
    m_reply = nullptr;
    reply->abort();
    reply->deleteLater();
    if (m_downloadFile.isOpen()) {
        m_downloadFile.close();
    }
}

void TeamPackDialog::reject()
{
    if (m_reply) {
        cancelNetworkRequest();
        QDialog::reject();
        return;
    }
    if (m_installTaskRunning) {
        m_status->setText(tr("Please wait for the current operation to finish."));
        return;
    }
    QDialog::reject();
}

void TeamPackDialog::closeEvent(QCloseEvent* event)
{
    if (m_reply) {
        cancelNetworkRequest();
        QDialog::closeEvent(event);
        return;
    }
    if (m_installTaskRunning) {
        m_status->setText(tr("Please wait for the current operation to finish."));
        event->ignore();
        return;
    }
    QDialog::closeEvent(event);
}

void TeamPackDialog::showError(const QString& message)
{
    m_status->setText(message);
    if (m_progress) {
        m_progress->setRange(0, 100);
        m_progress->setValue(0);
    }
    setBusy(false);
}











