/****************************************************************************
** Meta object code from reading C++ file 'matchwidget.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.8.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../../../src/grand-search/gui/exhibition/matchresult/matchwidget.h"
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'matchwidget.h' doesn't include <QObject>."
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
struct qt_meta_stringdata_CLASSGrandSearchSCOPEMatchWidgetENDCLASS_t {};
constexpr auto qt_meta_stringdata_CLASSGrandSearchSCOPEMatchWidgetENDCLASS = QtMocHelpers::stringData(
    "GrandSearch::MatchWidget",
    "sigCurrentItemChanged",
    "",
    "searchGroupName",
    "MatchedItem",
    "item",
    "sigShowNoContent",
    "noContent",
    "sigCloseWindow",
    "appendMatchedData",
    "MatchedItemMap",
    "matchedData",
    "clearMatchedData",
    "onSearchCompleted",
    "onShowMore",
    "selectNextItem",
    "selectPreviousItem",
    "handleItem",
    "onPreviewStateChanged",
    "preview",
    "onSelectItemByMouse"
);
#else  // !QT_MOC_HAS_STRINGDATA
#error "qtmochelpers.h not found or too old."
#endif // !QT_MOC_HAS_STRINGDATA
} // unnamed namespace

Q_CONSTINIT static const uint qt_meta_data_CLASSGrandSearchSCOPEMatchWidgetENDCLASS[] = {

 // content:
      12,       // revision
       0,       // classname
       0,    0, // classinfo
      12,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       3,       // signalCount

 // signals: name, argc, parameters, tag, flags, initial metatype offsets
       1,    2,   86,    2, 0x06,    1 /* Public */,
       6,    1,   91,    2, 0x06,    4 /* Public */,
       8,    0,   94,    2, 0x06,    6 /* Public */,

 // slots: name, argc, parameters, tag, flags, initial metatype offsets
       9,    1,   95,    2, 0x0a,    7 /* Public */,
      12,    0,   98,    2, 0x0a,    9 /* Public */,
      13,    0,   99,    2, 0x0a,   10 /* Public */,
      14,    0,  100,    2, 0x0a,   11 /* Public */,
      15,    0,  101,    2, 0x0a,   12 /* Public */,
      16,    0,  102,    2, 0x0a,   13 /* Public */,
      17,    0,  103,    2, 0x0a,   14 /* Public */,
      18,    1,  104,    2, 0x0a,   15 /* Public */,
      20,    1,  107,    2, 0x08,   17 /* Private */,

 // signals: parameters
    QMetaType::Void, QMetaType::QString, 0x80000000 | 4,    3,    5,
    QMetaType::Void, QMetaType::Bool,    7,
    QMetaType::Void,

 // slots: parameters
    QMetaType::Void, 0x80000000 | 10,   11,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Bool,   19,
    QMetaType::Void, 0x80000000 | 4,    5,

       0        // eod
};

Q_CONSTINIT const QMetaObject GrandSearch::MatchWidget::staticMetaObject = { {
    QMetaObject::SuperData::link<Dtk::Widget::DWidget::staticMetaObject>(),
    qt_meta_stringdata_CLASSGrandSearchSCOPEMatchWidgetENDCLASS.offsetsAndSizes,
    qt_meta_data_CLASSGrandSearchSCOPEMatchWidgetENDCLASS,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_stringdata_CLASSGrandSearchSCOPEMatchWidgetENDCLASS_t,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<MatchWidget, std::true_type>,
        // method 'sigCurrentItemChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<const MatchedItem &, std::false_type>,
        // method 'sigShowNoContent'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        // method 'sigCloseWindow'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'appendMatchedData'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const MatchedItemMap &, std::false_type>,
        // method 'clearMatchedData'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onSearchCompleted'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onShowMore'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'selectNextItem'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'selectPreviousItem'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'handleItem'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onPreviewStateChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const bool, std::false_type>,
        // method 'onSelectItemByMouse'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const MatchedItem &, std::false_type>
    >,
    nullptr
} };

void GrandSearch::MatchWidget::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<MatchWidget *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->sigCurrentItemChanged((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<MatchedItem>>(_a[2]))); break;
        case 1: _t->sigShowNoContent((*reinterpret_cast< std::add_pointer_t<bool>>(_a[1]))); break;
        case 2: _t->sigCloseWindow(); break;
        case 3: _t->appendMatchedData((*reinterpret_cast< std::add_pointer_t<MatchedItemMap>>(_a[1]))); break;
        case 4: _t->clearMatchedData(); break;
        case 5: _t->onSearchCompleted(); break;
        case 6: _t->onShowMore(); break;
        case 7: _t->selectNextItem(); break;
        case 8: _t->selectPreviousItem(); break;
        case 9: _t->handleItem(); break;
        case 10: _t->onPreviewStateChanged((*reinterpret_cast< std::add_pointer_t<bool>>(_a[1]))); break;
        case 11: _t->onSelectItemByMouse((*reinterpret_cast< std::add_pointer_t<MatchedItem>>(_a[1]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (MatchWidget::*)(const QString & , const MatchedItem & );
            if (_t _q_method = &MatchWidget::sigCurrentItemChanged; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (MatchWidget::*)(bool );
            if (_t _q_method = &MatchWidget::sigShowNoContent; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (MatchWidget::*)();
            if (_t _q_method = &MatchWidget::sigCloseWindow; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 2;
                return;
            }
        }
    }
}

const QMetaObject *GrandSearch::MatchWidget::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *GrandSearch::MatchWidget::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_CLASSGrandSearchSCOPEMatchWidgetENDCLASS.stringdata0))
        return static_cast<void*>(this);
    return Dtk::Widget::DWidget::qt_metacast(_clname);
}

int GrandSearch::MatchWidget::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = Dtk::Widget::DWidget::qt_metacall(_c, _id, _a);
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
void GrandSearch::MatchWidget::sigCurrentItemChanged(const QString & _t1, const MatchedItem & _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void GrandSearch::MatchWidget::sigShowNoContent(bool _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}

// SIGNAL 2
void GrandSearch::MatchWidget::sigCloseWindow()
{
    QMetaObject::activate(this, &staticMetaObject, 2, nullptr);
}
QT_WARNING_POP
