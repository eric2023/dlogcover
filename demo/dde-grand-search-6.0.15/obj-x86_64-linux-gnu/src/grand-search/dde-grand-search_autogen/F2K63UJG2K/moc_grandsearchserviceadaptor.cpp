/****************************************************************************
** Meta object code from reading C++ file 'grandsearchserviceadaptor.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.8.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../../../3rdparty/services/grandsearchserviceadaptor.h"
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'grandsearchserviceadaptor.h' doesn't include <QObject>."
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
struct qt_meta_stringdata_CLASSGrandSearchSCOPEGrandSearchServiceAdaptorENDCLASS_t {};
constexpr auto qt_meta_stringdata_CLASSGrandSearchSCOPEGrandSearchServiceAdaptorENDCLASS = QtMocHelpers::stringData(
    "GrandSearch::GrandSearchServiceAdaptor",
    "D-Bus Interface",
    "com.deepin.dde.GrandSearch",
    "D-Bus Introspection",
    "  <interface name=\"com.deepin.dde.GrandSearch\">\n    <signal name=\""
    "VisibleChanged\">\n      <arg direction=\"out\" type=\"b\" name=\"visi"
    "ble\"/>\n    </signal>\n    <method name=\"IsVisible\">\n      <arg di"
    "rection=\"out\" type=\"b\"/>\n    </method>\n    <method name=\"SetVis"
    "ible\">\n      <arg direction=\"in\" type=\"b\" name=\"visible\"/>\n  "
    "  </method>\n  </interface>\n",
    "VisibleChanged",
    "",
    "visible",
    "IsVisible",
    "SetVisible"
);
#else  // !QT_MOC_HAS_STRINGDATA
#error "qtmochelpers.h not found or too old."
#endif // !QT_MOC_HAS_STRINGDATA
} // unnamed namespace

Q_CONSTINIT static const uint qt_meta_data_CLASSGrandSearchSCOPEGrandSearchServiceAdaptorENDCLASS[] = {

 // content:
      12,       // revision
       0,       // classname
       2,   14, // classinfo
       3,   18, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       1,       // signalCount

 // classinfo: key, value
       1,    2,
       3,    4,

 // signals: name, argc, parameters, tag, flags, initial metatype offsets
       5,    1,   36,    6, 0x06,    1 /* Public */,

 // slots: name, argc, parameters, tag, flags, initial metatype offsets
       8,    0,   39,    6, 0x0a,    3 /* Public */,
       9,    1,   40,    6, 0x0a,    4 /* Public */,

 // signals: parameters
    QMetaType::Void, QMetaType::Bool,    7,

 // slots: parameters
    QMetaType::Bool,
    QMetaType::Void, QMetaType::Bool,    7,

       0        // eod
};

Q_CONSTINIT const QMetaObject GrandSearch::GrandSearchServiceAdaptor::staticMetaObject = { {
    QMetaObject::SuperData::link<QDBusAbstractAdaptor::staticMetaObject>(),
    qt_meta_stringdata_CLASSGrandSearchSCOPEGrandSearchServiceAdaptorENDCLASS.offsetsAndSizes,
    qt_meta_data_CLASSGrandSearchSCOPEGrandSearchServiceAdaptorENDCLASS,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_stringdata_CLASSGrandSearchSCOPEGrandSearchServiceAdaptorENDCLASS_t,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<GrandSearchServiceAdaptor, std::true_type>,
        // method 'VisibleChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        // method 'IsVisible'
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        // method 'SetVisible'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<bool, std::false_type>
    >,
    nullptr
} };

void GrandSearch::GrandSearchServiceAdaptor::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<GrandSearchServiceAdaptor *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->VisibleChanged((*reinterpret_cast< std::add_pointer_t<bool>>(_a[1]))); break;
        case 1: { bool _r = _t->IsVisible();
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = std::move(_r); }  break;
        case 2: _t->SetVisible((*reinterpret_cast< std::add_pointer_t<bool>>(_a[1]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (GrandSearchServiceAdaptor::*)(bool );
            if (_t _q_method = &GrandSearchServiceAdaptor::VisibleChanged; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 0;
                return;
            }
        }
    }
}

const QMetaObject *GrandSearch::GrandSearchServiceAdaptor::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *GrandSearch::GrandSearchServiceAdaptor::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_CLASSGrandSearchSCOPEGrandSearchServiceAdaptorENDCLASS.stringdata0))
        return static_cast<void*>(this);
    return QDBusAbstractAdaptor::qt_metacast(_clname);
}

int GrandSearch::GrandSearchServiceAdaptor::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QDBusAbstractAdaptor::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 3)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 3;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 3)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 3;
    }
    return _id;
}

// SIGNAL 0
void GrandSearch::GrandSearchServiceAdaptor::VisibleChanged(bool _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}
QT_WARNING_POP
