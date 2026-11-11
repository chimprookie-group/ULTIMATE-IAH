#pragma once

#include <QVariantMap>

// Parses `git status --porcelain=v1 -b` without touching the repository.
namespace GitStatus {
QVariantMap parse(const QString &output);
}
