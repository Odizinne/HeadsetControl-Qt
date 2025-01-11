_pkgname="headsetcontrol-qt"
pkgname="${_pkgname}-git"
pkgver=v24.r1.g9dd20a1
pkgrel=1
pkgdesc="Qt GUI for headsetcontrol"
arch=('x86_64')
url='https://github.com/Odizinne/HeadsetControl-Qt.git'
license=('GPL-3.0-or-later')
makedepends=('git' 'qt6-tools')
provides=("${_pkgname}=${pkgver}")
depends=('qt6-base' 'headsetcontrol')
source=("git+${url}#branch=main")
sha256sums=('SKIP')

pkgver() {
  cd "HeadsetControl-Qt"
  git describe --long --tags --abbrev=7 | sed 's/\([^-]*-g\)/r\1/;s/-/./g'
}

prepare() {
  mkdir -p "HeadsetControl-Qt/build"
}

build() {
  cd "HeadsetControl-Qt/build"
  qmake6 ..
  make
  mv HeadsetControl-Qt ${_pkgname} # rename package binary to package name
}

package() {
  install -Dm755 "${srcdir}/HeadsetControl-Qt/build/${_pkgname}" "${pkgdir}/usr/bin/${_pkgname}"
}
