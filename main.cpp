#include <QApplication>
#include <QWebEngineProfile>
#include <QWebEngineSettings>
#include "BrowserWindow.h"

int main(int argc, char *argv[])
{
    // Initialize WebEngine before app creation to avoid some OpenGL issues if relevant
    QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts);
    QApplication app(argc, argv);

    // Apply basic application styling
    app.setStyleSheet("QMainWindow { background-color: #f7f7f7; }");

    BrowserWindow window;
    window.resize(1200, 800);
    window.show();

    return app.exec();
}
