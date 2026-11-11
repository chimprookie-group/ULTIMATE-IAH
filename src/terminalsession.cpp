#include "terminalsession.h"

#include <QByteArrayList>
#include <QClipboard>
#include <QDir>
#include <QFileInfo>
#include <QGuiApplication>
#include <QProcessEnvironment>
#include <QSocketNotifier>
#include <QStandardPaths>
#include <QTimer>

#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <pty.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

TerminalSession::TerminalSession(QObject *parent)
    : QObject(parent) {}

TerminalSession::~TerminalSession() {
    stop();
}

QString TerminalSession::resolvedShell() const {
    QString candidate = qEnvironmentVariable("SHELL").trimmed();
    if (candidate.isEmpty())
        candidate = QStringLiteral("/bin/bash");
    if (!QFileInfo(candidate).isAbsolute()) {
        const QString resolved = QStandardPaths::findExecutable(candidate);
        if (!resolved.isEmpty())
            candidate = resolved;
    }
    if (!QFileInfo(candidate).isExecutable())
        candidate = QStringLiteral("/bin/bash");
    return candidate;
}

QString TerminalSession::shell() const {
    return resolvedShell();
}

void TerminalSession::start(const QString &cwd) {
    stop();

    QString directory = cwd;
    if (directory.isEmpty() || !QFileInfo(directory).isDir())
        directory = QDir::homePath();
    m_cwd = directory;
    emit cwdChanged();

    const QString shellPath = resolvedShell();
    const QByteArray shell = shellPath.toUtf8();
    const QByteArray base = QFileInfo(shellPath).fileName().toUtf8();
    const QByteArray dir = directory.toUtf8();

    // Build the environment before forking; the child then only calls
    // async-signal-safe functions (chdir and execve).
    QProcessEnvironment environment = QProcessEnvironment::systemEnvironment();
    environment.insert(QStringLiteral("TERM"), QStringLiteral("xterm-256color"));
    environment.insert(QStringLiteral("COLORTERM"), QStringLiteral("truecolor"));
    environment.insert(QStringLiteral("TERM_PROGRAM"), QStringLiteral("DeepCrasyH"));
    QByteArrayList entries;
    for (const QString &key : environment.keys())
        entries.append(key.toUtf8() + "=" + environment.value(key).toUtf8());
    std::vector<char *> envp;
    envp.reserve(size_t(entries.size()) + 1);
    for (QByteArray &entry : entries)
        envp.push_back(entry.data());
    envp.push_back(nullptr);

    struct winsize size {};
    size.ws_row = 24;
    size.ws_col = 80;

    m_pid = forkpty(&m_master, nullptr, nullptr, &size);
    if (m_pid == 0) {
        if (!dir.isEmpty())
            chdir(dir.constData());
        execle(shell.constData(), base.constData(), "-l", nullptr, envp.data());
        _exit(127);
    }

    if (m_pid < 0) {
        m_pid = -1;
        m_master = -1;
        emit runningChanged();
        emit exited(-1);
        return;
    }

    fcntl(m_master, F_SETFD, FD_CLOEXEC);
    fcntl(m_master, F_SETFL, fcntl(m_master, F_GETFL, 0) | O_NONBLOCK);
    m_decoder = QStringDecoder(QStringDecoder::Utf8);
    m_stopping = false;

    m_notifier = new QSocketNotifier(m_master, QSocketNotifier::Read, this);
    connect(m_notifier, &QSocketNotifier::activated, this, &TerminalSession::readOutput);

    emit runningChanged();
    emit started();

    if (!m_pendingInput.isEmpty()) {
        const QString pending = m_pendingInput;
        m_pendingInput.clear();
        QTimer::singleShot(80, this, [this, pending] { input(pending); });
    }
}

void TerminalSession::stop() {
    m_stopping = true;
    if (m_pid > 0) {
        kill(m_pid, SIGHUP);
        int status = 0;
        bool reaped = false;
        for (int attempt = 0; attempt < 20; ++attempt) {
            if (waitpid(m_pid, &status, WNOHANG) == m_pid) {
                reaped = true;
                break;
            }
            usleep(50000);
        }
        if (!reaped) {
            kill(m_pid, SIGKILL);
            waitpid(m_pid, &status, 0);
        }
        m_pid = -1;
        emit runningChanged();
    }
    closeMaster();
    m_stopping = false;
}

void TerminalSession::closeMaster() {
    if (m_notifier) {
        m_notifier->deleteLater();
        m_notifier = nullptr;
    }
    if (m_master >= 0) {
        ::close(m_master);
        m_master = -1;
    }
}

void TerminalSession::readOutput() {
    if (m_master < 0)
        return;

    char buffer[65536];
    for (;;) {
        const ssize_t count = ::read(m_master, buffer, sizeof(buffer));
        if (count > 0) {
            emit output(m_decoder.decode(QByteArray(buffer, int(count))));
            continue;
        }
        if (count < 0 && (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR))
            return;

        closeMaster();
        if (m_pid > 0) {
            int status = 0;
            waitpid(m_pid, &status, 0);
            const int code = WIFEXITED(status) ? WEXITSTATUS(status) : -1;
            m_pid = -1;
            if (!m_stopping)
                emit exited(code);
            emit runningChanged();
        }
        return;
    }
}

void TerminalSession::input(const QString &data) {
    if (m_master < 0 || data.isEmpty())
        return;
    const QByteArray bytes = data.toUtf8();
    qsizetype offset = 0;
    while (offset < bytes.size()) {
        const ssize_t written = ::write(m_master, bytes.constData() + offset, size_t(bytes.size() - offset));
        if (written > 0) {
            offset += written;
        } else if (written < 0 && errno == EINTR) {
            continue;
        } else {
            break;
        }
    }
}

void TerminalSession::run(const QString &command) {
    if (command.trimmed().isEmpty())
        return;
    const QString line = command.endsWith('\n') ? command : command + '\n';
    if (m_master >= 0)
        input(line);
    else
        m_pendingInput = line;
}

void TerminalSession::resize(int columns, int rows) {
    if (m_master < 0)
        return;
    struct winsize size {};
    size.ws_col = quint16(qBound(2, columns, 500));
    size.ws_row = quint16(qBound(1, rows, 200));
    ioctl(m_master, TIOCSWINSZ, &size);
}

void TerminalSession::copy(const QString &text) {
    QGuiApplication::clipboard()->setText(text);
}

void TerminalSession::paste() {
    input(QGuiApplication::clipboard()->text());
}
