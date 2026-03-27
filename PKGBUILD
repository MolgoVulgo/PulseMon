pkgname=pulsemon-api
pkgver=0.1.0
pkgrel=1
pkgdesc="Local Linux metrics API for PulseMon (FastAPI)"
arch=("x86_64" "aarch64")
url="https://github.com/MolgoVulgo/PulseMon"
license=("custom")
depends=(
  "python"
  "python-fastapi"
  "python-pydantic"
  "python-psutil"
  "uvicorn"
)
optdepends=(
  "python-httpx: diagnostics scripts"
)
backup=("etc/pulsemon/pulsemon-api.conf")
install="${pkgname}.install"
source=(
  "pulsemon-api.sh"
  "pulsemon-api.service"
  "pulsemon-api.conf"
)
sha256sums=(
  "SKIP"
  "SKIP"
  "SKIP"
)

package() {
  local _project_root="${startdir}"
  if [[ "${_project_root}" != /tmp/* ]]; then
    echo "error: this PKGBUILD must be built from /tmp (current startdir: ${_project_root})" >&2
    echo "hint: use ./makepkg-tmp.sh -si" >&2
    return 1
  fi

  if [[ ! -d "${_project_root}/api/app" ]]; then
    echo "error: api/app not found; run makepkg from the repository root." >&2
    return 1
  fi

  install -dm755 "${pkgdir}/usr/lib/pulsemon-api"
  cp -a "${_project_root}/api/app" "${pkgdir}/usr/lib/pulsemon-api/"

  install -Dm755 "${srcdir}/pulsemon-api.sh" "${pkgdir}/usr/bin/pulsemon-api"
  install -Dm644 "${srcdir}/pulsemon-api.service" "${pkgdir}/usr/lib/systemd/system/pulsemon-api.service"
  install -Dm644 "${srcdir}/pulsemon-api.conf" "${pkgdir}/etc/pulsemon/pulsemon-api.conf"
  install -Dm644 "${_project_root}/api/README.md" "${pkgdir}/usr/share/doc/${pkgname}/README.md"
}
