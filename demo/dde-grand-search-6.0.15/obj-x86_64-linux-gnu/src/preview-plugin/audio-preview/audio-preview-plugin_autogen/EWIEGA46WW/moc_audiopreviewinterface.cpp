/****************************************************************************
** Meta object code from reading C++ file 'audiopreviewinterface.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.8.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../../../../src/preview-plugin/audio-preview/audiopreviewinterface.h"
#include <QtCore/qmetatype.h>
#include <QtCore/qplugin.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'audiopreviewinterface.h' doesn't include <QObject>."
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
struct qt_meta_stringdata_CLASSGrandSearchSCOPEaudio_previewSCOPEAudioPreviewInterfaceENDCLASS_t {};
constexpr auto qt_meta_stringdata_CLASSGrandSearchSCOPEaudio_previewSCOPEAudioPreviewInterfaceENDCLASS = QtMocHelpers::stringData(
    "GrandSearch::audio_preview::AudioPreviewInterface"
);
#else  // !QT_MOC_HAS_STRINGDATA
#error "qtmochelpers.h not found or too old."
#endif // !QT_MOC_HAS_STRINGDATA
} // unnamed namespace

Q_CONSTINIT static const uint qt_meta_data_CLASSGrandSearchSCOPEaudio_previewSCOPEAudioPreviewInterfaceENDCLASS[] = {

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

Q_CONSTINIT const QMetaObject GrandSearch::audio_preview::AudioPreviewInterface::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_meta_stringdata_CLASSGrandSearchSCOPEaudio_previewSCOPEAudioPreviewInterfaceENDCLASS.offsetsAndSizes,
    qt_meta_data_CLASSGrandSearchSCOPEaudio_previewSCOPEAudioPreviewInterfaceENDCLASS,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_stringdata_CLASSGrandSearchSCOPEaudio_previewSCOPEAudioPreviewInterfaceENDCLASS_t,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<AudioPreviewInterface, std::true_type>
    >,
    nullptr
} };

void GrandSearch::audio_preview::AudioPreviewInterface::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    (void)_o;
    (void)_id;
    (void)_c;
    (void)_a;
}

const QMetaObject *GrandSearch::audio_preview::AudioPreviewInterface::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *GrandSearch::audio_preview::AudioPreviewInterface::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_CLASSGrandSearchSCOPEaudio_previewSCOPEAudioPreviewInterfaceENDCLASS.stringdata0))
        return static_cast<void*>(this);
    if (!strcmp(_clname, "PreviewPluginInterface"))
        return static_cast< PreviewPluginInterface*>(this);
    if (!strcmp(_clname, "com.deepin.grandsearch.FilePreviewInterface.iid"))
        return static_cast< GrandSearch::PreviewPluginInterface*>(this);
    return QObject::qt_metacast(_clname);
}

int GrandSearch::audio_preview::AudioPreviewInterface::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    return _id;
}
using namespace GrandSearch;
using namespace GrandSearch::audio_preview;

#ifdef QT_MOC_EXPORT_PLUGIN_V2
static constexpr unsigned char qt_pluginMetaDataV2_AudioPreviewInterface[] = {
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
    0x03,  0x75,  'A',  'u',  'd',  'i',  'o',  'P', 
    'r',  'e',  'v',  'i',  'e',  'w',  'I',  'n', 
    't',  'e',  'r',  'f',  'a',  'c',  'e', 
    0xff, 
};
QT_MOC_EXPORT_PLUGIN_V2(GrandSearch::audio_preview::AudioPreviewInterface, AudioPreviewInterface, qt_pluginMetaDataV2_AudioPreviewInterface)
#else
QT_PLUGIN_METADATA_SECTION
Q_CONSTINIT static constexpr unsigned char qt_pluginMetaData_AudioPreviewInterface[] = {
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
    0x03,  0x75,  'A',  'u',  'd',  'i',  'o',  'P', 
    'r',  'e',  'v',  'i',  'e',  'w',  'I',  'n', 
    't',  'e',  'r',  'f',  'a',  'c',  'e', 
    0xff, 
};
QT_MOC_EXPORT_PLUGIN(GrandSearch::audio_preview::AudioPreviewInterface, AudioPreviewInterface)
#endif  // QT_MOC_EXPORT_PLUGIN_V2

QT_WARNING_POP
