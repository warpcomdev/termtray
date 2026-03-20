#include <QAction>
#include <QApplication>
#include <QCloseEvent>
#include <QColor>
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QFont>
#include <QFontDatabase>
#include <QFontMetrics>
#include <QGuiApplication>
#include <QIcon>
#include <QMainWindow>
#include <QMenu>
#include <QPlainTextEdit>
#include <QProcess>
#include <QScrollBar>
#include <QStyle>
#include <QSystemTrayIcon>
#include <QTextCursor>
#include <QTextDocument>
#include <QTextOption>
#include <QTimer>

#include <cstdio>
#include <optional>

namespace {

constexpr int kScrollbackLines = 200;
constexpr int kTerminalCols = 80;
constexpr int kTerminalRows = 25;

struct Options {
    QColor textColor{0x66, 0xaa, 0x66};
    QString title{"termtray"};
    QString iconPath;
    QStringList command;
};

QString usageText() {
    return "Usage: termtray [-color RGB] [-title TEXT] [-icon PATH] -- <command> [args...]\n";
}

QString normalizeNewlines(QString text) {
    text.replace("\r\n", "\n");
    text.replace('\r', '\n');
    return text;
}

QString shellPreview(const QStringList &args) {
    return args.join(' ');
}

std::optional<QColor> parseNibbleColor(const QString &value, QString &error) {
    if (value.size() != 3) {
        error = QString("invalid -color \"%1\": expected exactly 3 hex digits like 0f0 or ccc").arg(value);
        return std::nullopt;
    }

    int channels[3] = {0, 0, 0};
    for (int i = 0; i < value.size(); ++i) {
        bool ok = false;
        const int nibble = QString(value[i]).toInt(&ok, 16);
        if (!ok) {
            error = QString("invalid -color \"%1\": expected hex digits 0-9 or a-f").arg(value);
            return std::nullopt;
        }
        channels[i] = nibble * 17;
    }

    return QColor(channels[0], channels[1], channels[2]);
}

std::optional<Options> parseArgs(const QStringList &args, QString &error) {
    Options options;

    for (int i = 0; i < args.size(); ++i) {
        const QString &arg = args[i];
        if (arg == "--") {
            options.command = args.mid(i + 1);
            return options;
        }

        if (arg == "-color") {
            if (i + 1 >= args.size()) {
                error = "missing value for -color\n" + usageText();
                return std::nullopt;
            }

            const auto color = parseNibbleColor(args[i + 1], error);
            if (!color.has_value()) {
                return std::nullopt;
            }

            options.textColor = *color;
            ++i;
            continue;
        }

        if (arg == "-title") {
            if (i + 1 >= args.size()) {
                error = "missing value for -title\n" + usageText();
                return std::nullopt;
            }

            options.title = args[i + 1];
            ++i;
            continue;
        }

        if (arg == "-icon") {
            if (i + 1 >= args.size()) {
                error = "missing value for -icon\n" + usageText();
                return std::nullopt;
            }

            options.iconPath = args[i + 1];
            ++i;
            continue;
        }

        options.command = args.mid(i);
        return options;
    }

    return options;
}

QIcon loadAppIcon(const QString &iconPath = QString()) {
    if (!iconPath.isEmpty()) {
        const QIcon selected = QIcon::fromTheme(iconPath);
        if (selected.isNull() == false) {
            return selected;
        }
    }

    const QStringList candidates = {
        QDir(QCoreApplication::applicationDirPath()).filePath("assets/icons/termtray.svg"),
        QDir::current().filePath("assets/icons/termtray.svg"),
    };

    for (const QString &candidate : candidates) {
        if (QFileInfo::exists(candidate)) {
            return QIcon(candidate);
        }
    }

    return QIcon::fromTheme("utilities-terminal");
}

QSize terminalWindowSize(const QFont &font) {
    const QFontMetrics metrics(font);
    const int width = metrics.horizontalAdvance(QLatin1Char('M')) * kTerminalCols + 36;
    const int height = metrics.lineSpacing() * kTerminalRows + 36;
    return QSize(width, height);
}

class TermTrayWindow : public QMainWindow {
public:
    explicit TermTrayWindow(const Options &options)
        : textView_(new QPlainTextEdit(this)),
          process_(new QProcess(this)),
          trayIcon_(new QSystemTrayIcon(this)),
          trayMenu_(new QMenu(this)),
          showAction_(new QAction("Show", this)),
          quitAction_(new QAction("Quit", this)),
          traySupported_(QSystemTrayIcon::isSystemTrayAvailable()) {
        setWindowTitle(options.title);
        setWindowIcon(loadAppIcon(options.iconPath));

        QFont font = QFontDatabase::systemFont(QFontDatabase::FixedFont);
        font.setStyleHint(QFont::Monospace);
        textView_->setFont(font);
        textView_->setReadOnly(true);
        textView_->setUndoRedoEnabled(false);
        textView_->setWordWrapMode(QTextOption::NoWrap);
        textView_->document()->setMaximumBlockCount(kScrollbackLines);
        textView_->setStyleSheet(QString(
            "QPlainTextEdit { background: #000000; color: %1; border: 0; padding: 6px; }")
            .arg(options.textColor.name()));
        setCentralWidget(textView_);
        resize(terminalWindowSize(font));

        QApplication::setQuitOnLastWindowClosed(false);

        connect(showAction_, &QAction::triggered, this, [this]() { restoreWindow(); });
        connect(quitAction_, &QAction::triggered, this, [this]() { requestQuit(); });

        if (traySupported_) {
            trayMenu_->addAction(showAction_);
            trayMenu_->addSeparator();
            trayMenu_->addAction(quitAction_);

            trayIcon_->setIcon(windowIcon().isNull() ? style()->standardIcon(QStyle::SP_ComputerIcon) : windowIcon());
            trayIcon_->setToolTip(options.title);
            trayIcon_->setContextMenu(trayMenu_);
            connect(trayIcon_, &QSystemTrayIcon::activated, this, [this](QSystemTrayIcon::ActivationReason reason) {
                if (reason == QSystemTrayIcon::Trigger || reason == QSystemTrayIcon::DoubleClick) {
                    restoreWindow();
                }
            });
            trayIcon_->show();
        }

        connect(process_, &QProcess::readyReadStandardOutput, this, [this]() {
            appendText(QString::fromLocal8Bit(process_->readAllStandardOutput()));
        });
        connect(process_, &QProcess::readyReadStandardError, this, [this]() {
            appendText(QString::fromLocal8Bit(process_->readAllStandardError()));
        });
        connect(process_, &QProcess::errorOccurred, this, [this](QProcess::ProcessError error) {
            if (error == QProcess::FailedToStart) {
                appendText(QString("[start error] %1\n").arg(process_->errorString()));
            }
        });
        connect(process_, qOverload<int, QProcess::ExitStatus>(&QProcess::finished), this,
                [this](int exitCode, QProcess::ExitStatus) {
                    if (exiting_) {
                        return;
                    }

                    if (exitCode == 0) {
                        appendText("\n[process exited successfully]\n");
                    } else {
                        appendText(QString("\n[process exited with status %1]\n").arg(exitCode));
                    }

                    QTimer::singleShot(0, qApp, &QApplication::quit);
                });
    }

    void launch(const QStringList &command) {
        if (command.isEmpty()) {
            appendText(usageText());
            return;
        }

        appendText(QString("$ %1\n").arg(shellPreview(command)));
        process_->setProgram(command.first());
        process_->setArguments(command.mid(1));
        process_->start();
    }

protected:
    void closeEvent(QCloseEvent *event) override {
        if (exiting_) {
            event->accept();
            return;
        }

        if (!traySupported_) {
            appendText("\n[unable to minimize to tray: system tray is not supported; exiting]\n");
            requestQuit();
            event->accept();
            return;
        }

        appendText("\n[window hidden to tray]\n");
        hide();
        event->ignore();
    }

private:
    void appendText(const QString &text) {
        if (text.isEmpty()) {
            return;
        }

        textView_->moveCursor(QTextCursor::End);
        textView_->insertPlainText(normalizeNewlines(text));
        textView_->verticalScrollBar()->setValue(textView_->verticalScrollBar()->maximum());
    }

    void restoreWindow() {
        show();
        raise();
        activateWindow();
    }

    void requestQuit() {
        if (exiting_) {
            return;
        }

        exiting_ = true;
        if (trayIcon_->isVisible()) {
            trayIcon_->hide();
        }

        if (process_->state() != QProcess::NotRunning) {
            process_->terminate();
            if (!process_->waitForFinished(300)) {
                process_->kill();
                process_->waitForFinished();
            }
        }

        QApplication::quit();
    }

    QPlainTextEdit *textView_;
    QProcess *process_;
    QSystemTrayIcon *trayIcon_;
    QMenu *trayMenu_;
    QAction *showAction_;
    QAction *quitAction_;
    bool traySupported_;
    bool exiting_ = false;
};

}  // namespace

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("termtray");
    QGuiApplication::setDesktopFileName("termtray");

    QString error;
    const QStringList args = QCoreApplication::arguments().mid(1);
    const auto parsed = parseArgs(args, error);
    if (!parsed.has_value()) {
        fprintf(stderr, "%s\n", error.toLocal8Bit().constData());
        return 2;
    }

    app.setApplicationDisplayName(parsed->title);

    TermTrayWindow window(*parsed);
    window.show();
    window.launch(parsed->command);

    return app.exec();
}
