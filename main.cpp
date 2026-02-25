#include <QCoreApplication>
#include <QtCore/QObject>
#include <QtCore/QThread>
#include <QtCore/QTimer> 
#include <QtCore/QLoggingCategory>
#include <iostream>
#include <chrono>

std::atomic<int> counter = 0;
class MyObject : public QObject {
  Q_OBJECT
public:
  MyObject() : QObject(nullptr) {}
  ~MyObject() = default;
public slots:
  void doSomething() {
    counter.fetch_add(1, std::memory_order_relaxed);
  }
};

#include "main.moc"

int main(int argc, char *argv[])
{

    QCoreApplication app(argc, argv);
    MyObject objectOnThread;
    std::vector<std::thread> workers;
    for(int i = 0; i < 4; ++i) {
      workers.emplace_back(std::thread([&]{
        while(1) {
          
          //std::printf("scheduling 1\n");
          QTimer::singleShot(0, &objectOnThread, &MyObject::doSomething);
          std::this_thread::sleep_for(std::chrono::milliseconds(20));
        }
      }));
    }
  

    while(1) {
     //std::printf("processEvents\n");
      app.processEvents();
      std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    
    return 0;
    //return app.exec();
}