export QTDIR=/usr/local/Trolltech/Qt-4.3.3
export PATH=$QTDIR/bin:$PATH
export QMAKESPEC=$QTDIR/mkspecs/macx-g++
export DYLD_LIBRARY_PATH=$QTDIR/lib

#export WITH_INSTALL=1
export BUILD_BASE_DIR=$PWD/../temp_build
export CONFIG_OPTIONS="--disable-shared"
export C_INCLUDE_PATH="$BUILD_BASE_DIR/$BUILD_STYLE/local/include:$QTDIR/include"
export CPLUS_INCLUDE_PATH=$C_INCLUDE_PATH
export LIBRARY_PATH="$BUILD_BASE_DIR/$BUILD_STYLE/local/lib:$QTDIR/lib"
export CFLAGS="-framework CoreFoundation"
export CXXFLAGS=$CFLAGS
export PKG_CONFIG_PATH="$BUILD_BASE_DIR/$BUILD_STYLE/local/lib/pkgconfig"
#export HAVE_UNIX98=1
export UB_PRODUCTS=bin/qsampler.app  #  Dummy (for 'clean' target)

case "$BUILD_STYLE" in
	*UB)
		BUILD_STYLE_BASE=${BUILD_STYLE/UB/}
		if test "x$ACTION" = "xclean"; then
			source $PROJECT_DIR/autoconf_builder.sh
			exit 0
		fi
		BUILD_STYLE="${BUILD_STYLE_BASE}ppc"
		/bin/sh $0 || exit $?
		BUILD_STYLE="${BUILD_STYLE_BASE}i386"
		/bin/sh $0 || exit $?
		BUILD_STYLE="${BUILD_STYLE_BASE}UB"
		ODIR=`dirname "$0"`
		cd "$BUILD_BASE_DIR"
		cp -r "${BUILD_STYLE_BASE}ppc/local/bin/qsampler.app" "${BUILD_STYLE_BASE}UB/local/bin"
		echo "qsampler.app has been copied to $BUILD_BASE_DIR/$BUILD_STYLE/local/bin"
		XDIR="local/bin/qsampler.app/Contents/MacOS"
		lipo -create "${BUILD_STYLE_BASE}ppc/$XDIR/qsampler" "${BUILD_STYLE_BASE}i386/$XDIR/qsampler" -output "$BUILD_STYLE/$XDIR/qsampler" || exit $?
		#  A trick: rename the executable to "qsampler " (note the whitespace) and place a wrapper
		#  script as "qsampler"
	#	cd "$BUILD_STYLE/$XDIR"
	#	mv qsampler "qsampler "
	#	cp "$ODIR/qsampler.wrapper" ./qsampler
	#	chmod +x qsampler

		#  Place the icon file and modify the Info.plist file
		cd "$BUILD_STYLE/$XDIR/.."  #  qsampler.app/Contents
		rm -rf Resources
		mkdir Resources
		cp "$ODIR/qsampler.icns" Resources/
		mv Info.plist Info.plist~
		sed '/CFBundleIconFile/,/<key>/s/<string>.*<\/string>/<string>qsampler.icns<\/string>/' Info.plist~ >Info.plist
		rm Info.plist~

		#  Embed the Qt frameworks and plugins
		if test -e /Library/Frameworks/QtCore.framework; then
			#  Copy frameworks
			mkdir -p Frameworks
			rm -rf Frameworks/*
			QTLIBS="QtCore QtGui QtSvg QtXml"
			for lib in $QTLIBS; do
				cp -R /Library/Frameworks/$lib.framework ./Frameworks/
			done
			#  Copy plugins
			rm -rf plugins
			mkdir -p plugins/imageformats
			QTPLUGINS="libqgif libqjpeg libqmng libqsvg libqtiff"
			for plugin in $QTPLUGINS; do
				cp -R /Developer/Applications/Qt/plugins/imageformats/$plugin.dylib ./plugins/imageformats/
			done
			#  Update the install names so that the executable can find the correct frameworks
			for lib in $QTLIBS; do
				install_name_tool -change $lib.framework/Versions/4/$lib @executable_path/../Frameworks/$lib.framework/Versions/4/$lib MacOS/qsampler
				install_name_tool -id @executable_path/../Frameworks/$lib.framework/Versions/4/$lib Frameworks/$lib.framework/Versions/4/$lib
				for lib2 in $QTLIBS; do
					install_name_tool -change $lib.framework/Versions/4/$lib @executable_path/../Frameworks/$lib.framework/Versions/4/$lib Frameworks/$lib2.framework/Versions/4/$lib2
				done
				for plugin in $QTPLUGINS; do
					install_name_tool -change $lib.framework/Versions/4/$lib @executable_path/../Frameworks/$lib.framework/Versions/4/$lib plugins/imageformats/$plugin.dylib
				done
			done
		fi
		cp -f "$ODIR/linuxsampler.starter" $BUILD_BASE_DIR/$BUILD_STYLE/local/bin/
		chmod +x "$BUILD_BASE_DIR/$BUILD_STYLE/local/bin/linuxsampler.starter"
		echo "Universal binary package qsampler.app has been created in $BUILD_BASE_DIR/$BUILD_STYLE/local/bin"
		exit 0
		;;
    *ppc)  export QMAKE_ARCHS="ppc" ;;
	*i386) export QMAKE_ARCHS="x86" ;;
esac

source $PROJECT_DIR/autoconf_builder.sh

if test "x$ACTION" != "xclean"; then
	#  "install"
	mkdir -p "$BUILD_BASE_DIR/$BUILD_STYLE/local/bin"
	cp -r "$BUILD_DIR/qsampler.app" "$BUILD_BASE_DIR/$BUILD_STYLE/local/bin"
	echo "qsampler.app has been copied to $BUILD_BASE_DIR/$BUILD_STYLE/local/bin"
fi
