# The Qt the Live TV add-on links (QtCore only): configure.ac asks qmake
# for its flags with modules/gui/qt/scripts/static_dirs.py
TEMPLATE = app

QT = core

CONFIG -= entrypoint
CONFIG -= debug_and_release
CONFIG += no_include_pwd
