FROM ubuntu:18.04

RUN apt-get update
RUN adduser build < /dev/null

RUN apt-get install -y wget tar unzip sudo gcc make

RUN wget -O /home/build/ogre.zip https://github.com/OGRECave/ogre/archive/v1.12.2.zip

RUN apt-get install -y intltool libgtk2.0-dev libgtk-3-dev libnotify-dev yelp-tools
RUN  apt-get install -y libavahi-client-dev libavahi-glib-dev gob2 librsvg2-bin
RUN  apt-get install -y libsdl2-dev

RUN wget -O /tmp/cmake.tgz https://github.com/Kitware/CMake/releases/download/v3.15.3/cmake-3.15.3-Linux-x86_64.tar.gz
RUN tar -x -C /opt/ -f /tmp/cmake.tgz && mv /opt/cmake-* /opt/cmake

RUN apt-get install -y libxaw7-dev libfreeimage-dev libpugixml-dev

WORKDIR /home/build
RUN sudo -i -u build unzip /home/build/ogre.zip

WORKDIR /

COPY patch-ogre-deps-linux.patch /patch-ogre-deps-linux.patch
RUN sudo -u build patch /home/build/ogre-1.12.2/CMake/Dependencies.cmake /patch-ogre-deps-linux.patch
RUN sudo -i -u build bash -c "cd /home/build/ogre-1.12.2 && \
        mkdir -p build && cd build && \
        /opt/cmake/bin/cmake .. \
                -DOpenGL_GL_PREFERENCE=GLVND \
                -DCMAKE_POSITION_INDEPENDENT_CODE=ON \
                -DCMAKE_BUILD_TYPE=Release \
                -DCMAKE_INSTALL_PREFIX=/home/build/libs/ogre"

COPY install-requirements.sh /
RUN bash /install-requirements.sh
RUN cp /home/build/ogre-1.12.2/build/Dependencies/lib/libzzip-0.so* /home/build/libs/ogre/lib/

RUN sudo -i -u build bash -c "cd /home/build/ogre-1.12.2/build && make install"

USER build
WORKDIR /home/build/