meta:
	ADDON_NAME = ofxRPlidar
	ADDON_DESCRIPTION = SLAMTEC RPLIDAR A Series SDK Wrapper for openFrameworks. https://www.slamtec.com/en/Lidar/A3
	ADDON_AUTHOR = Nariaki Iwatani
	ADDON_TAGS = "RPlidar" "LiDAR" "Scanner"
	ADDON_URL = https://github.com/nariakiiwatani/ofxRPlidar

common:
	ADDON_INCLUDES_EXCLUDE = libs/rplidar/src/hal%
	ADDON_INCLUDES_EXCLUDE += libs/rplidar/src/arch%

osx:
	ADDON_SOURCES_EXCLUDE = libs/rplidar/src/arch/win32%
	ADDON_SOURCES_EXCLUDE += libs/rplidar/src/arch/linux%
	ADDON_DEFINES = _MACOS

vs:
	ADDON_SOURCES_EXCLUDE = libs/rplidar/src/arch/macOS%
	ADDON_SOURCES_EXCLUDE += libs/rplidar/src/arch/linux%
	ADDON_DEFINES = _WIN32

linux:
	ADDON_SOURCES_EXCLUDE = libs/rplidar/src/arch/win32%
	ADDON_SOURCES_EXCLUDE += libs/rplidar/src/arch/macOS%
	ADDON_DEFINES = __GNUC__

linux64:
	ADDON_SOURCES_EXCLUDE = libs/rplidar/src/arch/win32%
	ADDON_SOURCES_EXCLUDE += libs/rplidar/src/arch/macOS%
	ADDON_DEFINES = __GNUC__

