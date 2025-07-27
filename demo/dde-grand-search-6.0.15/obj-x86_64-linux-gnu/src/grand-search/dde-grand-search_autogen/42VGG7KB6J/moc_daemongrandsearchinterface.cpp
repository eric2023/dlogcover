/****************************************************************************
** Meta object code from reading C++ file 'daemongrandsearchinterface.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.8.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../../../3rdparty/interfaces/daemongrandsearchinterface.h"
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'daemongrandsearchinterface.h' doesn't include <QObject>."
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
struct qt_meta_stringdata_CLASSGrandSearchSCOPEDaemonGrandSearchInterfaceENDCLASS_t {};
constexpr auto qt_meta_stringdata_CLASSGrandSearchSCOPEDaemonGrandSearchInterfaceENDCLASS = QtMocHelpers::stringData(
    "GrandSearch::DaemonGrandSearchInterface",
    "Matched",
    "",
    "session",
    "SearchCompleted",
    "Configuration",
    "QDBusPendingReply<QVariantMap>",
    "Configure",
    "QDBusPendingReply<bool>",
    "QVariantMap",
    "in0",
    "FeedBackStrategy",
    "KeepAlive",
    "MatchedBuffer",
    "QDBusPendingReply<QByteArray>",
    "MatchedResults",
    "OpenWithPlugin",
    "searcher",
    "item",
    "Search",
    "key",
    "SetFeedBackStrategy",
    "Terminate",
    "QDBusPendingReply<>"
);
#else  // !QT_MOC_HAS_STRINGDATA
#error "qtmochelpers.h not found or too old."
#endif // !QT_MOC_HAS_STRINGDATA
} // unnamed namespace

Q_CONSTINIT static const uint qt_meta_data_CLASSGrandSearchSCOPEDaemonGrandSearchInterfaceENDCLASS[] = {

 // content:
      12,       // revision
       0,       // classname
       0,    0, // classinfo
      12,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       2,       // signalCount

 // signals: name, argc, parameters, tag, flags, initial metatype offsets
       1,    1,   86,    2, 0x06,    1 /* Public */,
       4,    1,   89,    2, 0x06,    3 /* Public */,

 // slots: name, argc, parameters, tag, flags, initial metatype offsets
       5,    0,   92,    2, 0x0a,    5 /* Public */,
       7,    1,   93,    2, 0x0a,    6 /* Public */,
      11,    0,   96,    2, 0x0a,    8 /* Public */,
      12,    1,   97,    2, 0x0a,    9 /* Public */,
      13,    1,  100,    2, 0x0a,   11 /* Public */,
      15,    1,  103,    2, 0x0a,   13 /* Public */,
      16,    2,  106,    2, 0x0a,   15 /* Public */,
      19,    2,  111,    2, 0x0a,   18 /* Public */,
      21,    1,  116,    2, 0x0a,   21 /* Public */,
      22,    0,  119,    2, 0x0a,   23 /* Public */,

 // signals: parameters
    QMetaType::Void, QMetaType::QString,    3,
    QMetaType::Void, QMetaType::QString,    3,

 // slots: parameters
    0x80000000 | 6,
    0x80000000 | 8, 0x80000000 | 9,   10,
    0x80000000 | 6,
    0x80000000 | 8, QMetaType::QString,    3,
    0x80000000 | 14, QMetaType::QString,    3,
    0x80000000 | 14, QMetaType::QString,    3,
    0x80000000 | 8, QMetaType::QString, QMetaType::QString,   17,   18,
    0x80000000 | 8, QMetaType::QString, QMetaType::QString,    3,   20,
    0x80000000 | 8, 0x80000000 | 9,   10,
    0x80000000 | 23,

       0        // eod
};

Q_CONSTINIT const QMetaObject GrandSearch::DaemonGrandSearchInterface::staticMetaObject = { {
    QMetaObject::SuperData::link<QDBusAbstractInterface::staticMetaObject>(),
    qt_meta_stringdata_CLASSGrandSearchSCOPEDaemonGrandSearchInterfaceENDCLASS.offsetsAndSizes,
    qt_meta_data_CLASSGrandSearchSCOPEDaemonGrandSearchInterfaceENDCLASS,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_stringdata_CLASSGrandSearchSCOPEDaemonGrandSearchInterfaceENDCLASS_t,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<DaemonGrandSearchInterface, std::true_type>,
        // method 'Matched'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'SearchCompleted'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'Configuration'
        QtPrivate::TypeAndForceComplete<QDBusPendingReply<QVariantMap>, std::false_type>,
        // method 'Configure'
        QtPrivate::TypeAndForceComplete<QDBusPendingReply<bool>, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QVariantMap &, std::false_type>,
        // method 'FeedBackStrategy'
        QtPrivate::TypeAndForceComplete<QDBusPendingReply<QVariantMap>, std::false_type>,
        // method 'KeepAlive'
        QtPrivate::TypeAndForceComplete<QDBusPendingReply<bool>, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'MatchedBuffer'
        QtPrivate::TypeAndForceComplete<QDBusPendingReply<QByteArray>, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'MatchedResults'
        QtPrivate::TypeAndForceComplete<QDBusPendingReply<QByteArray>, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'OpenWithPlugin'
        QtPrivate::TypeAndForceComplete<QDBusPendingReply<bool>, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'Search'
        QtPrivate::TypeAndForceComplete<QDBusPendingReply<bool>, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'SetFeedBackStrategy'
        QtPrivate::TypeAndForceComplete<QDBusPendingReply<bool>, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QVariantMap &, std::false_type>,
        // method 'Terminate'
        QtPrivate::TypeAndForceComplete<QDBusPendingReply<>, std::false_type>
    >,
    nullptr
} };

void GrandSearch::DaemonGrandSearchInterface::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<DaemonGrandSearchInterface *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->Matched((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 1: _t->SearchCompleted((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 2: { QDBusPendingReply<QVariantMap> _r = _t->Configuration();
            if (_a[0]) *reinterpret_cast< QDBusPendingReply<QVariantMap>*>(_a[0]) = std::move(_r); }  break;
        case 3: { QDBusPendingReply<bool> _r = _t->Configure((*reinterpret_cast< std::add_pointer_t<QVariantMap>>(_a[1])));
            if (_a[0]) *reinterpret_cast< QDBusPendingReply<bool>*>(_a[0]) = std::move(_r); }  break;
        case 4: { QDBusPendingReply<QVariantMap> _r = _t->FeedBackStrategy();
            if (_a[0]) *reinterpret_cast< QDBusPendingReply<QVariantMap>*>(_a[0]) = std::move(_r); }  break;
        case 5: { QDBusPendingReply<bool> _r = _t->KeepAlive((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])));
            if (_a[0]) *reinterpret_cast< QDBusPendingReply<bool>*>(_a[0]) = std::move(_r); }  break;
        case 6: { QDBusPendingReply<QByteArray> _r = _t->MatchedBuffer((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])));
            if (_a[0]) *reinterpret_cast< QDBusPendingReply<QByteArray>*>(_a[0]) = std::move(_r); }  break;
        case 7: { QDBusPendingReply<QByteArray> _r = _t->MatchedResults((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])));
            if (_a[0]) *reinterpret_cast< QDBusPendingReply<QByteArray>*>(_a[0]) = std::move(_r); }  break;
        case 8: { QDBusPendingReply<bool> _r = _t->OpenWithPlugin((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2])));
            if (_a[0]) *reinterpret_cast< QDBusPendingReply<bool>*>(_a[0]) = std::move(_r); }  break;
        case 9: { QDBusPendingReply<bool> _r = _t->Search((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2])));
            if (_a[0]) *reinterpret_cast< QDBusPendingReply<bool>*>(_a[0]) = std::move(_r); }  break;
        case 10: { QDBusPendingReply<bool> _r = _t->SetFeedBackStrategy((*reinterpret_cast< std::add_pointer_t<QVariantMap>>(_a[1])));
            if (_a[0]) *reinterpret_cast< QDBusPendingReply<bool>*>(_a[0]) = std::move(_r); }  break;
        case 11: { QDBusPendingReply<> _r = _t->Terminate();
            if (_a[0]) *reinterpret_cast< QDBusPendingReply<>*>(_a[0]) = std::move(_r); }  break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (DaemonGrandSearchInterface::*)(const QString & );
            if (_t _q_method = &DaemonGrandSearchInterface::Matched; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (DaemonGrandSearchInterface::*)(const QString & );
            if (_t _q_method = &DaemonGrandSearchInterface::SearchCompleted; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 1;
                return;
            }
        }
    }
}

const QMetaObject *GrandSearch::DaemonGrandSearchInterface::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *GrandSearch::DaemonGrandSearchInterface::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_CLASSGrandSearchSCOPEDaemonGrandSearchInterfaceENDCLASS.stringdata0))
        return static_cast<void*>(this);
    return QDBusAbstractInterface::qt_metacast(_clname);
}

int GrandSearch::DaemonGrandSearchInterface::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QDBusAbstractInterface::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 12)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 12;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 12)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 12;
    }
    return _id;
}

// SIGNAL 0
void GrandSearch::DaemonGrandSearchInterface::Matched(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void GrandSearch::DaemonGrandSearchInterface::SearchCompleted(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}
QT_WARNING_POP
