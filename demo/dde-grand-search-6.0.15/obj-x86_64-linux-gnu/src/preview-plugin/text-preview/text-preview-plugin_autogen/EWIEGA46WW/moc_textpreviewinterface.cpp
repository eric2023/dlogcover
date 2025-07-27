/****************************************************************************
** Meta object code from reading C++ file 'textpreviewinterface.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.8.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../../../../src/preview-plugin/text-preview/textpreviewinterface.h"
#include <QtCore/qmetatype.h>
#include <QtCore/qplugin.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'textpreviewinterface.h' doesn't include <QObject>."
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
struct qt_meta_stringdata_CLASSGrandSearchSCOPEtext_previewSCOPETextPreviewInterfaceENDCLASS_t {};
constexpr auto qt_meta_stringdata_CLASSGrandSearchSCOPEtext_previewSCOPETextPreviewInterfaceENDCLASS = QtMocHelpers::stringData(
    "GrandSearch::text_preview::TextPreviewInterface"
);
#else  // !QT_MOC_HAS_STRINGDATA
#error "qtmochelpers.h not found or too old."
#endif // !QT_MOC_HAS_STRINGDATA
} // unnamed namespace

Q_CONSTINIT static const uint qt_meta_data_CLASSGrandSearchSCOPEtext_previewSCOPETextPreviewInterfaceENDCLASS[] = {

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

Q_CONSTINIT const QMetaObject GrandSearch::text_preview::TextPreviewInterface::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_meta_stringdata_CLASSGrandSearchSCOPEtext_previewSCOPETextPreviewInterfaceENDCLASS.offsetsAndSizes,
    qt_meta_data_CLASSGrandSearchSCOPEtext_previewSCOPETextPreviewInterfaceENDCLASS,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_stringdata_CLASSGrandSearchSCOPEtext_previewSCOPETextPreviewInterfaceENDCLASS_t,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<TextPreviewInterface, std::true_type>
    >,
    nullptr
} };

void GrandSearch::text_preview::TextPreviewInterface::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    (void)_o;
    (void)_id;
    (void)_c;
    (void)_a;
}

const QMetaObject *GrandSearch::text_preview::TextPreviewInterface::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *GrandSearch::text_preview::TextPreviewInterface::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_CLASSGrandSearchSCOPEtext_previewSCOPETextPreviewInterfaceENDCLASS.stringdata0))
        return static_cast<void*>(this);
    if (!strcmp(_clname, "PreviewPluginInterface"))
        return static_cast< PreviewPluginInterface*>(this);
    if (!strcmp(_clname, "com.deepin.grandsearch.FilePreviewInterface.iid"))
        return static_cast< GrandSearch::PreviewPluginInterface*>(this);
    return QObject::qt_metacast(_clname);
}

int GrandSearch::text_preview::TextPreviewInterface::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    return _id;
}
using namespace GrandSearch;
using namespace GrandSearch::text_preview;

#ifdef QT_MOC_EXPORT_PLUGIN_V2
static constexpr unsigned char qt_pluginMetaDataV2_TextPreviewInterface[] = {
    0xbf, 
    // "IID"
    0x02,  0x78,  0x2f,  'c',  'o',  'm',  '.',  'd', 
    'e',  'e',  'p',  'i',  'n',  '.',  'g',  'r', 
    'a',  'n',  'd',  's',  'e',  'a',  'r',  'c', 
    'h',  '.',  'F',  'i',  'l',  'e',  'P',  'r', 
    'e',  'v',  'i',  'e',  'w',  'I',  'n',  't', 
    'e',  'r',  'f',  'a',  'c',  'e',  '.',  'i', 
    'i',  'd', 
    // "className"
    0x03,  0x74,  'T',  'e',  'x',  't',  'P',  'r', 
    'e',  'v',  'i',  'e',  'w',  'I',  'n',  't', 
    'e',  'r',  'f',  'a',  'c',  'e', 
    0xff, 
};
QT_MOC_EXPORT_PLUGIN_V2(GrandSearch::text_preview::TextPreviewInterface, TextPreviewInterface, qt_pluginMetaDataV2_TextPreviewInterface)
#else
QT_PLUGIN_METADATA_SECTION
Q_CONSTINIT static constexpr unsigned char qt_pluginMetaData_TextPreviewInterface[] = {
    'Q', 'T', 'M', 'E', 'T', 'A', 'D', 'A', 'T', 'A', ' ', '!',
    // metadata version, Qt version, architectural requirements
    0, QT_VERSION_MAJOR, QT_VERSION_MINOR, qPluginArchRequirements(),
    0xbf, 
    // "IID"
    0x02,  0x78,  0x2f,  'c',  'o',  'm',  '.',  'd', 
    'e',  'e',  'p',  'i',  'n',  '.',  'g',  'r', 
    'a',  'n',  'd',  's',  'e',  'a',  'r',  'c', 
    'h',  '.',  'F',  'i',  'l',  'e',  'P',  'r', 
    'e',  'v',  'i',  'e',  'w',  'I',  'n',  't', 
    'e',  'r',  'f',  'a',  'c',  'e',  '.',  'i', 
    'i',  'd', 
    // "className"
    0x03,  0x74,  'T',  'e',  'x',  't',  'P',  'r', 
    'e',  'v',  'i',  'e',  'w',  'I',  'n',  't', 
    'e',  'r',  'f',  'a',  'c',  'e', 
    0xff, 
};
QT_MOC_EXPORT_PLUGIN(GrandSearch::text_preview::TextPreviewInterface, TextPreviewInterface)
#endif  // QT_MOC_EXPORT_PLUGIN_V2

QT_WARNING_POP
