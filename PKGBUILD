# Maintainer: CobbCoding 
pkgname='cano'
pkgver='r91.83ca80e'
pkgrel=1
pkgdesc="Terminal-based modal text editor"
arch=('x86_64')
url="https://github.com/CobbCoding1/cano"
license=('Apache 2.0')
makedepends=('git' 'make' 'ncurses' 'gcc')
source=("$pkgname::git+https://github.com/CobbCoding1/$pkgname.git")
md5sums=('SKIP')

pkgver() {
    cd "$pkgname"
    printf "r%s.%s" "$(git rev-list --count HEAD)" "$(git rev-parse --short HEAD)"
}

build() {
	cd "$pkgname"
	make -B
}

package() {
	cd "$pkgname"
    install -Dm755 ./build/"$pkgname" "$pkgdir/usr/bin/$pkgname"
    install -Dm755 ./README.md "$pkgdir/usr/share/doc/$pkgname"
}
