/****************************************************************************
** Meta object code from reading C++ file 'grandsearchinterface.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.8.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../../../src/dde-grand-search-daemon/dbusservice/grandsearchinterface.h"
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'grandsearchinterface.h' doesn't include <QObject>."
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
struct qt_meta_stringdata_CLASSGrandSearchSCOPEGrandSearchInterfaceENDCLASS_t {};
constexpr auto qt_meta_stringdata_CLASSGrandSearchSCOPEGrandSearchInterfaceENDCLASS = QtMocHelpers::stringData(
    "GrandSearch::GrandSearchInterface",
    "D-Bus Interface",
    "org.deepin.dde.GrandSearchDaemon",
    "Matched",
    "",
    "session",
    "SearchCompleted",
    "Search",
    "key",
    "Terminate",
    "MatchedResults",
    "MatchedBuffer",
    "OpenWithPlugin",
    "searcher",
    "item",
    "KeepAlive"
);
#else  // !QT_MOC_HAS_STRINGDATA
#error "qtmochelpers.h not found or too old."
#endif // !QT_MOC_HAS_STRINGDATA
} // unnamed namespace

Q_CONSTINIT static const uint qt_meta_data_CLASSGrandSearchSCOPEGrandSearchInterfaceENDCLASS[] = {

 // content:
      12,       // revision
       0,       // classname
       1,   14, // classinfo
       8,   16, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       2,       // signalCount

 // classinfo: key, value
       1,    2,

 // signals: name, argc, parameters, tag, flags, initial metatype offsets
       3,    1,   64,    4, 0x46,    1 /* Public | isScriptable */,
       6,    1,   67,    4, 0x46,    3 /* Public | isScriptable */,

 // slots: name, argc, parameters, tag, flags, initial metatype offsets
       7,    2,   70,    4, 0x4a,    5 /* Public | isScriptable */,
       9,    0,   75,    4, 0x4a,    8 /* Public | isScriptable */,
      10,    1,   76,    4, 0x4a,    9 /* Public | isScriptable */,
      11,    1,   79,    4, 0x4a,   11 /* Public | isScriptable */,
      12,    2,   82,    4, 0x4a,   13 /* Public | isScriptable */,
      15,    1,   87,    4, 0x4a,   16 /* Public | isScriptable */,

 // signals: parameters
    QMetaType::Void, QMetaType::QString,    5,
    QMetaType::Void, QMetaType::QString,    5,

 // slots: parameters
    QMetaType::Bool, QMetaType::QString, QMetaType::QString,    5,    8,
    QMetaType::Void,
    QMetaType::QByteArray, QMetaType::QString,    5,
    QMetaType::QByteArray, QMetaType::QString,    5,
    QMetaType::Bool, QMetaType::QString, QMetaType::QString,   13,   14,
    QMetaType::Bool, QMetaType::QString,    5,

       0        // eod
};

Q_CONSTINIT const QMetaObject GrandSearch::GrandSearchInterface::staticMetaObject = { {
    QMetaObject::SuperData::link<QDBusService::staticMetaObject>(),
    qt_meta_stringdata_CLASSGrandSearchSCOPEGrandSearchInterfaceENDCLASS.offsetsAndSizes,
    qt_meta_data_CLASSGrandSearchSCOPEGrandSearchInterfaceENDCLASS,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_stringdata_CLASSGrandSearchSCOPEGrandSearchInterfaceENDCLASS_t,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<GrandSearchInterface, std::true_type>,
        // method 'Matched'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'SearchCompleted'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'Search'
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'Terminate'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'MatchedResults'
        QtPrivate::TypeAndForceComplete<QByteArray, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'MatchedBuffer'
        QtPrivate::TypeAndForceComplete<QByteArray, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'OpenWithPlugin'
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'KeepAlive'
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>
    >,
    nullptr
} };

void GrandSearch::GrandSearchInterface::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<GrandSearchInterface *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->Matched((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 1: _t->SearchCompleted((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 2: { bool _r = _t->Search((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2])));
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = std::move(_r); }  break;
        case 3: _t->Terminate(); break;
        case 4: { QByteArray _r = _t->MatchedResults((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])));
            if (_a[0]) *reinterpret_cast< QByteArray*>(_a[0]) = std::move(_r); }  break;
        case 5: { QByteArray _r = _t->MatchedBuffer((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])));
            if (_a[0]) *reinterpret_cast< QByteArray*>(_a[0]) = std::move(_r); }  break;
        case 6: { bool _r = _t->OpenWithPlugin((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2])));
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = std::move(_r); }  break;
        case 7: { bool _r = _t->KeepAlive((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])));
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = std::move(_r); }  break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (GrandSearchInterface::*)(const QString & );
            if (_t _q_method = &GrandSearchInterface::Matched; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (GrandSearchInterface::*)(const QString & );
            if (_t _q_method = &GrandSearchInterface::SearchCompleted; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 1;
                return;
            }
        }
    }
}

const QMetaObject *GrandSearch::GrandSearchInterface::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *GrandSearch::GrandSearchInterface::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_CLASSGrandSearchSCOPEGrandSearchInterfaceENDCLASS.stringdata0))
        return static_cast<void*>(this);
    if (!strcmp(_clname, "QDBusContext"))
        return static_cast< QDBusContext*>(this);
    return QDBusService::qt_metacast(_clname);
}

int GrandSearch::GrandSearchInterface::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QDBusService::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 8)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 8;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 8)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 8;
    }
    return _id;
}

// SIGNAL 0
void GrandSearch::GrandSearchInterface::Matched(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void GrandSearch::GrandSearchInterface::SearchCompleted(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}
QT_WARNING_POP
