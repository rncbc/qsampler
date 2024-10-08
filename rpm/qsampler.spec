#
# spec file for package qsampler
#
# Copyright (C) 2004-2024, rncbc aka Rui Nuno Capela. All rights reserved.
# Copyright (C) 2007,2008,2015 Christian Schoenebeck
#
# All modifications and additions to the file contributed by third parties
# remain the property of their copyright owners, unless otherwise agreed
# upon. The license for this file, and modifications and additions to the
# file, is the same license as for the pristine package itself (unless the
# license for the pristine package is not an Open Source License, in which
# case the license is the MIT License). An "Open Source License" is a
# license that conforms to the Open Source Definition (Version 1.9)
# published by the Open Source Initiative.

# Please submit bugfixes or comments via http://bugs.opensuse.org/
#

Summary:	A LinuxSampler Qt GUI interface
Name:		qsampler
Version:	1.0.0
Release:	1.1
License:	GPL-2.0-or-later
Group:		Productivity/Multimedia/Sound/Midi
Source: 	%{name}-%{version}.tar.gz
URL:		https://qsampler.sourceforge.io/
#Packager:	rncbc.org


%if 0%{?fedora_version} >= 34 || 0%{?suse_version} > 1500 || ( 0%{?sle_version} == 150200 && 0%{?is_opensuse} )
%define qt_major_version  6
%else
%define qt_major_version  5
%endif

BuildRequires:	coreutils
BuildRequires:	pkgconfig
BuildRequires:	glibc-devel
BuildRequires:	cmake >= 3.15
%if 0%{?sle_version} >= 150200 && 0%{?is_opensuse}
BuildRequires:	gcc10 >= 10
BuildRequires:	gcc10-c++ >= 10
%define _GCC	/usr/bin/gcc-10
%define _GXX	/usr/bin/g++-10
%else
BuildRequires:	gcc >= 10
BuildRequires:	gcc-c++ >= 10
%define _GCC	/usr/bin/gcc
%define _GXX	/usr/bin/g++
%endif
%if 0%{qt_major_version} == 6
%if 0%{?sle_version} == 150200 && 0%{?is_opensuse}
BuildRequires:	qtbase6.8-static >= 6.8
BuildRequires:	qttools6.8-static
BuildRequires:	qttranslations6.8-static
BuildRequires:	qtsvg6.8-static
%else
BuildRequires:	cmake(Qt6LinguistTools)
BuildRequires:	pkgconfig(Qt6Core)
BuildRequires:	pkgconfig(Qt6Gui)
BuildRequires:	pkgconfig(Qt6Widgets)
BuildRequires:	pkgconfig(Qt6Svg)
BuildRequires:	pkgconfig(Qt6Network)
%endif
%else
BuildRequires:	cmake(Qt5LinguistTools)
BuildRequires:	pkgconfig(Qt5Core)
BuildRequires:	pkgconfig(Qt5Gui)
BuildRequires:	pkgconfig(Qt5Widgets)
BuildRequires:	pkgconfig(Qt5Svg)
BuildRequires:	pkgconfig(Qt5Network)
%endif
BuildRequires:	liblscp-devel >= 0.5.6
BuildRequires:	libgig-devel >= 3.3.0

%description
Qsampler is a LinuxSampler GUI front-end application written in C++ around
the Qt framework using Qt Designer. For the moment it just wraps the client
interface of LinuxSampler Control Protocol (LSCP) (http://www.linuxsampler.org).


%prep
%setup -q

%build
%if 0%{?sle_version} == 150200 && 0%{?is_opensuse}
source /opt/qt6.8-static/bin/qt6.8-static-env.sh
%endif
CXX=%{_GXX} CC=%{_GCC} \
cmake -DCMAKE_INSTALL_PREFIX=%{_prefix} -Wno-dev -B build
cmake --build build %{?_smp_mflags}

%install
DESTDIR="%{buildroot}" \
cmake --install build


%files
%license LICENSE
%doc README TRANSLATORS ChangeLog
#dir %{_datadir}/mime
#dir %{_datadir}/mime/packages
#dir %{_datadir}/applications
%dir %{_datadir}/icons/hicolor
%dir %{_datadir}/icons/hicolor/32x32
%dir %{_datadir}/icons/hicolor/32x32/apps
%dir %{_datadir}/icons/hicolor/32x32/mimetypes
%dir %{_datadir}/icons/hicolor/scalable
%dir %{_datadir}/icons/hicolor/scalable/apps
%dir %{_datadir}/icons/hicolor/scalable/mimetypes
%dir %{_datadir}/%{name}
%dir %{_datadir}/%{name}/translations
%dir %{_datadir}/%{name}/palette
%dir %{_datadir}/metainfo
#dir %{_datadir}/man
#dir %{_datadir}/man/man1
#dir %{_datadir}/man/fr
#dir %{_datadir}/man/fr/man1
%{_bindir}/%{name}
%{_datadir}/mime/packages/org.rncbc.%{name}.xml
%{_datadir}/applications/org.rncbc.%{name}.desktop
%{_datadir}/icons/hicolor/32x32/apps/org.rncbc.%{name}.png
%{_datadir}/icons/hicolor/scalable/apps/org.rncbc.%{name}.svg
%{_datadir}/icons/hicolor/32x32/mimetypes/org.rncbc.%{name}.application-x-%{name}*.png
%{_datadir}/icons/hicolor/scalable/mimetypes/org.rncbc.%{name}.application-x-%{name}*.svg
%{_datadir}/%{name}/translations/%{name}_*.qm
%{_datadir}/metainfo/org.rncbc.%{name}.metainfo.xml
%{_datadir}/man/man1/%{name}.1.gz
%{_datadir}/man/fr/man1/%{name}.1.gz
%{_datadir}/%{name}/palette/*.conf


%changelog
* Wed Jun 19 2024 Rui Nuno Capela <rncbc@rncbc.org> 1.0.0
- An Unthinkable Release.
* Wed May  1 2024 Rui Nuno Capela <rncbc@rncbc.org> 0.9.91
- A Spring'24 Release Candidate 2.
* Wed Apr 10 2024 Rui Nuno Capela <rncbc@rncbc.org> 0.9.90
- A Spring'24 Release Candidate.
* Wed Jan 24 2024 Rui Nuno Capela <rncbc@rncbc.org> 0.9.12
- A Winter'24 Release.
* Sat Sep  9 2023 Rui Nuno Capela <rncbc@rncbc.org> 0.9.11
- An End-of-Summer'23 Release.
* Thu Jun  1 2023 Rui Nuno Capela <rncbc@rncbc.org> 0.9.10
- A Spring'23 Release.
* Thu Mar 23 2023 Rui Nuno Capela <rncbc@rncbc.org> 0.9.9
- An Early-Spring'23 Release.
* Wed Dec 28 2022 Rui Nuno Capela <rncbc@rncbc.org> 0.9.8
- An End-of-Year'22 Release.
* Mon Oct  3 2022 Rui Nuno Capela <rncbc@rncbc.org> 0.9.7
- An Early-Autumn'22 Release.
* Sat Apr  2 2022 Rui Nuno Capela <rncbc@rncbc.org> 0.9.6
- A Spring'22 Release.
* Sun Jan  9 2022 Rui Nuno Capela <rncbc@rncbc.org> 0.9.5
- A Winter'22 Release.
* Sun Jul  4 2021 Rui Nuno Capela <rncbc@rncbc.org> 0.9.4
- Early-Summer'21 release.
* Tue May 11 2021 Rui Nuno Capela <rncbc@rncbc.org> 0.9.3
- Spring'21 release.
* Sun Mar 14 2021 Rui Nuno Capela <rncbc@rncbc.org> 0.9.2
- End-of-Winter'21 release.
* Sun Feb  7 2021 Rui Nuno Capela <rncbc@rncbc.org> 0.9.1
- Winter'21 release.
* Thu Dec 17 2020 Rui Nuno Capela <rncbc@rncbc.org> 0.9.0
- Winter'20 release.
* Fri Jul 31 2020 Rui Nuno Capela <rncbc@rncbc.org> 0.6.3
- Summer'20 release.
* Tue Mar 24 2020 Rui Nuno Capela <rncbc@rncbc.org> 0.6.2
- Spring'20 release.
* Sun Dec 22 2019 Rui Nuno Capela <rncbc@rncbc.org> 0.6.1
- Winter'19 release.
* Thu Oct 17 2019 Rui Nuno Capela <rncbc@rncbc.org> 0.6.0
- Autumn'19 release.
* Fri Jul 12 2019 Rui Nuno Capela <rncbc@rncbc.org> 0.5.6
- Summer'19 release.
* Thu Apr 11 2019 Rui Nuno Capela <rncbc@rncbc.org> 0.5.5
- Spring-Break'19 release.
* Mon Mar 11 2019 Rui Nuno Capela <rncbc@rncbc.org> 0.5.4
- Pre-LAC2019 release frenzy.
* Thu Dec 06 2018 Rui Nuno Capela <rncbc@rncbc.org> 0.5.3
- An End of Autumn'18 Release.
* Sun Jul 22 2018 Rui Nuno Capela <rncbc@rncbc.org> 0.5.2
- Summer'18 Release.
* Mon May 21 2018 Rui Nuno Capela <rncbc@rncbc.org> 0.5.1
- Pre-LAC2018 release frenzy.
* Tue Dec 12 2017 Rui Nuno Capela <rncbc@rncbc.org> 0.5.0
- An Autumn'17 release.
* Thu Apr 27 2017 Rui Nuno Capela <rncbc@rncbc.org> 0.4.3
- Pre-LAC2017 release frenzy.
* Mon Nov 14 2016 Rui Nuno Capela <rncbc@rncbc.org> 0.4.2
- A Fall'16 release.
* Wed Sep 14 2016 Rui Nuno Capela <rncbc@rncbc.org> 0.4.1
- End of Summer'16 release.
* Tue Apr  5 2016 Rui Nuno Capela <rncbc@rncbc.org> 0.4.0
- Spring'16 release frenzy.
* Sun Jul 19 2015 Rui Nuno Capela <rncbc@rncbc.org> 0.3.1
- Summer'15 release frenzy.
* Wed Mar 25 2015 Rui Nuno Capela <rncbc@rncbc.org> 0.3.0
- Pre-LAC2015 release frenzy.
* Tue Dec 31 2013 Rui Nuno Capela <rncbc@rncbc.org> 0.2.3
- A fifth of a Jubilee release.
* Mon May 17 2010 Rui Nuno Capela <rncbc@rncbc.org>
- Standard desktop icon fixing. 
* Sat Aug  1 2009 Rui Nuno Capela <rncbc@rncbc.org> 0.2.2
- New 0.2.2 release.
* Thu Dec 6 2007 Rui Nuno Capela <rncbc@rncbc.org>
- Qt4 migration complete.
* Mon Jun 25 2007 Rui Nuno Capela <rncbc@rncbc.org>
- Application icon is now installed to (prefix)/share/pixmaps.
- Declared fundamental build and run-time requirements.
- Destination install directory prefix is now in spec.
- Spec is now a bit more openSUSE compliant.
* Mon Jan 15 2007 Rui Nuno Capela <rncbc@rncbc.org>
- Added sampler channel FX send support at session save code-level.
- Global sampler volume slider/spinbox combo is now featured.
* Sun Dec 17 2006 Rui Nuno Capela <rncbc@rncbc.org>
- Added preliminary MIDI instrument mapping support.
* Thu Jun 01 2006 Rui Nuno Capela <rncbc@rncbc.org>
- Take a chance for a new 0.1.3 release.
- Changed deprecated copyright attribute to license.
* Wed Aug 24 2005 Rui Nuno Capela <rncbc@rncbc.org>
- Prepared auto-generation from configure.
* Tue Aug 16 2005 Rui Nuno Capela <rncbc@rncbc.org>
- Get in sync with latest offerings from liblscp (0.3.1) and
  specially libgig (2.0.2) which broke previous ABI, somewhat.
* Thu Jun 23 2005 Rui Nuno Capela <rncbc@rncbc.org>
- Even minor workings needs a rest.
* Mon Jun 13 2005 Rui Nuno Capela <rncbc@rncbc.org>
- The mantra of bugfixes.
* Mon May 23 2005 Rui Nuno Capela <rncbc@rncbc.org>
- Device configuration breakthrough.
* Fri Mar 4 2005 Rui Nuno Capela <rncbc@rncbc.org>
- Fifth alpha-release.
* Tue Nov 16 2004 Rui Nuno Capela <rncbc@rncbc.org>
- Prepared for the fourth alpha release.
* Tue Nov 16 2004 Rui Nuno Capela <rncbc@rncbc.org>
- Prepared for the fourth alpha release.
* Wed Jun 2 2004 Rui Nuno Capela <rncbc@rncbc.org>
- Created initial qsampler.spec
