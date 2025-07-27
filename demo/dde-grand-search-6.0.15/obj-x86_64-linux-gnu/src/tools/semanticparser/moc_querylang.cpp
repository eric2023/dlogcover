/****************************************************************************
** Meta object code from reading C++ file 'querylang.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.8.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "querylang.h"
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'querylang.h' doesn't include <QObject>."
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
struct qt_meta_stringdata_CLASSOrgDeepinAiDaemonQueryLangInterfaceENDCLASS_t {};
constexpr auto qt_meta_stringdata_CLASSOrgDeepinAiDaemonQueryLangInterfaceENDCLASS = QtMocHelpers::stringData(
    "OrgDeepinAiDaemonQueryLangInterface",
    "Enable",
    "QDBusPendingReply<bool>",
    "",
    "Query",
    "QDBusPendingReply<QString>",
    "content"
);
#else  // !QT_MOC_HAS_STRINGDATA
#error "qtmochelpers.h not found or too old."
#endif // !QT_MOC_HAS_STRINGDATA
} // unnamed namespace

Q_CONSTINIT static const uint qt_meta_data_CLASSOrgDeepinAiDaemonQueryLangInterfaceENDCLASS[] = {

 // content:
      12,       // revision
       0,       // classname
       0,    0, // classinfo
       2,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: name, argc, parameters, tag, flags, initial metatype offsets
       1,    0,   26,    3, 0x0a,    1 /* Public */,
       4,    1,   27,    3, 0x0a,    2 /* Public */,

 // slots: parameters
    0x80000000 | 2,
    0x80000000 | 5, QMetaType::QString,    6,

       0        // eod
};

Q_CONSTINIT const QMetaObject OrgDeepinAiDaemonQueryLangInterface::staticMetaObject = { {
    QMetaObject::SuperData::link<QDBusAbstractInterface::staticMetaObject>(),
    qt_meta_stringdata_CLASSOrgDeepinAiDaemonQueryLangInterfaceENDCLASS.offsetsAndSizes,
    qt_meta_data_CLASSOrgDeepinAiDaemonQueryLangInterfaceENDCLASS,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_stringdata_CLASSOrgDeepinAiDaemonQueryLangInterfaceENDCLASS_t,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<OrgDeepinAiDaemonQueryLangInterface, std::true_type>,
        // method 'Enable'
        QtPrivate::TypeAndForceComplete<QDBusPendingReply<bool>, std::false_type>,
        // method 'Query'
        QtPrivate::TypeAndForceComplete<QDBusPendingReply<QString>, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>
    >,
    nullptr
} };

void OrgDeepinAiDaemonQueryLangInterface::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<OrgDeepinAiDaemonQueryLangInterface *>(_o);
        (void)_t;
        switch (_id) {
        case 0: { QDBusPendingReply<bool> _r = _t->Enable();
            if (_a[0]) *reinterpret_cast< QDBusPendingReply<bool>*>(_a[0]) = std::move(_r); }  break;
        case 1: { QDBusPendingReply<QString> _r = _t->Query((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])));
            if (_a[0]) *reinterpret_cast< QDBusPendingReply<QString>*>(_a[0]) = std::move(_r); }  break;
        default: ;
        }
    }
}

const QMetaObject *OrgDeepinAiDaemonQueryLangInterface::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *OrgDeepinAiDaemonQueryLangInterface::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_CLASSOrgDeepinAiDaemonQueryLangInterfaceENDCLASS.stringdata0))
        return static_cast<void*>(this);
    return QDBusAbstractInterface::qt_metacast(_clname);
}

int OrgDeepinAiDaemonQueryLangInterface::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QDBusAbstractInterface::qt_metacall(_c, _id, _a);
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
    }
    return _id;
}
QT_WARNING_POP
