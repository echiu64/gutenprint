FROM fedora

RUN INSTALL_PKGS=" \
       which git tar wget hostname cups cups-devel automake libtool \
       autoconf gettext pkgconf-pkg-config gettext-devel \
       openjade doxygen \
       docbook-utils docbook-utils-pdf ImageMagick xml-common \
       libusb-devel gimp-devel make flex byacc zpaq time \
       valgrind texi2html file bzip2 xz" && \
     dnf install -y ${INSTALL_PKGS} && \
     rpm -V ${INSTALL_PKGS} && \
     dnf clean all && \
     mkdir -p /var/lib/origin

WORKDIR /var/tmp

COPY run-build /var/tmp

LABEL io.k8s.display-name="Gutenprint Base" \
      io.k8s.description="Gutenprint CI/CD Image" \
      io.openshift.tags="gutenprint-cicd" \
      io.podman.tags="gutenprint-cicd"

ENTRYPOINT ["/var/tmp/run-build"]
