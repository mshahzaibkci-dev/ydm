#pragma once
#include <QString>
#include <QHostAddress>
#include <optional>

// ── Filename sanitization ─────────────────────────────────────────────────
QString sanitizeFilename(const QString& name, const QString& fallback = QStringLiteral("download"));

// ── Path traversal guard ──────────────────────────────────────────────────
// Returns canonicalized path if it is safely under baseDir, else nullopt.
std::optional<QString> safeJoin(const QString& baseDir, const QString& part);

// ── SSRF / URL safety check ───────────────────────────────────────────────
// Returns true only for public http/https URLs (no loopback/private/reserved).
bool isSafeRemoteUrl(const QString& url);

// ── Formatting helpers ────────────────────────────────────────────────────
QString fmtSize(double bytes);
QString fmtEta(int seconds);

// ── Atomic file write ─────────────────────────────────────────────────────
bool atomicWriteText(const QString& path, const QString& data);

// ── API token ─────────────────────────────────────────────────────────────
QString loadOrCreateApiToken();

// ── yt-dlp / ffmpeg availability ─────────────────────────────────────────
bool checkYtdlpAvailable();
QString getYtdlpVersion();
QString getFfmpegVersion();

// ── Unique destination path ───────────────────────────────────────────────
QString uniqueDest(const QString& path);
