cmake_minimum_required(VERSION 3.16)
project(paint-ux VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_CXX_FLAGS_RELEASE "-Os -DNDEBUG -ffunction-sections -fdata-sections -fno-exceptions -fno-rtti -flto=auto")
set(CMAKE_EXE_LINKER_FLAGS_RELEASE "-Wl,--gc-sections -Wl,--strip-all -Wl,--as-needed -flto=auto -s")

include(CheckIPOSupported)
check_ipo_supported(RESULT ipo_supported OUTPUT ipo_error)
if(ipo_supported)
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE TRUE)
    message(STATUS "LTO activado en Release")
else()
    message(STATUS "LTO no soportado: ${ipo_error}")
endif()

find_package(Qt6 REQUIRED COMPONENTS Widgets Gui Core LinguistTools Svg SvgWidgets)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# ============================================================
# Traducciones
# ============================================================
file(MAKE_DIRECTORY ${CMAKE_SOURCE_DIR}/translations)

set(LANGUAGES es en fr de it pt zh ar ru ja ko tr pl nl sv hi)
foreach(LANG IN LISTS LANGUAGES)
    if(NOT EXISTS ${CMAKE_SOURCE_DIR}/translations/paintux_${LANG}.ts)
        file(WRITE ${CMAKE_SOURCE_DIR}/translations/paintux_${LANG}.ts
"<?xml version=\"1.0\" encoding=\"utf-8\"?>
<!DOCTYPE TS>
<TS version=\"2.1\" language=\"${LANG}\"></TS>
")
    endif()
    list(APPEND TS_FILES "translations/paintux_${LANG}.ts")
endforeach()

# Compila los .ts traducidos -> .qm
qt_add_translation(QM_FILES ${TS_FILES})

# ============================================================
# Fuentes
# ============================================================
set(MAIN_SOURCES
    src/main.cpp
)

set(CPP_SOURCES
    src/paintarea.cpp
    src/filter/ImageFilters.cpp
    src/utils/ExifLoader.cpp
    src/core/CustomBrushes.cpp
)

set(TOOLS_SOURCES
    src/tools/RetouchTools.h
    src/tools/LassoTools.h
    src/tools/PixelArtTools.h
    src/tools/PixelAnimationTools.h
    src/tools/GradientTools.h
    src/tools/MagicWandTools.h
    src/tools/DeformTools.h
    src/tools/SelectTools.h
    src/tools/BezierPathTool.h
    src/tools/TextEngine.h
)

set(CORE_SOURCES
    src/core/ToolManager.h
    src/core/CustomBrushes.h
    src/core/ShapeObjects.h
    src/core/TileSystem.h
    src/core/LayerStack.h
    src/core/MaskEditController.h
)

set(UTILS_SOURCES
    src/utils/ExifLoader.h
)

set(OTHER_SOURCES
    src/filter/ImageFilters.h
    src/paintarea.h
)

set(UI_SOURCES
    src/ui/mainwindow.h
    src/ui/mainwindow.cpp
)

add_executable(paint-ux
    ${MAIN_SOURCES}
    ${CPP_SOURCES}
    ${TOOLS_SOURCES}
    ${CORE_SOURCES}
    ${UTILS_SOURCES}
    ${OTHER_SOURCES}
    ${UI_SOURCES}
    ${QM_FILES}
)

target_include_directories(paint-ux PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/tools
    ${CMAKE_SOURCE_DIR}/src/filter
    ${CMAKE_SOURCE_DIR}/src/core
    ${CMAKE_SOURCE_DIR}/src/utils
    ${CMAKE_SOURCE_DIR}/src/ui
)

target_link_libraries(paint-ux PRIVATE Qt6::Widgets Qt6::Gui Qt6::Core Qt6::Svg Qt6::SvgWidgets)

# ============================================================
# lupdate MANUAL (no se ejecuta automáticamente)
# ============================================================
file(GLOB_RECURSE TRANSLATION_SOURCES
    ${CMAKE_SOURCE_DIR}/src/*.cpp
    ${CMAKE_SOURCE_DIR}/src/*.h
)

add_custom_target(update_translations
    COMMAND Qt6::lupdate ${TRANSLATION_SOURCES} -no-obsolete -ts ${TS_FILES}
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Actualizando archivos .ts con lupdate (solo cuando lo pidas explicitamente)..."
)

# ⚠️ IMPORTANTE: NO usamos add_dependencies aquí.
# Sin esto, cmake NO ejecuta lupdate en cada build.
# Si quieres regenerar los .ts, ejecuta manualmente:
#     make update_translations
# Luego vuelve a correr tu script de traducción (traducir.py)
# Y finalmente: make

# ============================================================
# Metadata
# ============================================================
include(GNUInstallDirs)

set(META_DIR ${CMAKE_BINARY_DIR}/metadata)
file(MAKE_DIRECTORY ${META_DIR})

file(WRITE ${META_DIR}/paint-ux.desktop
"[Desktop Entry]
Type=Application
Version=1.0
Name=Paint-UX
GenericName=Editor de imagenes
GenericName[en]=Image Editor
GenericName[fr]=Editeur d'images
GenericName[de]=Bildbearbeitung
Comment=Editor de imagenes ligero y facil
Comment[en]=Lightweight and easy image editor
Comment[fr]=Editeur d'images leger et facile
Comment[de]=Leichter und einfacher Bildeditor
Exec=paint-ux %F
Icon=paint-ux
Terminal=false
Categories=Graphics;2DGraphics;RasterGraphics;
MimeType=application/x-paintlux-project;
StartupNotify=true
")

file(WRITE ${META_DIR}/paint-ux.xml
"<?xml version=\"1.0\" encoding=\"UTF-8\"?>
<mime-info xmlns=\"http://www.freedesktop.org/standards/shared-mime-info\">
  <mime-type type=\"application/x-paintlux-project\">
    <comment>Proyecto Paintlux</comment>
    <comment xml:lang=\"en\">Paintlux Project</comment>
    <comment xml:lang=\"es\">Proyecto Paintlux</comment>
    <comment xml:lang=\"fr\">Projet Paintlux</comment>
    <comment xml:lang=\"de\">Paintlux-Projekt</comment>
    <comment xml:lang=\"it\">Progetto Paintlux</comment>
    <comment xml:lang=\"pt\">Projeto Paintlux</comment>
    <icon name=\"application-x-paintlux-project\"/>
    <glob pattern=\"*.ptx\"/>
    <glob pattern=\"*.PTX\"/>
    <magic priority=\"50\">
      <match value=\"PTX1\" type=\"string\" offset=\"0\"/>
    </magic>
  </mime-type>
</mime-info>
")

# ============================================================
# Instalacion
# ============================================================
install(TARGETS paint-ux DESTINATION ${CMAKE_INSTALL_BINDIR})
install(DIRECTORY assets DESTINATION ${CMAKE_INSTALL_DATADIR}/paint-ux)
install(FILES ${QM_FILES} DESTINATION ${CMAKE_INSTALL_DATADIR}/paint-ux/translations)

install(FILES assets/logo.svg
        DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/scalable/apps
        RENAME paint-ux.svg)

install(FILES assets/logo.svg
        DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/scalable/mimetypes
        RENAME application-x-paintlux-project.svg)

install(FILES ${META_DIR}/paint-ux.desktop
        DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)

install(FILES ${META_DIR}/paint-ux.xml
        DESTINATION ${CMAKE_INSTALL_DATADIR}/mime/packages)

# ============================================================
# postinst / postrm
# ============================================================
file(WRITE ${CMAKE_BINARY_DIR}/postinst
"#!/bin/sh
set -e

if [ \"$1\" = \"configure\" ]; then
    if [ -x /usr/bin/update-mime-database ]; then
        update-mime-database /usr/share/mime >/dev/null 2>&1 || true
    fi
    if [ -x /usr/bin/update-desktop-database ]; then
        update-desktop-database /usr/share/applications >/dev/null 2>&1 || true
    fi
    if [ -x /usr/bin/gtk-update-icon-cache ]; then
        gtk-update-icon-cache -f -t /usr/share/icons/hicolor >/dev/null 2>&1 || true
    fi
    if [ -x /usr/bin/xdg-icon-resource ]; then
        xdg-icon-resource forceupdate --theme hicolor >/dev/null 2>&1 || true
    fi
fi

exit 0
")

file(WRITE ${CMAKE_BINARY_DIR}/postrm
"#!/bin/sh
set -e

if [ \"$1\" = \"remove\" ] || [ \"$1\" = \"purge\" ]; then
    if [ -x /usr/bin/update-mime-database ]; then
        update-mime-database /usr/share/mime >/dev/null 2>&1 || true
    fi
    if [ -x /usr/bin/update-desktop-database ]; then
        update-desktop-database /usr/share/applications >/dev/null 2>&1 || true
    fi
    if [ -x /usr/bin/gtk-update-icon-cache ]; then
        gtk-update-icon-cache -f -t /usr/share/icons/hicolor >/dev/null 2>&1 || true
    fi
fi

exit 0
")

execute_process(
    COMMAND chmod 755 ${CMAKE_BINARY_DIR}/postinst ${CMAKE_BINARY_DIR}/postrm
)

# ============================================================
# CPack
# ============================================================
set(CPACK_PACKAGE_NAME "paint-ux")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Editor de imagenes ligero con UI tipo Paint y proyectos .ptx")
set(CPACK_PACKAGE_VENDOR "Paint-UX Project")
set(CPACK_PACKAGE_CONTACT "desarrollo@paintux.online")
set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}-Linux")

set(CPACK_GENERATOR "DEB;RPM")

set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Equipo Paint-UX <desarrollo@paintux.online>")
set(CPACK_DEBIAN_PACKAGE_SECTION "graphics")
set(CPACK_DEBIAN_PACKAGE_PRIORITY "optional")
set(CPACK_DEBIAN_PACKAGE_HOMEPAGE "https://paintux.online")
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS OFF)
set(CPACK_DEBIAN_PACKAGE_DEPENDS "libc6, libqt6core6, libqt6gui6, libqt6widgets6, libqt6svg6, libqt6svgwidgets6, libstdc++6")
set(CPACK_DEBIAN_PACKAGE_RECOMMENDS "qt6-image-formats-plugins")
set(CPACK_DEBIAN_PACKAGE_SUGGESTS "gimp, inkscape, krita")
set(CPACK_DEBIAN_COMPRESSION_TYPE "xz")
set(CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA
    "${CMAKE_BINARY_DIR}/postinst"
    "${CMAKE_BINARY_DIR}/postrm")

set(CPACK_RPM_PACKAGE_LICENSE "GPLv3")
set(CPACK_RPM_PACKAGE_GROUP "Applications/Multimedia")
set(CPACK_RPM_PACKAGE_URL "https://paintux.online")
set(CPACK_RPM_PACKAGE_DESCRIPTION "Paint-UX es un editor de imagenes minimalista con interfaz Ribbon, capas, animacion pixel art y proyectos .ptx.")
set(CPACK_RPM_PACKAGE_AUTOREQPROV ON)
set(CPACK_RPM_PACKAGE_REQUIRES "qt6-qtbase, qt6-qtsvg")
set(CPACK_RPM_PACKAGE_RECOMMENDS "qt6-qtimageformats")
set(CPACK_RPM_POST_INSTALL_SCRIPT_FILE "${CMAKE_BINARY_DIR}/postinst")
set(CPACK_RPM_POST_UNINSTALL_SCRIPT_FILE "${CMAKE_BINARY_DIR}/postrm")

set(CPACK_RPM_EXCLUDE_FROM_AUTO_FILELIST_ADDITION
    "/etc"
    "/usr"
    "/usr/bin"
    "/usr/share"
    "/usr/share/applications"
    "/usr/share/pixmaps"
    "/usr/share/icons"
    "/usr/share/icons/hicolor"
    "/usr/share/icons/hicolor/scalable"
    "/usr/share/icons/hicolor/scalable/apps"
    "/usr/share/icons/hicolor/scalable/mimetypes"
    "/usr/share/mime"
    "/usr/share/mime/packages"
    "/usr/share/doc"
    "/usr/share/man"
    "/usr/share/paint-ux"
    "/usr/share/paint-ux/translations"
)

include(CPack)

message(STATUS "========================================")
message(STATUS "  Paint-UX v${PROJECT_VERSION} (ALPHA)")
message(STATUS "  Estructura: src/ + src/tools/ + src/filter/ + src/core/ + src/utils/ + src/ui/")
message(STATUS "  Generadores: DEB + RPM")
message(STATUS "  Idiomas soportados: 16")
message(STATUS "  Ejecutable: paint-ux")
message(STATUS "  Formato proyecto: .ptx (binario)")
message(STATUS "  MIME type: application/x-paintlux-project (*.ptx)")
message(STATUS "  Instalacion: ${CMAKE_INSTALL_PREFIX}")
message(STATUS "========================================")
message(STATUS "")
message(STATUS "  Para regenerar traducciones desde el codigo fuente:")
message(STATUS "     make update_translations")
message(STATUS "     python3 traducir.py")
message(STATUS "     make")
message(STATUS "========================================")