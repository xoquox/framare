Name: harbour-framare
Version: 0.1.0
Release: 4
Summary: Send photos to Frameo-compatible photo frames
License: GPL-3.0-or-later
URL: https://github.com/xoquox/framare
Source0: %{name}-%{version}.tar.bz2
Requires: sailfishsilica-qt5
Requires: sailfishshare-components
Requires: sailfish-components-pickers-qt5
Requires: qt5-plugin-imageformat-jpeg
BuildRequires: pkgconfig(sailfishapp)
BuildRequires: pkgconfig(Qt5Core)
BuildRequires: pkgconfig(Qt5Qml)
BuildRequires: pkgconfig(Qt5Quick)
BuildRequires: pkgconfig(Qt5DBus)
BuildRequires: pkgconfig(Qt5Concurrent)
BuildRequires: desktop-file-utils

%description
Native photo sender with frame management and gallery sharing.
Uses the independent unframeo interoperability client.

%prep
%setup -q

%build
%qmake5 "UNFRAMEO_BINARY=$PWD/.local/unframeo/%{_target_cpu}/unframeo"
%make_build

%install
%qmake5_install
desktop-file-validate %{buildroot}%{_datadir}/applications/%{name}.desktop

%files
%license LICENSE upstream/NOTICE.unframeo .local/unframeo/licenses/*
%{_bindir}/%{name}
%{_libexecdir}/%{name}/unframeo
%{_datadir}/%{name}
%{_datadir}/applications/%{name}.desktop
%{_datadir}/icons/hicolor/86x86/apps/%{name}.png
%{_datadir}/icons/hicolor/108x108/apps/%{name}.png
%{_datadir}/icons/hicolor/128x128/apps/%{name}.png
%{_datadir}/icons/hicolor/172x172/apps/%{name}.png
