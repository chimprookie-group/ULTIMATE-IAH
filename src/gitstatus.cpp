#include "gitstatus.h"

#include <QStringList>

QVariantMap GitStatus::parse(const QString &output) {
    QVariantMap result{{"branch", QString()}, {"dirty", 0}};
    int dirty = 0;
    QString branch;

    const QStringList lines = output.split('\n', Qt::SkipEmptyParts);
    for (const QString &line : lines) {
        if (line.startsWith("## ")) {
            const QString header = line.mid(3).trimmed();
            if (header.startsWith("No commits yet on ")) {
                branch = header.mid(18).trimmed();
            } else {
                branch = header.section("...", 0, 0).section(' ', 0, 0).trimmed();
            }
            continue;
        }
        if (!line.trimmed().isEmpty())
            ++dirty;
    }

    result["branch"] = branch;
    result["dirty"] = dirty;
    return result;
}
