/****************************************************************************
** Meta object code from reading C++ file 'menu_screen.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../src/menu_screen.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'menu_screen.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.2. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_MenuScreen_t {
    QByteArrayData data[9];
    char stringdata0[133];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_MenuScreen_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_MenuScreen_t qt_meta_stringdata_MenuScreen = {
    {
QT_MOC_LITERAL(0, 0, 10), // "MenuScreen"
QT_MOC_LITERAL(1, 11, 17), // "continueRequested"
QT_MOC_LITERAL(2, 29, 0), // ""
QT_MOC_LITERAL(3, 30, 16), // "classicRequested"
QT_MOC_LITERAL(4, 47, 16), // "endlessRequested"
QT_MOC_LITERAL(5, 64, 20), // "leaderboardRequested"
QT_MOC_LITERAL(6, 85, 17), // "settingsRequested"
QT_MOC_LITERAL(7, 103, 13), // "exitRequested"
QT_MOC_LITERAL(8, 117, 15) // "nValueRequested"

    },
    "MenuScreen\0continueRequested\0\0"
    "classicRequested\0endlessRequested\0"
    "leaderboardRequested\0settingsRequested\0"
    "exitRequested\0nValueRequested"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_MenuScreen[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
       7,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       7,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    0,   49,    2, 0x06 /* Public */,
       3,    0,   50,    2, 0x06 /* Public */,
       4,    0,   51,    2, 0x06 /* Public */,
       5,    0,   52,    2, 0x06 /* Public */,
       6,    0,   53,    2, 0x06 /* Public */,
       7,    0,   54,    2, 0x06 /* Public */,
       8,    0,   55,    2, 0x06 /* Public */,

 // signals: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,

       0        // eod
};

void MenuScreen::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<MenuScreen *>(_o);
        Q_UNUSED(_t)
        switch (_id) {
        case 0: _t->continueRequested(); break;
        case 1: _t->classicRequested(); break;
        case 2: _t->endlessRequested(); break;
        case 3: _t->leaderboardRequested(); break;
        case 4: _t->settingsRequested(); break;
        case 5: _t->exitRequested(); break;
        case 6: _t->nValueRequested(); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (MenuScreen::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&MenuScreen::continueRequested)) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (MenuScreen::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&MenuScreen::classicRequested)) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (MenuScreen::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&MenuScreen::endlessRequested)) {
                *result = 2;
                return;
            }
        }
        {
            using _t = void (MenuScreen::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&MenuScreen::leaderboardRequested)) {
                *result = 3;
                return;
            }
        }
        {
            using _t = void (MenuScreen::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&MenuScreen::settingsRequested)) {
                *result = 4;
                return;
            }
        }
        {
            using _t = void (MenuScreen::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&MenuScreen::exitRequested)) {
                *result = 5;
                return;
            }
        }
        {
            using _t = void (MenuScreen::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&MenuScreen::nValueRequested)) {
                *result = 6;
                return;
            }
        }
    }
    Q_UNUSED(_a);
}

QT_INIT_METAOBJECT const QMetaObject MenuScreen::staticMetaObject = { {
    QMetaObject::SuperData::link<QWidget::staticMetaObject>(),
    qt_meta_stringdata_MenuScreen.data,
    qt_meta_data_MenuScreen,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *MenuScreen::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *MenuScreen::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_MenuScreen.stringdata0))
        return static_cast<void*>(this);
    return QWidget::qt_metacast(_clname);
}

int MenuScreen::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 7)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 7;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 7)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 7;
    }
    return _id;
}

// SIGNAL 0
void MenuScreen::continueRequested()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}

// SIGNAL 1
void MenuScreen::classicRequested()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}

// SIGNAL 2
void MenuScreen::endlessRequested()
{
    QMetaObject::activate(this, &staticMetaObject, 2, nullptr);
}

// SIGNAL 3
void MenuScreen::leaderboardRequested()
{
    QMetaObject::activate(this, &staticMetaObject, 3, nullptr);
}

// SIGNAL 4
void MenuScreen::settingsRequested()
{
    QMetaObject::activate(this, &staticMetaObject, 4, nullptr);
}

// SIGNAL 5
void MenuScreen::exitRequested()
{
    QMetaObject::activate(this, &staticMetaObject, 5, nullptr);
}

// SIGNAL 6
void MenuScreen::nValueRequested()
{
    QMetaObject::activate(this, &staticMetaObject, 6, nullptr);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
