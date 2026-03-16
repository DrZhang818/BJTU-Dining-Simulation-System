#include <QApplication>
#include <QMessageBox>
#include <QPushButton>
#include <QVBoxLayout>
#include <QWidget>

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);

    QWidget window;
    window.setWindowTitle("BJTU Dining Simulation System");
    window.resize(400, 300);

    QVBoxLayout* layout = new QVBoxLayout(&window);
    QPushButton* btn = new QPushButton("Start Simulation", &window);
    layout->addWidget(btn);

    QObject::connect(btn, &QPushButton::clicked, [&]() {
        QMessageBox::information(&window, "Success", "Qt6 application is running successfully!");
    });

    window.show();
    return app.exec();
}