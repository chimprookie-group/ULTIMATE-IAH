#include "skillmanager.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QSaveFile>
#include <QStandardPaths>
#include <QTextStream>
#include <QSet>
#include <algorithm>

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
    if (m_root == defaultRoot())
        seedDefaults();
    refresh();
}

QString SkillManager::defaultRoot() {
    return QDir::homePath() + "/.agents/skills";
}

QStringList SkillManager::defaultSkillIds() {
    return {QStringLiteral("research"), QStringLiteral("code-review"),
            QStringLiteral("test-first"), QStringLiteral("debug"),
            QStringLiteral("git-workflow"), QStringLiteral("github-project"),
            QStringLiteral("notes-vault"), QStringLiteral("security"),
            QStringLiteral("release"), QStringLiteral("automation"),
            QStringLiteral("refactor"), QStringLiteral("artcraft")};
}

QString SkillManager::defaultsMarker() const {
    return m_root + QStringLiteral("/.defaults-v1");
}

int SkillManager::defaultCount() const {
    const QStringList ids = defaultSkillIds();
    const QSet<QString> defaults(ids.begin(), ids.end());
    int count = 0;
    for (const QVariant &value : m_skills) {
        if (defaults.contains(value.toMap().value(QStringLiteral("id")).toString()))
            ++count;
    }
    return count;
}

bool SkillManager::seedDefaults(bool force) {
    if (!force && QFileInfo::exists(defaultsMarker())) {
        refresh();
        return true;
    }
    QDir().mkpath(m_root);
    const QStringList ids = defaultSkillIds();
    for (const QString &id : ids) {
        const QString directory = m_root + QLatin1Char('/') + id;
        const QString target = directory + QStringLiteral("/SKILL.md");
        if (QFileInfo::exists(target))
            continue;
        QFile resource(QStringLiteral(":/skills/default/") + id + QStringLiteral(".md"));
        if (!resource.open(QIODevice::ReadOnly))
            continue;
        QDir().mkpath(directory);
        QSaveFile file(target);
        if (!file.open(QIODevice::WriteOnly))
            continue;
        file.write(resource.readAll());
        if (!file.commit())
            continue;
        QFile::setPermissions(target, QFile::ReadOwner | QFile::WriteOwner);
    }
    QSaveFile marker(defaultsMarker());
    if (marker.open(QIODevice::WriteOnly)) {
        marker.write(QByteArray("Ultimate IAH default skills v1\n"));
        marker.commit();
    }
    refresh();
    return true;
}

QVariantMap SkillManager::readSkill(const QString &id) const {
    for (const QVariant &value : m_skills) {
        const QVariantMap skill = value.toMap();
        if (skill.value(QStringLiteral("id")).toString() != id
            && skill.value(QStringLiteral("name")).toString() != id)
            continue;
        QFile file(skill.value(QStringLiteral("path")).toString());
        if (!file.open(QIODevice::ReadOnly))
            return {};
        QVariantMap result = skill;
        result.insert(QStringLiteral("content"), QString::fromUtf8(file.readAll()).left(30000));
        return result;
    }
    return {};
}

QVariantList SkillManager::suggest(const QString &query, int limit) const {
    const QStringList raw = query.toLower().split(QRegularExpression(QStringLiteral("[^a-z0-9àâçéèêëîïôûùüÿñæœ]+")),
                                                  Qt::SkipEmptyParts);
    QStringList terms;
    const QSet<QString> stop = {QStringLiteral("the"), QStringLiteral("and"), QStringLiteral("pour"),
                                QStringLiteral("avec"), QStringLiteral("dans"), QStringLiteral("les"),
                                QStringLiteral("des"), QStringLiteral("une"), QStringLiteral("que"),
                                QStringLiteral("qui"), QStringLiteral("this"), QStringLiteral("that")};
    for (const QString &term : raw) {
        if (term.size() > 1 && !stop.contains(term))
            terms.append(term);
    }
    struct Scored { int score; QVariantMap skill; };
    QList<Scored> scored;
    for (const QVariant &value : m_skills) {
        const QVariantMap skill = value.toMap();
        const QString haystack = (skill.value(QStringLiteral("name")).toString() + " "
                                  + skill.value(QStringLiteral("description")).toString() + " "
                                  + skill.value(QStringLiteral("tags")).toStringList().join(' ')).toLower();
        int score = 0;
        for (const QString &term : terms) {
            if (haystack.contains(term))
                score += term.size() >= 4 ? 2 : 1;
        }
        if (score > 0)
            scored.append({score, skill});
    }
    std::sort(scored.begin(), scored.end(), [](const Scored &a, const Scored &b) { return a.score > b.score; });
    QVariantList out;
    for (int i = 0; i < scored.size() && i < limit; ++i) {
        QVariantMap skill = scored.at(i).skill;
        QFile file(skill.value(QStringLiteral("path")).toString());
        if (file.open(QIODevice::ReadOnly))
            skill.insert(QStringLiteral("content"), QString::fromUtf8(file.readAll()).left(8000));
        out.append(skill);
    }
    return out;
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
    QSet<QString> seen;
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
            QStringList tags;
            const QString tagsValue = frontMatterValue(text, "tags");
            for (const QString &tag : tagsValue.split(QRegularExpression(QStringLiteral("[, ]+")), Qt::SkipEmptyParts)) {
                QString clean = tag.trimmed();
                clean.remove(QRegularExpression(QStringLiteral("^\[|\]$")));
                if (!clean.isEmpty())
                    tags.append(clean);
            }
            if (seen.contains(entry.fileName()))
                continue;
            seen.insert(entry.fileName());
            skills.append(QVariantMap{
                {"id", entry.fileName()}, {"name", name}, {"description", description},
                {"path", file}, {"scope", root.second}, {"tags", tags},
                {"default", defaultSkillIds().contains(entry.fileName())},
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
