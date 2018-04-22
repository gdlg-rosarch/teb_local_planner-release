# Script generated with Bloom
pkgdesc="ROS - The teb_local_planner package implements a plugin to the base_local_planner of the 2D navigation stack. The underlying method called Timed Elastic Band locally optimizes the robot's trajectory with respect to trajectory execution time, separation from obstacles and compliance with kinodynamic constraints at runtime."
url='http://wiki.ros.org/teb_local_planner'

pkgname='ros-lunar-teb-local-planner'
pkgver='0.7.0_1'
pkgrel=1
arch=('any')
license=('BSD'
)

makedepends=('ros-lunar-base-local-planner'
'ros-lunar-catkin'
'ros-lunar-cmake-modules'
'ros-lunar-costmap-2d'
'ros-lunar-costmap-converter'
'ros-lunar-dynamic-reconfigure'
'ros-lunar-geometry-msgs'
'ros-lunar-interactive-markers'
'ros-lunar-libg2o'
'ros-lunar-message-generation'
'ros-lunar-nav-core'
'ros-lunar-nav-msgs'
'ros-lunar-pluginlib'
'ros-lunar-roscpp'
'ros-lunar-std-msgs'
'ros-lunar-tf'
'ros-lunar-tf-conversions'
'ros-lunar-visualization-msgs'
)

depends=('ros-lunar-base-local-planner'
'ros-lunar-costmap-2d'
'ros-lunar-costmap-converter'
'ros-lunar-dynamic-reconfigure'
'ros-lunar-geometry-msgs'
'ros-lunar-interactive-markers'
'ros-lunar-libg2o'
'ros-lunar-message-runtime'
'ros-lunar-nav-core'
'ros-lunar-nav-msgs'
'ros-lunar-pluginlib'
'ros-lunar-roscpp'
'ros-lunar-std-msgs'
'ros-lunar-tf'
'ros-lunar-tf-conversions'
'ros-lunar-visualization-msgs'
)

conflicts=()
replaces=()

_dir=teb_local_planner
source=()
md5sums=()

prepare() {
    cp -R $startdir/teb_local_planner $srcdir/teb_local_planner
}

build() {
  # Use ROS environment variables
  source /usr/share/ros-build-tools/clear-ros-env.sh
  [ -f /opt/ros/lunar/setup.bash ] && source /opt/ros/lunar/setup.bash

  # Create build directory
  [ -d ${srcdir}/build ] || mkdir ${srcdir}/build
  cd ${srcdir}/build

  # Fix Python2/Python3 conflicts
  /usr/share/ros-build-tools/fix-python-scripts.sh -v 2 ${srcdir}/${_dir}

  # Build project
  cmake ${srcdir}/${_dir} \
        -DCMAKE_BUILD_TYPE=Release \
        -DCATKIN_BUILD_BINARY_PACKAGE=ON \
        -DCMAKE_INSTALL_PREFIX=/opt/ros/lunar \
        -DPYTHON_EXECUTABLE=/usr/bin/python2 \
        -DPYTHON_INCLUDE_DIR=/usr/include/python2.7 \
        -DPYTHON_LIBRARY=/usr/lib/libpython2.7.so \
        -DPYTHON_BASENAME=-python2.7 \
        -DSETUPTOOLS_DEB_LAYOUT=OFF
  make
}

package() {
  cd "${srcdir}/build"
  make DESTDIR="${pkgdir}/" install
}

