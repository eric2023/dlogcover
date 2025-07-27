/****************************************************************************
** Meta object code from reading C++ file 'searchitem.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.8.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../../../src/grand-search-shell-plugin/searchitem.h"
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'searchitem.h' doesn't include <QObject>."
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
struct qt_meta_stringdata_CLASSdockSCOPESearchItemENDCLASS_t {};
constexpr auto qt_meta_stringdata_CLASSdockSCOPESearchItemENDCLASS = QtMocHelpers::stringData(
    "dock::SearchItem",
    "visibleChanged",
    "",
    "grandSearchVisibleChanged",
    "onGrandSearchVisibleChanged",
    "toggleGrandSearch",
    "toggleGrandSearchConfig",
    "setVisible",
    "visible",
    "grandSearchVisible"
);
#else  // !QT_MOC_HAS_STRINGDATA
#error "qtmochelpers.h not found or too old."
#endif // !QT_MOC_HAS_STRINGDATA
} // unnamed namespace

Q_CONSTINIT static const uint qt_meta_data_CLASSdockSCOPESearchItemENDCLASS[] = {

 // content:
      12,       // revision
       0,       // classname
       0,    0, // classinfo
       7,   14, // methods
       2,   71, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       2,       // signalCount

 // signals: name, argc, parameters, tag, flags, initial metatype offsets
       1,    1,   56,    2, 0x06,    3 /* Public */,
       3,    1,   59,    2, 0x06,    5 /* Public */,

 // slots: name, argc, parameters, tag, flags, initial metatype offsets
       4,    1,   62,    2, 0x08,    7 /* Private */,

 // methods: name, argc, parameters, tag, flags, initial metatype offsets
       5,    0,   65,    2, 0x02,    9 /* Public */,
       6,    0,   66,    2, 0x02,   10 /* Public */,
       7,    1,   67,    2, 0x02,   11 /* Public */,
       9,    0,   70,    2, 0x102,   13 /* Public | MethodIsConst  */,

 // signals: parameters
    QMetaType::Void, QMetaType::Bool,    2,
    QMetaType::Void, QMetaType::Bool,    2,

 // slots: parameters
    QMetaType::Void, QMetaType::Bool,    2,

 // methods: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Bool,    8,
    QMetaType::Bool,

 // properties: name, type, flags, notifyId, revision
       8, QMetaType::Bool, 0x00015103, uint(0), 0,
       9, QMetaType::Bool, 0x00015001, uint(1), 0,

       0        // eod
};

Q_CONSTINIT const QMetaObject dock::SearchItem::staticMetaObject = { {
    QMetaObject::SuperData::link<ds::DApplet::staticMetaObject>(),
    qt_meta_stringdata_CLASSdockSCOPESearchItemENDCLASS.offsetsAndSizes,
    qt_meta_data_CLASSdockSCOPESearchItemENDCLASS,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_stringdata_CLASSdockSCOPESearchItemENDCLASS_t,
        // property 'visible'
        QtPrivate::TypeAndForceComplete<bool, std::true_type>,
        // property 'grandSearchVisible'
        QtPrivate::TypeAndForceComplete<bool, std::true_type>,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<SearchItem, std::true_type>,
        // method 'visibleChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        // method 'grandSearchVisibleChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        // method 'onGrandSearchVisibleChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        // method 'toggleGrandSearch'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'toggleGrandSearchConfig'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'setVisible'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        // method 'grandSearchVisible'
        QtPrivate::TypeAndForceComplete<bool, std::false_type>
    >,
    nullptr
} };

void dock::SearchItem::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<SearchItem *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->visibleChanged((*reinterpret_cast< std::add_pointer_t<bool>>(_a[1]))); break;
        case 1: _t->grandSearchVisibleChanged((*reinterpret_cast< std::add_pointer_t<bool>>(_a[1]))); break;
        case 2: _t->onGrandSearchVisibleChanged((*reinterpret_cast< std::add_pointer_t<bool>>(_a[1]))); break;
        case 3: _t->toggleGrandSearch(); break;
        case 4: _t->toggleGrandSearchConfig(); break;
        case 5: _t->setVisible((*reinterpret_cast< std::add_pointer_t<bool>>(_a[1]))); break;
        case 6: { bool _r = _t->grandSearchVisible();
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = std::move(_r); }  break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (SearchItem::*)(bool );
            if (_t _q_method = &SearchItem::visibleChanged; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (SearchItem::*)(bool );
            if (_t _q_method = &SearchItem::grandSearchVisibleChanged; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 1;
                return;
            }
        }
    } else if (_c == QMetaObject::ReadProperty) {
        auto *_t = static_cast<SearchItem *>(_o);
        (void)_t;
        void *_v = _a[0];
        switch (_id) {
        case 0: *reinterpret_cast< bool*>(_v) = _t->visible(); break;
        case 1: *reinterpret_cast< bool*>(_v) = _t->grandSearchVisible(); break;
        default: break;
        }
    } else if (_c == QMetaObject::WriteProperty) {
        auto *_t = static_cast<SearchItem *>(_o);
        (void)_t;
        void *_v = _a[0];
        switch (_id) {
        case 0: _t->setVisible(*reinterpret_cast< bool*>(_v)); break;
        default: break;
        }
    } else if (_c == QMetaObject::ResetProperty) {
    } else if (_c == QMetaObject::BindableProperty) {
    }
}

const QMetaObject *dock::SearchItem::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *dock::SearchItem::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_CLASSdockSCOPESearchItemENDCLASS.stringdata0))
        return static_cast<void*>(this);
    return ds::DApplet::qt_metacast(_clname);
}

int dock::SearchItem::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = ds::DApplet::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 7)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 7;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 7)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 7;
    }else if (_c == QMetaObject::ReadProperty || _c == QMetaObject::WriteProperty
            || _c == QMetaObject::ResetProperty || _c == QMetaObject::BindableProperty
            || _c == QMetaObject::RegisterPropertyMetaType) {
        qt_static_metacall(this, _c, _id, _a);
        _id -= 2;
    }
    return _id;
}

// SIGNAL 0
void dock::SearchItem::visibleChanged(bool _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void dock::SearchItem::grandSearchVisibleChanged(bool _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}
QT_WARNING_POP
