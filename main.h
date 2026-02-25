#include <QCoreApplication>
#include <QtCore/QObject>
#include <QtCore/QThread>
#include <QtCore/QTimer>
#include <iostream>
#include <chrono>

class MyObject : public QObject {
  Q_OBJECT
public:
  MyObject() : QObject(nullptr) {}
  ~MyObject() = default;
public slots:
  void doSomething() {
    std::cout << "doing something" << std::endl;
  }
};