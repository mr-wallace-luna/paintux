cmake_minimum_required(VERSION 3.16)
project(paint-ux VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_CXX_FLAGS_RELEASE "-Os -DNDEBUG -ffunction-sections -fdata-sections -fno-exceptions -fno-rtti -flto=auto")
set(CMAKE_EXE_LINKER_FLAGS_RELEASE "-Wl,--gc-sections -Wl,--strip-all -Wl,--as-needed -flto=auto -s")

include(CheckIPOSupported)
check_ipo_supported(RESULT ipo_supported OUTPUT ipo_error)
if(ipo_supported)
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE TRUE)
    message(STATUS "🚀 LTO (Link Time Optimization) ACTIVADO en Release")
else()
    message(STATUS "⚠️  LTO no soportado por el compilador: ${ipo_error}")
endif()

find_package(Qt6 REQUIRED COMPONENTS Widgets Gui Core LinguistTools Svg SvgWidgets)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

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

qt_add_translation(QM_FILES ${TS_FILES})

set(MAIN_SOURCES
    src/main.cpp
)

set(CPP_SOURCES
    src/paintarea.cpp
    src/filter/ImageFilters.cpp
)

# ============================================================
# Tools: incluye TextEngine.h (movido desde core/)
# ============================================================
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

# ============================================================
# Core: SIN TextEngine.h (movido a tools/)
# ============================================================
set(CORE_SOURCES
    src/core/CustomBrushes.h
    src/core/ShapeObjects.h
    src/core/TileSystem.h
    src/core/LayerStack.h
    src/core/MaskEditController.h
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
    ${OTHER_SOURCES}
    ${UI_SOURCES}
    ${QM_FILES}
)

target_include_directories(paint-ux PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/tools
    ${CMAKE_SOURCE_DIR}/src/filter
    ${CMAKE_SOURCE_DIR}/src/core
    ${CMAKE_SOURCE_DIR}/src/ui
)

target_link_libraries(paint-ux PRIVATE Qt6::Widgets Qt6::Gui Qt6::Core Qt6::Svg Qt6::SvgWidgets)

file(GLOB TRANSLATION_SOURCES
    ${CMAKE_SOURCE_DIR}/src/*.cpp
    ${CMAKE_SOURCE_DIR}/src/*.h
    ${CMAKE_SOURCE_DIR}/src/tools/*.h
    ${CMAKE_SOURCE_DIR}/src/filter/*.h
    ${CMAKE_SOURCE_DIR}/src/filter/*.cpp
    ${CMAKE_SOURCE_DIR}/src/core/*.h
    ${CMAKE_SOURCE_DIR}/src/ui/*.h
    ${CMAKE_SOURCE_DIR}/src/ui/*.cpp
)

add_custom_target(update_translations ALL
    COMMAND Qt6::lupdate ${TRANSLATION_SOURCES} -ts ${TS_FILES}
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Actualizando archivos .ts con lupdate..."
)
add_dependencies(paint-ux update_translations)

include(GNUInstallDirs)
install(TARGETS paint-ux DESTINATION ${CMAKE_INSTALL_BINDIR})
install(DIRECTORY assets DESTINATION ${CMAKE_INSTALL_DATADIR}/paint-ux)
install(FILES ${QM_FILES} DESTINATION ${CMAKE_INSTALL_DATADIR}/paint-ux/translations)
install(FILES assets/logo.svg DESTINATION ${CMAKE_INSTALL_DATADIR}/pixmaps RENAME paint-ux.svg)

file(WRITE ${CMAKE_BINARY_DIR}/paint-ux.desktop
"[Desktop Entry]
Type=Application
Version=1.0
Name=Paint-UX
Comment=Herramienta de dibujo minimalista basada en Ribbon UI para Linux
Exec=paint-ux
Path=/usr/share/paint-ux
Icon=paint-ux
Terminal=false
Categories=Graphics;
")
install(FILES ${CMAKE_BINARY_DIR}/paint-ux.desktop DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)

set(CPACK_PACKAGE_NAME "paint-ux")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Herramienta de dibujo minimalista basada en Ribbon UI para Linux")
set(CPACK_PACKAGE_VENDOR "Paint-UX Project")
set(CPACK_PACKAGE_CONTACT "desarrollo@paintux.online")

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

set(CPACK_RPM_PACKAGE_LICENSE "GPLv3")
set(CPACK_RPM_PACKAGE_GROUP "Applications/Multimedia")
set(CPACK_RPM_PACKAGE_URL "https://paintux.online")
set(CPACK_RPM_PACKAGE_DESCRIPTION "Paint-UX es una herramienta de dibujo profesional con interfaz Ribbon, soporte para capas, animación pixel art, pinceles personalizados y filtros avanzados.")
set(CPACK_RPM_PACKAGE_AUTOREQPROV ON)
set(CPACK_RPM_PACKAGE_REQUIRES "qt6-qtbase, qt6-qtsvg")
set(CPACK_RPM_PACKAGE_RECOMMENDS "qt6-qtimageformats")

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
    "/usr/share/doc"
    "/usr/share/man"
)

include(CPack)

message(STATUS "========================================")
message(STATUS "  Paint-UX v${PROJECT_VERSION}")
message(STATUS "  Estructura: src/ + src/tools/ + src/filter/ + src/core/ + src/ui/")
message(STATUS "  Tools: Retouch, Lasso, PixelArt, PixelAnim, Gradient, MagicWand, Deform, Select, BezierPath, TextEngine")
message(STATUS "  Core: CustomBrushes, ShapeObjects, TileSystem, LayerStack, MaskEditController")
message(STATUS "  Filter: ImageFilters (.h + .cpp)")
message(STATUS "  PaintArea: paintarea.h + paintarea.cpp")
message(STATUS "  UI: mainwindow.h + mainwindow.cpp")
message(STATUS "  Generadores: DEB (dependencias manuales universales), RPM (auto-reqprov)")
message(STATUS "  Idiomas soportados: 16")
message(STATUS "  Ejecutable: paint-ux")
message(STATUS "  Instalación: ${CMAKE_INSTALL_PREFIX}")
message(STATUS "========================================")