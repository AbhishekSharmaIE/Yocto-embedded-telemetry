SUMMARY = "Bootinfo HTTP diagnostics server"
DESCRIPTION = "Lightweight C HTTP server exposing Linux system diagnostics as JSON."
HOMEPAGE = "https://github.com/example/yocto-embedded-telemetry"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

SRC_URI = "file://bootinfo-server.c \
           file://bootinfo-server.init"

S = "${WORKDIR}"

inherit update-rc.d

INITSCRIPT_NAME = "bootinfo-server"
INITSCRIPT_PARAMS = "start 99 S . stop 10 0 6 ."

do_compile() {
	${CC} ${CFLAGS} ${LDFLAGS} ${S}/bootinfo-server.c -o ${S}/bootinfo-server
}

do_install() {
	install -d ${D}${bindir}
	install -m 0755 ${S}/bootinfo-server ${D}${bindir}/bootinfo-server

	install -d ${D}${sysconfdir}/init.d
	install -m 0755 ${S}/bootinfo-server.init \
		${D}${sysconfdir}/init.d/bootinfo-server
}

FILES:${PN} = "${bindir}/bootinfo-server \
               ${sysconfdir}/init.d/bootinfo-server"
