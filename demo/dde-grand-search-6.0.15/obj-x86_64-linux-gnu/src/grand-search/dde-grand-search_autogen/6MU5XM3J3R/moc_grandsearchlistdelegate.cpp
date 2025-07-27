/****************************************************************************
** Meta object code from reading C++ file 'grandsearchlistdelegate.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.8.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../../../src/grand-search/gui/exhibition/matchresult/listview/grandsearchlistdelegate.h"
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'grandsearchlistdelegate.h' doesn't include <QObject>."
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
struct qt_meta_stringdata_CLASSGrandSearchSCOPEGrandSearchListDelegateENDCLASS_t {};
constexpr auto qt_meta_stringdata_CLASSGrandSearchSCOPEGrandSearchListDelegateENDCLASS = QtMocHelpers::stringData(
    "GrandSearch::GrandSearchListDelegate"
);
#else  // !QT_MOC_HAS_STRINGDATA
#error "qtmochelpers.h not found or too old."
#endif // !QT_MOC_HAS_STRINGDATA
} // unnamed namespace

Q_CONSTINIT static const uint qt_meta_data_CLASSGrandSearchSCOPEGrandSearchListDelegateENDCLASS[] = {

 // content:
      12,       // revision
       0,       // classname
       0,    0, // classinfo
       0,    0, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

       0        // eod
};

Q_CONSTINIT const QMetaObject GrandSearch::GrandSearchListDelegate::staticMetaObject = { {
    QMetaObject::SuperData::link<Dtk::Widget::DStyledItemDelegate::staticMetaObject>(),
    qt_meta_stringdata_CLASSGrandSearchSCOPEGrandSearchListDelegateENDCLASS.offsetsAndSizes,
    qt_meta_data_CLASSGrandSearchSCOPEGrandSearchListDelegateENDCLASS,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_stringdata_CLASSGrandSearchSCOPEGrandSearchListDelegateENDCLASS_t,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<GrandSearchListDelegate, std::true_type>
    >,
    nullptr
} };

void GrandSearch::GrandSearchListDelegate::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    (void)_o;
    (void)_id;
    (void)_c;
    (void)_a;
}

const QMetaObject *GrandSearch::GrandSearchListDelegate::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *GrandSearch::GrandSearchListDelegate::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_CLASSGrandSearchSCOPEGrandSearchListDelegateENDCLASS.stringdata0))
        return static_cast<void*>(this);
    return Dtk::Widget::DStyledItemDelegate::qt_metacast(_clname);
}

int GrandSearch::GrandSearchListDelegate::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = Dtk::Widget::DStyledItemDelegate::qt_metacall(_c, _id, _a);
    return _id;
}
QT_WARNING_POP
