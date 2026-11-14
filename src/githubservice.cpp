#include "githubservice.h"

#include "backend.h"

#include <QDesktopServices>
#include <QDir>
#include <QFileInfo>
#include <QJsonDocument>
#include <QProcess>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QUrl>

GitHubService::GitHubService(Backend *backend, QObject *parent)
    : QObject(parent), m_backend(backend) {
    refresh();
}

GitHubService::~GitHubService() = default;

QString GitHubService::workspace() const {
    return m_backend ? m_backend->workspace() : QDir::homePath();
}

QString GitHubService::ghBinary() const {
    const QString override = qEnvironmentVariable("ULTIMATE_IAH_GH");
    if (!override.isEmpty())
        return override;
    return QStandardPaths::findExecutable(QStringLiteral("gh"));
}

QJsonValue GitHubService::runGh(const QStringList &arguments, int timeoutMs, bool *ok,
                                QString *output) const {
    if (ok) *ok = false;
    const QString binary = ghBinary();
    if (binary.isEmpty())
        return QJsonValue();
    QProcess process;
    process.setProcessChannelMode(QProcess::SeparateChannels);
    process.setWorkingDirectory(workspace());
    process.start(binary, arguments);
    if (!process.waitForStarted(5000))
        return QJsonValue();
    if (!process.waitForFinished(timeoutMs)) {
        process.kill();
        process.waitForFinished(2000);
        if (output) *output = tr("Délai dépassé.");
        return QJsonValue();
    }
    const QString stdoutText = QString::fromUtf8(process.readAllStandardOutput()).trimmed();
    const QString stderrText = QString::fromUtf8(process.readAllStandardError()).trimmed();
    // gh auth status writes its scopes on stderr (or stdout depending on the
    // version); keep both so the parser works everywhere.
    if (output) *output = (stderrText + QLatin1Char('\n') + stdoutText).trimmed();
    if (process.exitStatus() != QProcess::NormalExit || process.exitCode() != 0) {
        if (output && output->isEmpty()) *output = stdoutText;
        return QJsonValue();
    }
    if (ok) *ok = true;
    if (stdoutText.isEmpty())
        return QJsonValue();
    QJsonParseError error;
    const QJsonDocument document = QJsonDocument::fromJson(stdoutText.toUtf8(), &error);
    if (error.error == QJsonParseError::NoError)
        return document.isArray() ? QJsonValue(document.array()) : QJsonValue(document.object());
    return QJsonValue(stdoutText);
}

void GitHubService::setLoading(bool loading) {
    if (m_loading == loading) return;
    m_loading = loading;
    emit loadingChanged();
}

void GitHubService::setError(const QString &error) {
    if (m_error == error) return;
    m_error = error;
    emit errorChanged();
}

void GitHubService::clearError() { setError({}); }

void GitHubService::refresh() {
    setLoading(true);
    setError({});
    bool ok = false;
    QString error;
    QJsonValue user = runGh({QStringLiteral("api"), QStringLiteral("user"), QStringLiteral("--jq"),
                             QStringLiteral(".login + \"\\n\" + (.name // \"\")")}, 15000, &ok, &error);
    if (!ok) {
        m_authenticated = false;
        m_login.clear();
        m_accountName.clear();
        m_repositories.clear();
        m_issues.clear();
        m_pulls.clear();
        setError(error.isEmpty() ? tr("GitHub CLI n'est pas connecté. Lancez gh auth login.") : error);
        setLoading(false);
        emit changed();
        return;
    }
    const QStringList lines = user.toString().split('\n');
    m_login = lines.value(0).trimmed();
    m_accountName = lines.value(1).trimmed();
    m_authenticated = !m_login.isEmpty();
    QString statusOutput;
    bool statusOk = false;
    runGh({QStringLiteral("auth"), QStringLiteral("status")}, 10000, &statusOk, &statusOutput);
    const QRegularExpression scopesExpression(QStringLiteral("Token scopes:\\s*(.+)"));
    const auto match = scopesExpression.match(statusOutput);
    m_scopes = match.hasMatch() ? match.captured(1).trimmed() : QString();
    QJsonValue repos = runGh({QStringLiteral("repo"), QStringLiteral("list"), QStringLiteral("--limit"), QStringLiteral("100"),
                              QStringLiteral("--json"), QStringLiteral("nameWithOwner,name,description,visibility,isPrivate,url,updatedAt,primaryLanguage,stargazerCount,isFork")},
                             30000, &ok, &error);
    if (ok && repos.isArray())
        m_repositories = parseRepositories(repos.toArray());
    else
        m_repositories.clear();
    if (!ok)
        setError(error.isEmpty() ? tr("Impossible de charger les dépôts GitHub.") : error);
    if (m_selectedRepo.isEmpty() && !m_repositories.isEmpty())
        m_selectedRepo = m_repositories.first().toMap().value(QStringLiteral("fullName")).toString();
    setLoading(false);
    emit changed();
    emit selectedChanged();
    if (!m_selectedRepo.isEmpty()) {
        loadIssues(m_selectedRepo);
        loadPulls(m_selectedRepo);
    }
}

QVariantList GitHubService::parseRepositories(const QJsonArray &array) {
    QVariantList out;
    for (const QJsonValue &value : array) {
        const QJsonObject repo = value.toObject();
        const QJsonObject language = repo.value(QStringLiteral("primaryLanguage")).toObject();
        out.append(QVariantMap{
            {QStringLiteral("fullName"), repo.value(QStringLiteral("nameWithOwner")).toString()},
            {QStringLiteral("name"), repo.value(QStringLiteral("name")).toString()},
            {QStringLiteral("description"), repo.value(QStringLiteral("description")).toString()},
            {QStringLiteral("visibility"), repo.value(QStringLiteral("visibility")).toString()},
            {QStringLiteral("private"), repo.value(QStringLiteral("isPrivate")).toBool()},
            {QStringLiteral("url"), repo.value(QStringLiteral("url")).toString()},
            {QStringLiteral("updatedAt"), repo.value(QStringLiteral("updatedAt")).toString()},
            {QStringLiteral("language"), language.value(QStringLiteral("name")).toString()},
            {QStringLiteral("stars"), repo.value(QStringLiteral("stargazerCount")).toInt()},
            {QStringLiteral("fork"), repo.value(QStringLiteral("isFork")).toBool()},
        });
    }
    return out;
}

QVariantList GitHubService::parseIssues(const QJsonArray &array) {
    QVariantList out;
    for (const QJsonValue &value : array) {
        const QJsonObject issue = value.toObject();
        const QJsonObject author = issue.value(QStringLiteral("author")).toObject();
        out.append(QVariantMap{
            {QStringLiteral("number"), issue.value(QStringLiteral("number")).toInt()},
            {QStringLiteral("title"), issue.value(QStringLiteral("title")).toString()},
            {QStringLiteral("state"), issue.value(QStringLiteral("state")).toString()},
            {QStringLiteral("updatedAt"), issue.value(QStringLiteral("updatedAt")).toString()},
            {QStringLiteral("url"), issue.value(QStringLiteral("url")).toString()},
            {QStringLiteral("author"), author.value(QStringLiteral("login")).toString()},
        });
    }
    return out;
}

QVariantList GitHubService::parsePulls(const QJsonArray &array) {
    QVariantList out;
    for (const QJsonValue &value : array) {
        const QJsonObject pull = value.toObject();
        out.append(QVariantMap{
            {QStringLiteral("number"), pull.value(QStringLiteral("number")).toInt()},
            {QStringLiteral("title"), pull.value(QStringLiteral("title")).toString()},
            {QStringLiteral("state"), pull.value(QStringLiteral("state")).toString()},
            {QStringLiteral("draft"), pull.value(QStringLiteral("isDraft")).toBool()},
            {QStringLiteral("updatedAt"), pull.value(QStringLiteral("updatedAt")).toString()},
            {QStringLiteral("url"), pull.value(QStringLiteral("url")).toString()},
            {QStringLiteral("branch"), pull.value(QStringLiteral("headRefName")).toString()},
        });
    }
    return out;
}

void GitHubService::selectRepository(const QString &fullName) {
    if (fullName == m_selectedRepo) return;
    m_selectedRepo = fullName;
    m_issues.clear();
    m_pulls.clear();
    emit selectedChanged();
    loadIssues(fullName);
    loadPulls(fullName);
}

QVariantMap GitHubService::selectedRepository() const {
    for (const QVariant &value : m_repositories) {
        if (value.toMap().value(QStringLiteral("fullName")).toString() == m_selectedRepo)
            return value.toMap();
    }
    return {};
}

void GitHubService::loadIssues(const QString &fullName) {
    if (fullName.isEmpty()) return;
    bool ok = false;
    QString error;
    const QJsonValue value = runGh({QStringLiteral("issue"), QStringLiteral("list"), QStringLiteral("--repo"), fullName,
                                    QStringLiteral("--limit"), QStringLiteral("30"), QStringLiteral("--json"),
                                    QStringLiteral("number,title,state,updatedAt,url,author")}, 30000, &ok, &error);
    if (ok && value.isArray())
        m_issues = parseIssues(value.toArray());
    else
        m_issues.clear();
    emit changed();
}

void GitHubService::loadPulls(const QString &fullName) {
    if (fullName.isEmpty()) return;
    bool ok = false;
    QString error;
    const QJsonValue value = runGh({QStringLiteral("pr"), QStringLiteral("list"), QStringLiteral("--repo"), fullName,
                                    QStringLiteral("--limit"), QStringLiteral("30"), QStringLiteral("--json"),
                                    QStringLiteral("number,title,state,isDraft,updatedAt,url,headRefName")}, 30000, &ok, &error);
    if (ok && value.isArray())
        m_pulls = parsePulls(value.toArray());
    else
        m_pulls.clear();
    emit changed();
}

QString GitHubService::repoName(const QString &fullName) const {
    return fullName.section(QLatin1Char('/'), -1);
}

QString GitHubService::clonePath(const QString &fullName) const {
    return workspace() + QLatin1Char('/') + repoName(fullName);
}

QJsonObject GitHubService::createRepositoryJson(const QString &name, const QString &description,
                                                bool privateRepo, bool clone) {
    QVariantMap result = createRepository(name, description, privateRepo, clone);
    return QJsonObject::fromVariantMap(result);
}

QVariantMap GitHubService::createRepository(const QString &name, const QString &description,
                                           bool privateRepo, bool clone) {
    const QString cleanName = name.trimmed();
    static const QRegularExpression pattern(QStringLiteral("^[A-Za-z0-9._-]{1,100}$"));
    if (!pattern.match(cleanName).hasMatch() || cleanName == QLatin1String(".") || cleanName == QLatin1String("..")) {
        setError(tr("Nom de dépôt invalide."));
        return {{QStringLiteral("ok"), false}, {QStringLiteral("error"), tr("Nom de dépôt invalide.")}};
    }
    setLoading(true);
    setError({});
    QStringList args{QStringLiteral("repo"), QStringLiteral("create"), cleanName,
                     privateRepo ? QStringLiteral("--private") : QStringLiteral("--public")};
    if (!description.trimmed().isEmpty())
        args << QStringLiteral("--description") << description.trimmed();
    bool ok = false;
    QString error;
    QJsonValue output = runGh(args, 60000, &ok, &error);
    if (!ok) {
        setError(error.isEmpty() ? tr("Création du dépôt impossible.") : error);
        setLoading(false);
        return {{QStringLiteral("ok"), false}, {QStringLiteral("error"), error}};
    }
    const QString fullName = m_login + QLatin1Char('/') + cleanName;
    QVariantMap result{{QStringLiteral("ok"), true}, {QStringLiteral("fullName"), fullName},
                       {QStringLiteral("url"), QStringLiteral("https://github.com/") + fullName}};
    if (clone) {
        const QString path = clonePath(fullName);
        QProcess process;
        process.setProcessChannelMode(QProcess::MergedChannels);
        process.setWorkingDirectory(workspace());
        process.start(ghBinary(), {QStringLiteral("repo"), QStringLiteral("clone"), fullName, path});
        process.waitForFinished(120000);
        result.insert(QStringLiteral("cloned"), process.exitCode() == 0);
        result.insert(QStringLiteral("path"), path);
        if (process.exitCode() != 0)
            result.insert(QStringLiteral("cloneError"), QString::fromUtf8(process.readAll()).trimmed());
    }
    refresh();
    setLoading(false);
    return result;
}

bool GitHubService::cloneRepository(const QString &fullName) {
    static const QRegularExpression repoPattern(QStringLiteral("^[A-Za-z0-9_.-]+/[A-Za-z0-9_.-]+$"));
    if (!repoPattern.match(fullName).hasMatch()
        || fullName.section(QLatin1Char('/'), -1) == QLatin1String("." )
        || fullName.section(QLatin1Char('/'), -1) == QLatin1String("..")) {
        setError(tr("Dépôt GitHub invalide."));
        return false;
    }
    const QString path = clonePath(fullName);
    if (QFileInfo::exists(path)) {
        setError(tr("Le dossier de destination existe déjà : %1").arg(path));
        return false;
    }
    setLoading(true);
    QString error;
    bool ok = false;
    runGh({QStringLiteral("repo"), QStringLiteral("clone"), fullName, path}, 120000, &ok, &error);
    setLoading(false);
    if (!ok) {
        setError(error.isEmpty() ? tr("Clonage impossible.") : error);
        return false;
    }
    setError({});
    return true;
}

bool GitHubService::createIssue(const QString &title, const QString &body) {
    if (m_selectedRepo.isEmpty() || title.trimmed().isEmpty()) return false;
    setLoading(true);
    QString error;
    bool ok = false;
    runGh({QStringLiteral("issue"), QStringLiteral("create"), QStringLiteral("--repo"), m_selectedRepo,
           QStringLiteral("--title"), title.trimmed(), QStringLiteral("--body"), body}, 60000, &ok, &error);
    setLoading(false);
    if (!ok) {
        setError(error.isEmpty() ? tr("Création de l'issue impossible.") : error);
        return false;
    }
    setError({});
    loadIssues(m_selectedRepo);
    return true;
}

QJsonObject GitHubService::createIssueJson(const QString &title, const QString &body) {
    return QJsonObject{{QStringLiteral("ok"), createIssue(title, body)}, {QStringLiteral("error"), error()}};
}

QVariantMap GitHubService::createPullRequest(const QString &title, const QString &body,
                                             const QString &base, const QString &head) {
    QVariantMap result{{QStringLiteral("ok"), false}};
    if (m_selectedRepo.isEmpty()) {
        setError(tr("Sélectionnez un dépôt avant de créer une pull request."));
        result.insert(QStringLiteral("error"), error());
        return result;
    }
    if (title.trimmed().isEmpty()) {
        setError(tr("Le titre de la pull request est obligatoire."));
        result.insert(QStringLiteral("error"), error());
        return result;
    }
    const QString targetBase = base.trimmed().isEmpty() ? QStringLiteral("main") : base.trimmed();
    const QString targetHead = head.trimmed();
    setLoading(true);
    QString processError;
    bool ok = false;
    const QJsonValue value = runGh(GitHubCli::pullRequestArguments(m_selectedRepo, title, body,
                                                                  targetBase, targetHead),
                                   60000, &ok, &processError);
    setLoading(false);
    if (!ok) {
        setError(processError.isEmpty() ? tr("Création de la pull request impossible.") : processError);
        result.insert(QStringLiteral("error"), error());
        return result;
    }
    setError({});
    result.insert(QStringLiteral("ok"), true);
    result.insert(QStringLiteral("repo"), m_selectedRepo);
    result.insert(QStringLiteral("base"), targetBase);
    result.insert(QStringLiteral("head"), targetHead);
    result.insert(QStringLiteral("url"), value.toString().section(QLatin1Char('\n'), 0, 0).trimmed());
    loadPulls(m_selectedRepo);
    return result;
}

QJsonObject GitHubService::createPullRequestJson(const QString &title, const QString &body,
                                                 const QString &base, const QString &head) {
    return QJsonObject::fromVariantMap(createPullRequest(title, body, base, head));
}

QJsonObject GitHubService::cloneJson(const QString &fullName) {
    return QJsonObject{{QStringLiteral("ok"), cloneRepository(fullName)}, {QStringLiteral("error"), error()}};
}

QJsonObject GitHubService::issuesJson(const QString &fullName) {
    if (!fullName.isEmpty()) loadIssues(fullName);
    return QJsonObject{{QStringLiteral("ok"), true},
                       {QStringLiteral("repo"), fullName.isEmpty() ? m_selectedRepo : fullName},
                       {QStringLiteral("issues"), QJsonArray::fromVariantList(m_issues)}};
}

QJsonObject GitHubService::pullsJson(const QString &fullName) {
    if (!fullName.isEmpty()) loadPulls(fullName);
    return QJsonObject{{QStringLiteral("ok"), true},
                       {QStringLiteral("repo"), fullName.isEmpty() ? m_selectedRepo : fullName},
                       {QStringLiteral("pulls"), QJsonArray::fromVariantList(m_pulls)}};
}

void GitHubService::openRepository(const QString &url) {
    if (!url.isEmpty()) QDesktopServices::openUrl(QUrl(url));
}

void GitHubService::openWorkspace() {
    QDesktopServices::openUrl(QUrl::fromLocalFile(workspace()));
}

QJsonObject GitHubService::status() const {
    return QJsonObject{
        {QStringLiteral("ok"), true},
        {QStringLiteral("authenticated"), m_authenticated},
        {QStringLiteral("login"), m_login},
        {QStringLiteral("name"), m_accountName},
        {QStringLiteral("scopes"), m_scopes},
        {QStringLiteral("repositories"), m_repositories.size()},
        {QStringLiteral("selectedRepo"), m_selectedRepo},
        {QStringLiteral("issues"), m_issues.size()},
        {QStringLiteral("pulls"), m_pulls.size()},
        {QStringLiteral("workspace"), workspace()},
    };
}
