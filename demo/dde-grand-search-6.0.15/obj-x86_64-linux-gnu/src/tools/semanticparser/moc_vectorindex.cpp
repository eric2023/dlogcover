/****************************************************************************
** Meta object code from reading C++ file 'vectorindex.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.8.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "vectorindex.h"
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'vectorindex.h' doesn't include <QObject>."
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
struct qt_meta_stringdata_CLASSOrgDeepinAiDaemonVectorIndexInterfaceENDCLASS_t {};
constexpr auto qt_meta_stringdata_CLASSOrgDeepinAiDaemonVectorIndexInterfaceENDCLASS = QtMocHelpers::stringData(
    "OrgDeepinAiDaemonVectorIndexInterface",
    "IndexDeleted",
    "",
    "appID",
    "files",
    "IndexStatus",
    "status",
    "Create",
    "QDBusPendingReply<bool>",
    "Delete",
    "DocFiles",
    "QDBusPendingReply<QStringList>",
    "Enable",
    "Search",
    "QDBusPendingReply<QString>",
    "query",
    "topK",
    "getAutoIndexStatus",
    "setAutoIndex",
    "QDBusPendingReply<>",
    "on"
);
#else  // !QT_MOC_HAS_STRINGDATA
#error "qtmochelpers.h not found or too old."
#endif // !QT_MOC_HAS_STRINGDATA
} // unnamed namespace

Q_CONSTINIT static const uint qt_meta_data_CLASSOrgDeepinAiDaemonVectorIndexInterfaceENDCLASS[] = {

 // content:
      12,       // revision
       0,       // classname
       0,    0, // classinfo
       9,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       2,       // signalCount

 // signals: name, argc, parameters, tag, flags, initial metatype offsets
       1,    2,   68,    2, 0x06,    1 /* Public */,
       5,    3,   73,    2, 0x06,    4 /* Public */,

 // slots: name, argc, parameters, tag, flags, initial metatype offsets
       7,    2,   80,    2, 0x0a,    8 /* Public */,
       9,    2,   85,    2, 0x0a,   11 /* Public */,
      10,    1,   90,    2, 0x0a,   14 /* Public */,
      12,    0,   93,    2, 0x0a,   16 /* Public */,
      13,    3,   94,    2, 0x0a,   17 /* Public */,
      17,    1,  101,    2, 0x0a,   21 /* Public */,
      18,    2,  104,    2, 0x0a,   23 /* Public */,

 // signals: parameters
    QMetaType::Void, QMetaType::QString, QMetaType::QStringList,    3,    4,
    QMetaType::Void, QMetaType::QString, QMetaType::QStringList, QMetaType::Int,    3,    4,    6,

 // slots: parameters
    0x80000000 | 8, QMetaType::QString, QMetaType::QStringList,    3,    4,
    0x80000000 | 8, QMetaType::QString, QMetaType::QStringList,    3,    4,
    0x80000000 | 11, QMetaType::QString,    3,
    0x80000000 | 8,
    0x80000000 | 14, QMetaType::QString, QMetaType::QString, QMetaType::Int,    3,   15,   16,
    0x80000000 | 14, QMetaType::QString,    3,
    0x80000000 | 19, QMetaType::QString, QMetaType::Bool,    3,   20,

       0        // eod
};

Q_CONSTINIT const QMetaObject OrgDeepinAiDaemonVectorIndexInterface::staticMetaObject = { {
    QMetaObject::SuperData::link<QDBusAbstractInterface::staticMetaObject>(),
    qt_meta_stringdata_CLASSOrgDeepinAiDaemonVectorIndexInterfaceENDCLASS.offsetsAndSizes,
    qt_meta_data_CLASSOrgDeepinAiDaemonVectorIndexInterfaceENDCLASS,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_stringdata_CLASSOrgDeepinAiDaemonVectorIndexInterfaceENDCLASS_t,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<OrgDeepinAiDaemonVectorIndexInterface, std::true_type>,
        // method 'IndexDeleted'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QStringList &, std::false_type>,
        // method 'IndexStatus'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QStringList &, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        // method 'Create'
        QtPrivate::TypeAndForceComplete<QDBusPendingReply<bool>, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QStringList &, std::false_type>,
        // method 'Delete'
        QtPrivate::TypeAndForceComplete<QDBusPendingReply<bool>, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QStringList &, std::false_type>,
        // method 'DocFiles'
        QtPrivate::TypeAndForceComplete<QDBusPendingReply<QStringList>, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'Enable'
        QtPrivate::TypeAndForceComplete<QDBusPendingReply<bool>, std::false_type>,
        // method 'Search'
        QtPrivate::TypeAndForceComplete<QDBusPendingReply<QString>, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        // method 'getAutoIndexStatus'
        QtPrivate::TypeAndForceComplete<QDBusPendingReply<QString>, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'setAutoIndex'
        QtPrivate::TypeAndForceComplete<QDBusPendingReply<>, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<bool, std::false_type>
    >,
    nullptr
} };

void OrgDeepinAiDaemonVectorIndexInterface::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<OrgDeepinAiDaemonVectorIndexInterface *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->IndexDeleted((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QStringList>>(_a[2]))); break;
        case 1: _t->IndexStatus((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QStringList>>(_a[2])),(*reinterpret_cast< std::add_pointer_t<int>>(_a[3]))); break;
        case 2: { QDBusPendingReply<bool> _r = _t->Create((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QStringList>>(_a[2])));
            if (_a[0]) *reinterpret_cast< QDBusPendingReply<bool>*>(_a[0]) = std::move(_r); }  break;
        case 3: { QDBusPendingReply<bool> _r = _t->Delete((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QStringList>>(_a[2])));
            if (_a[0]) *reinterpret_cast< QDBusPendingReply<bool>*>(_a[0]) = std::move(_r); }  break;
        case 4: { QDBusPendingReply<QStringList> _r = _t->DocFiles((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])));
            if (_a[0]) *reinterpret_cast< QDBusPendingReply<QStringList>*>(_a[0]) = std::move(_r); }  break;
        case 5: { QDBusPendingReply<bool> _r = _t->Enable();
            if (_a[0]) *reinterpret_cast< QDBusPendingReply<bool>*>(_a[0]) = std::move(_r); }  break;
        case 6: { QDBusPendingReply<QString> _r = _t->Search((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2])),(*reinterpret_cast< std::add_pointer_t<int>>(_a[3])));
            if (_a[0]) *reinterpret_cast< QDBusPendingReply<QString>*>(_a[0]) = std::move(_r); }  break;
        case 7: { QDBusPendingReply<QString> _r = _t->getAutoIndexStatus((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])));
            if (_a[0]) *reinterpret_cast< QDBusPendingReply<QString>*>(_a[0]) = std::move(_r); }  break;
        case 8: { QDBusPendingReply<> _r = _t->setAutoIndex((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<bool>>(_a[2])));
            if (_a[0]) *reinterpret_cast< QDBusPendingReply<>*>(_a[0]) = std::move(_r); }  break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (OrgDeepinAiDaemonVectorIndexInterface::*)(const QString & , const QStringList & );
            if (_t _q_method = &OrgDeepinAiDaemonVectorIndexInterface::IndexDeleted; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (OrgDeepinAiDaemonVectorIndexInterface::*)(const QString & , const QStringList & , int );
            if (_t _q_method = &OrgDeepinAiDaemonVectorIndexInterface::IndexStatus; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 1;
                return;
            }
        }
    }
}

const QMetaObject *OrgDeepinAiDaemonVectorIndexInterface::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *OrgDeepinAiDaemonVectorIndexInterface::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_CLASSOrgDeepinAiDaemonVectorIndexInterfaceENDCLASS.stringdata0))
        return static_cast<void*>(this);
    return QDBusAbstractInterface::qt_metacast(_clname);
}

int OrgDeepinAiDaemonVectorIndexInterface::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QDBusAbstractInterface::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 9)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 9;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 9)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 9;
    }
    return _id;
}

// SIGNAL 0
void OrgDeepinAiDaemonVectorIndexInterface::IndexDeleted(const QString & _t1, const QStringList & _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void OrgDeepinAiDaemonVectorIndexInterface::IndexStatus(const QString & _t1, const QStringList & _t2, int _t3)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t3))) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}
QT_WARNING_POP
