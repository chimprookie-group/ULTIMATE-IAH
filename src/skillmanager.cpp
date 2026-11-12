#include "skillmanager.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QSaveFile>
#include <QStandardPaths>
#include <QTextStream>

namespace {
QString frontMatterValue(const QString &text, const QString &key) {
    const QString prefix = key + ":";
    const QStringList lines = text.split('\n');
    for (int i = 0; i < lines.size(); ++i) {
        const QString line = lines.at(i);
        if (!line.trimmed().startsWith(prefix))
            continue;
        QString value = line.mid(line.indexOf(':') + 1).trimmed();
        if (value == ">" || value == "|") {
            QStringList parts;
            for (int j = i + 1; j < lines.size(); ++j) {
                const QString next = lines.at(j);
                if (next.trimmed().isEmpty()) {
                    if (!parts.isEmpty())
                        parts << QString();
                    continue;
                }
                if (!next.startsWith(' ') && !next.startsWith('\t'))
                    break;
                parts << next.trimmed();
            }
            return parts.join(' ').simplified();
        }
        return value.remove('"').remove('\'');
    }
    return {};
}
} // namespace

SkillManager::SkillManager(const QString &rootPath, QObject *parent)
    : QObject(parent),
      m_root(rootPath.isEmpty() ? defaultRoot() : rootPath),
      m_workspace(QDir::homePath()) {
    refresh();
}

QString SkillManager::defaultRoot() {
    return QDir::homePath() + "/.agents/skills";
}

void SkillManager::setWorkspace(const QString &workspace) {
    if (workspace.isEmpty() || workspace == m_workspace)
        return;
    m_workspace = workspace;
    refresh();
}

QString SkillManager::slug(const QString &name) const {
    QString value = name.toLower().trimmed();
    value.replace(QRegularExpression(QStringLiteral("[^a-z0-9_-]+")), QStringLiteral("-"));
    value.remove(QRegularExpression(QStringLiteral("^-+|-+$")));
    return value.isEmpty() ? QStringLiteral("skill") : value;
}

void SkillManager::refresh() {
    QVariantList skills;
    QList<QPair<QString, QString>> roots;
    roots.append({m_root, QStringLiteral("user")});
    roots.append({m_workspace + "/.agents/skills", QStringLiteral("project")});
    for (const auto &root : roots) {
        QDir directory(root.first);
        if (!directory.exists())
            continue;
        for (const QFileInfo &entry : directory.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name)) {
            const QString file = entry.filePath() + "/SKILL.md";
            QFile skill(file);
            if (!skill.open(QIODevice::ReadOnly))
                continue;
            const QString text = QString::fromUtf8(skill.readAll());
            QString name = frontMatterValue(text, "name");
            if (name.isEmpty())
                name = entry.fileName();
            QString description = frontMatterValue(text, "description");
            if (description.isEmpty()) {
                for (const QString &line : text.split('\n')) {
                    const QString clean = line.trimmed();
                    if (!clean.isEmpty() && !clean.startsWith('#') && !clean.startsWith("---")
                        && !clean.startsWith("name:") && !clean.startsWith("description:")) {
                        description = clean.left(220);
                        break;
                    }
                }
            }
            skills.append(QVariantMap{
                {"id", entry.fileName()}, {"name", name}, {"description", description},
                {"path", file}, {"scope", root.second},
            });
        }
    }
    m_skills = skills;
    emit changed();
}

bool SkillManager::createSkill(const QString &name, const QString &description,
                               const QString &instructions) {
    if (name.trimmed().isEmpty() || instructions.trimmed().isEmpty()) {
        setError(tr("Le nom et les instructions du skill sont obligatoires."));
        return false;
    }
    const QString id = slug(name);
    const QString directory = m_root + "/" + id;
    if (QFileInfo::exists(directory + "/SKILL.md")) {
        setError(tr("Ce skill existe déjà."));
        return false;
    }
    QDir().mkpath(directory);
    QSaveFile file(directory + "/SKILL.md");
    if (!file.open(QIODevice::WriteOnly)) {
        setError(tr("Impossible d'écrire le skill."));
        return false;
    }
    const QString content = QStringLiteral("---\nname: %1\ndescription: %2\n---\n\n# %1\n\n%3\n")
                                .arg(name.trimmed(), description.trimmed().isEmpty() ? name.trimmed() : description.trimmed(),
                                     instructions.trimmed());
    file.write(content.toUtf8());
    if (!file.commit()) {
        setError(tr("Impossible d'enregistrer le skill."));
        return false;
    }
    QFile::setPermissions(directory + "/SKILL.md", QFile::ReadOwner | QFile::WriteOwner);
    setError({});
    refresh();
    return true;
}

bool SkillManager::removeSkill(const QString &id) {
    if (id.isEmpty() || id.contains('/') || id.contains(".."))
        return false;
    const QString directory = m_root + "/" + id;
    if (!QFileInfo(directory + "/SKILL.md").isFile())
        return false;
    QDir(directory).removeRecursively();
    refresh();
    return true;
}

void SkillManager::setError(const QString &error) {
    if (error == m_error)
        return;
    m_error = error;
    emit errorChanged();
}

void SkillManager::clearError() { setError({}); }
