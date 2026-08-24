#include "p2ptransfermanager.h"
#include "serverconfig.h"

#include <QDateTime>
#include <QDir>
#include <QDirIterator>
#include <QFileInfo>
#include <QFutureWatcher>
#include <QJsonArray>
#include <QJsonDocument>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QSaveFile>
#include <QSettings>
#include <QStandardPaths>
#include <QUuid>
#include <QtConcurrentRun>
#include <algorithm>
#include <stdexcept>

namespace {
constexpr int RequestTimeoutMs = 30000;
constexpr int ConnectTimeoutMs = 18000;
constexpr int StallTimeoutMs = 30000;
constexpr int MaxSignalAttempts = 3;
constexpr int SignalRetryBaseDelayMs = 250;
constexpr int MaxEntries = 10000;
constexpr qint64 ChunkSize = 64 * 1024;
constexpr size_t MaxBuffered = 2 * 1024 * 1024;
QByteArray json(const QJsonObject &o) { return QJsonDocument(o).toJson(QJsonDocument::Compact); }
}

P2pTransferManager::P2pTransferManager(QObject *parent) : QObject(parent)
{
    rtc::InitLogger(rtc::LogLevel::Warning);
    setBaseUrl(MyFolderServerConfig::baseUrl());
    setReceiveRoot(QSettings().value("transfer/receiveRoot",
        QDir(QStandardPaths::writableLocation(QStandardPaths::DownloadLocation)).filePath("MyFolder")).toString());
    loadOutgoingSessions();
    m_watchdog.setInterval(1000);
    connect(&m_watchdog, &QTimer::timeout, this, [this] {
        checkTimeouts(QDateTime::currentMSecsSinceEpoch());
    });
    m_watchdog.start();
}

P2pTransferManager::~P2pTransferManager()
{
    m_watchdog.stop();
    const auto sessions = m_sessions.values();
    for (const auto &s : sessions) {
        if (s->channel) { s->channel->resetCallbacks(); s->channel->close(); }
        if (s->peer) { s->peer->resetCallbacks(); s->peer->close(); }
    }
    m_sessions.clear();
}

void P2pTransferManager::setBaseUrl(const QString &v) { QString n=v.trimmed(); while(n.endsWith('/'))n.chop(1); if(n!=m_baseUrl){m_baseUrl=n;emit baseUrlChanged();} }
void P2pTransferManager::setAuthToken(const QString &v) { if(v!=m_authToken){m_authToken=v;emit credentialsChanged();} }
void P2pTransferManager::setCurrentDeviceId(const QString &v) { if(v!=m_currentDeviceId){m_currentDeviceId=v;emit credentialsChanged();} }
void P2pTransferManager::setCurrentDeviceToken(const QString &v) { if(v!=m_currentDeviceToken){m_currentDeviceToken=v;emit credentialsChanged();} }
void P2pTransferManager::setReceiveRoot(const QString &v) { QString n=QDir::cleanPath(v); if(n!=m_receiveRoot){m_receiveRoot=n;emit receiveRootChanged();} }
bool P2pTransferManager::isDirectory(const QString &path) const { return QFileInfo(path).isDir(); }

void P2pTransferManager::sendPath(const QString &targetDeviceId, const QString &destinationPath,
                                  const QString &localPath)
{
    if (m_authToken.isEmpty() || m_currentDeviceId.isEmpty() || m_currentDeviceToken.isEmpty()
        || targetDeviceId.isEmpty() || targetDeviceId == m_currentDeviceId
        || !safeRelativePath(destinationPath) || !QFileInfo::exists(localPath)) {
        const QString reason = tr("P2P transfer parameters are invalid");
        emit createFailed(reason);
        emit outgoingFallbackRequested(QString(), targetDeviceId, destinationPath, localPath,
                                       QFileInfo(localPath).isDir(), reason);
        return;
    }
    auto *watcher = new QFutureWatcher<Manifest>(this);
    connect(watcher, &QFutureWatcher<Manifest>::finished, this, [=] {
        Manifest manifest=watcher->result(); watcher->deleteLater();
        if(!manifest.error.isEmpty()){
            emit createFailed(manifest.error);
            emit outgoingFallbackRequested(QString(), targetDeviceId, destinationPath,
                                           manifest.rootPath, manifest.directory, manifest.error);
            return;
        }
        createTask(targetDeviceId,destinationPath,manifest);
    });
    watcher->setFuture(QtConcurrent::run(&P2pTransferManager::scanPath, localPath));
}

void P2pTransferManager::createTask(const QString &targetDeviceId, const QString &destinationPath,
                                    const Manifest &manifest)
{
    QJsonArray files, dirs; qint64 total=0;
    for(const auto &f:manifest.files){files.append(QJsonObject{{"path",f.path},{"size",double(f.size)},{"sha256",f.sha256}});total+=f.size;}
    for(const auto &d:manifest.directories)dirs.append(d);
    QNetworkRequest req(QUrl(m_baseUrl+"/api/v1/forwards")); applyHeaders(req);
    req.setHeader(QNetworkRequest::ContentTypeHeader,"application/json; charset=utf-8");
    QJsonObject body{{"sourceDeviceId",m_currentDeviceId},{"targetDeviceId",targetDeviceId},
        {"destinationPath",destinationPath},{"deleteSource",false},{"channel","P2P"},
        {"files",files},{"directories",dirs}};
    auto *reply=m_network.post(req,json(body));
    connect(reply,&QNetworkReply::finished,this,[=]{
        int status=reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        QJsonObject obj=QJsonDocument::fromJson(reply->readAll()).object();
        QString net=reply->errorString();
        const int networkErrorCode = int(reply->error());
        if (status == 0) m_network.clearConnectionCache();
        reply->deleteLater();
        if(status!=201){
            QString message=obj.value("message").toString(net);
            if (message.isEmpty()) message = tr("No response from server");
            if (status == 0) message += tr(" (network error %1)").arg(networkErrorCode);
            setLastError(message);
            emit createFailed(message);
            emit outgoingFallbackRequested(QString(),targetDeviceId,destinationPath,
                                           manifest.rootPath,manifest.directory,message);
            return;
        }
        auto s=std::make_shared<Session>(); s->forwardId=obj.value("forwardId").toString();
        s->sourceDeviceId=m_currentDeviceId;s->targetDeviceId=targetDeviceId;s->destinationPath=destinationPath;
        s->token=obj.value("directTransferToken").toString();s->localRootPath=manifest.rootPath;
        s->files=manifest.files;s->directories=manifest.directories;s->directory=manifest.directory;s->totalBytes=total;s->source=true;
        if(s->forwardId.isEmpty()||s->token.isEmpty()){
            const QString reason = tr("Server returned no P2P capability");
            emit createFailed(reason);
            emit outgoingFallbackRequested(QString(), targetDeviceId, destinationPath,
                                           manifest.rootPath, manifest.directory, reason);
            return;
        }
        m_sessions.insert(s->forwardId,s); saveOutgoingSessions(); emit forwardCreated(s->forwardId);
    });
}

void P2pTransferManager::prepareIncoming(const QVariantMap &task)
{
    if(task.value("channel").toString()!="P2P"||task.value("targetDeviceId").toString()!=m_currentDeviceId)return;
    QString id=task.value("forwardId").toString(); if(id.isEmpty())return;
    auto s=m_sessions.value(id); if(!s){s=std::make_shared<Session>();m_sessions.insert(id,s);}
    if(s->incomingPrepared)return;
    s->forwardId=id;s->source=false;s->sourceDeviceId=task.value("sourceDeviceId").toString();
    s->targetDeviceId=m_currentDeviceId;s->destinationPath=task.value("destinationPath").toString();
    s->token=task.value("directTransferToken").toString();s->totalBytes=task.value("totalBytes").toLongLong();
    s->directories.clear(); for(const auto &v:task.value("directories").toList())s->directories.append(v.toString());
    s->files.clear(); for(const auto &v:task.value("files").toList()){auto m=v.toMap();s->files.append({m.value("path").toString(),{},m.value("sha256").toString(),m.value("size").toLongLong()});}
    if(s->token.isEmpty()||!safeRelativePath(s->destinationPath)){fail(id,"P2P_MANIFEST_INVALID",tr("Invalid P2P manifest"),false);return;}
    QDir base(QDir(m_receiveRoot).filePath(s->destinationPath)); if(!base.mkpath(".")){fail(id,"P2P_STORAGE_ERROR",tr("Cannot create receive directory"),false);return;}
    if(!s->directories.isEmpty()&&!s->directories.first().contains('/')){
        s->manifestRoot=s->directories.first();
        s->resolvedRoot=QFileInfo(availableTargetPath(base.filePath(s->manifestRoot))).fileName();
    }
    for(const auto &d:s->directories){
        if(!safeRelativePath(d)){fail(id,"P2P_MANIFEST_INVALID",tr("Cannot create manifest directory"),false);return;}
        const QString output=(!s->manifestRoot.isEmpty()&&(d==s->manifestRoot||d.startsWith(s->manifestRoot+"/")))
                ? s->resolvedRoot+d.mid(s->manifestRoot.size()) : d;
        if(!base.mkpath(output)){fail(id,"P2P_STORAGE_ERROR",tr("Cannot create manifest directory"),false);return;}
    }
    s->incomingPrepared=true;
}

void P2pTransferManager::handleTaskUpdated(const QVariantMap &task)
{
    if(task.value("channel").toString()!="P2P")return; QString id=task.value("forwardId").toString();
    const QString state=task.value("state").toString();
    if(task.value("targetDeviceId").toString()==m_currentDeviceId && (state=="ACCEPTED"||state=="TRANSFERRING")){prepareIncoming(task);ensurePeer(id,false);}
    if(task.value("sourceDeviceId").toString()==m_currentDeviceId && (state=="ACCEPTED"||state=="TRANSFERRING"))ensurePeer(id,true);
    if(state=="FAILED") {
        auto s=m_sessions.value(id);
        QString reason=task.value("failureReason").toString();
        if(reason.isEmpty())reason=tr("Remote P2P transfer failed");
        if(s&&s->source)fail(id,"P2P_REMOTE_FAILED",reason,true);
        else closeSession(id);
    } else if(state=="CANCELLED"||state=="COMPLETED"||state=="REJECTED")closeSession(id);
}

void P2pTransferManager::reconcileTasks(const QStringList &activeTaskIds)
{
    const QSet<QString> active(activeTaskIds.cbegin(), activeTaskIds.cend());
    const auto sessions = m_sessions.values();
    for (const auto &session : sessions) {
        if (session && !active.contains(session->forwardId))
            closeSession(session->forwardId);
    }
}

void P2pTransferManager::ensurePeer(const QString &id, bool offerer)
{
    auto s=m_sessions.value(id); if(!s||s->peer)return;
    try {
        rtc::Configuration c;
        c.enableIceTcp = true;
        if (m_testSignaling) {
            c.bindAddress = "127.0.0.1";
        } else {
            c.iceServers.emplace_back("stun:stun.cloudflare.com:3478");
            c.iceServers.emplace_back("stun:stun.l.google.com:19302");
        }
        s->peer=std::make_shared<rtc::PeerConnection>(c);
        s->startedMs=QDateTime::currentMSecsSinceEpoch();
        s->lastActivityMs=s->startedMs;
        std::weak_ptr<Session> weak=s;
        s->peer->onLocalDescription([this,id](rtc::Description d){QMetaObject::invokeMethod(this,[=]{sendSignal(id,{{"kind","description"},{"type",QString::fromStdString(d.typeString())},{"sdp",QString::fromStdString(std::string(d))}});});});
        s->peer->onLocalCandidate([this,id](rtc::Candidate cnd){QMetaObject::invokeMethod(this,[=]{sendSignal(id,{{"kind","candidate"},{"candidate",QString::fromStdString(std::string(cnd))},{"mid",QString::fromStdString(cnd.mid())}});});});
        s->peer->onStateChange([this,id](rtc::PeerConnection::State state){if(state==rtc::PeerConnection::State::Failed)QMetaObject::invokeMethod(this,[=]{auto x=m_sessions.value(id);fail(id,"P2P_CONNECTION_FAILED",tr("P2P connection failed"),x&&x->source);});});
        s->peer->onDataChannel([this,id](std::shared_ptr<rtc::DataChannel> dc){QMetaObject::invokeMethod(this,[=]{attachChannel(id,dc);});});
        if(offerer)attachChannel(id,s->peer->createDataChannel("myfolder-v1"));
    } catch(const std::exception &e){fail(id,"P2P_INIT_FAILED",QString::fromUtf8(e.what()),offerer);}
}

void P2pTransferManager::attachChannel(const QString &id,const std::shared_ptr<rtc::DataChannel> &dc)
{
    auto s=m_sessions.value(id);if(!s)return;s->channel=dc;dc->setBufferedAmountLowThreshold(MaxBuffered/2);
    dc->onOpen([this,id]{QMetaObject::invokeMethod(this,[=]{auto x=m_sessions.value(id);if(!x)return;x->lastActivityMs=QDateTime::currentMSecsSinceEpoch();if(x->source){x->channel->send(json({{"type","hello"},{"forwardId",id},{"token",x->token}}).toStdString());startNextFile(id);}});});
    dc->onBufferedAmountLow([this,id]{QMetaObject::invokeMethod(this,[=]{auto x=m_sessions.value(id);if(!x)return;x->lastActivityMs=QDateTime::currentMSecsSinceEpoch();pump(id);});});
    dc->onMessage([this,id](rtc::message_variant data){
        if(std::holds_alternative<std::string>(data)){QString value=QString::fromStdString(std::get<std::string>(data));QMetaObject::invokeMethod(this,[=]{processText(id,value);});}
        else {auto b=std::get<rtc::binary>(data);QByteArray value(reinterpret_cast<const char*>(b.data()),qsizetype(b.size()));QMetaObject::invokeMethod(this,[=]{processBinary(id,value);});}
    });
    dc->onError([this,id](std::string e){QString value=QString::fromStdString(e);QMetaObject::invokeMethod(this,[=]{auto x=m_sessions.value(id);fail(id,"P2P_CHANNEL_ERROR",value,x&&x->source);});});
    dc->onClosed([this,id]{QMetaObject::invokeMethod(this,[=]{auto x=m_sessions.value(id);if(x&&!x->completed)fail(id,"P2P_CHANNEL_CLOSED",tr("P2P channel closed before completion"),x->source);});});
}

void P2pTransferManager::handleSignal(const QJsonObject &payload)
{
    QString id=payload.value("forwardId").toString();if(id.isEmpty()||payload.value("fromDeviceId").toString()==m_currentDeviceId)return;
    auto s=m_sessions.value(id);if(!s)return;ensurePeer(id,s->source);auto signal=payload.value("signal").toObject();
    try {
        const QString kind=signal.value("kind").toString();
        if(kind=="description") {
            s->peer->setRemoteDescription(rtc::Description(signal.value("sdp").toString().toStdString()));
            s->remoteDescriptionSet=true;
            QTimer::singleShot(0,this,[this,id]{flushRemoteCandidates(id);});
        } else if(kind=="candidate") {
            const QString candidate=signal.value("candidate").toString();
            const QString mid=signal.value("mid").toString();
            if(candidate.isEmpty())throw std::invalid_argument("Empty remote candidate");
            if(s->pendingRemoteCandidates.size()>=256)throw std::invalid_argument("Too many queued remote candidates");
            s->pendingRemoteCandidates.append({candidate,mid});
            if(s->remoteDescriptionSet)
                QTimer::singleShot(0,this,[this,id]{flushRemoteCandidates(id);});
        }
    }catch(const std::exception &e){fail(id,"P2P_SIGNAL_INVALID",QString::fromUtf8(e.what()),s->source);}
}

void P2pTransferManager::flushRemoteCandidates(const QString &id,int attempt)
{
    auto s=m_sessions.value(id);
    if(!s||!s->peer||!s->remoteDescriptionSet||s->pendingRemoteCandidates.isEmpty())return;
    try {
        while(!s->pendingRemoteCandidates.isEmpty()) {
            const auto candidate=s->pendingRemoteCandidates.constFirst();
            s->peer->addRemoteCandidate(rtc::Candidate(candidate.first.toStdString(),candidate.second.toStdString()));
            s->pendingRemoteCandidates.removeFirst();
        }
    } catch(const std::exception &e) {
        const QString message=QString::fromUtf8(e.what());
        if(message.contains("remote description",Qt::CaseInsensitive)&&attempt<20) {
            QTimer::singleShot(100,this,[this,id,attempt]{flushRemoteCandidates(id,attempt+1);});
            return;
        }
        fail(id,"P2P_SIGNAL_INVALID",message,s->source);
    }
}

void P2pTransferManager::sendSignal(const QString &id,const QJsonObject &signal,int attempt)
{
    if (attempt == 0) emit localSignalReady(id, signal);
    if (m_testSignaling) return;
    QNetworkRequest req(QUrl(m_baseUrl+"/api/v1/forwards/"+QString::fromUtf8(QUrl::toPercentEncoding(id))+"/signal"));applyHeaders(req);req.setHeader(QNetworkRequest::ContentTypeHeader,"application/json");
    auto *reply=m_network.post(req,json(signal));
    connect(reply,&QNetworkReply::finished,this,[this,reply,id,signal,attempt]{
        const int status=reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        const QNetworkReply::NetworkError networkError=reply->error();
        const QJsonObject response=QJsonDocument::fromJson(reply->readAll()).object();
        QString err=response.value("message").toString(reply->errorString());
        reply->deleteLater();
        if(status==202)return;

        const bool temporary=status==0||status==408||status==425||status==429||status>=500;
        if(status==0)m_network.clearConnectionCache();
        if(temporary&&attempt+1<MaxSignalAttempts&&m_sessions.contains(id)){
            const int delay=SignalRetryBaseDelayMs*(1<<attempt);
            QTimer::singleShot(delay,this,[this,id,signal,attempt]{
                if(m_sessions.contains(id))sendSignal(id,signal,attempt+1);
            });
            return;
        }
        if(err.isEmpty())err=tr("P2P signaling request failed");
        if(status==0)err+=tr(" (network error %1)").arg(int(networkError));
        auto s=m_sessions.value(id);
        fail(id,"P2P_SIGNAL_FAILED",err,s&&s->source);
    });
}

void P2pTransferManager::startNextFile(const QString &id)
{
    auto s=m_sessions.value(id);if(!s||!s->source||!s->channel||!s->channel->isOpen()||s->awaitingAck)return;
    if(s->fileIndex>=s->files.size()){if(!s->doneSent){s->channel->send(json({{"type","done"}}).toStdString());s->doneSent=true;}return;}
    const auto &f=s->files[s->fileIndex];s->file=std::make_unique<QFile>(f.localPath);if(!s->file->open(QIODevice::ReadOnly)){fail(id,"P2P_SOURCE_CHANGED",tr("Source file cannot be opened"),true);return;}
    s->channel->send(json({{"type","file"},{"path",f.path},{"size",double(f.size)},{"sha256",f.sha256}}).toStdString());pump(id);
}

void P2pTransferManager::pump(const QString &id)
{
    auto s=m_sessions.value(id);if(!s||!s->source||!s->file||!s->channel||!s->channel->isOpen()||s->awaitingAck)return;
    while(s->channel->bufferedAmount()<MaxBuffered&&!s->file->atEnd()){QByteArray data=s->file->read(ChunkSize);if(data.isEmpty()&&!s->file->atEnd()){fail(id,"P2P_READ_FAILED",tr("Cannot read source file"),true);return;}rtc::binary b(reinterpret_cast<const std::byte*>(data.constData()),reinterpret_cast<const std::byte*>(data.constData()+data.size()));if(!s->channel->send(std::move(b)))break;s->transferred+=data.size();s->lastActivityMs=QDateTime::currentMSecsSinceEpoch();}
    if(s->file->atEnd()){s->file->close();s->file.reset();s->awaitingAck=true;s->lastActivityMs=QDateTime::currentMSecsSinceEpoch();s->channel->send(json({{"type","eof"},{"path",s->files[s->fileIndex].path}}).toStdString());}
}

void P2pTransferManager::processText(const QString &id,const QString &message)
{
    auto s=m_sessions.value(id);if(!s)return;s->lastActivityMs=QDateTime::currentMSecsSinceEpoch();QJsonObject o=QJsonDocument::fromJson(message.toUtf8()).object();QString type=o.value("type").toString();
    if(type=="hello"&&!s->source){if(o.value("forwardId").toString()!=id||o.value("token").toString()!=s->token){fail(id,"P2P_UNAUTHORIZED",tr("P2P capability token rejected"),false);return;}s->helloAccepted=true;return;}
    if(type=="file"&&!s->source){if(!s->helloAccepted){fail(id,"P2P_UNAUTHORIZED",tr("Missing P2P handshake"),false);return;}QString path=o.value("path").toString();auto it=std::find_if(s->files.begin(),s->files.end(),[&](const FileEntry &f){return f.path==path;});if(it==s->files.end()||!safeRelativePath(path)){fail(id,"P2P_MANIFEST_INVALID",tr("File is not in manifest"),false);return;}s->currentPath=path;s->currentReceived=0;QString output=(!s->manifestRoot.isEmpty()&&(path==s->manifestRoot||path.startsWith(s->manifestRoot+"/")))?s->resolvedRoot+path.mid(s->manifestRoot.size()):path;QString requested=QDir(QDir(m_receiveRoot).filePath(s->destinationPath)).filePath(output);s->currentFinalPath=s->manifestRoot.isEmpty()?availableTargetPath(requested):requested;s->currentPartPath=s->currentFinalPath+".myfolder-p2p-part";QDir().mkpath(QFileInfo(s->currentPartPath).absolutePath());s->file=std::make_unique<QFile>(s->currentPartPath);if(!s->file->open(QIODevice::WriteOnly|QIODevice::Truncate)){fail(id,"P2P_STORAGE_ERROR",tr("Cannot create target file"),false);return;}s->hash=std::make_unique<QCryptographicHash>(QCryptographicHash::Sha256);return;}
    if(type=="eof"&&!s->source){auto it=std::find_if(s->files.begin(),s->files.end(),[&](const FileEntry &f){return f.path==s->currentPath;});if(!s->file||it==s->files.end()){fail(id,"P2P_PROTOCOL_ERROR",tr("Unexpected file end"),false);return;}s->file->flush();s->file->close();s->file.reset();QString actual=QString::fromLatin1(s->hash->result().toHex());s->hash.reset();if(s->currentReceived!=it->size||actual!=it->sha256){QFile::remove(s->currentPartPath);fail(id,"P2P_HASH_MISMATCH",tr("P2P file SHA-256 verification failed"),false);return;}if(!QFile::rename(s->currentPartPath,s->currentFinalPath)){fail(id,"P2P_STORAGE_ERROR",tr("Cannot finalize received file"),false);return;}s->transferred+=it->size;publishIncomingProgress(id,s->transferred);emit incomingProgress(id,s->transferred);s->channel->send(json({{"type","ack"},{"path",s->currentPath}}).toStdString());s->currentPath.clear();return;}
    if(type=="ack"&&s->source){if(!s->awaitingAck||o.value("path").toString()!=s->files[s->fileIndex].path){fail(id,"P2P_PROTOCOL_ERROR",tr("Unexpected P2P acknowledgement"),true);return;}s->awaitingAck=false;s->confirmedBytes+=s->files[s->fileIndex].size;publishOutgoingProgress(id,s->confirmedBytes);++s->fileIndex;startNextFile(id);return;}
    if(type=="done"&&!s->source){
        s->completed=true;
        s->channel->send(json({{"type","complete"}}).toStdString());
        emit incomingCompleted(id,s->totalBytes);
        // Give the final acknowledgement a brief chance to leave the data channel,
        // then release ICE/DataChannel worker threads instead of leaking them forever.
        QTimer::singleShot(250,this,[this,id]{closeSession(id);});
        return;
    }
    if(type=="complete"&&s->source){
        s->completed=true;
        QTimer::singleShot(0,this,[this,id]{closeSession(id);});
        return;
    }
}

void P2pTransferManager::processBinary(const QString &id,const QByteArray &data)
{
    auto s=m_sessions.value(id);if(!s||s->source||!s->file||!s->hash){fail(id,"P2P_PROTOCOL_ERROR",tr("Unexpected P2P binary data"),false);return;}
    s->lastActivityMs=QDateTime::currentMSecsSinceEpoch();
    auto it=std::find_if(s->files.begin(),s->files.end(),[&](const FileEntry &f){return f.path==s->currentPath;});if(it==s->files.end()||s->currentReceived+data.size()>it->size||s->file->write(data)!=data.size()){fail(id,"P2P_STORAGE_ERROR",tr("Cannot write received file"),false);return;}s->hash->addData(data);s->currentReceived+=data.size();
}

void P2pTransferManager::fail(const QString &id,const QString &code,const QString &message,bool fallback)
{
    auto s=m_sessions.value(id);if(!s)return;setLastError(message);QString target=s->targetDeviceId,dest=s->destinationPath,local=s->localRootPath;bool dir=s->directory;closeSession(id);if(fallback)emit outgoingFallbackRequested(id,target,dest,local,dir,message);else emit incomingFailed(id,code,message);
}
void P2pTransferManager::closeSession(const QString &id){auto s=m_sessions.take(id);if(!s)return;if(s->file){s->file->close();s->file.reset();}if(!s->source&&!s->completed&&!s->currentPartPath.isEmpty())QFile::remove(s->currentPartPath);if(s->channel){s->channel->resetCallbacks();s->channel->close();}if(s->peer){s->peer->resetCallbacks();s->peer->close();}if(m_outgoingProgress.remove(id)>0)emit outgoingProgressChanged();if(m_incomingVerifiedProgress.remove(id)>0)emit incomingVerifiedProgressChanged();saveOutgoingSessions();}

void P2pTransferManager::checkTimeouts(qint64 now)
{
    const auto sessions=m_sessions.values();
    for(const auto &s:sessions){
        if(!s||s->completed)continue;
        const bool channelOpen=s->channel&&s->channel->isOpen();
        if(!channelOpen&&!s->awaitingAck&&s->startedMs>0&&now-s->startedMs>ConnectTimeoutMs){
            fail(s->forwardId,"P2P_TIMEOUT",tr("P2P negotiation timed out"),s->source);
            continue;
        }
        if((channelOpen||s->awaitingAck)&&s->lastActivityMs>0&&now-s->lastActivityMs>StallTimeoutMs){
            const bool waitingForAck=s->source&&s->awaitingAck;
            fail(s->forwardId,waitingForAck?"P2P_ACK_TIMEOUT":"P2P_STALLED",
                 waitingForAck?tr("P2P acknowledgement timed out"):tr("P2P transfer stalled"),s->source);
        }
    }
}

void P2pTransferManager::publishOutgoingProgress(const QString &id,qint64 transferredBytes)
{
    if(m_outgoingProgress.value(id).toLongLong()==transferredBytes&&m_outgoingProgress.contains(id))return;
    m_outgoingProgress.insert(id,transferredBytes);
    emit outgoingProgressReady(id,transferredBytes);
    emit outgoingProgressChanged();
}

void P2pTransferManager::publishIncomingProgress(const QString &id,qint64 verifiedBytes)
{
    if(m_incomingVerifiedProgress.value(id).toLongLong()==verifiedBytes&&m_incomingVerifiedProgress.contains(id))return;
    m_incomingVerifiedProgress.insert(id,verifiedBytes);
    emit incomingVerifiedProgressChanged();
}

QString P2pTransferManager::persistencePath() const
{
    const QString root=QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(root);return QDir(root).filePath("p2p-outgoing-v1.json");
}

void P2pTransferManager::saveOutgoingSessions() const
{
    QJsonArray sessions;
    for(const auto &s:m_sessions){
        if(!s->source||s->completed)continue;QJsonArray files,dirs;
        for(const auto &f:s->files)files.append(QJsonObject{{"path",f.path},{"localPath",f.localPath},{"sha256",f.sha256},{"size",double(f.size)}});
        for(const auto &d:s->directories)dirs.append(d);
        sessions.append(QJsonObject{{"forwardId",s->forwardId},{"sourceDeviceId",s->sourceDeviceId},
            {"targetDeviceId",s->targetDeviceId},{"destinationPath",s->destinationPath},{"token",s->token},
            {"localRootPath",s->localRootPath},{"directory",s->directory},{"totalBytes",double(s->totalBytes)},
            {"files",files},{"directories",dirs}});
    }
    QSaveFile file(persistencePath());if(file.open(QIODevice::WriteOnly)){file.write(QJsonDocument(sessions).toJson(QJsonDocument::Compact));file.commit();}
}

void P2pTransferManager::loadOutgoingSessions()
{
    QFile file(persistencePath());if(!file.open(QIODevice::ReadOnly))return;
    const auto items=QJsonDocument::fromJson(file.readAll()).array();
    for(const auto &value:items){auto o=value.toObject();auto s=std::make_shared<Session>();
        s->forwardId=o.value("forwardId").toString();s->sourceDeviceId=o.value("sourceDeviceId").toString();
        s->targetDeviceId=o.value("targetDeviceId").toString();s->destinationPath=o.value("destinationPath").toString();
        s->token=o.value("token").toString();s->localRootPath=o.value("localRootPath").toString();
        s->directory=o.value("directory").toBool();s->totalBytes=qint64(o.value("totalBytes").toDouble());s->source=true;
        for(const auto &v:o.value("files").toArray()){auto f=v.toObject();s->files.append({f.value("path").toString(),f.value("localPath").toString(),f.value("sha256").toString(),qint64(f.value("size").toDouble())});}
        for(const auto &v:o.value("directories").toArray())s->directories.append(v.toString());
        if(!s->forwardId.isEmpty()&&!s->token.isEmpty()&&!s->targetDeviceId.isEmpty()&&safeRelativePath(s->destinationPath))m_sessions.insert(s->forwardId,s);
    }
}
void P2pTransferManager::setLastError(const QString &v){if(v!=m_lastError){m_lastError=v;emit lastErrorChanged();}}
void P2pTransferManager::applyHeaders(QNetworkRequest &r)const{r.setTransferTimeout(RequestTimeoutMs);r.setRawHeader("Authorization",m_authToken.toUtf8());r.setRawHeader("X-Device-Id",m_currentDeviceId.toUtf8());r.setRawHeader("X-Device-Token",m_currentDeviceToken.toUtf8());}

QString P2pTransferManager::sha256File(const QString &path){QFile f(path);if(!f.open(QIODevice::ReadOnly))return{};QCryptographicHash h(QCryptographicHash::Sha256);return h.addData(&f)?QString::fromLatin1(h.result().toHex()):QString();}
P2pTransferManager::Manifest P2pTransferManager::scanPath(const QString &path){Manifest m;m.rootPath=QDir::cleanPath(path);QFileInfo root(m.rootPath);if(!root.exists()||root.isSymLink()){m.error=tr("Selected path no longer exists");return m;}if(root.isFile()){FileEntry f{root.fileName(),root.absoluteFilePath(),sha256File(root.absoluteFilePath()),root.size()};if(f.sha256.isEmpty()||!safeRelativePath(f.path))m.error=tr("Cannot hash selected file");else m.files.append(f);return m;}if(!root.isDir()||!safeRelativePath(root.fileName())){m.error=tr("Cannot read selected folder");return m;}m.directory=true;m.directories.append(root.fileName());QDirIterator it(root.absoluteFilePath(),QDir::AllEntries|QDir::NoDotAndDotDot|QDir::NoSymLinks,QDirIterator::Subdirectories);while(it.hasNext()){it.next();QFileInfo info=it.fileInfo();QString rel=QDir::fromNativeSeparators(QDir(root.absoluteFilePath()).relativeFilePath(info.absoluteFilePath()));QString p=root.fileName()+"/"+rel;if(!safeRelativePath(p)){m.error=tr("Unsafe folder path");return m;}if(info.isDir())m.directories.append(p);else if(info.isFile()&&info.isReadable())m.files.append({p,info.absoluteFilePath(),sha256File(info.absoluteFilePath()),info.size()});else{m.error=tr("Unreadable folder entry");return m;}if(m.files.size()>MaxEntries||m.directories.size()>MaxEntries){m.error=tr("Folder contains too many entries");return m;}}for(const auto &f:m.files)if(f.sha256.isEmpty()){m.error=tr("Cannot hash a folder file");break;}return m;}
bool P2pTransferManager::safeRelativePath(const QString &p){if(p.isEmpty()||p.startsWith('/')||p.endsWith('/')||p.contains('\\')||p.contains("//")||(p.size()>1&&p[0].isLetter()&&p[1]==':'))return false;for(const auto &x:p.split('/'))if(x.isEmpty()||x=="."||x=="..")return false;return true;}
QString P2pTransferManager::availableTargetPath(const QString &path){if(!QFile::exists(path)&&!QFile::exists(path+".myfolder-p2p-part"))return path;QFileInfo i(path);QString suffix=i.completeSuffix(),stem=i.fileName();if(!suffix.isEmpty())stem.chop(suffix.size()+1);for(int n=1;n<10000;++n){QString name=suffix.isEmpty()?QString("%1 (%2)").arg(stem).arg(n):QString("%1 (%2).%3").arg(stem).arg(n).arg(suffix);QString c=QDir(i.absolutePath()).filePath(name);if(!QFile::exists(c)&&!QFile::exists(c+".myfolder-p2p-part"))return c;}return QDir(i.absolutePath()).filePath(QUuid::createUuid().toString(QUuid::WithoutBraces));}
