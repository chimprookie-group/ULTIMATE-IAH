#include "workspacemodel.h"

#include <QClipboard>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QGuiApplication>
#include <QProcess>
#include <QSet>
#include <QStandardPaths>
#include <QStringList>

namespace {

constexpr int kMaxFiles = 1000;
constexpr qint64 kMaxPreview = 256 * 1024;

QString statusCode(const QString &raw) {
    if (raw == "??")
        return QStringLiteral("untracked");
    if (raw.contains('M'))
        return QStringLiteral("modified");
    if (raw.contains('A'))
        return QStringLiteral("added");
    if (raw.contains('D'))
        return QStringLiteral("deleted");
    if (raw.contains('R'))
        return QStringLiteral("renamed");
    return QStringLiteral("changed");
}

QString truncate(const QString &text, int limit) {
    return text.size() > limit ? text.left(limit) + QStringLiteral("\n… (truncated)") : text;
}

} // namespace

WorkspaceModel::WorkspaceModel(QObject *parent)
    : QObject(parent) {}

QString WorkspaceModel::resolve(const QString &path) const {
    if (m_workspace.isEmpty() || path.isEmpty())
        return {};
    const QFileInfo rootInfo(m_workspace);
    const QString root = rootInfo.canonicalFilePath();
    const QFileInfo info(QDir(m_workspace).absoluteFilePath(path));
    const QString canonical = info.canonicalFilePath();
    if (root.isEmpty() || canonical.isEmpty())
        return {};
    if (canonical != root && !canonical.startsWith(root + "/"))
        return {};
    return canonical;
}

QVariantMap WorkspaceModel::entry(const QString &relative, const QHash<QString, QString> &statuses) const {
    const QFileInfo info(QDir(m_workspace).absoluteFilePath(relative));
    const QString dir = QFileInfo(relative).path();
    QVariantMap file;
    file["path"] = relative;
    file["name"] = info.fileName();
    file["dir"] = dir == "." ? QString() : dir;
    file["size"] = info.size();
    file["status"] = statuses.value(relative);
    file["extension"] = info.suffix().toLower();
    return file;
}

void WorkspaceModel::refresh(const QString &workspace) {
    m_workspace = workspace.isEmpty() ? m_workspace : QDir::cleanPath(workspace);
    emit workspaceChanged();

    QHash<QString, QString> statuses;
    QStringList paths;
    const bool git = QFileInfo::exists(m_workspace + "/.git");
    const QString gitBinary = QStandardPaths::findExecutable(QStringLiteral("git"));

    if (git && !gitBinary.isEmpty()) {
        QProcess status;
        status.setWorkingDirectory(m_workspace);
        status.start(gitBinary, {QStringLiteral("status"), QStringLiteral("--porcelain=v1")});
        if (status.waitForFinished(3000)) {
            const QString output = QString::fromUtf8(status.readAllStandardOutput());
            for (const QString &line : output.split('\n', Qt::SkipEmptyParts)) {
                if (line.size() < 4)
                    continue;
                QString path = line.mid(3).trimmed();
                if (path.contains(" -> "))
                    path = path.section(" -> ", 1);
                if (path.startsWith('"') && path.endsWith('"'))
                    path = path.mid(1, path.size() - 2);
                statuses.insert(path, statusCode(line.left(2)));
            }
        }

        QProcess files;
        files.setWorkingDirectory(m_workspace);
        files.start(gitBinary, {QStringLiteral("ls-files"), QStringLiteral("-z"),
                                QStringLiteral("--cached"), QStringLiteral("--others"),
                                QStringLiteral("--exclude-standard")});
        if (files.waitForFinished(5000)) {
            const QByteArray output = files.readAllStandardOutput();
            for (const QByteArray &raw : output.split('\0')) {
                if (raw.isEmpty())
                    continue;
                paths << QString::fromUtf8(raw);
                if (paths.size() >= kMaxFiles)
                    break;
            }
        }
    }

    if (paths.isEmpty()) {
        QDirIterator iterator(m_workspace, QDir::Files | QDir::NoDotAndDotDot,
                              QDirIterator::Subdirectories);
        static const QSet<QString> skipped = {
            "node_modules", "build", "dist", "target", ".git", ".venv", "__pycache__",
            ".cache", "vendor"
        };
        while (iterator.hasNext()) {
            const QString absolute = iterator.next();
            const QString relative = QDir(m_workspace).relativeFilePath(absolute);
            if (relative.startsWith(".."))
                continue;
            bool skip = false;
            const QStringList parts = relative.split('/');
            for (const QString &part : parts) {
                if (skipped.contains(part) || (part.startsWith('.') && part != ".github")) {
                    skip = true;
                    break;
                }
            }
            if (skip)
                continue;
            paths << relative;
            if (paths.size() >= kMaxFiles)
                break;
        }
    }

    paths.removeDuplicates();
    paths.sort(Qt::CaseInsensitive);
    QVariantList all;
    for (const QString &path : paths) {
        if (!QFileInfo(QDir(m_workspace).absoluteFilePath(path)).isFile())
            continue;
        all.append(entry(path, statuses));
    }
    m_all = all;
    setFilter(m_filter);
}

void WorkspaceModel::setFilter(const QString &filter) {
    m_filter = filter.trimmed();
    QVariantList visible;
    for (const QVariant &value : m_all) {
        const QVariantMap file = value.toMap();
        if (m_filter.isEmpty() || file.value("path").toString().contains(m_filter, Qt::CaseInsensitive))
            visible.append(file);
    }
    m_files = visible;
    emit filesChanged();
}

QString WorkspaceModel::preview(const QString &path) const {
    const QString absolute = resolve(path);
    if (absolute.isEmpty())
        return tr("This file is outside the workspace.");
    QFile file(absolute);
    if (!file.open(QIODevice::ReadOnly))
        return tr("This file cannot be read.");
    const QByteArray data = file.read(kMaxPreview + 1);
    if (data.contains('\0'))
        return tr("Binary file — %1 bytes.").arg(file.size());
    QString text = QString::fromUtf8(data.left(kMaxPreview));
    if (data.size() > kMaxPreview)
        text += "\n" + tr("… (truncated)");
    return text;
}

QString WorkspaceModel::diff(const QString &path) const {
    const QString absolute = resolve(path);
    if (absolute.isEmpty())
        return tr("This file is outside the workspace.");

    const QString git = QStandardPaths::findExecutable(QStringLiteral("git"));
    if (!git.isEmpty() && QFileInfo::exists(m_workspace + "/.git")) {
        QProcess process;
        process.setWorkingDirectory(m_workspace);
        process.start(git, {QStringLiteral("diff"), QStringLiteral("HEAD"),
                            QStringLiteral("--no-color"), QStringLiteral("--"), path});
        if (process.waitForFinished(5000)) {
            QString output = QString::fromUtf8(process.readAllStandardOutput());
            if (output.trimmed().isEmpty()) {
                QProcess untracked;
                untracked.setWorkingDirectory(m_workspace);
                untracked.start(git, {QStringLiteral("diff"), QStringLiteral("--no-index"),
                                      QStringLiteral("--no-color"), QStringLiteral("/dev/null"), path});
                untracked.waitForFinished(5000);
                output = QString::fromUtf8(untracked.readAllStandardOutput());
            }
            if (!output.trimmed().isEmpty())
                return truncate(output, 320 * 1024);
        }
    }
    return tr("No diff for this file. Showing its contents instead.") + "\n\n" + preview(path);
}

QString WorkspaceModel::absolutePath(const QString &path) const {
    return resolve(path);
}

void WorkspaceModel::copyToClipboard(const QString &text) const {
    QGuiApplication::clipboard()->setText(text);
}
