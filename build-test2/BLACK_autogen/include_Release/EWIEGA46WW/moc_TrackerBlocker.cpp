/****************************************************************************
** Meta object code from reading C++ file 'TrackerBlocker.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.8.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../../TrackerBlocker.h"
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'TrackerBlocker.h' doesn't include <QObject>."
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
struct qt_meta_stringdata_CLASSTrackerBlockerENDCLASS_t {};
constexpr auto qt_meta_stringdata_CLASSTrackerBlockerENDCLASS = QtMocHelpers::stringData(
    "TrackerBlocker",
    "privacyChanged",
    "",
    "trackerBreakdownJson",
    "trackersBlockedToday",
    "trackersBlockedThisWeek",
    "trackersBlockedLast30Days",
    "mostContactedTracker",
    "mostContactedTrackerSites",
    "websitesContactedTrackers",
    "websitesVisited"
);
#else  // !QT_MOC_HAS_STRINGDATA
#error "qtmochelpers.h not found or too old."
#endif // !QT_MOC_HAS_STRINGDATA
} // unnamed namespace

Q_CONSTINIT static const uint qt_meta_data_CLASSTrackerBlockerENDCLASS[] = {

 // content:
      12,       // revision
       0,       // classname
       0,    0, // classinfo
       2,   14, // methods
       7,   28, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       1,       // signalCount

 // signals: name, argc, parameters, tag, flags, initial metatype offsets
       1,    0,   26,    2, 0x06,    8 /* Public */,

 // methods: name, argc, parameters, tag, flags, initial metatype offsets
       3,    0,   27,    2, 0x102,    9 /* Public | MethodIsConst  */,

 // signals: parameters
    QMetaType::Void,

 // methods: parameters
    QMetaType::QString,

 // properties: name, type, flags, notifyId, revision
       4, QMetaType::Int, 0x00015001, uint(0), 0,
       5, QMetaType::Int, 0x00015001, uint(0), 0,
       6, QMetaType::Int, 0x00015001, uint(0), 0,
       7, QMetaType::QString, 0x00015001, uint(0), 0,
       8, QMetaType::Int, 0x00015001, uint(0), 0,
       9, QMetaType::Int, 0x00015001, uint(0), 0,
      10, QMetaType::Int, 0x00015001, uint(0), 0,

       0        // eod
};

Q_CONSTINIT const QMetaObject TrackerBlocker::staticMetaObject = { {
    QMetaObject::SuperData::link<QWebEngineUrlRequestInterceptor::staticMetaObject>(),
    qt_meta_stringdata_CLASSTrackerBlockerENDCLASS.offsetsAndSizes,
    qt_meta_data_CLASSTrackerBlockerENDCLASS,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_stringdata_CLASSTrackerBlockerENDCLASS_t,
        // property 'trackersBlockedToday'
        QtPrivate::TypeAndForceComplete<int, std::true_type>,
        // property 'trackersBlockedThisWeek'
        QtPrivate::TypeAndForceComplete<int, std::true_type>,
        // property 'trackersBlockedLast30Days'
        QtPrivate::TypeAndForceComplete<int, std::true_type>,
        // property 'mostContactedTracker'
        QtPrivate::TypeAndForceComplete<QString, std::true_type>,
        // property 'mostContactedTrackerSites'
        QtPrivate::TypeAndForceComplete<int, std::true_type>,
        // property 'websitesContactedTrackers'
        QtPrivate::TypeAndForceComplete<int, std::true_type>,
        // property 'websitesVisited'
        QtPrivate::TypeAndForceComplete<int, std::true_type>,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<TrackerBlocker, std::true_type>,
        // method 'privacyChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'trackerBreakdownJson'
        QtPrivate::TypeAndForceComplete<QString, std::false_type>
    >,
    nullptr
} };

void TrackerBlocker::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<TrackerBlocker *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->privacyChanged(); break;
        case 1: { QString _r = _t->trackerBreakdownJson();
            if (_a[0]) *reinterpret_cast< QString*>(_a[0]) = std::move(_r); }  break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (TrackerBlocker::*)();
            if (_t _q_method = &TrackerBlocker::privacyChanged; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 0;
                return;
            }
        }
    } else if (_c == QMetaObject::ReadProperty) {
        auto *_t = static_cast<TrackerBlocker *>(_o);
        (void)_t;
        void *_v = _a[0];
        switch (_id) {
        case 0: *reinterpret_cast< int*>(_v) = _t->trackersBlockedToday(); break;
        case 1: *reinterpret_cast< int*>(_v) = _t->trackersBlockedThisWeek(); break;
        case 2: *reinterpret_cast< int*>(_v) = _t->trackersBlockedLast30Days(); break;
        case 3: *reinterpret_cast< QString*>(_v) = _t->mostContactedTracker(); break;
        case 4: *reinterpret_cast< int*>(_v) = _t->mostContactedTrackerSites(); break;
        case 5: *reinterpret_cast< int*>(_v) = _t->websitesContactedTrackers(); break;
        case 6: *reinterpret_cast< int*>(_v) = _t->websitesVisited(); break;
        default: break;
        }
    } else if (_c == QMetaObject::WriteProperty) {
    } else if (_c == QMetaObject::ResetProperty) {
    } else if (_c == QMetaObject::BindableProperty) {
    }
}

const QMetaObject *TrackerBlocker::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *TrackerBlocker::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_CLASSTrackerBlockerENDCLASS.stringdata0))
        return static_cast<void*>(this);
    return QWebEngineUrlRequestInterceptor::qt_metacast(_clname);
}

int TrackerBlocker::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWebEngineUrlRequestInterceptor::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 2)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 2;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 2)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 2;
    }else if (_c == QMetaObject::ReadProperty || _c == QMetaObject::WriteProperty
            || _c == QMetaObject::ResetProperty || _c == QMetaObject::BindableProperty
            || _c == QMetaObject::RegisterPropertyMetaType) {
        qt_static_metacall(this, _c, _id, _a);
        _id -= 7;
    }
    return _id;
}

// SIGNAL 0
void TrackerBlocker::privacyChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}
QT_WARNING_POP
