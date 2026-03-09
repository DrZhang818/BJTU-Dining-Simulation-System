#include <QApplication>
#include <QWidget>
#include <QPushButton>
#include <QVBoxLayout>

int main(int argc, char* argv[]) {
    QApplication a(argc, argv);

    QWidget window;
    window.setWindowTitle("Qt Application");
    window.resize(400, 300);

    QVBoxLayout* layout = new QVBoxLayout(&window);
    QPushButton* btn    = new QPushButton("Start Simulation", &window);
    layout->addWidget(btn);

    window.show();
    return a.exec();
}