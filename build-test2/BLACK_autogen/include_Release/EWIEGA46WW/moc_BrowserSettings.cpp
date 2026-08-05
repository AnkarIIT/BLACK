/****************************************************************************
** Meta object code from reading C++ file 'BrowserSettings.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.8.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../../BrowserSettings.h"
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'BrowserSettings.h' doesn't include <QObject>."
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
struct qt_meta_stringdata_CLASSBrowserSettingsENDCLASS_t {};
constexpr auto qt_meta_stringdata_CLASSBrowserSettingsENDCLASS = QtMocHelpers::stringData(
    "BrowserSettings",
    "settingsChanged",
    "",
    "setValue",
    "key",
    "value",
    "setBool",
    "openDefaultBrowserSettings",
    "searchEngine",
    "newWindowsWith",
    "newTabsWith",
    "removeHistoryItems",
    "removeDownloadListItems",
    "openSafeFiles"
);
#else  // !QT_MOC_HAS_STRINGDATA
#error "qtmochelpers.h not found or too old."
#endif // !QT_MOC_HAS_STRINGDATA
} // unnamed namespace

Q_CONSTINIT static const uint qt_meta_data_CLASSBrowserSettingsENDCLASS[] = {

 // content:
      12,       // revision
       0,       // classname
       0,    0, // classinfo
       4,   14, // methods
       6,   50, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       1,       // signalCount

 // signals: name, argc, parameters, tag, flags, initial metatype offsets
       1,    0,   38,    2, 0x06,    7 /* Public */,

 // methods: name, argc, parameters, tag, flags, initial metatype offsets
       3,    2,   39,    2, 0x02,    8 /* Public */,
       6,    2,   44,    2, 0x02,   11 /* Public */,
       7,    0,   49,    2, 0x02,   14 /* Public */,

 // signals: parameters
    QMetaType::Void,

 // methods: parameters
    QMetaType::Void, QMetaType::QString, QMetaType::QString,    4,    5,
    QMetaType::Void, QMetaType::QString, QMetaType::Bool,    4,    5,
    QMetaType::Void,

 // properties: name, type, flags, notifyId, revision
       8, QMetaType::QString, 0x00015001, uint(0), 0,
       9, QMetaType::QString, 0x00015001, uint(0), 0,
      10, QMetaType::QString, 0x00015001, uint(0), 0,
      11, QMetaType::QString, 0x00015001, uint(0), 0,
      12, QMetaType::QString, 0x00015001, uint(0), 0,
      13, QMetaType::Bool, 0x00015001, uint(0), 0,

       0        // eod
};

Q_CONSTINIT const QMetaObject BrowserSettings::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_meta_stringdata_CLASSBrowserSettingsENDCLASS.offsetsAndSizes,
    qt_meta_data_CLASSBrowserSettingsENDCLASS,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_stringdata_CLASSBrowserSettingsENDCLASS_t,
        // property 'searchEngine'
        QtPrivate::TypeAndForceComplete<QString, std::true_type>,
        // property 'newWindowsWith'
        QtPrivate::TypeAndForceComplete<QString, std::true_type>,
        // property 'newTabsWith'
        QtPrivate::TypeAndForceComplete<QString, std::true_type>,
        // property 'removeHistoryItems'
        QtPrivate::TypeAndForceComplete<QString, std::true_type>,
        // property 'removeDownloadListItems'
        QtPrivate::TypeAndForceComplete<QString, std::true_type>,
        // property 'openSafeFiles'
        QtPrivate::TypeAndForceComplete<bool, std::true_type>,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<BrowserSettings, std::true_type>,
        // method 'settingsChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'setValue'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'setBool'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        // method 'openDefaultBrowserSettings'
        QtPrivate::TypeAndForceComplete<void, std::false_type>
    >,
    nullptr
} };

void BrowserSettings::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<BrowserSettings *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->settingsChanged(); break;
        case 1: _t->setValue((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2]))); break;
        case 2: _t->setBool((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<bool>>(_a[2]))); break;
        case 3: _t->openDefaultBrowserSettings(); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (BrowserSettings::*)();
            if (_t _q_method = &BrowserSettings::settingsChanged; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 0;
                return;
            }
        }
    } else if (_c == QMetaObject::ReadProperty) {
        auto *_t = static_cast<BrowserSettings *>(_o);
        (void)_t;
        void *_v = _a[0];
        switch (_id) {
        case 0: *reinterpret_cast< QString*>(_v) = _t->searchEngine(); break;
        case 1: *reinterpret_cast< QString*>(_v) = _t->newWindowsWith(); break;
        case 2: *reinterpret_cast< QString*>(_v) = _t->newTabsWith(); break;
        case 3: *reinterpret_cast< QString*>(_v) = _t->removeHistoryItems(); break;
        case 4: *reinterpret_cast< QString*>(_v) = _t->removeDownloadListItems(); break;
        case 5: *reinterpret_cast< bool*>(_v) = _t->openSafeFiles(); break;
        default: break;
        }
    } else if (_c == QMetaObject::WriteProperty) {
    } else if (_c == QMetaObject::ResetProperty) {
    } else if (_c == QMetaObject::BindableProperty) {
    }
}

const QMetaObject *BrowserSettings::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *BrowserSettings::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_CLASSBrowserSettingsENDCLASS.stringdata0))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int BrowserSettings::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 4)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 4;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 4)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 4;
    }else if (_c == QMetaObject::ReadProperty || _c == QMetaObject::WriteProperty
            || _c == QMetaObject::ResetProperty || _c == QMetaObject::BindableProperty
            || _c == QMetaObject::RegisterPropertyMetaType) {
        qt_static_metacall(this, _c, _id, _a);
        _id -= 6;
    }
    return _id;
}

// SIGNAL 0
void BrowserSettings::settingsChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}
QT_WARNING_POP
