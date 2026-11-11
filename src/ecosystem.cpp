#include "ecosystem.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QStringList>

namespace {

QString readText(const QString &path, qint64 limit = 8192) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
        return {};
    return QString::fromUtf8(file.read(limit));
}

QString cleanValue(QString value) {
    value = value.trimmed();
    if ((value.startsWith('"') && value.endsWith('"')) || (value.startsWith('\'') && value.endsWith('\'')))
        value = value.mid(1, value.size() - 2);
    return value.trimmed();
}

QVariantMap command(const QString &id, const QString &label, const QString &line,
                    const QString &kind) {
    return QVariantMap{{"id", id}, {"label", label}, {"command", line}, {"kind", kind}};
}

void addCommand(QVariantList &commands, const QString &id, const QString &label,
                const QString &line, const QString &kind) {
    for (const QVariant &existing : commands) {
        if (existing.toMap().value("id").toString() == id)
            return;
    }
    commands.append(command(id, label, line, kind));
}

QString packageManager(const QString &workspace) {
    const QDir dir(workspace);
    if (dir.exists("bun.lock") || dir.exists("bun.lockb"))
        return "bun";
    if (dir.exists("pnpm-lock.yaml"))
        return "pnpm";
    if (dir.exists("yarn.lock"))
        return "yarn";
    if (dir.exists("package-lock.json"))
        return "npm";
    return "npm";
}

QVariantMap inspectNode(const QString &workspace, QStringList &files) {
    QVariantMap project;
    project["type"] = "node";
    const QString manager = packageManager(workspace);
    project["manager"] = manager;
    project["label"] = QStringLiteral("Node.js · %1").arg(manager);
    files << "package.json";

    QVariantList commands;
    addCommand(commands, "install", QObject::tr("Installer"), manager + " install", "setup");

    const QString packagePath = workspace + "/package.json";
    const QJsonObject root = QJsonDocument::fromJson(readText(packagePath).toUtf8()).object();
    const QJsonObject scripts = root.value("scripts").toObject();
    const QStringList preferred = {"test", "build", "lint", "typecheck", "check", "dev", "start"};
    QStringList names;
    for (const QString &name : preferred) {
        if (scripts.contains(name))
            names << name;
    }
    for (auto it = scripts.begin(); it != scripts.end() && names.size() < 8; ++it) {
        if (!names.contains(it.key()))
            names << it.key();
    }
    for (const QString &name : names) {
        addCommand(commands, "script:" + name, name, manager + " run " + name, "script");
    }
    if (!scripts.contains("test"))
        addCommand(commands, "test", QObject::tr("Tests"), manager + " test", "test");
    project["commands"] = commands;
    return project;
}

QVariantMap inspectPython(const QString &workspace, QStringList &files) {
    QVariantMap project;
    project["type"] = "python";
    project["label"] = "Python";
    project["manager"] = QFileInfo::exists(workspace + "/uv.lock") ? "uv" : "pip";
    QVariantList commands;
    addCommand(commands, "test", QObject::tr("Tests"), "python3 -m pytest", "test");
    if (!QStandardPaths::findExecutable("ruff").isEmpty())
        addCommand(commands, "lint", "Ruff", "ruff check .", "lint");
    if (!QStandardPaths::findExecutable("mypy").isEmpty())
        addCommand(commands, "typecheck", "Mypy", "mypy .", "typecheck");
    if (QFileInfo::exists(workspace + "/requirements.txt"))
        addCommand(commands, "install", QObject::tr("Installer"), "python3 -m pip install -r requirements.txt", "setup");
    if (QFileInfo::exists(workspace + "/uv.lock"))
        addCommand(commands, "sync", "uv sync", "uv sync", "setup");
    project["commands"] = commands;
    files << "Python";
    return project;
}

QVariantMap inspectRust(const QString &workspace, QStringList &files) {
    Q_UNUSED(workspace)
    QVariantMap project;
    project["type"] = "rust";
    project["label"] = "Rust";
    project["manager"] = "cargo";
    QVariantList commands;
    addCommand(commands, "test", QObject::tr("Tests"), "cargo test", "test");
    addCommand(commands, "build", QObject::tr("Compiler"), "cargo build", "build");
    addCommand(commands, "lint", "Clippy", "cargo clippy --all-targets", "lint");
    addCommand(commands, "fmt", "Format", "cargo fmt --check", "format");
    project["commands"] = commands;
    files << "Cargo.toml";
    return project;
}

QVariantMap inspectGo(const QString &workspace, QStringList &files) {
    Q_UNUSED(workspace)
    QVariantMap project;
    project["type"] = "go";
    project["label"] = "Go";
    project["manager"] = "go";
    QVariantList commands;
    addCommand(commands, "test", QObject::tr("Tests"), "go test ./...", "test");
    addCommand(commands, "build", QObject::tr("Compiler"), "go build ./...", "build");
    project["commands"] = commands;
    files << "go.mod";
    return project;
}

QVariantMap inspectQt(const QString &workspace, QStringList &files) {
    QDir dir(workspace);
    QString projectFile;
    for (const QString &candidate : dir.entryList({"*.pro"}, QDir::Files)) {
        projectFile = candidate;
        break;
    }
    QVariantMap project;
    project["type"] = "qt";
    project["label"] = "Qt / QML";
    project["manager"] = "qmake6";
    QVariantList commands;
    addCommand(commands, "qmake", "qmake6", "qmake6 " + (projectFile.isEmpty() ? QString() : projectFile), "build");
    addCommand(commands, "make", QObject::tr("Compiler"), "make", "build");
    project["commands"] = commands;
    files << (projectFile.isEmpty() ? "Qt" : projectFile);
    return project;
}

QVariantMap genericProject(const QString &workspace) {
    QVariantMap project;
    project["type"] = "workspace";
    project["label"] = QObject::tr("Espace de travail");
    project["manager"] = QString();
    QVariantList commands;
    addCommand(commands, "list", QObject::tr("Lister"), "ls -la", "inspect");
    project["commands"] = commands;
    project["workspace"] = workspace;
    return project;
}

} // namespace

Ecosystem::Ecosystem(QObject *parent)
    : QObject(parent) {}

QVariantList Ecosystem::scanTools() {
    struct Tool { const char *id; const char *label; const char *versionArg; };
    static const Tool specs[] = {
        {"dsh", "DeepSeek Harness", "--version"},
        {"opencode", "OpenCode", "--version"},
        {"git", "Git", "--version"},
        {"node", "Node.js", "--version"},
        {"bun", "Bun", "--version"},
        {"npm", "npm", "--version"},
        {"pnpm", "pnpm", "--version"},
        {"python3", "Python 3", "--version"},
        {"uv", "uv", "--version"},
        {"cargo", "Cargo", "--version"},
        {"go", "Go", "version"},
        {"make", "Make", "--version"},
        {"qmake6", "qmake6", "-v"},
        {"docker", "Docker", "--version"},
        {"ollama", "Ollama", "--version"},
    };

    QVariantList tools;
    for (const Tool &spec : specs) {
        const QString path = QStandardPaths::findExecutable(QString::fromLatin1(spec.id));
        QVariantMap tool;
        tool["id"] = QString::fromLatin1(spec.id);
        tool["label"] = QString::fromLatin1(spec.label);
        tool["available"] = !path.isEmpty();
        tool["path"] = path;
        tool["version"] = QString();
        if (!path.isEmpty()) {
            QProcess process;
            process.setProcessChannelMode(QProcess::MergedChannels);
            process.start(path, {QString::fromLatin1(spec.versionArg)});
            if (process.waitForStarted(800) && process.waitForFinished(800)) {
                QString output = QString::fromUtf8(process.readAll()).trimmed();
                tool["version"] = output.split('\n').value(0).trimmed().left(70);
            } else {
                process.kill();
                process.waitForFinished(50);
            }
        }
        tools.append(tool);
    }
    return tools;
}

QVariantMap Ecosystem::inspectProject(const QString &workspace) {
    if (workspace.isEmpty() || !QFileInfo(workspace).isDir())
        return genericProject(workspace);

    const QDir dir(workspace);
    QStringList files;
    QVariantMap project;
    if (dir.exists("package.json")) {
        project = inspectNode(workspace, files);
    } else if (dir.exists("Cargo.toml")) {
        project = inspectRust(workspace, files);
    } else if (dir.exists("go.mod")) {
        project = inspectGo(workspace, files);
    } else if (dir.exists("pyproject.toml") || dir.exists("requirements.txt")
               || dir.exists("setup.py")) {
        project = inspectPython(workspace, files);
    } else if (!dir.entryList({"*.pro"}, QDir::Files).isEmpty()) {
        project = inspectQt(workspace, files);
    } else if (dir.exists("CMakeLists.txt")) {
        project = QVariantMap{{"type", "cmake"}, {"label", "CMake"}, {"manager", "cmake"}};
        QVariantList commands;
        addCommand(commands, "configure", QObject::tr("Configurer"), "cmake -S . -B build", "build");
        addCommand(commands, "build", QObject::tr("Compiler"), "cmake --build build", "build");
        project["commands"] = commands;
        files << "CMakeLists.txt";
    } else if (dir.exists("Makefile")) {
        project = QVariantMap{{"type", "make"}, {"label", "Make"}, {"manager", "make"}};
        QVariantList commands;
        addCommand(commands, "make", QObject::tr("Compiler"), "make", "build");
        project["commands"] = commands;
        files << "Makefile";
    } else {
        project = genericProject(workspace);
    }

    if (dir.exists(".git")) {
        QVariantList commands = project.value("commands").toList();
        addCommand(commands, "git-status", "Git status", "git status --short --branch", "git");
        addCommand(commands, "git-diff", "Git diff", "git diff --stat", "git");
        project["commands"] = commands;
        files << ".git";
    }
    if (dir.exists("docker-compose.yml") || dir.exists("compose.yaml")) {
        QVariantList commands = project.value("commands").toList();
        addCommand(commands, "compose", "Docker Compose", "docker compose up", "run");
        project["commands"] = commands;
        files << "compose";
    }

    project["workspace"] = workspace;
    project["name"] = QFileInfo(workspace).fileName();
    project["files"] = files;
    if (!project.contains("commands"))
        project["commands"] = QVariantList{};
    return project;
}

QVariantList Ecosystem::inspectSkills(const QString &root) {
    QVariantList skills;
    QDir dir(root);
    if (!dir.exists())
        return skills;
    const QFileInfoList entries = dir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);
    for (const QFileInfo &entry : entries) {
        const QString skillFile = entry.filePath() + "/SKILL.md";
        if (!QFileInfo::exists(skillFile))
            continue;
        const QString text = readText(skillFile, 16384);
        QStringList lines = text.split('\n');

        QStringList front;
        QStringList bodyLines;
        int frontEnd = -1;
        if (!lines.isEmpty() && lines.first().trimmed() == "---") {
            for (int i = 1; i < lines.size(); ++i) {
                if (lines.at(i).trimmed() == "---") {
                    frontEnd = i;
                    break;
                }
            }
            if (frontEnd > 0) {
                front = lines.mid(1, frontEnd - 1);
                bodyLines = lines.mid(frontEnd + 1);
            }
        }
        if (frontEnd < 0)
            bodyLines = lines;

        QString name;
        QString description;
        for (int i = 0; i < front.size(); ++i) {
            const QString line = front.at(i);
            if (line.startsWith("name:")) {
                name = cleanValue(line.mid(5));
            } else if (line.startsWith("description:")) {
                QString value = line.mid(12).trimmed();
                if (value == ">" || value == "|") {
                    QStringList parts;
                    for (++i; i < front.size(); ++i) {
                        const QString part = front.at(i);
                        if (!part.isEmpty() && !part.startsWith(' ') && !part.startsWith('\t'))
                            break;
                        if (!part.trimmed().isEmpty())
                            parts << part.trimmed();
                    }
                    value = parts.join(' ');
                }
                description = cleanValue(value);
            }
        }

        if (name.isEmpty()) {
            for (const QString &line : bodyLines) {
                if (line.startsWith("# ") && !line.startsWith("## ")) {
                    name = line.mid(2).trimmed();
                    break;
                }
            }
        }
        if (name.isEmpty())
            name = entry.fileName();

        if (description.isEmpty()) {
            bool afterHeading = false;
            QStringList paragraph;
            for (const QString &raw : bodyLines) {
                const QString line = raw.trimmed();
                if (line.startsWith("# ") && !line.startsWith("## ")) {
                    afterHeading = true;
                    continue;
                }
                if (!afterHeading)
                    continue;
                if (line.isEmpty()) {
                    if (!paragraph.isEmpty())
                        break;
                    continue;
                }
                if (line.startsWith('#') || line.startsWith("---") || line.startsWith("name:")
                    || line.startsWith("description:"))
                    continue;
                paragraph << line;
            }
            description = paragraph.join(' ');
        }

        skills.append(QVariantMap{{"id", entry.fileName()},
                                  {"name", name},
                                  {"description", description.left(220)},
                                  {"path", skillFile}});
        if (skills.size() >= 80)
            break;
    }
    return skills;
}

QVariantList Ecosystem::scanSkills() const {
    QStringList roots;
    const QString home = QDir::homePath();
    roots << home + "/.agents/skills"
          << home + "/.config/opencode/skills"
          << home + "/.claude/skills";
    if (!m_workspace.isEmpty())
        roots << m_workspace + "/.opencode/skills" << m_workspace + "/.agents/skills";

    QVariantList all;
    QStringList seen;
    for (const QString &root : roots) {
        const QVariantList found = inspectSkills(root);
        for (const QVariant &value : found) {
            const QVariantMap skill = value.toMap();
            const QString key = skill.value("name").toString().toLower();
            if (seen.contains(key))
                continue;
            seen << key;
            all.append(skill);
        }
    }
    return all;
}

void Ecosystem::refresh(const QString &workspace) {
    const QString clean = workspace.isEmpty() ? m_workspace : QDir::cleanPath(workspace);
    if (clean != m_workspace) {
        m_workspace = clean;
        emit changed();
    }
    if (!m_toolsLoaded) {
        m_tools = scanTools();
        m_toolsLoaded = true;
    }
    m_project = inspectProject(m_workspace);
    m_skills = scanSkills();
    emit changed();
}

void Ecosystem::rescan() {
    m_toolsLoaded = false;
    refresh(m_workspace);
}

QString Ecosystem::brief() const {
    QStringList lines;
    const QString name = m_project.value("name").toString();
    lines << tr("Contexte de l'espace de travail %1 :").arg(name.isEmpty() ? m_workspace : name);
    lines << tr("- Chemin : %1").arg(m_workspace);
    const QString label = m_project.value("label").toString();
    if (!label.isEmpty())
        lines << tr("- Projet : %1").arg(label);
    const QVariantList commands = m_project.value("commands").toList();
    for (int i = 0; i < commands.size() && i < 4; ++i) {
        const QVariantMap command = commands.at(i).toMap();
        lines << tr("- Commande %1 : %2").arg(command.value("label").toString(),
                                                command.value("command").toString());
    }
    QStringList tools;
    for (const QVariant &value : m_tools) {
        const QVariantMap tool = value.toMap();
        if (tool.value("available").toBool())
            tools << tool.value("id").toString();
    }
    if (!tools.isEmpty())
        lines << tr("- Outils disponibles : %1").arg(tools.mid(0, 10).join(", "));
    if (!m_skills.isEmpty()) {
        QStringList names;
        for (int i = 0; i < m_skills.size() && i < 6; ++i)
            names << m_skills.at(i).toMap().value("name").toString();
        lines << tr("- Skills : %1").arg(names.join(", "));
    }
    return lines.join('\n');
}
