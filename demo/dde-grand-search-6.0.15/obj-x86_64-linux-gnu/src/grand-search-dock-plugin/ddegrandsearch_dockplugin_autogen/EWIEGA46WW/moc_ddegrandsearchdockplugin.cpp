/****************************************************************************
** Meta object code from reading C++ file 'ddegrandsearchdockplugin.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.8.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../../../src/grand-search-dock-plugin/ddegrandsearchdockplugin.h"
#include <QtGui/qtextcursor.h>
#include <QtCore/qmetatype.h>
#include <QtCore/qplugin.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'ddegrandsearchdockplugin.h' doesn't include <QObject>."
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
struct qt_meta_stringdata_CLASSGrandSearchSCOPEDdeGrandSearchDockPluginENDCLASS_t {};
constexpr auto qt_meta_stringdata_CLASSGrandSearchSCOPEDdeGrandSearchDockPluginENDCLASS = QtMocHelpers::stringData(
    "GrandSearch::DdeGrandSearchDockPlugin",
    "onVisibleChanged",
    "",
    "visible"
);
#else  // !QT_MOC_HAS_STRINGDATA
#error "qtmochelpers.h not found or too old."
#endif // !QT_MOC_HAS_STRINGDATA
} // unnamed namespace

Q_CONSTINIT static const uint qt_meta_data_CLASSGrandSearchSCOPEDdeGrandSearchDockPluginENDCLASS[] = {

 // content:
      12,       // revision
       0,       // classname
       0,    0, // classinfo
       1,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: name, argc, parameters, tag, flags, initial metatype offsets
       1,    1,   20,    2, 0x08,    1 /* Private */,

 // slots: parameters
    QMetaType::Void, QMetaType::Bool,    3,

       0        // eod
};

Q_CONSTINIT const QMetaObject GrandSearch::DdeGrandSearchDockPlugin::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_meta_stringdata_CLASSGrandSearchSCOPEDdeGrandSearchDockPluginENDCLASS.offsetsAndSizes,
    qt_meta_data_CLASSGrandSearchSCOPEDdeGrandSearchDockPluginENDCLASS,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_stringdata_CLASSGrandSearchSCOPEDdeGrandSearchDockPluginENDCLASS_t,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<DdeGrandSearchDockPlugin, std::true_type>,
        // method 'onVisibleChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<bool, std::false_type>
    >,
    nullptr
} };

void GrandSearch::DdeGrandSearchDockPlugin::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<DdeGrandSearchDockPlugin *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->onVisibleChanged((*reinterpret_cast< std::add_pointer_t<bool>>(_a[1]))); break;
        default: ;
        }
    }
}

const QMetaObject *GrandSearch::DdeGrandSearchDockPlugin::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *GrandSearch::DdeGrandSearchDockPlugin::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_CLASSGrandSearchSCOPEDdeGrandSearchDockPluginENDCLASS.stringdata0))
        return static_cast<void*>(this);
    if (!strcmp(_clname, "PluginsItemInterfaceV2"))
        return static_cast< PluginsItemInterfaceV2*>(this);
    if (!strcmp(_clname, "com.deepin.dock.PluginsItemInterface_V2"))
        return static_cast< PluginsItemInterfaceV2*>(this);
    return QObject::qt_metacast(_clname);
}

int GrandSearch::DdeGrandSearchDockPlugin::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 1)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 1;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 1)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 1;
    }
    return _id;
}
using namespace GrandSearch;

#ifdef QT_MOC_EXPORT_PLUGIN_V2
static constexpr unsigned char qt_pluginMetaDataV2_DdeGrandSearchDockPlugin[] = {
    0xbf, 
    // "IID"
    0x02,  0x78,  0x27,  'c',  'o',  'm',  '.',  'd', 
    'e',  'e',  'p',  'i',  'n',  '.',  'd',  'o', 
    'c',  'k',  '.',  'P',  'l',  'u',  'g',  'i', 
    'n',  's',  'I',  't',  'e',  'm',  'I',  'n', 
    't',  'e',  'r',  'f',  'a',  'c',  'e',  '_', 
    'V',  '2', 
    // "className"
    0x03,  0x78,  0x18,  'D',  'd',  'e',  'G',  'r', 
    'a',  'n',  'd',  'S',  'e',  'a',  'r',  'c', 
    'h',  'D',  'o',  'c',  'k',  'P',  'l',  'u', 
    'g',  'i',  'n', 
    // "MetaData"
    0x04,  0xa1,  0x63,  'a',  'p',  'i',  0x65,  '2', 
    '.',  '0',  '.',  '0', 
    0xff, 
};
QT_MOC_EXPORT_PLUGIN_V2(GrandSearch::DdeGrandSearchDockPlugin, DdeGrandSearchDockPlugin, qt_pluginMetaDataV2_DdeGrandSearchDockPlugin)
#else
QT_PLUGIN_METADATA_SECTION
Q_CONSTINIT static constexpr unsigned char qt_pluginMetaData_DdeGrandSearchDockPlugin[] = {
    'Q', 'T', 'M', 'E', 'T', 'A', 'D', 'A', 'T', 'A', ' ', '!',
    // metadata version, Qt version, architectural requirements
    0, QT_VERSION_MAJOR, QT_VERSION_MINOR, qPluginArchRequirements(),
    0xbf, 
    // "IID"
    0x02,  0x78,  0x27,  'c',  'o',  'm',  '.',  'd', 
    'e',  'e',  'p',  'i',  'n',  '.',  'd',  'o', 
    'c',  'k',  '.',  'P',  'l',  'u',  'g',  'i', 
    'n',  's',  'I',  't',  'e',  'm',  'I',  'n', 
    't',  'e',  'r',  'f',  'a',  'c',  'e',  '_', 
    'V',  '2', 
    // "className"
    0x03,  0x78,  0x18,  'D',  'd',  'e',  'G',  'r', 
    'a',  'n',  'd',  'S',  'e',  'a',  'r',  'c', 
    'h',  'D',  'o',  'c',  'k',  'P',  'l',  'u', 
    'g',  'i',  'n', 
    // "MetaData"
    0x04,  0xa1,  0x63,  'a',  'p',  'i',  0x65,  '2', 
    '.',  '0',  '.',  '0', 
    0xff, 
};
QT_MOC_EXPORT_PLUGIN(GrandSearch::DdeGrandSearchDockPlugin, DdeGrandSearchDockPlugin)
#endif  // QT_MOC_EXPORT_PLUGIN_V2

QT_WARNING_POP
