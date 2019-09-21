Name: 		pioneers
Summary: 	Playable implementation of the Settlers of Catan 
Version: 	15.6
Release: 	1
Group: 		Amusements/Games
License: 	GPL
Url: 		http://pio.sourceforge.net/
Packager: 	The Pioneers developers <pio-develop@lists.sourceforge.net>
Source: 	http://downloads.sourceforge.net/pio/pioneers-15.6.tar.gz
BuildRoot: 	%{_tmppath}/%{name}-%{version}-%{release}-root
BuildRequires:  libgnome-devel, scrollkeeper
BuildRequires:	gtk2-devel >= 3.22
BuildRequires:	glib2-devel >= 2.26
Requires(post):	scrollkeeper

%description 
Pioneers is a computerized version of a well known strategy board game. The
goal of the game is to colonize an island. The players play the first
colonists, hence the name Pioneers.

This is the client software to play the game.

%package 	ai
Summary:	Pioneers AI Player
Group:		Amusements/Games

%description 	ai
Pioneers is a computerized version of a well known strategy board game. The
goal of the game is to colonize an island. The players play the first
colonists, hence the name Pioneers.

This package contains a computer player that can take part in Pioneers games.

%package 	server-console
Summary:	Pioneers Console Server
Group:		Amusements/Games
Requires:	pioneers-server-data

%description 	server-console
Pioneers is a computerized version of a well known strategy board game. The
goal of the game is to colonize an island. The players play the first
colonists, hence the name Pioneers.

The packages contains the server.

%package 	server-gtk
Summary:	Pioneers GTK Server
Group:		Amusements/Games
Requires:	pioneers, pioneers-server-data

%description 	server-gtk
Pioneers is a computerized version of a well known strategy board game. The
goal of the game is to colonize an island. The players play the first
colonists, hence the name Pioneers.

The server has a user interface in which you can customise the game
parameters.  Once you are happy with the game parameters, press the Start
Server button, and the server will start listening for client connections.

%package 	server-data
Summary: 	Pioneers Data
Group: 		Amusements/Games

%description 	server-data
Pioneers is a computerized version of a well known strategy board game. The
goal of the game is to colonize an island. The players play the first
colonists, hence the name Pioneers.

This package contains the data files for a game server.

%package 	metaserver
Summary:	Pioneers Metaserver
Group:		Amusements/Games

%description 	metaserver
Pioneers is a computerized version of a well known strategy board game. The
goal of the game is to colonize an island. The players play the first
colonists, hence the name Pioneers.

The metaserver registers available game servers and offers them to new
players. It can also create new servers on client request.

%package	editor
Summary:	Pioneers Game Editor
Group:		Amusements/Games
Requires:	pioneers, pioneers-server-data

%description	editor
Pioneers is a computerized version of a well known strategy board game. The
goal of the game is to colonize an island. The players play the first
colonists, hence the name Pioneers.

The game editor allows maps and game descriptions to be created and edited
graphically.

%prep
%setup -q

%build
%configure
make

%install
make install DESTDIR="%buildroot"
rm -rf %{buildroot}%{localstatedir}/scrollkeeper/
%find_lang %{name}

%clean
rm -rf %{buildroot}

%files -f %name.lang
%defattr(-,root,root)
%doc AUTHORS COPYING ChangeLog INSTALL README NEWS 
%doc %_mandir/man6/pioneers.6.gz
%{_bindir}/pioneers
%{_datadir}/applications/pioneers.desktop
%{_datadir}/pixmaps/pioneers.png
%{_datadir}/pixmaps/pioneers/*
%{_datadir}/games/pioneers/themes/*
%{_datadir}/gnome/help/pioneers/C/*.xml
%{_datadir}/gnome/help/pioneers/C/images/*
%{_datadir}/omf/pioneers/pioneers-C.omf

%post
scrollkeeper-update -q -o %{_datadir}/omf/pioneers

%postun
scrollkeeper-update -q

%files ai
%defattr(-,root,root)
%doc AUTHORS COPYING ChangeLog INSTALL README NEWS 
%doc %_mandir/man6/pioneersai.6.gz
%{_bindir}/pioneersai
%{_datadir}/games/pioneers/computer_names

%files server-console
%defattr(-,root,root)
%doc AUTHORS COPYING ChangeLog INSTALL README NEWS 
%doc %_mandir/man6/pioneers-server-console.6.gz
%{_bindir}/pioneers-server-console

%files server-gtk
%defattr(-,root,root)
%doc AUTHORS COPYING ChangeLog INSTALL README NEWS 
%doc %_mandir/man6/pioneers-server-gtk.6.gz
%{_bindir}/pioneers-server-gtk
%{_datadir}/pixmaps/pioneers-server.png
%{_datadir}/applications/pioneers-server.desktop

%files server-data
%defattr(-,root,root)
%doc AUTHORS COPYING ChangeLog INSTALL README NEWS 
%{_datadir}/games/pioneers/*.game

%files metaserver
%defattr(-,root,root)
%doc %_mandir/man6/pioneers-metaserver.6.gz
%doc AUTHORS COPYING ChangeLog INSTALL README NEWS 
%{_bindir}/pioneers-metaserver

%files editor
%defattr(-,root,root)
%doc AUTHORS COPYING ChangeLog INSTALL README NEWS 
%{_bindir}/pioneers-editor
%{_datadir}/pixmaps/pioneers-editor.png
%{_datadir}/applications/pioneers-editor.desktop

