#include <QCoreApplication>
#include <QFile>
#include <QTextStream>
#include <QThread>

int main(int argc, char **argv) {
    QCoreApplication app(argc, argv);
    const QStringList args = app.arguments().mid(1);
    QTextStream out(stdout), err(stderr);
    if (args.contains("--hang")) {
        while (true) QThread::sleep(1);
    }
    if (args == QStringList{"version"}) {
        out << "Xray 25.6.8 (Xray, Penetrates Everything.) test-build\nA unified platform for anti-censorship.\n";
        return 0;
    }
    const int configIndex = args.indexOf("-config");
    if (args.size() >= 4 && args.at(0) == "run" && args.contains("-test") && configIndex >= 0 && configIndex + 1 < args.size()) {
        QFile f(args.at(configIndex + 1));
        if (!f.open(QIODevice::ReadOnly)) {
            err << "failed to read config\n";
            return 23;
        }
        const auto data = f.readAll();
        if (data.contains("invalid")) {
            out << "checking config\n";
            err << "failed to load config: invalid character\n";
            return 23;
        }
        out << "Configuration OK.\n";
        err << "stderr notice\n";
        return 0;
    }
    if (args.size() >= 3 && args.at(0) == "run" && configIndex >= 0 && configIndex + 1 < args.size()) {
        QFile f(args.at(configIndex + 1));
        if (!f.open(QIODevice::ReadOnly)) {
            err << "failed to read config\n";
            return 2;
        }
        const auto data = f.readAll();
        out << "runtime ready\n";
        out.flush();
        err << "runtime stderr\n";
        err.flush();
        if (data.contains("crash")) return 42;
        while (true) QThread::sleep(1);
    }
    err << "unknown arguments: " << args.join(' ') << "\n";
    return 2;
}
