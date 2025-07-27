/****************************************************************************
** Meta object code from reading C++ file 'entrancewidget.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.8.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../../../src/grand-search/gui/entrance/entrancewidget.h"
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'entrancewidget.h' doesn't include <QObject>."
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
struct qt_meta_stringdata_CLASSGrandSearchSCOPEEntranceWidgetENDCLASS_t {};
constexpr auto qt_meta_stringdata_CLASSGrandSearchSCOPEEntranceWidgetENDCLASS = QtMocHelpers::stringData(
    "GrandSearch::EntranceWidget",
    "searchTextChanged",
    "",
    "txt",
    "sigSelectNextItem",
    "sigSelectPreviousItem",
    "sigHandleItem",
    "sigCloseWindow",
    "onAppIconChanged",
    "searchGroupName",
    "MatchedItem",
    "item"
);
#else  // !QT_MOC_HAS_STRINGDATA
#error "qtmochelpers.h not found or too old."
#endif // !QT_MOC_HAS_STRINGDATA
} // unnamed namespace

Q_CONSTINIT static const uint qt_meta_data_CLASSGrandSearchSCOPEEntranceWidgetENDCLASS[] = {

 // content:
      12,       // revision
       0,       // classname
       0,    0, // classinfo
       6,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       5,       // signalCount

 // signals: name, argc, parameters, tag, flags, initial metatype offsets
       1,    1,   50,    2, 0x06,    1 /* Public */,
       4,    0,   53,    2, 0x06,    3 /* Public */,
       5,    0,   54,    2, 0x06,    4 /* Public */,
       6,    0,   55,    2, 0x06,    5 /* Public */,
       7,    0,   56,    2, 0x06,    6 /* Public */,

 // slots: name, argc, parameters, tag, flags, initial metatype offsets
       8,    2,   57,    2, 0x0a,    7 /* Public */,

 // signals: parameters
    QMetaType::Void, QMetaType::QString,    3,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,

 // slots: parameters
    QMetaType::Void, QMetaType::QString, 0x80000000 | 10,    9,   11,

       0        // eod
};

Q_CONSTINIT const QMetaObject GrandSearch::EntranceWidget::staticMetaObject = { {
    QMetaObject::SuperData::link<QFrame::staticMetaObject>(),
    qt_meta_stringdata_CLASSGrandSearchSCOPEEntranceWidgetENDCLASS.offsetsAndSizes,
    qt_meta_data_CLASSGrandSearchSCOPEEntranceWidgetENDCLASS,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_stringdata_CLASSGrandSearchSCOPEEntranceWidgetENDCLASS_t,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<EntranceWidget, std::true_type>,
        // method 'searchTextChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'sigSelectNextItem'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'sigSelectPreviousItem'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'sigHandleItem'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'sigCloseWindow'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onAppIconChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<const MatchedItem &, std::false_type>
    >,
    nullptr
} };

void GrandSearch::EntranceWidget::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<EntranceWidget *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->searchTextChanged((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 1: _t->sigSelectNextItem(); break;
        case 2: _t->sigSelectPreviousItem(); break;
        case 3: _t->sigHandleItem(); break;
        case 4: _t->sigCloseWindow(); break;
        case 5: _t->onAppIconChanged((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<MatchedItem>>(_a[2]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (EntranceWidget::*)(const QString & );
            if (_t _q_method = &EntranceWidget::searchTextChanged; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (EntranceWidget::*)();
            if (_t _q_method = &EntranceWidget::sigSelectNextItem; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (EntranceWidget::*)();
            if (_t _q_method = &EntranceWidget::sigSelectPreviousItem; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 2;
                return;
            }
        }
        {
            using _t = void (EntranceWidget::*)();
            if (_t _q_method = &EntranceWidget::sigHandleItem; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 3;
                return;
            }
        }
        {
            using _t = void (EntranceWidget::*)();
            if (_t _q_method = &EntranceWidget::sigCloseWindow; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 4;
                return;
            }
        }
    }
}

const QMetaObject *GrandSearch::EntranceWidget::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *GrandSearch::EntranceWidget::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_CLASSGrandSearchSCOPEEntranceWidgetENDCLASS.stringdata0))
        return static_cast<void*>(this);
    return QFrame::qt_metacast(_clname);
}

int GrandSearch::EntranceWidget::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QFrame::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 6)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 6;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 6)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 6;
    }
    return _id;
}

// SIGNAL 0
void GrandSearch::EntranceWidget::searchTextChanged(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void GrandSearch::EntranceWidget::sigSelectNextItem()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}

// SIGNAL 2
void GrandSearch::EntranceWidget::sigSelectPreviousItem()
{
    QMetaObject::activate(this, &staticMetaObject, 2, nullptr);
}

// SIGNAL 3
void GrandSearch::EntranceWidget::sigHandleItem()
{
    QMetaObject::activate(this, &staticMetaObject, 3, nullptr);
}

// SIGNAL 4
void GrandSearch::EntranceWidget::sigCloseWindow()
{
    QMetaObject::activate(this, &staticMetaObject, 4, nullptr);
}
QT_WARNING_POP
