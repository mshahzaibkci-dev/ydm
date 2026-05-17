#include "mainwindow.h"
#include "constants.h"
#include "utils.h"
#include "themeprovider.h"
#include "widgets/ydmdialog.h"
#include "widgets/statchip.h"
#include "widgets/statusbadge.h"
#include "widgets/glossyprogressbar.h"
#include "persistence.h"

#include <QApplication>
#include <QDir>
#include <QFileInfo>
#include <QDateTime>
#include <QHeaderView>
#include <QStatusBar>
#include <QTableWidgetItem>
#include <QTimer>
#include <QUrl>
#include <QDesktopServices>
#include <QScrollBar>
#include <QRegularExpression>
#include <QHBoxLayout>
#include <QLabel>

// ─────────────────────────────────────────────
//  Add download (from input panel)
// ─────────────────────────────────────────────
void MainWindow::addDownload() {
    QString url = m_urlEdit ? m_urlEdit->text().trimmed() : QString();
    if (url.isEmpty()) {
        YDMDialog::showWarning(this, u"No URL"_qs,
                               u"Please paste a URL before pressing Download."_qs);
        return;
    }

    // Basic format check
    if (!url.startsWith(u"http://"_qs) && !url.startsWith(u"https://"_qs)) {
        YDMDialog::showWarning(this, u"Invalid URL"_qs,
            u"Only http:// and https:// URLs are supported."_qs);
        return;
    }
    if (!isSafeRemoteUrl(url)) {
        YDMDialog::showWarning(this, u"Blocked URL"_qs,
            u"This URL targets a private or loopback address and was blocked."_qs);
        return;
    }
    if (m_manager->urlInQueue(url)) {
        YDMDialog::showInfo(this, u"Already Queued"_qs,
            u"This URL is already active in the queue."_qs);
        return;
    }

    QString fmtKey = m_fmtCombo ? m_fmtCombo->currentText() : u"Best Quality (auto)"_qs;
    QString folder = m_folderLabel ? m_folderLabel->text() : QDir::homePath() + u"/Downloads"_qs;
    if (folder.isEmpty()) folder = QDir::homePath() + u"/Downloads"_qs;
    if (!QFileInfo::exists(folder)) QDir().mkpath(folder);

    QString catDir = makeCategorizedOutputDir(folder, {}, fmtKey);

    auto* item = new DownloadItem(DownloadItem::create(url, catDir, fmtKey));
    if (m_playlistCheck && m_playlistCheck->isChecked())
        item->isPlaylist = true;

    m_manager->add(item);

    if (m_urlEdit) m_urlEdit->clear();
    switchPage(PAGE_QUEUE);
    updateStatusBar();
}

// ─────────────────────────────────────────────
//  Manager → UI: item added
// ─────────────────────────────────────────────
void MainWindow::onItemAdded(DownloadItem* item) {
    insertTableRow(item);
    // Switch queue stack to table if needed
    if (m_queueStack && m_queueStack->currentIndex() == 0)
        m_queueStack->setCurrentIndex(1);
    updateStatusBar();
}

// ─────────────────────────────────────────────
//  Manager → UI: item updated
// ─────────────────────────────────────────────
void MainWindow::onItemUpdated(DownloadItem* item) {
    updateTableRow(item);

    if (item->status == DownloadStatus::Completed) {
        appendToCompletedTable(item);
        appendToHistory(item);
        // Tray notification
        if (m_tray) {
            QString name = item->filename.isEmpty()
                           ? QUrl(item->url).host()
                           : item->filename;
            m_tray->showMessage(u"RAINAX — Download Complete"_qs,
                name.left(80), QSystemTrayIcon::Information, 5000);
        }
    } else if (item->status == DownloadStatus::Failed ||
               item->status == DownloadStatus::Cancelled) {
        appendToHistory(item);
    }
    updateStatusBar();
}

// ─────────────────────────────────────────────
//  Insert a row into the queue table
// ─────────────────────────────────────────────
void MainWindow::insertTableRow(DownloadItem* item) {
    if (!m_table) return;
    int row = m_table->rowCount();
    m_table->insertRow(row);
    m_table->setRowHeight(row, 52);

    // Simple text items
    auto cell = [&](int col, const QString& text) {
        auto* it = new QTableWidgetItem(text);
        it->setFlags(it->flags() & ~Qt::ItemIsEditable);
        m_table->setItem(row, col, it);
        return it;
    };

    cell(COL_ID,       item->id);
    cell(COL_URL,      item->url);
    cell(COL_FORMAT,   item->displayFmt);
    cell(COL_FILENAME, item->filename.isEmpty() ? u"—"_qs : item->filename);
    cell(COL_SIZE,     item->filesize.isEmpty() ? u"—"_qs : item->filesize);

    // Speed chip
    auto* speedChip = new StatChip(u"—"_qs, u"#9f97ff"_qs);
    m_table->setCellWidget(row, COL_SPEED, speedChip);
    m_speedChips[item->id] = speedChip;

    // ETA chip
    auto* etaChip = new StatChip(u"—"_qs, u"#60a5fa"_qs);
    m_table->setCellWidget(row, COL_ETA, etaChip);
    m_etaChips[item->id] = etaChip;

    // Progress bar
    auto* pb = new GlossyProgressBar;
    pb->setStatus(item->status);
    m_table->setCellWidget(row, COL_PROGRESS, pb);
    m_progressBars[item->id] = pb;

    // Status badge
    auto* badge = new StatusBadge(item->status);
    m_table->setCellWidget(row, COL_STATUS, badge);
    m_statusBadges[item->id] = badge;

    m_rowMap[item->id] = row;
}

// ─────────────────────────────────────────────
//  Update an existing row
// ─────────────────────────────────────────────
void MainWindow::updateTableRow(DownloadItem* item) {
    if (!m_table) return;
    auto it = m_rowMap.find(item->id);
    if (it == m_rowMap.end()) return;
    int row = it.value();

    // Filename
    if (auto* fi = m_table->item(row, COL_FILENAME)) {
        fi->setText(item->filename.isEmpty() ? u"—"_qs : item->filename);
    }
    // Size
    if (auto* si = m_table->item(row, COL_SIZE)) {
        si->setText(item->filesize.isEmpty() ? u"—"_qs : item->filesize);
    }

    // Speed chip
    if (auto* sc = m_speedChips.value(item->id)) {
        sc->setText(item->speed.isEmpty() ? u"—"_qs : item->speed);
    }
    // ETA chip
    if (auto* ec = m_etaChips.value(item->id)) {
        ec->setText(item->eta.isEmpty() ? u"—"_qs : item->eta);
    }
    // Progress bar
    if (auto* pb = m_progressBars.value(item->id)) {
        pb->setStatus(item->status);
        pb->setProgress(item->progress);
    }
    // Status badge
    if (auto* badge = m_statusBadges.value(item->id)) {
        badge->setStatus(item->status);
    }

    // Dim terminal rows
    const QColor dimColor(ThemeProvider::active().dim);
    const QColor activeColor(ThemeProvider::active().text);
    bool terminal = isTerminal(item->status);
    for (int col = COL_ID; col <= COL_URL; ++col) {
        if (auto* cell = m_table->item(row, col))
            cell->setForeground(terminal ? dimColor : activeColor);
    }
}

// ─────────────────────────────────────────────
//  Append to completed table
// ─────────────────────────────────────────────
void MainWindow::appendToCompletedTable(DownloadItem* item) {
    if (!m_completedTable) return;

    if (m_completedStack && m_completedStack->currentIndex() == 0)
        m_completedStack->setCurrentIndex(1);

    int row = m_completedTable->rowCount();
    m_completedTable->insertRow(row);
    m_completedTable->setRowHeight(row, 40);
    m_completedRows[item->id] = row;

    auto cell = [&](int col, const QString& text) {
        auto* it = new QTableWidgetItem(text);
        it->setFlags(it->flags() & ~Qt::ItemIsEditable);
        m_completedTable->setItem(row, col, it);
    };

    cell(0, item->filename.isEmpty() ? QUrl(item->url).host() : item->filename);
    cell(1, item->displayFmt);
    cell(2, item->filesize.isEmpty() ? u"—"_qs : item->filesize);
    cell(3, item->outputDir);
}

// ─────────────────────────────────────────────
//  Append to history
// ─────────────────────────────────────────────
void MainWindow::appendToHistory(DownloadItem* item) {
    QVariantMap entry;
    entry[u"time"_qs]     = QDateTime::currentDateTime().toString(u"yyyy-MM-dd HH:mm:ss"_qs);
    entry[u"url"_qs]      = item->url;
    entry[u"filename"_qs] = item->filename.isEmpty()
                            ? QUrl(item->url).host()
                            : item->filename;
    entry[u"status"_qs]   = statusToString(item->status);

    m_historyEntries.append(entry);

    // Trim
    while (m_historyEntries.size() > HISTORY_MAX_ENTRIES)
        m_historyEntries.removeFirst();

    Persistence::saveHistory(m_historyEntries);

    // Add to visible history table (if not filtered out)
    QString filter = m_historySearch ? m_historySearch->text().toLower() : QString();
    QString fname = entry[u"filename"_qs].toString().toLower();
    QString url   = entry[u"url"_qs].toString().toLower();
    if (filter.isEmpty() || fname.contains(filter) || url.contains(filter)) {
        int row = m_historyTable ? m_historyTable->rowCount() : -1;
        if (m_historyTable && row >= 0) {
            m_historyTable->insertRow(row);
            m_historyTable->setRowHeight(row, 36);

            auto cell = [&](int col, const QString& text) {
                auto* it = new QTableWidgetItem(text);
                it->setFlags(it->flags() & ~Qt::ItemIsEditable);
                m_historyTable->setItem(row, col, it);
            };
            cell(0, entry[u"time"_qs].toString());
            cell(1, entry[u"filename"_qs].toString());
            cell(2, entry[u"url"_qs].toString());
            cell(3, entry[u"status"_qs].toString());

            const Palette& c = ThemeProvider::active();
            QString st = entry[u"status"_qs].toString();
            QColor sc = (st == u"Completed"_qs) ? QColor(c.success) :
                        (st == u"Failed"_qs)     ? QColor(c.danger)  :
                        QColor(c.text_sec);
            if (m_historyTable->item(row, 3))
                m_historyTable->item(row, 3)->setForeground(sc);

            // Scroll to bottom
            m_historyTable->scrollToBottom();
        }
    }
}

// ─────────────────────────────────────────────
//  History loaded from background thread
// ─────────────────────────────────────────────
void MainWindow::onHistoryLoaded(const QList<QVariantMap>& entries) {
    m_historyEntries = entries;
    populateHistoryTable();
}

void MainWindow::populateHistoryTable() {
    if (!m_historyTable) return;
    m_historyTable->setRowCount(0);
    const Palette& c = ThemeProvider::active();
    QString filter = m_historySearch ? m_historySearch->text().toLower() : QString();

    for (const auto& entry : std::as_const(m_historyEntries)) {
        QString fname  = entry[u"filename"_qs].toString();
        QString url    = entry[u"url"_qs].toString();
        QString status = entry[u"status"_qs].toString();
        QString time   = entry[u"time"_qs].toString();

        if (!filter.isEmpty() &&
            !fname.toLower().contains(filter) &&
            !url.toLower().contains(filter))
            continue;

        int row = m_historyTable->rowCount();
        m_historyTable->insertRow(row);
        m_historyTable->setRowHeight(row, 36);

        auto cell = [&](int col, const QString& text) {
            auto* it = new QTableWidgetItem(text);
            it->setFlags(it->flags() & ~Qt::ItemIsEditable);
            m_historyTable->setItem(row, col, it);
        };
        cell(0, time);
        cell(1, fname.isEmpty() ? QUrl(url).host() : fname);
        cell(2, url);
        cell(3, status);

        QColor sc = (status == u"Completed"_qs) ? QColor(c.success) :
                    (status == u"Failed"_qs)     ? QColor(c.danger)  :
                    QColor(c.text_sec);
        if (m_historyTable->item(row, 3))
            m_historyTable->item(row, 3)->setForeground(sc);
    }
}

void MainWindow::filterHistory(const QString&) {
    populateHistoryTable();
}

// ─────────────────────────────────────────────
//  Queue loaded from background thread
// ─────────────────────────────────────────────
void MainWindow::onQueueLoaded(const QList<DownloadItem*>& items) {
    for (auto* item : items) {
        m_manager->restoreItem(item);
        insertTableRow(item);
    }
    if (!items.isEmpty() && m_queueStack)
        m_queueStack->setCurrentIndex(1);

    m_manager->tryStartNext();
    updateStatusBar();
}

// ─────────────────────────────────────────────
//  Refresh table (queue changed)
// ─────────────────────────────────────────────
void MainWindow::refreshTable() {
    updateStatusBar();
    if (m_queueStack) {
        m_queueStack->setCurrentIndex(
            m_manager->queue.isEmpty() ? 0 : 1);
    }
}

// ─────────────────────────────────────────────
//  Log line received
// ─────────────────────────────────────────────
void MainWindow::onLogLine(const QString& id, const QString& line) {
    if (!m_logView) return;
    // Cap log view at ~2000 lines
    QTextCursor cur = m_logView->textCursor();
    m_logView->moveCursor(QTextCursor::End);

    // Colour-code line types
    const Palette& c = ThemeProvider::active();
    QString html;
    if (line.contains(u"[ERROR]"_qs) || line.startsWith(u"ERROR"_qs)) {
        html = QStringLiteral("<span style='color:%1'>%2</span>")
               .arg(c.danger)
               .arg(line.toHtmlEscaped().trimmed());
    } else if (line.contains(u"[download]"_qs)) {
        html = QStringLiteral("<span style='color:%1'>%2</span>")
               .arg(c.accent2)
               .arg(line.toHtmlEscaped().trimmed());
    } else if (line.contains(u"[DONE]"_qs) || line.contains(u"Completed"_qs)) {
        html = QStringLiteral("<span style='color:%1'>%2</span>")
               .arg(c.success)
               .arg(line.toHtmlEscaped().trimmed());
    } else {
        html = QStringLiteral("<span style='color:%1'>%2</span>")
               .arg(c.text_sec)
               .arg(line.toHtmlEscaped().trimmed());
    }

    m_logView->append(QStringLiteral(
        "<span style='color:%1;font-size:10px;'>[%2]</span> %3")
        .arg(c.border2)
        .arg(id)
        .arg(html));

    // Scroll to bottom
    auto* sb = m_logView->verticalScrollBar();
    if (sb) sb->setValue(sb->maximum());

    // Prune if too long (keep last ~150 blocks)
    QTextDocument* doc = m_logView->document();
    while (doc->blockCount() > 1800)
        doc->setPlainText(doc->toPlainText().section('\n', -1500));
}

// ─────────────────────────────────────────────
//  Log view: clear
// ─────────────────────────────────────────────
void MainWindow::clearLogView() {
    if (m_logView) m_logView->clear();
}

// ─────────────────────────────────────────────
//  Table selection
// ─────────────────────────────────────────────
void MainWindow::onTableSelectionChanged() {
    m_selectedItemId = selectedId();
}

QString MainWindow::selectedId() const {
    if (!m_table) return {};
    auto rows = m_table->selectedItems();
    if (rows.isEmpty()) return {};
    int row = m_table->currentRow();
    if (row < 0) return {};
    auto* it = m_table->item(row, COL_ID);
    return it ? it->text() : QString();
}

// ─────────────────────────────────────────────
//  Toolbar actions
// ─────────────────────────────────────────────
void MainWindow::actionPause() {
    if (!m_selectedItemId.isEmpty())
        m_manager->pauseItem(m_selectedItemId);
}

void MainWindow::actionResume() {
    if (!m_selectedItemId.isEmpty())
        m_manager->resumeItem(m_selectedItemId);
}

void MainWindow::actionCancel() {
    if (m_selectedItemId.isEmpty()) return;
    bool ok = YDMDialog::showQuestion(this, u"Cancel Download"_qs,
                                      u"Cancel this download?"_qs);
    if (ok) m_manager->cancelItem(m_selectedItemId);
}

void MainWindow::actionClear() {
    m_manager->clearFinished();
    // Remove terminal rows from queue table
    if (!m_table) return;
    for (int row = m_table->rowCount() - 1; row >= 0; --row) {
        auto* it = m_table->item(row, COL_ID);
        if (!it) continue;
        QString id = it->text();
        bool found = false;
        for (auto* item : std::as_const(m_manager->queue))
            if (item->id == id) { found = true; break; }
        if (!found) {
            m_table->removeRow(row);
            m_rowMap.remove(id);
            m_progressBars.remove(id);
            m_statusBadges.remove(id);
            m_speedChips.remove(id);
            m_etaChips.remove(id);
        }
    }
    // Renumber rowMap
    m_rowMap.clear();
    for (int row = 0; row < m_table->rowCount(); ++row) {
        if (auto* it = m_table->item(row, COL_ID))
            m_rowMap[it->text()] = row;
    }
    refreshTable();
}

void MainWindow::actionClearHistory() {
    bool ok = YDMDialog::showQuestion(this,
        u"Clear History"_qs,
        u"Remove all download history? This cannot be undone."_qs);
    if (!ok) return;
    m_historyEntries.clear();
    Persistence::saveHistory(m_historyEntries);
    if (m_historyTable) m_historyTable->setRowCount(0);
}

// ─────────────────────────────────────────────
//  Ticker (1 s interval)
// ─────────────────────────────────────────────
void MainWindow::tickIndicators() {
    updateStatusBar();

    // Animate live pill
    int running = 0;
    for (const auto* item : std::as_const(m_manager->queue))
        if (item->status == DownloadStatus::Running) ++running;

    if (m_liveLabel && m_liveDot) {
        const Palette& c = ThemeProvider::active();
        if (running > 0) {
            m_liveLabel->setText(
                QStringLiteral("%1 active").arg(running));
            // Pulse the dot opacity (simple blink)
            static bool blink = false;
            blink = !blink;
            m_liveDot->setStyleSheet(
                QStringLiteral("color: %1; font-size: 10px; background: transparent; border: none; opacity: %2;")
                .arg(c.success)
                .arg(blink ? 1.0 : 0.4, 0, 'f', 1));
        } else {
            m_liveLabel->setText(u"Idle"_qs);
            m_liveDot->setStyleSheet(
                u"color: #3d4f68; font-size: 10px; background: transparent; border: none;"_qs);
        }
    }
}

// ─────────────────────────────────────────────
//  Status bar + stat boxes
// ─────────────────────────────────────────────
void MainWindow::updateStatusBar() {
    int active = 0, queued = 0, done = 0;
    for (const auto* item : std::as_const(m_manager->queue)) {
        if (item->status == DownloadStatus::Running ||
            item->status == DownloadStatus::Starting) ++active;
        else if (item->status == DownloadStatus::Queued)    ++queued;
        else if (item->status == DownloadStatus::Completed) ++done;
    }
    done += m_completedTable ? m_completedTable->rowCount() : 0;

    if (m_statActive) m_statActive->setValue(QString::number(active));
    if (m_statQueue)  m_statQueue->setValue(QString::number(queued));
    if (m_statDone)   m_statDone->setValue(QString::number(done));
    if (m_statSpeed)  m_statSpeed->setValue(
        active > 0 ? aggregateSpeedStr() : u"—"_qs);

    statusBar()->showMessage(
        QStringLiteral("  Active: %1   Queued: %2   Done: %3")
        .arg(active).arg(queued).arg(done));
}

// ─────────────────────────────────────────────
//  Aggregate speed across all running items
// ─────────────────────────────────────────────
QString MainWindow::aggregateSpeedStr() const {
    static QRegularExpression numRe(u"([\\d.]+)\\s*(\\S+)/s"_qs);
    double total = 0;
    for (const auto* item : std::as_const(m_manager->queue)) {
        if (item->status != DownloadStatus::Running || item->speed.isEmpty()) continue;
        auto m = numRe.match(item->speed);
        if (!m.hasMatch()) continue;
        double val = m.captured(1).toDouble();
        QString unit = m.captured(2).toUpper();
        if (unit == u"KB"_qs)      val *= 1024;
        else if (unit == u"MB"_qs) val *= 1024 * 1024;
        else if (unit == u"GB"_qs) val *= 1024.0 * 1024 * 1024;
        total += val;
    }
    if (total <= 0) return u"—"_qs;
    return fmtSize(total) + u"/s"_qs;
}
