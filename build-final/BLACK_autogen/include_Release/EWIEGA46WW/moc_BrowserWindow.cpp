/****************************************************************************
** Meta object code from reading C++ file 'BrowserWindow.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.8.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../../BrowserWindow.h"
#include <QtGui/qtextcursor.h>
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'BrowserWindow.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 68
#error "This file was generated using the moc from 6.8.0. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

#ifndef Q_CONSTINIT
#define Q_CONSTINIT
#endif

QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
QT_WARNING_DISABLE_GCC("-Wuseless-cast")
namespace {

#ifdef QT_MOC_HAS_STRINGDATA
struct qt_meta_stringdata_CLASSBrowserWindowENDCLASS_t {};
constexpr auto qt_meta_stringdata_CLASSBrowserWindowENDCLASS = QtMocHelpers::stringData(
    "BrowserWindow",
    "navigateToUrl",
    "",
    "updateUrlBar",
    "url",
    "updateNavigationState",
    "onLoadStarted",
    "onLoadProgress",
    "progress",
    "onLoadFinished",
    "ok",
    "closeWindow",
    "minimizeWindow",
    "maximizeWindow",
    "shareAction",
    "addTabAction",
    "closeTab",
    "index",
    "setCurrentTab",
    "toggleSidebar",
    "toggleTabOverview",
    "showTabOverview",
    "hideTabOverview",
    "showSettingsMenu",
    "updateWebViewTheme"
);
#else  // !QT_MOC_HAS_STRINGDATA
#error "qtmochelpers.h not found or too old."
#endif // !QT_MOC_HAS_STRINGDATA
} // unnamed namespace

Q_CONSTINIT static const uint qt_meta_data_CLASSBrowserWindowENDCLASS[] = {

 // content:
      12,       // revision
       0,       // classname
       0,    0, // classinfo
      19,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: name, argc, parameters, tag, flags, initial metatype offsets
       1,    0,  128,    2, 0x08,    1 /* Private */,
       3,    1,  129,    2, 0x08,    2 /* Private */,
       5,    0,  132,    2, 0x08,    4 /* Private */,
       6,    0,  133,    2, 0x08,    5 /* Private */,
       7,    1,  134,    2, 0x08,    6 /* Private */,
       9,    1,  137,    2, 0x08,    8 /* Private */,
      11,    0,  140,    2, 0x08,   10 /* Private */,
      12,    0,  141,    2, 0x08,   11 /* Private */,
      13,    0,  142,    2, 0x08,   12 /* Private */,
      14,    0,  143,    2, 0x08,   13 /* Private */,
      15,    0,  144,    2, 0x08,   14 /* Private */,
      16,    1,  145,    2, 0x08,   15 /* Private */,
      18,    1,  148,    2, 0x08,   17 /* Private */,
      19,    0,  151,    2, 0x08,   19 /* Private */,
      20,    0,  152,    2, 0x08,   20 /* Private */,
      21,    0,  153,    2, 0x08,   21 /* Private */,
      22,    0,  154,    2, 0x08,   22 /* Private */,
      23,    0,  155,    2, 0x08,   23 /* Private */,
      24,    0,  156,    2, 0x08,   24 /* Private */,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void, QMetaType::QUrl,    4,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Int,    8,
    QMetaType::Void, QMetaType::Bool,   10,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Int,   17,
    QMetaType::Void, QMetaType::Int,   17,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,

       0        // eod
};

Q_CONSTINIT const QMetaObject BrowserWindow::staticMetaObject = { {
    QMetaObject::SuperData::link<QMainWindow::staticMetaObject>(),
    qt_meta_stringdata_CLASSBrowserWindowENDCLASS.offsetsAndSizes,
    qt_meta_data_CLASSBrowserWindowENDCLASS,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_stringdata_CLASSBrowserWindowENDCLASS_t,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<BrowserWindow, std::true_type>,
        // method 'navigateToUrl'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'updateUrlBar'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QUrl &, std::false_type>,
        // method 'updateNavigationState'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onLoadStarted'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onLoadProgress'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        // method 'onLoadFinished'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        // method 'closeWindow'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'minimizeWindow'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'maximizeWindow'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'shareAction'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'addTabAction'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'closeTab'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        // method 'setCurrentTab'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        // method 'toggleSidebar'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'toggleTabOverview'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'showTabOverview'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'hideTabOverview'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'showSettingsMenu'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'updateWebViewTheme'
        QtPrivate::TypeAndForceComplete<void, std::false_type>
    >,
    nullptr
} };

void BrowserWindow::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<BrowserWindow *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->navigateToUrl(); break;
        case 1: _t->updateUrlBar((*reinterpret_cast< std::add_pointer_t<QUrl>>(_a[1]))); break;
        case 2: _t->updateNavigationState(); break;
        case 3: _t->onLoadStarted(); break;
        case 4: _t->onLoadProgress((*reinterpret_cast< std::add_pointer_t<int>>(_a[1]))); break;
        case 5: _t->onLoadFinished((*reinterpret_cast< std::add_pointer_t<bool>>(_a[1]))); break;
        case 6: _t->closeWindow(); break;
        case 7: _t->minimizeWindow(); break;
        case 8: _t->maximizeWindow(); break;
        case 9: _t->shareAction(); break;
        case 10: _t->addTabAction(); break;
        case 11: _t->closeTab((*reinterpret_cast< std::add_pointer_t<int>>(_a[1]))); break;
        case 12: _t->setCurrentTab((*reinterpret_cast< std::add_pointer_t<int>>(_a[1]))); break;
        case 13: _t->toggleSidebar(); break;
        case 14: _t->toggleTabOverview(); break;
        case 15: _t->showTabOverview(); break;
        case 16: _t->hideTabOverview(); break;
        case 17: _t->showSettingsMenu(); break;
        case 18: _t->updateWebViewTheme(); break;
        default: ;
        }
    }
}

const QMetaObject *BrowserWindow::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *BrowserWindow::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_CLASSBrowserWindowENDCLASS.stringdata0))
        return static_cast<void*>(this);
    return QMainWindow::qt_metacast(_clname);
}

int BrowserWindow::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QMainWindow::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 19)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 19;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 19)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 19;
    }
    return _id;
}
QT_WARNING_POP
