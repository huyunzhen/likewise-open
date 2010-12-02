#
# Copyright (c) Brian Koropoff
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#     * Redistributions of source code must retain the above copyright
#       notice, this list of conditions and the following disclaimer.
#     * Redistributions in binary form must reproduce the above copyright
#       notice, this list of conditions and the following disclaimer in the
#       documentation and/or other materials provided with the distribution.
#     * Neither the name of the MakeKit project nor the names of its
#       contributors may be used to endorse or promote products derived
#       from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR CONTRIBUTORS
# BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
# THE POSSIBILITY OF SUCH DAMAGE.
#

##
#
# package-rpm.sh -- build RPM packages
#
##

### section configure

DEPENDS="core program package"

option()
{
    mk_option \
        OPTION="package-rpm" \
        VAR="MK_PACKAGE_RPM" \
        PARAM="yes|no" \
        DEFAULT="yes" \
        HELP="Enable building RPM packages"

    mk_option \
        OPTION="rpm-dir" \
        VAR="MK_PACKAGE_RPM_DIR" \
        PARAM="path" \
        DEFAULT="$MK_PACKAGE_DIR/rpm" \
        HELP="Subdirectory for built RPM packages"
}

configure()
{
    mk_export MK_PACKAGE_RPM_DIR

    if mk_check_program PROGRAM=rpmbuild && [ "$MK_PACKAGE_RPM" = "yes" ]
    then
        mk_msg "RPM package building: enabled"
        mk_export MK_PACKAGE_RPM_ENABLED=yes
    else
        mk_msg "RPM package building: disabled"
        mk_export MK_PACKAGE_RPM_ENABLED=no
    fi
}

mk_rpm_enabled()
{
    [ "$MK_PACKAGE_RPM_ENABLED" = "yes" ]
}

mk_rpm_do()
{
    mk_push_vars PACKAGE SPECFILE VERSION
    mk_parse_params

    RPM_PKGDIR=".rpm-${PACKAGE}"
    RPM_PACKAGE="$PACKAGE"
    RPM_VERSION="$VERSION"
    RPM_SUBPACKAGES=""
    RPM_SUBPACKAGE=""
    RPM_DEPS=""

    mk_resolve_file "$RPM_PKGDIR"
    RPM_RES_PKGDIR="$result"
    mk_safe_rm "$RPM_RES_PKGDIR"

    for i in BUILD RPMS SOURCES SPECS SRPMS
    do
        mk_mkdir "$RPM_RES_PKGDIR/$i"
    done

    RPM_SPECFILE="${RPM_PKGDIR}/SPECS/${SPECFILE##*/}"
    RPM_RES_SPECFILE="${RPM_RES_PKGDIR}/SPECS/${SPECFILE##*/}"

    mk_output_file INPUT="$SPECFILE" OUTPUT="$RPM_SPECFILE"
    mk_quote "$result"
    RPM_DEPS="$RPM_DEPS $result"

    # Emit empty clean section to prevent staging directory
    # from being removed
    cat >>"${RPM_RES_SPECFILE}" <<EOF
%clean

EOF

    _mk_rpm_files_begin

    mk_subpackage_do()
    {
        mk_push_vars SUBPACKAGE
        mk_parse_params

        [ -z "$SUBPACKAGE" ] && SUBPACKAGE="$1"
        RPM_SUBPACKAGE="$SUBPACKAGE"
        RPM_SUBPACKAGES="$RPM_SUBPACKAGES $SUBPACKAGE"

        _mk_rpm_files_end
        _mk_rpm_files_begin "$SUBPACKAGE"

        mk_pop_vars
    }

    mk_subpackage_done()
    {
        unset RPM_SUBPACKAGE RPM_SUBINSTALLFILE RPM_SUBDIRFILE
    }

    mk_package_files()
    {
        for _i in "$@"
        do
            echo "$_i"
        done >> "${RPM_RES_SPECFILE}"
    }

    mk_package_dirs()
    {
        # RPM requires the directory
        # to actually exist beforehand.
        # Add a rule to create the directory and
        # add it to the dependency list
        for _i
        do
            mk_target \
                TARGET="$_i" \
                mk_mkdir "&$_i"

            mk_quote "$result"
            RPM_DEPS="$RPM_DEPS $result"
        done

        for _i in "$@"
        do
            echo "%dir $_i"
        done >> "${RPM_RES_SPECFILE}"
    }

    mk_pop_vars
}

mk_rpm_done()
{
    _mk_rpm_files_end

    mk_target \
        TARGET="@${MK_PACKAGE_RPM_DIR}/${RPM_PACKAGE}" \
        DEPS="$RPM_DEPS @all" \
        _mk_build_rpm "${RPM_PACKAGE}" "&${RPM_PKGDIR}" "&${RPM_SPECFILE}"
    master="$result"

    mk_add_phony_target "$master"
    mk_add_subdir_target "$master"

    unset RPM_PACKAGE RPM_SUBPACKAGE RPM_INSTALLFILE RPM_SUBINSTALLFILE RPM_PKGDIR
    unset RPM_SUBPACKAGES
    unset -f mk_package_files mk_package_dirs mk_subpackage_do mk_subpackage_done

    result="$master"
}

_mk_rpm_files_begin()
{
    {
        echo "%files $1"
        echo "%defattr(-,root,root)"
    } >> "${RPM_RES_SPECFILE}"
}

_mk_rpm_files_end()
{
    printf "\n" >> "${RPM_RES_SPECFILE}"
}

### section build

_mk_build_rpm()
{
    # $1 = package name
    # $2 = build directory
    # $3 = spec file

    mk_msg_domain "rpm"

    mk_safe_rm "${MK_PACKAGE_RPM_DIR}/$1"
    mk_mkdir "${MK_PACKAGE_RPM_DIR}/$1"
    mk_msg "begin $1"
    mk_run_quiet_or_fail ${RPMBUILD} \
        --define "_topdir ${MK_ROOT_DIR}/${2}" \
        --define "_unpackaged_files_terminate_build 0" \
        --define "_prefix ${MK_PREFIX}" \
        --define "_exec_prefix ${MK_EPREFIX}" \
        --define "_bindir ${MK_BINDIR}" \
        --define "_sbindir ${MK_SBINDIR}" \
        --define "_sysconfdir ${MK_SYSCONFDIR}" \
        --define "_datadir ${MK_DATADIR}" \
        --define "_includedir ${MK_INCLUDEDIR}" \
        --define "_libdir ${MK_LIBDIR}" \
        --define "_libexecdir ${MK_LIBEXECDIR}" \
        --define "_localstatedir ${MK_LOCALSTATEDIR}" \
        --define "_mandir ${MK_MANDIR}" \
        --buildroot="${MK_ROOT_DIR}/${MK_STAGE_DIR}" \
        -bb "$3"
    mk_run_or_fail mv "${2}/RPMS"/*/*.rpm "${MK_PACKAGE_RPM_DIR}/$1/."
    for i in "${MK_PACKAGE_RPM_DIR}/$1"/*.rpm
    do
        mk_msg "built ${i##*/}"
    done
    mk_msg "end $1"
}
