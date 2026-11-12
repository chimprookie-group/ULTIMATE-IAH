#include "agentmanager.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QSaveFile>
#include <QStandardPaths>

namespace {
QString slug(const QString &name) {
    QString value = name.toLower().trimmed();
    value.replace(QRegularExpression(QStringLiteral("[^a-z0-9_-]+")), QStringLiteral("-"));
    value.remove(QRegularExpression(QStringLiteral("^-+|-+$")));
    return value.isEmpty() ? QStringLiteral("agent") : value;
}
} // namespace

AgentManager::AgentManager(const QString &path, QObject *parent)
    : QObject(parent), m_path(path.isEmpty() ? defaultPath() : path) {
    load();
}

QString AgentManager::defaultPath() {
    return QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation) + "/agents.json";
}

QVariantMap AgentManager::builtin(const QString &id) const {
    if (id == "planner") {
        return QVariantMap{
            {"id", "planner"}, {"name", tr("Planificateur")}, {"role", "planner"}, {"builtin", true},
            {"description", tr("Décompose, anticipe les risques, définit les critères de vérification.")},
            {"prompt", tr("Tu es le planificateur d'Ultimate IAH. Analyse la tâche, inspecte le projet si nécessaire, puis produis un plan numéroté, concret et vérifiable. Ne modifie aucun fichier. Identifie les risques, les dépendances et les commandes de vérification.")},
        };
    }
    if (id == "builder") {
        return QVariantMap{
            {"id", "builder"}, {"name", tr("Exécutant")}, {"role", "builder"}, {"builtin", true},
            {"description", tr("Implémente, teste, corrige et rend le résultat vérifiable.")},
            {"prompt", tr("Tu es l'exécutant d'Ultimate IAH. Applique le plan dans le projet, modifie réellement les fichiers nécessaires, lance les vérifications pertinentes et corrige les erreurs. Sois précis, ne t'arrête pas à une explication.")},
        };
    }
    if (id == "reviewer") {
        return QVariantMap{
            {"id", "reviewer"}, {"name", tr("Réviseur")}, {"role", "reviewer"}, {"builtin", true},
            {"description", tr("Relit le diff, cherche les régressions, juge la vérification.")},
            {"prompt", tr("Tu es le réviseur d'Ultimate IAH. Examine le travail réellement effectué dans le projet, relis les changements et les tests, cherche les bugs, régressions, cas limites et problèmes de sécurité. Termine par « VERDICT: PASS » ou « VERDICT: FAIL » avec les corrections nécessaires.")},
        };
    }
    if (id == "researcher") {
        return QVariantMap{
            {"id", "researcher"}, {"name", tr("Chercheur")}, {"role", "researcher"}, {"builtin", true},
            {"description", tr("Explore le code et les sources, synthétise sans modifier.")},
            {"prompt", tr("Tu es le chercheur d'Ultimate IAH. Explore le code, les fichiers et les sources utiles, puis réponds avec une synthèse sourcée et les incertitudes. Ne modifie rien.")},
        };
    }
    return {};
}

bool AgentManager::load() {
    m_custom.clear();
    QFile file(m_path);
    if (file.exists() && file.open(QIODevice::ReadOnly)) {
        const QJsonArray array = QJsonDocument::fromJson(file.readAll()).array();
        for (const QJsonValue &value : array) {
            const QJsonObject object = value.toObject();
            if (object.value("id").toString().isEmpty() || object.value("prompt").toString().isEmpty())
                continue;
            m_custom.append(QVariantMap{
                {"id", object.value("id").toString()},
                {"name", object.value("name").toString(object.value("id").toString())},
                {"role", object.value("role").toString("custom")},
                {"builtin", false},
                {"description", object.value("description").toString()},
                {"prompt", object.value("prompt").toString()},
            });
        }
    }
    rebuild();
    return true;
}

bool AgentManager::save() {
    QJsonArray array;
    for (const QVariant &value : m_custom) {
        const QVariantMap profile = value.toMap();
        array.append(QJsonObject{
            {"id", profile.value("id").toString()},
            {"name", profile.value("name").toString()},
            {"role", profile.value("role").toString()},
            {"description", profile.value("description").toString()},
            {"prompt", profile.value("prompt").toString()},
        });
    }
    QDir().mkpath(QFileInfo(m_path).absolutePath());
    QSaveFile file(m_path);
    if (!file.open(QIODevice::WriteOnly)) {
        setError(tr("Impossible d'écrire les profils d'agents."));
        return false;
    }
    file.write(QJsonDocument(array).toJson(QJsonDocument::Indented));
    if (!file.commit()) {
        setError(tr("Impossible d'enregistrer les profils d'agents."));
        return false;
    }
    return true;
}

void AgentManager::rebuild() {
    m_profiles = QVariantList{builtin("planner"), builtin("builder"), builtin("reviewer"), builtin("researcher")};
    for (const QVariant &value : m_custom)
        m_profiles.append(value);
    emit changed();
}

QString AgentManager::uniqueId(const QString &name) const {
    QString base = slug(name);
    QString candidate = base;
    int suffix = 2;
    while (!profile(candidate).isEmpty())
        candidate = base + "-" + QString::number(suffix++);
    return candidate;
}

bool AgentManager::addProfile(const QString &name, const QString &role, const QString &prompt) {
    if (name.trimmed().isEmpty() || prompt.trimmed().isEmpty()) {
        setError(tr("Le nom et le prompt de l'agent sont obligatoires."));
        return false;
    }
    const QString id = uniqueId(name);
    m_custom.append(QVariantMap{
        {"id", id}, {"name", name.trimmed()}, {"role", role.trimmed().isEmpty() ? "custom" : role.trimmed()},
        {"builtin", false}, {"description", tr("Profil personnalisé")}, {"prompt", prompt.trimmed()},
    });
    if (!save())
        return false;
    setError({});
    rebuild();
    return true;
}

bool AgentManager::removeProfile(const QString &id) {
    for (int i = 0; i < m_custom.size(); ++i) {
        if (m_custom.at(i).toMap().value("id").toString() == id) {
            m_custom.removeAt(i);
            if (!save())
                return false;
            rebuild();
            return true;
        }
    }
    return false;
}

void AgentManager::refresh() {
    load();
}

QVariantMap AgentManager::profile(const QString &id) const {
    for (const QVariant &value : m_profiles)
        if (value.toMap().value("id").toString() == id)
            return value.toMap();
    return {};
}

QString AgentManager::promptFor(const QString &id) const {
    return profile(id).value("prompt").toString();
}

void AgentManager::setError(const QString &error) {
    if (error == m_error)
        return;
    m_error = error;
    emit errorChanged();
}
