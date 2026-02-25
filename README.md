# Intro
This repo is for https://qt-project.atlassian.net/browse/QTBUG-144362 to try and show that there's an issue with QMutex.


You can run `run.sh` to build and run this project with tsan on and without the runtime annotations/suppressions from Qt.

A couple notes:

* This uses QT 6.5.5 which is not qtbase head but the closest public version to what's being used internally
* I'm on a Mac which does not have futexs in 6.5.5 because it does not have [this pr](https://github.com/qt/qtbase/commit/d2368cde70062bbaa9db463d76cd135dccb55e23) yet.
    * Even at HEAD, using `-DFEATURE_appstore_compliant` turns off futex support because it relies on private Apple APIs and I can repro this issue there.


# Hypothesis

In the case where there's contention and [unlockInternal calls wakeUp to try and pass the lock directly to a waiter](https://github.com/qt/qtbase/blob/ffe3f3565e8b9a5c7e0ea6b33dc7993d3902077e/src/corelib/thread/qmutex.cpp#L799), are there any read/write barriers that would require all writes to be committed before the handoff? This path seems to have a lot of relaxed/release loads and stores.

the [d->wait condition in lockInternal](https://github.com/qt/qtbase/blob/ffe3f3565e8b9a5c7e0ea6b33dc7993d3902077e/src/corelib/thread/qmutex.cpp#L736) also does all relaxed operations before returning true. 

I think unlockInternal can pass the lock to the waiter without doing any acquire loads if the waiter is already at the d->wait check in lockInternal

# Latest Status

Making sure to run this with `-DFEATURE_santize_thread=ON` I know get the stack trace below. It seem [this line](https://github.com/qt/qtbase/blob/31729a29d0762f7dc98f3dfab67e84325f9925e0/src/corelib/kernel/qcoreapplication.cpp#L1675) is racing to write the `data-canWait` for the reciever's thread data.

```
WARNING: ThreadSanitizer: data race (pid=73403)
  Write of size 1 at 0x0001064012b1 by thread T8:
    #0 QCoreApplication::postEvent(QObject*, QEvent*, int) qcoreapplication.cpp:1675 (libQt6Core_debug.6.5.5.dylib:arm64+0x152864)
    #1 QMetaObject::invokeMethodImpl(QObject*, QtPrivate::QSlotObjectBase*, Qt::ConnectionType, void*) qmetaobject.cpp:1617 (libQt6Core_debug.6.5.5.dylib:arm64+0x18b420)
    #2 QTimer::singleShotImpl(int, Qt::TimerType, QObject const*, QtPrivate::QSlotObjectBase*) qtimer.cpp:350 (libQt6Core_debug.6.5.5.dylib:arm64+0x2b5884)
    #3 void QTimer::singleShot<int, void (MyObject::*)()>(int, Qt::TimerType, QtPrivate::FunctionPointer<void (MyObject::*)()>::Object const*, void (MyObject::*)()) qtimer.h:89 (hello_world_app:arm64+0x1000074e0)
    #4 void QTimer::singleShot<int, void (MyObject::*)()>(int, QtPrivate::FunctionPointer<void (MyObject::*)()>::Object const*, void (MyObject::*)()) qtimer.h:76 (hello_world_app:arm64+0x1000073f4)
    #5 main::$_0::operator()() const main.cpp:34 (hello_world_app:arm64+0x100007318)
    #6 decltype(std::declval<main::$_0>()()) std::__1::__invoke[abi:ne180100]<main::$_0>(main::$_0&&) invoke.h:344 (hello_world_app:arm64+0x10000723c)
    #7 void std::__1::__thread_execute[abi:ne180100]<std::__1::unique_ptr<std::__1::__thread_struct, std::__1::default_delete<std::__1::__thread_struct>>, main::$_0>(std::__1::tuple<std::__1::unique_ptr<std::__1::__thread_struct, std::__1::default_delete<std::__1::__thread_struct>>, main::$_0>&, std::__1::__tuple_indices<>) thread.h:199 (hello_world_app:arm64+0x100007168)
    #8 void* std::__1::__thread_proxy[abi:ne180100]<std::__1::tuple<std::__1::unique_ptr<std::__1::__thread_struct, std::__1::default_delete<std::__1::__thread_struct>>, main::$_0>>(void*) thread.h:208 (hello_world_app:arm64+0x10000651c)

  Previous write of size 1 at 0x0001064012b1 by thread T7:
    #0 QCoreApplication::postEvent(QObject*, QEvent*, int) qcoreapplication.cpp:1675 (libQt6Core_debug.6.5.5.dylib:arm64+0x152864)
    #1 QMetaObject::invokeMethodImpl(QObject*, QtPrivate::QSlotObjectBase*, Qt::ConnectionType, void*) qmetaobject.cpp:1617 (libQt6Core_debug.6.5.5.dylib:arm64+0x18b420)
    #2 QTimer::singleShotImpl(int, Qt::TimerType, QObject const*, QtPrivate::QSlotObjectBase*) qtimer.cpp:350 (libQt6Core_debug.6.5.5.dylib:arm64+0x2b5884)
    #3 void QTimer::singleShot<int, void (MyObject::*)()>(int, Qt::TimerType, QtPrivate::FunctionPointer<void (MyObject::*)()>::Object const*, void (MyObject::*)()) qtimer.h:89 (hello_world_app:arm64+0x1000074e0)
    #4 void QTimer::singleShot<int, void (MyObject::*)()>(int, QtPrivate::FunctionPointer<void (MyObject::*)()>::Object const*, void (MyObject::*)()) qtimer.h:76 (hello_world_app:arm64+0x1000073f4)
    #5 main::$_0::operator()() const main.cpp:34 (hello_world_app:arm64+0x100007318)
    #6 decltype(std::declval<main::$_0>()()) std::__1::__invoke[abi:ne180100]<main::$_0>(main::$_0&&) invoke.h:344 (hello_world_app:arm64+0x10000723c)
    #7 void std::__1::__thread_execute[abi:ne180100]<std::__1::unique_ptr<std::__1::__thread_struct, std::__1::default_delete<std::__1::__thread_struct>>, main::$_0>(std::__1::tuple<std::__1::unique_ptr<std::__1::__thread_struct, std::__1::default_delete<std::__1::__thread_struct>>, main::$_0>&, std::__1::__tuple_indices<>) thread.h:199 (hello_world_app:arm64+0x100007168)
    #8 void* std::__1::__thread_proxy[abi:ne180100]<std::__1::tuple<std::__1::unique_ptr<std::__1::__thread_struct, std::__1::default_delete<std::__1::__thread_struct>>, main::$_0>>(void*) thread.h:208 (hello_world_app:arm64+0x10000651c)

  Location is heap block of size 152 at 0x000106401220 allocated by main thread:
    #0 operator new(unsigned long) <null>:77602112 (libclang_rt.tsan_osx_dynamic.dylib:arm64e+0x84210)
    #1 QThreadData::current(bool) qthread_unix.cpp:176 (libQt6Core_debug.6.5.5.dylib:arm64+0x613488)
    #2 QThread::currentThread() qthread.cpp:407 (libQt6Core_debug.6.5.5.dylib:arm64+0x471c98)
    #3 QCoreApplicationPrivate::QCoreApplicationPrivate(int&, char**) qcoreapplication.cpp:456 (libQt6Core_debug.6.5.5.dylib:arm64+0x14cac0)
    #4 QCoreApplicationPrivate::QCoreApplicationPrivate(int&, char**) qcoreapplication.cpp:434 (libQt6Core_debug.6.5.5.dylib:arm64+0x14ccc4)
    #5 QCoreApplication::QCoreApplication(int&, char**, int) qcoreapplication.cpp:801 (libQt6Core_debug.6.5.5.dylib:arm64+0x14ede0)
    #6 QCoreApplication::QCoreApplication(int&, char**, int) qcoreapplication.cpp:803 (libQt6Core_debug.6.5.5.dylib:arm64+0x14f718)
    #7 main main.cpp:26 (hello_world_app:arm64+0x100003b78)

  Thread T8 (tid=66278561, running) created by main thread at:
    #0 pthread_create <null>:77602112 (libclang_rt.tsan_osx_dynamic.dylib:arm64e+0x309d8)
    #1 std::__1::__libcpp_thread_create[abi:ne180100](_opaque_pthread_t**, void* (*)(void*), void*) __threading_support:317 (hello_world_app:arm64+0x100006484)
    #2 std::__1::thread::thread<main::$_0, void>(main::$_0&&) thread.h:218 (hello_world_app:arm64+0x100006228)
    #3 std::__1::thread::thread<main::$_0, void>(main::$_0&&) thread.h:213 (hello_world_app:arm64+0x100003ecc)
    #4 main main.cpp:30 (hello_world_app:arm64+0x100003bc0)

  Thread T7 (tid=66278560, running) created by main thread at:
    #0 pthread_create <null>:77602112 (libclang_rt.tsan_osx_dynamic.dylib:arm64e+0x309d8)
    #1 std::__1::__libcpp_thread_create[abi:ne180100](_opaque_pthread_t**, void* (*)(void*), void*) __threading_support:317 (hello_world_app:arm64+0x100006484)
    #2 std::__1::thread::thread<main::$_0, void>(main::$_0&&) thread.h:218 (hello_world_app:arm64+0x100006228)
    #3 std::__1::thread::thread<main::$_0, void>(main::$_0&&) thread.h:213 (hello_world_app:arm64+0x100003ecc)
    #4 main main.cpp:30 (hello_world_app:arm64+0x100003bc0)

SUMMARY: ThreadSanitizer: data race qcoreapplication.cpp:1675 in QCoreApplication::postEvent(QObject*, QEvent*, int)
```

# qtbase diff

The only modifications made to qtbase are to turn off the TSAN surpressions

```
--- a/src/corelib/thread/qtsan_impl.h
+++ b/src/corelib/thread/qtsan_impl.h
@@ -35,22 +35,22 @@ inline void futexRelease(void *addr, void *addr2 = nullptr)
 
 inline void mutexPreLock(void *addr, unsigned flags)
 {
-    ::__tsan_mutex_pre_lock(addr, flags);
+    //::__tsan_mutex_pre_lock(addr, flags);
 }
 
 inline void mutexPostLock(void *addr, unsigned flags, int recursion)
 {
-    ::__tsan_mutex_post_lock(addr, flags, recursion);
+    //::__tsan_mutex_post_lock(addr, flags, recursion);
 }
 
 inline void mutexPreUnlock(void *addr, unsigned flags)
 {
-    ::__tsan_mutex_pre_unlock(addr, flags);
+    //::__tsan_mutex_pre_unlock(addr, flags);
 }
 
 inline void mutexPostUnlock(void *addr, unsigned flags)
 {
-    ::__tsan_mutex_post_unlock(addr, flags);
+    //::__tsan_mutex_post_unlock(addr, flags);
 }

```
